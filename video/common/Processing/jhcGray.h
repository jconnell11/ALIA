// jhcGray.h : methods for converting image to monochrome
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2018 IBM Corporation
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

#ifndef _JHCGRAY_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGRAY_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Methods for converting image to monochrome.

class jhcGray
{
// PRIVATE MEMBER VARIABLES
private:
  static int instances;
  static UC8 *third, *blut, *glut, *rlut;


// PROTETED MEMBER VARIABLES
protected:
  jhcImg mbase;


// PUBLIC MEMBER FUNCTIONS
public:
  jhcGray ();
  ~jhcGray ();

  // external functions
  int ForceMono (jhcImg& dest, const jhcImg& src, int style =1);
  int Mono3 (jhcImg& dest, const jhcImg& src, int style =1);
  int MonoAvg (jhcImg& dest, const jhcImg& src) const;
  int MonoAvg16 (jhcImg& dest, const jhcImg& red, const jhcImg& grn, const jhcImg& blu) const;
  int MonoRG (jhcImg& dest, const jhcImg& src) const;
  int MonoSamp (jhcImg& dest, const jhcImg& src) const;
  int Intensity (jhcImg& dest, const jhcImg& src) const;
  int PseudoInt (jhcImg& dest, const jhcImg& src) const;
  int Equalize (jhcImg& dest, const jhcImg& src, int thresh =0) const;
  int EqualizeRGB (jhcImg& dest, const jhcImg& src, int thresh =0) const;

};


#endif




