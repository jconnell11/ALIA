// jhcStats.cpp : various staistical properties of image regions
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

#include <math.h>
#include "Interface/jhcMessage.h"

#include "Processing/jhcStats.h"


///////////////////////////////////////////////////////////////////////////
//                             Maxima Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Find maximum value in whole image (at least part in default ROI).

int jhcStats::MaxVal (const jhcImg& src) const
{
  return MaxVal(src, src);
}


//= Examines only a particular area given x, y, w, and h directly.

int jhcStats::MaxVal (const jhcImg& src, int x, int y, int w, int h) const 
{
  jhcRoi patch(src);

  patch.SetRoi(x, y, w, h);
  return MaxVal(src, patch);
}


//= Like other MaxVal but takes position and size in an array: {x, y, w, h}.

int jhcStats::MaxVal (const jhcImg& src, const int *specs) const 
{
  jhcRoi patch(src);

  patch.SetRoi(specs);
  return MaxVal(src, patch);
}


//= Like other MaxVal but extracts position and size of region from a ROI.
// treats all bytes of the image the same (e.g. R, G, B)

int jhcStats::MaxVal (const jhcImg& src, const jhcRoi& patch) const 
{
  if (src.Valid(2))
    return MaxVal_16(src, patch);
  if (!src.Valid())
    return Fatal("Bad image to jhcStats::MaxVal");

  // process specified region
  jhcRoi p2;
  int x, y, rcnt, rh, rsk;
  UL32 roff;
  const UC8 *s;
  UC8 big;

  p2.CopyRoi(patch);
  p2.RoiClip(src);
  rcnt = p2.RoiW() * src.Fields();
  rh = p2.RoiH();
  src.RoiParams(&roff, &rsk, p2);
  s = src.PxlSrc() + roff;
  big = *s;

  for (y = rh; y > 0; y--, s += rsk)
    for (x = rcnt; x > 0; x--, s++)
      big = __max(big, *s);
  return big;
}


//= Treats pixels as 16 bit values and finds maximum.
// always does whole image

int jhcStats::MaxVal_16 (const jhcImg& src) const 
{
  return MaxVal_16(src, src);
}


//= Treats pixels as 16 bit values and finds maximum in region.

int jhcStats::MaxVal_16 (const jhcImg& src, const jhcRoi& patch) const 
{
  jhcRoi p2;

  if (!src.Valid(2))
    return Fatal("Bad image to jhcStats::MaxVal_16");
  p2.CopyRoi(patch);
  p2.RoiClip(src);

  // local variables
  int x, y, h = p2.RoiH(), rw = p2.RoiW(), rsk = src.RoiSkip(p2) >> 1;
  const US16 *s = (const US16 *) src.RoiSrc(p2);
  int big = *s;

  for (y = h; y > 0; y--, s+= rsk)
    for (x = rw; x > 0; x--, s++)
      big = __max(big, *s);
  return big;
}


//= Finds best value and returns pixel location where this occurred.
// biased toward top and right if there are ties

int jhcStats::MaxLoc (int *mx, int *my, const jhcImg& src, const jhcRoi& area) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::MaxLoc");

  // process specified region
  jhcRoi a2;
  UL32 roff;
  const UC8 *s;
  int x, y, rw, rh, rsk, wx = 0, wy = 0, big = -1;

  a2.CopyRoi(area);
  a2.RoiClip(src);
  rw = a2.RoiW();
  rh = a2.RoiH();
  src.RoiParams(&roff, &rsk, a2);
  s = src.PxlSrc() + roff;
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*s >= big) 
      {
        big = *s;
        wx = x;
        wy = y;
      }
      s++;
    }
    s += rsk;
  }

  // copy relevant data
  if (mx != NULL)
    *mx = rw - wx + a2.RoiX();
  if (my != NULL)
    *my = rh - wy + a2.RoiY();
  return big;
}


//= Like MaxLoc but prefers maximum closest to centerline of area.

int jhcStats::MaxCentH (int *mx, int *my, const jhcImg& src, const jhcRoi& area) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::MaxCentH");

  // process specified region
  jhcRoi a2;
  const UC8 *s;
  UL32 roff;
  int x, y, rw, rh, rw2, rh2, rsk, poff, boff, wx = 0, wy = 0, big = -1;

  a2.CopyRoi(area);
  a2.RoiClip(src);
  rw = a2.RoiW();
  rh = a2.RoiH();
  rw2 = rw >> 1;
  rh2 = rh >> 1;
  src.RoiParams(&roff, &rsk, a2);
  s = src.PxlSrc() + roff;
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*s >= big) 
      {
        // compute distance to center of cropped ROI
        poff = abs(x - rw2);
        if ((big < 0) || (poff < boff))
        {
          big = *s;
          boff = poff;
          wx = x;
          wy = y;
        }
      }
      s++;
    }
    s += rsk;
  }

  // copy relevant data
  if (mx != NULL)
    *mx = rw - wx + a2.RoiX();
  if (my != NULL)
    *my = rh - wy + a2.RoiY();
  return big;
}


///////////////////////////////////////////////////////////////////////////
//                             Minima Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Find minimum value in whole image (at least part in default ROI).

int jhcStats::MinVal (const jhcImg& src) const 
{
  return MinVal(src, src);
}


//= Examines only a particular area given x, y, w, and h directly.

int jhcStats::MinVal (const jhcImg& src, int x, int y, int w, int h) const 
{
  jhcRoi patch(src);

  patch.SetRoi(x, y, w, h);
  return MinVal(src, patch);
}


//= Like other MinVal but takes position and size in an array: {x, y, w, h}.

int jhcStats::MinVal (const jhcImg& src, const int *specs) const 
{
  jhcRoi patch(src);

  patch.SetRoi(specs);
  return MinVal(src, patch);
}


//= Like other MinVal but extracts position and size of region from a ROI.

int jhcStats::MinVal (const jhcImg& src, const jhcRoi& patch) const 
{
  if (src.Valid(2))
    return MinVal_16(src, patch);
  if (!src.Valid())
    return Fatal("Bad image to jhcStats::MinVal");

  // process specified region
  jhcRoi p2;
  int x, y, rcnt, rh, rsk;
  UL32 roff;
  const UC8 *s;
  UC8 sm;

  p2.CopyRoi(patch);
  p2.RoiClip(src);
  rcnt = p2.RoiW() * src.Fields();
  rh = p2.RoiH();
  src.RoiParams(&roff, &rsk, p2);
  s = src.PxlSrc() + roff;
  sm = *s;
  for (y = rh; y > 0; y--, s += rsk)
    for (x = rcnt; x > 0; x--, s++)
      sm = __min(sm, *s);
  return sm;
}


//= Treats pixels as 16 bit values and finds minimum.
// always does whole image

int jhcStats::MinVal_16 (const jhcImg& src) const 
{
  return MinVal_16(src, src);
}


//= Treats pixels as 16 bit values and finds maximum in region.

int jhcStats::MinVal_16 (const jhcImg& src, const jhcRoi& patch) const 
{
  jhcRoi p2;

  if (!src.Valid(2))
    return Fatal("Bad image to jhcStats::MinVal_16");
  p2.CopyRoi(patch);
  p2.RoiClip(src);

  // local variables
  int x, y, h = p2.RoiH(), rw = p2.RoiW(), rsk = src.RoiSkip(p2) >> 1;
  const US16 *s = (const US16 *) src.RoiSrc(p2);
  int sm = *s;

  for (y = h; y > 0; y--, s+= rsk)
    for (x = rw; x > 0; x--, s++)
      sm = __min(sm, *s);
  return sm;
}


///////////////////////////////////////////////////////////////////////////
//                          Monochrome Averages                          //
///////////////////////////////////////////////////////////////////////////

//= Find average value of pixels in whole image (at least in default ROI).
// only considers values at or above threshold

double jhcStats::AvgVal (const jhcImg& src, int th) const 
{
  return AvgVal(src, src, th);
}


//= Examines only a particular area given x, y, w, and h directly.

double jhcStats::AvgVal (const jhcImg& src, int x, int y, int w, int h, int th) const 
{
  jhcRoi patch(src);

  patch.SetRoi(x, y, w, h);
  return AvgVal(src, patch, th);
}


//= Like other AvgVal but takes position and size in an array: {x, y, w, h}.

double jhcStats::AvgVal (const jhcImg& src, const int *specs, int th) const 
{
  jhcRoi patch(src);

  patch.SetRoi(specs);
  return AvgVal(src, patch, th);
}


//= Like other AvgVal but extracts position and size of region from a ROI.

double jhcStats::AvgVal (const jhcImg& src, const jhcRoi& patch, int th) const 
{
  if (src.Valid(2))
    return AvgVal_16(src, patch, th);
  if (!src.Valid())
  {
    Fatal("Bad image to jhcStats::AvgVal");
    return -1.0;
  }

  // process specified region
  jhcRoi p2;
  int x, y, rcnt, rh, rsk;
  UL32 roff;
  UC8 v = BOUND(th);
  UL32 sum = 0, cnt = 0;
  const UC8 *s;

  p2.CopyRoi(patch);
  p2.RoiClip(src);
  rcnt = p2.RoiW() * src.Fields();
  rh = p2.RoiH();
  src.RoiParams(&roff, &rsk, p2);
  s = src.PxlSrc() + roff;
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      if (*s >= v) 
      {
        sum += *s;
        cnt++;
      }
      s++;
    }
    s += rsk;
  }
  if (cnt == 0)
    return -1.0;
  return(sum / (double) cnt);
}


//= Get the average of values at or above threshold for a patch in a 16 bit image.

double jhcStats::AvgVal_16 (const jhcImg& src, const jhcRoi& patch, int th) const 
{
  jhcRoi p2;

  // check image and get constrained region
  if (!src.Valid(2))
  {
    Fatal("Bad image to jhcStats::AvgVal_16");
    return -1.0;
  }
  p2.CopyRoi(patch);
  p2.RoiClip(src);

  // general ROI parameters
  int x, y, rw = p2.RoiW(), rh = p2.RoiH(), rsk = src.RoiSkip(p2) >> 1;
  const US16 *s = (const US16 *) src.RoiSrc(p2);
  UL32 sum = 0, cnt = 0;

  // process specified region
  for (y = rh; y > 0; y--, s += rsk)
    for (x = rw; x > 0; x--, s++)
      if (*s >= th) 
      {
        sum += *s;
        cnt++;
      }

  // derive average from sum and count
  if (cnt == 0)
    return -1.0;
  return(sum / (double) cnt);
}


//= Computes average value in areas where mask pixel is greater than threshold.

double jhcStats::AvgOver (const jhcImg& src, const jhcImg& mask, int th) const 
{
  if (!src.Valid() || !src.SameSize(mask, 1))
  {
    Fatal("Bad image to jhcStats::AvgOver");
    return -1.0;
  }

  // local variables for unknown number of fields
  UC8 v = BOUND(th);
  UL32 sum = 0, cnt = 0;
  int i, x, y, nf = src.Fields(), rw = mask.RoiW(), rh = mask.RoiH();
  int rsk = mask.RoiSkip(), ssk = src.RoiSkip(mask);
  const UC8 *m = mask.RoiSrc(), *s = src.RoiSrc(mask);

  // examine pixels inside ROI of mask
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*m++ <= v) 
        s += nf;
      else
        for (i = nf; i > 0; i--)
        {
          sum += *s++;
          cnt++;
        }
    m += rsk;
    s += ssk;
  }
  if (cnt == 0)
    return -1.0;
  return(sum / (double) cnt);
}


//= Computes average value in areas where mask pixel is less than threshold.

double jhcStats::AvgUnder (const jhcImg& src, const jhcImg& mask, int th) const 
{
  if (!src.Valid() || !src.SameSize(mask, 1))
  {
    Fatal("Bad image to jhcStats::AvgUnder");
    return -1.0;
  }

  // local variables for unknown number of fields
  UC8 v = BOUND(th);
  UL32 sum = 0, cnt = 0;
  int i, x, y, nf = src.Fields(), rw = mask.RoiW(), rh = mask.RoiH();
  int rsk = mask.RoiSkip(), ssk = src.RoiSkip(mask);
  const UC8 *m = mask.RoiSrc(), *s = src.RoiSrc(mask);

  // examine pixels inside ROI of mask
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*m++ > v) 
        s += nf;
      else
        for (i = nf; i > 0; i--)
        {
          sum += *s++;
          cnt++;
        }
    m += rsk;
    s += ssk;
  }
  if (cnt == 0)
    return -1.0;
  return(sum / (double) cnt);
}


//= Find average absolute difference of pixels between two images.
// Resets ROIs in both images to be their intersection.

double jhcStats::AvgDiff (jhcImg& imga, jhcImg& imgb) const
{
  if (!imga.SameFormat(imgb))
  {
    Fatal("Bad image to jhcStats::AvgDiff");
    return -1.0;
  }
  imga.MergeRoi(imgb);
  imgb.CopyRoi(imga);

  // general ROI case
  int i, y, rcnt = imga.RoiCnt(), rh = imga.RoiH(), rsk = imga.RoiSkip();
  UL32 roff = imga.RoiOff(), sum = 0;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  
  // collect absolute differences
  for (y = rh; y > 0; y--)
  {
    for (i = rcnt; i > 0; i--)
      if (*a >= *b)
        sum += (*a++ - *b++);
      else
        sum += (*b++ - *a++);
    a += rsk;
    b += rsk;
  }

  // handle case of zero area ROI (avoids divide by zero also)
  if (sum == 0)
    return 0.0;
  return(sum / (double)(rcnt * rh));
}


///////////////////////////////////////////////////////////////////////////
//                             Color Averages                            //
///////////////////////////////////////////////////////////////////////////

//= Find separate R, G, and B averages in whole image (at least in default ROI).

int jhcStats::AvgRGB (double *r, double *g, double *b, const jhcImg& src) const 
{
  return AvgRGB(r, g, b, src, src);
}


//= Examines only a particular area given x, y, w, and h directly.

int jhcStats::AvgRGB (double *r, double *g, double *b, const jhcImg& src, 
                      int x, int y, int w, int h) const 
{
  jhcRoi patch(src);

  patch.SetRoi(x, y, w, h);
  return AvgRGB(r, g, b, src, patch);
}


//= Like other AvgRGB but takes position and size in an array: {x, y, w, h}.

int jhcStats::AvgRGB (double *r, double *g, double *b, const jhcImg& src, const int *specs) const 
{
  jhcRoi patch(src);

  patch.SetRoi(specs);
  return AvgRGB(r, g, b, src, patch);
}


//= Like other AvgRGB but extracts position and size of region from a ROI.

int jhcStats::AvgRGB (double *r, double *g, double *b, const jhcImg& src, const jhcRoi& patch) const 
{
  if (!src.Valid(3))
    return Fatal("Bad image to jhcStats::AvgRGB");

  // process specified region collecting sums 
  jhcRoi p2;
  int x, y, rw, rh, rsk;
  UL32 roff;
  double area;
  UL32 sum[3] = {0, 0, 0};
  const UC8 *s;

  p2.CopyRoi(patch);
  p2.RoiClip(src);
  rw = p2.RoiW();
  rh = p2.RoiH();
  src.RoiParams(&roff, &rsk, p2);
  s = src.PxlSrc() + roff;
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      sum[0] += s[0];
      sum[1] += s[1];
      sum[2] += s[2];
      s += 3;
    }
    s += rsk;
  }
  
  // turn sums into averages
  area = (double)(rw * rh);
  if (area <= 0.0)
  {
    *b = -1.0;
    *g = -1.0;
    *r = -1.0;
  }
  else
  {
    *b = sum[0] / area;
    *g = sum[1] / area;
    *r = sum[2] / area;
  }
  return 1;
}


//= Finds averages in three channels where mask is above threshold.

int jhcStats::AvgOverRGB (double *r, double *g, double *b, const jhcImg& src, 
                          const jhcImg& mask, int th) const 
{
  if (!src.Valid(3) || !src.SameSize(mask, 1))
    return Fatal("Bad image to jhcStats::AvgOverRGB");
  
  // process specified region collecting sums 
  int x, y, rw = mask.RoiW(), rh = mask.RoiH();
  int rsk = mask.RoiSkip(), ssk = src.RoiSkip(mask);
  const UC8 *m = mask.RoiSrc(), *s = src.RoiSrc(mask);
  UL32 sum[3] = {0, 0, 0};
  double area;

  // examine image inside ROI of mask
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*m++ > th)
      {
        sum[0] += s[0];
        sum[1] += s[1];
        sum[2] += s[2];
      }
      s += 3;
    }
    m += rsk;
    s += ssk;
  }
  
  // turn sums into averages
  area = (double)(rw * rh);
  if (area <= 0.0)
  {
    *b = -1.0;
    *g = -1.0;
    *r = -1.0;
  }
  else
  {
    *b = sum[0] / area;
    *g = sum[1] / area;
    *r = sum[2] / area;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Pixel Counting                             //
///////////////////////////////////////////////////////////////////////////

//= See if there are any non-zero pixels.

int jhcStats::AnyNZ (const jhcImg& src) const 
{
  if (!src.Valid())
    return Fatal("Bad image to jhcStats::AnyNZ");

  // general ROI case
  int x, y, rcnt = src.RoiCnt(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();

  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*s++ > 0)
        return 1;
    s += rsk;
  }
  return 0;
}


//= See if there is any pixel above threshold.

int jhcStats::AnyOver (const jhcImg& src, int th) const 
{
  if (!src.Valid())
    return Fatal("Bad image to jhcStats::AnyOver");

  // general ROI case
  int x, y, rcnt = src.RoiCnt(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();

  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*s++ > th)
        return 1;
    s += rsk;
  }
  return 0;
}


//= See if any pixel in patch of 16 bit image is above the threshold.
// returns 1 if something over, 0 if none over, -1 for no pixels in patch

int jhcStats::AnyOver_16 (const jhcImg& src, const jhcRoi& patch, int th) const
{
  jhcRoi p2;
  int x, y, rw, rh, sk;
  const US16 *s;

  if (!src.Valid(2))
    return Fatal("Bad image to jhcStats::AnyOver_16");

  // intersect ROI with image
  p2.CopyRoi(patch);
  p2.RoiClip(src);
  if (p2.RoiArea() <= 0)
    return -1;

  // get loop values and initial pixel
  rw = p2.RoiW();
  rh = p2.RoiH();
  sk = src.RoiSkip(p2) >> 1;
  s = (const US16 *) src.RoiSrc(p2);

  // check all pixels in region
  for (y = rh; y > 0; y--, s += sk)
    for (x = rw; x > 0; x--, s++)
      if (*s > th)
        return 1;
  return 0;
}


//= Count number of pixels above give threshold.

int jhcStats::CountOver (const jhcImg& src, int th) const 
{
  int ans = 0;

  if (src.Valid(2))
    return CountOver_16(src, th);
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::CountOver");
  if ((src.RoiW() % 4) == 0)
    return CountOver4(src, th);

  // general ROI case
  int x, y, rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  UC8 val = BOUND(th);
  const UC8 *s = src.RoiSrc();

  for (y = rh; y > 0; y--, s += rsk)
    for (x = rw; x > 0; x--, s++)
      if (*s > val)
        ans++;
  return ans;
}


//= Special version for ROIs that are multiples of 4 pixels wide.
// about 3x faster than general version -- uses explict loop unrolling 
// and lookup table to prevent instruction cache flush

int jhcStats::CountOver4 (const jhcImg& src, int th) const 
{
  int inc[256];
  int v, val = __max(0, __min(th, 255)), ans = 0;

  if (!src.Valid(1) || ((src.RoiW() % 4) != 0))
    return Fatal("Bad image to jhcStats::CountOver4");

  // build table
  for (v = 0; v <= val; v++)
    inc[v] = 0;
  for (v = val + 1; v <= 255; v++)
    inc[v] = 1;

  // general ROI case
  int x, y, rw4 = src.RoiW() >> 2, rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();

  for (y = rh; y > 0; y--, s += rsk)
    for (x = rw4; x > 0; x--, s += 4)
      ans += (inc[s[0]] + inc[s[1]] + inc[s[2]] + inc[s[3]]);
  return ans;
}


//= Count number of pixels in a 16 bit image that are above give threshold.

int jhcStats::CountOver_16 (const jhcImg& src, int th) const 
{
  if (!src.Valid(2))
    return Fatal("Bad image to jhcStats::CountOver_16");

  // general ROI case
  int x, y, rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip() >> 1, ans = 0;
  const US16 *s = (const US16 *) src.RoiSrc();

  for (y = rh; y > 0; y--, s += rsk)
    for (x = rw; x > 0; x--, s++)
      if (*s > th)
        ans++;
  return ans;
}


//= Count number of pixels above give threshold in a patch.

int jhcStats::CountOver (const jhcImg& src, const jhcRoi& patch, int th) const 
{
  int ans = 0;

  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::CountOver");

  // process specified region
  jhcRoi p2;
  int x, y, rcnt, rh, rsk;
  UL32 roff;
  const UC8 *s;

  p2.CopyRoi(patch);
  p2.RoiClip(src);
  rcnt = p2.RoiW() * src.Fields();
  rh = p2.RoiH();
  src.RoiParams(&roff, &rsk, p2);
  s = src.PxlSrc() + roff;
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*s++ > th)
        ans++;
    s += rsk;
  }
  return ans;
}


//= Complement of value from CountOver, counts pixels equal to or below threshold.

int jhcStats::CountUnder (const jhcImg& src, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::CountUnder");

  return(src.RoiArea() - CountOver(src, th - 1));
}


//= Count number of pixels under given threshold in a patch.

int jhcStats::CountUnder (const jhcImg& src, const jhcRoi& patch, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::CountUnder");

  return(patch.RoiArea() - CountOver(src, patch, th - 1));
}


//= Like CountOver but normalizes by the size of the image (or ROI).

double jhcStats::FracOver (const jhcImg& src, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::FracOver");

  return(CountOver(src, th) / (double) src.RoiArea());
}


//= Like other FracOver but restricts attention to a patch.

double jhcStats::FracOver (const jhcImg& src, const jhcRoi& patch, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::FracOver");

  return(CountOver(src, patch, th) / (double) patch.RoiArea());
}


//= Complement of value from FracOver, finds fraction equal to or below threshold.

double jhcStats::FracUnder (const jhcImg& src, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::FracUnder");

  return(1.0 - FracOver(src, th - 1));
}


//= Like other FracUnder but restricts attention to a patch.

double jhcStats::FracUnder (const jhcImg& src, const jhcRoi& patch, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::FracUnder");

  return(CountUnder(src, patch, th) / (double) patch.RoiArea());
}


///////////////////////////////////////////////////////////////////////////
//                         Border Pixel Counting                         //
///////////////////////////////////////////////////////////////////////////

//= See how many pixels are above threshold on a particular border of an image.
// side: 0 = left, 1 = top, 2 = right, 3 = bottom

int jhcStats::SideCount (const jhcImg& src, int side, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::SideCount");

  // local variables (ignores ROI)
  int i, n = src.XDim(), ssk = 1, e = side & 0x03;
  const UC8 *s = src.PxlSrc();
  int cnt = 0;

  // setup pixel start and stepping parameters
  if (e == 1)
    s = src.RoiSrc(0, src.YLim());
  else if (e == 2)
    s = src.RoiSrc(src.XLim(), 0);
  if ((e == 0) || (e == 2))
  {
    ssk = src.Line();
    n = src.YDim();
  } 

  // compute answer
  for (i = n; i > 0; i--, s += ssk)
    if (*s > th)
      cnt++;
  return cnt;
}


//= See what fraction of the pixels are above threshold on some image border.
// side: 0 = left, 1 = top, 2 = right, 3 = bottom

double jhcStats::SideFrac (const jhcImg& src, int side, int th) const 
{
  int e = side & 0x03, n = SideCount(src, side, th);

  if ((e == 0) || (e == 2))
    return(n / (double) src.YDim());
  return(n / (double) src.XDim());
}


///////////////////////////////////////////////////////////////////////////
//                           Shape Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Compute x and y coordinates of center after thresholding.
// essential pre-thresholds image and computes result for binary
// returns total area or -1 if error

int jhcStats::Centroid (double *xc, double *yc, const jhcImg& src, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::Centroid");

  // general ROI case
  int area = 0, xsum = 0, ysum = 0;
  int x, y, rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  
  // find centroid relative to ROI
  for (y = 0; y < rh; y++, s += rsk)
    for (x = 0; x < rw; x++, s++)
      if (*s > th)
      {
        xsum += x;
        ysum += y;
        area++;
      }

  // convert to full image coordinates
  if (area <= 0)
  {
    *xc = src.RoiAvgX();
    *yc = src.RoiAvgY();
    return -1;
  }
  *xc = xsum / (double) area + src.RoiX();
  *yc = ysum / (double) area + src.RoiY();
  return area;
}


//= Like other Centroid except constrained to a particular area.
// essentially pre-thresholds image

int jhcStats::Centroid (double *xc, double *yc, const jhcImg& src, const jhcRoi& patch, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::Centroid");

  // general ROI case
  jhcRoi p(patch);
  int x, y, rw, rh, rsk, area = 0, xsum = 0, ysum = 0;
  const UC8 *s;
  
  // sanity check on area
  p.RoiClip(src);
  rw = p.RoiW();
  rh = p.RoiH();
  rsk = src.RoiSkip(p);
  s = src.RoiSrc(p);

  // find centroid relative to ROI
  for (y = 0; y < rh; y++, s += rsk)
    for (x = 0; x < rw; x++, s++)
      if (*s > th)
      {
        xsum += x;
        ysum += y;
        area++;
      }

  // convert to full image coordinates
  if (area <= 0)
  {
    *xc = p.RoiAvgX();
    *yc = p.RoiAvgY();
    return -1;
  }
  *xc = xsum / (double) area + p.RoiX();
  *yc = ysum / (double) area + p.RoiY();
  return area;
}


//= Similar to Centroid but also gives eccentricity and orientation.
// constrained to a particular region and to pixel above threshold
// uses gray scale to weight contribution of each pixel
// returns total area or -1 if error

int jhcStats::Shape (double *xc, double *yc, double *ecc, double *ang, 
                     const jhcImg& src, const jhcRoi& patch, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::Shape");

  // general ROI case
  jhcRoi p;
  double xmid, ymid, mxx, myy, mxy, rt, den;
  long long x2sum = 0, y2sum = 0, xysum = 0;
  int x, y, rw, rh, rsk, wt, wtx, wty, cnt = 0, area = 0, xsum = 0, ysum = 0;
  const UC8 *s;

  // sanity check on area
  p.CopyRoi(patch);
  p.RoiClip(src);
  rw = p.RoiW();
  rh = p.RoiH();
  rsk = src.RoiSkip(p);
  s = src.RoiSrc(p);

  // find centroid relative to ROI 
  for (y = 0; y < rh; y++, s += rsk)
    for (x = 0; x < rw; x++, s++)
      if (*s > th)
      {
        // collect data for moments wrt ROI
        wt = *s;
        wtx = wt * x;
        wty = wt * y;
        x2sum += wtx * x;
        y2sum += wty * y;
        xysum += wtx * y;
        xsum += wtx;
        ysum += wty;
        area += wt;
        cnt++;
      }

  // make sure some pixels were examined
  if (cnt <= 0)
  {
    *xc = p.RoiAvgX();
    *yc = p.RoiAvgY();
    *ecc = 1.0;
    *ang = 0.0;
    return -1;
  }

  // digest raw data into central moments
  xmid = xsum / (double) area;
  ymid = ysum / (double) area;
  mxx = x2sum - (area * xmid * xmid);
  myy = y2sum - (area * ymid * ymid);
  mxy = xysum - (area * xmid * ymid);

  // save middle
  if (xc != NULL)
    *xc = xmid + p.RoiX();
  if (yc != NULL)
    *yc = ymid + p.RoiY();

  // find angle of orientation (degrees)
  if (ang != NULL)
  {
    if ((mxy == 0) && (mxx == mxy))
      *ang = 0.0;
    else
      *ang = 0.5 * R2D * atan2(-2.0 * mxy, mxx - myy);
    if (*ang < 0.0) 
      *ang += 180.0; 
  }

  // find equivalent ellipse length / width
  if (ecc != NULL)
  {
    rt = (double) sqrt(4.0 * mxy * mxy + (mxx - myy) * (mxx - myy));  
    den = mxx + myy - rt;
    if (den == 0.0)
      *ecc = 4.0 * area / PI;
    else
      *ecc = (double) sqrt((mxx + myy + rt) / den);
  }
  return cnt;
}


//= Similar to Shape but returns standard deviation in each direction
// constrained to a particular region and to pixel above threshold
// uses gray scale to weight contribution of each pixel
// returns weighted area or -1 if error

double jhcStats::Cloud (double *xc, double *yc, double *sdx, double *sdy, 
                        const jhcImg& src, const jhcRoi& patch, int th) const 
{
  if (!src.Valid(1) || (xc == NULL) || (yc == NULL) || (sdx == NULL) || (sdy == NULL))
    return Fatal("Bad input to jhcStats::Cloud");

  // general ROI case
  jhcRoi p(patch);
  double xmid, ymid;
  long long x2sum = 0, y2sum = 0;
  int x, y, wt, rw, rh, rsk, cnt = 0, area = 0, xsum = 0, ysum = 0;
  const UC8 *s;
  
  // sanity check on area
  p.RoiClip(src);
  rw = p.RoiW();
  rh = p.RoiH();
  rsk = src.RoiSkip(p);
  s = src.RoiSrc(p);

  // find centroid relative to ROI 
  for (y = 0; y < rh; y++, s += rsk)
    for (x = 0; x < rw; x++, s++)
      if (*s > th)
      {
        // collect data for moments wrt ROI
        wt = *s;
        x2sum += (wt * x * x);
        y2sum += (wt * y * y);
        xsum += (wt * x);
        ysum += (wt * y);
        area += wt;
        cnt++;
      }

  // make sure some pixels were examined
  if (cnt <= 0)
  {
    *xc = p.RoiAvgX();
    *yc = p.RoiAvgY();
    *sdx = 0.0;
    *sdy = 0.0;
    return -1;
  }

  // find means and variances E[(x - E[x])^2] = E[x^2] - E[x]^2
  xmid = xsum / (double) area;
  ymid = ysum / (double) area;
  *sdx = sqrt((x2sum / (double) area) - (xmid * xmid));
  *sdy = sqrt((y2sum / (double) area) - (ymid * ymid));

  // convert centroid to full image coordinates
  *xc = xmid + p.RoiX();
  *yc = ymid + p.RoiY();
  return((double) area);  // was cnt
}


//= Sets ROI to match area of image above threshold value.
// can give image itself as dest in order to set its active ROI
// returns number of pixels over threshold

int jhcStats::RegionNZ (jhcRoi &dest, const jhcImg& src, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::RegionNZ");
  dest.RoiClip(src);

  // general ROI case
  int x, y, x0, x1, y0, y1, cnt = 0;
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();
  
  // find min and max values above threshold
  for (y = 0; y < rh; y++, s += rsk)
    for (x = 0; x < rw; x++, s++)
      if (*s > th)
      {
        if (cnt++ == 0)
        {
          x0 = x;
          x1 = x;
          y0 = y;
          y1 = y;
        }
        else
        {
          if (x > x1)
            x1 = x;
          else if (x < x0)
            x0 = x;
          if (y > y1)
            y1 = y;
          else if (y < y0)
            y0 = y;
        }
      }

  // convert to full coordinates and load into ROI
  if (cnt == 0)
    dest.ClearRoi();
  else
    dest.SetRoi(x0 + src.RoiX(), y0 + src.RoiY(), x1 - x0 + 1, y1 - y0 + 1);
  return cnt;
}


//= Find the coordinates of the highest non-zero point in the image.

int jhcStats::PtMaxY (int& px, int& py, const jhcImg& src, int th, int bias) const
{
 if (!src.Valid(1))
    return Fatal("Bad image to jhcStats::PtMaxY");

  // general ROI case
  int x0 = src.RoiX(), xlim = src.RoiLimX(), y0 = src.RoiY(), ylim = src.RoiLimY();
  int x, y, ln = src.Line(), any = 0;
  const UC8 *s, *s0 = src.RoiSrc(x0, ylim);
  
  // scan down from the top stopping at first good pixel
  px = x0;
  py = y0;
  for (y = ylim; y >= y0; y--, s0 -= ln)
  {
    s = s0;
    for (x = x0; x <= xlim; x++, s++)
      if (*s > th)
      {
        px = x;
        py = y;
        any = 1;
        if (bias <= 0)                 // return min x value immediately
          return 1;
      }
    if (any > 0)                       // all pixels on winning line examined
      break;
  }
  return 1;
}


