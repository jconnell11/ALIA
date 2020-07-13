// jhcFindPlane.h : finds flat areas in depth images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2016 IBM Corporation
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

#ifndef _JHCFINDPLANE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFINDPLANE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"             // common vision
#include "Data/jhcParam.h"


//= Finds flat areas in depth images.
// assumes roll is small (--> robot flat for floor, head tilt small for wall)
// breaks image into a number of bands (V for floor, H for wall)
// estimates plane tilt and camera offset in each band
// combines similar bands to generate estimate of plane (with roll)

class jhcFindPlane 
{
// PRIVATE MEMBER VARIABLES
private:
  static const int bands = 8;   /** Number of vertical bands in image. */

  // image points likely on surface
  int pcnt[bands];
  int *ok[bands];
  int *ix[bands], *iy[bands], *iz[bands];
  double *cx[bands], *cy[bands], *cz[bands];                         
  int ilim;

  // results of analyzing each band
  int sort[1000];
  int keep[bands], vpt[bands];
  double fit[bands], ang[bands], off[bands];
  double err, ht, tilt, roll; 


// PROTECTED MEMBER VARIABLES
protected:
  // expected image size
  int iw, ih;


// PUBLIC MEMBER VARIABLES
public:
  // debugging print statements
  int noisy;

  // line fitting and band merging parameters
  jhcParam fps;
  int vstep, hstep, pmin, bmin;
  double fmin, dev, htol, atol;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization 
  ~jhcFindPlane ();
  jhcFindPlane ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL) {return fit_params(fname);}
  int SaveVals (const char *fname) const {return fps.SaveVals(fname);}

  // configuration
  void SetSize (const jhcImg& ref);
  void SetSize (int x, int y);

  // access to image sizes
  int XDim () const {return iw;}
  int YDim () const {return ih;}
  int NumBands () const {return bands;}

  // access to estimation results
  double EstErr () const  {return err;}
  double EstHt () const   {return ht;}
  double EstTilt () const {return tilt;}
  double EstRoll () const {return roll;}

  // access to line merging results
  int LineKeep (int b) const   {return keep[__max(0, __min(b, bands - 1))];}
  int LineCnt (int b) const    {return  vpt[__max(0, __min(b, bands - 1))];}
  double LineFit (int b) const {return  fit[__max(0, __min(b, bands - 1))];}
  double LineOff (int b) const {return  off[__max(0, __min(b, bands - 1))];}
  double LineAng (int b) const {return  ang[__max(0, __min(b, bands - 1))];}

  // main functions
  int SurfaceData (double& ha, double& va, double& d, const jhcImg& d16, 
                   const jhcRoi *area =NULL, int vert =0);
  double Fit3D (double& t, double& r, double& h, const jhcImg& d16, 
                int dir =0, double dh =0.0, double ksc =0.9659, double kf =525.0);  // was 0.9624 then 1

  // debugging functions
  int Seeds (jhcImg& dest, int detail =1, int dir =-1);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int fit_params (const char *fname);

  // plane from lines
  int form_clique (double s[], int group[]) const;
  int add_compatible (double s[], int mark[], int base) const;
  int try_band (double s[], int b) const;

  // plane fitting
  double init_stats (double s[], int b) const;
  void add_point (double s[], double x, double y, double z) const;
  double plane_err (double s[]) const;

  // line fitting
  void ht_gate (int b, double dh, double h, double t);
  int pick_start (int b);
  int sort_band (int b);
  void line_fit (int b, int first);
  void add_line (double s[], double y, double z) const;
  double line_vals (double& m, double& b, double s[]) const;

  // seed points
  void vbot_bands (const jhcImg& d16, double dsc, double finv);
  void vtop_bands (const jhcImg& d16, double dsc, double finv);
  void hlf_bands (const jhcImg& d16, double dsc, double finv);
  void hrt_bands (const jhcImg& d16, double dsc, double finv);
  void alloc_pts (int n);
  void dealloc_pts ();

};


#endif  // once




