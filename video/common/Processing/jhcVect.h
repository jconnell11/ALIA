// jhcVect.h : pixel-wise combine values from different fields of same image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2016 IBM Corporation
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

#ifndef _JHCVECT_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCVECT_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Pixel-wise combine values from different fields of same image.

class jhcVect
{
// PUBLIC MEMBER FUNCTIONS
public:
  // general functions
  int SumAll (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int AvgAll (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int MaxAll (jhcImg& dest, const jhcImg& src) const;
  int MinAll (jhcImg& dest, const jhcImg& src) const;
  int MedianAll (jhcImg& dest, const jhcImg& src) const;
  int HiAvgAll (jhcImg& dest, const jhcImg& src) const;
  int LoAvgAll (jhcImg& dest, const jhcImg& src) const;
  int AllWithin (jhcImg& dest, const jhcImg& src, int lo, int hi) const;

  // color image functions
  int ValidRGB (jhcImg& dest, const jhcImg& src, int lo, int hi) const;
  int AnyOverRGB (jhcImg& dest, const jhcImg& src, int rth, int gth, int bth) const;
  int FieldsOverRGB (jhcImg& dest, const jhcImg& src, int rth, int gth, int bth) const;
  int WtdSumRGB (jhcImg& dest, const jhcImg& src, double rsc, double gsc, double bsc) const;
  int MaxDevRGB (jhcImg& dest, const jhcImg& src, int rval, int gval, int bval) const;


// PRIVATE MEMBER FUNCTIONS
private:
  int AvgAll_3 (jhcImg& dest, const jhcImg& src, double sc) const;
  int MaxAll_3 (jhcImg& dest, const jhcImg& src) const;
  int MinAll_3 (jhcImg& dest, const jhcImg& src) const;
  int AllWithin_3 (jhcImg& dest, const jhcImg& src, int lo, int hi) const;
};


#endif




