// jhcOverhead3D.h : combine depth sensors into an overhead height map
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2016-2020 IBM Corporation
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

#ifndef _JHCOVERHEAD3D_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCOVERHEAD3D_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <math.h>

#include "Data/jhcImg.h"             // common video
#include "Data/jhcParam.h"
#include "System/jhcFill.h"
#include "Processing/jhcALU.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcHist.h"
#include "Processing/jhcResize.h"
#include "Processing/jhcThresh.h"

#include "Depth/jhcSurface3D.h"      // common robot


//= Combines depth sensors into an overhead height map.
// borrows a lot from class jhcScreenPos (in Muriel project)
// <pre>
// class tree and parameters:
//
//   Overhead3D    cps[] rps[] mps
//     Surface3D
//       PlaneEst
//
// </pre>

class jhcOverhead3D : public jhcSurface3D, 
                      protected jhcALU, protected jhcArea, protected jhcHist, protected jhcResize, protected jhcThresh,
                      virtual protected jhcDraw
{
// PRIVATE MEMBER VARIABLES
private:
  jhcFill fill;
  jhcImg ctmp, dmsk, mask;
  int smax;

  // plane fitting results
  double efit, tfit, rfit, hfit;
  double tavg, ravg, havg;
  int fit, nfit;


// PROTECTED MEMBER VARIABLES
protected:
  double hfov, vfov;


// PUBLIC MEMBER VARIABLES
public:
  jhcImg map, map2;          /** Fused height map.    */
  jhcArr hhist;              /** Height histogram.    */
  char name[40];             /** Configuration tag.   */
  double ztab;               /** Table height now.    */
  int rasa;                  /** New ingest flag.     */
  int *used;                 /** Sensors combined.    */

  // camera parameters (multiple Kinects)
  jhcParam *cps;
  double *cx, *cy, *cz, *p0, *t0, *r0, *rmax;
  int *dev;

  // restriction regions (multiple Kinects)
  jhcParam *rps;
  int *rx, *ry;

  // map parameters
  jhcParam mps;
  double mw, mh, x0, y0, zlo, zhi, ipp, ztab0;

  // parameters for surface fitting
  jhcParam pps;
  double srng, rough, dt, dr, dh;
  int npts, wfit;

  // Kinect beam parameters 
  jhcParam kps;
  double dlf, drt, dtop, dbot;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcOverhead3D ();
  jhcOverhead3D (int ncam =1);
  void AllocCams (int ncam);
  int NumCam (int lim =0) const 
    {return((lim <= 0) ? smax : __min(lim, smax));}
  int XDim () const {return map.XDim();}
  int YDim () const {return map.YDim();}
  int InputW () const {return jhcSurface3D::XDim();}
  int InputH () const {return jhcSurface3D::YDim();}

  // map parameters as functions
  double MX0 () const {return x0;}
  double MY0 () const {return y0;}
  double MDX () const {return(x0 - 0.5 * mw);}
  double MSC (const jhcImg& dest) const 
    {return(dest.YDim() / (double) map.YDim());} 
  double ISC (const jhcImg& dest) const
    {return(dest.YDim() / (double) InputH());}

  // parameter utilities
  void SetMap (double w, double h, double x, double y, 
               double lo, double hi, double pel, double ht);
  void SetFit (double d, int n, double e, double t, double r, double h, int w =100);
  void SetCam (int n, double x, double y, double z, double pan, double tilt, 
               double roll =0.0, double rng =180.0, int dnum =0); 
  void SetCam (int n, const jhcMatrix& pos, const jhcMatrix& dir, double rng =180.0, int dnum =0)
    {SetCam(n, pos.X(), pos.Y(), pos.Z(), dir.P(), dir.T(), dir.R(), rng, dnum);}

  // processing parameter bundles
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname, int geom =0) const;
  int SaveCfg (const char *fname, int geom =0) const;

  // camera utilities
  void CopyCams (const jhcOverhead3D& ref);
  int DumpLoc (jhcMatrix& loc, int cam =0) const;
  int LoadLoc (int cam, const jhcMatrix& loc);
  int DumpPose (jhcMatrix& pose, int cam =0) const;
  int LoadPose (int cam, const jhcMatrix& pose);
  int Active () const;
  int FirstCam () const;
  int LastCam () const;
  bool CamOK (int i) const {return(Dev(i) >= 0);}
  int Dev (int i) const 
    {return(((i < 0) || (i >= smax)) ? -1 : dev[i]);}

  // image normalization
  bool Sideways (int cam =0) const;
  int CorrectW (int cam =0) const {return((Sideways(cam)) ? InputH() : InputW());}
  int CorrectH (int cam =0) const {return((Sideways(cam)) ? InputW() : InputH());}
  bool AlreadyOK (const jhcImg& ref, int cam =0, int big =0) const;
  void RollSize (jhcImg& dest, const jhcImg& ref, int cam =0, int fields =0, int big =0) const;
  jhcImg *Correct (jhcImg& dest, const jhcImg& src, int cam =0, int big =0);
  double ImgRoll (int cam =0) const;

  // main functions
  void SrcSize (int w =640, int h =480, double f =525.0, double sc=0.9659);
  void Reset ();
  int Ingest (const jhcImg& d16, int cam =0, int zst =0, double zlim =0.0) 
    {return Ingest(d16, zlo, zhi, cam, zst, zlim);}
  int Ingest (const jhcImg& d16, double bot, double top, int cam =0, int zst =0, double zlim =0.0);
  int Reproject (jhcImg& dest, const jhcImg& d16, int cam =0, int zst =0, double zlim =0.0, int clr =1) 
    {return Reproject(dest, d16, zlo, zhi, cam, zst, zlim, clr);}
  int Reproject (jhcImg& dest, const jhcImg& d16, double bot, double top, int cam =0, int zst =0, double zlim =0.0, int clr =1);
  int Reproject2 (jhcImg& rgb, jhcImg& hts, const jhcImg& col, const jhcImg& d16, int cam =0, int clr =1);
  void Interpolate (int sc =9, int pmin =3);

  // pose correction from plane fitting
  bool NoPlane () const   {return(fit <= 0);}
  double TiltDev () const {return((fit > 0) ? tfit : tavg);}
  double RollDev () const {return((fit > 0) ? rfit : ravg);}
  double HtDev () const   {return((fit > 0) ? hfit : havg);}

  // position and size conversion routines
  double W2X (double wx) const {return((wx + x0) / ipp);}
  double W2Y (double wy) const {return((wy + y0) / ipp);}
  double M2X (double ix) const {return(ix * ipp - x0);}
  double M2Y (double iy) const {return(iy * ipp - y0);}
  double IPP () const {return ipp;}
  double PPI () const {return(1.0 / ipp);}
  double P2I (double pels) const {return(ipp * pels);}
  double I2P (double ins) const {return(ins / ipp);}
  int PELS (double ins) const {return ROUND(ins / ipp);}

  // map height conversion routines
  double ZPI () const {return(253.0 / (zhi - zlo));}
  double IPZ () const {return((zhi - zlo) / 253.0);}
  double Z2I (double z) const   {return(ztab + DZ2I(z));}   
  int I2Z (double ht) const     {return DI2Z(ht - ztab);}   
  double DZ2I (double dz) const {return(zlo + (dz - 1.0) * (zhi - zlo) / 253.0);} 
  int DI2Z (double dht) const   {return(ROUND(253.0 * (dht - zlo) / (zhi - zlo)) + 1);}
  int ZDEV (double dht) const   {return ROUND(253.0 * dht / (zhi - zlo));}

  // plane fitting
  int Restricted (int cam =0) const;
  double EstPose (double& t, double& r, double& h, const jhcImg& d16, 
                  int cam =0, double ztol =4.0);
  int EstDev (jhcImg& devs, double dmax =2.0, double ztol =4.0);
  int PlaneDev (jhcImg& devs, const jhcImg& hts, double dmax =2.0, double search =0.0, const jhcRoi *area =NULL);
  void PlaneVals () const;
  double PickPlane (double hpref, int amin =200, int bin =4, double flip =0.0);

  // surface intersection
  int BeamFill (jhcImg& dest, double z =0.0, int r =255, int g =255, int b =255) const; 
  int BeamEmpty (jhcImg& dest, double z =0.0, int t =1, int r =255, int g =255, int b =255) const; 
  int BeamCorners (double *x, double *y, double z =0.0) const;

  // debugging graphics
  void AdjGeometry (int cam =0);
  int ShowCams (jhcImg& dest,           int t =1, int r =-5, int g =0, int b =0) const;
  int CamLoc (jhcImg& dest,    int cam, int t =1, int r =-5, int g =0, int b =0) const;
  int ShowZones (jhcImg& dest,          int t =1, int r =-5, int g =0, int b =0) const;
  int CamZone (jhcImg& dest,   int cam, int t =1, int r =-5, int g =0, int b =0) const;
  int ShowPads (jhcImg& dest,           int t =1, int r =-5, int g =0, int b =0) const;
  int Footprint (jhcImg& dest, int cam, int t =1, int r =-5, int g =0, int b =0) const;
  int AreaEst (jhcImg& dest,   int cam, int t =1, int r =-5, int g =0, int b =0) const;


// PROTECTED MEMBER FUNCTIONS
protected:
  // processing parameters
  int cam_params (int n, const char *fname);
  int flat_params (int n, const char *fname);
  int map_params (const char *fname);
  int plane_params (const char *fname);
  int beam_params (const char *fname);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void dealloc ();
  void std_cams ();

  // plane fitting
  int z_err (jhcImg& devs, const jhcImg& hts, double dmax, double lo, double hi) const;


};


#endif  // once




