// jhcGray.cpp : ways to generate a black-and-white image from a color one
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2018 IBM Corporation
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

#include "Processing/jhcGray.h"


///////////////////////////////////////////////////////////////////////////
//                             Initialization                            //
///////////////////////////////////////////////////////////////////////////

// Done this way to make only one copy and keep it off the stack.
// Static member variables must be initialized at file scope.

int jhcGray::instances = 0;

UC8 *jhcGray::third = NULL;
UC8 *jhcGray::blut = NULL, *jhcGray::glut = NULL, *jhcGray::rlut = NULL;


//= Default constructor sets up some tables.

jhcGray::jhcGray ()
{
  int i;

  // see if initialization already done by another instance
  if (instances++ > 0)
    return;

  // allocate space for all the tables
  third = new UC8 [768];
  blut = new UC8 [256];
  glut = new UC8 [256];
  rlut = new UC8 [256];
  if ((third == NULL) || (blut == NULL) || (glut == NULL) || (rlut == NULL))
    Fatal("Could not allocate table in jhcGray");

  // compute one third of all possible sums (AvgRGB)
  for (i = 0; i <= 765; i++)
    third[i] = (UC8)((i + 1) / 3);

  // compute weighting tables for RGB to Intensity
  for (i = 0; i <= 255; i++)
  {
    rlut[i] = BOUND(ROUND(0.30 * i));
    glut[i] = BOUND(ROUND(0.59 * i));
    blut[i] = BOUND(ROUND(0.11 * i));
  }
}


//= Cleans up allocated tables if last instance in existence.

jhcGray::~jhcGray ()
{
  if (--instances > 0)
    return;

  // free all arrays
  if (rlut != NULL)
    delete [] rlut;
  if (glut != NULL)
    delete [] glut;
  if (blut != NULL)
    delete [] blut;
  if (third != NULL)
    delete [] third;
  
  // reinitialize
  instances = 0;
  third = NULL;
  blut  = NULL;
  glut  = NULL;
  rlut  = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                          External Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Uses monochrome method suggested.
//   1 = RGB avg, 2 = weighted intensity, 3 = just green, 4 = just red, 5 = just blue

int jhcGray::ForceMono (jhcImg& dest, const jhcImg& src, int style) 
{
  // check for RGB image
  if (!dest.Valid(1, 3) || !src.SameSize(dest))
    return Fatal("Bad images to jhcGray::ForceMono");
  if (dest.Fields() == 3)
    return Mono3(dest, src, style);

  // normal monochrome case
  if (src.Fields() == 1)
    return dest.CopyArr(src);
  if (style == 2)
    return Intensity(dest, src);
  if (style == 3)
    return dest.CopyField(src, 1);
  if (style == 4)
    return dest.CopyField(src, 2);
  if (style == 5)
    return dest.CopyField(src, 0);
  return MonoAvg(dest, src);
}


//= Takes a true color image and makes a 3 field monochrome image with R=G=B.
//   1 = RGB average, 2 = weighted intensity, 3 = just green field

int jhcGray::Mono3 (jhcImg& dest, const jhcImg& src, int style) 
{
  if (!dest.Valid(3) || !src.SameFormat(dest))
    return Fatal("Bad images to jhcGray::Mono3");

  mbase.SetSize(src, 1);
  ForceMono(mbase, src, style);
  dest.CopyField(mbase, 0, 2);
  dest.CopyField(mbase, 0, 1);
  dest.CopyField(mbase, 0, 0);
  return 1;
}


//= Fills destination with (R + G + B) / 3 rounded.

int jhcGray::MonoAvg (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcGray::MonoAvg");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // apply "global" lookup table to image (sum = b + g + r)
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s += 3)
      *d = third[s[0] + s[1] + s[2]];
  return 1;
}


//= Fills 16 bit destination with (R + G + B) / 3 rounded.
// takes 16 bit input with separated color plance

int jhcGray::MonoAvg16 (jhcImg& dest, const jhcImg& red, const jhcImg& grn, const jhcImg& blu) const
{
  if (!dest.Valid(2) || !dest.SameFormat(red) || !dest.SameFormat(grn) || !dest.SameFormat(blu))
    return Fatal("Bad images to jhcGray::MonoAvg16");
  dest.CopyRoi(red);
  dest.MergeRoi(grn);
  dest.MergeRoi(blu);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip() >> 1, ssk = red.RoiSkip(dest) >> 1;
  US16 *d = (US16 *) dest.RoiDest();
  const US16 *r = (const US16 *) red.RoiSrc(dest);
  const US16 *g = (const US16 *) grn.RoiSrc(dest), *b = (const US16 *) blu.RoiSrc(dest);
  
  // compute averages
  for (y = rh; y > 0; y--, d += dsk, r += ssk, g += ssk, b += ssk)
    for (x = rw; x > 0; x--, d++, r++, g++, b++)
      *d = ((int)(*r) + *g + *b) / 3;
  return 1;
}


//= Destination gets average of just red and green fields (blue often noisy).

int jhcGray::MonoRG (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcGray::MonoRG");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  
  // apply "global" lookup table to image (sum = b + g + r)
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      *d++ = (UC8)((s[1] + s[2]) >> 1);
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Sample pixels of a large color image to make a smaller monochrome image.
// uses average of RGB, only does integer resampling, always does full ROI

int jhcGray::MonoSamp (jhcImg& dest, const jhcImg& src) const
{
  int x, y, w = dest.XDim(), h = dest.YDim(), dsk = dest.Skip();
  int step = src.YDim() / h, st3 = 3 * step, ssk = step * src.Line() - w * st3;
  const UC8 *s = src.PxlSrc();
  UC8 *d = dest.PxlDest();

  // check for big enough color input
  if (!dest.Valid(1) || !src.Valid(3) || (step <= 0) || (src.XDim() < (w * step)))
    return Fatal("Bad images to jhcGray::MonoSamp");
  dest.FullRoi();

  // sample every "step" pixels horizontally and vertically
  for (y = h; y > 0; y--, d += dsk, s += ssk)
    for (x = w; x > 0; x--, d++, s += st3)
      *d = third[s[0] + s[1] + s[2]];
  return 1;
}


//= Destination gets psycho-physically weighted sum of red, green, and blue.
//   I = 0.30 * r + 0.59 * g + 0.11 * b

int jhcGray::Intensity (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcGray::Intensity");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, v, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // apply "global" lookup table to image (sum = b + g + r)
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      v = blut[s[0]] + glut[s[1]] + rlut[s[2]];
      *d++ = (UC8) __min(v, 255);
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Faster method (on some machines) to compute grayscale intensity.
// coefs [5 9 2] / 16 = [0.3125 0.5625 0.1250] vs [0.30 0.59 0.11]
// about 30% faster on Pentium 4 than jhcGray::Intensity 

int jhcGray::PseudoInt (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcGray::PseudoInt");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // convert all pixels in scan order
  for (y = rh; y > 0; y--, s += ssk, d += dsk)
    for (x = rw; x > 0; x--, s += 3, d++)
      d[0] = ((s[0] << 1) + (s[1] << 3) + s[1] + (s[2] << 2) + s[2]) >> 4;
  return 1;
}


//= Perform histogram equalization on image.
// ignores pixels below thresh when making histogram

int jhcGray::Equalize (jhcImg& dest, const jhcImg& src, int thresh) const
{
  if (src.Fields() == 3)
    return EqualizeRGB(dest, src, thresh);
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcGray::Equalize"); 
  dest.CopyRoi(src);

  // general ROI case
  int cnt[256];
  UC8 map[256];
  UC8 *d = dest.RoiDest();
  const UC8 *s;
  int x, y, i, v, all = 0;
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  double sc;
  int below = 0;
    
  // clear then build a histogram within the ROI
  for (i = 0; i < 256; i++)
    cnt[i] = 0;
  s = src.RoiSrc();
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, s++)
      if (*s >= thresh)
      {
        cnt[*s] += 1;
        all++;
      }
    s += rsk;
  }

  // build a gray scale mapping table from histogram
  sc = 255.0 / (double) all;
  for (i = 0; i < 256; i++)
  {
    below += cnt[i] / 2;
    v = ROUND(sc * below);
    map[i] = BOUND(v);
    below += (cnt[i] + 1) / 2;
  }

  // apply table to image
  s = src.RoiSrc();
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, s++, d++)
      *d = map[*s];
    s += rsk;
    d += rsk;
  }
  return 1;
}


//= Perform histogram equalization on color image.
// use average of RGB as intensity estimate
// ignores pixels below thresh when building histogram

int jhcGray::EqualizeRGB (jhcImg& dest, const jhcImg& src, int thresh) const
{
  if (!dest.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcGray::EqualizeRGB"); 
  dest.CopyRoi(src);

  // general ROI case
  int cnt[768];
  int mult[768];
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  int x, y, i, v, f, t3 = 3 * thresh, all = 0;
  double sc;
  int below = 0;
    
  // clear then build a histogram within the ROI
  for (i = 0; i < 768; i++)
    cnt[i] = 0;
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, s += 3)
    {
      v = s[0] + s[1] + s[2];
      if (v >= t3)
      {
        cnt[v] += 1;
        all++;
      }
    }
    s += rsk;
  }

  // build a normalization table from histogram
  sc = 65536.0 * 765.0 / (double) all;
  mult[0] = 0;
  below = cnt[0];
  for (i = 1; i < 768; i++)
  {
    below += cnt[i] / 2;
    mult[i] = ROUND(sc * below / (double) i);
    below += (cnt[i] + 1) / 2;
  }

  // apply table to image
  s = src.RoiSrc();
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, s += 3, d += 3)
    {
      v = s[0] + s[1] + s[2];
      f = mult[v];
      v = (f * s[0]) >> 16;
      d[0] = BOUND(v);
      v = (f * s[1]) >> 16;
      d[1] = BOUND(v);
      v = (f * s[2]) >> 16;
      d[2] = BOUND(v);
    }
    s += rsk;
    d += rsk;
  }
  return 1;
}




