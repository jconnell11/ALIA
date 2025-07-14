// jhcFilter.h : cleans up noisy video images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2003-2013 IBM Corporation
// Copyright 2024 Etaoin Systems
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

#pragma once

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Cleans up noisy video images.

class jhcFilter_0
{
// PUBLIC MEMBER VARIABLE
public:
  jhcImg est;    /** Image holding averages (i.e. result of smoothing). */
  jhcImg var;    /** Image holding the estimated variances.             */
  double nv[3];  /** Channel pixel noise estimates (default = 8^2).     */
  int first;     /** Whether this is the first input image.             */
  double f0;     /** Bias toward observed variance (default = 0.1).     */


// PUBLIC MEMBER FUNCTIONS
public:
  jhcFilter_0 ();

  void Reset();
  void SetSize (const jhcImg& ref);
  int Flywheel (const jhcImg& src, int init =0);


// PRIVATE MEMBER FUNCTIONS
private:
  int fly_mono (const jhcImg& src, int init);
  int fly_float (const jhcImg& src);
};


/////////////////////////////////////////////////////////////////////////////

//= Allows transparent insertion of MMX class over top of normal class.

#ifdef JHC_MMX
#include "MMX/jhcFilter_mmx.h"
#else
typedef jhcFilter_0 jhcFilter;
#endif





