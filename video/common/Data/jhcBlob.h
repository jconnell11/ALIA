// jhcBlob.h : list contains SRI blob parameters
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
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

#ifndef _JHCBLOB_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBLOB_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcArr.h"
#include "Data/jhcImg.h"
#include "Data/jhcBBox.h"


//= List contains SRI blob parameters.
// Computes and manipulates pixel moments.
// Extension of BBox class to contain several more co-indexed arrays.
// This form chosen because individual blobs are not very useful.
// Feature array indices run from 1 to valid - 1 inclusive (0 not used).

class jhcBlob : public jhcBBox
{
protected:  
  int *xsum, *ysum;
  long long *x2sum, *y2sum, *xysum;
  double *XAvg, *YAvg, *Width, *Aspect, *Angle, *Val;
  UC8 *Label;

public:
  ~jhcBlob ();
  jhcBlob ();
  jhcBlob (int ni);
  jhcBlob (const jhcBlob& ref);
  jhcBlob (const jhcBBox& ref);
  void SetSize (int ni);
  void SetSize (const jhcBBox& ref);

  // read only access
  int BlobArea (int index) const;
  double BlobValue (int index, double sc =1.0) const;
  double BlobAspect (int index) const;
  double BlobAngle (int index, int fix =0) const;
  double BlobAngleEcc (int index, double eth =1.2, int fix =0) const;
  double BlobWidth (int index) const;
  double BlobLength (int index) const;
  int BlobCentroid (double *x, double *y, int index) const;
  double BlobX (int index) const
    {return(((index < 0) || (index >= total)) ? 0.0 : XAvg[index]);}
  double BlobY (int index) const
    {return(((index < 0) || (index >= total)) ? 0.0 : YAvg[index]);}
  int BlobMajor (double& x0, double& y0, double& x1, double& y1, int index) const;
  int BlobMinor (double& x0, double& y0, double& x1, double& y1, int index) const;

  // feature extraction
  int FindParams (const jhcImg& src, int append =0, int val0 =0);
  int AvgEach (const jhcImg& src, const jhcImg& data, int clr =1);
  int MinEach (const jhcImg& src, const jhcImg& data, int nz =0, int clr =1);
  int MaxEach (const jhcImg& src, const jhcImg& data, int clr =1);
  void ResetBlobs (int val0 =0);
  void ResetLims (int w, int h);
  int CopyAll (const jhcBlob& src);

  // blob selection
  int MaxArea (int sth =1) const;
  int KingBlob (int sth =1) const;
  int Highest (int sth =1) const;
  int Nearest (double x, double y, int sth =1) const;
  int MinY (int sth =1) const;
  int MaxY (int sth =1) const;

  // blob elimination
  void AspectThresh (double ath, int sth =0, int good =1, int bad =0);
  void WidthThresh (double wid, int sth =0, int good =1, int bad =0);
  void LengthThresh (double len, int sth =0, int good =1, int bad =0);
  void ValueThresh (double th, int sth =0, int good =1, int bad =0);
  void HorizThresh (double x, int sth =0, int good =1, int bad =0);
  void VertThresh (double y, int sth =0, int good =1, int bad =0);
  void AngleThresh (double ath, double emin =1.0, 
                    int sth =0, int good =1, int bad =0);
  void AngleKeep (double alo, double ahi, double emin =1.0, 
                  int sth =0, int good =1, int bad =0);
  int CountValid (int sth =0) const;  
  int CountStatus (int sth =1) const;
  int Nth (int n) const;

  // region tagging
  int SeedCenters (jhcImg& dest) const;

  // visualization
  int MapParam (jhcImg& dest, const jhcImg& src, int p =4, double lim =0.0) const;
  int MapValue (jhcImg& dest, const jhcImg& src) const
    {return MapParam(dest, src, 12);}

  // axis box functions
  int Profiles (jhcArr& lf, jhcArr& rt, const jhcImg& src, int i, double eth =0.0) const;
  int ABox (double& xm, double& ym, double& len, double& wid, const jhcImg& src, int i, double eth =0.0) const;
  int ABoxCorners (double x[], double y[], double xm, double ym, double len, double wid, int i, double eth =0.0) const;
  void ABoxEnd (double& tx, double& ty, double x[], double y[], int dir =1) const;
  int ABoxFrac (jhcImg& dest, const jhcImg& src, double lo, double hi, int i, double eth =0.0) const;


protected:
  void DeallocBlob ();
  void InitBlob ();
  double GetParam (int i, int p) const;

};


#endif




