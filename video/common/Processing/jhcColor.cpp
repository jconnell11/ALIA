// jhcColor.cpp : various routines for manipulating color in images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
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

#include <math.h>
#include "Interface/jhcMessage.h"

#include "Processing/jhcColor.h"


///////////////////////////////////////////////////////////////////////////
//                           Initialization                              //
///////////////////////////////////////////////////////////////////////////

// Done this way to make only one copy and keep it off the stack.
// Static member variables must be initialized at file scope.

int jhcColor::instances = 0;

UL32 *jhcColor::norm8 = NULL;   
UC8 *jhcColor::third = NULL; 
UC8 *jhcColor::blut = NULL, *jhcColor::glut = NULL, *jhcColor::rlut = NULL;
UC8 *jhcColor::rgsc = NULL, *jhcColor::bysc = NULL;  
int *jhcColor::rf2 = NULL, *jhcColor::gf2 = NULL, *jhcColor::bf2 = NULL;
UC8 (*jhcColor::ratio)[256] = NULL;
UC8 (*jhcColor::invtan2)[256] = NULL;


//= Load up some precomputed arrays for various functions.

jhcColor::jhcColor ()
{
  int m, i, x, y, f, v;
  UL32 sum;
  double dx, dy, val;
  double rad3 = sqrt(3.0), ang255 = 256.0 / (2.0 * 3.141592653589);
  double rg = 0.5 * rad3, by = 1.0;   // length = std. dev., angle = hue

  // see if initialization already done by another instance
  if (instances++ > 0)
    return;

  // allocate space for all the tables
  norm8 = new UL32 [768];   
  third = new UC8 [768];
  blut = new UC8 [256];
  glut = new UC8 [256];
  rlut = new UC8 [256];
  rgsc = new UC8 [511];
  bysc = new UC8 [511];
  rf2 = new int [256];
  gf2 = new int [256];
  bf2 = new int [256];
  ratio = new UC8 [256][256];
  invtan2 = new UC8 [512][256];
  if ((norm8 == NULL) || (third == NULL)   || 
      (blut == NULL)  || (glut == NULL)    || (rlut == NULL) ||
      (rgsc == NULL)  || (bysc == NULL)    ||
      (rf2 == NULL)   || (gf2 == NULL)     || (bf2 == NULL)  ||
      (ratio == NULL) || (invtan2 == NULL))
    Fatal("Could not allocate tables in jhcColor");

  // normalization for sums 1 to 765 (Sat)
  norm8[0] = 65280;          
  for (i = 1; i < 768; i++)
    norm8[i] = ROUND(65280.0 / i);  // 65280 = 255 * 256

  // compute one third of all possible sums (MaskHSI)
  for (i = 0; i <= 765; i++)
    third[i] = (UC8)((i + 1) / 3);

  // compute weighting tables for RGB to Intensity
  for (i = 0; i <= 255; i++)
  {
    rlut[i] = BOUND(ROUND(0.30 * i));
    glut[i] = BOUND(ROUND(0.59 * i));
    blut[i] = BOUND(ROUND(0.11 * i));
  }

  // compute difference scaling tables
  for (v = -255; v <= 255; v++)
  {
    rgsc[v + 255] = BOUND(rg * v + 128.0); 
    bysc[v + 255] = BOUND(by * v + 128.0); 
  }

  // compute inverse scaling coefficients for all maxima (MaxBoost)
  for (i = 0; i <= 255; i++)
    ratio[0][i] = 0;
  for (m = 1; m <= 255; m++)
  {
    f = ROUND(256.0 * (255.0 / m));
    sum = 128;
    for (i = 0; i <= 255; i++)
    {
      v = sum >> 8;
      ratio[m][i] = BOUND(v);
      sum += f;
    }
  }

  // create 2 input inverse tangent table (SelectHCI)
  for (y = 0; y < 256; y++)
  {
    dy = rad3 * (y - 128);
    for (x = 0; x < 512; x++)
    {
      dx = (double)(x - 256);
      val = ang255 * atan2(dy, dx);
      if (val < 0.0)
        val += 256.0;
      invtan2[x][y] = (UC8) ROUND(val);
    }
  }

  // compute weighting tables for RGB to Double Intensity
  for (i = 0; i <= 255; i++)
  {
    rf2[i] = (int)(0.60 * 65536.0 * i);
    gf2[i] = (int)(1.18 * 65536.0 * i);
    bf2[i] = (int)(0.22 * 65536.0 * i);
  }
}


//= Cleans up allocated tables if last instance in existence.

jhcColor::~jhcColor ()
{
  if (--instances > 0)
    return;

  // free all arrays
  if (invtan2 != NULL)
    delete [] invtan2;
  if (ratio != NULL)
    delete [] ratio;
  if (bf2 != NULL)
    delete [] bf2;
  if (gf2 != NULL)
    delete [] gf2;
  if (rf2 != NULL)
    delete [] rf2;
  if (bysc != NULL)
    delete [] bysc;
  if (rgsc != NULL)
    delete [] rgsc;
  if (rlut != NULL)
    delete [] rlut;
  if (glut != NULL)
    delete [] glut;
  if (blut != NULL)
    delete [] blut;
  if (third != NULL)
    delete [] third;
  if (norm8 != NULL)
    delete [] norm8;
  
  // reinitialize
  instances = 0;
  norm8   = NULL;
  third   = NULL;
  blut    = NULL;
  glut    = NULL;
  rlut    = NULL;
  rgsc    = NULL;
  bysc    = NULL;
  rf2     = NULL;
  gf2     = NULL;
  bf2     = NULL;
  ratio   = NULL;
  invtan2 = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                        Color Transformations                          //
///////////////////////////////////////////////////////////////////////////

//= Pump up all intensities so either R, G, or B is 255.
// for triples whose max is less than th, set to black
// limit scaling factor to less than bmax (if nonzero)

int jhcColor::MaxBoost (jhcImg& dest, const jhcImg& src, int th) const
{
  if (!dest.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcColor::MaxBoost");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, r, g, b, m;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;

  // normalize pixel triples if max is above threshold 
  // uses "global" lookup table 
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      b = s[0];
      g = s[1];
      m = __max(g, b);
      r = s[2];
      s += 3;
      m = __max(r, m);

      if (m <= th)
      {
        d[0] = 0;
        d[1] = 0;
        d[2] = 0;
      }
      else
      {
        d[0] = ratio[m][b];
        d[1] = ratio[m][g];
        d[2] = ratio[m][r];
      }
      d += 3;
    }
    s += rsk;
    d += rsk;
  }
  return 1;
}


//= Pump up all intensities so either R, G, or B is 255.
// limit channel boost factor to fmax at most

int jhcColor::MaxColor (jhcImg& dest, const jhcImg& src, double fmax) const
{
  if (!dest.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcColor::MaxColor");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, r, g, b, m, top = ROUND(255.0 / fmax);
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;

  // normalize pixel triples if max is above threshold 
  // uses "global" lookup table 
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      // find maximum color channel
      b = s[0];
      g = s[1];
      m = __max(g, b);
      r = s[2];
      s += 3;
      m = __max(r, m);

      // apply associated scaling factor
      m = __max(m, top);
      d[0] = ratio[m][b];
      d[1] = ratio[m][g];
      d[2] = ratio[m][r];
      d += 3;
    }
    s += rsk;
    d += rsk;
  }
  return 1;
}


//= Multiply each color component by separate scale factor.
// all results limited to 255 maximum

int jhcColor::ScaleRGB (jhcImg& dest, const jhcImg& src, double rsc, double gsc, double bsc) const
{
  if (!src.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcColor::ScaleRGB");
  if ((rsc < 0.0) || (gsc < 0.0) || (bsc < 0.0))
    return 0;
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, c, val;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 scaled[3][256];
  int sum[3] = {128, 128, 128};
  int f[3];
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;

  // compute answers for all possible values
  f[0] = ROUND(256.0 * bsc);
  f[1] = ROUND(256.0 * gsc);
  f[2] = ROUND(256.0 * rsc);
  for (c = 0; c < 3; c++)
    for (i = 0; i <= 255; i++)
    {
      val = (int)(sum[c] >> 8);
      if (val >= 255)
        scaled[c][i] = 255;
      else
        scaled[c][i] = (UC8) val;
      sum[c] += f[c];
    }

  // apply lookup table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      d[0] = scaled[0][s[0]];
      d[1] = scaled[1][s[1]];
      d[2] = scaled[2][s[2]];
      d += 3;
      s += 3;
    }
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Combines several 16 bit color planes into one 16 bit monochrome image.
// all results limited to 65535 maximum

int jhcColor::ScaleRGB_16 (jhcImg& dest, const jhcImg& red, const jhcImg& grn, const jhcImg& blu, 
                           double rsc, double gsc, double bsc) const
{
  if (!dest.Valid(2) || 
      !dest.SameFormat(red) || !dest.SameFormat(grn) || !dest.SameFormat(blu))
    return Fatal("Bad images to jhcColor::ScaleRGB_16");
  if ((rsc < 0.0) || (gsc < 0.0) || (bsc < 0.0) || 
      (rsc >= 10.0) || (gsc >= 10.0) || (bsc >= 10.0))
    return 0;
  dest.CopyRoi(red);
  dest.MergeRoi(grn);
  dest.MergeRoi(blu);

  // general ROI case
  int x, y, sum, rw = dest.RoiW(), rh = dest.RoiH(); 
  int dsk = dest.RoiSkip() >> 1, ssk = red.RoiSkip(dest) >> 1;
  int rf = ROUND(1024.0 * rsc), gf = ROUND(1024.0 * gsc), bf = ROUND(1024.0 * bsc);
  const US16 *r = (const US16 *) red.RoiSrc(dest);
  const US16 *g = (const US16 *) grn.RoiSrc(dest);
  const US16 *b = (const US16 *) blu.RoiSrc(dest);
  US16 *d = (US16 *) dest.RoiDest();

  for (y = rh; y > 0; y--, d += dsk, r += ssk, g += ssk, b += ssk)
    for (x = rw; x > 0; x--, d++, r++, g++, b++)
    {
      sum = (rf * (*r) + gf * (*g) + bf * (*b) + 512) >> 10; 
      *d = (US16) __min(sum, 65535);
    }
  return 1;
}


//= Pump up all intensities so the average is 85.

int jhcColor::IsoLum (jhcImg& dest, const jhcImg& src) const
{
  if (!src.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcColor::IsoLum");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, r, g, b, i3, v;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff;
  double f;

  // normalize pixel triples if max is above threshold 
  // uses "global" lookup table 
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      b = s[0];
      g = s[1];
      r = s[2];
      s += 3;
      i3 = r + g + b;
      if (i3 == 0)
      {
        d[0] = 0;
        d[1] = 0;
        d[2] = 0;
      }
      else
      {
        // very inefficient!
        f = 255.0 / i3;
        v = ROUND(f * b);
        d[0] = BOUND(v);
        v = ROUND(f * g);
        d[1] = BOUND(v);
        v = ROUND(f * r);
        d[2] = BOUND(v);
      }
      d += 3;
    }
    s += rsk;
    d += rsk;
  }
  return 1;
}


//= Computes 6 channels of opponent color.
//   R = 2 * (r - g), G = 2 * (g - r), B = 2 * b - r - g, Y = r + g - 2 * b
//   W = 0.60 r + 1.18 g + 0.22 b - 256, K = 256 - 0.60 r - 1.18 g - 0.22 b

int jhcColor::HexColor (jhcImg& red, jhcImg& grn, jhcImg& blu, jhcImg& yel, 
                        jhcImg& wht, jhcImg& blk, const jhcImg& src, int x2) const
{
  if (!src.Valid(3) || !src.SameSize(red, 1) ||
      !red.SameFormat(grn) || !red.SameFormat(blu) || !red.SameFormat(yel) ||
      !red.SameFormat(wht) || !red.SameFormat(blk))
    return Fatal("Bad images to jhcColor::HexColor");
  red.CopyRoi(src);
  grn.CopyRoi(src);
  blu.CopyRoi(src);
  yel.CopyRoi(src);
  wht.CopyRoi(src);
  blk.CopyRoi(src);

  // general ROI case
  int x, y, val, up = 1, dn = 0, rw = src.RoiW(), rh = src.RoiH();
  int ssk = src.RoiSkip(), dsk = red.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *r = red.RoiDest(), *g = grn.RoiDest(), *b = blu.RoiDest();
  UC8 *l = yel.RoiDest(), *w = wht.RoiDest(), *k = blk.RoiDest();

  // see if doubling of R-G, B-Y range requested
  if (x2 <= 0)
  {
    up = 0;
    dn = 1;
  }

  // do color conversion in relevant part of image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, s += 3, r++, g++, b++, l++, w++, k++)
    {
      val = (s[2] - s[1]) << up;
      *r = BOUND(val);
      val = -val;
      *g = BOUND(val);
      val = ((s[0] << 1) - s[1] - s[2]) >> dn;
      *b = BOUND(val);
      val = -val;
      *l = BOUND(val);
      val = ((bf2[s[0]] + gf2[s[1]] + rf2[s[2]]) >> 16) - 256;
      *w = (UC8)((val <= 0) ? 0 : val);
      *k = (UC8)((val >= 0) ? 0 : -val);
    }
    s += ssk;
    r += dsk;
    g += dsk;
    b += dsk;
    l += dsk;
    w += dsk;
    k += dsk;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Alternate Color Spaces                          //
///////////////////////////////////////////////////////////////////////////

//= Sees how reddish a region, zero if g > r (and sc > 0).
// scale factor can be negative to get greenish regions
// essentially red minus green

int jhcColor::Redness (jhcImg& dest, const jhcImg& src, double sc) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::Redness");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, val, f = ROUND(256.0 * sc);
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  UC8 scaled[512];
  int sum = -255 * f + 128;

  // precompute all scaled differences
  for (i = -255; i <= 255; i++)
  {
    val = (int)(sum >> 8);
    scaled[i + 255] = BOUND(val);
    sum += f;
  }

  // apply lookup table to image (b = s[0], g = s[1], r = s[2]) 
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      *d++ = scaled[(int) s[2] - (int) s[1] + 255];
      s += 3; 
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Sees how bluish a region is, zero if y > b and (sc > 0).
// scale factor can be negative to get yellowish regions
// essentially blue minus yellow (average of red and green)

int jhcColor::Blueness (jhcImg& dest, const jhcImg& src, double sc) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::Blueness");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, val, f = ROUND(256.0 * sc);   
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 scaled[1021];
  int sum = -510 * f + 256;

  // precompute all scaled differences
  for (i = -510; i <= 510; i++)
  {
    val = (int)(sum >> 9);          // divides by 2
    scaled[i + 510] = BOUND(val);
    sum += f;
  }

  // apply lookup table to image (b = s[0], g = s[1], r = s[2]) 
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--)
    {
      val = (int) s[0] + (int) s[0] - (int) s[1] - (int) s[2];
      *d++ = scaled[val + 510];
      s += 3; 
    }
  return 1;
}


//= Compute full color differences R-G and Y-B from an input image.
// values are "signed" with zero at 128

int jhcColor::ColorDiffs (jhcImg& rg, jhcImg& yb, const jhcImg& src) const
{
  if (!src.Valid(3) || !src.SameSize(rg, 1) || !src.SameSize(yb, 1))
    return Fatal("Bad images to jhcColor::ColorDiffs");
  rg.CopyRoi(src);
  yb.CopyRoi(src);

  // general ROI case
  int x, y, rw = src.RoiW(), rh = src.RoiH(), ssk = src.RoiSkip(), dsk = rg.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *rgd = rg.RoiDest(), *ybd = yb.RoiDest();

  for (y = rh; y > 0; y--, s += ssk, rgd += dsk, ybd += dsk)
    for (x = rw; x > 0; x--, s += 3, rgd++, ybd++) 
    {
      *rgd = (UC8)((s[2] - s[1] + 256) >> 1);
      *ybd = (UC8)((s[2] + s[1] - s[0] - s[0] + 512) >> 2);
    }
  return 1;
}

//= Compute full color differences R-C and G-M from an input image.
// values are "signed" with zero at 128

int jhcColor::ColorDiffsRC (jhcImg& rc, jhcImg& gm, const jhcImg& src) const
{
  if (!src.Valid(3) || !src.SameSize(rc, 1) || !src.SameSize(gm, 1))
    return Fatal("Bad images to jhcColor::ColorDiffsRC");
  rc.CopyRoi(src);
  gm.CopyRoi(src);

  // general ROI case
  int x, y, rw = src.RoiW(), rh = src.RoiH(), ssk = src.RoiSkip(), dsk = rc.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *rcd = rc.RoiDest(), *gmd = gm.RoiDest();

  for (y = rh; y > 0; y--, s += ssk, rcd += dsk, gmd += dsk)
    for (x = rw; x > 0; x--, s += 3, rcd++, gmd++) 
    {
      *rcd = (UC8)((s[2] + s[2] - s[0] - s[1] + 512) >> 2);
      *gmd = (UC8)((s[1] + s[1] - s[0] - s[2] + 512) >> 2);
    }
  return 1;
}


//= Compares red component to opposite cyan component.
// scale factor can be negative to get aqua-colored regions
// essentially red minus cyan (average of blue and green)

int jhcColor::RCDiff (jhcImg& dest, const jhcImg& src, double sc) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::RCDiff");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, val, f = ROUND(256.0 * sc);   
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 scaled[1021];
  int sum = -510 * f + 256;

  // precompute all scaled differences
  for (i = -510; i <= 510; i++)
  {
    val = (int)(sum >> 9);          // divides by 2
    scaled[i + 510] = BOUND(val);
    sum += f;
  }

  // apply lookup table to image (b = s[0], g = s[1], r = s[2]) 
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--)
    {
      val = (int) s[2] + (int) s[2] - (int) s[1] - (int) s[0];
      *d++ = scaled[val + 510];
      s += 3; 
    }
  return 1;
}


//= Compares green component to opposite magenta component.
// scale factor can be negative to get purple regions
// essentially green minus magenta (average of red and blue)

int jhcColor::GMDiff (jhcImg& dest, const jhcImg& src, double sc) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::GMDiff");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i, val, f = ROUND(256.0 * sc);   
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 scaled[1021];
  int sum = -510 * f + 256;

  // precompute all scaled differences
  for (i = -510; i <= 510; i++)
  {
    val = (int)(sum >> 9);          // divides by 2
    scaled[i + 510] = BOUND(val);
    sum += f;
  }

  // apply lookup table to image (b = s[0], g = s[1], r = s[2]) 
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--)
    {
      val = (int) s[1] + (int) s[1] - (int) s[0] - (int) s[2];
      *d++ = scaled[val + 510];
      s += 3; 
    }
  return 1;
}


//= Fills destination with saturation of other RGB image (full color = 255).
// sets saturation to zero in areas of low intensity (e.g. under 50)

int jhcColor::Sat (jhcImg& dest, const jhcImg& src, int ith) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::Sat");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, r, g, b, i, m, i3 = 3 * ith;
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  UL32 scaled[768];

  // figure out all intensity (x3) normalizations
  for (i = 0; i <= 765; i++)
    scaled[i] = 3 * norm8[i];

  // process pixels
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      b = s[0];
      g = s[1];
      r = s[2];
      s += 3;
      i = r + g + b;
      if ((i == 0) || (i <= i3))
        *d++ = 0;
      else
      {
        m = __min(r, g);
        m = __min(m, b);
        *d++ = (UC8)(255 - ((m * scaled[i]) >> 8));
      }
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Interprets hue vector magnitude as saturation (full color = 255).
// this is equivalent to standard deviation of RGB over average
// sets saturation to zero in areas of low intensity (e.g. under 50)

int jhcColor::VectSat (jhcImg& dest, const jhcImg& src, int ith) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::VectSat");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, r, g, b, i, vs, i3 = 3 * ith;
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  double dx, dy, rad3 = sqrt(3.0), sc = 255.0 / sqrt(2.0);
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();

  // process pixels
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      b = s[0];
      g = s[1];
      r = s[2];
      s += 3;
      i = r + g + b;
      if ((i == 0) || (i <= i3))
        *d++ = 0;
      else
      {
        // very inefficient inner loop!
        dx = rad3 * (g - b);  
        dy = 2.0 * r - g - b;
        vs = ROUND(sc * sqrt(dx * dx + dy * dy) / i);
        *d++ = BOUND(vs);
      }
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Fills image with hue of other RGB image (360 deg = 255).
// zeroes hue in regions of low intensity or sat (e.g under 25)
// def is the value used to record ambiguous hue (e.g. 0)

int jhcColor::Hue (jhcImg& dest, const jhcImg& src, int sth, int ith, int def) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::Hue");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, r, g, b, i, m, xval, yval, i3 = 3 * ith;
  int sinc = ROUND(65536.0 * (1.0 - (sth / 255.0)) / 3.0);
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UL32 ssum = 32768;
  UC8 dval = BOUND(def);
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  int smin[768];

  // figure out saturation thresholds for all intensities
  for (i = 0; i <= 765; i++)
  {
    smin[i] = (int)(ssum >> 16);
    ssum += sinc;
  }

  // process pixels
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      b = s[0];
      g = s[1];
      r = s[2];
      s += 3;
      i = r + g + b;
      if ((i == 0) || (i <= i3))
        *d++ = dval;
      else
      {
        m = __min(r, g);
        m = __min(m, b);
        if (m > smin[i])
          *d++ = dval;
        else
        {
          yval = g - b;
          xval = (r << 1) - g - b;
          *d++ = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
//          variation for better scaling of small differences
//          if ((-128 < yval) && (yval < 128) && (-256 < xval) && (xval < 256))
//            *d++ = invtan2[xval + 256][yval + 128];
//          else
//            *d++ = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
        }
      }
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Similar to basic Hue function, but also returns a mask of where color is valid.

int jhcColor::HueMask (jhcImg& dest, jhcImg& gate, const jhcImg& src, int sth, int ith) const
{
  if (!dest.Valid(1) || !dest.SameFormat(gate) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::HueMask");
  dest.CopyRoi(src);
  gate.CopyRoi(src);

  // general ROI case
  int x, y, r, g, b, i, m, xval, yval, i3 = 3 * ith;
  int sinc = ROUND(65536.0 * (1.0 - (sth / 255.0)) / 3.0);
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UL32 ssum = 32768;
  UC8 *d = dest.RoiDest(), *v = gate.RoiDest();
  const UC8 *s = src.RoiSrc();
  int smin[768];

  // figure out saturation thresholds for all intensities
  for (i = 0; i <= 765; i++)
  {
    smin[i] = (int)(ssum >> 16);
    ssum += sinc;
  }

  // process pixels
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d++, v++, s += 3)
    {
      b = s[0];
      g = s[1];
      r = s[2];
      i = r + g + b;
      *d = 0;
      *v = 0;
      if ((i > i3) && (i != 0))
      {
        m = __min(r, g);
        m = __min(m, b);
        if (m <= smin[i])
        {
          yval = g - b;
          xval = (r << 1) - g - b;
          *d = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
          *v = 255;
        }
      }
    }
    d += dsk;
    v += dsk;
    s += ssk;
  }
  return 1;
}


//= Computes hue, saturation, and intensity where mask image is non-zero.

int jhcColor::MaskHSI (jhcImg& hue, jhcImg& sat, jhcImg& brite, 
                       const jhcImg& src, const jhcImg& mask) const
{
  if (!src.Valid(3) || !src.SameSize(mask, 1) ||
      !mask.SameFormat(hue) || !mask.SameFormat(sat) || !mask.SameFormat(brite)) 
    return Fatal("Bad images to jhcColor::MaskHSI");
  hue.CopyRoi(src);
  hue.MergeRoi(mask);
  sat.CopyRoi(hue);
  brite.CopyRoi(hue);

  // general ROI case
  int x, y, r, g, b, i, m, xval, yval;
  int rw = hue.RoiW(), rh = hue.RoiH(), dsk = hue.RoiSkip(), ssk = src.RoiSkip();
  UL32 roff = hue.RoiOff();
  UC8 *hv = hue.PxlDest() + roff, *sv = sat.PxlDest() + roff;
  UC8 *iv = brite.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + src.RoiOff(), *av = mask.PxlSrc() + roff;
  
  // process image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      // if outside mask, record zeroes for HSI
      if (*av++ == 0)
      {
        s += 3;
        *hv++ = 0;
        *sv++ = 0;
        *iv++ = 0;
        continue;
      }

      // extract RGB for this pixel
      b = s[0];
      g = s[1];
      r = s[2];
      s += 3;

      // compute and save intensity
      i = third[r + g + b];
      *iv++ = (UC8) i;

      // compute and save intensity
      m = __min(r, g);
      m = __min(m, b);
      *sv++ = (UC8)(255 - ((m * norm8[i]) >> 8));

      // compute and save hue
      yval = g - b;
      xval = (r << 1) - g - b;
      *hv++ = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
//      variation for better scaling of small differences
//      if ((-128 < yval) && (yval < 128) && (-256 < xval) && (xval < 256))
//        *hv++ = invtan2[xval + 256][yval + 128];
//      else
//        *hv++ = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
    }
    hv += dsk;
    sv += dsk;
    iv += dsk;
    av += dsk;
    s += ssk;
  }
  return 1;
}


//= Computes hue, saturation, and intensity in ROI.

int jhcColor::RGBtoHSI (jhcImg& hue, jhcImg& sat, jhcImg& brite, const jhcImg& src) const
{
  if (!src.Valid(3) || !src.SameSize(hue, 1) ||
      !hue.SameFormat(sat) || !hue.SameFormat(brite)) 
    return Fatal("Bad images to jhcColor::RGBtoHSI");
  hue.CopyRoi(src);
  sat.CopyRoi(hue);
  brite.CopyRoi(hue);

  // general ROI case
  int x, y, r, g, b, i, m, xval, yval;
  int rw = hue.RoiW(), rh = hue.RoiH(), dsk = hue.RoiSkip(), ssk = src.RoiSkip();
  UL32 roff = hue.RoiOff();
  UC8 *hv = hue.PxlDest() + roff, *sv = sat.PxlDest() + roff;
  UC8 *iv = brite.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  
  // process image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      // extract RGB for this pixel
      b = s[0];
      g = s[1];
      r = s[2];
      s += 3;

      // compute and save intensity
      i = third[r + g + b];
      *iv++ = (UC8) i;

      // compute and save intensity
      m = __min(r, g);
      m = __min(m, b);
      *sv++ = (UC8)(255 - ((m * norm8[i]) >> 8));

      // compute and save hue
      yval = g - b;
      xval = (r << 1) - g - b;
      *hv++ = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
//      variation for better scaling of small differences
//      if ((-128 < yval) && (yval < 128) && (-256 < xval) && (xval < 256))
//        *hv++ = invtan2[xval + 256][yval + 128];
//      else
//        *hv++ = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
    }
    hv += dsk;
    sv += dsk;
    iv += dsk;
    s += ssk;
  }
  return 1;
}


//= Convert standard color image into (R-G, W-K, B-Y) difference image.

int jhcColor::RGBtoDiff (jhcImg& cdiff, const jhcImg& rgb) const
{
  if (!rgb.Valid(3) || !rgb.SameFormat(cdiff) || rgb.SameImg(cdiff))
    return Fatal("Bad images to jhcColor::RGBtoDiff");
  cdiff.CopyRoi(rgb);

  // general ROI case
  int v, x, y, rw = cdiff.RoiW(), rh = cdiff.RoiH(), sk = cdiff.RoiSkip();
  UC8 *d = cdiff.RoiDest();
  const UC8 *s = rgb.RoiSrc();

  // find differences and apply tables
  for (y = rh; y > 0; y--, d += sk, s += sk)
    for (x = rw; x > 0; x--, d += 3, s += 3)
    {
      // compute red-green difference for red channel
      v = s[2] - s[1] + 255;
      d[2] = rgsc[v];

      // compute blue-yellow difference for blue channel
      v = (s[0] + s[0] - s[1] - s[2] + 510) >> 1;
      d[0] = bysc[v];

      // compute intensity in green channel using "global" lookup tables
      v = blut[s[0]] + glut[s[1]] + rlut[s[2]];
      if (v <= 255)
        d[1] = (UC8) v;
      else
        d[1] = 255;
    }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Region Selection                              //
///////////////////////////////////////////////////////////////////////////

//= Looks for regions of image that have the given hue and saturation.
// labels pixels that pass all tests with 255, failed pixels with 0
// uses intensity = RGB avg, saturation = 255 * [1 - (RGB min / intensity)]
// hue is the standard form (0 = red, 85 = green, 170 = blue)

int jhcColor::SelectHSI (jhcImg& dest, const jhcImg& src, 
                         int hlo, int hhi, int slo, int shi, int ilo, int ihi) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::SelectHSI");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, r, g, b, i, m, h;
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  int i0 = 3 * ilo, i1 = 3 * ihi, s0 = 255 - slo, s1 = 255 - shi;
  int h0 = hlo, h1 = hhi;
  int xval, yval;
  UC8 in = 255, out = 0;
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  
  // decide how broad range is and whether zero is included
  if (hhi < hlo)
  {
    h0 = hhi;
    h1 = hlo;
    in = 0;
    out = 255;
  }

  // process image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      b = s[0];
      g = s[1];
      r = s[2];
      s += 3;
      i = r + g + b;
      if ((i < i0) || (i > i1))
        *d++ = 0;
      else
      {
        m = __min(r, g);
        m = __min(m, b);
        m *= 765;
        if ((m > i * s0) || (m < i * s1))
          *d++ = 0;
        else
        {
          yval = g - b;
          xval = (r << 1) - g - b;
          h = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
//          variation for better scaling of small differences
//          if ((-128 < yval) && (yval < 128) && (-256 < xval) && (xval < 256))
//            h = invtan2[xval + 256][yval + 128];
//          else
//            h = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
          if ((h >= h0) && (h <= h1))
            *d++ = in;
          else
            *d++ = out;
        }
      }
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Like other SelectHSI but assume hue, saturation, and intensity have already been.
// computed and provide as arrays. 
// Also, only examines pixels for which the value in "mask" is non-zero.

int jhcColor::SelectHSI (jhcImg& dest, 
                         const jhcImg& hue, const jhcImg& sat, const jhcImg& brite, const jhcImg& mask, 
                         int hlo, int hhi, int slo, int shi, int ilo, int ihi) const
{
  if (!dest.Valid(1) || !dest.SameFormat(mask) ||
      !dest.SameFormat(hue) || !dest.SameFormat(sat) || !dest.SameFormat(brite))
    return Fatal("Bad images to jhcColor::SelectHSI (given HSI)");
  dest.CopyRoi(mask);
  dest.MergeRoi(hue);
  dest.MergeRoi(sat);
  dest.MergeRoi(brite);

  // general ROI case
  int x, y, h0 = hlo, h1 = hhi;
  UC8 in = 255, out = 0;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *av = mask.PxlSrc() + roff, *hv = hue.PxlSrc() + roff;
  const UC8  *sv = sat.PxlSrc() + roff, *iv = brite.PxlSrc() + roff;

  // decide how broad range is and whether zero is included
  if (hhi < hlo)
  {
    h0 = hhi;
    h1 = hlo;
    in = 0;
    out = 255;
  }

  // process image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      // set output to zero if mask is zero
      if (*av++ == 0)
      {
        *d++ = 0;
        hv++;
        sv++;
        iv++;
        continue;
      }

      // test HSI values to set output
      if ((*iv < ilo) || (*iv > ihi))
        *d++ = 0;
      else if ((*sv < slo) || (*sv > shi))
        *d++ = 0;
      else if ((*hv < h0) || (*hv > h1))
        *d++ = out;
      else
        *d++ = in;

      // increment pointers
      hv++;
      sv++;
      iv++;
    }
    d += rsk;
    av += rsk;
    hv += rsk;
    sv += rsk;
    iv += rsk;
  }
  return 1;
}


//= Looks for regions of image that have the given hue and CHROMA.
// labels pixels that pass all tests with 255, failed pixels with 0
// hue is the standard form (0 = red, 85 = green, 170 = blue)

int jhcColor::SelectHCI (jhcImg& dest, const jhcImg& src, 
                         int hlo, int hhi, int clo, int chi, int ilo, int ihi) const
{
  if (!dest.SameSize(src) || !dest.Valid(1) || !src.Valid(3))
    return Fatal("Bad images to jhcColor::SelectHCI");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, r, g, b, i, m, c, h;
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  int i0 = 3 * ilo, i1 = 3 * ihi, c0 = 3 * clo, c1 = 3 * chi;
  int h0 = hlo, h1 = hhi;
  int xval, yval;
  UC8 in = 255, out = 0;
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();

  // decide how broad range is and whether zero is included
  if (hhi < hlo)
  {
    h0 = hhi;
    h1 = hlo;
    in = 0;
    out = 255;
  }

  // process image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      b = s[0];
      g = s[1];
      r = s[2];
      s += 3;
      i = r + g + b;
      if ((i < i0) || (i > i1))
        *d++ = 0;
      else
      {
        m = __min(r, g);
        m = __min(m, b);
        c = i - 3 * m;
        if ((c < c0) || (c > c1))
          *d++ = 0;
        else
        {
          yval = g - b;
          xval = (r << 1) - g - b;
          h = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
//          variation for better scaling of small differences
//          if ((-128 < yval) && (yval < 128) && (-256 < xval) && (xval < 256))
//            h = invtan2[xval + 256][yval + 128];
//          else
//            h = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
          if ((h >= h0) && (h <= h1))
            *d++ = in;
          else
            *d++ = out;
        }
      }
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Set pixel to 255 if it exactly matched RGB spec, else 0.

int jhcColor::ExactRGB (jhcImg& dest, const jhcImg& src, int r, int g, int b) const
{
  if (!dest.SameSize(src) || !dest.Valid(1) || !src.Valid(3))
    return Fatal("Bad images to jhcColor::ExactRGB");
  dest.CopyRoi(src);

  // general ROI case
  int x, y;
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();

  // process image
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if ((s[0] == b) && (s[1] == g) && (s[2] == r))
        *d++ = 255;
      else
        *d++ = 0;
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Like SelectHSI but returns graded response given tolerances.
// ramps responses off outside specified full-on range ___/~~\___
// combined response is the minimum in each aspect

int jhcColor::SoftHSI (jhcImg& dest, const jhcImg& src,                  
                       int hlo, int hhi, int slo, int shi, int ilo, int ihi,
                       int hdrop, int sdrop, int idrop) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcColor::SoftHSI");
  dest.CopyRoi(src);

  // general ROI case
  int v, x, y, r, g, b, i, iv, s, sv, h, hv;
  int i0 = __max(0, ilo), s0 = __max(0, slo), h0 = __max(0, hlo);
  int i1 = __min(ihi, 255), s1 = __min(shi, 255), h1 = __min(hhi, 255);
  int imin = i0 - idrop, imax = i1 + idrop, istep = ROUND(255.0 / (idrop + 1.0)); 
  int smin = s0 - sdrop, smax = s1 + sdrop, sstep = ROUND(255.0 / (sdrop + 1.0)); 
  int hmin, hmax, hstep = ROUND(255.0 / (hdrop + 1.0)); 
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  int xval, yval;
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *p = src.PxlSrc() + src.RoiOff();
  UC8 ipass[256], spass[256], hpass[256];

  // keep wrapping hue around until a positive linear sequence is found
  if (h0 < hdrop)
  {
    h0 += 256;
    h1 += 256;
  }
  if (h0 > h1)
    h1 += 256;
  hmin = h0 - hdrop;
  hmax = h1 + hdrop;

  // set default values for all tables
  for (v = 0; v < 256; v++)
  {
    ipass[v] = 0;
    spass[v] = 0;
    hpass[v] = 0;
  }
  
  // build intensity table
  for (v = imin; v <= imax; v++)
    if (v < i0)
      ipass[v] = (UC8)(istep * (v - imin));
    else if (v <= i1)
      ipass[v] = 255;
    else 
      ipass[v] = (UC8)(istep * (imax - v));

  // build saturation table
  for (v = smin; v <= smax; v++)
    if (v < s0)
      spass[v] = (UC8)(sstep * (v - smin));
    else if (v <= s1)
      spass[v] = 255;
    else 
      spass[v] = (UC8)(sstep * (smax - v));

  // build hue table
  for (v = hmin; v <= hmax; v++)
    if (v < h0)
      hpass[v & 0xFF] = (UC8)(hstep * (v - hmin));
    else if (v <= h1)
      hpass[v & 0xFF] = 255;
    else
      hpass[v & 0xFF] = (UC8)(hstep * (hmax - v));
 
  // process image using tables
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d++, p += 3)
    {
      b = p[0];
      g = p[1];
      r = p[2];
      i = (21845 * (r + g + b) + 32768) >> 16;  // 21846 = 65536 / 3
      iv = ipass[i];
      if (iv == 0)
      {
        *d = 0;
        continue;
      }
      s = __min(r, g);
      s = __min(s, b);
      if (s != 0)
        s = (((65280 * s) / i) + 128) >> 8;     // 65280 = 255 * 256 
      sv = spass[255 - s];
      if (sv == 0)
      {
        *d = 0;
        continue;
      }
      yval = g - b;
      xval = (r << 1) - g - b;
      h = invtan2[(xval + 512) >> 1][(yval + 256) >> 1];
      hv = hpass[h];
      if (hv == 0) 
      {
        *d = 0;
        continue;
      }
      *d = (UC8) __min(hv, __min(sv, iv));
    }
    d += dsk;
    p += ssk;
  }
  return 1;
}


//= Select approximately flesh colored regions.
// may want to do BoxThresh(d, s, 9, 128) on this to clean up edges

int jhcColor::SkinTone (jhcImg& dest, const jhcImg& src) const
{
  // parameters:            H0  H1  S0   S1   I0   I1
  return SoftHSI(dest, src, 10, 40, 50, 100, 100, 230, 10, 20, 20);
}


//= Alternate tuning for finding flesh colored regions.
// helps to apply jhcFilter first to despeckle

int jhcColor::SkinTone2 (jhcImg& dest, const jhcImg& src) const
{
  // parameters:            H0   H1  S0  S1  I0   I1
  return SoftHSI(dest, src, 245, 15, 10, 80, 50, 230, 10, 20, 20);
}


///////////////////////////////////////////////////////////////////////////
//                       Assembly and Disassembly                        //
///////////////////////////////////////////////////////////////////////////

//= Turn a monochrome image into an RGB one by duplicating into all fields.

int jhcColor::CopyMono (jhcImg& dest, const jhcImg& src) const
{
  if (!src.Valid(1) || !src.SameSize(dest, 3))
    return Fatal("Bad images to jhcColor::CopyMono");

  dest.CopyField(src, 0, 2);
  dest.CopyField(src, 0, 1);
  dest.CopyField(src, 0, 0);
  return 1;
}


//= Turn color image into 3 monochrome images.

int jhcColor::SplitRGB (jhcImg& r, jhcImg& g, jhcImg& b, const jhcImg& src) const
{
  if (!src.Valid(3)    || !src.SameSize(r, 1) || 
      !r.SameFormat(g) || !r.SameFormat(b))
    return Fatal("Bad images to jhcColor::SplitRGB");

  r.CopyField(src, 2, 0);
  g.CopyField(src, 1, 0);
  b.CopyField(src, 0, 0);
  return 1;
}


//= Combine 3 monochrome images into one color image.

int jhcColor::MergeRGB (jhcImg& dest, const jhcImg& r, const jhcImg& g, const jhcImg& b) const
{
  if (!dest.Valid(3)   || !dest.SameSize(r, 1) || 
      !r.SameFormat(g) || !r.SameFormat(b))
    return Fatal("Bad images to jhcColor::MergeRGB");
  
  dest.CopyField(r, 0, 2);
  dest.CopyField(g, 0, 1);
  dest.CopyField(b, 0, 0);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Raw Camera Color                           //
///////////////////////////////////////////////////////////////////////////

//= Converts a Bayer patterned monochrome image to a normal RGB color image.
// pixels on boundary of image are written to black always

int jhcColor::DeBayer (jhcImg& dest, const jhcImg& src) const
{
  if (src.Valid(2))
    return DeBayer16(dest, src);
  if (!src.Valid(1) || !src.SameSize(dest, 3))
    return Fatal("Bad images to jhcColor::DeBayer");
  dest.CopyRoi(src);

  // always does whole image irrespective of ROI (some matrix values not needed)
  int x, y, w = dest.XDim(), wm2 = w - 2, hm2 = dest.YDim() - 2;
  int dln = dest.Line(), sln = src.Line(), sln2 = sln << 1, sln3 = sln2 + sln;
  int dsk = dest.Skip(), dsk2 = dsk + dln, ssk2 = src.Skip() + sln + 2;
  int p00 = 0,    p10 = 1,        p20 = 2;   //   p30 = 3;
  int p01 = sln,  p11 = sln  + 1, p21 = sln  + 2, p31 = sln  + 3;
  int p02 = sln2, p12 = sln2 + 1, p22 = sln2 + 2, p32 = sln2 + 3;
  int             p13 = sln3 + 1, p23 = sln3 + 2, p33 = sln3 + 3;  // p03 = sln3,
  int b00 = 0, g00 = 1, r00 = 2, b01 = dln,     g01 = dln + 1, r01 = dln + 2;
  int b10 = 3, g10 = 4, r10 = 5, b11 = dln + 3, g11 = dln + 4, r11 = dln + 5;
  UC8 *d = dest.PxlDest();
  const UC8 *s = src.PxlSrc();

  // black bottom row
  for (x = w; x > 0; x--, d += 3)
  {
    d[0] = 0; 
    d[1] = 0; 
    d[2] = 0; 
  }
  d += dsk;

  // interior of image 
  for (y = hm2; y > 0; y -= 2, d += dsk2, s += ssk2)
  {
    // black on left (2 lines)
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    d[dln]     = 0;
    d[dln + 1] = 0;
    d[dln + 2] = 0;
    d += 3;

    for (x = wm2; x > 0; x -= 2, d += 6, s += 2)
    {
      // over an R pixel
      d[b00] = (UC8)((s[p00] + s[p20] + s[p02] + s[p22]) >> 2);  // blue avg 4
      d[g00] = (UC8)((s[p10] + s[p01] + s[p21] + s[p12]) >> 2);  // green avg 4
      d[r00] = (UC8)  s[p11];                                    // red copy

      // over a G pixel
      d[b10] = (UC8)((s[p20] + s[p22]) >> 1);                    // blue avg 2
      d[g10] = (UC8)  s[p21];                                    // green copy
      d[r10] = (UC8)((s[p11] + s[p31]) >> 1);                    // red avg 2

      // over another G pixel
      d[b01] = (UC8)((s[p02] + s[p22]) >> 1);                    // blue avg 2
      d[g01] = (UC8)  s[p12];                                    // green copy
      d[r01] = (UC8)((s[p11] + s[p13]) >> 1);                    // red avg 2

      // over a B pixel
      d[b11] = (UC8)  s[p22];                                    // blue copy
      d[g11] = (UC8)((s[p21] + s[p12] + s[p32] + s[p23]) >> 2);  // green avg 4
      d[r11] = (UC8)((s[p11] + s[p31] + s[p13] + s[p33]) >> 2);  // red avg 4
    }

    // black on right (2 lines)
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    d[dln]     = 0;
    d[dln + 1] = 0;
    d[dln + 2] = 0;
    d += 3;
  }

  // black top row
  for (x = w; x > 0; x--, d += 3)
  {
    d[0] = 0; 
    d[1] = 0; 
    d[2] = 0; 
  }
  return 1;
}


//= Converts a Bayer patterned monochrome image with GB in corner (not BG) to a normal RGB color image.
// pixels on boundary of image are written to black always

int jhcColor::DeBayer_GB (jhcImg& dest, const jhcImg& src) const
{
  if (src.Valid(2))
    return DeBayer16_GB(dest, src);
  if (!src.Valid(1) || !src.SameSize(dest, 3))
    return Fatal("Bad images to jhcColor::DeBayer_GB");
  dest.CopyRoi(src);

  // always does whole image irrespective of ROI
  int x, y, w = dest.XDim(), wm2 = w - 2, hm2 = dest.YDim() - 2;
  int dln = dest.Line(), sln = src.Line(), sln2 = sln << 1, sln3 = sln2 + sln;
  int dsk = dest.Skip(), dsk2 = dsk + dln, ssk2 = src.Skip() + sln + 2;
  int             p10 = 1,        p20 = 2,        p30 = 3;   // p00 = 0;
  int p01 = sln,  p11 = sln  + 1, p21 = sln  + 2, p31 = sln  + 3;
  int p02 = sln2, p12 = sln2 + 1, p22 = sln2 + 2, p32 = sln2 + 3;
  int p03 = sln3, p13 = sln3 + 1, p23 = sln3 + 2;  // p33 = sln3 + 3;
  int b00 = 0, g00 = 1, r00 = 2, b01 = dln,     g01 = dln + 1, r01 = dln + 2;
  int b10 = 3, g10 = 4, r10 = 5, b11 = dln + 3, g11 = dln + 4, r11 = dln + 5;
  UC8 *d = dest.PxlDest();
  const UC8 *s = src.PxlSrc();

  // black bottom row
  for (x = w; x > 0; x--, d += 3)
  {
    d[0] = 0; 
    d[1] = 0; 
    d[2] = 0; 
  }
  d += dsk;

  // interior of image 
  for (y = hm2; y > 0; y -= 2, d += dsk2, s += ssk2)
  {
    // black on left (2 lines)
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    d[dln]     = 0;
    d[dln + 1] = 0;
    d[dln + 2] = 0;
    d += 3;

    for (x = wm2; x > 0; x -= 2, d += 6, s += 2)
    {
      // over a G pixel
      d[b00] = (UC8)((s[p10] + s[p12]) >> 1);                    // blue avg 2
      d[g00] = (UC8)  s[p11];                                    // green copy
      d[r00] = (UC8)((s[p01] + s[p21]) >> 1);                    // red avg 2

      // over an R pixel
      d[b10] = (UC8)((s[p10] + s[p30] + s[p12] + s[p32]) >> 2);  // blue avg 4
      d[g10] = (UC8)((s[p20] + s[p11] + s[p31] + s[p22]) >> 2);  // green avg 4
      d[r10] = (UC8)  s[p21];                                    // red copy

      // over a B pixel
      d[b01] = (UC8)  s[p12];                                    // blue copy
      d[g01] = (UC8)((s[p11] + s[p02] + s[p22] + s[p13]) >> 2);  // green avg 4
      d[r01] = (UC8)((s[p01] + s[p21] + s[p03] + s[p23]) >> 2);  // red avg 4

      // over another G pixel
      d[b11] = (UC8)((s[p12] + s[p32]) >> 1);                    // blue avg 2
      d[g11] = (UC8)  s[p22];                                    // green copy
      d[r11] = (UC8)((s[p21] + s[p23]) >> 1);                    // red avg 2
    }

    // black on right (2 lines)
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    d[dln]     = 0;
    d[dln + 1] = 0;
    d[dln + 2] = 0;
    d += 3;
  }

  // black top row
  for (x = w; x > 0; x--, d += 3)
  {
    d[0] = 0; 
    d[1] = 0; 
    d[2] = 0; 
  }
  return 1;
}


//= Converts a Bayer patterned 16 bit image to a normal RGB color image.
// could conceivably get 2 more bits of dynamic range by not down shifting

int jhcColor::DeBayer16 (jhcImg& dest, const jhcImg& src, int off, int sh) const
{
  if (!src.Valid(2) || !src.SameSize(dest, 3))
    return Fatal("Bad images to jhcColor::DeBayer16");
  dest.CopyRoi(src);

  // always does whole image irrespective of ROI
  int x, y, r, g, b, w = dest.XDim(), wm2 = w - 2, hm2 = dest.YDim() - 2;
  int dln = dest.Line(), sln = src.Line() >> 1, sln2 = sln << 1, sln3 = sln2 + sln;
  int dsk = dest.Skip(), dsk2 = dsk + dln, ssk2 = (src.Skip() >> 1) + sln + 2;
  int p00 = 0,    p10 = 1,        p20 = 2;   //   p30 = 3;
  int p01 = sln,  p11 = sln  + 1, p21 = sln  + 2, p31 = sln  + 3;
  int p02 = sln2, p12 = sln2 + 1, p22 = sln2 + 2, p32 = sln2 + 3;
  int             p13 = sln3 + 1, p23 = sln3 + 2, p33 = sln3 + 3;  // p03 = sln3;
  int b00 = 0, g00 = 1, r00 = 2, b01 = dln,     g01 = dln + 1, r01 = dln + 2;
  int b10 = 3, g10 = 4, r10 = 5, b11 = dln + 3, g11 = dln + 4, r11 = dln + 5;
  UC8 *d = dest.PxlDest();
  const US16 *s = (const US16 *) src.PxlSrc();

  // black bottom row
  for (x = w; x > 0; x--, d += 3)
  {
    d[0] = 0; 
    d[1] = 0; 
    d[2] = 0; 
  }
  d += dsk;

  // interior of image 
  for (y = hm2; y > 0; y -= 2, d += dsk2, s += ssk2)
  {
    // black on left (2 lines)
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    d[dln]     = 0;
    d[dln + 1] = 0;
    d[dln + 2] = 0;
    d += 3;

    for (x = wm2; x > 0; x -= 2, d += 6, s += 2)
    {
      // over an R pixel
      b = (s[p00] + s[p20] + s[p02] + s[p22]) >> 2;  // blue avg 4
      g = (s[p10] + s[p01] + s[p21] + s[p12]) >> 2;  // green avg 4
      r =  s[p11];                                   // red copy
      d[b00] = BOUND((b - off) >> sh);
      d[g00] = BOUND((g - off) >> sh);
      d[r00] = BOUND((r - off) >> sh);

      // over a G pixel
      b = (s[p20] + s[p22]) >> 1;                    // blue avg 2
      g =  s[p21];                                   // green copy
      r = (s[p11] + s[p31]) >> 1;                    // red avg 2
      d[b10] = BOUND((b - off) >> sh);
      d[g10] = BOUND((g - off) >> sh);
      d[r10] = BOUND((r - off) >> sh);

      // over another G pixel
      b = (s[p02] + s[p22]) >> 1;                    // blue avg 2
      g =  s[p12];                                   // green copy
      r = (s[p11] + s[p13]) >> 1;                    // red avg 2
      d[b01] = BOUND((b - off) >> sh);
      d[g01] = BOUND((g - off) >> sh);
      d[r01] = BOUND((r - off) >> sh);

      // over a B pixel
      b =  s[p22];                                   // blue copy
      g = (s[p21] + s[p12] + s[p32] + s[p23]) >> 2;  // green avg 4
      r = (s[p11] + s[p31] + s[p13] + s[p33]) >> 2;  // red avg 4
      d[b11] = BOUND((b - off) >> sh);
      d[g11] = BOUND((g - off) >> sh);
      d[r11] = BOUND((r - off) >> sh);
    }

    // black on right (2 lines)
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    d[dln]     = 0;
    d[dln + 1] = 0;
    d[dln + 2] = 0;
    d += 3;
  }

  // black top row
  for (x = w; x > 0; x--, d += 3)
  {
    d[0] = 0; 
    d[1] = 0; 
    d[2] = 0; 
  }
  return 1;
}


//= Converts a Bayer patterned 16 bit image with GB in corner (not BG) to a normal RGB color image.
// could conceivably get 2 more bits of dynamic range by not down shifting

int jhcColor::DeBayer16_GB (jhcImg& dest, const jhcImg& src, int off, int sh) const
{
  if (!src.Valid(2) || !src.SameSize(dest, 3))
    return Fatal("Bad images to jhcColor::DeBayer16_GB");
  dest.CopyRoi(src);

  // always does whole image irrespective of ROI
  int x, y, r, g, b, w = dest.XDim(), wm2 = w - 2, hm2 = dest.YDim() - 2;
  int dln = dest.Line(), sln = src.Line() >> 1, sln2 = sln << 1, sln3 = sln2 + sln;
  int dsk = dest.Skip(), dsk2 = dsk + dln, ssk2 = (src.Skip() >> 1) + sln + 2;
  int             p10 = 1,        p20 = 2,        p30 = 3;   // p00 = 0;
  int p01 = sln,  p11 = sln  + 1, p21 = sln  + 2, p31 = sln  + 3;
  int p02 = sln2, p12 = sln2 + 1, p22 = sln2 + 2, p32 = sln2 + 3;
  int p03 = sln3, p13 = sln3 + 1, p23 = sln3 + 2;  // p33 = sln3 + 3;
  int b00 = 0, g00 = 1, r00 = 2, b01 = dln,     g01 = dln + 1, r01 = dln + 2;
  int b10 = 3, g10 = 4, r10 = 5, b11 = dln + 3, g11 = dln + 4, r11 = dln + 5;
  UC8 *d = dest.PxlDest();
  const US16 *s = (const US16 *) src.PxlSrc();

  // black bottom row
  for (x = w; x > 0; x--, d += 3)
  {
    d[0] = 0; 
    d[1] = 0; 
    d[2] = 0; 
  }
  d += dsk;

  // interior of image 
  for (y = hm2; y > 0; y -= 2, d += dsk2, s += ssk2)
  {
    // black on left (2 lines)
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    d[dln]     = 0;
    d[dln + 1] = 0;
    d[dln + 2] = 0;
    d += 3;

    for (x = wm2; x > 0; x -= 2, d += 6, s += 2)
    {
      // over a G pixel
      b = (s[p10] + s[p12]) >> 1;                    // blue avg 2
      g =  s[p11];                                   // green copy
      r = (s[p01] + s[p21]) >> 1;                    // red avg 2
      d[b00] = BOUND((b - off) >> sh);
      d[g00] = BOUND((g - off) >> sh);
      d[r00] = BOUND((r - off) >> sh);

      // over an R pixel
      b = (s[p10] + s[p30] + s[p12] + s[p32]) >> 2;  // blue avg 4
      g = (s[p20] + s[p11] + s[p31] + s[p22]) >> 2;  // green avg 4
      r =  s[p21];                                   // red copy
      d[b10] = BOUND((b - off) >> sh);
      d[g10] = BOUND((g - off) >> sh);
      d[r10] = BOUND((r - off) >> sh);

      // over a B pixel
      b =  s[p12];                                   // blue copy
      g = (s[p11] + s[p02] + s[p22] + s[p13]) >> 2;  // green avg 4
      r = (s[p01] + s[p21] + s[p03] + s[p23]) >> 2;  // red avg 4
      d[b01] = BOUND((b - off) >> sh);
      d[g01] = BOUND((g - off) >> sh);
      d[r01] = BOUND((r - off) >> sh);

      // over another G pixel
      b = (s[p12] + s[p32]) >> 1;                    // blue avg 2
      g =  s[p22];                                   // green copy
      r = (s[p21] + s[p23]) >> 1;                    // red avg 2
      d[b11] = BOUND((b - off) >> sh);
      d[g11] = BOUND((g - off) >> sh);
      d[r11] = BOUND((r - off) >> sh);
    }

    // black on right (2 lines)
    d[0] = 0;
    d[1] = 0;
    d[2] = 0;
    d[dln]     = 0;
    d[dln + 1] = 0;
    d[dln + 2] = 0;
    d += 3;
  }

  // black top row
  for (x = w; x > 0; x--, d += 3)
  {
    d[0] = 0; 
    d[1] = 0; 
    d[2] = 0; 
  }
  return 1;
}


//= Converts a Bayer patterned 16 bit image to three 16 bit images (R + G + B).
// NORMAL pattern is assumed to be:
// <pre>
//
//   03:G  13:R  23:G  33:R
//        +----------+
//   02:B |12:G  22:B| 32:G
//        |          |
//   01:G |11:R  21:G| 31:R   <== D block corner @ 11 wrt S
//        +----------+
//   00:B  10:G  20:B  30:G   <== S @ 00 (lags 1 row and 1 column)
//
// <pre>
// uses lateral average of 2 pixels for Red @ Green and Blue @ Green 
// uses average of 4 pixels for G @ R, G @ B, R @ B, and B @ R
// pixels on boundary of image are written to black always
// could conceivably get 2 more bits of dynamic range by not down shifting

int jhcColor::DeBayer16 (jhcImg& red, jhcImg& grn, jhcImg& blu, const jhcImg& src) const
{
  if (!src.Valid(2) || !src.SameFormat(red) || !src.SameFormat(grn) || !src.SameFormat(blu))
    return Fatal("Bad images to jhcColor::DeBayer16");
  red.CopyRoi(src);
  grn.CopyRoi(src);
  blu.CopyRoi(src);

  // always does whole image irrespective of ROI
  int x, y, w = red.XDim(), wm2 = w - 2, hm2 = red.YDim() - 2;
  int dln = red.Line() >> 1, sln = src.Line() >> 1, sln2 = sln << 1, sln3 = sln2 + sln;
  int dsk = red.Skip() >> 1, dsk2 = dsk + dln, ssk2 = (src.Skip() >> 1) + sln + 2;
  int p00 = 0,    p10 = 1,        p20 = 2;   //   p30 = 3;
  int p01 = sln,  p11 = sln  + 1, p21 = sln  + 2, p31 = sln  + 3;
  int p02 = sln2, p12 = sln2 + 1, p22 = sln2 + 2, p32 = sln2 + 3;
  int             p13 = sln3 + 1, p23 = sln3 + 2, p33 = sln3 + 3;  // p03 = sln3;
  int i00 = 0, i01 = dln, i10 = 1, i11 = dln + 1; 
  US16 *rd = (US16 *) red.PxlDest(), *gd = (US16 *) grn.PxlDest(), *bd = (US16 *) blu.PxlDest();
  const US16 *s = (const US16 *) src.PxlSrc();

  // black bottom row
  for (x = w; x > 0; x--, rd++, gd++, bd++)
  {
    *bd = 0; 
    *gd = 0; 
    *rd = 0; 
  }
  bd += dsk;
  gd += dsk;
  rd += dsk;

  // interior of image 
  for (y = hm2; y > 0; y -= 2, rd += dsk2, gd += dsk2, bd += dsk2, s += ssk2)
  {
    // black on left (2 lines)
    bd[dln] = 0;
    gd[dln] = 0;
    rd[dln] = 0;
    *bd++ = 0;
    *gd++ = 0;
    *rd++ = 0;

    for (x = wm2; x > 0; x -= 2, rd += 2, gd += 2, bd += 2, s += 2)
    {
      // over an R pixel
      bd[i00] = (s[p00] + s[p20] + s[p02] + s[p22]) >> 2;  // blue avg 4
      gd[i00] = (s[p10] + s[p01] + s[p21] + s[p12]) >> 2;  // green avg 4
      rd[i00] =  s[p11];                                   // red copy

      // over a G pixel
      bd[i10] = (s[p20] + s[p22]) >> 1;                    // blue avg 2
      gd[i10] =  s[p21];                                   // green copy
      rd[i10] = (s[p11] + s[p31]) >> 1;                    // red avg 2

      // over another G pixel
      bd[i01] = (s[p02] + s[p22]) >> 1;                    // blue avg 2
      gd[i01] =  s[p12];                                   // green copy
      rd[i01] = (s[p11] + s[p13]) >> 1;                    // red avg 2

      // over a B pixel
      bd[i11] =  s[p22];                                   // blue copy
      gd[i11] = (s[p21] + s[p12] + s[p32] + s[p23]) >> 2;  // green avg 4
      rd[i11] = (s[p11] + s[p31] + s[p13] + s[p33]) >> 2;  // red avg 4
    }

    // black on right (2 lines)
    bd[dln] = 0;
    gd[dln] = 0;
    rd[dln] = 0;
    *bd++ = 0;
    *gd++ = 0;
    *rd++ = 0;
  }

  // black top row
  for (x = w; x > 0; x--, rd++, gd++, bd++)
  {
    *bd = 0; 
    *gd = 0; 
    *rd = 0; 
  }
  return 1;
}


//= Converts a Bayer patterned 16 bit image with GB in corner (not BG) to three 16 bit images.
// pattern is assumed to be:
// <pre>
//
//   03:R  13:G  23:R  33:G
//        +----------+
//   02:G |12:B  22:G| 32:B
//        |          |
//   01:R |11:G  21:R| 31:G   <== D block corner @ 11 wrt S
//        +----------+
//   00:G  10:B  20:G  30:B   <== S @ 00 (lags 1 row and 1 column)
//
// <pre>
// uses lateral average of 2 pixels for Red @ Green and Blue @ Green 
// uses average of 4 pixels for G @ R, G @ B, R @ B, and B @ R
// pixels on boundary of image are written to black always
// could conceivably get 2 more bits of dynamic range by not down shifting

int jhcColor::DeBayer16_GB (jhcImg& red, jhcImg& grn, jhcImg& blu, const jhcImg& src) const
{
  if (!src.Valid(2) || !src.SameFormat(red) || !src.SameFormat(grn) || !src.SameFormat(blu))
    return Fatal("Bad images to jhcColor::DeBayer16_GB");
  red.CopyRoi(src);
  grn.CopyRoi(src);
  blu.CopyRoi(src);

  // always does whole image irrespective of ROI
  int x, y, w = red.XDim(), wm2 = w - 2, hm2 = red.YDim() - 2;
  int dln = red.Line() >> 1, sln = src.Line() >> 1, sln2 = sln << 1, sln3 = sln2 + sln;
  int dsk = red.Skip() >> 1, dsk2 = dsk + dln, ssk2 = (src.Skip() >> 1) + sln + 2;
  int             p10 = 1,        p20 = 2,        p30 = 3;   // p00 = 0;
  int p01 = sln,  p11 = sln  + 1, p21 = sln  + 2, p31 = sln  + 3;
  int p02 = sln2, p12 = sln2 + 1, p22 = sln2 + 2, p32 = sln2 + 3;
  int p03 = sln3, p13 = sln3 + 1, p23 = sln3 + 2;  // p33 = sln3 + 3;
  int i00 = 0, i01 = dln, i10 = 1, i11 = dln + 1; 
  US16 *rd = (US16 *) red.PxlDest(), *gd = (US16 *) grn.PxlDest(), *bd = (US16 *) blu.PxlDest();
  const US16 *s = (const US16 *) src.PxlSrc();

  // black bottom row
  for (x = w; x > 0; x--, rd++, gd++, bd++)
  {
    *bd = 0; 
    *gd = 0; 
    *rd = 0; 
  }
  bd += dsk;
  gd += dsk;
  rd += dsk;

  // interior of image 
  for (y = hm2; y > 0; y -= 2, rd += dsk2, gd += dsk2, bd += dsk2, s += ssk2)
  {
    // black on left (2 lines)
    bd[dln] = 0;
    gd[dln] = 0;
    rd[dln] = 0;
    *bd++ = 0;
    *gd++ = 0;
    *rd++ = 0;

    for (x = wm2; x > 0; x -= 2, rd += 2, gd += 2, bd += 2, s += 2)
    {
      // over a G pixel
      bd[i00] = (s[p10] + s[p12]) >> 1;                    // blue avg 2
      gd[i00] =  s[p11];                                   // green copy
      rd[i00] = (s[p01] + s[p21]) >> 1;                    // red avg 2

      // over an R pixel
      bd[i10] = (s[p10] + s[p30] + s[p12] + s[p32]) >> 2;  // blue avg 4
      gd[i10] = (s[p20] + s[p11] + s[p31] + s[p22]) >> 2;  // green avg 4
      rd[i10] =  s[p21];                                   // red copy

      // over a B pixel
      bd[i01] =  s[p12];                                   // blue copy
      gd[i01] = (s[p11] + s[p02] + s[p22] + s[p13]) >> 2;  // green avg 4
      rd[i01] = (s[p01] + s[p21] + s[p03] + s[p23]) >> 2;  // red avg 4

      // over another G pixel
      bd[i11] = (s[p12] + s[p32]) >> 1;                    // blue avg 2
      gd[i11] =  s[p22];                                   // green copy
      rd[i11] = (s[p21] + s[p23]) >> 1;                    // red avg 2
    }

    // black on right (2 lines)
    bd[dln] = 0;
    gd[dln] = 0;
    rd[dln] = 0;
    *bd++ = 0;
    *gd++ = 0;
    *rd++ = 0;
  }

  // black top row
  for (x = w; x > 0; x--, rd++, gd++, bd++)
  {
    *bd = 0; 
    *gd = 0; 
    *rd = 0; 
  }
  return 1;
}


//= Converts a Bayer patterned 16 bit image with GR in corner (not BG) to three 16 bit images.
// pattern is assumed to be:
// <pre>
//
//   03:B  13:G  23:B  33:G
//        +----------+
//   02:G |12:R  22:G| 32:R
//        |          |
//   01:B |11:G  21:B| 31:G   <== D block corner @ 11 wrt S
//        +----------+
//   00:G  10:R  20:G  30:R   <== S @ 00 (lags 1 row and 1 column)
//
// <pre>
// uses lateral average of 2 pixels for Red @ Green and Blue @ Green 
// uses average of 4 pixels for G @ R, G @ B, R @ B, and B @ R
// pixels on boundary of image are written to black always
// could conceivably get 2 more bits of dynamic range by not down shifting

int jhcColor::DeBayer16_GR (jhcImg& red, jhcImg& grn, jhcImg& blu, const jhcImg& src) const
{
  if (!src.Valid(2) || !src.SameFormat(red) || !src.SameFormat(grn) || !src.SameFormat(blu))
    return Fatal("Bad images to jhcColor::DeBayer16_GR");
  red.CopyRoi(src);
  grn.CopyRoi(src);
  blu.CopyRoi(src);

  // always does whole image irrespective of ROI
  int x, y, w = red.XDim(), wm2 = w - 2, hm2 = red.YDim() - 2;
  int dln = red.Line() >> 1, sln = src.Line() >> 1, sln2 = sln << 1, sln3 = sln2 + sln;
  int dsk = red.Skip() >> 1, dsk2 = dsk + dln, ssk2 = (src.Skip() >> 1) + sln + 2;
  int             p10 = 1,        p20 = 2,        p30 = 3;   // p00 = 0;
  int p01 = sln,  p11 = sln  + 1, p21 = sln  + 2, p31 = sln  + 3;
  int p02 = sln2, p12 = sln2 + 1, p22 = sln2 + 2, p32 = sln2 + 3;
  int p03 = sln3, p13 = sln3 + 1, p23 = sln3 + 2;  // p33 = sln3 + 3;
  int i00 = 0, i01 = dln, i10 = 1, i11 = dln + 1; 
  US16 *rd = (US16 *) red.PxlDest(), *gd = (US16 *) grn.PxlDest(), *bd = (US16 *) blu.PxlDest();
  const US16 *s = (const US16 *) src.PxlSrc();

  // black bottom row
  for (x = w; x > 0; x--, rd++, gd++, bd++)
  {
    *bd = 0; 
    *gd = 0; 
    *rd = 0; 
  }
  bd += dsk;
  gd += dsk;
  rd += dsk;

  // interior of image 
  for (y = hm2; y > 0; y -= 2, rd += dsk2, gd += dsk2, bd += dsk2, s += ssk2)
  {
    // black on left (2 lines)
    bd[dln] = 0;
    gd[dln] = 0;
    rd[dln] = 0;
    *bd++ = 0;
    *gd++ = 0;
    *rd++ = 0;

    for (x = wm2; x > 0; x -= 2, rd += 2, gd += 2, bd += 2, s += 2)
    {
      // over a G pixel
      bd[i00] = (s[p01] + s[p21]) >> 1;                    // blue avg 2
      gd[i00] =  s[p11];                                   // green copy
      rd[i00] = (s[p10] + s[p12]) >> 1;                    // red avg 2

      // over a B pixel
      bd[i10] =  s[p21];                                   // blue copy
      gd[i10] = (s[p20] + s[p11] + s[p31] + s[p22]) >> 2;  // green avg 4
      rd[i10] = (s[p10] + s[p30] + s[p12] + s[p32]) >> 2;  // red avg 4

      // over an R pixel
      bd[i01] = (s[p01] + s[p21] + s[p03] + s[p23]) >> 2;  // blue avg 4
      gd[i01] = (s[p11] + s[p02] + s[p22] + s[p13]) >> 2;  // green avg 4
      rd[i01] =  s[p12];                                   // red copy

      // over another G pixel
      bd[i11] = (s[p21] + s[p23]) >> 1;                    // blue avg 2
      gd[i11] =  s[p22];                                   // green copy
      rd[i11] = (s[p12] + s[p32]) >> 1;                    // red avg 2
    }

    // black on right (2 lines)
    bd[dln] = 0;
    gd[dln] = 0;
    rd[dln] = 0;
    *bd++ = 0;
    *gd++ = 0;
    *rd++ = 0;
  }

  // black top row
  for (x = w; x > 0; x--, rd++, gd++, bd++)
  {
    *bd = 0; 
    *gd = 0; 
    *rd = 0; 
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Debugging Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Generate an image showing color at various angles for standard HSI color space.
// R-Y:G-B space, automatically resizes output image to (511 511) x 3

void jhcColor::HueMap (jhcImg& map) const
{
  UC8 *m;
  double irad3 = 2.0 / sqrt(3.0);
  int x, y, g0, b0, r, g, b, adj, sk, lum = 130;

  // set size and default to black (also does axes)
  map.SetSize(511, 511, 3);
  map.FillArr(0);

  // scan each line of image
  m = map.PxlDest();
  sk = map.Skip();
  for (y = -255; y <= 255; y++, m += sk)
  {
    // leave x axis black
    if (y == 0)
    {
      m += 1533;                       // 3 * 511
      continue;
    }

    // y = 0.5 * rad3 * (g - b): find smallest g, b values to achieve y
    g0 = 0;
    b0 = 0;
    if (y >= 0)
      g0 = ROUND(y * irad3);
    else
      b0 = ROUND(-y * irad3);

    // scan each row of image
    for (x = -255; x <= 255; x++, m += 3) 
    {
      // leave y axis black
      if (x == 0)
        continue;

      // x = r - 0.5 * (g + b): find smallest r value to achieve x
      r = x + (g0 + b0) / 2;
      g = g0;
      b = b0;
      if (r < 0)
      {
        g -= r;
        b -= r;
        r = 0;
      }

      // check if out of range else move to standard intensity
      if ((r > 255) || (g > 255) || (b > 255))
        continue;
      adj = lum - (r + g + b) / 3;
      if (adj < 0)
        adj = 0;
      else
        adj = __min(adj, __min(255 - r, __min(255 - g, 255 - b)));
      m[0] = (UC8)(b + adj);
      m[1] = (UC8)(g + adj);
      m[2] = (UC8)(r + adj);
    }
  }
}  


//= Generate an image showing color at various angles for opponent color space.
// R-G:Y-B space, automatically resizes output image to (511 511) x 3
// identical to HueMap except 30 degrees rotated

void jhcColor::OppMap (jhcImg& map) const
{
  UC8 *m, *m0;
  double irad3 = 2.0 / sqrt(3.0);
  int x, y, r0, g0, r, g, b, adj, ln, lum = 130;

  // set size and default to black 
  map.SetSize(511, 511, 3);
  map.FillArr(0);

  // scan each row of image
  m0 = map.PxlDest();
  ln = map.Line();
  for (x = -255; x <= 255; x++, m0 += 3)
  {
    // leave y axis black
    if (x == 0)
      continue;

    // x = 0.5 * sqrt(3)(r - g): find smallest r, g values to achieve x
    r0 = 0;
    g0 = 0;
    if (x >= 0)
      r0 = ROUND(x * irad3);
    else
      g0 = ROUND(-x * irad3);
    if ((r0 < 0) || (g0 < 0))
      continue;

    // scan each column of image
    m = m0;
    for (y = -255; y <= 255; y++, m += ln) 
    {
      // leave x axis black
      if (y == 0)
        continue;

      // y = 0.5 * (r + g) - b: find smallest b value to achieve y
      r = r0;
      g = g0;
      b = (r0 + g0) / 2 - y;
      if (b < 0)
      {
        r -= b;
        g -= b;
        b = 0;
      }

      // check if out of range else move to standard intensity
      if ((r > 255) || (g > 255) || (b > 255))
        continue;
      adj = lum - (r + g + b) / 3;
      if (adj < 0)
        adj = 0;
      else
        adj = __min(adj, __min(255 - r, __min(255 - g, 255 - b)));
      m[0] = (UC8)(b + adj);
      m[1] = (UC8)(g + adj);
      m[2] = (UC8)(r + adj);
    }
  }
}


//= Generate an image showing color at various angles for excess green color space.
// R-G:G-B space, automatically resizes output image to (511 511) x 3
// stretches area assigned to orange, better than hexagonal P-G:R-B space

void jhcColor::GexMap (jhcImg& map) const
{
  UC8 *m;
  int x, y, g0, b0, r, g, b, adj, sk, lum = 130;

  // set size and default to black 
  map.SetSize(511, 511, 3);
  map.FillArr(0);

  // scan each line of image
  m = map.PxlDest();
  sk = map.Skip();
  for (y = -255; y <= 255; y++, m += sk)
  {
    // leave x axis black
    if (y == 0)
    {
      m += 1533;                       // 3 * 511
      continue;
    }

    // y = g - b: find smallest g, b values to achieve y 
    g0 = 0;
    b0 = 0;
    if (y >= 0)
      g0 = y;
    else
      b0 = -y;

    // scan each row of image
    for (x = -255; x <= 255; x++, m += 3) 
    {
      // leave y axis black
      if (x == 0)
        continue;

      // x = r - g: find smallest r value to achieve x
      r = x + g0;
      g = g0;
      b = b0;
      if (r < 0)
      {
        g -= r;
        b -= r;
        r = 0;
      }

      // check if out of range else move to standard intensity
      if ((r > 255) || (g > 255) || (b > 255))
        continue;
      adj = lum - (r + g + b) / 3;
      if (adj < 0)
        adj = 0;
      else
        adj = __min(adj, __min(255 - r, __min(255 - g, 255 - b)));
      m[0] = (UC8)(b + adj);
      m[1] = (UC8)(g + adj);
      m[2] = (UC8)(r + adj);
    }
  }
}

