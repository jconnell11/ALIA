// jhcThresh.cpp : useful thresholding and gating operations
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
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

#include "Interface/jhcMessage.h"

#include "Processing/jhcThresh.h"


///////////////////////////////////////////////////////////////////////////
//                           Sharp Thresholds                            //
///////////////////////////////////////////////////////////////////////////

//= Fill destination with thresholded version of source image.
// if threshold is negative, values above level get zero

int jhcThresh_0::Threshold (jhcImg& dest, const jhcImg& src, int th, int mark) const 
{
  if (src.Valid(2))
    return Thresh16(dest, src, th, mark);
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::Threshold");
  dest.CopyRoi(src);

  // check for simplest cases
  if ((th == 0) && (mark == 0))
    return dest.FillArr(BOUND(mark));
  if (th == 255)
    return dest.FillArr(0);

  // general ROI case
  int x, y, i;
  UC8 val = BOUND(th);
  UC8 under = 0, over = BOUND(mark);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 thv[256];
 
  // compute result for all possible input values
  if (th < 0)
  {
    val = BOUND(-th);
    under = over;
    over = 0;
  }
  for (i = 0; i <= 255; i++)
    if (i > val)
      thv[i] = over;
    else
      thv[i] = under;

  // apply lookup table to image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      *d = thv[*s];
  return 1;
}


//= Threshold input image but only within ROI given (ignores src ROI).

int jhcThresh_0::Threshold (jhcImg& dest, const jhcImg& src, const jhcRoi& area, int th, int mark) const
{
  if (dest.Valid(2) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::Threshold");
 
  // general ROI case
  UC8 thv[256];
  int x, y, i, rcnt = area.RoiW() * dest.Fields(), rh = area.RoiH(), rsk = dest.RoiSkip(area);
  UC8 val = BOUND(th), under = 0, over = BOUND(mark);
  const UC8 *s = src.RoiSrc(area);
  UC8 *d = dest.RoiDest(area);

  // compute result for all possible input values
  if (th < 0)
  {
    val = BOUND(-th);
    under = over;
    over = 0;
  }
  for (i = 0; i <= 255; i++)
    if (i > val)
      thv[i] = over;
    else
      thv[i] = under;

  // apply lookup table to image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      *d = thv[*s];
  return 1;
}


//= Threshold a 16 bit image making pixels above "th" have value "mark".
// if threshold is negative, values above level get zero

int jhcThresh_0::Thresh16 (jhcImg& dest, const jhcImg& src, int th, int mark) const 
{
  if (!src.Valid(2) || !src.SameSize(dest, 1))
    return Fatal("Bad images to jhcThresh::Thresh16");
  dest.CopyRoi(src);

  // check for simplest cases
  if ((th == 0) && (mark == 0))
    return dest.FillArr(mark);
  if (th == 255)
    return dest.FillArr(0);

  // general ROI case
  UC8 under = 0, over = BOUND(mark);
  int x, y, val = th, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip() >> 1;
  UC8 *d = dest.RoiDest();
  const US16 *s = (const US16 *) src.RoiSrc();

  // see if pixels under threshold should be marked instead
  if (th < 0)
  {
    val = -th;
    under = over;
    over = 0;
  }

  // apply threshold to image
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s++)
      if (*s > val)
        *d = over;
      else
        *d = under;
  return 1;
}


//= Marks as 255 any pixels in range [lo hi] inclusive while others are zeroed.

int jhcThresh_0::Between (jhcImg& dest, const jhcImg& src, int lo, int hi, int mark) const
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::Between");
  dest.CopyRoi(src);
  
  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 m = BOUND(mark);
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // test all pixels and record result
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      if ((*s >= lo) && (*s <= hi))
        *d = m;
      else
        *d = 0;
  return 1; 
}


//= Looks for deviations of more than threshold from middle value of 128.
// useful for things like RawSobel edge magnitudes

int jhcThresh_0::AbsOver (jhcImg& dest, const jhcImg& src, int th) const
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::AbsOver");
  dest.CopyRoi(src);
  
  // general ROI case
  int lo = 128 - th, hi = 128 + th;
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // test all pixels and record result
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      if ((*s < lo) || (*s > hi))
        *d = 255;
      else
        *d = 0;
  return 1; 

}


//= Make a three level image of 0, 128, and 255 based on thresholds.
// normally [0]lo[128]hi[255] unless range reversed: [255]hi[128]lo[0]

int jhcThresh_0::Trinary (jhcImg& dest, const jhcImg& src, int lo, int hi) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::Trinary");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 thv[256];

  // compute result for all possible input values
  if (hi >= lo)
  {
    for (i = 0; i <= 255; i++)
      if (i <= lo)
        thv[i] = 0;
      else if (i < hi)
        thv[i] = 128;
      else
        thv[i] = 255;
  }
  else
  {
    for (i = 0; i <= 255; i++)
      if (i <= hi)
        thv[i] = 255;
      else if (i < lo)
        thv[i] = 128;
      else
        thv[i] = 0;
  }

  // apply lookup table to image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      *d = thv[*s];
  return 1;
}


//= Sets dest to 255 if both src1 and src2 fall between lo and hi inclusive.

int jhcThresh_0::BothWithin (jhcImg& dest, const jhcImg& src1, const jhcImg& src2, int lo, int hi) const 
{
  if (!dest.SameFormat(src1) || !dest.SameFormat(src2))
    return Fatal("Bad images to jhcThresh::BothWithin");
  dest.CopyRoi(src1);
  dest.MergeRoi(src2);

  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s1 = src1.RoiSrc(dest), *s2 = src2.RoiSrc(dest);

  // test all pixels and record result
  for (y = rh; y > 0; y--, d += rsk, s1 += rsk, s2 += rsk)
    for (x = rcnt; x > 0; x--, d++ , s1++, s2++)
      if ((*s1 >= lo) && (*s1 <= hi) && (*s2 >= lo) && (*s2 <= hi))
        *d = 255;
      else
        *d = 0;
  return 1; 
}


//= Mark areas in destination where src has exactly the key value.
// works with monochrome of double field images (e.g. connected components)

int jhcThresh_0::MatchKey (jhcImg& dest, const jhcImg& src, int key, int mark) const 
{
  if (!src.Valid(1, 2) || !src.SameSize(dest, 1))
    return Fatal("Bad images to jhcThresh::MatchKey");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip(), ssk2 = ssk >> 1;
  UC8 m = BOUND(mark);
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  const US16 *s2 = (const US16 *) s;

  if (src.Fields() == 1)        // single field source
  {
    for (y = rh; y > 0; y--, d += dsk, s += ssk)
      for (x = rw; x > 0; x--, d++, s++)
        if (*s == key)
          *d = m;
        else
          *d = 0;
  }
  else                          // double field source
  {
    for (y = rh; y > 0; y--, d += dsk, s += ssk2)
      for (x = rw; x > 0; x--, d++, s2++)
        if (*s2 == key)
          *d = m;
        else
          *d = 0;
  }
  return 1;
}


//= Sets values close to 128 to be exactly 128.

int jhcThresh_0::DeadBand (jhcImg& dest, const jhcImg& src, int delta) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::DeadBand");
  dest.CopyRoi(src);
  
  // general ROI case
  int x, y, lo = 128 - delta, hi = 128 + delta;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // test all pixels and record result
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      if ((*s >= lo) && (*s <= hi))
        *d = 128;
      else
        *d = *s;
  return 1; 
}


//= Sets values below level to be zero, leaves higher values untouched.

int jhcThresh_0::Squelch (jhcImg& dest, const jhcImg& src, int level) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::Squelch");
  dest.CopyRoi(src);
  
  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // test all pixels and record result
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      if (*s >= level)
        *d = *s;
      else
        *d = 0;
  return 1; 
}


//= Sets values above threshold to zero but leaves others unchanged.

int jhcThresh_0::ZeroOver (jhcImg& dest, const jhcImg& src, int level) const
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::ZeroOver");
  dest.CopyRoi(src);
  
  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // test all pixels and record result
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      if (*s <= level)
        *d = *s;
      else
        *d = 0;
  return 1; 
}


//= Mark pixels in src that are at least delta over the values in ref.
// can provide a negative delta to mark src > ref - abs(delta)

int jhcThresh_0::OverBy (jhcImg& dest, const jhcImg& src, const jhcImg& ref, int delta, int mark) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(ref))
    return Fatal("Bad images to jhcThresh::OverBy");
  dest.CopyRoi(src);
  dest.MergeRoi(ref);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(dest), *r = ref.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--, d += rsk, s += rsk, r += rsk)
    for (x = rw; x > 0; x--, d++, s++, r++)
      if (*s > (*r + delta))
        *d = (UC8) mark;
      else
        *d = 0;
  return 1;
}


//= Mark pixels in src that are at least delta under the values in ref.
// can provide a negative delta to mark src < ref + abs(delta)

int jhcThresh_0::UnderBy (jhcImg& dest, const jhcImg& src, const jhcImg& ref, int delta, int mark) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(ref))
    return Fatal("Bad images to jhcThresh::UnderBy");
  dest.CopyRoi(src);
  dest.MergeRoi(ref);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(dest), *r = ref.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--, d += rsk, s += rsk, r += rsk)
    for (x = rw; x > 0; x--, d++, s++, r++)
      if (*s < (*r - delta))
        *d = (UC8) mark;
      else
        *d = 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Soft Thresholding                           //
///////////////////////////////////////////////////////////////////////////

//= Input values between limits map onto a linear slope between the ends.
// all values below "lo" get 0, those at or above "hi" get 255
// used to be called "RampThresh"

int jhcThresh_0::RampOver (jhcImg& dest, const jhcImg& src, int lo, int hi) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::RampOver");
  if (hi < lo)
    return 0;
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  UC8 thv[256];
  double sc = 0.0;

  // compute results for all possible input values
  if (hi != lo)
    sc = 255.0 / (double)(hi - lo);
  for (i = 0; i <= 255; i++)
    if (i < lo)
      thv[i] = 0;
    else if (i >= hi)
      thv[i] = 255;
    else
      thv[i] = BOUND(ROUND(sc * (i - lo)));

  // apply lookup table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = thv[*s++];
    d += rsk;
    s += rsk;
  }
  return 1; 
}


//= Variation where a middle value and slope are given.

int jhcThresh_0::RampOver (jhcImg& dest, const jhcImg& src, int mid, double slope) const 
{
  int delta;

  if (slope <= 0.0)
    return 0;

  delta = ROUND(127.5 / slope);
  return RampOver(dest, src, mid - delta, mid + delta);
}


//= Input values between limits map onto a linear slope between the ends.
// all values below "lo" get 255, those at or above "hi" get 0
// used to be called "RampThresh"

int jhcThresh_0::RampUnder (jhcImg& dest, const jhcImg& src, int lo, int hi) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::RampUnder");
  if (hi < lo)
    return 0;
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  UC8 thv[256];
  double sc = 0.0;

  // compute results for all possible input values
  if (hi != lo)
    sc = 255.0 / (double)(hi - lo);
  for (i = 0; i <= 255; i++)
    if (i < lo)
      thv[i] = 255;
    else if (i >= hi)
      thv[i] = 0;
    else
      thv[i] = 255 - BOUND(ROUND(sc * (i - lo)));

  // apply lookup table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = thv[*s++];
    d += rsk;
    s += rsk;
  }
  return 1; 
}


//= Variation where a middle value and slope are given.

int jhcThresh_0::RampUnder (jhcImg& dest, const jhcImg& src, int mid, double slope) const 
{
  int delta;

  if (slope <= 0.0)
    return 0;

  delta = ROUND(127.5 / slope);
  return RampUnder(dest, src, mid - delta, mid + delta);
}


//= Variation on gradual threshold where a value and allowed deviation are given.
// used to be called "SoftThresh"

int jhcThresh_0::SoftOver (jhcImg& dest, const jhcImg& src, int th, int dev) const 
{
  return RampOver(dest, src, th - dev, th + dev);
}


//= Another variation where a value and fractional change are given.

int jhcThresh_0::SoftOver (jhcImg& dest, const jhcImg& src, int th, double frac) const 
{
  int lo = ROUND((1.0 - frac) * th), hi = ROUND((1.0 + frac) * th);

  return RampOver(dest, src, lo, hi);
}


//= Variation on gradual threshold where a value and allowed deviation are given.
// used to be called "SoftThresh"

int jhcThresh_0::SoftUnder (jhcImg& dest, const jhcImg& src, int th, int dev) const 
{
  return RampUnder(dest, src, th - dev, th + dev);
}


//= Another variation where a value and fractional change are given.

int jhcThresh_0::SoftUnder (jhcImg& dest, const jhcImg& src, int th, double frac) const 
{
  int lo = ROUND((1.0 - frac) * th), hi = ROUND((1.0 + frac) * th);

  return RampUnder(dest, src, lo, hi);
}


//= Trapezoidal membership function.
// ramps from 0 to 255 starting at lo - dev up to lo + dev
// ramps from 255 to 0 starting at hi - dev up to hi + dev
// if lo greater than hi then values outside range are 255 instead of 0

int jhcThresh_0::InRange (jhcImg& dest, const jhcImg& src, int lo, int hi, int dev) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::InRange");
  if (dev < 0)
    return 0;
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, v0, v1, v2, v3, l = lo, h = hi;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  UC8 thv[256];
  double sc = 0.0;

  // check for range reversal and triangular vs trapezoidal
  if (hi < lo)
  {
    l = hi;
    h = lo;
  }
  v0 = l - dev;
  v1 = l + dev;
  v2 = h - dev;
  v3 = h + dev;
  if (v2 < v1)
    v1 = (v1 + v2) / 2;

  // compute results for all possible input values
  if (dev != 0)
    sc = 255.0 / (2.0 * dev);
  for (i = 0; i <= 255; i++)
    if (i < v0)
      thv[i] = 0;
    else if (i < v1)
      thv[i] = BOUND(ROUND(sc * (i - v0)));
    else if (i <= v2)
      thv[i] = 255;
    else if (i < v3)
      thv[i] = 255 - BOUND(ROUND(sc * (i - v2)));
    else
      thv[i] = 0;
  if (hi < lo)
    for (i = 0; i <= 255; i++)
      thv[i] = 255 - thv[i];

  // apply lookup table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = thv[*s++];
    d += rsk;
    s += rsk;
  }
  return 1; 
}


///////////////////////////////////////////////////////////////////////////
//                   Direct Soft Threshold Combination                   //
///////////////////////////////////////////////////////////////////////////

//= Takes minimum of old and soft thresholding of src.
// values mapped from 0 under th - soft, to 255 at th + soft

int jhcThresh_0::MinOver (jhcImg& dest, const jhcImg& old, const jhcImg& src, int th, int soft) const 
{
  if (!dest.SameFormat(src) || !old.SameFormat(src))
    return Fatal("Bad images to jhcThresh::MinOver");
  if (soft < 0)
    return 0;
  dest.CopyRoi(src);
  dest.MergeRoi(old);

  // general ROI case
  int x, y, i, lo = th - soft, hi = th + soft;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff, *p = old.PxlSrc() + roff;
  UC8 thv[256];
  UC8 v0, v;
  double sc = 0.0;

  // compute results for all possible input values
  if (hi != lo)
    sc = 255.0 / (double)(hi - lo);
  for (i = 0; i <= 255; i++)
    if (i < lo)
      thv[i] = 0;
    else if (i >= hi)
      thv[i] = 255;
    else
      thv[i] = BOUND(ROUND(sc * (i - lo)));

  // apply lookup table to image and combine with old value
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      v0 = *p++;
      v  = thv[*s++];
      *d++ = __min(v0, v);
    }
    d += rsk;
    s += rsk;
    p += rsk;
  }
  return 1; 
}


//= Takes minimum of old and soft thresholding of src.
// values mapped from 255 at th - soft, to 0 over th + soft

int jhcThresh_0::MinUnder (jhcImg& dest, const jhcImg& old, const jhcImg& src, int th, int soft) const 
{
  if (!dest.SameFormat(src) || !old.SameFormat(src))
    return Fatal("Bad images to jhcThresh::MinUnder");
  if (soft < 0)
    return 0;
  dest.CopyRoi(src);
  dest.MergeRoi(old);

  // general ROI case
  int x, y, i, lo = th - soft, hi = th + soft;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff, *p = old.PxlSrc() + roff;
  UC8 thv[256];
  UC8 v0, v;
  double sc = 0.0;

  // compute results for all possible input values
  if (hi != lo)
    sc = 255.0 / (double)(hi - lo);
  for (i = 0; i <= 255; i++)
    if (i < lo)
      thv[i] = 255;
    else if (i >= hi)
      thv[i] = 0;
    else
      thv[i] = 255 - BOUND(ROUND(sc * (i - lo)));

  // apply lookup table to image and combine with old value
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      v0 = *p++;
      v = thv[*s++];
      *d++ = __min(v0, v);
    }
    d += rsk;
    s += rsk;
    p += rsk;
  }
  return 1; 
}


//= Takes maximum of old and soft thresholding of src.
// values mapped from 0 under th - soft, to 255 at th + soft

int jhcThresh_0::MaxOver (jhcImg& dest, const jhcImg& old, const jhcImg& src, int th, int soft) const 
{
  if (!dest.SameFormat(src) || !old.SameFormat(src))
    return Fatal("Bad images to jhcThresh::MaxOver");
  if (soft < 0)
    return 0;
  dest.CopyRoi(src);
  dest.MergeRoi(old);

  // general ROI case
  int x, y, i, lo = th - soft, hi = th + soft;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff, *p = old.PxlSrc() + roff;
  UC8 thv[256];
  UC8 v0, v;
  double sc = 0.0;

  // compute results for all possible input values
  if (hi != lo)
    sc = 255.0 / (double)(hi - lo);
  for (i = 0; i <= 255; i++)
    if (i < lo)
      thv[i] = 0;
    else if (i >= hi)
      thv[i] = 255;
    else
      thv[i] = BOUND(ROUND(sc * (i - lo)));

  // apply lookup table to image and combine with old value
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      v0 = *p++;
      v  = thv[*s++];
      *d++ = __max(v0, v);
    }
    d += rsk;
    s += rsk;
    p += rsk;
  }
  return 1; 
}


//= Takes maximum of old and soft thresholding of src.
// values mapped from 255 at th - soft, to 0 over th + soft

int jhcThresh_0::MaxUnder (jhcImg& dest, const jhcImg& old, const jhcImg& src, int th, int soft) const 
{
  if (!dest.SameFormat(src) || !old.SameFormat(src))
    return Fatal("Bad images to jhcThresh::MaxUnder");
  if (soft < 0)
    return 0;
  dest.CopyRoi(src);
  dest.MergeRoi(old);

  // general ROI case
  int x, y, i, lo = th - soft, hi = th + soft;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff, *p = old.PxlSrc() + roff;
  UC8 thv[256];
  UC8 v0, v;
  double sc = 0.0;

  // compute results for all possible input values
  if (hi != lo)
    sc = 255.0 / (double)(hi - lo);
  for (i = 0; i <= 255; i++)
    if (i < lo)
      thv[i] = 255;
    else if (i >= hi)
      thv[i] = 0;
    else
      thv[i] = 255 - BOUND(ROUND(sc * (i - lo)));

  // apply lookup table to image and combine with old value
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      v0 = *p++;
      v = thv[*s++];
      *d++ = __max(v0, v);
    }
    d += rsk;
    s += rsk;
    p += rsk;
  }
  return 1; 
}


//= If keep src where greater than lo1 and lo2 else set dest to zero.

int jhcThresh_0::KeepPeak (jhcImg& dest, const jhcImg& lo1, const jhcImg& src, const jhcImg& lo2) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(lo1) || !dest.SameFormat(lo2) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcThresh::KeepPeak");
  dest.CopyRoi(lo1);
  dest.MergeRoi(src);
  dest.MergeRoi(lo2);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(dest), *a = lo1.RoiSrc(dest), *b = lo2.RoiSrc();
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--, d += rsk, s += rsk, a += rsk, b += rsk)
    for (x = rw; x > 0; x--, d++, s++, a++, b++)
      if ((*s > *a) && (*s > *b))
        *d = *s;
      else
        *d = 0;
  return 1;
}


//= Set destination to alt value where source is at or below bar (else leave same).

int jhcThresh_0::KeepOver (jhcImg& dest, const jhcImg& src, const jhcImg& bar, int alt) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(bar))
    return Fatal("Bad images to jhcThresh::KeepOver");
  dest.CopyRoi(src);
  dest.MergeRoi(bar);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(dest), *b = bar.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--, d += rsk, s += rsk, b += rsk)
    for (x = rw; x > 0; x--, d++, s++, b++)
      if (*s <= *b)
        *d = (UC8) alt;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                               Gating                                  //
///////////////////////////////////////////////////////////////////////////

//= Gates monochrome image either above or below given value depending on sign.

int jhcThresh_0::ThreshGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, int def) const 
{
  if (th < 0)
    return UnderGate(dest, src, gate, -th, def);
  return OverGate(dest, src, gate, th, def);
}


//= Gates color image either above or below given value depending on sign.

int jhcThresh_0::ThreshGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, 
                                int rdef, int gdef, int bdef) const 
{
  if (th < 0)
    return UnderGateRGB(dest, src, gate, -th, rdef, gdef, bdef);
  return OverGateRGB(dest, src, gate, th, rdef, gdef, bdef);
}


//= Copy values when indicator is over threshold, else zero.
// last argument is value to insert when gating test fails (e.g. 0)

int jhcThresh_0::OverGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, int def) const 
{
  if (dest.Valid(1))
    return OverGateBW(dest, src, gate, th, def);
  if (dest.Valid(2))
    return OverGate16(dest, src, gate, th, def);
  if (dest.Valid(3))
    return OverGateRGB(dest, src, gate, th, def, def, def);
  if (!dest.SameFormat(src) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::OverGate");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, i;
  UC8 dval = BOUND(def), v = BOUND(th);
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*g++ > v)
        for (i = nf; i > 0; i--)
          *d++ = *s++;
      else
      { 
        for (i = nf; i > 0; i--)
          *d++ = dval;
        s += nf;
      }
    d += rsk;
    s += rsk;
    g += gsk;
  }
  return 1;
}


//= Like basic version but for images where source is RGB and gate is monochrome.

int jhcThresh_0::OverGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, 
                            int rdef, int gdef, int bdef) const 
{
  if (!dest.Valid(3) || !dest.SameFormat(src) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::OverGateRGB");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // general ROI case
  UC8 v = BOUND(th);
  UC8 defs[3];
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip(dest);
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest), *g = gate.RoiSrc(dest);

  defs[0] = BOUND(bdef);
  defs[1] = BOUND(gdef);
  defs[2] = BOUND(rdef);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*g++ > v)
      {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
      }
      else
      { 
        d[0] = defs[0];
        d[1] = defs[1];
        d[2] = defs[2];
      }
      d += 3;
      s += 3;        
    }
    d += rsk;
    s += rsk;
    g += gsk;
  }
  return 1;
}


//= Like basic version but for case where src and dest are both monochrome.
// special 32 bit version is not noticeably faster

int jhcThresh_0::OverGateBW (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, int def) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(gate))
    return Fatal("Bad images to jhcThresh::OverGateBW");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // check for simplest cases
  if (th < 0) 
    return dest.CopyArr(src);
  if (th >= 255)
    return dest.FillArr(def);

  // local variables
  UC8 dval = BOUND(def), v = BOUND(th);
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest), *g = gate.RoiSrc(dest);

  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*g++ > v)
        *d++ = *s++;
      else
      {
        *d++ = dval;
        s++;
      }
    d += rsk;
    s += rsk;
    g += rsk;
  }
  return 1;
}


//= Like basic version but for case where src and dest are both 16 bit.

int jhcThresh_0::OverGate16 (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, int def) const 
{
  if (!dest.Valid(2) || !dest.SameFormat(src) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::OverGate16");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // check for simplest cases
  if (th < 0) 
    return dest.CopyArr(src);
  if (th >= 65535)
    return dest.FillArr(def);

  // local variables
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk2 = dest.RoiSkip() >> 1, gsk = gate.RoiSkip(dest);
  US16 dval = (US16)(__max(0, __min(def, 65535)));
  const UC8 *g = gate.RoiSrc(dest);
  const US16 *s = (const US16 *) src.RoiSrc(dest);
  US16 *d = (US16 *) dest.RoiDest();

  for (y = rh; y > 0; y--, d += rsk2, s += rsk2, g += gsk)
    for (x = rw; x > 0; x--, d++, s++, g++)
      if (*g > th)
        *d = *s;
      else
        *d = dval;
  return 1;
}


//= Copy values when indicator is under threshold, else zero.
// last argument is value to insert when gating test fails (e.g. 0)

int jhcThresh_0::UnderGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, int def) const 
{
  if (dest.SameFormat(gate))
    return UnderGateBW(dest, src, gate, th, def);
  if (dest.Valid(3) && gate.Valid(1))
    return UnderGateRGB(dest, src, gate, th, def, def, def);
  if (!dest.SameFormat(src) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::UnderGate");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, i;
  UC8 dval = BOUND(def), v = BOUND(th);
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*g++ < v)
        for (i = nf; i > 0; i--)
          *d++ = *s++;
      else
      { 
        for (i = nf; i > 0; i--)
          *d++ = dval;
        s += nf;
      }
    d += rsk;
    s += rsk;
    g += gsk;
  }
  return 1;
}


//= Like basic version but for images with 3 fields.

int jhcThresh_0::UnderGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, 
                               int rdef, int gdef, int bdef) const 
{
  if (!dest.Valid(3) || !dest.SameFormat(src) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::UnderGateRGB");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // general ROI case
  UC8 v = BOUND(th);
  UC8 defs[3];
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip(dest);
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest), *g = gate.RoiSrc(dest);

  defs[0] = BOUND(bdef);
  defs[1] = BOUND(gdef);
  defs[2] = BOUND(rdef);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*g++ < v)
      {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
      }
      else
      { 
        d[0] = defs[0];
        d[1] = defs[1];
        d[2] = defs[2];
      }
      d += 3;
      s += 3;        
    }
    d += rsk;
    s += rsk;
    g += gsk;
  }
  return 1;
}


//= Like basic version but for case where src and gate (typically monochrome).
// special 32 bit version is not noticeably faster

int jhcThresh_0::UnderGateBW (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, int def) const 
{
  if (!dest.SameFormat(src) || !dest.SameFormat(gate))
    return Fatal("Bad images to jhcThresh::UnderGateBW");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // check for simplest cases
  if (th <= 0)
    return dest.FillArr(def);
  if (th > 255)
    return dest.CopyArr(src);

  // local variables
  int x, y;
  UC8 dval = BOUND(def), v = BOUND(th);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8  *s = src.RoiSrc(dest), *g = gate.RoiSrc(dest);

  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*g++ < v)
        *d++ = *s++;
      else
      {
        *d++ = dval;
        s++;
      }
    d += rsk;
    s += rsk;
    g += rsk;
  }
  return 1;
}


//= Copy values when indicator is between thresholds (inclusive).
// if lo >= hi then copy values when outside range
// last argument is value to insert when gating test fails (e.g. 0)

int jhcThresh_0::BandGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int lo, int hi, int def) const 
{
  if (dest.SameFormat(gate))
    return BandGateBW(dest, src, gate, lo, hi, def);
  if (dest.Valid(3) && gate.Valid(1))
    return BandGateRGB(dest, src, gate, lo, hi, def, def, def);
  if (!dest.SameFormat(src) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::BandGate");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, i, rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();

  // see if notch instead of band
  if (lo >= hi)
  {
    for (y = rh; y > 0; y--, d += rsk, s += rsk, g += gsk)
      for (x = rw; x > 0; x--, d += nf, s += nf, g++)
        if ((*g < hi) || (*g > lo))                         
          for (i = nf; i > 0; i--)
            d[i] = s[i];
        else
          for (i = nf; i > 0; i--)
            d[i] = (UC8) def;
    return 1;
  }

  // within limits
  for (y = rh; y > 0; y--, d += rsk, s += rsk, g += gsk)
    for (x = rw; x > 0; x--, d += nf, s += nf, g++)
      if ((*g >= lo) && (*g <= hi))
        for (i = nf; i > 0; i--)
          d[i] = s[i];
      else
        for (i = nf; i > 0; i--)
          d[i] = (UC8) def;
  return 1;
}


//= Like basic version but for images where source is RGB and gate is monochrome.

int jhcThresh_0::BandGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int lo, int hi, 
                              int rdef, int gdef, int bdef) const 
{
  if (!dest.Valid(3) || !dest.SameFormat(src) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::BandGateRGB");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip(dest);
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest), *g = gate.RoiSrc(dest);

  // see if notch instead of band
  if (lo >= hi)
  {
    for (y = rh; y > 0; y--, d += rsk, s += rsk, g += gsk)
      for (x = rw; x > 0; x--, d += 3, s += 3, g++)
        if ((*g < hi) || (*g > lo))
        {
          d[0] = s[0];
          d[1] = s[1];
          d[2] = s[2];
        }
        else
        { 
          d[0] = (UC8) bdef;
          d[1] = (UC8) gdef;
          d[2] = (UC8) rdef;
        }
    return 1;
  }

  // within limits
  for (y = rh; y > 0; y--, d += rsk, s += rsk, g += gsk)
    for (x = rw; x > 0; x--, d += 3, s += 3, g++)
      if ((*g >= lo) && (*g <= hi))
      {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
      }
      else
      { 
        d[0] = (UC8) bdef;
        d[1] = (UC8) gdef;
        d[2] = (UC8) rdef;
      }
  return 1;
}


//= Like basic version but for case where src and dest are same depth (usually mono).

int jhcThresh_0::BandGateBW (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int lo, int hi, int def) const 
{
  if (!dest.SameFormat(src) || !dest.SameFormat(gate))
    return Fatal("Bad images to jhcThresh::BandGateBW");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // local variables
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest), *g = gate.RoiSrc(dest);

  // see if notch instead of band
  if (lo >= hi)
  {
    for (y = rh; y > 0; y--, d += rsk, s += rsk, g += rsk)
      for (x = rcnt; x > 0; x--, d++, s++, g++)
        if ((*g < hi) || (*g > lo))
          *d = *s;
        else
          *d = (UC8) def;
    return 1;
  }

  // within limits
  for (y = rh; y > 0; y--, d += rsk, s += rsk, g += rsk)
    for (x = rcnt; x > 0; x--, d++, s++, g++)
      if ((*g >= lo) && (*g <= hi))
        *d = *s;
      else
        *d = (UC8) def;
  return 1;
}


//= Multiply value by mask when mask is non-zero, mix default for rest.
// actually computes dest = gate * src + (1 - gate) * def;

int jhcThresh_0::AlphaGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int def) const 
{
  if (!dest.SameFormat(src) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::AlphaGate");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);
  if (dest.Fields() == 1)
    return AlphaGateBW(dest, src, gate, def);
  if (dest.Fields() == 3)
    return AlphaGateRGB(dest, src, gate, def, def, def);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, i, m, v;
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip();
  UC8 sub = BOUND(def);
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff; 
  const UC8 *s = src.PxlSrc() + roff;
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();
  UC8 dval[256];

  // compute all fractions of the default value 
  // will be given alpha so use complement of this in table
  for (m = 0; m <= 255; m++)
  {
    v = ((256 - m) * sub) >> 8;
    dval[m] = BOUND(v);
  }

  // process pixels either using multiplication and the table
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      m = (int)(*g++);
      if (m == 255)
        for (i = nf; i > 0; i--)
          *d++ = *s++;
      else if (m == 0)
      {
        for (i = nf; i > 0; i--)
          *d++ = sub;
        s += nf;
      }
      else
      {
        v = dval[m];
        m++;
        for (i = nf; i > 0; i--)
          *d++ = (UC8)(((m * (int)(*s++)) >> 8) + v);
      }
    }
    d += rsk;
    s += rsk;
    g += gsk;
  }
  return 1;
}


//= Like basic version but for images with 3 fields.

int jhcThresh_0::AlphaGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, 
                               int rdef, int gdef, int bdef) const 
{
  if (!dest.Valid(3) || !dest.SameFormat(src) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::AlphaGateRGB");
  dest.CopyRoi(src);
  dest.MergeRoi(gate);

  // general ROI case 
  int x, y, m, mp1, v;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip();
  UC8 rsub = BOUND(rdef), gsub = BOUND(gdef), bsub = BOUND(bdef);
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();
  UC8 dvals[256][3];

  // compute all fractions of the default value 
  // will be given alpha so use complement of this in table
  for (m = 0; m <= 255; m++)
  {
    v = ((256 - m) * bsub) >> 8;
    dvals[m][0] = BOUND(v);
    v = ((256 - m) * gsub) >> 8;
    dvals[m][1] = BOUND(v);
    v = ((256 - m) * rsub) >> 8;
    dvals[m][2] = BOUND(v);
  }

  // process pixels using multiplication and the table
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      m = (int)(*g++);
      if (m == 0)
      {
        d[0] = bsub;
        d[1] = gsub;
        d[2] = rsub;
      }
      else if (m == 255)
      {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
      }
      else
      {
        mp1 = m + 1;
        d[0] = (UC8)(((mp1 * (int) s[0]) >> 8) + dvals[m][0]);
        d[1] = (UC8)(((mp1 * (int) s[1]) >> 8) + dvals[m][1]);
        d[2] = (UC8)(((mp1 * (int) s[2]) >> 8) + dvals[m][2]);
      }
      d += 3;
      s += 3;
    }
    d += rsk;
    s += rsk;
    g += gsk;
  }
  return 1;
}


//= Like basic version but for case where src and dest are monochrome.

int jhcThresh_0::AlphaGateBW (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int def) const 
{
  int x, y, m, v;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 sub = BOUND(def);
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();
  UC8 dval[256];

  // compute all fractions of the default value 
  // will be given alpha so use complement of this in table
  for (m = 0; m <= 255; m++)
  {
    v = ((256 - m) * sub) >> 8;
    dval[m] = BOUND(v);
  }

  // process pixels either using multiplication and the table
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      m = (int)(*g++);
      if (m == 255)
        *d++ = *s++;
      else if (m == 0)
      {
        *d++ = sub;
        s++;
      }
      else
        *d++ = (UC8)((((m + 1) * (int)(*s++)) >> 8) + dval[m]);
    }
    d += rsk;
    s += rsk;
    g += rsk;
  }
  return 1;
}


//= Mixes two images together with a pixel-by-pixel weighting.
// where awt = 255 then output is exactly imga at that pixel
// where awt = 0 then output is exactly immb at that pixel
// at other places d = (g + 1) * a + (255 - g) * b

int jhcThresh_0::Composite (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, const jhcImg& awt) const 
{
  if (!dest.SameFormat(imga) || !dest.SameFormat(imgb) || !dest.SameSize(awt, 1))
    return Fatal("Bad images to jhcThresh::Composite");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);
  dest.MergeRoi(awt);
  if (dest.Fields() == 1)
    return CompositeBW(dest, imga, imgb, awt);
  if (dest.Fields() == 3)
    return CompositeRGB(dest, imga, imgb, awt);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, i, fa, fb;
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields();
  int rsk = dest.RoiSkip(), gsk = awt.RoiSkip(dest);
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff; 
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  const UC8 *g = awt.PxlSrc() + awt.RoiOff(dest);

  // process pixels using multiplication 
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*g == 0)
        for (i = 0; i < nf; i++)
          d[i] = b[i];
      else if (*g == 255)
        for (i = 0; i < nf; i++)
          d[i] = a[i];
      else
      {
        fa = *g + 1;
        fb = 256 - fa;
        for (i = 0; i < nf; i++)
          d[i] = (UC8)((fa * a[i] + fb * b[i]) >> 8);
      }
      d += nf;
      a += nf;
      b += nf;
      g++;
    }
    d += rsk;
    a += rsk;
    b += rsk;
    g += gsk;
  }
  return 1;
}


//= Like basic version but for images with 3 fields.

int jhcThresh_0::CompositeRGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, const jhcImg& awt) const 
{
  if (!dest.Valid(3) || !dest.SameFormat(imga) || !dest.SameFormat(imgb) || !dest.SameSize(awt, 1))
    return Fatal("Bad images to jhcThresh::CompositeRGB");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);
  dest.MergeRoi(awt);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, fa, fb;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int rsk = dest.RoiSkip(), gsk = awt.RoiSkip(dest);
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff; 
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  const UC8 *g = awt.PxlSrc() + awt.RoiOff(dest);

  // process pixels using multiplication 
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*g == 0)
      {
        d[0] = b[0];
        d[1] = b[1];
        d[2] = b[2];
      }
      else if (*g == 255)
      {
        d[0] = a[0];
        d[1] = a[1];
        d[2] = a[2];
      }
      else
      {
        fa = *g + 1;
        fb = 256 - fa;
        d[0] = (UC8)((fa * a[0] + fb * b[0]) >> 8);
        d[1] = (UC8)((fa * a[1] + fb * b[1]) >> 8);
        d[2] = (UC8)((fa * a[2] + fb * b[2]) >> 8);
      }
      d += 3;
      a += 3;
      b += 3;
      g++;
    }
    d += rsk;
    a += rsk;
    b += rsk;
    g += gsk;
  }
  return 1;
}


//= Like basic version but for case where src and dest are monochrome.

int jhcThresh_0::CompositeBW (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, const jhcImg& awt) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(imga) || !dest.SameFormat(imgb) || !dest.SameSize(awt, 1))
    return Fatal("Bad images to jhcThresh::CompositeBW");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);
  dest.MergeRoi(awt);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, fa, fb;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff; 
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  const UC8 *g = awt.PxlSrc() + roff;

  // process pixels using multiplication 
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*g == 0)
        *d = *b;
      else if (*g == 255)
        *d = *a;
      else
      {
        fa = *g + 1;
        fb = 256 - fa;
        *d = (UC8)((fa * (*a) + fb * (*b)) >> 8);
      }
      d++;
      a++;
      b++;
      g++;
    }
    d += rsk;
    a += rsk;
    b += rsk;
    g += rsk;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Pixel Switching                            //
///////////////////////////////////////////////////////////////////////////

//= Copies src to dest but substitutes pixels from marks when marks is non-zero.

int jhcThresh_0::OverlayNZ (jhcImg& dest, const jhcImg& src, const jhcImg& marks) const 
{
  int nf = dest.Fields();

  if (!dest.Valid() || !dest.SameFormat(src) || !dest.SameFormat(marks))
    return Fatal("Bad images to jhcThresh::OverlayNZ");
  dest.CopyRoi(src);
  dest.MergeRoi(marks);
  if (nf == 1)
    return OverlayNZ_BW(dest, src, marks);
  if (nf == 3)
    return OverlayNZ_RGB(dest, src, marks);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, i, any;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff, *m = marks.PxlSrc() + roff;

  // process pixels 
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      // see if any field of pixel from marks array is non-zero
      any = 0;
      for (i = 0; i < nf; i++)
        if (m[i] > 0)
        {
          any = 1;
          break;
        }

      // copy all fields from either marks or source depending
      if (any != 0)
      {
        for (i = nf; i > 0; i--)
          *d++ = *m++;
        s += nf;
      }
      else
      {
        for (i = nf; i > 0; i--)
          *d++ = *s++;
        m += nf;
      }
    }
    d += rsk;
    s += rsk;
    m += rsk;
  }
  return 1;
}


//= Like OverlayNZ but image is know to be RGB.

int jhcThresh_0::OverlayNZ_RGB (jhcImg& dest, const jhcImg& src, const jhcImg& marks) const 
{
  int x, y;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff, *m = marks.PxlSrc() + roff;

  // take RGB from marks if R, G, or B non-zero, else copy from src
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if ((m[0] > 0) || (m[1] > 0) || (m[2] > 0))
      {
        d[0] = m[0];
        d[1] = m[1];
        d[2] = m[2];
      }
      else
      {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
      }
      d += 3;
      s += 3;
      m += 3;
    }
    d += rsk;
    s += rsk;
    m += rsk;
  }
  return 1;
}


//= Like OverlayNZ but image is known to be monochrome.

int jhcThresh_0::OverlayNZ_BW (jhcImg& dest, const jhcImg& src, const jhcImg& marks) const 
{
  int x, y;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff, *m = marks.PxlSrc() + roff;

  // take pixel value from marks if non-zero, else from src
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*m > 0)
      {
        *d++ = *m++;
        s++;
      }
      else
      {
        *d++ = *s++;
        m++;
      }
    }
    d += rsk;
    s += rsk;
    m += rsk;
  }
  return 1;
}


//= Copy values when indicator is over threshold, else keep original.

int jhcThresh_0::SubstOver (jhcImg& dest, const jhcImg& alt, const jhcImg& gate, int th) const 
{
  if (!dest.SameFormat(alt) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::SubstOver");
  dest.CopyRoi(alt);
  dest.MergeRoi(gate); 

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, i;
  UC8 v = BOUND(th);
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip(dest);
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = alt.PxlSrc() + alt.RoiOff(dest);
  const UC8 *g = gate.PxlSrc() + gate.RoiOff(dest);

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*g++ > v)
      {
        for (i = nf; i > 0; i--)
          *d++ = *a++;
      }
      else
      {
        a += nf;
        d += nf;
      }
    d += rsk;
    a += rsk;
    g += gsk;
  }
  return 1;
}


//= Copy values when indicator is under threshold, else keep original.

int jhcThresh_0::SubstUnder (jhcImg& dest, const jhcImg& alt, const jhcImg& gate, int th) const 
{
  if (!dest.SameFormat(alt) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::SubstUnder");
  dest.CopyRoi(alt);
  dest.MergeRoi(gate);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, i;
  UC8 v = BOUND(th);
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = alt.PxlSrc() + roff;
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*g++ < v)
        for (i = nf; i > 0; i--)
          *d++ = *a++;
      else
      {
        a += nf;
        d += nf;
      }
    d += rsk;
    a += rsk;
    g += gsk;
  }
  return 1;
}


//= Copy values when indicator is exactly as specified, else keep original.

int jhcThresh_0::SubstKey (jhcImg& dest, const jhcImg& alt, const jhcImg& gate, int val) const 
{
  if (!dest.SameFormat(alt) || !dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcThresh::SubstKey");
  dest.CopyRoi(alt);
  dest.MergeRoi(gate);

  // general ROI case for an arbitrary number of fields (seldom used)
  int x, y, i;
  UC8 v = BOUND(val);
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields();
  int rsk = dest.RoiSkip(), gsk = gate.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = alt.PxlSrc() + roff;
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*g++ == v)
        for (i = nf; i > 0; i--)
          *d++ = *a++;
      else
      {
        a += nf;
        d += nf;
      }
    d += rsk;
    a += rsk;
    g += gsk;
  }
  return 1;
}


//= Set dest to mark where pixel of val is between lo and hi (inclusive).  
// more like a drawing function, updates destination's ROI to include any pixels set

int jhcThresh_0::MarkTween (jhcImg& dest, const jhcImg& val, int lo, int hi, int mark) const
{
  int x0 = val.RoiX(), y0 = val.RoiY(), x2 = val.RoiX2(), y2 = val.RoiY2(), sk = val.RoiSkip();
  int x, y, nx, xlo = dest.XLim(), xhi = 0, ylo = dest.YLim(), yhi = 0, ny = 0;
  const UC8 *v = val.RoiSrc();
  UC8 *d = dest.RoiDest(val);

  if (!dest.Valid(1) || !dest.SameFormat(val))
    return Fatal("Bad images to jhcThresh::MarkTween");
  for (y = y0; y < y2; y++, d += sk, v += sk)
  {
    for (nx = 0, x = x0; x < x2; x++, d++, v++)
      if ((*v >= lo) && (*v <= hi))
      {
        *d = (UC8) mark;
        adj_lims(x, xlo, xhi, nx);
      }
    if (nx > 0)
      adj_lims(y, ylo, yhi, ny);
  }
  if (ny > 0)
    dest.AbsorbRoi(xlo, xhi, ylo, yhi);
  return 1;
}


//= Adjust x or y range limit during normal image scan.

void jhcThresh::adj_lims (int v, int& lo, int& hi, int& n) const
{
  if (n <= 0)
    lo = __min(lo, v);
  else
    hi = __max(hi, v);
  n = 1;
} 


///////////////////////////////////////////////////////////////////////////
//                             Area Restriction                          //
///////////////////////////////////////////////////////////////////////////

//= Set ROI limits to enclose all non-zero portions of image.
// final region fits within ROI set for source image

int jhcThresh_0::RoiNZ (jhcRoi& region, const jhcImg& src) const 
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcThresh::RoiNZ");

  // general case: examine all pixels in ROI of source image
  int x, y, nw, nh, x0, x1, y0, y1, anx = 0, any = 0;
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.PxlSrc() + src.RoiOff();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*s++ > 0)
      {
        if (any == 0)
        {
          // first pixel always has minimum y
          any = 1;
          anx = 1;
          x0 = x;
          x1 = x;
          y0 = y;
          y1 = y;
        }
        else
        { 
          // new pixel always has y greater than or equal to old
          y1 = y;
          if (anx == 0)
          {
            // first pixel in row is only candidate for minimum
            anx = 1;
            x0 = __max(x0, x);
          }
          x1 = __min(x1, x);
        }
      }
    anx = 0;
    s += rsk;
  }

  // convert loop indices into dimensions and coordinates for new ROI
  if (any == 0)
    region.SetRoi(0, 0, 0, 0);
  else
  {
    nw = x0 - x1 + 1;
    nh = y0 - y1 + 1;
    x0 = src.RoiX() + (rw - x0);
    y0 = src.RoiY() + (rh - y0);
    region.SetRoi(x0, y0, nw, nh);
  }
  return 1;
}


//= Like RoiNZ, but set ROI to parts of image at or above some threshold.
// final region fits within ROI set for source image

int jhcThresh_0::RoiThresh (jhcRoi& region, const jhcImg& src, int th) const 
{
  if (!src.Valid(1))
    return Fatal("Bad images to jhcThresh::RoiThresh");
  if (th == 0)
    return RoiNZ(region, src);

  // general case: examine all pixels in ROI of source image
  int x, y, nw, nh, x0, x1, y0, y1, anx = 0, any = 0;
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.PxlSrc() + src.RoiOff();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*s++ >= th)
      {
        if (any == 0)
        {
          // first pixel always has minimum y
          any = 1;
          anx = 1;
          x0 = x;
          x1 = x;
          y0 = y;
          y1 = y;
        }
        else
        { 
          // new pixel always has y greater than or equal to old
          y1 = y;
          if (anx == 0)
          {
            // first pixel in row is only candidate for minimum
            anx = 1;
            x0 = __max(x0, x);
          }
          x1 = __min(x1, x);
        }
      }
    anx = 0;
    s += rsk;
  }

  // convert loop indices into dimensions and coordinates for new ROI
  if (any > 0)
  {
    nw = x0 - x1 + 1;
    nh = y0 - y1 + 1;
    x0 = src.RoiX() + (rw - x0);
    y0 = src.RoiY() + (rh - y0);
    region.SetRoi(x0, y0, nw, nh);
  }
  return 1;
}

