// jhcRuns.h : processes images using four directions of run lengths
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#ifndef _JHCRUNS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCRUNS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include <stdlib.h>

#include "Data/jhcImg.h"


//= Processes images using four directions of run lengths.
// spun-off from jhcArea

class jhcRuns
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg a1, b1, c1, d1;


// PUBLIC MEMBER FUNCTIONS
public:
  // run lengths
  int RunsH (jhcImg& dest, const jhcImg& src, double sc =1.0, int bdok =1) const;
  int RunsV (jhcImg& dest, const jhcImg& src, double sc =1.0, int bdok =1) const;
  int RunsD1 (jhcImg& dest, const jhcImg& src, double sc =1.0, int bdok =1) const;
  int RunsD2 (jhcImg& dest, const jhcImg& src, double sc =1.0, int bdok =1) const;

  // shape properties
  int MinRun (jhcImg& dest, const jhcImg& src, double sc =1.0);
  int MinDir (jhcImg& dest, const jhcImg& src, double sc =1.0);
  int AdjMin (jhcImg& dest, const jhcImg& src, double sc =1.0);
  int OrthoMin (jhcImg& dest, const jhcImg& src, double sc =1.0);
  int AdjAvg (jhcImg& dest, const jhcImg& src, double sc =1.0);
  int OrthoAvg (jhcImg& dest, const jhcImg& src, double sc =1.0);

  // region bulking
  int Convexify (jhcImg& dest, const jhcImg& src, int maxgap =0);
  int ConvexAll (jhcImg& dest, const jhcImg& src, int maxgap =0);
  int ConvexClaim (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int maxgap =0);
  int ConvexH (jhcImg& dest, const jhcImg& src, int maxgap =0, int fill =255);
  int ConvexUp (jhcImg& dest, const jhcImg& src, int maxgap =0, int fill =255);

  // edge filling
  int StripOutside (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun =0, int bdok =0);
  int InsideRuns (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun =0, int bdok =0, int cnt =0);
  int KeepSpanH (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun =0, int bdok =0) const;
  int KeepSpanV (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun =0, int bdok =0) const;
  int KeepSpanD1 (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun =0, int bdok =0) const;
  int KeepSpanD2 (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun =0, int bdok =0) const;

  // region claiming
  int BorderDist (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok =0);
  int ExtendH (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok =0) const;
  int ExtendV (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok =0) const;
  int ExtendD1 (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok =0) const;
  int ExtendD2 (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok =0) const;

  // gap interpolation
  int MinRamp (jhcImg& dest, const jhcImg& src, int maxgap =5, int diag =1);
  int AvgRamp (jhcImg& dest, const jhcImg& src, int maxgap =5, int diag =1);
  int RampH (jhcImg& dest, const jhcImg& src, int maxgap =5) const;
  int RampV (jhcImg& dest, const jhcImg& src, int maxgap =5) const;
  int RampD1 (jhcImg& dest, const jhcImg& src, int maxgap =5) const;
  int RampD2 (jhcImg& dest, const jhcImg& src, int maxgap =5) const;

  // gap filling
  int LowestAll (jhcImg& dest, const jhcImg& src, int maxgap =5, int diag =1);
  int LowestH (jhcImg& dest, const jhcImg& src, int maxgap =5) const;
  int LowestV (jhcImg& dest, const jhcImg& src, int maxgap =5) const;
  int LowestD1 (jhcImg& dest, const jhcImg& src, int maxgap =5) const;
  int LowestD2 (jhcImg& dest, const jhcImg& src, int maxgap =5) const;

  // non-maximum suppression
  int InflectH (jhcImg& dest, const jhcImg& src, int th =15) const;
  int InflectV (jhcImg& dest, const jhcImg& src, int th =15) const;
  int InflectD1 (jhcImg& dest, const jhcImg& src, int th =15) const;
  int InflectD2 (jhcImg& dest, const jhcImg& src, int th =15) const;

  // well formed-ness
  int StopAt (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const;
  int StopAtH (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const;
  int StopAtV (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const;
  int StopAtD1 (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const;
  int StopAtD2 (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const;

  // object bottoms
  int SmallGapH (jhcImg& dest, const jhcImg& src, int wmax) const;


// PRIVATE MEMBER FUNCTIONS
private:
  void nzm (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  void min_mark (jhcImg& mark, jhcImg& narr, const jhcImg& wid, int val) const;
  void inv (jhcImg& dest, const jhcImg& src) const;
  void cutoff (jhcImg& dest, int maxval) const;
  void nzor (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  void thresh (jhcImg& dest, const jhcImg& src, int th, int over, int under) const;
  void th_sum (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, int bth, int mark) const;
  void nz_avg (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;

};


#endif




