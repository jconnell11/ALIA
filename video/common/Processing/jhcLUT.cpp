// jhcLUT.cpp : replaces pixels by a function of original value
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

#include "Processing/jhcLUT.h"


///////////////////////////////////////////////////////////////////////////
//                           Initialization                              //
///////////////////////////////////////////////////////////////////////////

// Done this way to make only one copy and keep it off the stack.
// Static member variables must be initialized at file scope.

int jhcLUT_0::instances = 0;

UC8 *jhcLUT_0::lgt = NULL; 


//= Constructor sets up some standard tables.

jhcLUT_0::jhcLUT_0 ()
{
  int i, bot, mid = 80;
  double v, k, n;

  // see if initialization already done by another instance
  if (instances++ > 0)
    return;
  lgt = new UC8 [256];
  if (lgt == NULL)
    Fatal("Could not allocate table in jhcLUT");

  // compute special log values for all possible intensities
  k = 128.0 / log(255.0 / mid);
  n = k * log(255.0) - 255.0;
  bot = (int)(exp(n / k)) + 1;
  for (i = 0; i < __min(bot, 255); i++)
    lgt[i] = 0;
  for (i = bot; i <= 255; i++)
  {
    v = k * log((double) i) - n;
    if (v <= 0.0)
      lgt[i] = 0;
    else if (v >= 255.0)
      lgt[i] = 255;
    else
      lgt[i] = (UC8) ROUND(v);
  }
}


//= Cleans up allocated tables if last instance in existence.

jhcLUT_0::~jhcLUT_0 ()
{
  if (--instances > 0)
    return;

  if (lgt != NULL)
    delete [] lgt;
  instances = 0;
  lgt = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                          Multiplication Etc.                          //
///////////////////////////////////////////////////////////////////////////

//= Find largest value in image and scale all to make this be 255.

int jhcLUT_0::PumpUp (jhcImg& dest, const jhcImg& src) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::PumpUp");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, val, f;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  UC8 scaled[256];

  // find max val in ROI
  val = *s;
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      if (*s > val) 
        val = *s;
      s++;
    }
    s += rsk;
  }
  if (val <= 0)
    return 0;

  // figure out scaling for all possible values
  f = ROUND(256.0 * (255.0 / val));
  for (i = 0; i <= 255; i++)
  {
    val = (i * f + 128) >> 8;
    if (val >= 255)
      scaled[i] = 255;
    else
      scaled[i] = (UC8) val;
  }
    
  // apply LUT to ROI to rescale other values
  s = src.PxlSrc() + roff;
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = scaled[*s++];
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Multiply all values by sc but limit result to 255 maximum.

int jhcLUT_0::ClipScale (jhcImg& dest, const jhcImg& src, double sc) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::ClipScale");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, val, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  UC8 scaled[256];
  int sum = 128;

  // compute answers for all possible values
  for (i = 0; i <= 255; i++)
  {
    val = (int)(sum >> 8);
    if (val >= 255)
      scaled[i] = 255;
    else
      scaled[i] = (UC8) val;
    sum += f;
  }

  // apply lookup table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = scaled[*s++];
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Like ClipScale but for signed values where 128 is zero point.
// essentially stretches contrast around middle

int jhcLUT_0::CenterScale (jhcImg& dest, const jhcImg& src, double sc) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::CenterScale");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, val, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 scaled[256];
  int sum = 128;

  // compute answers for all possible values
  for (i = 0; i <= 127; i++)
  {
    val = 128 + (int)(sum >> 8);
    if (val >= 255)
      scaled[128 + i] = 255;
    else
      scaled[128 + i] = (UC8) val;
    val = 128 - (int)(sum >> 8);
    if (val <= 0)
      scaled[128 - i] = 0;
    else
      scaled[128 - i] = (UC8) val;
    sum += f;
  }
  val = 128 - (int)(sum >> 8);
  if (val <= 0)
    scaled[0] = 0;
  else
    scaled[0] = (UC8) val;

  // apply lookup table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = scaled[*s++];
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Multiply all pixels in each field by a scale factor for that field.
// full in-line version does not break into fields for processing (4.4x faster)

int jhcLUT_0::AdjustRGB (jhcImg& dest, const jhcImg& src, double rf, double gf, double bf) const 
{
  if (!dest.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::AdjustRGB");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, val;
  int rd = ROUND(256 * rf), gd = ROUND(256 * gf), bd = ROUND(256 * bf);
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 rscaled[256], gscaled[256], bscaled[256];
  int rsum = 128, gsum = 128, bsum = 128;

  // compute answers for all possible values
  for (i = 0; i <= 255; i++)
  {
    // entry for red table
    val = (int)(rsum >> 8);
    if (val >= 255)
      rscaled[i] = 255;
    else
      rscaled[i] = (UC8) val;
    rsum += rd;

    // entry for green table
    val = (int)(gsum >> 8);
    if (val >= 255)
      gscaled[i] = 255;
    else
      gscaled[i] = (UC8) val;
    gsum += gd;

    // entry for blue table
    val = (int)(bsum >> 8);
    if (val >= 255)
      bscaled[i] = 255;
    else
      bscaled[i] = (UC8) val;
    bsum += bd;
  }

  // apply lookup tables to image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      d[0] = bscaled[s[0]];
      d[1] = gscaled[s[1]];
      d[2] = rscaled[s[2]];
      d += 3;
      s += 3;
    }
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Change every pixel value to some other value based on a lookup table.

int jhcLUT_0::MapVals (jhcImg& dest, const jhcImg& src, const UC8 map[]) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::MapVals");
  dest.CopyRoi(src);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  
  // apply lookup table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = map[*s++];
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Multiply each pixel by "sc" then square it.
// has additional divisor of 255 so at sc=1 255->255

int jhcLUT_0::Square (jhcImg& dest, const jhcImg& src, double sc) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::Square");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 ans[256];
  double sc2 = sc * sc / 255.0;

  // compute answers for all possible values
  for (i = 0; i <= 255; i++)
    ans[i] = BOUND(ROUND(sc2 * i * i));

  // apply lookup table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = ans[*s++];
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Take absolute value relative to 128 zero point.

int jhcLUT_0::AbsVal (jhcImg& dest, const jhcImg& src) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::AbsVal");
  dest.CopyRoi(src);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // process image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = (UC8) abs(*s++ - 128);
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Take absolute value relative to some reference level and scale it.
// pixels with value "lvl" get 255, other get lesser values

int jhcLUT_0::MatchVal (jhcImg& dest, const jhcImg& src, int lvl, double sc) const
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::MatchVal");
  dest.CopyRoi(src);

  // general ROI case
  UC8 tab[256];
  int i, v, x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // build table
  for (i = 0; i < 256; i++)
  {
    v = 255 - abs(ROUND(sc * (i - lvl)));
    tab[i] = (UC8) __max(0, v);
  }

  // process image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      *d = tab[*s];
  return 1;
}


//= Takes a log transform of values (e.g. intensity).
// computes v2(v) = k * log(v) - n, where v is pixel
// constants adjusted so that v2(255) = 255 and v2(mid) = 128

int jhcLUT_0::Log (jhcImg& dest, const jhcImg& src, int mid) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::Log");
  if (mid == 80)
    return Logify(dest, src);
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, bot;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 ans[256];
  double v, k = 128.0 / log(255.0 / mid), n = k * log(255.0) - 255.0;

  // compute answers for all possible values
  bot = (int)(exp(n / k)) + 1;
  for (i = 0; i < __min(bot, 255); i++)
    ans[i] = 0;
  for (i = bot; i <= 255; i++)
  {
    v = k * log((double) i) - n;
    if (v <= 0.0)
      ans[i] = 0;
    else if (v >= 255.0)
      ans[i] = 255;
    else
      ans[i] = (UC8) ROUND(v);
  }

  // apply lookup table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = ans[*s++];
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Transforms image src using gamma function transfer curve.
// the curve is generated so that an input value of mid is 
// transformed to an output value of 128
// (from Norman Haas)

int jhcLUT_0::Gamma (jhcImg& dest, const jhcImg& src, int mid) const
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::Gamma");
  if (&dest != &src) 
	  dest.CopyRoi(src);

  // general ROI case
  UC8 ans[256];
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  double v, fi, step = 1.0 / 255.0, gamma = log(0.5) / log(mid / 255.0);
  int i, x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();

  // compute answers for all possible values
  fi = step;
  ans[0] = 0;
  for (i = 1; i <= 255; i++, fi += step)
  {
    v = (double) pow(fi, gamma);
    if (v <= 0.0)   
      ans[i] = 0;
    else if (v >= 1.0) 
      ans[i] = 255;
    else
      ans[i] = (UC8) ROUND(255.0 * v);
  }

  // apply lookup table to image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++)
      *d++ = ans[*s++];
  return 1;
}


//= Transforms image src sigmoid transfer function. 
// the basic sigmoid is generated according to the formula:
//   y = 1 / (1 + EXP(m * x))) ,
// where x ranges from -1 to 1 and y ranges from 0 to 1.
// 'm' = 4, which produces a curve whose slope is 1.0008
// at the midpoint (preserving contrast in the center of 
// the range, if the midpoint is at 128).
// For use here, both are rescaled to 0 - 255. X is 
// shifted so that it is centered on the input parameter
// 'mid'. (The transform of 'mid' will be 128.) Then, the
// scaling of x is compressed as necessary so the smaller
// half (depending if x is more or less than 128) fits
// in the range, and the tail of the sigmoid is extended
// in the larger half.  This results in a greater slope
// at the midpoint, tailing off to zero at both ends.
// (from Norman Haas)

int jhcLUT_0::Sigmoid (jhcImg& dest, const jhcImg& src, int mid) const
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::Sigmoid");
  if (&dest != &src) 
	  dest.CopyRoi(src);

  // general ROI case
  UC8 ans[256], samples[256] = 
    {   5,   5,   5,   5,   5,   5,   6,   6,   6,   6,   6,   6,   7,   7,   7,   7,
        8,   8,   8,   8,   8,   9,   9,   9,  10,  10,  10,  10,  11,  11,  11,  12,
       12,  13,  13,  13,  14,  14,  15,  15,  15,  16,  16,  17,  17,  18,  18,  19, 
       19,  20,  21,  21,  22,  22,  23,  24,  24,  25,  26,  27,  27,  28,  29,  30, 
       31,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  46,
       47,  48,  49,  50,  52,  53,  54,  56,  57,  58,  60,  61,  63,  64,  66,  67, 
       69,  70,  72,  74,  75,  77,  79,  80,  82,  84,  86,  87,  89,  91,  93,  95,
       97,  99, 100, 102, 104, 106, 108, 110, 112, 114, 116, 118, 120, 122, 124, 126,
      128, 130, 132, 134, 136, 138, 140, 142, 144, 146, 148, 150, 152, 154, 156, 157,
      159, 161, 163, 165, 167, 169, 170, 172, 174, 176, 177, 179, 181, 182, 184, 186,
      187, 189, 190, 192, 193, 195, 196, 198, 199, 200, 202, 203, 204, 206, 207, 208,
      209, 210, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225,
      225, 226, 227, 228, 229, 229, 230, 231, 232, 232, 233, 234, 234, 235, 235, 236,
      237, 237, 238, 238, 239, 239, 240, 240, 241, 241, 241, 242, 242, 243, 243, 243,
      244, 244, 245, 245, 245, 246, 246, 246, 246, 247, 247, 247, 248, 248, 248, 248,
      248, 249, 249, 249, 249, 250, 250, 250, 250, 250, 250, 251, 251, 251, 251, 251};
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  double sc, v;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int i, x, y, lo, hi, sub; 

  // determine step size and limits
  if (mid < 128)
    sc = 128.0 / mid;
  else
    sc = 128.0 / (256 - mid);
  lo = (int)(128.0 - (128.0 / sc));
  hi = (int)(128.5 + (128.0 / sc));

  // compute answers for all possible values
  for (i = 0; i < lo; i++)
	  ans[i] = 4;
  for (i = lo; i < hi; i++)
  {
    v = sc * (i - 128);
    if (v < 0.0)
      sub = (int)(v - 0.5);
    else
      sub = (int)(v + 0.5);
	  ans[i] = samples[128 + sub];
  }
  for (i = hi; i < 256; i++)
	  ans[i] = 252;

  // apply lookup table to image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      *d = ans[*s];
  return 1;
}


//= Transforms image so similar intensity ratios are similar differences.
// faster than general Log because table is precomputed

int jhcLUT_0::Logify (jhcImg& dest, const jhcImg& src) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::Logify");
  dest.CopyRoi(src);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // apply lookup table to image
  for (y = dest.RoiH(); y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      *d = lgt[*s];
  return 1;
}


//= Shifts 16 bit values to right and clips to a maximum number of bits.
// a negative shift (rt < 0) shifts to the left instead
// can also subtract off some constant before shifting

int jhcLUT_0::Shift16 (jhcImg& dest, const jhcImg& src, int rt, int bits, int off) const 
{
  if (!dest.Valid(2) || !dest.SameFormat(src) || (rt < -15))
    return Fatal("Bad images to jhcLUT::Shift16");
  dest.CopyRoi(src);
  if ((bits <= 0) || (rt >= 16))
    return dest.FillArr(0);

  // general ROI case
  int bmax = __min(bits, 16), top = (0x1 << bmax) - 1, lf = -rt;
  int x, y, v, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip() >> 1;
  const US16 *s = (const US16 *) src.RoiSrc();
  US16 *d = (US16 *) dest.RoiDest();

  if (rt > 0)
    for (y = rh; y > 0; y--, d += rsk, s += rsk)
      for (x = rw; x > 0; x--, d++, s++)
      {
        v = (*s - off) >> rt;
        *d = (US16) __max(0, __min(v, top));
      }
  else
    for (y = rh; y > 0; y--, d += rsk, s += rsk)
      for (x = rw; x > 0; x--, d++, s++)
      {
        v = (*s - off) << lf;
        *d = (US16) __max(0, __min(v, top));
      }
  return 1;
}


//= Keeps only bits set to one in bit mask.

int jhcLUT_0::BitMask (jhcImg& dest, const jhcImg& src, int bits) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || (bits < 0))
    return Fatal("Bad images to jhcLUT::Mask");
  dest.CopyRoi(src);

  if ((bits & 0xFF) == 0) 
    return dest.FillArr(0);
  if ((bits & 0xFF) == 0xFF)
    return dest.CopyArr(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rw; x > 0; x--, d++, s++)
      *d = *s & bits;
  return 1;
}


//= Performs linear remapping of each pixel so v = sc * (v0 - off).
// similar to jhcThresh::RampOver function

int jhcLUT_0::Linear (jhcImg& dest, const jhcImg& src, int off, double sc) const
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::Linear");
  dest.CopyRoi(src);

  // general ROI case
  UC8 tab[256];
  int v, v0, x, y, rcnt = dest.RoiCnt(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();

  // build lookup table
  for (v0 = 0; v0 < 256; v0++)
  {
    v = ROUND(sc * (v0 - off));
    tab[v0] = BOUND(v);
  }

  // apply lookup table to image
  for (y = dest.RoiH(); y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      *d = tab[*s];
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Simple Value Alteration                         //
///////////////////////////////////////////////////////////////////////////

//= Fill image with (255 - val) in every pixel position.
// can use this like a logical "not" function

int jhcLUT_0::Complement (jhcImg& dest, const jhcImg& src) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::Complement");
  dest.CopyRoi(src);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  
  // apply function to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = ~(*s++);
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Force all pixel values to be less than or equal to value given.

int jhcLUT_0::LimitMax (jhcImg& dest, const jhcImg& src, int val) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::LimitMax");
  dest.CopyRoi(src);

  // check degenerate cases
  if (val >= 255)
  {
    dest.CopyArr(src);
    return 1;
  }
  if (val <= 0)
  {
    dest.FillArr(0);
    return 1;
  }

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 bval = BOUND(val);
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  
  // apply function to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*s <= bval)
        *d++ = *s++;
      else
      {
        *d++ = bval;
        s++;
      }
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Constrain all pixels to be within specified limits (inclusive).

int jhcLUT_0::LimitRng (jhcImg& dest, const jhcImg& src, int lo, int hi) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::LimitRng");
  dest.CopyRoi(src);

  // check degenerate cases
  if ((hi >= 255) && (lo <= 0))
  {
    dest.CopyArr(src);
    return 1;
  }
  if (hi <= 0)
  {
    dest.FillArr(0);
    return 1;
  }
  if (lo >= 255)
  {
    dest.FillArr(255);
    return 1;
  }
  if (hi == lo)
  {
    dest.FillArr(hi);
    return 1;
  }

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 bhi = BOUND(hi), blo = BOUND(lo);
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest);
  
  // apply function to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      *d++ = __max(blo, __min(*s, bhi));
      s++;
    }
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Color version of LimitMax with 3 separate limits.

int jhcLUT_0::LimitRGB (jhcImg& dest, const jhcImg& src, int rval, int gval, int bval) const 
{
  if (!src.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::LimitRGB");

  // check degenerate cases
  if ((rval <= 0) && (gval <= 0) && (bval <= 0))
  {
    dest.CopyRoi(src);
    dest.FillArr(0);
    return 1;
  }
  dest.CopyArr(src);
  if ((rval >= 255) && (gval >= 255) && (bval >= 255))
    return 1;

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 r = BOUND(rval), g = BOUND(gval), b = BOUND(bval);
  UC8 *d = dest.RoiDest();
  
  // apply limits to copied image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (d[0] > b)
        d[0] = b;
      if (d[1] > g)
        d[1] = g;
      if (d[2] > r)
        d[2] = r;
      d += 3;
    }
    d += rsk;
  }
  return 1;
}


//= Force all pixel values to be greater than or equal to value given.

int jhcLUT_0::LimitMin (jhcImg& dest, const jhcImg& src, int val) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::LimitMin");
  dest.CopyRoi(src);

  // check degenerate cases
  if (val >= 255)
  {
    dest.FillArr(255);
    return 1;
  }
  if (val <= 0)
  {
    dest.CopyArr(src);
    return 1;
  }

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 bval = BOUND(val);
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  
  // apply function to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*s >= bval)
        *d++ = *s++;
      else
      {
        *d++ = bval;
        s++;
      }
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Add or subtract an offset (with saturation) to all pixels.

int jhcLUT_0::Offset (jhcImg& dest, const jhcImg& src, int val) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::Offset");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, nval;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  
  // apply function to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      nval = *s++ + val;
      *d++ = BOUND(nval);
    }
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Like Offset but can specify separate offsets for each channel.

int jhcLUT_0::OffsetRGB (jhcImg& dest, const jhcImg& src, int rval, int gval, int bval) const 
{
  if (!dest.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::OffsetRGB");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, nval;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // apply function to image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      nval = s[0] + bval;
      d[0] = BOUND(nval);
      nval = s[1] + gval;
      d[1] = BOUND(nval);
      nval = s[2] + rval;
      d[2] = BOUND(nval);
      d += 3;
      s += 3;
    }
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Add a value to each pixel with wrap-around (cyclic).
// sometimes useful for edge directions

int jhcLUT_0::CycOffset (jhcImg& dest, const jhcImg& src, int val) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::CycOffset");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // apply function to image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
      *d = (*s + val) & 0xFF;
  return 1;
}


//= Bitwise AND each pixel value with binary mask.
// useful for converting 0-360 angles to 0-180

int jhcLUT_0::AndVal (jhcImg& dest, const jhcImg& src, int val) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::AndVal");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, cnt = dest.RoiCnt(), rh = dest.RoiH(), sk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();
  
  // apply function to image
  for (y = rh; y > 0; y--, d += sk, s += sk)
    for (x = cnt; x > 0; x--, d++, s++)
      *d = *s & val;
  return 1;
}


//= Bitwise OR each pixel value with binary mask.
// useful for turning all values odd

int jhcLUT_0::OrVal (jhcImg& dest, const jhcImg& src, int val) const 
{
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcLUT::OrVal");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, cnt = dest.RoiCnt(), rh = dest.RoiH(), sk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();
  
  // apply function to image
  for (y = rh; y > 0; y--, d += sk, s += sk)
    for (x = cnt; x > 0; x--, d++, s++)
      *d = (UC8)(*s | val);
  return 1;
}


//= Add a fixed increment to all pixels where gate is above threshold.

int jhcLUT_0::IncOver (jhcImg& dest, const jhcImg& gate, int val, int th) const 
{
  if (!dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcLUT::IncOver");
  dest.CopyRoi(gate);

  // general ROI case
  int x, y, i, nval, nf = dest.Fields();
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();
  
  // apply function to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*g++ > th)
        for (i = 0; i < nf; i++)
        {
          nval = *d + val;
          *d++ = BOUND(nval);
        }
      else
        d += nf;
    d += rsk;
    g += rsk;
  }
  return 1;
}


//= Add a fixed increment to all pixels where gate is above threshold.

int jhcLUT_0::IncUnder (jhcImg& dest, const jhcImg& gate, int val, int th) const 
{
  if (!dest.SameSize(gate, 1))
    return Fatal("Bad images to jhcLUT::IncUnder");
  dest.CopyRoi(gate);

  // general ROI case
  int x, y, i, nval, nf = dest.Fields();
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *g = gate.PxlSrc() + gate.RoiOff();
  
  // apply function to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*g++ < th)
        for (i = 0; i < nf; i++)
        {
          nval = *d + val;
          *d++ = BOUND(nval);
        }
      else
        d += nf;
    d += rsk;
    g += rsk;
  }
  return 1;
}


//= Everywhere pixel equals a certain value, replace with other value.

int jhcLUT_0::Replace (jhcImg& dest, int targ, int subst) const 
{
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  
  // apply function to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--, d++)
      if (*d == targ)
        *d = (UC8) subst;
    d += rsk;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Depth Conversions                          //
///////////////////////////////////////////////////////////////////////////

//= Convert full 4xmm depth image into monochrome version (dark = far).
// can optionally shift down by 1 or 2 bits to get full range
// 0 = unknown, 1 = beyond range, else 2-255 = 8.1, 14.8, or 28.1 ft to 17.3"

int jhcLUT_0::Night8 (jhcImg& d8, const jhcImg& d16, int sh) const
{
  if (!d8.Valid(1) || !d8.SameSize(d16, 2))
    return Fatal("Bad images to jhcLUT::Night8");
  d8.CopyRoi(d16);

  // general ROI case
  int x, y, v, n = __max(0, __min(sh, 2)) + 5;
  int w = d8.RoiW(), h = d8.RoiH(), ssk2 = d16.Skip() >> 1, dsk = d8.Skip();
  const US16 *s = (US16 *) d16.RoiSrc();
  UC8 *d = d8.RoiDest();

  // convert pixels
  for (y = h; y > 0; y--, d += dsk, s += ssk2)
    for (x = w; x > 0; x--, d++, s++)
      if ((*s < 1760) || (*s > 40000))
        *d = 0;
      else
      {
        // scale range (sh 0 = 8.1' max, 1 = 14.8' max, 2 = 28.1' max)
        v = 255 - ((*s - 1760) >> n);
        if (v <= 0)
          *d = 1;
        else
          *d = (UC8) v;
      }

  // possibly brighten further
  if (sh < 0)
    ClipScale(d8, d8, 1 << (-sh));
  return 1;
}


//= Convert 8 bit image back to approximate full 4xmm depth (bright = far).

int jhcLUT_0::Fog16 (jhcImg& d16, const jhcImg& d8) const
{
  if (!d16.Valid(2) || !d16.SameSize(d8, 1))
    return Fatal("Bad images to jhcLUT::Fog16");
  d16.CopyRoi(d8);

  // general ROI case
  int x, y, w = d8.RoiW(), h = d8.RoiH(), ssk = d8.Skip(), dsk2 = d16.Skip() >> 1;
  const UC8 *s = d8.RoiSrc();
  US16 *d = (US16 *) d16.RoiDest();

  // convert pixels
  for (y = h; y > 0; y--, d += dsk2, s += ssk)
    for (x = w; x > 0; x--, d++, s++)
      if (*s == 0)
        *d = 65535;                  // invalid
      else if (*s == 1)
        *d = 40000;                  // max range
      else
        *d = 9920 - (*s << 5);
  return 1;
}


//= Convert full 4xmm depth image into monochrome version (dark = far).
// linearly maps value so lo16 goes to hi8, hi16 goes to lo8
// 0 = unknown, else 1-255 = grayscale depth in range (saturates)

int jhcLUT_0::Remap16 (jhcImg& d8, const jhcImg& d16, int lo16, int hi16, int lo8, int hi8) const
{
  if (!d8.Valid(1) || !d8.SameSize(d16, 2))
    return Fatal("Bad images to jhcLUT::Night8");
  d8.CopyRoi(d16);

  // general ROI case
  int x, y, f, w = d8.RoiW(), h = d8.RoiH(), ssk2 = d16.Skip() >> 1, dsk = d8.Skip();
  UC8 v;
  const US16 *s = (US16 *) d16.RoiSrc();
  UC8 *d = d8.RoiDest();

  // get integer conversion coefficient
  f = ((hi8 - lo8) << 16) / (hi16 - lo16);

  // convert pixels
  for (y = h; y > 0; y--, d += dsk, s += ssk2)
    for (x = w; x > 0; x--, d++, s++)
      if ((*s < 1760) || (*s > 40000))
        *d = 0;
      else
      {
        v = (UC8)(255 - ((f * (*s - lo16) + 32768) >> 16));
        *d = __max(0, __min(v, 254)) + 1;
      }
  return 1;
}
