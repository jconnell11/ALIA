// jhcVidFlex.cpp : utilities for adapting to video sizes and rates
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2004-2011 IBM Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// 
///////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include "Interface/jhcMessage.h"

#include "Processing/jhcVidFlex.h"


///////////////////////////////////////////////////////////////////////////
//                        Creation and Destruction                       //
///////////////////////////////////////////////////////////////////////////

//= Default constructor sets up initial values.

jhcVidFlex::jhcVidFlex ()
{
  rw = 320;
  rh = 240;
  rrate = 30.0;
  rsize = 1.0;
  FlexSize(320, 240, 30.0);
  FlexScale(1.0);
}

 
//= Utility to only print messages when "noisy" variable is positive.

void jhcVidFlex::Report (const char *msg, ...) const
{
  va_list args;
  char val[1000];

  if (noisy <= 0)
    return;
  va_start(args, msg); 
  vsprintf(val, msg, args);
  Pause(val);
}


//= Utility to only print messages when "noisy" variable at or above level.

void jhcVidFlex::Report (int level, const char *msg, ...) const
{
  va_list args;
  char val[1000];

  if (noisy < level)
    return;
  va_start(args, msg); 
  vsprintf(val, msg, args);
  Pause(val);
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Set the size of the canonical reference image (and maybe frame rate).

void jhcVidFlex::FlexRef (int w, int h, double fps, double size)
{
  rw = __max(1, w);
  rh = __max(1, h);
  rrate = __max(0.001, fps);
  rsize = __max(0.001, size);
  adj_space();
  adj_time();
}


//= Set the size of the current video frame (and maybe frame rate).

void jhcVidFlex::FlexSize (int w, int h, double fps)
{
  vw = __max(1, w);
  vh = __max(1, h);
  adj_space();
  if (fps > 0.0)
    FlexRate(fps);
}


//= Set the size of the current video frame using a sample image.

void jhcVidFlex::FlexSize (const jhcImg& ref, double fps)
{
  FlexSize(ref.XDim(), ref.YDim(), fps);
}


//= Set the current video frame rate directly.

void jhcVidFlex::FlexRate (double fps)
{
  vrate = __max(0.001, fps);
  adj_time();
}


//= Sets an additional scaling factor based on detected feature size.

void jhcVidFlex::FlexScale (double size)
{
  vsize = __max(0.001, size);
  adj_feat();
}


//= Adjust pre-computed spatial factors.

void jhcVidFlex::adj_space ()
{
  hf  = vh / (double) rh;
  ihf = rh / (double) vh;
  af = (vw * vh) / (double)(rw * rh);
  adj_feat();
}


//= Adjust pre-computed temporal factors.

void jhcVidFlex::adj_time ()
{
  tf  = vrate / rrate;
  itf = rrate / vrate;
}


//= Adjust pre-computed feature scaling factors.

void jhcVidFlex::adj_feat ()
{
  sc = vsize / rsize;
  hf2 = sc * hf;
  af2 = sc * sc * af;
}


///////////////////////////////////////////////////////////////////////////
//                            Resizing Utilities                         //
///////////////////////////////////////////////////////////////////////////

//= Mask scale.
// adjust mask spatial extent for canonical image to fit current size
// always returns an odd number greater than or equal to 1

int jhcVidFlex::msc (int dim) const
{
  return(2 * ROUND((dim - 1) * hf) + 1);
}


//= Pixel Scale.
// adjust pixel difference for canonical image to fit current size

int jhcVidFlex::psc (int pels) const
{
  return(ROUND(pels * hf));
}


//= Area Scale.
// adjusts area threshold based on value for canonical image

int jhcVidFlex::asc (int area) const
{
  return((int)(area * af + 0.5));
}


//= Mask Feature Scale.
// adjust mask spatial extent for canonical image to fit current size
// also scales for detected feature scale relative to reference scale
// always returns an odd number greater than or equal to 1

int jhcVidFlex::mfsc (int dim) const
{
  return(2 * ROUND((dim - 1) * hf2) + 1);
}


//= Pixel Feature Scale.
// also scales for detected feature scale relative to reference scale
// adjust pixel difference for canonical image to fit current size

int jhcVidFlex::pfsc (int pels) const
{
  return(ROUND(pels * hf2));
}


//= Area Feature Scale.
// also scales for detected feature scale relative to reference scale
// adjusts area threshold based on value for canonical image

int jhcVidFlex::afsc (int area) const
{
  return((int)(area * af2 + 0.5));
}


//= Time Scale.
// adjusts a temporal delay relative to canonical frames per second

int jhcVidFlex::tsc (int frames) const
{
  return ROUND(frames * tf);
}


//= Decay Scale.
// adjusts an IIR decay constant relative to canonical video rate

double jhcVidFlex::dsc (double decay) const
{
  return pow(decay, itf);
}


//= Blending Scale.
// adjusts a blending update factor relative to canonical 30 fps video

double jhcVidFlex::bsc (double frac) const
{
  return(frac * itf);
}


//= Adjusts a pixel velocity for frame size and video rate.

double jhcVidFlex::vsc (double vel) const
{
  return(vel * hf * itf);
}



