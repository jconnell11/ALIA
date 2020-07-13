// jhcParse3D.h : find head and hands of people using overhead map
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2020 IBM Corporation
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

#ifndef _JHCPARSE3D_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCPARSE3D_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"           // common video
#include "Data/jhcBBox.h"    
#include "Data/jhcBlob.h"  
#include "Data/jhcImg.h"  
#include "Data/jhcParam.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcGroup.h"
#include "Processing/jhcLabel.h"
#include "Processing/jhcThresh.h"

#include "People/jhcBodyData.h"   // common robot


//= Find head and hands of people using overhead map.
// <pre>
// Head finding steps:
//   separates top-down depth map into blobs at chest level to find potential people
//   cuts each candidate at eye level wrt top and checks for good head shape and size
//   cuts each remaining candidate at shoulder level and checks for good shape and size
// Hand finding steps:
//   appends biggest nearby blob to each valid head in case arm was separated
//   generates radial projection of blob(s) around head center with 1 degree step size
//   takes two biggest peaks if at least minimal distance radially from head center
//   examines overhead depth around each peak to get finger height and check arm length
// Pointing rays used to be are lines from each head to associated hands (ray_est0)
//   now finds major axis of pixels in the fingertips to wrist zone (ray_est)
// All detection information stored in array of jhcBodyData called "raw"
// jhcBodyData class has more functionality than needed but subsumes old jhcBodyParts
// </pre>

class jhcParse3D : protected jhcArea, protected jhcGroup, protected jhcLabel, protected jhcThresh,
                   virtual protected jhcDraw
{
// PROTECTED MEMBER VARIABLES
protected:
  static const int rmax = 50;          /** Maximum number of detections. */

  // map coordinate transform and size
  jhcMatrix w2m; 
  double ipp;
  int mw, mh;


// PRIVATE MEMBER VARIABLES
private:
  // head finding
  jhcImg floor, chest, mid, arm, step;
  jhcImg cc, cc2, cc0;
  jhcBBox box;
  jhcBlob blob, blob2;
  jhcArr hist;
  jhcMatrix m2w;
  int xlink[rmax], ylink[rmax];        /** Head centers in map. */
  double z0, z1, rot, x0, y0;
  int nr;

  // arm finding
  jhcArr star0;


// PROTECTED MEMBER VARIABLES
protected:
  // debugging graphics
  char tmp[80];


// PUBLIC MEMBER VARIABLES
public:
  // the actual heads, hands, and rays found
  jhcBodyData *raw;                              // rmax entries

  // save intermediate head results (for graphics)
  int dbg;

  // intermediate arm results (for graphics)
  jhcArr star[rmax];
  int lpk[rmax], rpk[rmax], stx[rmax], sty[rmax];

  // parameters for bulk person separation
  jhcParam bps;
  int sth;
  double wall, ch, sm, amin, amax, h0, h1;

  // parameters for head filtering
  jhcParam hps;
  int pcnt;
  double chop, hmin, hecc, w0, w1, edn, margin;

  // parameters for shoulder filtering
  jhcParam sps;
  double shdn, smin, secc, sw0, wrel, arel, ring;

  // parameters for finding and reconnecting arms
  jhcParam aps;
  int sth2, ret;
  double alev, sm2, arm0, agrab, arm1;

  // parameters for finding hands
  jhcParam gps;
  int ssm;
  double afall, fsz, fpct, foff, ext0, ext1, back;

  // parameters for finding pointing direction
  jhcParam eps;
  int ref, fit;
  double flen, fecc, flat, dip, plen;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcParse3D ();
  ~jhcParse3D ();
  void MapSize (const jhcImg& ref) {MapSize(ref.XDim(), ref.YDim());}
  void MapSize (int x, int y);
  void SetScale (double lo =20.0, double hi =90.0, double sc =0.5);
  void SetView (double rot =0.0, double xref =0.0, double yref =0.0);
  void SetView (const double *p6) {SetView(p6[0] - 90.0, p6[3], p6[4]);}
  int ParseWid () const {return mw;}
  int ParseHt () const  {return mh;}
  double ParseScale () const {return ipp;}
  const jhcMatrix& ToMap () const {return w2m;}

  // parameter utilites
  void SetChest (double wz, double ht, double sc, int th, double a0, double a1, double lo, double hi)
    {wall = wz; ch = ht; sm = sc; sth = th; amin = a0; amax = a1; h0 = lo; h1 = hi;}
  void SetHead (double hz, double a, double e, double sm, double big, double dz, double ej, int pk)
    {chop = hz; hmin = a; hecc = e; w0 = sm; w1 = big; edn = dz; margin = ej; pcnt = pk;}
  void SetShoulder (double dz, double a, double e, double w, double wr, double ar, double r)
    {shdn = dz; smin = a; secc = e; sw0 = w; wrel = wr; arel = ar; ring = r;}
  void SetArm (double z, double sc, int th, double a0, int add, double r, double a1)
    {alev = z; sm2 = sc; sth2 = th; arm0 = a0; ret = add; agrab = r; arm1 = a1;}
  void SetHand (int sm, double dn, double reg, double pc, double xy, double r0, double r1, double dr)
    {ssm = sm; afall = dn; fsz = reg; fpct = pc; foff = xy; ext0 = r0; ext1 = r1; back = dr;}
  void SetAim (double hr, double e, double f, double dt, double r0)
    {flen = hr; fecc = e; flat = f; dip = dt; plen = r0;}

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  int FindPeople (const jhcImg& map);
  int NumRaw () const {return nr;}
  bool PersonBlob (const jhcMatrix& probe) const;
  int BlobAt (double wx, double wy) const;

  // debugging graphics
  int NoWalls (jhcImg& dest) const;
  int ChestMap (jhcImg& dest) const;
  int ChestBlobs (jhcImg& dest) const;
  int HeadLevels (jhcImg& dest) const;
  int ArmMap (jhcImg& dest) const;
  int ArmBlobs (jhcImg& dest) const;
  int ArmClaim (jhcImg& dest);
  int ArmExtend (jhcImg& dest);
  int RawMark (jhcImg& dest, int invert =0, double sz =8.0, int col =5)         
   {return MarkHeads(dest, raw, nr, invert, sz, 2, col);}
  int RawHeads (jhcImg& dest, int invert =0, double sz =8.0, int col =5) const   
   {return ShowHeads(dest, raw, nr, invert, sz, col);}
  int RawHands (jhcImg& dest, int invert =0) const   
   {return ShowHands(dest, raw, nr, invert);}
  int RawRays (jhcImg& dest, int invert =0, double zlev =0.0) const 
   {return ShowRays(dest, raw, nr, invert, zlev);}
  int RawRaysY (jhcImg& dest, int invert =0, double yoff =0.0) const 
   {return ShowRaysY(dest, raw, nr, invert, yoff);}
  int RawRaysX (jhcImg& dest, int invert =0, double xoff =0.0) const 
   {return ShowRaysX(dest, raw, nr, invert, xoff);}

  // list-based graphics
  int MarkHeads (jhcImg& dest, jhcBodyData *items, int n, int invert =0, double sz =8.0, int style =2, int col =5);
  int ShowHeads (jhcImg& dest, const jhcBodyData *items, int n, int invert =0, double sz =8.0, int col =5) const;
  int ShowHands (jhcImg& dest, const jhcBodyData *items, int n, int invert =0, int col =7) const;
  int ShowRays (jhcImg& dest, const jhcBodyData *items, int n, int invert =0, double zlev =0.0, int pt =3) const;
  int ShowRaysY (jhcImg& dest, const jhcBodyData *items, int n, int invert =0, double yoff =0.0, int pt =3) const;
  int ShowRaysX (jhcImg& dest, const jhcBodyData *items, int n, int invert =0, double xoff =0.0, int pt =3) const;

  // frontal view utility (original map origin was at middle of bottom)
  void BeamCoords (jhcMatrix& alt, const jhcMatrix& ref) const
    {alt.RelVec3(ref, x0 - 0.5 * mw * ipp, y0, 0.0);}
  void InvBeamCoords (jhcMatrix& alt, const jhcMatrix& ref) const
    {alt.RelVec3(ref, 0.5 * mw * ipp - x0, -y0, 0.0);}


// PROTECTED MEMBER FUNCTIONS
protected:
  // debugging graphics
  const char *label (const jhcBodyData& guy, int style);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameter manipulation 
  int chest_params (const char *fname);
  int head_params (const char *fname);
  int shoulder_params (const char *fname);
  int arm_params (const char *fname);
  int hand_params (const char *fname);
  int finger_params (const char *fname);

  // head finding
  int find_heads (const jhcImg& ohd);
  int chk_head (int n, double h, const jhcImg& view, 
                const jhcImg& comp, int i, const jhcRoi& area);
  int chk_shoulder (int n, double w, double a, const jhcImg& view, 
                    const jhcImg& comp, int i, const jhcRoi& area);
  double find_max (const jhcImg& val, const jhcImg& comp, int i, const jhcRoi& area);
  void thresh_within (jhcImg& dest, const jhcImg& src, int th, 
                      const jhcImg& comp, int i, const jhcRoi& area) const;
  void first_nz (int& x, int& y, const jhcImg& src, 
                 const jhcImg& comp, int i, const jhcRoi& area) const;
  int ht2pel (double ht) const;
  double pel2ht (int pel) const;
  void mid_back (int& cx, int& cy, int hd, int sh) const;

  // arm finding
  void find_arms (const jhcImg& ohd, int nh);
  int arm_peaks (const jhcImg& comp, jhcBBox& b, int hx, int hy, int bnum, int i);
  void radial_plot (jhcArr& plot, int hx, int hy, const jhcImg& comp, const jhcBBox& b, int i) const;
  int grab_arm (int hx, int hy, const jhcImg& comp, const jhcBBox& b, int i) const;
  void finger_area (jhcRoi& tip, int hx, int hy, const jhcArr& plot, int pk) const;
  int finger_loc (int& ix, int& iy, int hx, int hy, const jhcImg& map, 
                  const jhcImg& comp, int bnum, int alt, jhcRoi& area) const;
  double arm_coords (jhcBodyData *item, int ix, int iy, int iz, int mx, int my, int side) const;
  int swap_arms (int i);

  // ray finding
  int est_ray (jhcBodyData *item, int side, const jhcImg& map, int ix, int iy, int iz,
               const jhcImg& comp, int bnum, int alt, const jhcRoi& body);
  int area_stats (double s[], const jhcImg& map, int ix, int iy, int iz,
                 const jhcImg& comps, int bnum, int alt, const jhcRoi& area);
  int find_axis (double& ax, double& ay, double& az, const double s[]) const;

  // whether world point is within viewing bounds by a certain amount
  virtual int visible (const jhcMatrix& pt, double margin) const {return 1;}


};


#endif  // once




