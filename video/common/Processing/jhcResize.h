// jhcResize.h : standard resizing functions for images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#ifndef _JHCRESIZE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCRESIZE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Library functions for changing the size of an image.

class jhcResize
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg t2;
  int *temp;
  int tsize;


// PUBLIC MEMBER FUNCTIONS
public:
  ~jhcResize ();
  jhcResize ();

  // sampling methods
  int ForceSize (jhcImg& dest, const jhcImg& src, int style =1);
  int Sample (jhcImg& dest, const jhcImg& src) const;
  int SampleN (jhcImg& dest, const jhcImg& src) const;
  int Double (jhcImg& dest, const jhcImg& src) const;
  int DoubleRGB (jhcImg& dest, const jhcImg& src) const;
  int Decimate (jhcImg& dest, const jhcImg& src, int factor) const;
  int DecimateRGB (jhcImg& dest, const jhcImg& src, int factor) const;
  int Decimate_16 (jhcImg& dest, const jhcImg& src, int factor) const;
  jhcImg *Smallest (jhcImg& alt, jhcImg& src) const;

  // averaging methods
  int Smooth (jhcImg& dest, const jhcImg& src);
  int SmoothN (jhcImg& dest, const jhcImg& src);
  int DimScaled (int big, int sm, int sc) const;
  int AvgHalf (jhcImg& dest, const jhcImg& src) const;
  int AvgHalfRGB (jhcImg& dest, const jhcImg& src) const;
  int AvgThird (jhcImg& dest, const jhcImg& src) const;
  int AvgThirdRGB (jhcImg& dest, const jhcImg& src) const;
  int MinHalf_16 (jhcImg& dest, const jhcImg& src) const;
  int Blocks (jhcImg& dest, const jhcImg& src, int sx, int sy, int bw, int bh) const;
  int Blocks_16 (jhcImg& dest, const jhcImg& src, int sx, int sy, int bw, int bh) const;

  // non-integer resizing
  int Interpolate (jhcImg& dest, const jhcImg &src, const jhcRoi& a) const;
  int Interpolate (jhcImg& dest, const jhcImg &src, int ax, int ay, int aw, int ah) const;
  int InterpolateNZ (jhcImg& dest, const jhcImg &src, const jhcRoi& a) const;
  int InterpolateNZ (jhcImg& dest, const jhcImg &src, int ax, int ay, int aw, int ah) const;
  int Resample (jhcImg& dest, const jhcImg& src, double cx, double cy, double xsc, double ysc =0.0) const;
  int Resample_16 (jhcImg& dest, const jhcImg& src, double cx, double cy, double xsc, double ysc =0.0) const;
  int FillFrom (jhcImg& dest, const jhcImg& src, int conform =0) const;
  int Bicubic (jhcImg& dest, const jhcImg& src, int conform =0);

  // rigid transforms
  int Rigid (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy, 
             double px, double py, int def =0, double xsc =1.0, double ysc =1.0) const;
  int RigidRGB (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy, 
                double px, double py, int r =0, int g =0, int b =0, double xsc =1.0, double ysc =1.0) const;
  int RigidMix (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy, 
                double px, double py, int def =0, double xsc =1.0, double ysc =1.0) const;
  int RigidMixRGB (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy, 
                   double px, double py, int r =0, int g =0, int b =0, double xsc =1.0, double ysc =1.0) const;
  int RigidMixNZ (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy, 
                  double px, double py, double xsc =1.0, double ysc =1.0) const;

  // compositing and insertion
  int Insert (jhcImg& dest, const jhcImg& src, int xoff =0, int yoff =0) const;
  int Extract (jhcImg& dest, const jhcImg& src, int xoff =0, int yoff =0) const;
  int ExtField (jhcImg& dest, const jhcImg& src, int xoff =0, int yoff =0, int f =0) const;
  int CopyPart (jhcImg& dest, const jhcImg& src, int rx, int ry, int rw, int rh) const;
  int CopyPart (jhcImg& dest, const jhcImg& src, const jhcRoi& a) const;
  int ExtRotateRGB (jhcImg& dest, const jhcImg& src, double cx, double cy, double ang) const;
  int Zoom (jhcImg& dest, const jhcImg& src, int cx, int cy, int mag =1) const;
  jhcImg *Image4 (jhcImg& alt, jhcImg& src) const;

  // simple image reshaping
  int FlipH (jhcImg& dest, const jhcImg& src) const;
  int FlipH (jhcImg& dest) const;
  int Mirror (jhcImg& dest, const jhcImg& src, int rev =1) const;
  int FlipV (jhcImg& dest, const jhcImg& src) const;
  int FlipV (jhcImg& dest) const;
  int UpsideDown (jhcImg& dest, const jhcImg& src) const;
  int UpsideDown (jhcImg& dest) const;
  int Invert (jhcImg& dest, const jhcImg& src, int inv =1) const;
  int RotateCW (jhcImg& dest, const jhcImg& src) const;
  int RotateCCW (jhcImg& dest, const jhcImg& src) const;

  // subpixel shifting
  int Shift (jhcImg& dest, const jhcImg& src, int dx, int dy) const;
  int Shift (jhcImg& dest, int dx, int dy, int def =0) const;
  int FracShift (jhcImg& dest, const jhcImg& src, double dx, double dy) const;
  int FracSamp (jhcImg& dest, const jhcImg& src, double dx =0.0, double dy =0.0) const;
  int LineShift (jhcImg& dest, const jhcImg& src, const double *vdx, double dy) const;
  int LineSamp (jhcImg& dest, const jhcImg& src, const double *vdx, double dy) const;

  // four panel images
  int QuadX (const jhcImg& ref, int cv =0, int ev =0) const;
  int QuadY (const jhcImg& ref, int ch =0, int eh =0) const;
  jhcImg *QuadSize (jhcImg& target, const jhcImg& src,  
                    int cv =0, int ch =0, int ev =0, int eh =0) const;
  int GetQuad (jhcImg& dest, const jhcImg& src, int n, 
               int cv =0, int ch =0, int ev =0, int eh =0) const;
  int GetHalf (jhcImg& dest, const jhcImg& src, int n, 
               int cv =0, int ev =0, int eh =0) const;

  // NTSC fields
  int MixOddEven (jhcImg& dest, const jhcImg& odd, const jhcImg& even) const;
  int MixOddEven2 (jhcImg& dest, const jhcImg& odd, const jhcImg& even) const;
  int GetOddEven (jhcImg& odd, jhcImg& even, const jhcImg& src) const;
  int GetHalfOE (jhcImg& odd, jhcImg& even, const jhcImg& src) const;
  int GetAvgOE (jhcImg& odd, jhcImg& even, const jhcImg& src) const;


// PROTECTED MEMBER FUNCTIONS
protected:
  void alloc (int sz);
  void dealloc ();

  int generic_sm (jhcImg& dest, const jhcImg& src);

  int FracShift_BW (jhcImg& dest, const jhcImg& src, double dx, double dy) const;
  int FracShift_RGB (jhcImg& dest, const jhcImg& src, double dx, double dy) const;
  int FracSamp_BW (jhcImg& dest, const jhcImg& src, double dx, double dy) const;
  int FracSamp_RGB (jhcImg& dest, const jhcImg& src, double dx, double dy) const;
  int LineShift_BW (jhcImg& dest, const jhcImg& src, const double *vdx, double fdy) const;
  int LineShift_RGB (jhcImg& dest, const jhcImg& src, const double *vdx, double fdy) const;
  int LineSamp_BW (jhcImg& dest, const jhcImg& src, const double *vdx, double fdy) const;
  int LineSamp_RGB (jhcImg& dest, const jhcImg& src, const double *vdx, double fdy) const;
  void edge_dup (jhcImg& dest, int nx, int ny) const;
  void edge_dup_16 (jhcImg& dest, int nx, int ny) const;
  int Bicubic_BW (jhcImg& dest, const jhcImg& src, int conform =0);
  int Bicubic_16 (jhcImg& dest, const jhcImg& src, int conform =0);
  int Bicubic_RGB (jhcImg& dest, const jhcImg& src, int conform =0);

};


#endif




