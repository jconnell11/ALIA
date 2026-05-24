// jhcVisMotion.h : look for areas of visual motion in color image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2026 Etaoin Systems
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

#include "Data/jhcBlob.h"              // common video
#include "Data/jhcImg.h"   
#include "Data/jhcParam.h"
#include "Processing/jhcALU.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcGray.h"
#include "Processing/jhcGroup.h"
#include "Processing/jhcLUT.h"
#include "Processing/jhcResize.h"
#include "Processing/jhcStats.h"
#include "Processing/jhcThresh.h"


//= Look for areas of visual motion in color image.

class jhcVisMotion : private jhcALU, private jhcArea,   private jhcGray,  private jhcGroup, 
                     private jhcLUT, private jhcResize, private jhcStats, private jhcThresh
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg sm, gray0, gray1, diff, acc, cc;
  jhcImg *mono, *prev;
  double cf;

  // motion regions
  jhcBlob blob;
  double mx, my, pmot, tmot, seek;
  UL32 targ;
  int cnt, itch;

  // parked gaze direction
  UL32 wait, fix;
  int crink;


// PRIVATE MEMBER PARAMETERS
private:
  // askew parameters
  double pdef, tdef, atol, park, annoy, restore;

  // detection parameters
  double stare, barf, fade;
  int big, sc, amin, sure;


// PUBLIC MEMEBER VARIABLES
public:
  jhcParam aps, dps;
  jhcImg sal;                // for display


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcVisMotion ();
  jhcVisMotion ();
  void SetSize (int iw, int ih, double flen);
  bool Center (double& x, double& y)
    {if (cnt < sure) return false; x = mx; y = my; return true;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const; 

  // main functions
  void Reset ();
  void Analyze (const jhcImg& col, double pan, double tilt, double stable);
  bool Interest (double& pan, double& tilt) const
    {if (itch <= 0) return false; pan = pmot; tilt = tmot; return true;}
  bool Askew (double& pan, double& tilt) const
    {if (crink <= 0) return false; pan = pdef; tilt = tdef; return true;}


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int detect_params (const char *fname);
  int askew_params (const char *fname);

  // main functions
  int clr_acc ();
  int find_regions (double pan, double tilt);
  void swap_bufs ();
  void chk_nominal (double pan, double tilt, double stable);


};
