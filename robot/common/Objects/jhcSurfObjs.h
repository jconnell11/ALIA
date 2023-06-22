// jhcSurfObjs.h : find objects on surfaces using single mobile depth scanner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2023 Etaoin Systems
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
// analyzes height in narrow range around table (typ. -2" to +18")
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
  const jhcImg *kill;
  int phase;

  // top portion finding
  jhcImg high;

  // object color analysis and cached data
  jhcPatchProps pp;
  double **cfrac;
  int **cvec;
  int ntrk;


// PUBLIC MEMBER VARIABLES
public:
  // depth segmentation parameters
  jhcParam zps;
  double sfar, wexp;
  int pth, cup, bej, rmode;

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
  void AdjBase (double dx0, double dy0, double dr);
  void AdjNeck (const jhcMatrix& loc, const jhcMatrix& dir);
  int FindObjects (const jhcImg& col, const jhcImg& d16, const jhcImg *mask =NULL);

  // object properties
  int Closest () const;
  double DistXY (int i) const;
  double World (jhcMatrix& loc, int i) const;
  double World (double& wx, double& wy, int i) const;
  double FullTop (double& wx, double& wy, double& wid, double& len, int i, double slice =0.3);
  int NearTable (jhcMatrix& tpt, int i) const;
  void ForcePose (int i, double wx, double wy, double wz, double ang);
  int Spectralize (const jhcImg& col, const jhcImg& d16, int i, int clr =0);
  int DegColor (int i, int cnum) const;
  double AmtColor (int i, int cnum) const;

  // coordinate transforms
  void FullXY (double& wx, double& wy, double mx, double my) const;
  void PelsXY (double& wx, double& wy, double ix, double iy) const;
  double FullAngle (double mdir) const;
  double FullOrient (double mdir) const;
  void ViewXY (double& mx, double& my, double wx, double wy) const;
  void ViewPels (double& ix, double& iy, double wx, double wy) const;
  void ViewPels (int& ix, int& iy, double wx, double wy) const;
  double ViewAngle (double wdir) const;
  double ViewOrient (double wdir) const;
  void CamPels (int& ix, int &iy, const jhcMatrix& wpt, int ydim =480) const;

  // debugging graphics
  int AttnCam (jhcImg& dest, int pick =2, int known =3, int all =5);
  int MarkCam (jhcImg& dest, const jhcMatrix& wpt, int col =6);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void dealloc ();

  // processing parameters
  int tall_params (const char *fname);  
  int flat_params (const char *fname);

  // segmentation
  int tall_objs ();
  int flat_objs ();

  // overrides
  void raw_objs (int trk);
  virtual double find_hmax (int i, const jhcRoi *area);
  void occluded () {}

  // object properties
  void obj_slice (jhcImg& dest, int lab, double up) const;

  // debugging graphics
  void attn_obj (jhcImg& dest, int i, int t, int col);


};


#endif  // once




