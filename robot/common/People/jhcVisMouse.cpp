// jhcVisMouse.cpp : tracks user's arm and hand to simulate a computer mouse
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2013 IBM Corporation
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

#include "Interface/jhcMessage.h"

#include "People/jhcVisMouse.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcVisMouse::jhcVisMouse ()
{
  Defaults();
  Reset();
}


//= Reset state for the beginning of a sequence.

void jhcVisMouse::Reset ()
{
  // positions
  ix = 0;
  iy = 0;
  mx = 0;
  my = 0;
  cx = 0;
  cy = 0;
  px = 0;
  py = 0;

  // timing
  first = 1;
  bad = 0;
  cnt = 0;
  bored = 0;
}


//= Set sizes of internal images based on a reference image.

void jhcVisMouse::SetSize (jhcImg& ref)
{
  SetSize(ref.XDim(), ref.YDim());
}


//= Set sizes of internal images directly.

void jhcVisMouse::SetSize (int x, int y)
{
  ref.SetSize(x, y, 3);
  diff.SetSize(x, y, 1);
  bin.SetSize(diff);
  big.SetSize(diff);
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcVisMouse::Defaults (const char *fname)
{
  int ok = 1;

  ok &= mouse_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcVisMouse::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= mps.SaveVals(fname);
  return ok;
}


//= Parameters used for something.

int jhcVisMouse::mouse_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("vis_gest", 0);
  ps->NextSpec4( &th,      50, "Difference threshold");  
  ps->NextSpec4( &sc,       9, "Binary smoothing");  
  ps->NextSpec4( &amin,    50, "Min expected arm area");  
  ps->NextSpec4( &amax, 20000, "Max expected arm area");  
  ps->NextSpec4( &clr,     10, "Wait for empty scene");  
  ps->NextSpec4( &wait,     5, "Wait for click signal");  

  ps->NextSpec4( &same,   100, "Variation in static scene");  // was 50
  ps->NextSpec4( &stale,   45, "Wait for static reset");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Tracks human hand and treats like a mouse on the input image.
// return: -1 = acquired new background, 0 = tracking, 1 = first stable (click)
// Note: needs unenhanced image to perform differencing properly

int jhcVisMouse::Pointing (int& x, int& y, const jhcImg& src)
{
  int pa = a, click = 0, mv = 2;

  // set up default position and check if first image
  x = 0;
  y = 0;
  if (first > 0)
    return pointing_ref(src);

  // find difference from background
  WtdSAD_RGB(diff, src, ref);
  Threshold(bin, diff, th);
  BoxThresh(big, bin, sc);      
  a = CountOver(big);

  // check if static scene change (e.g. large object shifted)
  if ((a < amin) || (abs(a - pa) > same))
    bored = 0;
  else 
  {
    bored++;
    if (bored == stale)
      return pointing_ref(src);
  }

  // check if hand present
  if (a < amax)
    bad = 0;                        
  else 
  {
    bad++;
    if (bad == clr)
      return pointing_ref(src);
  }

  // track max extension relative to SW corner
  PtMaxY(ix, iy, big);
  if ((ix + iy) >= (mx + my))
  {
    mx = ix;
    my = iy;
  }

  // generate click when movement stops
  if (a < amin)
  {
    mx = 0;
    my = 0;
    cnt = 0;
  }
  else if ((abs(px - mx) > mv) || (abs(py - my) > mv))
    cnt = 0;
  else if (cnt++ == wait)
  {
    cx = mx;
    cy = my;
    click = 1;
  }
  px = mx;
  py = my;

  // return
  x = cx;
  y = cy;
  return click;
}


//= Save image as reference and reset state.
// always returns -1 for convenience

int jhcVisMouse::pointing_ref (const jhcImg& src)
{
  Reset();
  ref.CopyArr(src);
  first = 0;
  return -1;
}



