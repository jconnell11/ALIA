// jhcVect.cpp : routines operating on fields within a single image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2016 IBM Corporation
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

#include "Processing/jhcVect.h"


///////////////////////////////////////////////////////////////////////////
//                          General Functions                            //
///////////////////////////////////////////////////////////////////////////

//= For each pixel takes the saturated sum of values in all fields.

int jhcVect::SumAll (jhcImg& dest, const jhcImg& src, double sc) const
{
  return AvgAll(dest, src, sc * src.Fields());
}


//= For each pixel takes the average of values in all fields.
// can multiply by a small factor (e.g. number of fields yields sum)

int jhcVect::AvgAll (jhcImg& dest, const jhcImg& src, double sc) const
{
  if ((dest.Fields() != 1) || !dest.SameSize(src))
    return Fatal("Bad images to jhcVect::AvgAll");
  if (sc < 0.0)
    return 0;
  dest.CopyRoi(src);

  // special cases
  if (src.Fields() == 1)
    return dest.CopyArr(src);
  if (src.Fields() == 3)
    return AvgAll_3(dest, src, sc);

  // general ROI case (seldom used)
  int i, x, y, f;
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = src.Fields();
  int sum = 128, inc = ROUND(256.0 * sc / nf);
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  int scaled[256];
    
  // figure out scaling so final divide becomes a shift
// this can have significant rounding errors!
  for (i = 0; i <= 255; i++)
  {
    scaled[i] = sum;
    sum += inc;
  }

  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      sum = 128;
      for (f = nf; f > 0; f--)
        sum += scaled[*s++];
      sum >>= 8;
      if (sum >= 255)
        *d++ = 255;
      else
        *d++ = (UC8) sum;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Same as AvgAll but for images with exactly 3 fields (e.g. RGB color).

int jhcVect::AvgAll_3 (jhcImg& dest, const jhcImg& src, double sc) const
{
  int i, v, x, y;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int sum = 128, inc = ROUND(256.0 * sc / 3.0);
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  UC8 third[768];
      
  // figure out one third of all possible sums
// this can have significant rounding errors!
  for (i = 0; i < 768; i++)
  {
    v = sum >> 8;
    third[i] = BOUND(v);
    sum += inc;
  }

  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      *d++ = third[s[0] + s[1] + s[2]];
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= For each pixel takes the maximum of values in all fields.

int jhcVect::MaxAll (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(1) || !dest.SameSize(src))
    return Fatal("Bad images to jhcVect::MaxAll");
  dest.CopyRoi(src);

  // special cases
  if (src.Fields() == 1)
    return dest.CopyArr(src);
  if (src.Fields() == 3)
    return MaxAll_3(dest, src);

  // general ROI case (seldom used)
  int x, y, f;
  int rw = dest.RoiW(), rh = dest.RoiH(), nfm1 = src.Fields() - 1;
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  UC8 win;
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      win = *s++;
      for (f = nfm1; f > 0; f--)
      {
        if (*s > win)
          win = *s;
        s++;
      }
      *d++ = win; 
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Same as MaxAll but for images with exactly 3 fields.

int jhcVect::MaxAll_3 (jhcImg& dest, const jhcImg& src) const
{
  int x, y;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  UC8 win;
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      win = s[0];
      if (s[1] > win)
        win = s[1];
      if (s[2] > win)
        win = s[2];
      *d++ = win;
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= For each pixel takes the maximum of values in all fields.

int jhcVect::MinAll (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(1) || !dest.SameSize(src))
    return Fatal("Bad images to jhcVect::MinAll");
  dest.CopyRoi(src);

  // special cases
  if (src.Fields() == 1)
    return dest.CopyArr(src);
  if (src.Fields() == 3)
    return MinAll_3(dest, src);

  // general ROI case (seldom used)
  int x, y, f;
  int rw = dest.RoiW(), rh = dest.RoiH(), nfm1 = src.Fields() - 1;
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  UC8 win;
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      win = *s++;
      for (f = nfm1; f > 0; f--)
      {
        if (*s < win)
          win = *s;
        s++;
      }
      *d++ = win; 
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Same as MinAll but for images with exactly 3 fields.

int jhcVect::MinAll_3 (jhcImg& dest, const jhcImg& src) const
{
  int x, y;
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  UC8 win;
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      win = s[0];
      if (s[1] < win)
        win = s[1];
      if (s[2] < win)
        win = s[2];
      *d++ = win;
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= For each pixel takes the median of values in all fields.
// only works for RGB images currently

int jhcVect::MedianAll (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcVect::MedianAll");
  dest.CopyRoi(src);

  // local variables
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  UC8 win;
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if ((s[1] <= s[0]) && (s[0] <= s[2]))
        win = s[0];
      else if ((s[0] <= s[1]) && (s[1] <= s[2]))
        win = s[1];
      else
        win = s[2];
      *d++ = win;
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Takes the average of the two largest values in all fields.
// only works for RGB images currently

int jhcVect::HiAvgAll (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcVect::HiAvgAll");
  dest.CopyRoi(src);

  // local variables
  int x, y, win, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if ((s[0] <= s[1]) && (s[0] <= s[2]))
        win = (s[1] + s[2]) >> 1;
      else if ((s[1] <= s[0]) && (s[1] <= s[2]))
        win = (s[0] + s[2]) >> 1;
      else
        win = (s[1] + s[2]) >> 1;
      *d++ = (UC8) win;
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Takes the average of the two smallest values in all fields.
// only works for RGB images currently

int jhcVect::LoAvgAll (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcVect::LoAvgAll");
  dest.CopyRoi(src);

  // local variables
  int x, y, win, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if ((s[0] >= s[1]) && (s[0] >= s[2]))
        win = (s[1] + s[2]) >> 1;
      else if ((s[1] >= s[0]) && (s[1] >= s[2]))
        win = (s[0] + s[2]) >> 1;
      else
        win = (s[1] + s[2]) >> 1;
      *d++ = (UC8) win;
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Checks that each pixel falls within specified range.

int jhcVect::AllWithin (jhcImg& dest, const jhcImg& src, int lo, int hi) const
{
  if (!dest.Valid(1) || !dest.SameSize(src))
    return Fatal("Bad images to jhcVect::AllWithin");
  dest.CopyRoi(src);

  // special cases
  if (src.Fields() == 3)
    return AllWithin_3(dest, src, lo, hi);

  // general ROI case (seldom used)
  int x, y, i;
  int rw = dest.RoiW(), rh = dest.RoiH(), nf = src.Fields();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 ans, bot = BOUND(lo), top = BOUND(hi);
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      ans = 255;
      for (i = 0; i < nf; i++)
        if ((s[i] < bot) || (s[i] > top))
        {
          ans = 0;
          break;
        }

      *d++ = ans;
      s += nf;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Same as AllWithin but for images with exactly 3 fields.

int jhcVect::AllWithin_3 (jhcImg& dest, const jhcImg& src, int lo, int hi) const
{
  int x, y;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  UC8 bot = BOUND(lo), top = BOUND(hi);
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if ((s[0] < bot) || (s[0] > top) || 
          (s[1] < bot) || (s[1] > top) || 
          (s[2] < bot) || (s[2] > top))
        *d++ = 0;
      else
        *d++ = 255;
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                      Color Image Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Set dest to 255 everywhere R, G, B components are all between limits.

int jhcVect::ValidRGB (jhcImg& dest, const jhcImg& src, int lo, int hi) const
{
  if (!src.Valid(3) || !src.SameSize(dest, 1))
    return Fatal("Bad images to jhcVect::ValidRGB");
  dest.CopyRoi(src);

  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s += 3)
      if ((s[0] >= lo) && (s[0] <= hi) && 
          (s[1] >= lo) && (s[1] <= hi) && 
          (s[2] >= lo) && (s[2] <= hi))
        *d = 255;
      else
        *d = 0;
  return 1;
}


//= Sets dest to 255 when either R, G, or B is above given threshold.

int jhcVect::AnyOverRGB (jhcImg& dest, const jhcImg& src, int rth, int gth, int bth) const
{
  if (!src.Valid(3) || !src.SameSize(dest, 1))
    return Fatal("Bad images to jhcVect::AnyOverRGB");
  dest.CopyRoi(src);

  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if ((s[0] > bth) || (s[1] > gth) || (s[2] > rth))
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


//= Adds 85 to dest for each R, G, or B above given threshold.

int jhcVect::FieldsOverRGB (jhcImg& dest, const jhcImg& src, int rth, int gth, int bth) const
{
  if (!src.Valid(3) || !src.SameSize(dest, 1))
    return Fatal("Bad images to jhcVect::FieldsOverRGB");
  dest.CopyRoi(src);

  int x, y, v, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      v = 0;
      if (s[0] > bth)
        v += 85;
      if (s[1] > gth) 
        v += 85;
      if (s[2] > rth)
        v += 85;
      *d++ = (UC8) v;
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Combine three fields using various weights.

int jhcVect::WtdSumRGB (jhcImg& dest, const jhcImg& src, double rsc, double gsc, double bsc) const
{
  if (!src.Valid(3) || !src.SameSize(dest, 1))
    return Fatal("Bad images to jhcVect::WtdSumRGB");
  dest.CopyRoi(src);

  // local variables
  int i, x, y, v, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const UC8 *s = src.PxlSrc() + src.RoiOff();
  UL32 rinc, ginc, binc, rsum = 0, gsum = 0, bsum = 0;
  UL32 rval[256], gval[256], bval[256];

  // precompute scaling tables
  rinc = (UL32)(rsc * 65536.0 + 0.5);
  ginc = (UL32)(gsc * 65536.0 + 0.5);
  binc = (UL32)(bsc * 65536.0 + 0.5);
  for (i = 0; i < 256; i++)
  {
    bval[i] = bsum;
    gval[i] = gsum;
    rval[i] = rsum;
    bsum += binc;
    gsum += ginc;
    rsum += rinc;
  }
  
  // combine all fields for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      v = (bval[s[0]] + gval[s[1]] + rval[s[2]] + 32768) >> 16;
      *d++ = BOUND(v);
      s += 3;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= For each pixel find maximum across channels of absolute deviation from given values.

int jhcVect::MaxDevRGB (jhcImg& dest, const jhcImg& src, int rval, int gval, int bval) const
{
  if (!src.Valid(3) || !src.SameSize(dest, 1))
    return Fatal("Bad images to jhcVect::MaxDevRGB");
  dest.CopyRoi(src);

  // local variables
  int x, y, dev, big, rw = src.RoiW(), rh = src.RoiH(), ssk = src.Skip(), dsk = dest.Skip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s += 3)
    {
      big = abs(s[0] - bval);
      dev = abs(s[1] - gval);
      big = __max(big, dev);
      dev = abs(s[2] - rval);
      big = __max(big, dev);
      *d = (UC8) big;
    }      
  return 1;
}

