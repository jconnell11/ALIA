// jhcCamCal.h : camera calibration using a known floor rectangle
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2016 IBM Corporation
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

#ifndef _JHCCAMCAL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCCAMCAL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"        // common video

#include "Geometry/jhcMatrix.h"   // common robot


//= Camera calibration using a known floor rectangle.

class jhcCamCal 
{
// PRIVATE MEMBER VARIABLES
private:
  jhcMatrix i2f, f2i, i2w, w2i;
  int cnum, iw, ih;
  double mx, my, mfx, mfy, pan, tilt, roll, focal, aspect;  

// PUBLIC MEMBER VARIABLES
public:
  // camera geometry and rectangle target description
  jhcParam gps;
  double cx0, cy0, cz0, rcx, rcy, tz, rwid, rht;

  // calibration points in image (rectangle corners)
  jhcParam ips;
  double cx[4], cy[4];

  // calibration points in world (rectangle corners)
  jhcParam wps;
  double wx[4], wy[4];


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcCamCal (int cam =0);
  void SetNum (int cam) {cnum = cam;}
  void SetSize (int w, int h);
  int XDim () const {return iw;}
  int YDim () const {return ih;}

  // processing parameter manipulation 
  int LoadCfg (const char *fname =NULL);
  int SaveCfg (const char *fname) const;
  void SetGeom (double xcam, double ycam, double zcam, 
                double xtarg, double ytarg, double ztarg, double wtarg, double htarg);  
  void WorldPts (double x0, double y0, double x1, double y1,
                 double x2, double y2, double x3, double y3);
  void ImagePts (double x0, double y0, double x1, double y1,
                 double x2, double y2, double x3, double y3);

  // core calibration
  void SetCam (const jhcMatrix& pos) {SetCam(pos.X(), pos.Y(), pos.Z());}
  void SetCam (double x, double y, double z) {cx0 = x; cy0 = y; cz0 = z;}
  void RectCent (double x, double y, double z) {rcx = x; rcy = y; tz = z;} 
  void ImageRect (const double *rx, const double *ry, double *ax =NULL, double *ay =NULL);
  double Calibrate (double jitter =0.0);

  // physics based read-only results 
  double Pan () const    {return pan;}
  double Tilt () const   {return tilt;}
  double Roll () const   {return roll;}
  double Focal () const  {return focal;}
  double Aspect () const {return aspect;}
  double FloorMidX () const {return mfx;}
  double FloorMidY () const {return mfy;}

  // main functions
  void FromFloor (double& ix, double& iy, double fx, double fy) const;
  void ToFloor   (double& fx, double& fy, double ix, double iy) const;
  void FromWorld (double& ix, double& iy, double wx, double wy, double wz) const;
  void ToWorld   (double& wx, double& wy, double wz, double ix, double iy) const;
  double WorldHt (double wy, double ix, double iy) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int geom_params (const char *fname);
  int world_params (const char *fname);
  int image_params (const char *fname);

  // core calibration
  void world_rect ();
  void alt_marks (double *ix, double *iy, double jitter, int var) const;
  double mark_error (const double *ix, const double *iy) const;

  // physics based transform
  void cal_core (const double *ix, const double *iy);
  void est_angles (double& p, double& t, double& r, double fx0, double fy0) const;
  double adj_scale (double &foc, const double *ix, const double *iy, 
                    const double *fx, const double *fy) const;

  // basic homography
  void homography (jhcMatrix& h, const double *ix, const double *iy, 
                   const double *fx, const double *fy) const;
  void homo_norm (double& dx, double& dy, double& sc, double *nx, double *ny, 
                  const double *x, const double *y) const;

};


#endif  // once




