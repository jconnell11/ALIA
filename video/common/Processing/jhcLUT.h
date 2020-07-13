// jhcLUT.h : straight pixel to pixel transforms on a single image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
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

#ifndef _JHCLUT_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCLUT_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Straight pixel to pixel transforms on a single image.

class jhcLUT_0
{
// PRIVATE MEMBER VARIABLES
private:
  static int instances;
  static UC8 *lgt;


// PROTECTED MEMBER VARIABLES
protected:
  jhcImg tmp;


// PUBLIC MEMBER FUNCTIONS
public:
  jhcLUT_0();
  ~jhcLUT_0();

  // multiplication etc.
  int PumpUp (jhcImg& dest, const jhcImg& src) const;
  int ClipScale (jhcImg& dest, const jhcImg& src, double sc) const;
  int CenterScale (jhcImg& dest, const jhcImg& src, double sc) const;
  int AdjustRGB (jhcImg& dest, const jhcImg& src, double rf, double gf, double bf) const;
  int MapVals (jhcImg& dest, const jhcImg& src, const UC8 map[]) const;
  int Square (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int AbsVal (jhcImg& dest, const jhcImg& src) const;
  int MatchVal (jhcImg& dest, const jhcImg& src, int lvl, double sc =1.0) const;
  int Log (jhcImg& dest, const jhcImg& src, int mid =80) const;
  int Gamma (jhcImg& dest, const jhcImg& src, int mid =80) const;
  int Sigmoid (jhcImg& dest, const jhcImg& src, int mid =80) const;
  int Logify (jhcImg& dest, const jhcImg& src) const;
  int Shift16 (jhcImg& dest, const jhcImg& src, int rt, int bits =16, int off =0) const;
  int BitMask (jhcImg& dest, const jhcImg& src, int bits) const;
  int Stretch (jhcImg& dest, const jhcImg& src, int lo, int hi) const 
    {return Linear(dest, src, lo, (hi - lo) / 255.0);}
  int Linear (jhcImg& dest, const jhcImg& src, int off, double sc) const;

  // simple value alteration
  int Complement (jhcImg& dest, const jhcImg& src) const;
  int LimitMax (jhcImg& dest, const jhcImg& src, int val) const;
  int LimitMin (jhcImg& dest, const jhcImg& src, int val) const;
  int LimitRng (jhcImg& dest, const jhcImg& src, int lo, int hi) const;
  int LimitRGB (jhcImg& dest, const jhcImg& src, int rval, int gval, int bval) const;
  int Offset (jhcImg& dest, const jhcImg& src, int val =1) const;
  int OffsetRGB (jhcImg& dest, const jhcImg& src, int rval, int gval, int bval) const;
  int CycOffset (jhcImg& dest, const jhcImg& src, int val =64) const;
  int AndVal (jhcImg& dest, const jhcImg& src, int val =0x7F) const;
  int OrVal (jhcImg& dest, const jhcImg& src, int val =0x01) const;
  int IncOver (jhcImg& dest, const jhcImg& gate, int val, int th =0) const;
  int IncUnder (jhcImg& dest, const jhcImg& gate, int val, int th=255) const;
  int Replace (jhcImg& dest, int targ, int subst =255) const;

  // Kinect depth images
  int Night8 (jhcImg& d8, const jhcImg& d16, int sh =0) const;
  int Fog16 (jhcImg& d16, const jhcImg& d8) const;
  int Remap16 (jhcImg& d8, const jhcImg& d16, int lo16, int hi16, int lo8 =1, int hi8 =255) const;

};


#endif


/////////////////////////////////////////////////////////////////////////////

//= Allows transparent insertion of MMX class over top of normal class.

#ifdef JHC_MMX
#include "MMX/jhcLUT_mmx.h"
#else
typedef jhcLUT_0 jhcLUT;
#endif


