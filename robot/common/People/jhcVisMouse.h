// jhcVisMouse.h : tracks user's arm and hand to simulate a computer mouse
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

#ifndef _JHCVISMOUSE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCVISMOUSE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Data/jhcParam.h"
#include "Processing/jhcALU.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcStats.h"
#include "Processing/jhcThresh.h"


//= Tracks user's arm and hand to simulate a computer mouse.

class jhcVisMouse : private jhcALU, private jhcArea, private jhcStats, private jhcThresh 
{
friend class CKleptoDoc;

// PRIVATE MEMBER VARIABLES
private:
  jhcImg ref, diff, bin, big;
  int ix, iy, mx, my, cx, cy, px, py;
  int a, bad, bored, cnt, first;


// PUBLIC MEMBER VARIABLES
public:
  // parameters
  jhcParam mps;
  int th, sc, amin, amax, clr, wait, same, stale;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcVisMouse ();
  void Reset ();
  void SetSize (jhcImg& ref);
  void SetSize (int x, int y);

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // intermediate data
  int ArmArea () const  {return a;}
  int ImgX () const     {return ix;};
  int ImgY () const     {return iy;};
  int MouseX () const   {return mx;};
  int MouseY () const   {return my;};
  int ClickX () const   {return cx;};
  int ClickY () const   {return cy;};
  int GotClick () const {if ((cx > 0) || (cy > 0)) return 1; return 0;}
  const jhcImg *Reference ()  {return &ref;}
  const jhcImg *Difference () {return &diff;}
  const jhcImg *Binary ()     {return &bin;}
  const jhcImg *Smoothed ()   {return &big;}

  // main functions
  int Pointing (int& x, int& y, const jhcImg& src);
  

// PRIVATE MEMBER FUNCTIONS
private:
  int mouse_params (const char *fname);

  int pointing_ref (const jhcImg& src);

};


#endif  // once




