// jhcShift.h : image and patch alignment via rigid shifting
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2003-2013 IBM Corporation
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

#ifndef _JHCSHIFT_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSHIFT_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Performs image and patch alignment via rigid shifting.

class jhcShift 
{
// PRIVATE MEMBER VARIABLES
private:
  static int instances;
  static UC8 *third;

  jhcImg tmp, tmp2, tmp3;


// PUBLIC MEMBER VARIABLES
public:
  double rdx, rdy, bdx, bdy;


// PUBLIC MEMBER FUNCTIONS
public:
  jhcShift ();
  ~jhcShift ();

  // image alignment
  double AlignCross (double *xoff, double *yoff, const jhcImg& src, const jhcImg& ref, const jhcImg *mask, 
                     int xlo, int xhi, int ylo, int yhi, int samp =4, int track =0) const;
  double AlignFull (double *xoff, double *yoff, const jhcImg& src, const jhcImg& ref, const jhcImg *mask,
                    int xlo, int xhi, int ylo, int yhi, int samp =4, int track =0) const;

  // line by line shifts
  int EstWobble (double *vdx, const jhcImg& src, const jhcImg& ref, const jhcImg& mask,       
                 double fdx =0.0, double fdy =0.0, 
                 int wx =2, int samp =1, int sm =0, int mode =0) const; 

  // patch finding
  double PatchFull (double *xoff, double *yoff, const jhcImg& src, const jhcImg& ref, 
                    int xlo, int xhi, int ylo, int yhi, int samp =1, int track =0) const;
  double PatchCross (double *xoff, double *yoff, const jhcImg& src, const jhcImg& ref,
                     int xlo, int xhi, int ylo, int yhi, int samp =1, int track =0) const;

  // undoing camera motion
  int FixShift (jhcImg& dest, const jhcImg& src, const jhcImg& ref, int xrng =8, int yrng =8, 
                double *fx =NULL, double *fy =NULL);
  int CrispColor (jhcImg& dest, const jhcImg& src, int rng =1, int samp =1);


// PRIVATE MEMBER FUNCTIONS
private:
  double d2 (double x1, double y1, double x2, double y2) const;
  double parabolic (int loc, double before, double here, double after, 
                    double def, double dip =0.0) const;
  double offset_sad (int dx, int dy, int samp, 
                     const jhcImg& src, const jhcImg& ref, const jhcImg *mask) const;
  double offset_sad_all (int dx, int dy, int samp, const jhcImg& src, const jhcImg& ref) const;                     
  double line_sad0 (const UC8 *s0, const UC8 *r0, 
                    const UC8 *m0, int xr, int xtop, int samp) const;  
  double line_sad1 (const UC8 *s0, const UC8 *r0, 
                    const UC8 *m0, int xr, int xtop, int samp, int run) const;
  double line_sad2 (const UC8 *s0, const UC8 *r0, 
                    const UC8 *m0, int xr, int xtop, int samp, int w) const;                           
  double patch_sad (int dx, int dy, int samp, const jhcImg& src, const jhcImg& ref) const;

  void get_mono (jhcImg& dest, const jhcImg& src) const;
  void frac_samp (jhcImg& dest, const jhcImg& src, double dx, double dy) const;
  void frac_samp3 (jhcImg& dest, const jhcImg& src, double dx, double dy) const;

};


#endif




