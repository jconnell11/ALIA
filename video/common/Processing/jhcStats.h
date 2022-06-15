// jhcStats.h : image statistics
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
// Copyright 2022 Etaoin Systems
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

#ifndef _JHCSTATS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSTATS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Pull out various numbers characterizing patches of pixels.

class jhcStats
{
// PUBLIC MEMBER FUNCTIONS
public:
  // maxima 
  int MaxVal (const jhcImg& src) const;
  int MaxVal (const jhcImg& src, int x, int y, int w, int h) const;
  int MaxVal (const jhcImg& src, const int *specs) const;
  int MaxVal (const jhcImg& src, const jhcRoi& patch) const;
  int MaxVal_16 (const jhcImg& src) const;  
  int MaxVal_16 (const jhcImg& src, const jhcRoi& patch) const;
  int MaxLoc (int *mx, int *my, const jhcImg& src, const jhcRoi& area) const;
  int MaxCentH (int *mx, int *my, const jhcImg& src, const jhcRoi& area) const;

  // minima
  int MinVal (const jhcImg& src) const;
  int MinVal (const jhcImg& src, int x, int y, int w, int h) const;
  int MinVal (const jhcImg& src, const int *specs) const;
  int MinVal (const jhcImg& src, const jhcRoi& patch) const;
  int MinVal_16 (const jhcImg& src) const;  
  int MinVal_16 (const jhcImg& src, const jhcRoi& patch) const;

  // monochrome averages
  double AvgVal (const jhcImg& src, int th =0) const;
  double AvgVal (const jhcImg& src, int x, int y, int w, int h, int th =0) const;
  double AvgVal (const jhcImg& src, const int *specs, int th =0) const;
  double AvgVal (const jhcImg& src, const jhcRoi& patch, int th =0) const;
  double AvgVal_16 (const jhcImg& src, const jhcRoi& patch, int th =0) const;
  double AvgOver (const jhcImg& src, const jhcImg& mask, int th =0) const;
  double AvgUnder (const jhcImg& src, const jhcImg& mask, int th =0) const;
  double AvgDiff (jhcImg& imga, jhcImg& imgb) const;

  // color averages
  int AvgRGB (double *r, double *g, double *b, const jhcImg& src) const;
  int AvgRGB (double *r, double *g, double *b, const jhcImg& src, 
              int x, int y, int w, int h) const;
  int AvgRGB (double *r, double *g, double *b, const jhcImg& src, const int *specs) const;
  int AvgRGB (double *r, double *g, double *b, const jhcImg& src, const jhcRoi& patch) const;
  int AvgOverRGB (double *r, double *g, double *b, const jhcImg& src, const jhcImg& mask, int th =0) const;

  // pixel counting
  int AnyNZ (const jhcImg& src) const;
  int AnyOver (const jhcImg& src, int th) const;
  int AnyOver_16 (const jhcImg& src, const jhcRoi& patch, int th) const;
  int CountOver (const jhcImg& src, int th =0) const;
  int CountOver4 (const jhcImg& src, int th =0) const;
  int CountOver_16 (const jhcImg& src, int th =0) const;
  int CountOver (const jhcImg& src, const jhcRoi& patch, int th =0) const;
  int CountUnder (const jhcImg& src, int th =255) const;
  int CountUnder (const jhcImg& src, const jhcRoi& patch, int th =255) const;
  double FracOver (const jhcImg& src, int th =0) const;
  double FracOver (const jhcImg& src, const jhcRoi& patch, int th =0) const;
  double FracUnder (const jhcImg& src, int th =255) const;
  double FracUnder (const jhcImg& src, const jhcRoi& patch, int th =0) const;

  // border pixel counting
  int SideCount (const jhcImg& src, int side, int th =0) const;
  double SideFrac (const jhcImg& src, int side, int th =0) const;

  // shape functions
  int Centroid (double *xc, double *yc, const jhcImg& src, int th =0) const;
  int Centroid (double *xc, double *yc, const jhcImg& src, const jhcRoi& patch, int th =0) const;
  int Shape (double *xc, double *yc, double *ecc, double *ang, 
             const jhcImg& src, const jhcRoi& patch, int th =0) const;
  double Cloud (double *xc, double *yc, double *sdx, double *sdy, 
                const jhcImg& src, const jhcRoi& patch, int th =0) const;
  double Ellipse (double *xc, double *yc, double *wid, double *len, 
                  const jhcImg& src, const jhcRoi& patch, int th =0) const;
  int RegionNZ (jhcRoi &dest, const jhcImg& src, int th =0) const;
  int PtMaxY (int& px, int& py, const jhcImg& src, int th =0, int bias =1) const; 
  double NearPt (int& px, int& py, const jhcImg& src, int tx, int ty, int th =0) const;
  double NearCent (int& px, int& py, const jhcImg& src, int th =0) const
    {return NearPt(px, py, src, src.XDim() >> 1, src.YDim() >> 1, th);}
  double NearSect (int& px, int& py, const jhcImg& src, double ang, double dev, int th =0) const;


};


#endif




