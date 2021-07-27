// jhcRoi.cpp - functions for manipulating rectangular regions
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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "Data/jhcRoi.h"


//////////////////////////////////////////////////////////////////////////////
//                     Basic creation and deletion                          //
//////////////////////////////////////////////////////////////////////////////

//= Basic constructor.

jhcRoi::jhcRoi ()
{
  init_roi();
}


//= Create new ROI based on an old ROI.
// only copies clipping limits, NOT position and size 

jhcRoi::jhcRoi (const jhcRoi& src)
{
  init_roi();
  RoiClip(src);
  MaxRoi();
}


//= Create new ROI with particular dimensions.

jhcRoi::jhcRoi (int dx, int dy, int dw, int dh, int xclip, int yclip)
{
  init_roi();
  rx = dx;
  ry = dy;
  rw = dw;
  rh = dh;
  w = xclip;
  h = yclip;
  fix_roi();
} 


//= Similar to other constructor but takes an array of 4 values.
// assumes ROI is set to maximum dimensions (clips to rx2 and ry2)

jhcRoi::jhcRoi (int specs[])
{
  init_roi();
  rx = specs[0];
  ry = specs[1];
  rw = specs[2];
  rh = specs[3];
  w = rx + rw;
  h = ry + rh;
  fix_roi();
}


//= Default settings.

void jhcRoi::init_roi ()
{
  ClearRoi();
  w = 0;
  h = 0;
}


/////////////////////////////////////////////////////////////////////////////
//                          Full Initialization                            //
/////////////////////////////////////////////////////////////////////////////

//= Make Region Of Interest be the same as in another ROI.
// assumes src is already consistent (properly clipped)

void jhcRoi::CopyRoi (const jhcRoi& src)
{
  if (&src != this)
  {
    rx = src.rx;
    ry = src.ry;
    rw = src.rw;
    rh = src.rh;
    area = src.area;

    w = src.w;
    h = src.h;
  }
}


//= Write default values to basic geometry fields.

void jhcRoi::ClearRoi ()
{
  rx = 0;
  ry = 0;
  rw = 0;
  rh = 0;
  area = 0;
}


//= Make Region of Interest be as big as clipping region.

void jhcRoi::MaxRoi ()
{
  rx = 0;
  ry = 0;
  rw = w;
  rh = h;
  area = w * (int) h;
}


//= Set restriction range for processing.

void jhcRoi::SetRoi (int x, int y, int wid, int ht)
{
  rx = x;
  ry = y;
  rw = wid;
  rh = ht;
  fix_roi();
}


//= Extracts ROI parameters from an array.
// optionally rescales area entries by some factor

void jhcRoi::SetRoi (const int specs[], double f)
{
  rx = ROUND(f * specs[0]);
  ry = ROUND(f * specs[1]);
  rw = ROUND(f * specs[2]);
  rh = ROUND(f * specs[3]);
  fix_roi();
}


//= Like SetRoi, but takes corner and dimensions as fractions of overall sizes.
// assumes clipping range (w h) has already been set

void jhcRoi::SetRoi (double xf, double yf, double wf, double hf)
{
  rx = ROUND(xf * w);
  ry = ROUND(yf * h);
  rw = ROUND(wf * w);
  rh = ROUND(hf * h);
  fix_roi();
}


//= Build an integer ROI which best captures the center and size given.
// can optionally scale size (not position) by some factor 
// if ht <= 0 then copies width
// all coordinates are ABSOLUTE (not fractional)

void jhcRoi::SetCenter (double xc, double yc, double wid, double ht, double f)
{
  double wf = wid * f, hf = ((ht > 0.0) ? ht * f : wf);

  SetRoi(ROUND(xc - 0.5 * wf), ROUND(yc - 0.5 * hf), ROUND(wf), ROUND(hf));
}


//= Sets ROI to match an array of corner coordinates.
// takes xmin, ymin, xmax, ymax (NOT w and h)

void jhcRoi::SetRoiPts (const int lims[])
{
  rx = lims[0];
  ry = lims[1];
  rw = lims[2] - lims[0] + 1;
  rh = lims[3] - lims[1] + 1;
  fix_roi();
}


//= Set some restriction values for processing.
// negative values mean no change to that value

void jhcRoi::DefRoi (int x, int y, int wid, int ht)
{
  // handle defaults
  if (x >= 0)
    rx = x;
  if (y >= 0)
    ry = y;
  if (wid >= 0)
    rw = wid;
  if (ht >= 0)
    rh = ht;
  fix_roi();
}


//= Like DefRoi, but takes corner and dimensions as fractions of overall sizes.
// assumes clipping range (w h) has already been set

void jhcRoi::DefRoi (double xf, double yf, double wf, double hf)
{
  if (xf > 0.0) 
    rx = ROUND(xf * w);
  if (yf > 0.0)
    ry = ROUND(yf * h);
  if (wf > 0.0)
    rw = ROUND(wf * w);
  if (hf > 0.0)
    rh = ROUND(hf * h);
  fix_roi();
}


//= Sets ROI based on discrete values for min and max coords.

void jhcRoi::SetRoiLims (int x0, int y0, int x1, int y1)
{
  rx = x0;
  ry = y0;
  rw = x1 - x0 + 1;
  rh = y1 - y0 + 1;
  fix_roi();
}


//= Read out basic information into 4 variables.

void jhcRoi::RoiSpecs (int *x, int *y, int *wid, int *ht) const 
{
  if (x != NULL)
    *x = rx;
  if (y != NULL)
    *y = ry;
  if (wid != NULL)
    *wid = rw;
  if (ht != NULL)
    *ht = rh;
}


//= Read out basic information into a 4 element array.

int *jhcRoi::RoiSpecs (int specs[]) const 
{
  specs[0] = rx;
  specs[1] = ry;
  specs[2] = rw;
  specs[3] = rh;
  return specs;
}


/////////////////////////////////////////////////////////////////////////////
//                          Clipping Alteration                            //
/////////////////////////////////////////////////////////////////////////////

//= Set bounds for maximum x and y coordinates (minimums always zero).
// negative values mean no change to that value

void jhcRoi::RoiClip (int wid, int ht)
{
  if (wid > 0)
    w = wid;
  if (ht > 0)
    h = ht;
  fix_roi();
}


//= Copying clipping region from another ROI.

void jhcRoi::RoiClip (const jhcRoi& src)
{
  w = src.w;
  h = src.h;
  fix_roi();
}


//////////////////////////////////////////////////////////////////////////////
//                            Position Alteration                           //
//////////////////////////////////////////////////////////////////////////////

//= Keeps the same size but moves a small amount as indicated.

void jhcRoi::MoveRoi (int dx, int dy)
{
  rx += dx;
  ry += dy;
  fix_roi();
}


//= Try to make region include the given point.
// shifts but does not reshape the region

void jhcRoi::IncludeRoi (int x, int y)
{
  if (x < rx)
    rx = x;
  else if (x > (rx + rw - 1))
    rx = x - (rw - 1);
  if (y < ry)
    ry = y;
  else if (y > (ry + rh - 1))
    ry = y - (rh - 1);
  fix_roi();
}


//= Make the region be centered on the given coordinates.
// If either coordinate is less than zero, no change is made
// negative dimensions mean no change to that value
// NOTE: for floating point absolute values use SetCenter

void jhcRoi::CenterRoi (int midx, int midy, int wid, int ht)
{
  if (wid >= 0)
    rw = wid;
  if (ht >= 0)
    rh = ht;
  if (midx >= 0)
    rx = midx - rw / 2;
  if (midy >= 0)
    ry = midy - rh / 2;
  fix_roi();
}


//= Like other CenterRoi but takes FRACTIONAL coordinates wrt image.
// NOTE: for floating point absolute values use SetCenter

void jhcRoi::CenterRoi (double cx, double cy, double wid, double ht)
{
  if (wid >= 0)
    rw = ROUND(wid * w);
  if (ht >= 0)
    rh = ROUND(ht * h);
  if (cx >= 0)
    rx = ROUND((cx - 0.5 * wid) * w);
  if (cy >= 0)
    ry = ROUND((cy - 0.5 * ht) * h);
  fix_roi();
}


//= Carve out a ROI within some reference ROI using fractional coordinates.

void jhcRoi::RoiWithin (double fx, double fy, double fw, double fh, const jhcRoi& ref)
{
  SetRoi(ref.LocalX(fx), ref.LocalY(fy), ref.LocalW(fw), ref.LocalH(fh));
}


//= Like about but x and y are center positions, not corners.

void jhcRoi::CenterWithin (double cfx, double cfy, double fw, double fh, const jhcRoi& ref)
{
  CenterRoi(ref.LocalX(cfx), ref.LocalY(cfy), ref.LocalW(fw), ref.LocalH(fh));
}


//= Shift the bounding box to make its center closer to other center.
// Alpha determines how complete the shift is (0 to 1).
// If force is non-zero, always adjust by at least 1 pixel.

void jhcRoi::ShiftRoi (const jhcRoi& src, double alpha, int force)
{
  double a = __min(alpha, 1.0);

  if (a > 0.0)
  {
    rx += change_amt(rx + 0.5 * rw, src.rx + 0.5 * src.rw, a, force);
    ry += change_amt(ry + 0.5 * rh, src.ry + 0.5 * src.rh, a, force);
    fix_roi();
  }
}


// computes equivalent ROI for image zoom by sc around point (cx, cy)
// assumes new and old images are both the size specified by src

void jhcRoi::ZoomRoi (const jhcRoi& src, double cx, double cy, double sc)
{
  rx = ROUND(sc * (src.rx - cx) + 0.5 * src.w);
  ry = ROUND(sc * (src.ry - cy) + 0.5 * src.h);
  rw = ROUND(sc * src.rw);
  rh = ROUND(sc * src.rh);
  w = src.w;
  h = src.h;
  fix_roi();
}


//= Reposition region for upside down version of some image with size (w h).
// this is a rigid rotation (both x and y change)

void jhcRoi::InvertRoi (int w, int h)
{
  rx = w - rx - rw;
  ry = h - ry - rh; 
  fix_roi();
}


//= Reposition region for left-right flipped version of some image with size (w h).
// this is a reflection only (y unaffected)

void jhcRoi::MirrorRoi (int w)
{
  rx = w - rx - rw;
  fix_roi();
}


//////////////////////////////////////////////////////////////////////////////
//                            Size Alteration                               //
//////////////////////////////////////////////////////////////////////////////

//= Expand both sides away from middle, negative values shrink instead.

void jhcRoi::GrowRoi (int dw2, int dh2)
{
  rx -= dw2;
  ry -= dh2;
  rw += 2 * dw2;
  rh += 2 * dh2;
  fix_roi();
}


//= Grow region by the given number of pixels on each side.

void jhcRoi::PadRoi (int lf, int bot, int rt, int top)
{
  rx -= lf;
  ry -= bot;
  rw += (lf + rt);
  rh += (bot + top);
  fix_roi();
}


//= Make region be given size, still centered at original location.
// if either parameter is less than zero, that dimension is unchanged

void jhcRoi::ResizeRoi (int wdes, int hdes)
{
  if (wdes >= 0)
  {
    rx += (rw - wdes) / 2;
    rw = wdes;
  }
  if (hdes >= 0)
  {
    ry += (rh - hdes) / 2;
    rh = hdes;
  }
  fix_roi();
}


//= Just change width and height by a factor.

void jhcRoi::ResizeRoi (double fw, double fh)
{
  int pw = rw, ph = rh;

  if (fh <= 0.0)
    fh = fw;
  rw = ROUND(fw * rw);
  rh = ROUND(fh * rh);
  rx += (pw - rw) / 2;
  ry += (ph - rh) / 2;
  fix_roi();
}


//= Multiplies offsets, dimensions, and clipping by a constant.
// If fy is zero, factor fx is used to preserve aspect ratio.
// Useful for adjusting a fixed ROI to a subsampled image.
// Note: Clipping must be changed separately (for image protection).

void jhcRoi::ScaleRoi (double fx, double fy)
{
  double fy2 = fy;

  if (fy == 0.0)
    fy2 = fx;
  rx = ROUND(fx  * rx);
  ry = ROUND(fy2 * ry);
  rw = ROUND(fx  * rw);
  rh = ROUND(fy2 * rh);
  fix_roi();
}


//= Adjust a ROI for a different sized image.

void jhcRoi::ScaleRoi (const jhcRoi& ref, double f)
{
  rx = ROUND(f * ref.rx);
  ry = ROUND(f * ref.ry);
  rw = ROUND(f * ref.rw);
  rh = ROUND(f * ref.rh);
  w  = ROUND(f * ref.w);
  h  = ROUND(f * ref.h);
  fix_roi();
}


//= Like other ScaleRoi but takes new maximum dimensions and compares them.
//   to the current clipping range to compute change
// If hmax is zero, keeps current aspect ratio in new ROI.
// Note: Clipping must be changed separately (for image protection).

void jhcRoi::ScaleRoi (int wmax, int hmax)
{
  if ((wmax == 0) || (w == 0) || (h == 0))
    return;
  ScaleRoi(wmax / (double) w, hmax / (double) h);
}


//= Change width and height make this ROI more like other ROI.
// Beta determines how complete the resizing is (0 to 1).
// If force is non-zero, always adjust by at least 1 pixel.

void jhcRoi::ShapeRoi (const jhcRoi& src, double beta, int force)
{
  double b = __min(beta, 1.0), pw = rw, ph = rh;

  if (b > 0.0)
  {
    rw += change_amt(rw, src.rw, b, force);
    rh += change_amt(rh, src.rh, b, force);
    rx -= ROUND(0.5 * (rw - pw));
    ry -= ROUND(0.5 * (rh - ph));
    fix_roi();
  }
}


//////////////////////////////////////////////////////////////////////////////
//                           Complex Alteration                             //
//////////////////////////////////////////////////////////////////////////////

//= Sets own ROI to be intersection with other ROI.

void jhcRoi::MergeRoi (const jhcRoi& src)
{
  int sx = src.rx, sy = src.ry, sw = src.rw, sh = src.rh;

  // take intersection relative to bounding coordinates
  RoiTrim(&sx, &sy, &sw, &sh);

  // copy to this image
  rx = sx;
  ry = sy;
  rw = sw;
  rh = sh;
  fix_roi();
}


//= Enlarges ROI so it includes the specified area (inclusive of endpoints).

void jhcRoi::AbsorbRoi (int x0, int x1, int y0, int y1)
{
  int rx1 = rx + rw - 1, ry1 = ry + rh - 1;

  // sanity check
  if (NullRoi() > 0)
  {
    SetRoiLims(x0, x1, y0, y1);
    return;
  }

  // extend included range
  rx  = __min(rx, x0);
  ry  = __min(ry, y0);
  rx1 = __max(rx1, x1);
  ry1 = __max(ry1, y1);  

  // recompute dimensions
  rw = rx1 - rx + 1;
  rh = ry1 - ry + 1;
  fix_roi();
}


//= Sets own ROI to include other ROI.

void jhcRoi::AbsorbRoi (const jhcRoi& src)
{
  int rx2 = rx + rw, ry2 = ry + rh;
  int sx2 = src.rx + src.rw, sy2 = src.ry + src.rh;

  // sanity check
  if (NullRoi() > 0)
  {
    CopyRoi(src);
    return;
  }

  // take union relative to bounding coordinates
  rx  = __min(rx, src.rx);
  rx2 = __max(rx2, sx2);
  ry  = __min(ry, src.ry);
  ry2 = __max(ry2, sy2);

  // find new width and height
  rw = rx2 - rx;
  rh = ry2 - ry;
  fix_roi();
}


//= Copies ROI if current area is zero, else expands self to include new ROI.

void jhcRoi::Union (const jhcRoi *src)
{
  if (src == NULL)
    return;
  if (area == 0)
    CopyRoi(*src);
  else
    AbsorbRoi(*src);
}


//= Stretch ROI to include point given.

void jhcRoi::StretchRoi (int px, int py)
{
  int rx2 = rx + rw, ry2 = ry + rh;

  rx  = __min(rx,  px);
  rx2 = __max(rx2, px + 1);
  ry  = __min(ry,  py);
  ry2 = __max(ry2, py + 1);

  rw = rx2 - rx;
  rh = ry2 - ry;
  fix_roi();
}


//= Simultaneously resize and move ROI to be more like other.
// Alpha determines movement, beta determines reshaping.
// If beta is negative, alpha value is used.
// Better behavior near boundaries than Move + Resize.
// If force is non-zero, always adjust by at least 1 pixel.

void jhcRoi::MorphRoi (const jhcRoi& src, double alpha, double beta, int force)
{
  double a = __min(alpha, 1.0);
  double b = ((beta >= 0.0) ? __min(beta, 1.0) : a);
  double px = rx, py = ry, pw = rw, ph = rh;

  // resize box and also compute change to corner position
  if (a > 0.0)
  {
    rw += change_amt(rw, src.rw, b, force);
    rh += change_amt(rh, src.rh, b, force);
    px -= 0.5 * (rw - pw);
    py -= 0.5 * (rh - ph);
  }

  // move corner based on difference in mid x and mid y
  if (b > 0.0)
  {
    px += change_amt(rx + 0.5 * pw, src.rx + 0.5 * src.rw, a, force);
    py += change_amt(ry + 0.5 * ph, src.ry + 0.5 * src.rh, a, force);
  }

  // round corner position and clip coords (if needed)
  if ((a > 0.0) || (b > 0.0))
  {
    rx = ROUND(px);
    ry = ROUND(py);
    fix_roi(); 
  }
}


//= Figure how much to change a quantity, can force amount to be non-zero.

int jhcRoi::change_amt (double src, double targ, double frac, int force) const 
{
  int delta;

  if (targ == src)
    return 0;
  delta = ROUND(frac * (targ - src));
  if ((delta != 0) || (force <= 0))
    return delta;
  if (targ > src)
    return 1;
  return -1;
}


//////////////////////////////////////////////////////////////////////////////
//                      Comparison and Utilities                            //
//////////////////////////////////////////////////////////////////////////////

//= Return distance of point from center of bounding box.

double jhcRoi::CenterDist (int x, int y) const
{
  double dx = x - rx + 0.5 * rw, dy = y - ry + 0.5 * rh;

  return((double) sqrt(dx * dx + dy * dy));
}


//= See if point inside this Roi.

int jhcRoi::RoiContains (int x, int y) const 
{
  if ((x >= rx) && (x < (rx + rw)) && 
      (y >= ry) && (y < (ry + rh)))
    return 1;
  return 0;
}


//= See if some other Roi is completely inside this Roi.

int jhcRoi::RoiContains (const jhcRoi& tst) const 
{
  if ((tst.rx >= rx) && ((tst.rx + tst.rw) <= (rx + rw)) &&  
      (tst.ry >= ry) && ((tst.ry + tst.rh) <= (ry + rh)))
    return 1;
  return 0;
}


//= Find how many pixels are in the overlap between two ROIs.
// disregards additional restrictions due to clipping limits 

int jhcRoi::RoiOverlap (const jhcRoi& src) const 
{
  int sx = src.rx, sy = src.ry, sw = src.rw, sh = src.rh;

  RoiTrim(&sx, &sy, &sw, &sh);
  if ((sw <= 0) || (sh <= 0))
    return 0;
  return(sw * sh);
}


//= Finds overlap fraction with respect to larger area.

double jhcRoi::RoiLapBig (const jhcRoi& tst) const
{
  return(RoiOverlap(tst) / (double) __max(area, tst.area));
}


//= Finds overlap fraction with respect to smaller area.
// returns 0 if either ROI is zero area

double jhcRoi::RoiLapSmall (const jhcRoi& tst) const
{
  int sm = __min(area, tst.area);

  if (sm <= 0)
    return 0.0;
  return(RoiOverlap(tst) / (double) sm);
}


//= Clip some other given rectangle to fit within ROI.

void jhcRoi::RoiTrim (int *nx, int *ny, int *nw, int *nh) const 
{
  int rx2 = rx + rw, ry2 = ry + rh;
  int nx2 = *nx + *nw, ny2 = *ny + *nh;

  *nx = __max(rx, __min(*nx, rx2));
  *ny = __max(ry, __min(*ny, ry2));
  nx2 = __max(rx, __min(nx2, rx2));
  ny2 = __max(ry, __min(ny2, ry2));
  *nw = nx2 - *nx;
  *nh = ny2 - *ny;  
}


//= Load up short array with corner coordinates of region.
// array gets xmin, ymin, xmax, ymax (NOT w and h)

int *jhcRoi::RoiPts (int lims[]) const 
{
  lims[0] = rx;
  lims[1] = ry;
  lims[2] = rx + rw - 1;
  lims[3] = ry + rh - 1;
  return lims;
}


//= Generate standard string describing Roi posiiton and shape.
// useful for debugging

char *jhcRoi::RoiText (char *ans, int clip, int ssz) const 
{
  if (ans == NULL)
    return NULL;
  if (clip > 0)
    sprintf_s(ans, ssz, "@ (%d %d) x (%d %d) [%d %d]", rx, ry, rw, rh, w, h);
  else
    sprintf_s(ans, ssz, "@ (%d %d) x (%d %d)", rx, ry, rw, rh);
  return ans;
}


//= Returns 1 if ROI covers the maximum possible clipping area.

int jhcRoi::FullRoi () const 
{
  if ((rx == 0) && (ry == 0) && (rw == w) && (rh == h))
    return 1;
  return 0;
}


//= Makes sure current ROI coordinates are non-negative.
// and within x and y clipping limits (if any)
// NOTE: old version used to ALWAYS clip to zero (changed 11/13)

void jhcRoi::fix_roi ()
{
  int rx2 = rx + rw, ry2 = ry + rh;

  // adjust horizontally
  if (w > 0)
  {
    if (rx < 0)
      rx = 0;
    else if (rx >= w)
      rx = w - 1;
    if (rx2 < 0)
      rx2 = 0;
    else if (rx2 > w)
      rx2 = w;
  }

  // adjust vertically
  if (h > 0)
  {
    if (ry < 0)
      ry = 0;
    else if (ry > h)
      ry = h - 1;
    if (ry2 < 0)
      ry2 = 0;
    else if (ry2 > h)
      ry2 = h;
  }

  // compute new size
  rw = rx2 - rx;
  if (rw < 0)
    rw = 0;
  rh = ry2 - ry;
  if (rh < 0)
    rh = 0;
  area = rw * rh;
}




