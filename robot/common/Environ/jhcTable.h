// jhcTable.h : find supporting surfaces in full height depth map
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021-2023 Etaoin Systems
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

#ifndef _JHCTABLE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTABLE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"               // common video
#include "Data/jhcBlob.h"
#include "Data/jhcImg.h"
#include "Data/jhcParam.h"
#include "Processing/jhcArea.h"      
#include "Processing/jhcGroup.h"
#include "Processing/jhcHist.h"
#include "Processing/jhcThresh.h"

#include "Geometry/jhcKalVec.h"        // common robot   


//= Find supporting surfaces in full height depth map.
// typically only sees part of surface controlled by neck pan and tilt angles
// normally latches onto closest surface which is about height of arm
// can externally bias to a certain preferred height ("hpref") or range of distances ("dpref")

class jhcTable : private jhcArea, private jhcGroup, private jhcHist, private jhcThresh
{
friend class CBanzaiDoc;               // for debugging

// PRIVATE MEMBER VARIABLES
private:
  // surface detection
  jhcImg wmap, wbin, wsm, wcc;
  jhcArr hhist;
  jhcBlob wlob;
  double wipp, zbot, zrng, ex, ey;

  // current state
  double hx, hy, hz;
  int fcnt;

  // immediate surface results
  jhcKalVec tmid;
  double offset, zest, ztab;
  int tsel;


// PRIVATE MEMBER PARAMETERS
private:
  // height finding parameters
  double margin, over, flip, under, dp, dt;
  int hsm, ppel;

  // surface candidate parameters
  double ztol, pmix, pn, hn;
  int wsc, wth, wmin;


// PUBLIC MEMBER VARIABLES
public:
  jhcParam hps, cps;

  // surface preferences
  double dpref, hpref, xpref, ypref;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcTable ();
  jhcTable ();
  double MidX () const {return wmap.RoiAvgX();}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // configuration
  void SetSize (const jhcImg& ref) {SetSize(ref.XDim(), ref.YDim());}
  void SetSize (int x, int y);

  // main functions
  void Reset ();
  double PickPlane (const jhcImg& hts, double res, double z0, double z1);
  int FindSurf (const jhcMatrix& head, double ht);
  double PlaneZ () const {return ztab;}
  void BiasSurf (double wx, double wy, double wz) {xpref = wx; ypref = wy; hpref = wz;}
  void BiasSurf (const jhcMatrix& loc) {BiasSurf(loc.X(), loc.Y(), loc.Z());}

  // alternative targets
  double BestSurf (double x, double y);
  void InitSurf () {tsel = -1; tmid.Clear(0.0);}
  double NextSurf ();

  // target information
  bool SurfOK () const {return(tsel >= 0);}
  void SurfMid (jhcMatrix& surf) const {surf.Copy(tmid);}
  void SurfEdge (jhcMatrix& edge, const jhcMatrix& mid, double off) const;
  void SurfEdge (jhcMatrix& edge, double inset =0.0) const {SurfEdge(edge, tmid, offset - inset);}
  double SurfHt() const {return tmid.Z();}
  double SurfOff () const {return offset;}
  double SurfDist () const {return(tmid.PlaneVec3() - offset);}
//  void SurfGaze (double& pan, double& tilt, const jhcMatrix& edge) const;
//  int SurfGaze (double& pan, double& tilt) const;
  double SurfMove (double wid, int ymin =0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int height_params (const char *fname);
  int cand_params (const char *fname);

  // main functions
  double z2i (int z) const    {return(zbot + (z - 1) * zrng / 253.0);}
  int i2z (double ht) const   {return(ROUND(253.0 * (ht - zbot) / zrng) + 1);} 
  int zdev (double dht) const {return ROUND(253.0 * dht / zrng);}

  // main functions
  void hist_range (jhcArr& hist, const jhcImg& hts, double close, double far);
  int update_surf (int t);


};


#endif  // once




