// jhcPlainFloor.h : makes synthetic depth image for an untextured floor
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

#include "Data/jhcBBox.h"              // common video
#include "Data/jhcImg.h"        
#include "Processing/jhcALU.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcColor.h"
#include "Processing/jhcDist.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcEdge.h"
#include "Processing/jhcGray.h"
#include "Processing/jhcGroup.h"
#include "Processing/jhcLUT.h"
#include "Processing/jhcThresh.h"


//= Makes synthetic depth image for an untextured floor.

class jhcPlainFloor: private jhcALU, private jhcArea, protected jhcColor, private jhcDist, private jhcDraw, 
                     private jhcEdge, private jhcGray, private jhcGroup, protected jhcLUT, private jhcThresh
{
friend class CFocusDoc;                // for debugging

// PRIVATE MEMBER VARIABLES
private:
  jhcImg boost, comps, seeds, mono, tmp2, edge, sep, reg, floor;
  jhcBBox boxes;
  double flen, ht, tilt;


// PROTECTED MEMBER VARIABLES
protected:
  jhcImg tmp; 
  int iw, ih;


// PUBLIC MEMBER PARAMETERS
public:
  // region finding parameters
  double sc, msc;
  int eth, bd, blk, smin, rsm;

  // depth inference parameters
  double frac;
  int amin, claim, bot, gap;
 

// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcPlainFloor ();
  jhcPlainFloor ();

  // main functions
  void Init (double f, int w =640, int h =480);
  int Range (jhcImg& d16, const jhcImg& rgb, double h, double t);
  int Grass (jhcImg& dest);


// PRIVATE MEMBER FUNCTIONS
private:
  // segmentation and estimation
  void combo_edges (jhcImg& sob, jhcImg& bw, const jhcImg& rgb);
  void bland_areas (jhcImg& proto, const jhcImg& sob, const jhcImg& bw);
  void pick_floor (jhcImg& gnd, jhcImg& cc, const jhcImg& proto);
  void ground_d16 (jhcImg& d16, const jhcImg& gnd) const;
  void obst_scan (jhcImg& d16, const jhcImg& cc, const jhcImg& gnd) const;

};
