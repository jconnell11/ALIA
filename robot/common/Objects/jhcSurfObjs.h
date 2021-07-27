// jhcSurfObjs.h : find objects on surfaces using single mobile depth scanner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2021 Etaoin Systems
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

#include "Processing/jhcColor.h"       // common video
#include "Processing/jhcGray.h"     
#include "Processing/jhcRuns.h"     

#include "Objects/jhcPatchProps.h"     // common robot

#include "Objects/jhcBumps.h"


//= Find objects on surfaces using single mobile depth scanner.
// uses surface height estimate to make local map of beam region
// map itself is sensor relative, but objects have global coordinates
// objects are either depth protrusion or isolated surface markings 
// <pre>
// class tree and parameters:
//
//   SurfObjs          zps gps     
//     Bumps           dps sps tps
//       Overhead3D    cps[] rps[] mps
//         Surface3D
//           PlaneEst
//
// </pre>

class jhcSurfObjs : public jhcBumps, private jhcColor, private jhcGray, private jhcRuns
{
friend class CBanzaiDoc;     // for debugging

// PRIVATE MEMBER VARIABLES
private:
  // current camera pose 
  double xcomp, ycomp, pcomp;

  // color segmentation
  jhcImg pat, gray, cdet, bgnd, rim, gcc;
  jhcBlob glob;
  jhcArr wkhist;
  int wk0, wk1;

  // object mask and segmentation alternation
  jhcImg cmsk;          
  int phase;

  // object color analysis and cached data
  jhcPatchProps pp;
  double **cfrac;
  int **cvec;
  int ntrk;


// PUBLIC MEMBER VARIABLES
public:
  // depth segmentation parameters
  jhcParam zps;
  double lrel, flip, sfar, wexp;
  int ppel, pth, cup, bej;

  // color segmentation parameters
  jhcParam gps;
  double kdrop, wdrop, line;
  int idev, csm, cth, hole, bgth;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSurfObjs ();
  jhcSurfObjs (int n =50);
  void SetCnt (int n);

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  void Reset ();
  void AdjTracks (const jhcMatrix& loc, const jhcMatrix& dir);
  int FindObjects (const jhcImg& col, const jhcImg& d16);

  // object properties
  int Closest () const;
  double DistXY (int i) const;
  double World (jhcMatrix& loc, int i) const;
  int Spectralize (const jhcImg& col, const jhcImg& d16, int i, int clr =0);
  int DegColor (int i, int cnum) const;
  double AmtColor (int i, int cnum) const;

  // debugging graphics
  int AttnCam (jhcImg& dest, int most =5, int pick =2);
 

// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void dealloc ();

  // processing parameters
  int tall_params (const char *fname);  
  int flat_params (const char *fname);

  // main functions 
  void adj_tracks (double sx, double sy, double pan);

  // segmentation
  int tall_objs ();
  int flat_objs ();

  // overrides
  void raw_objs (int trk);
  virtual double find_hmax (int i, const jhcRoi *area);
  void occluded () {}

  // debugging graphics
  void attn_obj (jhcImg& dest, int i, int col);


};


#endif  // once




