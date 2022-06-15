// jhcBlob.cpp : manipulate blob centroids, eccentricities, etc.
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

#include <math.h>
#include "Interface/jhcMessage.h"

#include "Data/jhcBlob.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Destructor cleans up allocation (base destructor called automatically).

jhcBlob::~jhcBlob ()
{
  DeallocBlob();
}


//= Default constructor (default constructor for base called automatically).
// NOTE: assumes size = 0 causing FindParams to barf

jhcBlob::jhcBlob ()
{
  InitBlob();
}


//= More explicit handling of model jhcBBlob (really should not be needed).

jhcBlob::jhcBlob (const jhcBlob& ref) : jhcBBox((jhcBBox &) ref)
{
  InitBlob();
  SetSize(ref);
} 


//= Make list with same size as a jhcBBox list (or a jhcBlob list).

jhcBlob::jhcBlob (const jhcBBox& ref) : jhcBBox(ref)
{
  InitBlob();
  SetSize(ref);
} 


//= Make list of a particular size.

jhcBlob::jhcBlob (int ni) : jhcBBox(ni)
{
  InitBlob();
  SetSize(ni);
}


//= Make arrays same size as those in some other object.

void jhcBlob::SetSize (const jhcBBox& ref)
{
  SetSize(ref.Size());
}


//= Allocate arrays of requested size.

void jhcBlob::SetSize (int ni)
{
  // sanity check
#ifdef _DEBUG
  if ((ni <= 0) || (ni > 100000))
    Pause("jhcBlob::SetSize - Trying to allocate %ld blobs!", ni);
#endif

  // check if current arrays can be reused
  if ((ni != total) || (xsum == NULL))
  {
    if (total > 0)
      DeallocBlob();
 
    // allocate temporary accumulators
    xsum  = new int [ni];    
    ysum  = new int [ni];    
    x2sum = new long long [ni];    
    y2sum = new long long [ni];    
    xysum = new long long [ni];    

    // do core allocation  
    XAvg   = new double [ni];
    YAvg   = new double [ni];
    Width  = new double [ni];
    Aspect = new double [ni];
    Angle  = new double [ni];
    Val    = new double [ni];
    Label  = new UC8 [ni];

    // check that it all succeeded
    if (ni > 0)
      if ((xsum == NULL) || (ysum == NULL) || (x2sum == NULL) || (y2sum == NULL) || 
          (xysum == NULL) || (XAvg == NULL) || (YAvg == NULL) || (Width == NULL) ||
          (Aspect == NULL) || (Angle == NULL) || (Val == NULL) || (Label == NULL))
        Fatal("jhcBlob::SetSize - Array allocation failed!");   
  }

  // sets "total" and "valid" to new vals
  jhcBBox::SetSize(ni);  
}


//= Get rid of allocated arrays.

void jhcBlob::DeallocBlob ()
{
  // get rid of core arrays
  delete [] Label;
  delete [] Val;
  delete [] Angle;
  delete [] Aspect;
  delete [] Width;
  delete [] YAvg;
  delete [] XAvg;

  // get rid of temporary accumulators
  delete [] xysum;
  delete [] y2sum;
  delete [] x2sum;
  delete [] ysum;
  delete [] xsum;

  // clear pointers
  InitBlob();
}


//= Set values to defaults.

void jhcBlob::InitBlob ()
{
  // clear accumulators
  xsum  = NULL;
  ysum  = NULL;
  x2sum = NULL;
  y2sum = NULL;
  xysum = NULL;

  // clear core values
  XAvg   = NULL;
  YAvg   = NULL;
  Width  = NULL;
  Aspect = NULL;
  Angle  = NULL;
  Val    = NULL;
  Label  = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                           Read Only Access                            //
///////////////////////////////////////////////////////////////////////////

//= Get total number of pixels (not bounding box area).

int jhcBlob::BlobArea (int index) const
{
  if ((index < 0) || (index >= total))
    return -1;
  return pixels[index];
}


//= Get marked value associated with blob from functions like MaxEach.

double jhcBlob::BlobValue (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return -1.0;
  return(sc * Val[index]);
}


//= Get aspect ratio of blob based on moments (not bounding box).

double jhcBlob::BlobAspect (int index) const
{
  if ((index < 0) || (index >= total))
    return -1.0;
  return Aspect[index];
}


//= Get orientation of major axis (in degrees).
// fix = 0 is for legacy code, fix = 1 for conventional XY plane

double jhcBlob::BlobAngle (int index, int fix) const
{
  if ((index < 0) || (index >= total))
    return -1.0;
  if (fix > 0)
    return(180.0 - Angle[index]);
  return Angle[index];
}


//= Get computed orientation of major axis if elongation is high enough.
// returns 90.0 degrees (vertical) for nearly round things

double jhcBlob::BlobAngleEcc (int index, double eth, int fix) const
{
  double a0 = 90.0;                  // default is vertical

  // check for valid blob
  if ((index < 0) || (index >= total))
    return -1.0;

  // test elongation to see if computed angle should be used
  if ((eth <= 0.0) || (Aspect[index] >= eth))
    a0 = Angle[index];

  // possibly change angle interpretation
  if (fix > 0)
    return(180.0 - a0);
  return a0;
}


//= Get width of equivalent ellipse (semi-minor axis diameter).

double jhcBlob::BlobWidth (int index) const
{
  if ((index < 0) || (index >= total))
    return -1.0;
  return Width[index];
}


//= Get length of equivalent ellipse (semi-major axis diameter).

double jhcBlob::BlobLength (int index) const
{
  if ((index < 0) || (index >= total))
    return -1.0;
  return(Width[index] * Aspect[index]);
}


//= Return centroid of blob based on moments (not bounding box).
// if index out of bounds, returns 0 and does not change x and y

int jhcBlob::BlobCentroid (double *x, double *y, int index) const
{
  if ((index < 0) || (index >= total))
    return 0;
  *x = XAvg[index];
  *y = YAvg[index];
  return 1;
}


//= Determine endpoints of major axis in image coordinates (y=0 at bottom).
// line is equal to ellipse length and passes through centroid
// if index out of bounds, returns 0 and does not change x and y

int jhcBlob::BlobMajor (double& x0, double& y0, double& x1, double& y1, int index) const
{
  double seg, rad, cs, ss;

  if ((index < 0) || (index >= total))
    return 0;
  seg = 0.5 * Width[index] * Aspect[index];
  rad = D2R * (180.0 - Angle[index]);
  cs  = seg * cos(rad);
  ss  = seg * sin(rad);
  x0 = XAvg[index] + cs;
  y0 = YAvg[index] + ss;
  x1 = XAvg[index] - cs;
  y1 = YAvg[index] - ss;
  return 1;
}


//= Determine endpoints of major axis in image coordinates (y=0 at bottom).
// line is equal to ellipse width and passes through centroid
// if index out of bounds, returns 0 and does not change x and y

int jhcBlob::BlobMinor (double& x0, double& y0, double& x1, double& y1, int index) const
{
  double seg, rad, cs, ss;

  if ((index < 0) || (index >= total))
    return 0;
  seg = 0.5 * Width[index];
  rad = D2R * (270.0 - Angle[index]);
  cs  = seg * cos(rad);
  ss  = seg * sin(rad);
  x0 = XAvg[index] + cs;
  y0 = YAvg[index] + ss;
  x1 = XAvg[index] - cs;
  y1 = YAvg[index] - ss;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Feature Extraction                           //
///////////////////////////////////////////////////////////////////////////

//= Fills blob list with parameters based on segmented image.
// forces highly eccentric blobs to be 1 pixel wide
// angles munged for good display (CCW from 0 at 3 o'clock)
// ignores blobs labelled as zero (presumably the background)
// sets "valid" to reflect range of entries filled
// can optionally append newly found blobs to end of current list
// returns the actual number of blobs analyzed (incl. null blob 0)
// NOTE: if nothing seems to be found make sure SetSize was called

int jhcBlob::FindParams (const jhcImg& src, int append, int val0)
{
  if (!src.Valid(2))
    return Fatal("Bad image to jhcBlob::FindParams");

  // general ROI case (append)
  int i, j = 0, start = valid, last = -1;
  int x, y, w = src.XDim(), h = src.YDim(), ssk2 = src.RoiSkip() >> 1;
  int rx = src.RoiX(), ry = src.RoiY(), rx2 = src.RoiLimX(), ry2 = src.RoiLimY();
  double a, xc, yc, mxx, myy, mxy, ang, rt, den, ecc, rad;
  US16 *s = (US16 *)(src.PxlSrc() + src.RoiOff());

  // zero all parameters at start 
  if (append <= 0)
  {
    start = 0;
    ResetBlobs(val0);
    ResetLims(w, h);
  }
  
  // accumulate moments for each blob 
  for (y = ry; y <= ry2; y++, s += ssk2)
    for (x = rx; x <= rx2; x++, s++)
    {
      i = (int) *s;
      if ((i > 0) && (i < total))
      {
        // update bounding box (1.35x faster than StretchRoi version)
        xlo[i] = __min(x, xlo[i]);
        xhi[i] = __max(x, xhi[i]);
        ylo[i] = __min(y, ylo[i]);
        yhi[i] = __max(y, yhi[i]);

        // update moment accumulators
        pixels[i] += 1;
        xsum[i]   += x;
        ysum[i]   += y;
        x2sum[i]  += x * x;
        y2sum[i]  += y * y;
        xysum[i]  += x * y;

        // adjust list length
        status[i] = 1;
        last = __max(i, last);
      }
    }
  valid = last + 1;

  // copy limits back to ROIs
  for (i = start; i < valid; i++)
    if (status[i] > 0)
      GetRoi(i)->SetRoi(xlo[i], ylo[i], xhi[i] - xlo[i] + 1, yhi[i] - ylo[i] + 1);

  // convert moments into useful parameters 
  for (i = start; i < valid; i++)
    if ((a = (double) pixels[i]) > 0.0)
    {
      // get centroid and central moments
      xc  = xsum[i] / a;
      yc  = ysum[i] / a;
      mxx = x2sum[i] - (a * xc * xc);
      myy = y2sum[i] - (a * yc * yc);
      mxy = xysum[i] - (a * xc * yc);

      // find orientation axis
      if ((mxy == 0) && (mxx == mxy))
        ang = 0.0;
      else
        ang = (0.5 * R2D) * atan2(-2.0 * mxy, mxx - myy);
      if (ang < 0.0) 
        ang += 180.0; 

      // determine eccentricity (major / minor axis lengths)
      rt = (double) sqrt(4.0 * mxy * mxy + (mxx - myy) * (mxx - myy));
      den = mxx + myy - rt;
      if (den == 0.0)
        ecc = (4.0 / PI) * a;
      else
        ecc = (double) sqrt((mxx + myy + rt) / den);
      rad = (double) sqrt(a / (PI * ecc));            // minor axis length

      // save into arrays
      XAvg[i]   = xc;
      YAvg[i]   = yc;
      Angle[i]  = ang;
      Width[i]  = 2.0 * rad;
      Aspect[i] = ecc;
      j++;
    }
  return(j);
}


//= Computes average value of some image over each component.
// Also fills in area of each component (but not moments).
// Sets "valid" to reflect range of entries filled.

int jhcBlob::AvgEach (const jhcImg& src, const jhcImg& data, int clr) 
{
  if (!src.Valid(2) || !src.SameSize(data, 1))
    return Fatal("Bad image to jhcBlob::AvgEach");

  // general ROI case (uses component ROI but ignores data ROI)
  int i, last = -1;
  int x, y, rw = src.RoiW(), rh = src.RoiH();
  int ssk2 = src.RoiSkip() >> 1, dsk = data.RoiSkip(src);
  const US16 *s = (const US16 *) src.RoiSrc();
  const UC8 *d = data.RoiSrc(src);

  // zero value and count at start 
  for (i = 0; i < total; i++)
  {
    if (clr > 0)
      status[i] = 0;
    pixels[i] = 0;
    Val[i]    = 0.0;
  }
  valid = 0;

  // accumulate areas and value sums for each component
  for (y = rh; y > 0; y--, d += dsk, s += ssk2)
    for (x = rw; x > 0; x--, d++, s++)
    {
      i = (int) *s;
      if ((i > 0) && (i < total))
      {
        if (clr > 0)
          status[i] = 1;
        pixels[i] += 1;
        Val[i]    += *d;
        if (i > last)
          last = i;
      }
    }
  valid = last + 1;

  // convert sums into averages
  for (i = 1; i < valid; i++)
    if (status[i] > 0)
      Val[i] /= (double) pixels[i];
  return 1;
}


//= Computes minimum value of some image over each component.
// Can optionally ignore zeroes (set nz > 0) in data image.
// Also fills in area of each component (but not moments).
// Sets "valid" to reflect range of entries filled.

int jhcBlob::MinEach (const jhcImg& src, const jhcImg& data, int nz, int clr) 
{
  if (!src.Valid(2) || !src.SameSize(data, 1))
    return Fatal("Bad image to jhcBlob::MinEach");

  // general ROI case (uses component ROI but ignores data ROI)
  int i, last = -1;
  int x, y, rw = src.RoiW(), rh = src.RoiH();
  int ssk2 = src.RoiSkip() >> 1, dsk = data.RoiSkip(src);
  const US16 *s = (const US16 *) src.RoiSrc();
  const UC8 *d = data.RoiSrc(src); 

  // zero value and count at start 
  for (i = 0; i < total; i++)
  {
    if (clr > 0)
      status[i] = 0;
    pixels[i] = 0;
    Val[i]    = 0.0;
  }
  valid = 0;

  // accumulate areas and value sums for each component
  for (y = rh; y > 0; y--, d += dsk, s += ssk2)
    for (x = rw; x > 0; x--, d++, s++)
    {
      i = (int) *s;
      if ((i > 0) && (i < total))
      {
        if ((*d > 0) || (nz <= 0))
          if ((pixels[i] == 0) || (*d < Val[i]))
            Val[i] = *d;
        if (clr > 0)
          status[i] = 1;
        pixels[i] += 1;
        if (i > last)
          last = i;
      }
    }
  valid = last + 1;
  return 1;
}


//= Computes maximum value of some image over each component.
// Also fills in area of each component (but not moments).
// Sets "valid" to reflect range of entries filled.

int jhcBlob::MaxEach (const jhcImg& src, const jhcImg& data, int clr)
{
  if (!src.Valid(2) || !src.SameSize(data, 1))
    return Fatal("Bad image to jhcBlob::MaxEach");

  // general ROI case (uses component ROI but ignores data ROI)
  int i, last = -1;
  int x, y, rw = src.RoiW(), rh = src.RoiH();
  int ssk2 = src.RoiSkip() >> 1, dsk = data.RoiSkip(src);
  const US16 *s = (const US16 *) src.RoiSrc();
  const UC8 *d = data.RoiSrc(src);

  // zero value and count at start 
  for (i = 0; i < total; i++)
  {
    if (clr > 0)
      status[i] = 0;
    pixels[i] = 0;
    Val[i]    = 0.0;
  }
  valid = 0;

  // accumulate areas and value sums for each component
  for (y = rh; y > 0; y--, d += dsk, s += ssk2)
    for (x = rw; x > 0; x--, d++, s++)
    {
      i = (int) *s;
      if ((i > 0) && (i < total))
      {
        if ((pixels[i] == 0) || (*d > Val[i]))
          Val[i] = *d;
        if (clr > 0)
          status[i] = 1;
        pixels[i] += 1;
        if (i > last)
          last = i;
      }
    }
  valid = last + 1;
  return 1;
}


//= Invalidates all blobs and zeroes their entries.
// Touches all elements, not just in the "valid" range.

void jhcBlob::ResetBlobs (int val0)
{
  int i;

  ResetAll(val0);
  for (i = 0; i < total; i++)
  {
    // clear jhcBBox properties
    ClearItem(i);

    // clear temporary accumulator arrays
    pixels[i] = 0;
    xsum[i]   = 0;
    ysum[i]   = 0;
    x2sum[i]  = 0;
    y2sum[i]  = 0;
    xysum[i]  = 0;

    // clear digested values and status flags
    XAvg[i]   = 0.0;
    YAvg[i]   = 0.0;
    Aspect[i] = 0.0;
    Angle[i]  = 0.0;
    Width[i]  = 0.0;
    Val[i]    = 0.0;
    Label[i]  = 0;
  }
}


//= Initialize bounding box values.

void jhcBlob::ResetLims (int w, int h)
{
  int i, xlim = w - 1, ylim = h - 1;

  for (i = 0; i < total; i++)
  {
    xlo[i] = xlim;
    xhi[i] = 0;
    ylo[i] = ylim;
    yhi[i] = 0;
  }
}


//= Copy all assigned fields from one structure to another.
// Note: explict cast of argument to jhcBBox::CopyAll does not work

int jhcBlob::CopyAll (const jhcBlob& src)
{
  int i;

  if (total < src.valid)
    return -1;

  jhcBBox::CopyAll(src);
  for (i = 0; i < valid; i++)
  {
    pixels[i] = src.pixels[i];
    XAvg[i]   = src.XAvg[i];
    YAvg[i]   = src.YAvg[i];
    Aspect[i] = src.Aspect[i];
    Angle[i]  = src.Angle[i];
    Width[i]  = src.Width[i];
    Label[i]  = src.Label[i];
  }

  return valid;
}


///////////////////////////////////////////////////////////////////////////
//                            Blob Selection                             //
///////////////////////////////////////////////////////////////////////////

//= Find area of largest valid blob, ignores entries with status below "sth".

int jhcBlob::MaxArea (int sth) const
{
  int i, best = 0;

  for (i = 1; i < valid; i++)
    if ((status[i] >= sth) && (pixels[i] > best))
      best = pixels[i];
  return best;
}


//= Find index of largest valid blob, ignores entries with status below "sth".

int jhcBlob::KingBlob (int sth) const
{
  int i, best = 0, win = -1;

  for (i = 1; i < valid; i++)
    if ((status[i] >= sth) && (pixels[i] > best))
    {
      best = pixels[i];
      win = i;
    }
  return win;
}


//= Find index of highest valid blob, ignores entries with status below "sth".

int jhcBlob::Highest (int sth) const
{
  int i, win = -1;
  double best = 0.0;

  for (i = 1; i < valid; i++)
    if ((status[i] >= sth) && (YAvg[i] > best))
    {
      best = YAvg[i];
      win = i;
    }
  return win;
}


//= Find index of blob with centroid closest to point, ignores entries below "sth".

int jhcBlob::Nearest (double x, double y, int sth) const
{
  int i, win = -1;
  double dx, dy, dist, best = 0.0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
    {
      dx = XAvg[i] - x;
      dy = YAvg[i] - y;
      dist = dx * dx + dy * dy;
      if ((win < 0) || (dist < best))
      {
        best = dist;
        win = i;
      }
    }
  return win;
}


//= Find index of blob with centroid lowest in scene.

int jhcBlob::MinY (int sth) const
{
  int i, win = -1;
  double best;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
      if ((win < 0) || (YAvg[i] < best))
      {
        best = YAvg[i];
        win = i;
      }
  return win;
}


//= Find index of blob with centroid highest in scene.

int jhcBlob::MaxY (int sth) const
{
  int i, win = -1;
  double best;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
      if ((win < 0) || (YAvg[i] > best))
      {
        best = YAvg[i];
        win = i;
      }
  return win;
}


///////////////////////////////////////////////////////////////////////////
//                          Blob Elimination                             //
///////////////////////////////////////////////////////////////////////////

//= Mark as invalid any blobs below aspect ratio specified.
// if limit is negative, invalidate blobs above limit 

void jhcBlob::AspectThresh (double ath, int sth, int good, int bad)
{
  int i, over = good, under = bad;
  double val = ath;

  if (val < 0)
  {
    val = -ath;
    over = bad;
    under = good;
  }
  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (Aspect[i] >= val)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Mark as invalid any blobs below width specified.
// if limit is negative, invalidate blobs above limit 

void jhcBlob::WidthThresh (double ath, int sth, int good, int bad)
{
  int i, over = good, under = bad;
  double val = ath;

  if (val < 0)
  {
    val = -ath;
    over = bad;
    under = good;
  }
  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (Width[i] >= val)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Mark as invalid any blobs below length specified.
// if limit is negative, invalidate blobs above limit 

void jhcBlob::LengthThresh (double ath, int sth, int good, int bad)
{
  int i, over = good, under = bad;
  double val = ath;

  if (val < 0)
  {
    val = -ath;
    over = bad;
    under = good;
  }
  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if ((Aspect[i] * Width[i]) >= val)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Mark as invalid any blobs with "Value" field below threshold specified.
// if limit is negative, invalidate blobs above limit 
// useful in conjunction with functions such as MaxEach

void jhcBlob::ValueThresh (double th, int sth, int good, int bad)
{
  int i, over = good, under = bad;
  double lim = th;

  if (lim < 0)
  {
    lim = -th;
    over = bad;
    under = good;
  }
  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (Val[i] >= lim)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Mark as invalid any blobs with centroid x below threshold specified.
// if limit is negative, invalidate blobs above limit 

void jhcBlob::HorizThresh (double x, int sth, int good, int bad)
{
  int i, over = good, under = bad;
  double lim = x;

  if (lim < 0)
  {
    lim = -x;
    over = bad;
    under = good;
  }
  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (XAvg[i] >= lim)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Mark as invalid any blobs with centroid y below threshold specified.
// if limit is negative, invalidate blobs above limit 

void jhcBlob::VertThresh (double y, int sth, int good, int bad)
{
  int i, over = good, under = bad;
  double lim = y;

  if (lim < 0)
  {
    lim = -y;
    over = bad;
    under = good;
  }
  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (YAvg[i] >= lim)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Mark as invalid any blob with an orientation below limit specified.
// only considers blobs with significant eintation (over emin)
// a negative threshold means invalidate any blob with higher angle

void jhcBlob::AngleThresh (double ath, double emin, int sth, int good, int bad)
{
  int i, over = good, under = bad;
  double val = ath;

  if (val < 0)
  {
    val = -ath;
    over = bad;
    under = good;
  }
  for (i = 1; i < valid; i++)
    if ((status[i] > sth) && (Aspect[i] >= emin))
    {
      if (Angle[i] >= val)
        status[i] = over;
      else
        status[i] = under;
    } 
}


//= Mark as invalid any blob with an orientation outside limits specified.
// only considers blobs with significant eintation (over emin)
// if ahi less than alo, keeps blobs outside this range

void jhcBlob::AngleKeep (double alo, double ahi, double emin, int sth, int good, int bad)
{
  int i, in = good, out = bad;
  double bot = alo, top = ahi;

  if (ahi < alo)
  {
    bot = ahi;
    top = alo;
    in = bad;
    out = good;
  }
  for (i = 1; i < valid; i++)
    if ((status[i] > sth) && (Aspect[i] >= emin))
    {
      if ((Angle[i] >= bot) && (Angle[i] <= top))
        status[i] = in;
      else
        status[i] = out;
    } 
}


//= Returns number of blobs not ruled out by filters.

int jhcBlob::CountValid (int sth) const
{
  int i, cnt = 0;

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
      cnt++;
  return cnt;
}


//= Returns number of blobs with exactly the given status value.

int jhcBlob::CountStatus (int sth) const
{
  int i, cnt = 0;

  for (i = 1; i < valid; i++)
    if (status[i] == sth)
      cnt++;
  return cnt;
}


//= Find the index for the nth valid object.

int jhcBlob::Nth (int n) const
{
  int i, cnt = 0;

  for (i = 1; i < valid; i++)
    if (status[i] > 0)
    { 
      if (cnt == n)
        return i;
      cnt++;
    }
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                            Region Tagging                             //
///////////////////////////////////////////////////////////////////////////

//= Turn on one pixel at centroid of each blob (rest become zero).
// often preliminary step to Vornoi or Nearest claiming strategy

int jhcBlob::SeedCenters (jhcImg& dest) const
{
  int i, f = dest.Fields();

  if (f > 2)
    return Fatal("Bad image to jhcBlob::SeedCenters");
  dest.FillArr(0);
  for (i = 1; i < valid; i++)
    if (status[i] > 0)
    {
      if (f == 1)
        dest.ASet(ROUND(XAvg[i]), ROUND(YAvg[i]), 0, 
                  (UC8)(i & 0xFF));
      else
        dest.ASet(ROUND(XAvg[i]), ROUND(YAvg[i]), 0, 
                  (US16)(i & 0xFFFF));
    }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Visualization                              //
///////////////////////////////////////////////////////////////////////////

//= Fill array with 8 bit version of some parameter mapped onto each blob.
// Values are normalized so that "lim" corresponds to 255 (not needed for some).
// Parameter choices are:
// <pre>
//   0 = Area (common)     8 = XMin  
//   1 = Width (common)    9 = XMax
//   2 = Aspect (common)  10 = YMin
//   3 = Angle (common)   11 = YMax
//   4 = Mark             12 = Val (common)
//   5 = Label            13 = Length (common)
//   6 = XAvg             14 = BBox width
//   7 = YAvg             15 = BBox height
//                        16 = BBox max     
// </pre>

int jhcBlob::MapParam (jhcImg& dest, const jhcImg& src, int p, double lim) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad images to jhcBlob::MapParam");
  dest.CopyRoi(src);

  // general ROI case
  int i;
  int x, y, col, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk2 = src.RoiSkip() >> 1;
  double val, sc = 1.0, top = lim;
  const US16 *s = (const US16 *)(src.PxlSrc() + src.RoiOff());
  UC8 *d = dest.PxlDest() + dest.RoiOff();

  // force scaling or use maximum value of parameter
  if (top <= 0.0)
    for (i = 1; i < valid; i++)
      if (status[i] > 0)
      {
        val = GetParam(i, p);
        if (val > top)
          top = val;
      }
  if (top > 0.0)
    sc = 255.0 / top;

  // generate normalized labels
  for (i = 1; i < valid; i++)
    if (status[i] > 0)
    {
      col = ROUND(sc * GetParam(i, p));
      Label[i] = BOUND(col);
    }
    else
      Label[i] = 0;

  // remap pixels in ROI
  for (y = rh; y > 0; y--, d += dsk, s += ssk2)
    for (x = rw; x > 0; x--, d++, s++)
    {
      i = *s;
      if (i < valid)
        *d = Label[i];
      else
        *d = 0;
    }
  return 1;
}


//= Extract the value of a parameter using special request number.
// Does not check bounds

double jhcBlob::GetParam (int i, int p) const
{
  switch (p)
  {
    case 0:
      return((double) pixels[i]);
    case 1:
      return Width[i];
    case 2:
      return Aspect[i];
    case 3:
      return Angle[i];
    case 4:
      return((double) status[i]);
    case 5:
      return((double) Label[i]);
    case 6:
      return XAvg[i];
    case 7:
      return YAvg[i];
    case 8:
      return((double) items[i].RoiX());
    case 9:
      return((double) items[i].RoiLimX());
    case 10:
      return((double) items[i].RoiY());
    case 11:
      return((double) items[i].RoiLimY());
    case 12:
      return Val[i];
    case 13:
      return(Aspect[i] * Width[i]);
    case 14:
      return((double) items[i].RoiW());
    case 15:
      return((double) items[i].RoiH());
    case 16:
      return((double) __max(items[i].RoiW(), items[i].RoiH()));
    default:
      return -1.0;
  }
}


///////////////////////////////////////////////////////////////////////////
//                           Axis Box Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Finds lateral limits of object with respect to ellipse axis through centroid.
// needs 16 bit connected components image and original component number 
// generates max left and right displacements for each point along axis
// bins are integral pixels along axis, and values are integral displacements
// centroid is at mid-point in each array (may be asymmetric up and down)
// difference: (rt - lf) = width, average: 0.5 * (rt + lf) = axial bend
// if index out of bounds, then returns 0 and leaves arrays unchanged

int jhcBlob::Profiles (jhcArr& lf, jhcArr& rt, const jhcImg& src, int i, double eth) const
{
  // check for reasonable arguments
  if (!src.Valid(2) || (lf.Size() != rt.Size()))
    return Fatal("Bad inputs to jhcBlob::Profiles");
  if ((i <= 0) || (i >= valid))
    return 0;

  // clear profile arrays
  lf.Fill(0);
  rt.Fill(0);

  // local variables
  int x, y, n, lat, sz = lf.Size();
  int x0 = xlo[i], x1 = xhi[i], y0 = ylo[i], y1 = yhi[i], sk = src.RoiSkip(x1 - x0 + 1) >> 1;
  double h, w, mid = 0.5 * (sz - 1), r = -D2R * BlobAngleEcc(i, eth, 1), c = cos(r), s = sin(r);
  double dx = x0 - XAvg[i], dy = y0 - YAvg[i], h0 = dx * c - dy * s, w0 = dx * s + dy * c; 
  const US16 *p = (const US16 *) src.RoiSrc(x0, y0);

  // scan source pixels inside axis-parallel bounding box
  // measure distances from centroid relative to ellipse axes 
  for (y = y0; y <= y1; y++, p += sk, h0 -= s, w0 += c)
  {
    h = h0;
    w = w0;
    for (x = x0; x <= x1; x++, p++, h += c, w += s)
      if (*p == i)
      {
        // convert offset along axis to array index
        n = ROUND(mid + h);
        if ((n >= 0) && (n < sz))
        {
          // record offset orthogonal to axis if better than previous
          lat = ROUND(w);
          lf.AMin(n, lat);
          rt.AMax(n, lat);
        }
      }
  }
  return 1;
}


//= Finds tilted bounding box aligned with ellipse axis (BlobAngle).
// needs access to original 16 bit connected component image to check pixels
// if eccentricity less then given threshold then returns axis parallel box
// if index out of bounds, returns 0 and does not change x, y, len, and wid

int jhcBlob::ABox (double& xm, double& ym, double& len, double& wid, const jhcImg& src, int i, double eth) const
{
  if (!src.Valid(2))
    return Fatal("Bad image to jhcBlob::ABox");
  if ((i <= 0) || (i >= valid))
    return 0;

  // local variables
  int x, y, x0 = xlo[i], x1 = xhi[i], y0 = ylo[i], y1 = yhi[i], sk = src.RoiSkip(x1 - x0 + 1) >> 1;
  double r = -D2R * BlobAngleEcc(i, eth, 1), c = cos(r), s = sin(r);
  double dx = x0 - XAvg[i], dy = y0 - YAvg[i], h0 = dx * c - dy * s, w0 = dx * s + dy * c; 
  double h, w, dh, dw, bot = 0.0, top = 0.0, lf = 0.0, rt = 0.0;
  const US16 *p = (const US16 *) src.RoiSrc(x0, y0);

  // scan source pixels inside axis-parallel bounding box
  // measure distances from centroid relative to ellipse axes 
  for (y = y0; y <= y1; y++, p += sk, h0 -= s, w0 += c)
  {
    h = h0;
    w = w0;
    for (x = x0; x <= x1; x++, p++, h += c, w += s)
      if (*p == i)
      {
        // find extrema along major axis
        bot = __min(bot, h);
        top = __max(top, h);

        // find extrema perpendicular to major axis
        lf = __min(lf, w);
        rt = __max(rt, w);
      }
  }

  // convert extreme points to box measurements
  len = top - bot;
  wid = rt - lf;
  dh = 0.5 * (top + bot);
  dw = 0.5 * (rt + lf);
  xm = XAvg[i] + dh * c + dw * s;
  ym = YAvg[i] - dh * s + dw * c;
  return 1;
}


//= Gets oriented bounding box corners based on fundamental jhcBlob::ABox() info about object
// takes an array of four x and y values for output, pass to jhcDraw::DrawCorners to view
// elongation axis runs at blob orientation from avg(xy2, xy3) to avg(xy0, xy1) 
// if eccentricity less then given threshold then returns an axis parallel box
// if index out of bounds, returns 0 and does write any x's or y's 
// NOTE: old version took "ang" directly instead of blob number and eccentricity threshold

int jhcBlob::ABoxCorners (double x[], double y[], double xm, double ym, double len, double wid, int i, double eth) const
{
  double ang = BlobAngleEcc(i, eth, 1), r = D2R * ang, c = cos(r), s = sin(r);
  double len2 = 0.5 * len, wid2 = 0.5 * wid, clen = c * len2, slen = s * len2, cwid = c * wid2, swid = s * wid2;

  if ((i <= 0) || (i >= valid))
    return 0;
  x[0] = xm + clen - swid;
  y[0] = ym + slen + cwid;
  x[1] = xm + clen + swid;
  y[1] = ym + slen - cwid;
  x[2] = xm - clen + swid;
  y[2] = ym - slen - cwid;
  x[3] = xm - clen - swid;
  y[3] = ym - slen + cwid;
  return 1;
}


//= Find the end of the elongation axis along canonical angle, or opposite to it.
// requires ABox corner coords, returns midpoint of selected end of ABox

void jhcBlob::ABoxEnd (double& tx, double& ty, double x[], double y[], int dir) const
{
  if (dir > 0)
  {
    tx = 0.5 * (x[0] + x[1]);
    ty = 0.5 * (y[0] + y[1]);
  }
  else
  {
    tx = 0.5 * (x[2] + x[3]);
    ty = 0.5 * (y[2] + y[3]);
  }
}


//= Generates a binary mask running from center offset lo to hi along the elongation axis.
// section is sliced along axis which runs from -len/2 to +len/2
// to get an end try lo = len/4 and hi = len/2, or lo = -len/2 and hi = -len/4
// requires ABox corner coords and original connected components image for labels
// useful for getting color histograms of top, middle, and bottom of an object
// NOTE: sets ROI of destination image encompass just new mask (for efficiency)
// returns area of mask for convenience

int jhcBlob::ABoxFrac (jhcImg& dest, const jhcImg& src, double lo, double hi, int i, double eth) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad image to jhcBlob::ABoxFrac");
  dest.FillArr(0);
  if ((i <= 0) || (i >= valid))
    return 0;

  // local variables
  int x, y, x0 = xlo[i], x1 = xhi[i], y0 = ylo[i], y1 = yhi[i];
  int psk2 = src.RoiSkip(x1 - x0 + 1) >> 1, dsk = dest.RoiSkip(x1 - x0 + 1);
  double r = -D2R * BlobAngleEcc(i, eth, 1), c = cos(r), s = sin(r);
  double h, h0 = (x0 - XAvg[i]) * c - (y0 - YAvg[i]) * s;
  int lf = dest.XLim(), rt = 0, bot = dest.YLim(), top = 0, cnt = 0;
  const US16 *p = (const US16 *) src.RoiSrc(x0, y0);
  UC8 *d = dest.RoiDest(x0, y0);

  // scan source pixels inside axis-parallel bounding box
  // measure distances from centroid relative to ellipse axes 
  for (y = y0; y <= y1; y++, d += dsk, p += psk2, h0 -= s)
  {
    h = h0;
    for (x = x0; x <= x1; x++, d++, p++, h += c)
      if ((*p == i) && (h >= lo) && (h <= hi))
      {
        // copy pixel and work on output bounding box
        *d = 255;
        lf  = __min(lf, x);
        rt  = __max(rt, x);
        bot = __min(bot, y);
        top = __max(top, y);
        cnt++;
      }
  }

  // set image bounding box
  dest.SetRoi(lf, bot, rt - lf + 1, top - bot + 1);
  return cnt;
}
