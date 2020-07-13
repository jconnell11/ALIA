// jhcKnob.cpp : slowly varying control value
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2001-2004 IBM Corporation
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

#include <stdlib.h>       // needed for __max and __min
#include <math.h>         // needed for fabs

#include "Data/jhcKnob.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor.

jhcKnob::~jhcKnob ()
{
}


//= Default constructor.

jhcKnob::jhcKnob ()
{
  DefLims(0, 0, 0);
  FracMove(0.1, 0, 0);
  Reset(0);
}


//= Specify basic values: default and range limits.
// if hi less than or same as lo then no range enforced

void jhcKnob::DefLims (int start, int lo, int hi)
{
  vdef = start;
  vmin = lo;
  vmax = hi;
}


//= Define default value and limits as fractions of some range.
// if hi less than or same as lo then no range enforced

void jhcKnob::DefLimsF (int rng, double start, double lo, double hi)
{
  DefLims((int)(start * rng + 0.5), (int)(lo * rng + 0.5), (int)(hi * rng + 0.5));
}


//= Specify how to change value.
// fraction of difference to move, minimum motion (for integers), 
// and deadband size (half width)

void jhcKnob::FracMove (double dmix, int umin, int dead)
{
  frac = dmix;
  move = umin;
  tol  = dead;
}


///////////////////////////////////////////////////////////////////////////
//                            Basic Operations                           //
///////////////////////////////////////////////////////////////////////////

//= Set exactly to default value.
// if init is non-zero, first update will force value

double jhcKnob::Reset (int init)
{
  first = init;
  val = (double) vdef; 
  return val;
}   


//= Force a new value directly (within limits if any).
// prevents future updates from directly forcing

double jhcKnob::Force (double target)
{
  first = 0;
  if (vmax > vmin) 
    val = __max(vmin, __min(target, vmax));
  else 
    val = target;
  return val;
}


//= Make value be at least as high as low.

double jhcKnob::AtLeast (double low)
{
  if (val < low)
    val = low;
  return val;
}


//= Make value be no higher than high.

double jhcKnob::NoMore (double high)
{
  if (val > high)
    val = high;
  return val;
}


//= Move toward new value by fraction of difference.
// can use different gains depending on direction from current
// can guarantee a move of at least "move"
// enforces range limits (if any), forces if "first" is non-zero

double jhcKnob::Update (double target)
{
  double d = target - val, fd = frac * d;

  if (first > 0)                      // initial direct copy
    return Force(target);
  if ((tol < 0) && (fabs(d) > -tol))  // copy big changes, smooth small ones
    return Force(target);
  if ((tol > 0) && (fabs(d) <= tol))  // no update needed
    return val;
  if (move <= 0)                      // no minimum spec'd
    return Force(val + fd);
  if (d >= 0)
    return Force(val + __max(__min(d, move), fd));
  return Force(val - __max(__min(-d, move), -fd));
}


//= Update to a value that is factor times the current value.

double jhcKnob::Scale (double factor)
{
  return Update(factor * val);
}


//= Move back toward default value.

double jhcKnob::Decay ()
{
  return Update((double) vdef);
}



