// jhcALU.h : computes functions of two arrays of pixels
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
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

#ifndef _JHCALU_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALU_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Computes functions of two arrays of pixels.
// these are like the old DataCube MaxVideo ALU functions

class jhcALU_0
{
// PRIVATE MEMBER VARIABLES
private:
  static int instances;
  static UL32 *recip;


// PUBLIC MEMBER FUNCTIONS
public:
  jhcALU_0 ();
  ~jhcALU_0();

  // simple differences
  int ClipDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int LiftDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =0.5) const;
  int Decrement (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =0.5) const;
  int CycDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int CycDist (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =2.0) const;
  int AbsDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int AbsThresh (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, int th = 128) const;
  int AbsDiffRGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, 
                  double rsc, double gsc, double bsc) const;
  int WtdSAD_RGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, 
                  double rsc =0.33, double gsc =0.33, double bsc =0.33) const;
  int WtdSSD_RGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, 
                  double rsc =0.33, double gsc =0.33, double bsc =0.33) const;
  int WtdRMS_RGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, 
                  double rsc =0.33, double gsc =0.33, double bsc =0.33) const;

  // relative differences
  int RelBoost (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int RelDrop (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int RelDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int FracBoost (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int FracDrop (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int FracDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
 
  // statistical comparisons
  int MaxFcn (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int MinFcn (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int MaxComp2 (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int MinComp2 (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int NZMin (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int NZAvg (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int XorFcn (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int MaxWithin (jhcImg& dest, const jhcImg& src, int th =0) const;
  int NumOver (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, int ath =128, int bth =128);

  // mixing functions
  int ClipSum (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int AvgFcn (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int Blend  (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double afrac, double sc =1.0) const;
  int StepToward (jhcImg& dest, const jhcImg& goal, const jhcImg& src, int lim =10) const;
  int MixToward (jhcImg& dest, const jhcImg& goal, const jhcImg& src, double f, int always =0) const;
  int MixUnder (jhcImg& dest, const jhcImg& goal, const jhcImg& src, 
                const jhcImg& gate, int th =128, double f =0.1, int always =1) const;
  int Magnitude (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =0.7071) const;
  
  // signal processing
  int NormBy (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int CenterNorm (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0, int dmin =1) const;
  int AbsRatio (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc =1.0) const;
  int Mult (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int MultMid (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const;
  int MultRGB (jhcImg& dest, const jhcImg& src, const jhcImg& fact) const;

};


#endif


/////////////////////////////////////////////////////////////////////////////

//= Allows transparent insertion of MMX class over top of normal class.

#ifdef JHC_MMX
#include "MMX/jhcALU_mmx.h"
#else
typedef jhcALU_0 jhcALU;
#endif

