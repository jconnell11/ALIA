// jhcBumps.h : finds and tracks objects on table
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2016-2019 IBM Corporation
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

#ifndef _JHCBUMPS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBUMPS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"               // common video
#include "Data/jhcBlob.h"              
#include "Data/jhcImg.h"
#include "Data/jhcParam.h"
#include "Processing/jhcGroup.h"
#include "Processing/jhcLabel.h"
#include "Processing/jhcStats.h"

#include "Depth/jhcOverhead3D.h"       // common robot
#include "Geometry/jhcSmTrack.h"


//= Finds and tracks objects on table.
// originally developed for multiple stationary Kinects
// information below is wrt native map coordinates (inches)
// <pre>
// raw array has nr (detections) or nr2 (+ under hand) entries:
//   0 = x center
//   1 = y center
//   2 = z center
//   3 = x extent wrt center
//   4 = y extent wrt center
//   5 = z extent wrt center
//   6 = ellipse length
//   7 = ellipse width
//   8 = ellipse angle
//   9 = hand center x (if touched)
//  10 = hand center y (if touched)
//
// shp array has pos.Limit() (tracked) entries:
//   0 = x extent (pos has x center TX)
//   1 = y extent (pos has y center TY)
//   2 = z extent (pos has z center TZ)
//   3 = ellipse length
//   4 = ellipse width
//   5 = ellipse angle
//
// class tree and parameters:
//     
//   Bumps           dps sps tps
//     Overhead3D    cps[] rps[] mps
//       Surface3D
//         PlaneEst
//
// </pre>

class jhcBumps : public jhcOverhead3D, protected jhcGroup, protected jhcLabel, private jhcStats
{
friend class CMesaDoc;       // for debugging
friend class CBanzaiDoc;

// PRIVATE MEMBER VARIABLES
private:
  // object detection
  jhcImg prev, hand, hcc;
  jhcRoi troi;
  int nr, nr2;

  // object tracking
  double **raw;
  int *ralt, *rlab;
  int total, rlim;

  // touch source
  int *touch;

  // debugging graphics
  char tmp[80];


// PROTECTED MEMBER VARIABLES
protected:
  // object detection
  jhcBlob blob;
  jhcBlob *alt_blob;
  jhcImg det, obj, cc;
  jhcArr pks;
  int surf;

  // object tracking
  jhcSmTrack pos;
  double **shp;


// PUBLIC MEMBER VARIABLES
public:
  // table surface mask
  jhcImg top;

  // environment calibration
  jhcArr hts;        

  // detection parameters
  jhcParam dps;
  double hobj, htol, hmix;
  int sm, pmin, sc, sth, amin;

  // shape estimate parameters
  jhcParam sps;
  int pcnt;
  double xyf, zf, xymix, zmix, amix;

  // special object detection
  jhcParam tps;
  int tcnt, hold;
  double tlen1, tlen0, twid1, twid0, tht1, tht0;

  // tracking flag for background thread
  int trk_bg;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcBumps ();
  jhcBumps (int n =50);
  void SetCnt (int n);
  bool TableMask () const {return(surf > 0);}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname, int geom =0) const;

  // main functions
  void Reset (int notop =0);
  void Surface (int trk =1);
  int Analyze (int trk =1);
  int CntTracked () const {return pos.Count();}
  int CntValid (int trk =1) const {return((trk > 0) ? CntTracked() : nr2);}
  int AnyTouch () const;

  // object properties 
  int ObjLimit (int trk =1) const;
  bool ObjOK (int i, int trk =1) const;
  int ObjID (int i, int trk =1) const;
  int ObjTrack (int id) const;
  const char *ObjDesc (int i, int trk =1) const;
  double PosX (int i, int trk =1) const; 
  double PosY (int i, int trk =1) const; 
  double PosZ (int i, int trk =1) const; 
  double SizeX (int i, int trk =1) const;
  double SizeY (int i, int trk =1) const;
  double SizeZ (int i, int trk =1) const;
  double Major (int i, int trk =1) const;
  double Minor (int i, int trk =1) const;
  double Angle (int i, int trk =1) const;
  double Elongation (int i, int trk =1) const;
  double MaxDim (int i, int trk =1) const;
  bool Contact (int i, int trk =1) const;
  double MapX (int i, int trk =1) const {return W2X(PosX(i, trk));}
  double MapY (int i, int trk =1) const {return W2Y(PosY(i, trk));}
  int Flat (int i) const;
  int Component (int) const;

  // display helpers
  void SetTag (int i, const char *txt);
  const char *GetTag (int i) const;
  int SetState (int i, int val);
  int GetState (int i) const;

  // auxiliary object-person array
  int TouchID (int i, int trk =1) const;
  int SetTouch (int i, int src, int trk =1);

  // proximity detection
  bool ObjNear (double hx, double hy, int trk =1, double bloat =0.0, double ecc =1.0) const;
  bool AgtNear (double hx, double hy, double dist =0.0) const;

  // environment calibration
  int TableHt (int update =1);
  bool OverTable (double wx, double wy) const;

  // debugging graphics
  int MaxID (int trk =1) const;
  int MinID (int trk =1) const;
  int ClickN (int mx, int my, int trk =1) const;
  int ShowAll (jhcImg& dest, int trk =1, int invert =0, int style =2);
  int Targets (jhcImg& dest, const char *desc =NULL, int trk =1, int invert =0);
  int Occlusions (jhcImg& dest);
  int Touches (jhcImg& dest) const;
  int ArmEnds (jhcImg& dest) const;
  int Ellipses (jhcImg& dest, double rect =0.0, int trk =1, int style =2);
  int TrackBox (jhcImg& dest, int i, int num =1, int invert =0, int style =2);
  int RawBox (jhcImg& dest, int i, int num =1, int invert =0);
  int ObjsCam (jhcImg& dest, int cam, int trk =1, int rev =0, int style =2);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void dealloc ();

  // processing parameters
  int detect_params (const char *fname);
  int shape_params (const char *fname);
  int target_params (const char *fname);

  // main functions
  virtual void raw_objs (int trk);
  void obj_boxes (jhcBlob *b, int flat);
  virtual double find_hmax (int i, const jhcRoi *area);
  void adj_shapes ();

  // occlusion handling
  virtual void occluded ();
  void img_box (double& xc, double& yc, double& wid, double& len, int i) const;
  int arm_end (int& ex, int& ey, double xc, double yc) const;
  int incl_x (double xc, double wid, int ex) const; 
  int incl_y (double yc, double len, int ey) const; 
  int drag_x (double xc, double yc, double wid, double ht, const jhcImg& src, int mark) const;
  int drag_y (double xc, double yc, double wid, double ht, const jhcImg& src, int mark) const;
  void make_det (int i, double xc, double yc, double zc, double *wlh, int ex, int ey);

  // target finding
  int mark_targets (const char *name, int trk);

  // read-only properties
  bool ok_idx (int i, int trk) const;

  // debugging graphics
  const char *label (int i, int style =2);

};


#endif  // once




