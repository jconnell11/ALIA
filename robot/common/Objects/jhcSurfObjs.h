// jhcSurfObjs.h : find objects on surfaces using single mobile depth scanner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
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

#ifndef _JHCSURFOBJS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSURFOBJS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Processing/jhcRuns.h"        // common video

#include "Objects/jhcBumps.h"


//= Find objects on surfaces using single mobile depth scanner.
// uses surface height estimate to make local map of beam region
// map itself is sensor relative, but objects have global coordinates

class jhcSurfObjs : public jhcBumps, private jhcRuns
{
// PRIVATE MEMBER VARIABLES
private:
  // current camera pose 
  double xcomp, ycomp, pcomp;


// PUBLIC MEMBER VARIABLES
public:
  // surface finding parameters
  jhcParam zps;
  double lrel, sfar, wexp;
  int ppel, spel, cup, bej;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSurfObjs ();
  jhcSurfObjs ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  void Reset ();
  int FindObjects (const jhcImg& d16, const jhcMatrix& pos, const jhcMatrix& dir);
  double World (jhcMatrix& loc, int i) const;
 

// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int surf_params (const char *fname);

  // main functions 
  void adj_tracks (double sx, double sy, double pan);

  // overrides
  void raw_objs (int trk);
  void occluded () {}


};


#endif  // once




