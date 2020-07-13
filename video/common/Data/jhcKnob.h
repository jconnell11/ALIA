// jhcKnob.h : smoothly varying control parameter
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

#ifndef _JHCKNOB_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCKNOB_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Smoothly varying control parameter.
// can enforce range limits, do IIR update and decay
// setup primarily for integer values

class jhcKnob
{
public:
  double val;   /** The current value.                                   */
  double frac;  /** Fractional amount to adjust value (default = 0.1).   */ 
  int vdef;     /** The starting value (default = 0).                    */
  int vmin;     /** The minimum allowed value (ignore if = vmax).        */
  int vmax;     /** The maximum allowed value (ignore if = vmin).        */
  int tol;      /** Size of change to ignore or copy if over (negative). */
  int move;     /** All adjustments must be at least this big.           */
  int first;    /** Whether the first value should be copied directly.   */

public:
  ~jhcKnob ();
  jhcKnob ();

  void DefLims (int start, int lo =0, int hi =0);
  void DefLimsF (int rng, double start, double lo =0.0, double hi =0.0);
  void FracMove (double dmix, int umin =0, int dead =0);

  int Ival (double f =1.0) {return (int)(f * val + 0.5);}       /** Return (scaled) integer value. */
  double Recip () {return ((val == 0.0) ? 0.0 : (1.0 / val));}  /** Return 1.0 / val reciprocal.   */

  double Reset (int init =0);
  double Force (double target);
  double AtLeast (double low);
  double NoMore (double high);
  double Update (double target);
  double Scale (double factor);
  double Decay ();
};


#endif




