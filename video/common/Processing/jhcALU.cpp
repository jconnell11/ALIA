// jhcALU.cpp : functions to add, subtract, multiply, etc. two images
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

#include "Processing/jhcALU.h"


// Most functions operate on multiple fields (8 bits each) in parallel

///////////////////////////////////////////////////////////////////////////
//                           Initialization                              //
///////////////////////////////////////////////////////////////////////////

// Done this way to make only one copy and keep it off the stack.
// Static member variables must be initialized at file scope.

int jhcALU_0::instances = 0;

UL32 *jhcALU_0::recip = NULL; 


//= Constructor sets up some standard tables.

jhcALU_0::jhcALU_0 ()
{
  int den; 

  // see if initialization already done by another instance
  if (instances++ > 0)
    return;
  recip = new UL32 [512];
  if (recip == NULL)
    Fatal("Could not allocate table in jhcALU");

  // normalization for sums 1 to 510 (RelDiff)
  recip[0] = 65536;
  for (den = 1; den < 512; den++)
    recip[den] = ROUND(65536.0 / den);
}


//= Cleans up allocated tables if last instance in existence.

jhcALU_0::~jhcALU_0 ()
{
  if (--instances > 0)
    return;

  if (recip != NULL)
    delete [] recip;
  instances = 0;
  recip = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                        Simple Differences                             //
///////////////////////////////////////////////////////////////////////////

//= Fills destination with the difference of two other images (all fields).
// scales difference by a small factor and clips result to between 0 and 255
// special 32 bit version is not noticeably faster

int jhcALU_0::ClipDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::ClipDiff");
  if (sc == 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, v, diff, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UC8 scaled[512];
  int sum = -255 * f + 128;

  // compute arbitrarily scaled result for all possible differences
  for (diff = -255; diff <= 255; diff++)
  {
    v = (int)(sum >> 8);
    scaled[diff + 255] = BOUND(v);
    sum += f;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = scaled[(int)(*a++) - (int)(*b++) + 255];
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Fills destination with the difference of two other images (all fields).
// scales difference by a small factor and adds to 128 (to allow negatives)

int jhcALU_0::LiftDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::LiftDiff");
  if (sc == 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, v, diff, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UC8 scaled[512];
  int sum = -255 * f + 32768;

  // compute arbitrarily scaled result for all possible differences
  for (diff = -255; diff <= 255; diff++)
  {
    v = (int)(sum >> 8);
    scaled[diff + 255] = BOUND(v);
    sum += f;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = scaled[(int)(*a++) - (int)(*b++) + 255];
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Subtracts fraction of imgb from imga and bounds result.
// scale of imga is always 1, unlike Blend where it is 1 - sc

int jhcALU_0::Decrement (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::Decrement");
  if (sc == 0.0)
    return dest.CopyArr(imga);
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  UC8 scaled[256];
  int x, y, pel, v, f = ROUND(256.0 * sc), sum = 128;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // compute arbitrarily scaled version of all inputs
  for (pel = 0; pel <= 255; pel++, sum += f)
    scaled[pel] = (UC8)(sum >> 8);

  // run lookup table over image
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
    {
      v = (int)(*a) - scaled[(int)(*b)];
      *d = BOUND(v);
    }
  return 1;
}


//= Fills destination with the difference of two other images (all fields).
// assumes values are CYCLIC, like angles or hues, so 0 = 256
// scales difference by a small factor and adds to 128 (to allow negatives)

int jhcALU_0::CycDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::CycDiff");
  if (sc == 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, v, diff, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UC8 scaled[256];
  int sum = -128 * f + 32768;

  // compute arbitrarily scaled result for all possible differences
  for (diff = -128; diff < 128; diff++)
  {
    v = (int)(sum >> 8);
    scaled[diff + 128] = BOUND(v);
    sum += f;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      diff = (int)(*a++) - (int)(*b++);
      if (diff >= 128)
        diff -= 256;
      else if (diff < -128)
        diff += 256;
      *d++ = scaled[diff + 128];
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Fills destination with the ABSOLUTE difference of two images (all fields).
// assumes values are CYCLIC, like angles or hues, so 0 = 256

int jhcALU_0::CycDist (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::CycDist");
  if (sc == 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, v, diff, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UC8 scaled[129];
  int sum = 128;

  // compute arbitrarily scaled result for all possible differences
  for (diff = 0; diff <= 128; diff++)
  {
    v = (int)(sum >> 8);
    scaled[diff] = BOUND(v);
    sum += f;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      diff = (int)(*a++) - (int)(*b++);
      if (diff >= 128)
        diff = 256 - diff;
      else if (diff < -128)
        diff += 256;
      else if (diff < 0)
        diff = -diff;
      *d++ = scaled[diff];
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Fills destination with the absolute difference of two other images (all fields).
// scales difference by a small factor and clips result to between 0 and 255

int jhcALU_0::AbsDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::AbsDiff");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, val, diff, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 v;
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UC8 scaled[512];
  int sum = 128;

  // compute arbitrarily scaled result for all possible differences
  for (diff = 0; diff <= 255; diff++)
  {
    val = (int)(sum >> 8);
    v = BOUND(val);
    scaled[255 - diff] = v; 
    scaled[255 + diff] = v;
    sum += f;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = scaled[(int)(*a++) - (int)(*b++) + 255];
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Fills destination thresholded absolute difference of two other images.
// a negative value marks all pixels where difference is below threshold

int jhcALU_0::AbsThresh (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, int th) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::AbsThresh");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, val;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 bth = BOUND(th), over = 255, under = 0;
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);

  // setup sign of threshold
  if (th < 0)
  {
    bth = BOUND(-th);
    over = 0;
    under = 255;
  }

  // run function on ROI part of image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      val = abs(*a++ - *b++);
      if (val > bth)
        *d++ = over;
      else
        *d++ = under;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Like AbsDiff but allows independent scaling of red, green, and blue channels.

int jhcALU_0::AbsDiffRGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, 
                          double rsc, double gsc, double bsc) const
{
  if (!dest.Valid(3) || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::AbsDiffRGB");
  if ((rsc < 0.0) || (gsc < 0.0) || (bsc < 0.0))
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, val, diff;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 v;
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 rscaled[512], gscaled[512], bscaled[512];
  double rsum = 128.0, gsum = 128.0, bsum = 128.0;
  double rf = rsc * 256.0, gf = gsc * 256.0, bf = bsc * 256.0;

  // compute arbitrarily scaled result for all possible differences
  for (diff = 0; diff <= 255; diff++)
  {
    val = ((int) rsum) >> 8;
    v = BOUND(val);
    rscaled[255 - diff] = v; 
    rscaled[255 + diff] = v;
    rsum += rf;
    val = ((int) gsum) >> 8;
    v = BOUND(val);
    gscaled[255 - diff] = v; 
    gscaled[255 + diff] = v;
    gsum += gf;
    val = ((int) bsum) >> 8;
    v = BOUND(val);
    bscaled[255 - diff] = v; 
    bscaled[255 + diff] = v;
    bsum += bf;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      d[0] = bscaled[(int)(a[0]) - (int)(b[0]) + 255];
      d[1] = gscaled[(int)(a[1]) - (int)(b[1]) + 255];
      d[2] = rscaled[(int)(a[2]) - (int)(b[2]) + 255];
      d += 3;
      a += 3;  
      b += 3;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Takes field-wise absolute differences, scales them, then adds them together.
//  D = rsc * abs(Ra - Rb) + gsc * abs(Ga - Gb) + bsc * abs(Ba - Bb)

int jhcALU_0::WtdSAD_RGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, 
                          double rsc, double gsc, double bsc) const
{  
  if (!imga.Valid(3) || !imga.SameFormat(imgb) || !imga.SameSize(dest, 1))
    return Fatal("Bad images to jhcALU::WtdSAD_RGB");
  if ((rsc < 0.0) || (gsc < 0.0) || (bsc < 0.0))
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int rscaled[512], gscaled[512], bscaled[512];
  double rsum = 128.0, gsum = 128.0, bsum = 128.0;
  double rf = rsc * 256.0, gf = gsc * 256.0, bf = bsc * 256.0;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), rsk = imga.RoiSkip(rw);
  int x, y, v, diff;
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);

  // compute arbitrarily scaled result for all possible differences
  for (diff = 0; diff <= 255; diff++, rsum += rf, gsum += gf, bsum += bf)
  {
    v = ROUND(rsum);
    rscaled[255 - diff] = v; 
    rscaled[255 + diff] = v;
    v = ROUND(gsum);
    gscaled[255 - diff] = v; 
    gscaled[255 + diff] = v;
    v = ROUND(bsum);
    bscaled[255 - diff] = v; 
    bscaled[255 + diff] = v;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--, d += dsk, a += rsk, b += rsk)
    for (x = rw; x > 0; x--, d++, a += 3, b += 3)
    {
      v =  bscaled[(int)(a[0]) - (int)(b[0]) + 255];
      v += gscaled[(int)(a[1]) - (int)(b[1]) + 255];
      v += rscaled[(int)(a[2]) - (int)(b[2]) + 255];
      v = (v >> 8);
      *d = (UC8) __min(v, 255);
    }
  return 1;
}


//= Takes field-wise absolute differences, scales them, then adds them together.
//  D = rsc * (Ra - Rb)^2  +  gsc * (Ga - Gb)^2  +  bsc * (Ba - Bb)^2
// Note: changed 10/13 so that scale factors are NO LONGER squared

int jhcALU_0::WtdSSD_RGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, 
                          double rsc, double gsc, double bsc) const
{
  if (!imga.Valid(3) || !imga.SameFormat(imgb) || !imga.SameSize(dest, 1))
    return Fatal("Bad images to jhcALU::WtdSSD_RGB");
  if ((rsc < 0.0) || (gsc < 0.0) || (bsc < 0.0))
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int rscaled[512], gscaled[512], bscaled[512];
  double rf = 256.0 * rsc, gf = 256.0 * gsc, bf = 256.0 * bsc;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), rsk = imga.RoiSkip(rw);
  int x, y, v, vt, diff;
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);

  // compute arbitrarily scaled result for all possible differences
  for (diff = 0; diff <= 255; diff++)
  {
    v = diff * diff;
    vt = ROUND(rf * v);
    rscaled[255 - diff] = vt;
    rscaled[255 + diff] = vt;
    vt = ROUND(gf * v);
    gscaled[255 - diff] = vt;
    gscaled[255 + diff] = vt;
    vt = ROUND(bf * v);
    bscaled[255 - diff] = vt;
    bscaled[255 + diff] = vt;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--, d += dsk, a += rsk, b += rsk)
    for (x = rw; x > 0; x--, d++, a += 3, b += 3)
    {
      v =  bscaled[(int)(a[0]) - (int)(b[0]) + 255];
      v += gscaled[(int)(a[1]) - (int)(b[1]) + 255];
      v += rscaled[(int)(a[2]) - (int)(b[2]) + 255];
      v >>= 8;
      *d = (UC8) __min(v, 255);
    }
  return 1;
}


//= Takes field-wise differences, squares and scales them, then gets square root of sum.
//  D = sqrt[ rsc * (Ra - Rb)^2  +  gsc * (Ga - Gb)^2  +  bsc * (Ba - Bb)^2 ]
// normalizes squaring so 255 in one field yields 255 in result (with sc = 1)

int jhcALU_0::WtdRMS_RGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, 
                          double rsc, double gsc, double bsc) const
{
  if (!imga.Valid(3) || !imga.SameFormat(imgb) || !imga.SameSize(dest, 1))
    return Fatal("Bad images to jhcALU::WtdRMS_RGB");
  if ((rsc < 0.0) || (gsc < 0.0) || (bsc < 0.0))
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int rscaled[512], gscaled[512], bscaled[512];
  double rf = 256.0 * rsc, gf = 256.0 * gsc, bf = 256.0 * bsc;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), rsk = imga.RoiSkip(rw);
  int x, y, v, vt, diff;
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);

  // compute arbitrarily scaled result for all possible differences
  for (diff = 0; diff <= 255; diff++)
  {
    v = diff * diff;
    vt = ROUND(rf * v);
    rscaled[255 - diff] = vt;
    rscaled[255 + diff] = vt;
    vt = ROUND(gf * v);
    gscaled[255 - diff] = vt;
    gscaled[255 + diff] = vt;
    vt = ROUND(bf * v);
    bscaled[255 - diff] = vt;
    bscaled[255 + diff] = vt;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--, d += dsk, a += rsk, b += rsk)
    for (x = rw; x > 0; x--, d++, a += 3, b += 3)
    {
      v =  bscaled[(int)(a[0]) - (int)(b[0]) + 255];
      v += gscaled[(int)(a[1]) - (int)(b[1]) + 255];
      v += rscaled[(int)(a[2]) - (int)(b[2]) + 255];
      v = ROUND(sqrt((double)(v >> 8)));
      *d = (UC8) __min(v, 255);
    }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Relative Differences                           //
///////////////////////////////////////////////////////////////////////////

//= Computes pixel-wise absolute difference normalized by smaller image.
//   val = 255 * |a - b| / min(a, b) where 255 corresponds to 1.0
// for instance a = 150 and a = 67 with b = 100 give same result
// can also scale difference by a small factor and clip result properly
// good for measuring shifts in albedo and judging shadows

int jhcALU_0::RelBoost (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::RelBoost");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, diff, v, num = ROUND(255.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UL32 den[256];
  int sm;

  // compute arbitrarily scaled denomiator for all possible sums
  for (sm = 0; sm <= 255; sm++)
    den[sm] = num * recip[sm];

  // run lookup table over images (rounding omitted)
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      sm = __max(1, __min(*a, *b));
      diff = (int)(*a++) - (int)(*b++);
      if (diff < 0)
        diff = -diff;
      v = (diff * den[sm]) >> 16;
      if (v >= 255)
        *d++ = 255;
      else
        *d++ = (UC8) v;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Computes pixel-wise absolute difference normalized by larger image.
//   val = 255 * |a - b| / max(a, b) where 255 corresponds to 1.0
// can also scale difference by a small factor and clip result properly

int jhcALU_0::RelDrop (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::RelDrop");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int i, x, y, diff, v, num = ROUND(255.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UL32 den[256];
  UC8 big;

  // compute arbitrarily scaled denomiator for all possible sums
  for (i = 0; i <= 255; i++)
    den[i] = num * recip[i];

  // run lookup table over images (rounding omitted)
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      big = __max(1, __max(*a, *b));
      diff = (int)(*a++) - (int)(*b++);
      if (diff < 0)
        diff = -diff;
      v = (diff * den[big]) >> 16;
      if (v >= 255)
        *d++ = 255;
      else
        *d++ = (UC8) v;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Computes pixel-wise absolute difference normalized by pixel average.
//   val = 255 * |a - b| / (a + b) where 255 corresponds to 1.0
// can also scale difference by a small factor and clip result properly

int jhcALU_0::RelDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::RelDiff");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, diff, sum, v, num = ROUND(255.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UL32 den[512];

  // compute arbitrarily scaled denomiator for all possible sums
  for (sum = 0; sum <= 510; sum++)
    den[sum] = num * recip[sum];

  // run lookup table over images (rounding omitted)
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      diff = (int)(*a) - (int)(*b);
      if (diff < 0)
        diff = -diff;
      sum  = *a++ + *b++;
      v = (diff * den[sum]) >> 16;
      if (v >= 255)
        *d++ = 255;
      else
        *d++ = (UC8) v;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Computes brightening normalized by second image.
//   val = 255 * (a - b) / b where 255 corresponds to 1.0
// can also scale difference by a small factor and clip result properly

int jhcALU_0::FracBoost (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::FracBoost");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, diff, v, num = ROUND(255.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UL32 den[256];

  // compute arbitrarily scaled denomiator for all possible sums
  for (v = 0; v <= 255; v++)
    den[v] = num * recip[v];

  // run lookup table over images (rounding omitted)
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*a <= *b)
      {
        *d++ = 0;
        a++;
        b++;
      }
      else
      {
        diff = (int)(*a++) - (int)(*b);
        v = (diff * den[*b++]) >> 16;
        if (v >= 255)
          *d++ = 255;
        else
          *d++ = (UC8) v;
      }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Computes dimming normalized by second image.
//   val = 255 * (b - a) / b where 255 corresponds to 1.0
// can also scale difference by a small factor and clip result properly

int jhcALU_0::FracDrop (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::FracDrop");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, diff, v, num = ROUND(255.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UL32 den[256];

  // compute arbitrarily scaled denomiator for all possible sums
  for (v = 0; v <= 255; v++)
    den[v] = num * recip[v];

  // run lookup table over images (rounding omitted)
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*b <= *a)
      {
        *d++ = 0;
        a++;
        b++;
      }
      else
      {
        diff = (int)(*b) - (int)(*a++);
        v = (diff * den[*b++]) >> 16;
        if (v >= 255)
          *d++ = 255;
        else
          *d++ = (UC8) v;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Computes pixel-wise difference normalized by second image.
//   val = 255 * |a - b| / b where 255 corresponds to 1.0
// can also scale difference by a small factor and clip result properly

int jhcALU_0::FracDiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::FracDiff");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, diff, v, num = ROUND(255.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UL32 den[256];

  // compute arbitrarily scaled denomiator for all possible sums
  for (v = 0; v <= 255; v++)
    den[v] = num * recip[v];

  // run lookup table over images (rounding omitted)
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*b <= *a)
      {
        *d++ = 0;
        a++;
        b++;
      }
      else
      {
        diff = (int)(*a++) - (int)(*b);
        if (diff < 0)
          diff = -diff;
        v = (diff * den[*b++]) >> 16;
        if (v >= 255)
          *d++ = 255;
        else
          *d++ = (UC8) v;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Statistical Comparisions                        //
///////////////////////////////////////////////////////////////////////////

//= Combine two images to give pairwise maximum at every pixel.
// one possible implementation of a logical "or" function

int jhcALU_0::MaxFcn (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::MaxFcn");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // compare pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
      *d = __max(*a, *b);
  return 1;
}


//= Combine two images to give pairwise minimum at every pixel.
// one possible implementation of a logical "and" function

int jhcALU_0::MinFcn (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::MinFcn");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // compare pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
      *d = __min(*a, *b);
  return 1;
}


//= One possible implementation of a logical "ornot" function (was MinMax).
// essentially takes the complement of the second image and 
// combines with first image to give pairwise maximum at every pixel

int jhcALU_0::MaxComp2 (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::MaxComp2");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, bc, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // compare pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
    {
      bc = 255 - *b;
      *d = (UC8) __max(*a, bc);
    }
  return 1;
}


//= One possible implementation of a logical "andnot" function (was MinMax).
// essentially takes the complement of the second image and 
// combines with first image to give pairwise minimum at every pixel

int jhcALU_0::MinComp2 (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::MinComp2");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, bc, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // compare pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
    {
      bc = 255 - *b;
      *d = (UC8) __min(*a, bc);
    }
  return 1;
}


//= Combine two images to give pairwise minimum at every pixel.
// only considers pixels which have values greater than 0

int jhcALU_0::NZMin (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::NZMin");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // compare pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
      if ((*b == 0) || ((*a != 0) && (*a < *b)))
        *d = *a;
      else 
        *d = *b;
  return 1;
}


//= Combine two images to give pairwise average at every pixel.
// only considers pixels which have values greater than 0

int jhcALU_0::NZAvg (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::NZAvg");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // compare pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
      if (*b == 0)
        *d = *a;
      else if (*a == 0)
        *d = *b;
      else
        *d = (UC8)(((int)(*a) + (int)(*b)) >> 1); 
  return 1;
}


//= Bitwise XOR pixels from two images.
// sometimes useful for combining drawing with image

int jhcALU_0::XorFcn (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::XorFcn");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // combine pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
      *d = (*a ^ *b) & 0xFF;
  return 1;
}


//= Take max of images, but only where dest is already greater than threshold.
// useful for combining overhead depth map with smoothed version

int jhcALU_0::MaxWithin (jhcImg& dest, const jhcImg& src, int th) const
{
  if (!dest.Valid() || !dest.SameFormat(src))
    return Fatal("Bad images to jhcALU::MaxWithin");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // combine pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      if (*d > th)
        *d = __max(*d, *s);
  return 1;
}


//= Set to 255 if both over threshold, 128 if just one over, otherwise zero.

int jhcALU_0::NumOver (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, int ath, int bth)
{
  if (!dest.Valid(1) || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::NumOver");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // combine pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rw; x > 0; x--, d++, a++, b++)
      if ((*a > ath) && (*b > bth))
        *d = 255;
      else if ((*a > ath) || (*b > bth))
        *d = 128;
      else
        *d = 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Mixing Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Adds pixels in two images together and clips maximum value to 255.
// can also multiply by a small scale factor if desired
// one possible implementation of a logical "or" function

int jhcALU_0::ClipSum (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::ClipSum");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, i, v, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UC8 scaled[512];
  UL32 sum = 128;

  // compute arbitrarily scaled result for all possible differences
  for (i = 0; i < 512; i++)
  {
    v = (int)(sum >> 8);
    scaled[i] = BOUND(v);
    sum += f;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = scaled[(int)(*a++) + (int)(*b++)];
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Combine two images to give pairwise average at every pixel.

int jhcALU_0::AvgFcn (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::AvgFcn");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;

  // compare pixels in ROI
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = (UC8)((((int) *a++) + ((int) *b++)) >> 1);
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Combine two images to where each pixel is a weighted sum of the two sources.
// bfrac = 1 - afrac so sums still limited 0 to 255

int jhcALU_0::Blend (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double afrac, double sc) const
                   
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::Blend");
  if ((afrac < 0.0) || (sc <= 0.0))
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // special cases (could use ClipScale too)
  if (sc == 1.0)
  {
    if (afrac == 1.0) 
      return dest.CopyArr(imga);
    if (afrac == 0.0) 
      return dest.CopyArr(imgb);
    if (afrac == 0.5) 
      return AvgFcn(dest, imga, imgb);
  }

  // general ROI case
  int i, x, y, v, asum = 0, bsum = 0;
  int ainc = ROUND(256.0 * sc * afrac), binc = ROUND(256.0 * sc * (1.0 - afrac));
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  int asc[256], bsc[256];

  // precompute component scaling tables
  for (i = 0; i <= 255; i++)
  {
    asc[i] = asum;
    bsc[i] = bsum;
    asum += ainc;
    bsum += binc;
  }

  // apply lookup table
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      v = (asc[*a++] + bsc[*b++] + 128) >> 8;
      *d++ = BOUND(v);
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Alter the src image to be more like the goal image.
// limits pixel changes to +/- lim values

int jhcALU_0::StepToward (jhcImg& dest, const jhcImg& goal, const jhcImg& src, int lim) const
{
  if (!dest.Valid() || !dest.SameFormat(src) || !dest.SameFormat(goal))
    return Fatal("Bad images to jhcALU::StepToward");
  dest.CopyRoi(src);
  dest.MergeRoi(goal);

  // general ROI case
  int x, y, diff;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff, *g = goal.PxlSrc() + roff;

  // add in saturated difference
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      diff = *g++ - *s;
      diff = __max(-lim, __min(diff, lim));
      *d++ = (UC8)(*s++ + diff);
    }
    d += rsk;
    s += rsk;
    g += rsk;
  }
  return 1;
}


//= Alter the src image to be more like the goal image.
// alters by some fraction of difference with minimum step enforced

int jhcALU_0::MixToward (jhcImg& dest, const jhcImg& goal, const jhcImg& src, double f, int always) const
{
  if (!dest.Valid() || !dest.SameFormat(src) || !dest.SameFormat(goal))
    return Fatal("Bad images to jhcALU::MixToward");
  dest.CopyRoi(src);
  dest.MergeRoi(goal);

  // general ROI case
  UC8 dsc[256];
  int i, diff, x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(), *g = goal.RoiSrc();
  UC8 *d = dest.RoiDest();

  // precompute component scaling table
  dsc[0] = 0;
  for (i = 1; i <= 255; i++)
  {
    diff = ROUND(f * i);
    dsc[i] = (UC8) __min(i, __max(always, diff));
  }

  // add in fractional difference
  for (y = rh; y > 0; y--, d += rsk, g += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, g++, s++)
      if (*g >= *s)
        *d = *s + dsc[*g - *s];
      else
        *d = *s - dsc[*s - *g];
  return 1;
}


//= Like MixToward but only perfrom update where agte image is below threshold.

int jhcALU_0::MixUnder (jhcImg& dest, const jhcImg& goal, const jhcImg& src, 
                        const jhcImg& mask, int th, double f, int always) const
{
  if (!dest.Valid() || !dest.SameFormat(src) || 
      !dest.SameFormat(goal) || !dest.SameSize(mask, 1))
    return Fatal("Bad images to jhcALU::MixUnder");
  dest.CopyRoi(src);
  dest.MergeRoi(goal);
  dest.MergeRoi(mask);

  // general ROI case
  int x, y, i, diff;
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields(); 
  int rsk = dest.RoiSkip(), msk = mask.RoiSkip(rw);
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest);
  const UC8 *g = goal.RoiSrc(dest), *m = mask.RoiSrc(dest);
  UC8 dsc[256];

  // precompute component scaling table
  dsc[0] = 0;
  for (i = 1; i <= 255; i++)
  {
    diff = ROUND(f * i);
    dsc[i] = (UC8) __max(always, diff);
  }

  // add in fractional difference
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*m++ >= th)
      {
        for (i = nf; i > 0; i--)
          *d++ = *s++;
        g += nf;
      }
      else
        for (i = nf; i > 0; i--)
        {
          diff = *g++ - *s;
          if (diff >= 0)
            diff = *s++ + dsc[diff];
          else
            diff = *s++ - dsc[-diff];
          *d++ = BOUND(diff);
        }
    d += rsk;
    s += rsk;
    g += rsk;
    m += msk;
  }
  return 1;
}


//= Take square root of sum of squares of values (relative to zero).
// for a more efficient implementation on signed values see jhcEdge::RMS

int jhcALU_0::Magnitude (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::Magnitude");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, v, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(), *b = imgb.RoiSrc();

  // process pixels in image
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
    {
      v = ROUND(sc * sqrt((double)((*a) * (*a) + (*b) * (*b))));
      *d = BOUND(v);
    }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Signal Processing                             //
///////////////////////////////////////////////////////////////////////////

//= Divide all pixels in one image by those in the other.
// with sc = 1.0, a result of 256 is a ratio of one
// works with both monochrome and color images for "imga"

int jhcALU_0::NormBy (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameSize(imgb))
    return Fatal("Bad images to jhcALU::NormBy");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  UC8 unity = BOUND(ROUND(sc * 255.0));
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = dest.Fields();
  int i, x, y, f, v, dsk = dest.RoiSkip(), ssk = imgb.RoiSkip(dest);
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();
  int r[256];
  double sc16 = 65536.0 * sc;

  // build scaling array 
  r[0] = 65536;
  for (i = 1; i <= 255; i++)
    r[i] = ROUND(sc16 / i);

  // compare pixels in ROI
  for (y = rh; y > 0; y--, d += dsk, a += dsk, b += ssk)
    for (x = rw; x > 0; x--, b++)
      for (f = nf; f > 0; f--, d++, a++)
        if (*b > 0)
        {
          v = ((*a * r[*b]) + 128) >> 8;  
          *d = BOUND(v);
        }
        else if (*a > 0)
          *d = 255;
        else
          *d = unity;
  return 1;
}


//= Like NormBy but treats 128 as zero and values as signed.
// variably stretches contrast around center
// can limit denominator to some small value to prevent zero divides

int jhcALU_0::CenterNorm (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc, int dmin) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::CenterNorm");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int i, x, y, v;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  int r[256];
  double sc16 = 65536.0 * sc;

  // build scaling array 
  for (i = 0; i <= 255; i++)
    r[i] = ROUND(sc16 / __max(i, dmin));

  // compare pixels in ROI
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      i = __max(*b, 1);
      if (*a >= 128)
        v = 128 + ((((*a - 128) * r[i]) + 128) >> 8);  
      else
        v = 128 - ((((128 - *a) * r[i]) + 128) >> 8);  
      *d++ = BOUND(v);
      a++;
      b++;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Computes percentage change between two images.
//   v = 255 * min(a, b) / max(a, b) where 255 corresponds to 1.0
// can also scale difference by a small factor and clip result properly

int jhcALU_0::AbsRatio (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const
{
  if (!dest.Valid() || !dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::AbsRatio");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, v, num = ROUND(255.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  UL32 den[256];

  // compute arbitrarily scaled denomiator for all possible sums
  for (v = 0; v <= 255; v++)
    den[v] = num * recip[v];

  // run lookup table over images (rounding omitted)
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      // very inefficient version!
      if (*a < *b)
        v = ROUND(255.0 * sc * *a / *b);
      else if (*b < *a)
        v = ROUND(255.0 * sc * *b / *a);
      else
        v = ROUND(255.0 * sc);
      *d++ = (UC8) __min(v, 255);
      a++;
      b++;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Multiplies two images together.
// assumes 255 corresponds to a factor of one: dest = a * (b + 1) / 256
// one possible implementation of a logical "and" function

int jhcALU_0::Mult (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::Mult");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);

  // compare pixels in ROI
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = (UC8)(((int)(*a++) * (1 + (int)(*b++))) >> 8);
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Multiplies two images together.
// differs from Mult in that assumes 128 corresponds to a factor of one
//   i.e. dest = src * (fact / 128)

int jhcALU_0::MultMid (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const
{
  if (!dest.SameFormat(imga) || !dest.SameFormat(imgb))
    return Fatal("Bad images to jhcALU::MultMid");
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int val, x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);

  // compare pixels in ROI
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      val = ((int)(*a++) * (int)(*b++)) >> 7;
      *d++ = (UC8) __min(val, 255);
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Multiplies the pixels in an RGB image by the values in a monochrome image.
// function implemented is: dest = src * (fact / 128) result is saturated

int jhcALU_0::MultRGB (jhcImg& dest, const jhcImg& src, const jhcImg& fact) const
{
  if (!dest.SameFormat(src) || !dest.SameSize(fact, 1))
    return Fatal("Bad images to jhcALU::MultRGB");
  dest.CopyRoi(src);
  dest.MergeRoi(fact);

  // general ROI case
  int sc, val, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int rsk = dest.RoiSkip(), fsk = fact.RoiSkip(dest);
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest), *f = fact.RoiSrc(dest);

  // scale up or down pixels in ROI
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      sc = (int)(*f++);
      if (sc == 128)
      {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
      }
      else
      {
        val = (sc * (int)(s[0])) >> 7;
        d[0] = (UC8) __min(val, 255);
        val = (sc * (int)(s[1])) >> 7;
        d[1] = (UC8) __min(val, 255);
        val = (sc * (int)(s[2])) >> 7;
        d[2] = (UC8) __min(val, 255);
      }
      s += 3;
      d += 3;
    }
    s += rsk;
    d += rsk;
    f += fsk;
  }
  return 1;
}

