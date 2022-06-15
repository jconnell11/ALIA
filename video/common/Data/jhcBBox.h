// jhcBBox.h : manipulation of object bounding boxes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2017 IBM Corporation
// Copyright 2021 Etaoin Systems
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

#ifndef _JHCBBOX_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBBOX_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcRoi.h"
#include "Data/jhcImg.h"


//= Manipulation of object bounding boxes.
// List of regions with an observation count and possibly a velocity.
// This form chosen because individual bboxes are not very useful.
// Feature array indices run from 1 to valid - 1 inclusive (0 not used).

class jhcBBox 
{
protected:
  int fxlo, fxhi, fylo, fyhi;
  int total, valid;
  int *status, *count, *pixels;
  double *aux, *vx, *vy, *vz;
  int *xlo, *xhi, *ylo, *yhi;
  jhcRoi *items;

public:
  ~jhcBBox ();
  jhcBBox ();
  jhcBBox (const jhcBBox& ref);
  jhcBBox (int ni);
  void SetSize (const jhcBBox& ref);
  void SetSize (int ni);
  int Size () const {return total;}    /** Maximum number of boxes allowed in list. */
  int Active () const {return valid;}  /** Current number of valid boxes in list.   */

  // raw data access
  jhcRoi *GetRoi (int index);
  const jhcRoi *ReadRoi (int index) const;
  int GetRoi (jhcRoi& dest, int index) const;
  int BoxMidX (int index, double sc =1.0) const;
  int BoxMidY (int index, double sc =1.0) const;
  double BoxAvgX (int index, double sc =1.0) const;
  double BoxAvgY (int index, double sc =1.0) const;
  double BoxLf (int index, double sc =1.0) const;
  double BoxRt (int index, double sc =1.0) const;
  double BoxBot (int index, double sc =1.0) const;
  double BoxTop (int index, double sc =1.0) const;
  double BoxW (int index, double sc =1.0) const;
  double BoxH (int index, double sc =1.0) const;
  int PixelCnt (int index) const;

  // list manipulation
  int GetStatus (int index) const;
  int GetCount (int index) const;
  double GetAux (int index) const;
  int GetSpeed (double *x, double *y, int index =0) const;
  double GetZoom (int index) const;
  int SetStatus (int index, int val);
  int SetCount (int index, int val);
  int SetAux (int index, double val);
  int SetSpeed (int index, double x, double y);
  int SetZoom (int index, double z);
  int ClearItem (int index);
  int CopyItem (int index, const jhcBBox& src, int si);

  // feature extraction
  int FindBBox (const jhcImg& src, int val0 =0);
  int TopBoxes (const jhcImg& src, double f0 =0.5, double f1 =1.5, double mag =1.0);
  int CountOver (int sth, int shrink);
  int CountOver (int sth =1) const;
  int IndexOver (int n, int sth =1) const;

  // selection
  int MaxAreaBB (int sth =1) const;
  int MaxBB (int sth =1) const;
  int Biggest (int sth =1) const;
  int Smallest (int sth =1) const;
  int RightBB (int sth =1) const;
  int LeftBB (int sth =1) const;
  int TopBB (int sth =1) const;
  int GapBottomBB (int sth =1) const;
  int ClosestBB (int x, int y, int sth =1) const;
  int BottomBB (int sth =1) const;
  int BestOverlap (const jhcRoi& target, double oth =0.0) const;

  // list combination
  void CopyAll (const jhcBBox& src);
  void AddItems (const jhcBBox& xtra);
  void ApplyVel (int zoom =0);
  void ComputeVel (const jhcBBox& last, double mix =0.5);
  void RemSimilar (const jhcBBox& known, double close =0.5);
  void MatchTo (jhcBBox& xtra, double close =0.8, double fmv =0.5, double fsz =0.5);

  // value alteration
  void ResetAll (int val0 =0);
  void AllStatus (int val =0);
  void RemapStatus (int old, int now);
  void AbsStatus ();
  void BinStatus (int sth =1, int val =1);
  void CheckCounts (int add, int del);
  void PassHatched (int schk =2, double frac =0.5);
  void ClipAll (int xdim, int ydim);
  void ShapeAll (double alo, double ahi);

  // elimination
  void KeepOnly (int focus);
  void RemBorder (int w, int h, 
                  int dl =10, int dr =-1, int db =-1, int dt =-1, 
                  int sth =0, int bad =0);
  void RemBorder (const jhcImg& ref, int bd =10, int sth =0, int bad =0);
  void RemTouch (const jhcRoi& ref);
  void PixelThresh (int ath, int max_ej =4, int sth =0, int bad =0);
  void AreaThresh (int ath, int sth =0, int good =1, int bad =0);
  void AreaThreshBB (int ath, int sth =0, int bad =0);
  void AspectThreshBB (double asp, int sth =0, int good =1, int bad =0);
  void ElongThreshBB (double asp, int sth =0, int good =1, int bad =0);
  void FillThreshBB (double fill, int sth =0, int good =1, int bad =0);
  void WidthThreshBB (int wid, int sth =0, int good =1, int bad =0);
  void HeightThreshBB (int ht, int sth =0, int good =1, int bad =0);
  void DimsThreshBB (int wid, int ht, int sth =0, int good =1, int bad =0);
  void InsideThreshBB (const jhcRoi& area, int outside =0, int sth =0, int good =1, int bad =0);
  void OverlapThreshBB (const jhcRoi& area, int cnt =1, int sth =0, int good =1, int bad =0);
  void ContainThreshBB (const jhcRoi& area, int outside =0, int sth =0, int good =1, int bad =0);
  void YTopThresh (int ymin, int sth =0, int good =1, int bad =0);
  void YBotThresh (int ymin, int sth =0, int good =1, int bad =0);
  void YClipThresh (int ytop, double maxfrac =0.1, int sth =0, int good =1, int bad =0);

  // pixel statistics
  int OverlapMask (const jhcImg& labels, const jhcImg& marks, int cnt =1, int good =1, int bad =0);
  int OverlapRoi (const jhcImg& labels, const jhcRoi& area, int cnt =1, int good =1, int bad =0); 
  int OverlapBest (const jhcImg& labels, const jhcImg& marks, int sth =0);

  // region tagging
  int Poison (const jhcImg& labels, const jhcImg &marks);
  int PoisonOver (const jhcImg& labels, const jhcImg &marks, int th =0);
  int PoisonWithin (const jhcImg& labels, const jhcRoi& area);
  int Retain (const jhcImg& labels, const jhcImg &marks);
  int RetainOver (const jhcImg& labels, const jhcImg &marks, int th =0);

  // individual components
  int MarkBlob (jhcImg& dest, const jhcImg& src, int index,  int val =255, int only =1) const;
  int TightMask (jhcImg& dest, const jhcImg& src, int index, int pad =0) const;
  int LowestPels (jhcImg& dest, const jhcImg& src, int index, int cnt, int clr =0) const;
  int HighestPels (jhcImg& dest, const jhcImg& src, int index, int cnt, int clr =0) const;

  // visualization
  int OverGate (jhcImg& dest, const jhcImg& src, int sth =1) const;
  int DrawPatch (jhcImg& dest, int sth =1) const;
  int DrawOutline (jhcImg& dest, int sth =1, double mag =1.0) const;
  int MarkOutline (jhcImg& dest, int sth =1, double mag =1.0, int t =1, int r =255, int g =0, int b =255) const;
  int OnlyOutline (jhcImg& dest, int sth =1, double mag =1.0, int t =1, int r =255, int g =0, int b =255) const;
  int ThreshValid (jhcImg& dest, const jhcImg& src, int sth =1, int mark =255) const;
  int ValidPixels (jhcImg& dest, const jhcImg& src, int mark =255, int key =1) const;
  int CopyRegions (jhcImg& dest, const jhcImg& src) const;
  int CopyOnly (jhcImg& dest, const jhcImg& src, int sth) const;
  int MarkOver (jhcImg& dest, const jhcImg& src, int sth =0, int val =255, int clr =1) const;
  int MarkRegions (jhcImg& dest, const jhcImg& src, int val =255, int clr =1) const
    {return MarkOver(dest, src, 0, val, clr);}
  int MarkRange (jhcImg& dest, const jhcImg& src, int slo =0, int shi =1, int val =255, int clr =1) const;
  int MarkOnly (jhcImg& dest, const jhcImg& src, int sth =0, int val =255, int clr =1) const
    {return MarkRange(dest, src, sth, sth, val, clr);}


protected:
  void DeallocBBox ();
  void InitBBox ();
  int bd_touch (const jhcRoi& box);

};


#endif




