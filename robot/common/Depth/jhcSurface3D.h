// jhcSurface3D.h : interprets scene relative to a flat plane
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2020 IBM Corporation
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

#ifndef _JHCSURFACE3D_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSURFACE3D_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"             // common vision

#include "Geometry/jhcMatrix.h"      // common robot
#include "Geometry/jhcPlaneEst.h"


//= Interprets scene relative to a flat plane.

class jhcSurface3D : protected jhcPlaneEst
{
// PRIVATE MEMBER VARIABLES
private:
  jhcMatrix i2m;               // transform from image to map
  jhcMatrix xform;             // refine matrix for floor projection
  int iw, ih, hw, hh;          // expected image size


// PROTECTED MEMBER VARIABLES
protected:
  jhcMatrix m2i;               // transform from map to image
  jhcImg wxyz;                 // cached pixel coordinates


// PUBLIC MEMBER VARIABLES
public:
  double cx, cy, cz, p0, t0, r0, kf, ksc;  // camera pose, focal length, and scaling
  double z0, z1, zmax, ipp, dmax;          // map formation parameters


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  jhcSurface3D ();
  void SetSize (const jhcImg& ref, int full =0);
  void SetSize (int x =640, int y =480, int full =0);

  // configuration
  void SetPose (const double *p6) 
   {p0 = p6[0]; t0 = p6[1]; r0 = p6[2]; cx = p6[3]; cy = p6[4]; cz = p6[5];}
  void SetCamera (double x =0.0, double y =0.0, double z =100.5) 
    {cx = x; cy = y; cz = z;}
  void SetView (double p =90.0, double t =-20.0, double r =0.0) 
    {p0 = p; t0 = t; r0 = r;}
  void SetOptics (double f =525.0, double sc =0.9659) 
    {kf = f; ksc = sc;}
  void SetProject (double lo =36.0, double hi =78.0, double cut =84.0, double sc =0.3, double rng =240.0)
    {z0 = lo; z1 = hi; zmax = cut; ipp = sc; dmax = rng;}

  // access to image sizes
  int XDim () const  {return iw;}
  int YDim () const  {return ih;}
  int XDim2 () const {return hw;}
  int YDim2 () const {return hh;}

  // local plane fitting
  double CamCalib (double& t, double& r, double& h, const jhcImg& src, double z0, double ztol, 
                   double zlo, double zhi, double ipp, double yoff =0.0, const jhcRoi *area =NULL);

  // standard overhead map
  void BuildMatrices (double cpan, double ctilt, double croll, double x0, double y0, double z0);
  int FloorMap0 (jhcImg& dest, const jhcImg& d16, int clr, 
                 double pan, double tilt, double roll, double xcam, double ycam, double zcam);
  int FloorMap (jhcImg& dest, const jhcImg& d16, int clr, 
                double pan, double tilt, double roll, double xcam, double ycam, double zcam);
  int FloorMap2 (jhcImg& dest, const jhcImg& d16, int clr, 
                 double pan, double tilt, double roll, double xcam, double ycam, double zcam, int n =1);
  int FloorColor (jhcImg& rgb, jhcImg& hts, const jhcImg& col, const jhcImg& d16, int clr,
                  double pan, double tilt, double roll, double xcam, double ycam, double zcam);
  double FloorHt (int pixel);
  int FloorPel (double ht);

  // shortenend versions of overhead map functions
  void BuildMatrices ()
    {BuildMatrices(p0, t0 + 90.0, r0, cx, cy, cz);}
  int FloorMap0 (jhcImg& dest, const jhcImg& d16, int clr =1) 
    {return FloorMap0(dest, d16, clr, p0, t0, r0, cx, cy, cz);}    
  int FloorMap (jhcImg& dest, const jhcImg& d16, int clr =1) 
    {return FloorMap(dest, d16, clr, p0, t0, r0, cx, cy, cz);}    
  int FloorMap2 (jhcImg& dest, const jhcImg& d16, int clr =1, int n =1) 
    {return FloorMap2(dest, d16, clr, p0, t0, r0, cx, cy, cz, n);}    
  int FloorFwd0 (jhcImg& dest, const jhcImg& d16, double tilt, double roll, double ht)
    {return FloorMap0(dest, d16, 1, 0.0, tilt, roll, 0.0, 0.0, ht);}
  int FloorFwd (jhcImg& dest, const jhcImg& d16, double tilt, double roll, double ht)
    {return FloorMap(dest, d16, 1, 0.0, tilt, roll, 0.0, 0.0, ht);}
  int FloorFwd2 (jhcImg& dest, const jhcImg& d16, double tilt, double roll, double ht, int n =1)
    {return FloorMap2(dest, d16, 1, 0.0, tilt, roll, 0.0, 0.0, ht, n);}
  int FloorColor (jhcImg& rgb, jhcImg& hts, const jhcImg& col, const jhcImg& d16, int clr =1)
    {return FloorColor(rgb, hts, col, d16, clr, p0, t0, r0, cx, cy, cz);}

  // height analysis with back mapping
  int CacheXYZ (const jhcImg& d16, double cpan, double ctilt, double croll =0.0, double dmax =300.0);
  int Plane (jhcImg& dest, double ipp =0.3, double yoff =0.0, 
             double zoff =0.0, double zrng =2.0, double zmax =72.0, int pos =0); 
  int Slice (jhcImg& dest, double z0 =4.0, double z1 =84.0, 
             double ipp =0.3, double yoff =0.0, int inc =255);
  int MapBack (jhcImg& dest, const jhcImg& src, double z0 =-500.0, double z1 =500.0, 
               double ipp =0.3, double yoff =0.0, int fill =0);

  // reverse mapping (incremental)
  int FrontMask (jhcImg& mask, const jhcImg& d16, double over, double under, const jhcImg& cc, int n);

  // coordinate transformations
  void ToCache (double& mx, double& my, double& mz, 
                double ix, double iy, double iz) const;
  void FromCache (double& ix, double& iy, double& iz, 
                  double mx, double my, double mz) const;
  void WorldPt (double& wx, double& wy, double& wz, 
                double ix, double iy, double iz, double sc =1.0) const;
  void WorldPt (jhcMatrix& w, double ix, double iy, double iz, double sc =1.0) const;
  double ImgPtZ (double& ix, double& iy, 
                 double wx, double wy, double wz, double sc =1.0) const; 
  int ImgPt (double& ix, double& iy, 
             double wx, double wy, double wz, double sc =1.0) const; 
  int ImgRect (jhcRoi& box, double wx, double wy, double wz, 
               double xsz, double zsz, double sc =1.0) const;
  int ImgCube (jhcRoi& box, double wx, double wy, double wz, 
               double xsz, double ysz, double zsz, double sc =1.0) const;
  int ImgPrism (jhcRoi& box, double wx, double wy, double wz, double ang,
                double len, double wid, double ht, double sc =1.0) const;
  int ImgSphere (jhcRoi& box, double wx, double wy, double wz, 
                 double diam, double sc =1.0) const;
  int ImgCylinder (jhcRoi& box, double wx, double wy, double wz, 
                   double diam, double zsz, double sc =1.0) const;
  double ImgScale (double wx, double wy, double wz, double sc =1.0, double test =1.0) const;

  // vector versions
  double ImgPtZ (double& ix, double& iy, const jhcMatrix& w, double sc =1.0) const
    {return ImgPtZ(ix, iy, w.X(), w.Y(), w.Z(), sc);}
  int ImgPt (double& ix, double& iy, const jhcMatrix& w, double sc =1.0) const
    {return ImgPt(ix, iy, w.X(), w.Y(), w.Z(), sc);}
  int ImgRect (jhcRoi& box, const jhcMatrix& w, double xsz, double zsz, double sc =1.0) const
    {return ImgRect(box, w.X(), w.Y(), w.Z(), xsz, zsz, sc);}
  int ImgCube (jhcRoi& box, const jhcMatrix& w, double xsz, double ysz, double zsz, double sc =1.0) const
    {return ImgCube(box, w.X(), w.Y(), w.Z(), xsz, ysz, zsz, sc);}
  int ImgSphere (jhcRoi& box, const jhcMatrix& w, double diam, double sc =1.0) const
    {return ImgSphere(box, w.X(), w.Y(), w.Z(), diam, sc);}
  int ImgCylinder (jhcRoi& box, const jhcMatrix& w, double diam, double zsz, double sc =1.0) const
    {return ImgCylinder(box, w.X(), w.Y(), w.Z(), diam, zsz, sc);}
  double ImgScale (const jhcMatrix& w, double sc =1.0, double test =1.0) const
    {return ImgScale(w.X(), w.Y(), w.Z(), sc, test);}

  // debugging functions
  int Heights (jhcImg& dest, double zoff =0.0, double zrng =2.0, int pos =0);
  int Ground (jhcImg& dest, double th =1.0);


};


#endif  // once




