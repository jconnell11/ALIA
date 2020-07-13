// jhcRoi.h : class for Regions Of Interest
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
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

#ifndef _JHCROI_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCROI_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Axis-parallel rectangular Regions Of Interest.
// (x y) denotes the corner with minimum x and y coordinates
// this is the LOWER LEFT corner wrt images (1st quadrant, like a graph)
// all coordinates always positive and clipped (if w and h are non-zero) 

class jhcRoi 
{
// PROTECTED MEMBER VARIABLES
protected:
  int area;
  int w, h;
  int rw, rh, rx, ry;


// PUBLIC MEMBER FUNCTIONS
public:
  // basic creation 
  jhcRoi ();
  jhcRoi (const jhcRoi& src);
  jhcRoi (int dx, int dy, int dw, int dh, int xclip =0, int yclip =0);
  jhcRoi (int specs[]);

  // initialization
  virtual void CopyRoi (const jhcRoi& src);
  void ClearRoi ();
  void MaxRoi ();
  void SetRoi (int x, int y, int wid =-1, int ht =-1);
  void SetRoi (const int specs[], double f =1.0);
  void SetRoi (double xf, double yf, double wf =0.0, double hf =0.0);
  void SetCenter (double xc, double yc, double wid, double ht =0.0, double f =1.0);
  void DefRoi (int x, int y, int wid =-1, int ht =-1);
  void DefRoi (double xf, double yf, double wf =0.0, double hf =0.0);
  void SetRoiPts (const int lims[]);
  void SetRoiLims (int x0, int y0, int x1, int y1);
  void RoiWithin (double fx, double fy, double fw, double fh, const jhcRoi& ref); 
  void CenterWithin (double cfx, double cfy, double fw, double fh, const jhcRoi& ref); 
  void RoiSpecs (int *x, int *y, int *wid, int *ht) const;
  int *RoiSpecs (int specs[]) const;

  // clipping
  virtual void RoiClip (int xmax, int ymax);
  virtual void RoiClip (const jhcRoi& src);

  // read-only member variable access
  int XDim () const {return w;}                           /** Clipping range maximum X (e.g image width).    */
  int YDim () const {return h;}                           /** Clipping range maximum Y (e.g. image height).  */
  int XLim () const {return(w - 1);}                      /** Highest valid X index (e.g. last valid pixel). */
  int YLim () const {return(h - 1);}                      /** Highest valid Y index (e.g. last valid pixel). */
  int MaxDim () const {return __max(w, h);}               /** Larger of clipping dimensions (image size).    */
  int MinDim () const {return __min(w, h);}               /** Smaller of clipping dimensions (image size).   */
  int XDimF (double f =0.5) const {return ROUND(f * w);}  /** Fraction of image width (default = half).      */
  int YDimF (double f =0.5) const {return ROUND(f * h);}  /** Fraction of image height (default = half).     */

  int RoiX () const {return rx;}                    /** Lowest X of region of interest. */
  int RoiY () const {return ry;}                    /** Lowest Y of region of interest. */
  int RoiW () const {return rw;}                    /** Width of region of interest.    */
  int RoiH () const {return rh;}                    /** Height of region of interest.   */
  int RoiMaxDim () const {return __max(rw, rh);}    /** Biggest dimension of region. */
  int RoiMinDim () const {return __min(rw, rh);}    /** Smallest dimension of region. */

  int RoiMidX () const {return(rx + (rw / 2));}     /** Mid integer X coord of region. */
  int RoiMidY () const {return(ry + (rh / 2));}     /** Mid integer Y coord of region. */
  int RoiX2 ()   const {return(rx + rw);}           /** Limiting max X (min + wid).    */
  int RoiY2 ()   const {return(ry + rh);}           /** Limiting max Y (min + ht).     */
  int RoiLimX () const {return(rx + rw - 1);}       /** Highest valid X in region.     */
  int RoiLimY () const {return(ry + rh - 1);}       /** Highest valid Y in region.     */

  double RoiAvgX () const {return(rx + 0.5 * (rw - 1));}     /** Mid X float coord of region. */
  double RoiAvgY () const {return(ry + 0.5 * (rh - 1));}     /** Mid Y float coord of region. */
  void RoiLims (int& x0, int& y0, int& x1, int& y1) const    /** Range of valid coordinates.  */
    {x0 = rx; y0 = ry; x1 = rx + rw - 1; y1 = ry + rh - 1;}  
  void RoiClamp (int& x, int& y, int x0, int y0) const       /** Constrain point inside ROI.  */ 
    {x = __min(__max(x, rx), rx + rw - 1); y = __min(__max(y, ry), ry + rh - 1);}

  double FracX () const {return(rx / (double) w);}  /** Lowest X as frac of image width.       */
  double FracY () const {return(ry / (double) h);}  /** Lowest Y as frac of image height.      */
  double FracW () const {return(rw / (double) w);}  /** Region width as frac of image width.   */
  double FracH () const {return(rh / (double) h);}  /** Region height as frac of image height. */

  double FracMidX () const {return((rx + (rw / 2)) / (double) w);}  /** X middle of region as frac of wid. */
  double FracMidY () const {return((ry + (rh / 2)) / (double) h);}  /** Y middle of region as frac of ht.  */
  double FracX2 ()   const {return((rx + rw) / (double) w);}        /** Limiting max X as frac of wid.     */
  double FracY2 ()   const {return((ry + rh) / (double) h);}        /** Limiting max Y as frac of ht.      */
  double FracLimX () const {return((rx + rw - 1) / (double) w);}    /** Highest valid X as frac of wid.    */
  double FracLimY () const {return((ry + rh - 1) / (double) h);}    /** Highest valid X as frac of ht.     */

  int LocalX (double f) const {return(rx + ROUND(f * rw));}  /** Full image X of fractional interior coord. */
  int LocalY (double f) const {return(ry + ROUND(f * rh));}  /** Full image Y of fractional interior coord. */
  int LocalW (double f) const {return ROUND(f * rw);}        /** Pixel count for fraction of region width.  */
  int LocalH (double f) const {return ROUND(f * rh);}        /** Pixel count for fraction of region height. */

  int RoiArea (double f =1.0) const {return ROUND(f * area);}   /** Total area covered (or some fraction). */
  double RoiAspect () const {return(rh / (double) rw);}         /** Ratio of region's height to its width. */

  // positioning
  void MoveRoi (int dx, int dy);
  void IncludeRoi (int x, int y);
  void CenterRoi (int cx, int cy, int wid =-1, int ht =-1);
  void CenterRoi (double cx, double cy, double wid, double ht =0.0);
  void CFracRoi (double cx, double cy, double wid, double ht);
  void ShiftRoi (const jhcRoi& src, double alpha =0.5, int force =0);
  void ZoomRoi (const jhcRoi& src, double cx, double cy, double sc);
  void InvertRoi (int w, int h);
  void MirrorRoi (int w);

  // sizing
  void GrowRoi (int dw2, int dh2);
  void ResizeRoi (int wdes, int hdes);
  void ResizeRoi (double fw, double fh =0.0);
  void ScaleRoi (double fx, double fy =0.0);
  void ScaleRoi (const jhcRoi& ref, double f);
  void ScaleRoi (int wmax, int hmax =0);
  void ShapeRoi (const jhcRoi& src, double beta =0.1, int force =0);

  // full alteration
  void MergeRoi (const jhcRoi& src);
  void AbsorbRoi (int x0, int x1, int y0, int y1);
  void AbsorbRoi (const jhcRoi& src);
  void Union (const jhcRoi *src);
  void StretchRoi (int px, int py);
  void MorphRoi (const jhcRoi& src, double alpha =0.5, double beta =-1.0, int force =0);

  // predicates and utilities
  double CenterDist (int x, int y) const;
  int RoiContains (int x, int y) const;
  int RoiContains (const jhcRoi& tst) const;
  int RoiOverlap (const jhcRoi& src) const;
  double RoiLapBig (const jhcRoi& tst) const;
  double RoiLapSmall (const jhcRoi& tst) const;
  void RoiTrim (int *nx, int *ny, int *nw, int *nh) const;
  int *RoiPts (int lims[]) const;
  char *RoiText (char *ans, int clip, int ssz) const;
  int FullRoi () const;
  int NullRoi () const {return((area == 0) ? 1 : 0);}

  // utilities - convenience
  template <size_t ssz>
    char *RoiText (char (&ans)[ssz], int clip =0) const
      {return RoiText(ans, clip, ssz);}


// PROTECTED MEMBER FUNCTIONS
protected:
  int change_amt (double src, double targ, double frac, int force) const;
  void fix_roi ();


// PRIVATE MEMBER FUNCTIONS
private:
  void init_roi ();


};


#endif
