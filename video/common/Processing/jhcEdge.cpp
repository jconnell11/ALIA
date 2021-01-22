// jhcEdge.cpp : some standard edge finders
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2000-2013 IBM Corporation
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

#include "Processing/jhcEdge.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////
 
// Done this way to make only one copy and keep it off the stack.
// Static member variables must be initialized at file scope.

int jhcEdge::instances = 0;

UC8 *jhcEdge::third = NULL;
US16 (*jhcEdge::arct)[256] = NULL;
US16 (*jhcEdge::root)[256] = NULL;


//= Default constructor builds some tables.

jhcEdge::jhcEdge ()
{
  double rt2 = 256.0 / sqrt(2.0), pi2 = 2.0 * 3.14159265358979;
  double val, y2, ya, sc = 65536.0 / pi2;
  int i, x, y, ival;

  // see if initialization already done by another instance
  if (instances++ > 0)
    return;
  arct  = new US16 [256][256];
  root  = new US16 [256][256];
  third = new UC8 [768];
  if ((arct == NULL) || (root == NULL) || (third == NULL))
    Fatal("Could not allocate tables in jhcEdge");

  // root takes abs(dx) and abs(dy) for magnitude
  for (y = 0; y <= 255; y++)
  {
    y2 = y * y;
    for (x = 0; x <= 255; x++)
    {
      val = rt2 * sqrt(x * x + y2);
      if (val > 65535.0)
        val = 65535.0;
      root[y][x] = (US16)(val + 0.5);
    }
  }

  // arct takes dx + 128 and dy + 128 for angle
  for (y = 0; y <= 255; y++)
  {
    ya = y - 128;
    for (x = 0; x <= 255; x++)
    {
      val = atan2((double)(128 - x), ya);
      if (val < 0.0)                         // force +/-180 to 0-360
        val += pi2;
      ival = (int)(sc * val + 0.5);
      if (ival >= 65536)                     // wrap around check
        ival -= 65536;
      arct[y][x] = (US16) ival;
    }
  }

  // compute one third of all possible sums (AvgRGB)
  for (i = 0; i <= 765; i++)
    third[i] = (UC8)((i + 1) / 3);
}


//= Cleans up allocated tables if last instance in existence.

jhcEdge::~jhcEdge ()
{
  if (--instances > 0)
    return;

  // free all arrays
  if (third != NULL)
    delete [] third;
  if (root != NULL)
    delete [] root;
  if (arct != NULL)
    delete [] arct;

  // reinitialize
  instances = 0;
  root = NULL;
  arct = NULL;
  third = NULL;
}


////////////////////////////////////////////////////////////////////////////////
//                       Simplest Edge Finder                                 //
////////////////////////////////////////////////////////////////////////////////

//= Vector sum of adjacent orthogonal (not diagonal) differences.
// magnitude scaled (by 1/sqrt(2)) to fit in range [0 255]

int jhcEdge::RobEdge (jhcImg& dest, const jhcImg& src, double sc) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcEdge::RobEdge");
  dest.CopyRoi(src);

  // general ROI case
  int val, sf = ROUND(sc * 256.0);
  int x, y, dx, dy, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = dsk + 1;
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(), *a = s + src.Line();

  // compute in interior (except for left border)
  for (y = rh - 1; y > 0; y--, d += dsk, a += ssk, s += ssk) 
  {
    *d++ = 0;
    for (x = rw - 1; x > 0; x--, d++, a++, s++)
    {
      dx = abs(s[1] - s[0]);
      dy = abs(a[1] - s[1]);
      val = sf * root[dy][dx];
      *d = BOUND(val >> 16);
    }
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  return 1;
}


//= Compute direction based on signs and strength of orthogonal responses.

int jhcEdge::RobDir (jhcImg& dest, const jhcImg& src) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcEdge::RobDir");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, val, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = dsk + 1;
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(), *a = s + src.Line();

  // compute in interior (except for left border)
  for (y = rh - 1; y > 0; y--, d += dsk, a += ssk, s += ssk) 
  {
    *d++ = 0;
    for (x = rw - 1; x > 0; x--, d++, a++, s++)
    {
      dx = s[1] - s[0];
      dy = a[1] - s[1];
      if ((dx >= -128) && (dx < 128) && (dy >= -128) && (dy < 128))
        val = arct[128 + dy][128 + dx];
      else
        val = arct[(256 + dy) >> 1][(256 + dx) >> 1];
      *d = (UC8)(val >> 8);
    }
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  return 1;
}


//= Return the raw mask in each direction.

int jhcEdge::RawRob (jhcImg& xm, jhcImg& ym, const jhcImg& src) const 
{
  if (!xm.Valid(1) || !xm.SameFormat(src) || xm.SameImg(src) || 
      !ym.Valid(1) || !ym.SameFormat(src) || ym.SameImg(src))
    return Fatal("Bad images to jhcEdge::RawRob");
  xm.CopyRoi(src);
  ym.CopyRoi(src);

  // general ROI case
  int x, y, val, rw = xm.RoiW(), rh = xm.RoiH();
  int dsk = xm.RoiSkip(), ssk = dsk + 1;
  UC8 *xr = xm.RoiDest(), *yr = ym.RoiDest();
  const UC8 *s = src.RoiSrc(), *a = s + src.Line();

  // compute in interior (except for left border)
  for (y = rh - 1; y > 0; y--, xr += dsk, yr += dsk, a += ssk, s += ssk) 
  {
    *xr++ = 0;
    *yr++ = 0;
    for (x = rw - 1; x > 0; x--, xr++, yr++, a++, s++)
    {
      val = s[1] - s[0] + 128;
      *xr = BOUND(val);
      val = a[1] - s[1] + 128;
      *yr = BOUND(val);
    }
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, xr++, yr++)
  {
    *xr = 0;
    *yr = 0;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Standard Small Kernel                          //
///////////////////////////////////////////////////////////////////////////

//= Standard 3 by 3 Sobel masks, sets borders to zero.
// uses masks:
// <pre>
//        1  2  1         -1  0  1
//  dy =  0  0  0   dx =  -2  0  2
//       -1 -2 -1         -1  0  1
// </pre>
// magnitude scaled (by 1/sqrt(2)) to fit in range [0 255]

int jhcEdge::SobelEdge (jhcImg& dest, const jhcImg& src, double sc) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcEdge::SobelEdge");
  dest.CopyRoi(src);

  // general ROI case
  int val, sf = ROUND(sc * 256.0);
  int x, y, dx, dy, rw = dest.RoiW(), rh = dest.RoiH();
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = dsk + 1;
  UC8 *d = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *d++ = 0;
    for (x = rw - 2; x > 0; x--, d++, a++, s++, b++)
    {
      dy = abs((a[0] + (a[1] << 1) + a[2]) - (b[0] + (b[1] << 1) + b[2])) >> 2;
      dx = abs((a[2] + (s[2] << 1) + b[2]) - (a[0] + (s[0] << 1) + b[0])) >> 2;
      val = (sf * root[dy][dx]) >> 16;
      *d = (UC8) __min(val, 255);
	  }
    *d = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  return 1;
}


//= Compute direction based on signs and strength of Sobel mask responses.
// can optionally prevent angle from being 0 (forced to 1 instead)

int jhcEdge::SobelDir (jhcImg& dest, const jhcImg& src, int nz) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcEdge::SobelDir");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, val, rw = dest.RoiW(), rh = dest.RoiH();
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = dsk + 1;
  UC8 *d = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *d++ = 0;
    for (x = rw - 2; x > 0; x--, d++, a++, s++, b++)
    {
      dy = ((a[0] + (a[1] << 1) + a[2]) - (b[0] + (b[1] << 1) + b[2])) >> 2;
      dx = ((a[2] + (s[2] << 1) + b[2]) - (a[0] + (s[0] << 1) + b[0])) >> 2;
      if ((dx >= -128) && (dx < 128) && (dy >= -128) && (dy < 128))
        val = arct[128 + dy][128 + dx];
      else
        val = arct[(256 + dy) >> 1][(256 + dx) >> 1];
      val >>= 8;
      if ((nz > 0) && (val == 0))
        *d = 1;
	    else
        *d = (UC8) val;
    }
    *d = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  return 1;
}


//= Computes the direction at each pixel where magnitude is at least mth.
// valid directions are non-zero: 1 = 1.4 degs, 255 = 358.9 degs (360/256)
// can optionally restrict angles: 1 = 0.7 degs, 255 = 179.3 degs (180/256)

int jhcEdge::SobelAng (jhcImg& dest, const jhcImg& src, int mth, int mod180) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcEdge::SobelAng");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, val, rw = dest.RoiW(), rh = dest.RoiH(), m2 = mth * mth;
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = dsk + 1;
  UC8 *d = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *d++ = 0;
    for (x = rw - 2; x > 0; x--, d++, a++, s++, b++)
    {
      // compute mask convolution results
      dy = ((a[0] + (a[1] << 1) + a[2]) - (b[0] + (b[1] << 1) + b[2])) >> 2;
      dx = ((a[2] + (s[2] << 1) + b[2]) - (a[0] + (s[0] << 1) + b[0])) >> 2;

      // check edge strength
      if ((dx * dx + dy * dy) < m2)
	    {
        *d = 0;
        continue;
	    }

      // resolve angle
      if ((dx >= -128) && (dx < 128) && (dy >= -128) && (dy < 128))
        val = arct[128 + dy][128 + dx];
      else
        val = arct[(256 + dy) >> 1][(256 + dx) >> 1];

      // adjust range and guarantee non-zero
      if (mod180 > 0)
        val = (val >> 7) & 0xFF;
	    else
        val >>= 8;
      *d = (UC8) __max(1, val);
    }
    *d = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  return 1;
}


//= Computes both the edge magnitude and direction at the same time.
// can optionally prevent angle from being 0 (forced to 1 instead)
// uses masks:
// <pre>
//        1  2  1         -1  0  1
//  dy =  0  0  0   dx =  -2  0  2
//       -1 -2 -1         -1  0  1
// </pre>

int jhcEdge::SobelFull (jhcImg& mag, jhcImg& dir, const jhcImg& src, double sc, int nz) const 
{
  if (!mag.Valid(1) || !mag.SameFormat(dir) || !mag.SameFormat(src) || 
      mag.SameImg(src) || dir.SameImg(src))
    return Fatal("Bad images to jhcEdge::SobelFull");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  int val, sf = ROUND(sc * 256.0);
  int x, y, dx, dy, rw = mag.RoiW(), rh = mag.RoiH();
  int ln = src.Line(), dsk = mag.RoiSkip() + 1, ssk = dsk + 1;
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk - 1;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, m += dsk, a += ssk, s += ssk) 
  {
    *m++ = 0;
    *d++ = 0;
    for (x = rw - 2; x > 0; x--, m++, d++, a++, s++)
    {
      // basic mask convolution results
      dy = ((a[0] + (a[1] << 1) + a[2]) - (b[0] + (b[1] << 1) + b[2])) >> 2;
      dx = ((a[2] + (s[2] << 1) + b[2]) - (a[0] + (s[0] << 1) + b[0])) >> 2;

      // figure out direction (preserve small difference for better accuracy)
      if ((dx >= -128) && (dx < 128) && (dy >= -128) && (dy < 128))
        val = arct[128 + dy][128 + dx];
      else
        val = arct[(256 + dy) >> 1][(256 + dx) >> 1];
      val >>= 8;
      if ((nz > 0) && (val == 0))
        *d = 1;
	    else
        *d = (UC8) val;

      // figure out magnitude
      val = (sf * root[abs(dy)][abs(dx)]) >> 16;
      *m = (UC8) __min(val, 255);
    }
    *m = 0;
    *d = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Color Versions                             //
///////////////////////////////////////////////////////////////////////////

//= Runs edge finder on the red, green, and blue planes of an image separately.
// returns a color images with edges confined to respective planes

int jhcEdge::SobelRGB (jhcImg& dest, const jhcImg& src, double sc)  
{
  if (!dest.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcEdge::SobelRGB");
  dest.CopyRoi(src);
  tmp.SetSize(src, 1);
  tmp2.SetSize(src, 1);

  // process each field and combine results
  tmp.CopyField(src, 0);                       // blue
  SobelEdge(tmp2, tmp, sc);
  dest.CopyField(tmp2, 0, 0);
  tmp.CopyField(src, 1);                       // green
  SobelEdge(tmp2, tmp, sc);
  dest.CopyField(tmp2, 0, 1);
  tmp.CopyField(src, 2);                       // red
  SobelEdge(tmp2, tmp, sc);
  dest.CopyField(tmp2, 0, 2);
  return 1;
}


//= Computes magnitude based on absolute value of color differences in each channel.
// returns a monochrome image with scaled overall intensity 

int jhcEdge::SobelMagRGB (jhcImg& dest, const jhcImg& src, double sc) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcEdge::SobelMagRGB");
  dest.CopyRoi(src);

  // general ROI case
  int val, sf = ROUND(sc * 256.0);
  int x, y, f, dx, dy, rw = dest.RoiW(), rh = dest.RoiH();
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = src.RoiSkip() + 6;
  UC8 *m = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, m++)
    *m = 0;
  m += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, m += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;                                     // left border
    for (x = rw - 2; x > 0; x--, m++)
    {
      // combine basic mask convolution results from each field
      dx = 0;
      dy = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        dy += abs((a[0] + (a[3] << 1) + a[6]) - (b[0] + (b[3] << 1) + b[6]));
        dx += abs((a[6] + (s[6] << 1) + b[6]) - (a[0] + (s[0] << 1) + b[0]));
      }

      // figure out magnitude
      dx = third[dx >> 2];
      dy = third[dy >> 2];
      val = (sf * root[dy][dx]) >> 16;
      *m = (UC8) __min(val, 255);
    }
    *m = 0;                                       // right border
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, m++)
    *m = 0;
  return 1;
}


//= Computes magntiude based on root mean square of color differences in each channel.
// returns a monochrome image with scaled overall intensity 

int jhcEdge::SobelMagRGB2 (jhcImg& dest, const jhcImg& src, double sc) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcEdge::SobelMagRGB2");
  dest.CopyRoi(src);

  // general ROI case
  double sf = sc / (12.0 * sqrt(2.0));
  int x, y, f, dx, dy, dx2, dy2, val, rw = dest.RoiW(), rh = dest.RoiH();
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = src.RoiSkip() + 6;
  UC8 *m = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, m++)
    *m = 0;
  m += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, m += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;                                     // left border
    for (x = rw - 2; x > 0; x--, m++)
    {
      // combine basic mask convolution results from each field
      dx2 = 0;
      dy2 = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        dy = (a[0] + (a[3] << 1) + a[6]) - (b[0] + (b[3] << 1) + b[6]);
        dy2 += dy * dy;
        dx = (a[6] + (s[6] << 1) + b[6]) - (a[0] + (s[0] << 1) + b[0]);
        dx2 += dx * dx;
      }

      // figure out magnitude
      val = ROUND(sf * sqrt((double)(dx2 + dy2)));
      *m = (UC8) __min(val, 255);
    }
    *m = 0;                                       // right border
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, m++)
    *m = 0;
  return 1;
}


//= Computes the direction at each pixel in a color image where magnitude is at least mth.
// computes magntiude based on absolute value of color differences in each channel
// uses diagonal mask to resolve into 1st or 2nd quadrant (0-90 if dx > 0 and dy > 0)
// valid directions are non-zero: 1 = 0.7 degs, 255 = 179.3 degs (180/256)

int jhcEdge::SobelAngRGB (jhcImg& dest, const jhcImg& src, int mth) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcEdge::SobelAngRGB");
  dest.CopyRoi(src);

  // general ROI case
  int mth2 = 288 * mth * mth;                                    // 288 = 2 * (3 * 4)^2
  int x, y, f, dx, dy, d1, d2, val, rw = dest.RoiW(), rh = dest.RoiH();
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = src.RoiSkip() + 6;
  UC8 *d = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *d++ = 0;                                     // left border
    for (x = rw - 2; x > 0; x--, d++)
    {
      // combine basic mask convolution results from each field
      dx = 0;
      dy = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        dy += abs((a[0] + (a[3] << 1) + a[6]) - (b[0] + (b[3] << 1) + b[6]));
        dx += abs((a[6] + (s[6] << 1) + b[6]) - (a[0] + (s[0] << 1) + b[0]));
      }

      // figure out if magnitude big enough
      val = dx * dx + dy * dy;
      if (val < mth2)
	    {
        *d = 0;
        continue;
	    }

      // resolve basic angle (0-90) and guarantee non-zero
      dx = third[dx >> 2];
      dy = third[dy >> 2];
      if ((dx < 128) && (dy < 128))
        val = arct[128 + dy][128 + dx];
      else
        val = arct[(256 + dy) >> 1][(256 + dx) >> 1];
      val = (val >> 7) & 0xFF;
      val = __max(1, val);

      // compute diagonal convolution results 
      a -= 3;
      s -= 3;
      b -= 3;
      d1 = 0;
      d2 = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        d1 += abs(((a[0] << 1) + a[3] + s[0]) - (s[6] + b[3] + (b[6] << 1)));
        d2 += abs((a[3] + (a[6] << 1) + s[6]) - (s[0] + (b[0] << 1) + b[3]));
      }

      // possibly adjust quadrant
      if (d1 > d2)
        val = 256 - val;
      *d = (UC8) val;
    }
    *d = 0;                                       // right border
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  return 1;
}


//= Computes the direction in a color image using RMS difference for masks.
// computes magntiude based on absolute value of color differences in each channel
// uses diagonal mask to resolve into 1st or 2nd quadrant (0-90 if dx > 0 and dy > 0)
// valid directions are non-zero: 1 = 0.7 degs, 255 = 179.3 degs (180/256)

int jhcEdge::SobelAngRGB2 (jhcImg& dest, const jhcImg& src, int mth) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 3))
    return Fatal("Bad images to jhcEdge::SobelAngRGB2");
  dest.CopyRoi(src);

  // general ROI case
  double norm = 1.0 / 48.0;
  int mth2 = 288 * mth * mth;                                    // 288 = 2 * (3 * 4)^2
  int x, y, f, dx, dy, dx2, dy2, d1, d2, sq1, sq2, val, rw = dest.RoiW(), rh = dest.RoiH();
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = src.RoiSkip() + 6;
  UC8 *d = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *d++ = 0;                                     // left border
    for (x = rw - 2; x > 0; x--, d++)
    {
      // combine basic mask convolution results from each field
      dx2 = 0;
      dy2 = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        dy = (a[0] + (a[3] << 1) + a[6]) - (b[0] + (b[3] << 1) + b[6]);
        dy2 += dy * dy;
        dx = (a[6] + (s[6] << 1) + b[6]) - (a[0] + (s[0] << 1) + b[0]);
        dx2 += dx * dx;
      }

      // figure out if magnitude big enough
      val = dx2 + dy2;
      if (val < mth2)
	    {
        *d = 0;
        continue;
	    }

      // resolve basic angle (0-90) and guarantee non-zero
      dx = ROUND(sqrt(norm * dx2));
      dy = ROUND(sqrt(norm * dy2));
      if ((dx < 128) && (dy < 128))
        val = arct[128 + dy][128 + dx];
      else
        val = arct[(256 + dy) >> 1][(256 + dx) >> 1];
      val = (val >> 7) & 0xFF;
      val = __max(1, val);

      // compute diagonal convolution results 
      a -= 3;
      s -= 3;
      b -= 3;
      sq1 = 0;
      sq2 = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        d1 = ((a[0] << 1) + a[3] + s[0]) - (s[6] + b[3] + (b[6] << 1));
        sq1 += d1 * d1;
        d2 = (a[3] + (a[6] << 1) + s[6]) - (s[0] + (b[0] << 1) + b[3]);
        sq2 += d2 * d2;
      }

      // possibly adjust quadrant
      if (sq1 > sq2)
        val = 256 - val;
      *d = (UC8) val;
    }
    *d = 0;                                       // right border
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  return 1;
}


//= Computes magnitude and direction (mod 180) of edges in a color image.
// computes magntiude based on absolute value of color differences in each channel
// uses diagonal mask to resolve into 1st or 2nd quadrant (0-90 if dx > 0 and dy > 0)
// can optionally prevent angle from being 0 (forced to 1 instead)
// valid directions are: 1 = 0.7 degs, 255 = 179.3 degs (180/256)

int jhcEdge::SobelFullRGB (jhcImg& mag, jhcImg& dir, const jhcImg& src, double sc, int nz) const
{
  if (!mag.Valid(1) || !mag.SameFormat(dir) || !mag.SameSize(src, 3))
    return Fatal("Bad images to jhcEdge::SobelFullRGB");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  int val, sf = ROUND(sc * 256.0);
  int x, y, f, dx, dy, d1, d2, rw = mag.RoiW(), rh = mag.RoiH();
  int ln = src.Line(), dsk = mag.RoiSkip() + 1, ssk = src.RoiSkip() + 6;
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk - 1;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, m += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;                                     // left border
    *d++ = 0;  
    for (x = rw - 2; x > 0; x--, m++, d++)
    {
      // combine basic mask convolution results from each field
      dx = 0;
      dy = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        dy += abs((a[0] + (a[3] << 1) + a[6]) - (b[0] + (b[3] << 1) + b[6]));
        dx += abs((a[6] + (s[6] << 1) + b[6]) - (a[0] + (s[0] << 1) + b[0]));
      }

      // figure out magnitude
      val = (sf * root[dy][dx]) >> 16;
      *m = (UC8) __min(val, 255);

      // resolve basic angle (0-90) and guarantee non-zero
      if ((dx < 128) && (dy < 128))
        val = arct[128 + dy][128 + dx];
      else
        val = arct[(256 + dy) >> 1][(256 + dx) >> 1];
      val = (val >> 7) & 0xFF;

      // compute diagonal convolution results 
      a -= 3;
      s -= 3;
      b -= 3;
      d1 = 0;
      d2 = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        d1 += abs(((a[0] << 1) + a[3] + s[0]) - (s[6] + b[3] + (b[6] << 1)));
        d2 += abs((a[3] + (a[6] << 1) + s[6]) - (s[0] + (b[0] << 1) + b[3]));
      }

      // possibly adjust quadrant
      if (d1 > d2)
        val = 256 - val;
      if ((nz > 0) && (val == 0))
        *d = 1;
	    else
        *d = (UC8) val;
    }
    *m = 0;                                       // right border
    *d = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


//= Computes magnitude and direction (mod 180) of RMS edges in a color image.
// computes magntiude based on root mean square of color differences across channels
// uses diagonal mask to resolve into 1st or 2nd quadrant (0-90 if dx > 0 and dy > 0)
// can optionally prevent angle from being 0 (forced to 1 instead)
// valid directions are: 1 = 0.7 degs, 255 = 179.3 degs (180/256)

int jhcEdge::SobelFullRGB2 (jhcImg& mag, jhcImg& dir, const jhcImg& src, double sc, int nz) const
{
  if (!mag.Valid(1) || !mag.SameFormat(dir) || !mag.SameSize(src, 3))
    return Fatal("Bad images to jhcEdge::SobelFullRGB2");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  double norm = 1.0 / 48.0, sf = sc / (12.0 * sqrt(2.0));
  int x, y, f, val, dx, dy, dx2, dy2, d1, d2, sq1, sq2, rw = mag.RoiW(), rh = mag.RoiH();
  int ln = src.Line(), dsk = mag.RoiSkip() + 1, ssk = src.RoiSkip() + 6;
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk - 1;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, m += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;                                     // left border
    *d++ = 0;  
    for (x = rw - 2; x > 0; x--, m++, d++)
    {
      // combine basic mask convolution results from each field
      dx2 = 0;
      dy2 = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        dy = (a[0] + (a[3] << 1) + a[6]) - (b[0] + (b[3] << 1) + b[6]);
        dy2 += dy * dy;
        dx = (a[6] + (s[6] << 1) + b[6]) - (a[0] + (s[0] << 1) + b[0]);
        dx2 += dx * dx;
      }

      // figure out magnitude
      val = ROUND(sf * sqrt((double)(dx2 + dy2)));
      *m = (UC8) __min(val, 255);

      // resolve basic angle (0-90) and guarantee non-zero
      dx = ROUND(sqrt(norm * dx2));
      dy = ROUND(sqrt(norm * dy2));
      if ((dx < 128) && (dy < 128))
        val = arct[128 - dy][128 - dx];
      else
        val = arct[(256 - dy) >> 1][(256 - dx) >> 1];
      val >>= 7;

      // compute diagonal convolution results 
      a -= 3;
      s -= 3;
      b -= 3;
      sq1 = 0;
      sq2 = 0;
      for (f = 3; f > 0; f--, a++, s++, b++)
      {
        d1 = ((a[0] << 1) + a[3] + s[0]) - (s[6] + b[3] + (b[6] << 1));
        sq1 += d1 * d1;
        d2 = (a[3] + (a[6] << 1) + s[6]) - (s[0] + (b[0] << 1) + b[3]);
        sq2 += d2 * d2;
      }

      // possibly adjust quadrant
      if (sq1 > sq2)
        val = 256 - val;
      if ((nz > 0) && (val == 0))
        *d = 1;
	    else
        *d = (UC8) val;
    }
    *m = 0;                                       // right border
    *d = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
//                         Categorized Directions                             //
////////////////////////////////////////////////////////////////////////////////

//= Condensed version of Sobel edge finder gives approximate direction and magnitude.
//   dir = 255 or 192 for vertical (45 to 135 degs), 128 or 64 for horizontal (-45 to 45 degs)
//   mag = 255 if above hi threshold, 128 if only above lo threshold
// thresholds corrected to be compatible with other versions of Sobel

int jhcEdge::SobelHV (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi, double lo) const 
{
  if (src.Valid(3))
    return SobelHV_RGB(mag, dir, src, hi, lo);
  if (!src.Valid(1) || !src.SameFormat(mag) || !src.SameFormat(dir) ||
      src.SameImg(mag) || src.SameImg(dir))
    return Fatal("Bad images to jhcEdge::SobelHV");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, m2, lo2 = ROUND(lo * lo * 32.0), hi2 = ROUND(hi * hi * 32.0);
  int rw_2 = mag.RoiW() - 2, rh_2 = mag.RoiH() - 2, dsk = mag.RoiSkip(), ssk = src.RoiSkip() + 2; 
  const UC8 *b = src.RoiSrc(), *s = b + src.Line(), *a = s + src.Line();
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk;
  d += dsk;

  // compute in interior (except for left and right border)
  for (y = rh_2; y > 0; y--, m += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;
    *d++ = 0;
    for (x = rw_2; x > 0; x--, m++, d++, a++, s++, b++)
    {
      // get mask response
      dy = a[0] + (a[1] << 1) + a[2] - b[0] - (b[1] << 1) - b[2];
      dx = a[2] + (s[2] << 1) + b[2] - a[0] - (s[0] << 1) - b[0];
      m2 = dx * dx + dy * dy;

      // see if any edge at all based on magnitude
      if (m2 < lo2)
        *m = 0;
      else if (m2 < hi2)
        *m = 128;
      else 
        *m = 255;

      // resolve into horizontal or vertical
      if (m2 == 0)
        *d = 0;
      else if (abs(dy) > abs(dx))
      {
        if (dy > 0)
          *d = 128;
        else
          *d = 64;
      }
      else 
      {
        if (dx > 0)
          *d = 255;
        else
          *d = 192;
      }
    }
    *m++ = 0;
    *d++ = 0;
  }

  // cannot compute value for top line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


//= Condensed version of Sobel edge finder gives approximate COLOR direction and magnitude.
//   dir = 255 or 192 for vertical (45 to 135 degs), 128 or 64 for horizontal (-45 to 45 degs)
//   mag = 255 if above hi threshold, 128 if only above lo threshold
// magnitude and angle is taken from maximum edge strength in R, G, or B color field
// thresholds corrected to be compatible with other versions of Sobel

int jhcEdge::SobelHV_RGB (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi, double lo) const 
{
  if (!src.Valid(3) || !src.SameSize(mag, 1) || !src.SameSize(dir, 1))
    return Fatal("Bad images to jhcEdge::SobelHV_RGB");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, m2, cdx, cdy, cm2;
  int lo2 = ROUND(lo * lo * 32.0), hi2 = ROUND(hi * hi * 32.0);
  int rw_2 = mag.RoiW() - 2, rh_2 = mag.RoiH() - 2, dsk = mag.RoiSkip(), ssk = src.RoiSkip() + 6; 
  const UC8 *b = src.RoiSrc(), *s = b + src.Line(), *a = s + src.Line();
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk;
  d += dsk;

  // compute in interior (except for left and right border)
  for (y = rh_2; y > 0; y--, m += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;
    *d++ = 0;
    for (x = rw_2; x > 0; x--, m++, d++, a += 3, s += 3, b += 3)
    {
      // blue channel
      dy = a[0] + (a[3] << 1) + a[6] - b[0] - (b[3] << 1) - b[6];
      dx = a[6] + (s[6] << 1) + b[6] - a[0] - (s[0] << 1) - b[0];
      m2 = dx * dx + dy * dy;

      // green channel
      cdy = a[1] + (a[4] << 1) + a[7] - b[1] - (b[4] << 1) - b[7];
      cdx = a[7] + (s[7] << 1) + b[7] - a[1] - (s[1] << 1) - b[1];
      cm2 = cdx * cdx + cdy * cdy;
      if (cm2 > m2)
      {
        m2 = cm2;
        dx = cdx;
        dy = cdy;
      } 

      // red channel
      cdy = a[2] + (a[5] << 1) + a[8] - b[2] - (b[5] << 1) - b[8];
      cdx = a[8] + (s[8] << 1) + b[8] - a[2] - (s[2] << 1) - b[2];
      cm2 = cdx * cdx + cdy * cdy;
      if (cm2 > m2)
      {
        m2 = cm2;
        dx = cdx;
        dy = cdy;
      } 

      // see if any edge at all based on magnitude
      if (m2 < lo2)
        *m = 0;
      else if (m2 < hi2)
        *m = 128;
      else 
        *m = 255;

      // resolve into horizontal or vertical
      if (m2 == 0)
        *d = 0;
      else if (abs(dy) > abs(dx))
      {
        if (dy > 0)
          *d = 128;
        else
          *d = 64;
      }
      else 
      {
        if (dx > 0)
          *d = 255;
        else
          *d = 192;
      }
    }
    *m++ = 0;
    *d++ = 0;
  }

  // cannot compute value for top line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


//= Breaks image into horizontal, vertical, and diagonal edges with two magnitudes.
//   dir = 255 or 224 for V, 192 or 160 of D1, 128 or 96 for D2, 64 or 32 for H
//   mag = 255 if above hi threshold, 128 if only above lo threshold
// thresholds corrected to be compatible with other versions of Sobel

int jhcEdge::SobelHVD (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi, double lo) const 
{
  if (src.Valid(3))
    return SobelHVD_RGB(mag, dir, src, hi, lo);
  if (!src.Valid(1) || !src.SameFormat(mag) || !src.SameFormat(dir) ||
      src.SameImg(mag) || src.SameImg(dir))
    return Fatal("Bad images to jhcEdge::SobelHVD");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, m2;
  int lo2 = ROUND(lo * lo * 32.0), hi2 = ROUND(hi * hi * 32.0);
  int rw_2 = mag.RoiW() - 2, rh_2 = mag.RoiH() - 2, dsk = mag.RoiSkip(), ssk = src.RoiSkip() + 2; 
  const UC8 *b = src.RoiSrc(), *s = b + src.Line(), *a = s + src.Line();
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk;
  d += dsk;

  // compute in interior (except for left and right border)
  for (y = rh_2; y > 0; y--, m += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;
    *d++ = 0;
    for (x = rw_2; x > 0; x--, m++, d++, a++, s++, b++)
    {
      // get mask response
      dy = a[0] + (a[1] << 1) + a[2] - b[0] - (b[1] << 1) - b[2];
      dx = a[2] + (s[2] << 1) + b[2] - a[0] - (s[0] << 1) - b[0];
      m2 = dx * dx + dy * dy;

      // see if any edge at all based on magnitude
      if (m2 < lo2)
        *m = 0;
      else if (m2 < hi2)
        *m = 128;
      else 
        *m = 255;

      // resolve into some 30 degree slice
      if (m2 == 0)
        *d = 0;
      else if (dx > 0)
      {
        if (dy > 0)           // first quadrant
        {
          if (dy > (dx << 1))          // 26.5 degs
            *d = 64;
          else if (dx <= (dy << 1))    // 63.4 degs
            *d = 128;
          else 
            *d = 255;
        }
        else                  // second quadrant
        {
          if (dx > (-dy << 1))         // 116.5 degs
            *d = 255;
          else if (-dy <= (dx << 1))   // 153.4 dges
            *d = 192;        
          else 
            *d = 32;
        }
      }
      else
      {
        if (dy <= 0)          // third quadrant
        {
          if (-dy > (-dx << 1))        // 206.5 degs
            *d = 32;
          else if (-dx <= (-dy << 1))  // 243.4 degs
            *d = 96;
          else 
            *d = 224;
        }
        else                  // fourth quadrant
        {
          if (-dx > (dy << 1))         // 296.5 degs
            *d = 224;
          else if (dy <= (-dx << 1))   // 333.4 degs
            *d = 160;
          else 
            *d = 64;
        }
      }
    }
    *m++ = 0;
    *d++ = 0;
  }

  // cannot compute value for top line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


//= Breaks image into horizontal, vertical, and diagonal COLOR edges with two magnitudes.
//   dir = 255 or 224 for V, 192 or 160 of D1, 128 or 96 for D2, 64 or 32 for H
//   mag = 255 if above hi threshold, 128 if only above lo threshold
// magnitude and angle is taken from maximum edge strength in R, G, or B color field
// thresholds corrected to be compatible with other versions of Sobel

int jhcEdge::SobelHVD_RGB (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi, double lo) const 
{
  if (!src.Valid(3) || !src.SameSize(mag, 1) || !src.SameSize(dir, 1))
    return Fatal("Bad images to jhcEdge::SobelHVD_RGB");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, m2, cdx, cdy, cm2;
  int lo2 = ROUND(lo * lo * 32.0), hi2 = ROUND(hi * hi * 32.0);
  int rw_2 = mag.RoiW() - 2, rh_2 = mag.RoiH() - 2, dsk = mag.RoiSkip(), ssk = src.RoiSkip() + 6; 
  const UC8 *b = src.RoiSrc(), *s = b + src.Line(), *a = s + src.Line();
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk;
  d += dsk;

  // compute in interior (except for left and right border)
  for (y = rh_2; y > 0; y--, m += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;
    *d++ = 0;
    for (x = rw_2; x > 0; x--, m++, d++, a += 3, s += 3, b += 3)
    {
      // blue channel
      dy = a[0] + (a[3] << 1) + a[6] - b[0] - (b[3] << 1) - b[6];
      dx = a[6] + (s[6] << 1) + b[6] - a[0] - (s[0] << 1) - b[0];
      m2 = dx * dx + dy * dy;

      // green channel
      cdy = a[1] + (a[4] << 1) + a[7] - b[1] - (b[4] << 1) - b[7];
      cdx = a[7] + (s[7] << 1) + b[7] - a[1] - (s[1] << 1) - b[1];
      cm2 = cdx * cdx + cdy * cdy;
      if (cm2 > m2)
      {
        m2 = cm2;
        dx = cdx;
        dy = cdy;
      } 

      // red channel
      cdy = a[2] + (a[5] << 1) + a[8] - b[2] - (b[5] << 1) - b[8];
      cdx = a[8] + (s[8] << 1) + b[8] - a[2] - (s[2] << 1) - b[2];
      cm2 = cdx * cdx + cdy * cdy;
      if (cm2 > m2)
      {
        m2 = cm2;
        dx = cdx;
        dy = cdy;
      } 

      // see if any edge at all based on magnitude
      if (m2 < lo2)
        *m = 0;
      else if (m2 < hi2)
        *m = 128;
      else 
        *m = 255;

      // resolve into some 30 degree slice
      if (m2 == 0)
        *d = 0;
      else if (dx > 0)
      {
        if (dy > 0)           // first quadrant
        {
          if (dy > (dx << 1))          // 26.5 degs
            *d = 64;
          else if (dx <= (dy << 1))    // 63.4 degs
            *d = 128;
          else 
            *d = 255;
        }
        else                  // second quadrant
        {
          if (dx > (-dy << 1))         // 116.5 degs
            *d = 255;
          else if (-dy <= (dx << 1))   // 153.4 dges
            *d = 192;        
          else 
            *d = 32;
        }
      }
      else
      {
        if (dy <= 0)          // third quadrant
        {
          if (-dy > (-dx << 1))        // 206.5 degs
            *d = 32;
          else if (-dx <= (-dy << 1))  // 243.4 degs
            *d = 96;
          else 
            *d = 224;
        }
        else                  // fourth quadrant
        {
          if (-dx > (dy << 1))         // 296.5 degs
            *d = 224;
          else if (dy <= (-dx << 1))   // 333.4 degs
            *d = 160;
          else 
            *d = 64;
        }
      }
    }
    *m++ = 0;
    *d++ = 0;
  }

  // cannot compute value for top line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


//= Sobel edge finder gives direction to +/- 14 degrees and two magnitude levels.
//   mag = 255 if above hi threshold, 128 if only above lo threshold
//   dir = middle of sectors (9, 25, 39, 56, 73, ...) = (13, 35, 55, 77, 103, ...) degs
// angle marked as 0 where it cannot be determined (magnitude = 0)
// thresholds corrected to be compatible with other versions of Sobel
// takes about 15% longer than SobelHV but gives better angle resolution

int jhcEdge::Sobel22 (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi, double lo) const 
{
  if (src.Valid(3))
    return Sobel22_RGB(mag, dir, src, hi, lo);
  if (!src.Valid(1) || !src.SameFormat(mag) || !src.SameFormat(dir) ||
      src.SameImg(mag) || src.SameImg(dir))
    return Fatal("Bad images to jhcEdge::Sobel22");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, m2;
  int lo2 = ROUND(lo * lo * 32.0), hi2 = ROUND(hi * hi * 32.0);
  int rw_2 = mag.RoiW() - 2, rh_2 = mag.RoiH() - 2, dsk = mag.RoiSkip(), ssk = src.RoiSkip() + 2; 
  const UC8 *b = src.RoiSrc(), *s = b + src.Line(), *a = s + src.Line();
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk;
  d += dsk;

  // compute in interior (except for left and right border)
  for (y = rh_2; y > 0; y--, m += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;
    *d++ = 0;
    for (x = rw_2; x > 0; x--, m++, d++, a++, s++, b++)
    {
      // get mask response
      dy = a[0] + (a[1] << 1) + a[2] - b[0] - (b[1] << 1) - b[2];
      dx = a[2] + (s[2] << 1) + b[2] - a[0] - (s[0] << 1) - b[0];
      m2 = dx * dx + dy * dy;

      // see if any edge at all based on magnitude
      if (m2 < lo2)
        *m = 0;
      else if (m2 < hi2)
        *m = 128;
      else 
        *m = 255;

      // resolve into some 30 degree slice
      if (m2 == 0)
        *d = 0;
      else if (dx > 0)
      {
        if (dy > 0)           // first quadrant
        {
          if (dy > (dx << 1))          // 26.5 degs
            *d = 9;
          else if (dy > dx)            // 45.0 degs
            *d = 25;
          else if (dx <= (dy << 1))    // 63.4 degs
            *d = 39;
          else 
            *d = 55;
        }
        else                  // second quadrant
        {
          if (dx > (-dy << 1))         // 116.5 degs
            *d = 73;
          else if (dx > -dy)           // 135.0 degs
            *d = 89;
          else if (-dy <= (dx << 1))   // 153.4 dges
            *d = 103;        
          else 
            *d = 119;
        }
      }
      else
      {
        if (dy <= 0)          // third quadrant
        {
          if (-dy > (-dx << 1))        // 206.5 degs
            *d = 137;
          else if (-dy > -dx)          // 225.0 degs
            *d = 153;
          else if (-dx <= (-dy << 1))  // 243.4 degs
            *d = 167;
          else 
            *d = 183;
        }
        else                  // fourth quadrant
        {
          if (-dx > (dy << 1))         // 296.5 degs
            *d = 201;
          else if (-dx > dy)           // 315.0 degs
            *d = 217;
          else if (dy <= (-dx << 1))   // 333.4 degs
            *d = 231;
          else 
            *d = 247;
        }
      }
    }
    *m++ = 0;
    *d++ = 0;
  }

  // cannot compute value for top line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


//= Sobel COLOR edge finder gives direction to +/- 14 degrees and two magnitude levels.
//   mag = 255 if above hi threshold, 128 if only above lo threshold
//   dir = middle of sectors (9, 25, 39, 56, 73, ...) = (13, 35, 55, 77, 103, ...) degs
// angle marked as 0 where it cannot be determined (magnitude = 0)
// magnitude and angle is taken from maximum edge strength in R, G, or B color field
// thresholds corrected to be compatible with other versions of Sobel
// takes about 15% longer than SobelHV but gives better angle resolution

int jhcEdge::Sobel22_RGB (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi, double lo) const 
{
  if (!src.Valid(3) || !src.SameSize(mag, 1) || !src.SameSize(dir, 1))
    return Fatal("Bad images to jhcEdge::Sobel22_RGB");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, m2, cdx, cdy, cm2;
  int lo2 = ROUND(lo * lo * 32.0), hi2 = ROUND(hi * hi * 32.0);
  int rw_2 = mag.RoiW() - 2, rh_2 = mag.RoiH() - 2, dsk = mag.RoiSkip(), ssk = src.RoiSkip() + 6; 
  const UC8 *b = src.RoiSrc(), *s = b + src.Line(), *a = s + src.Line();
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk;
  d += dsk;

  // compute in interior (except for left and right border)
  for (y = rh_2; y > 0; y--, m += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;
    *d++ = 0;
    for (x = rw_2; x > 0; x--, m++, d++, a += 3, s += 3, b += 3)
    {
      // blue channel
      dy = a[0] + (a[3] << 1) + a[6] - b[0] - (b[3] << 1) - b[6];
      dx = a[6] + (s[6] << 1) + b[6] - a[0] - (s[0] << 1) - b[0];
      m2 = dx * dx + dy * dy;

      // green channel
      cdy = a[1] + (a[4] << 1) + a[7] - b[1] - (b[4] << 1) - b[7];
      cdx = a[7] + (s[7] << 1) + b[7] - a[1] - (s[1] << 1) - b[1];
      cm2 = cdx * cdx + cdy * cdy;
      if (cm2 > m2)
      {
        m2 = cm2;
        dx = cdx;
        dy = cdy;
      } 

      // red channel
      cdy = a[2] + (a[5] << 1) + a[8] - b[2] - (b[5] << 1) - b[8];
      cdx = a[8] + (s[8] << 1) + b[8] - a[2] - (s[2] << 1) - b[2];
      cm2 = cdx * cdx + cdy * cdy;
      if (cm2 > m2)
      {
        m2 = cm2;
        dx = cdx;
        dy = cdy;
      } 

      // see if any edge at all based on magnitude
      if (m2 < lo2)
        *m = 0;
      else if (m2 < hi2)
        *m = 128;
      else 
        *m = 255;

      // resolve into some 30 degree slice
      if (m2 == 0)
        *d = 0;
      else if (dx > 0)
      {
        if (dy > 0)           // first quadrant
        {
          if (dy > (dx << 1))          // 26.5 degs
            *d = 9;
          else if (dy > dx)            // 45.0 degs
            *d = 25;
          else if (dx <= (dy << 1))    // 63.4 degs
            *d = 39;
          else 
            *d = 55;
        }
        else                  // second quadrant
        {
          if (dx > (-dy << 1))         // 116.5 degs
            *d = 73;
          else if (dx > -dy)           // 135.0 degs
            *d = 89;
          else if (-dy <= (dx << 1))   // 153.4 dges
            *d = 103;        
          else 
            *d = 119;
        }
      }
      else
      {
        if (dy <= 0)          // third quadrant
        {
          if (-dy > (-dx << 1))        // 206.5 degs
            *d = 137;
          else if (-dy > -dx)          // 225.0 degs
            *d = 153;
          else if (-dx <= (-dy << 1))  // 243.4 degs
            *d = 167;
          else 
            *d = 183;
        }
        else                  // fourth quadrant
        {
          if (-dx > (dy << 1))         // 296.5 degs
            *d = 201;
          else if (-dx > dy)           // 315.0 degs
            *d = 217;
          else if (dy <= (-dx << 1))   // 333.4 degs
            *d = 231;
          else 
            *d = 247;
        }
      }
    }
    *m++ = 0;
    *d++ = 0;
  }

  // cannot compute value for top line of image
  for (x = rw_2 + 2; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


//= Sobel edge finder gives direction in HV and D12 quadrants with two magnitude levels.
//   mag = 255 if above hi threshold, 128 if only above lo threshold
//   hv = 32 or 96 for horizontal, 160 or 224 for vertical 
//   d12 = 32 or 96 for major diagonal, 160 or 224 for minor diagonal
// angle marked as 0 where magnitude is below lo threshold
// thresholds corrected to be compatible with other versions of Sobel
// <pre>
//           \ 160 /               96 | 160
//             \ /                    |
//    hv =  96  x  32      d12 =  ----+-----
//             / \                    |
//           / 224 \              224 |  32
// </pre>

int jhcEdge::SobelQuad (jhcImg& mag, jhcImg& hv, jhcImg& d12, const jhcImg& src, double hi, double lo) const 
{
  if (src.Valid(3))
    return SobelQuad_RGB(mag, hv, d12, src, hi, lo);
  if (!src.Valid(1) || !src.SameFormat(mag) || !src.SameFormat(hv) || !src.SameFormat(d12) ||
      src.SameImg(mag) || src.SameImg(hv) || src.SameImg(d12))
    return Fatal("Bad images to jhcEdge::SobelQuad");
  mag.CopyRoi(src);
  hv.CopyRoi(src);
  d12.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, m2;
  int lo2 = __max(1, ROUND(lo * lo * 32.0)), hi2 = ROUND(hi * hi * 32.0);
  int rw_2 = mag.RoiW() - 2, rh_2 = mag.RoiH() - 2, dsk = mag.RoiSkip(), ssk = src.RoiSkip() + 2; 
  const UC8 *b = src.RoiSrc(), *s = b + src.Line(), *a = s + src.Line();
  UC8 *m = mag.RoiDest(), *q = hv.RoiDest(), *d = d12.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw_2 + 2; x > 0; x--, m++, q++, d++)
  {
    *m = 0;
    *q = 0;
    *d = 0;
  }
  m += dsk;
  q += dsk;
  d += dsk;

  // compute in interior (except for left and right border)
  for (y = rh_2; y > 0; y--, m += dsk, q += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;
    *q++ = 0;
    *d++ = 0;
    for (x = rw_2; x > 0; x--, m++, q++, d++, a++, s++, b++)
    {
      // get mask response
      dy = a[0] + (a[1] << 1) + a[2] - b[0] - (b[1] << 1) - b[2];
      dx = a[2] + (s[2] << 1) + b[2] - a[0] - (s[0] << 1) - b[0];
      m2 = dx * dx + dy * dy;

      // see if any edge at all based on magnitude
      if (m2 < lo2)
      {
        *m = 0;
        *q = 0;
        *d = 0;
        continue;
      }
      if (m2 < hi2)
        *m = 128;
      else 
        *m = 255;

      // resolve into a diagonal quadrant
      if (dx > abs(dy))         // centered on 0 degs
        *q = 32;
      else if (dy > abs(dx))    // centered on 90 degs
        *q = 160;
      else if (-dx > abs(dy))   // centered on 180 degs
        *q = 96;
      else                      // centered on 270 degs
        *q = 224;

      // resolve into an orthogonal quadrant
      if (dx > 0)
      {
        if (dy <= 0)            // centered on 315 degs
          *d = 32;
        else                    // centered on 45 degs
          *d = 160;
      }
      else
      {
        if (dy > 0)             // centered on 135 degs
          *d = 96;
        else                    // centered on 225 degs
          *d = 224;         
      }
    }
    *m++ = 0;
    *q++ = 0;
    *d++ = 0;
  }

  // cannot compute value for top line of image
  for (x = rw_2 + 2; x > 0; x--, m++, q++, d++)
  {
    *m = 0;
    *q = 0;
    *d = 0;
  }
  return 1;
}


//= Sobel COLOR edge finder gives direction in HV and D12 quadrants with two magnitude levels.
//   mag = 255 if above hi threshold, 128 if only above lo threshold
//   hv = 32 or 96 for horizontal, 160 or 224 for vertical 
//   d12 = 32 or 96 for major diagonal, 160 or 224 for minor diagonal
// angle marked as 0 where magnitude is below lo threshold
// magnitude and angle is taken from maximum edge strength in R, G, or B color field
// thresholds corrected to be compatible with other versions of Sobel
// <pre>
//           \ 160 /               96 | 160
//             \ /                    |
//    hv =  96  x  32      d12 =  ----+-----
//             / \                    |
//           / 224 \              224 |  32
// </pre>

int jhcEdge::SobelQuad_RGB (jhcImg& mag, jhcImg& hv, jhcImg& d12, const jhcImg& src, double hi, double lo) const 
{
  if (!src.Valid(3) || !src.SameSize(mag, 1) || !src.SameSize(hv, 1) || !src.SameSize(d12, 1))
    return Fatal("Bad images to jhcEdge::SobelQuad_RGB");
  mag.CopyRoi(src);
  hv.CopyRoi(src);
  d12.CopyRoi(src);

  // general ROI case
  int x, y, dx, dy, m2, cdx, cdy, cm2;
  int lo2 = __max(1, ROUND(lo * lo * 32.0)), hi2 = ROUND(hi * hi * 32.0);
  int rw_2 = mag.RoiW() - 2, rh_2 = mag.RoiH() - 2, dsk = mag.RoiSkip(), ssk = src.RoiSkip() + 6; 
  const UC8 *b = src.RoiSrc(), *s = b + src.Line(), *a = s + src.Line();
  UC8 *m = mag.RoiDest(), *q = hv.RoiDest(), *d = d12.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw_2 + 2; x > 0; x--, m++, q++, d++)
  {
    *m = 0;
    *q = 0;
    *d = 0;
  }
  m += dsk;
  q += dsk;
  d += dsk;

  // compute in interior (except for left and right border)
  for (y = rh_2; y > 0; y--, m += dsk, q += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;
    *q++ = 0;
    *d++ = 0;
    for (x = rw_2; x > 0; x--, m++, q++, d++, a += 3, s += 3, b += 3)
    {
      // blue channel
      dy = a[0] + (a[3] << 1) + a[6] - b[0] - (b[3] << 1) - b[6];
      dx = a[6] + (s[6] << 1) + b[6] - a[0] - (s[0] << 1) - b[0];
      m2 = dx * dx + dy * dy;

      // green channel
      cdy = a[1] + (a[4] << 1) + a[7] - b[1] - (b[4] << 1) - b[7];
      cdx = a[7] + (s[7] << 1) + b[7] - a[1] - (s[1] << 1) - b[1];
      cm2 = cdx * cdx + cdy * cdy;
      if (cm2 > m2)
      {
        m2 = cm2;
        dx = cdx;
        dy = cdy;
      } 

      // red channel
      cdy = a[2] + (a[5] << 1) + a[8] - b[2] - (b[5] << 1) - b[8];
      cdx = a[8] + (s[8] << 1) + b[8] - a[2] - (s[2] << 1) - b[2];
      cm2 = cdx * cdx + cdy * cdy;
      if (cm2 > m2)
      {
        m2 = cm2;
        dx = cdx;
        dy = cdy;
      } 

      // see if any edge at all based on magnitude
      if (m2 < lo2)
      {
        *m = 0;
        *q = 0;
        *d = 0;
        continue;
      }
      if (m2 < hi2)
        *m = 128;
      else 
        *m = 255;

      // resolve into a diagonal quadrant
      if (dx > abs(dy))         // centered on 0 degs
        *q = 32;
      else if (dy > abs(dx))    // centered on 90 degs
        *q = 160;
      else if (-dx > abs(dy))   // centered on 180 degs
        *q = 96;
      else                      // centered on 270 degs
        *q = 224;

      // resolve into an orthogonal quadrant
      if (dx > 0)
      {
        if (dy <= 0)            // centered on 315 degs
          *d = 32;
        else                    // centered on 45 degs
          *d = 160;
      }
      else
      {
        if (dy > 0)             // centered on 135 degs
          *d = 96;
        else                    // centered on 225 degs
          *d = 224;         
      }
    }
    *m++ = 0;
    *q++ = 0;
    *d++ = 0;
  }

  // cannot compute value for top line of image
  for (x = rw_2 + 2; x > 0; x--, m++, q++, d++)
  {
    *m = 0;
    *q = 0;
    *d = 0;
  }
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
//                         Lower Level Results                                //
////////////////////////////////////////////////////////////////////////////////

//= Returns the basic DX and DY convolution results.
// <pre>
//     X: -1  0 +1      Y: +1 +2 +1
//        -2  0 +2          0  0  0
//        -1  0 +1         -1 -2 -1
// </pre>
// "top" values in masks are on bottom in displayed images
// results are lifted to 128 (i.e. 138 = +10, 118 = -10)

int jhcEdge::RawSobel (jhcImg& xm, jhcImg& ym, const jhcImg& src) const 
{
  if (!src.Valid(1) || !src.SameFormat(xm) || !src.SameFormat(ym))
    return Fatal("Bad images to jhcEdge::RawSobel");
  xm.CopyRoi(src);
  ym.CopyRoi(src);

  // general ROI case
  int x, y, val, rw = src.RoiW(), rh = src.RoiH();
  int dsk = xm.RoiSkip(), ssk = src.RoiSkip() + 2, ln = src.Line(); 
  const UC8 *a = src.RoiSrc(), *s = a + ln, *b = s + ln;
  UC8 *xv = xm.RoiDest(), *yv = ym.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, xv++, yv++)
  {
    *xv = 128;
    *yv = 128;
  }
  xv += dsk;
  yv += dsk;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, xv += dsk, yv += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *xv++ = 128;
    *yv++ = 128;
    for (x = rw - 2; x > 0; x--, xv++, yv++, a++, s++, b++)
    {
      val = (a[2] + (s[2] << 1) + b[2]) - (a[0] + (s[0] << 1) + b[0]);
      *xv = (UC8)((val + 1024) >> 3);
      val = (a[0] + (a[1] << 1) + a[2]) - (b[0] + (b[1] << 1) + b[2]);
      *yv = (UC8)((val + 1024) >> 3);
    }
    *xv++ = 128;
    *yv++ = 128;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, xv++, yv++)
  {
    *xv = 128;
    *yv = 128;
  }
  return 1;
}


//= Like RawSobel but also gives convolution results for diagonal masks.
// <pre>
//    D1: +2 +1  0     D2:  0 +1 +2
//        +1  0 -1         -1  0 +1
//         0 -1 -2         -1 -2  0
// </pre>
// "top" values in masks are on bottom in displayed images
// results are lifted to 128 and scaled by half (i.e. 118 = -20)

int jhcEdge::RawSobel4 (jhcImg& xm, jhcImg& ym, 
                        jhcImg& d1m, jhcImg& d2m, const jhcImg& src) const 
{
  if (!src.Valid(1) || !src.SameFormat(xm) || !src.SameFormat(ym) ||
      !src.SameFormat(d1m) || !src.SameFormat(d2m))
    return Fatal("Bad images to jhcEdge::RawSobel4");
  xm.CopyRoi(src);
  ym.CopyRoi(src);
  d1m.CopyRoi(src);
  d2m.CopyRoi(src);

  // general ROI case
  int x, y, val, rw = src.RoiW(), rh = src.RoiH();
  int dsk = xm.RoiSkip(), ssk = src.RoiSkip() + 2, ln = src.Line(); 
  const UC8 *a = src.RoiSrc(), *s = a + ln, *b = s + ln;
  UC8 *xv = xm.RoiDest(), *yv = ym.RoiDest();
  UC8 *d1v = d1m.RoiDest(), *d2v = d2m.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, xv++, yv++, d1v++, d2v++)
  {
    *xv  = 128;
    *yv  = 128;
    *d1v = 128;
    *d2v = 128;
  }
  xv  += dsk;
  yv  += dsk;
  d1v += dsk;
  d2v += dsk;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, xv += dsk, yv += dsk, d1v += dsk, d2v += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *xv++  = 128;
    *yv++  = 128;
    *d1v++ = 128;
    *d2v++ = 128;
    for (x = rw - 2; x > 0; x--, xv++, yv++, d1v++, d2v++, a++, s++, b++)
    {
      val = (a[2] + (s[2] << 1) + b[2]) - (a[0] + (s[0] << 1) + b[0]);
      *xv = (UC8)((val + 1024) >> 3);
      val = (a[0] + (a[1] << 1) + a[2]) - (b[0] + (b[1] << 1) + b[2]);
      *yv = (UC8)((val + 1024) >> 3);
      val = ((a[0] << 1) + a[1] + s[0]) - (s[2] + b[1] + (b[2] << 1));
      *d1v = (UC8)((val + 1024) >> 3);
      val = (a[1] + (a[2] << 1) + s[2]) - (s[0] + (b[0] << 1) + b[1]);
      *d2v = (UC8)((val + 1024) >> 3);
    }
    *xv++  = 128;
    *yv++  = 128;
    *d1v++ = 128;
    *d2v++ = 128;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, xv++, yv++, d1v++, d2v++)
  {
    *xv  = 128;
    *yv  = 128;
    *d1v = 128;
    *d2v = 128;
  }
  return 1;
}


//= Returns magnitude of edges with directions in range of alo to ahi degrees.
// if alo greater than ahi does wrap around through zero, can also take angle modulo 180

int jhcEdge::DirSel (jhcImg& dest, const jhcImg& src, double alo, double ahi, 
                     int mod180, double sc) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcEdge::DirSel");
  dest.CopyRoi(src);

  // general ROI case
  int a0, a1, ang, val, sf = ROUND(sc * 256.0);
  int x, y, dx, dy, rw = dest.RoiW(), rh = dest.RoiH();
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = dsk + 1;
  UC8 *d = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // figure out angle range in 0 - 255 (or 0 - 128) values
  if (alo >= 0.0)
    a0 = ROUND(alo * 256.0 / 360.0);
  else
    a0 = ROUND((alo + 360.0) * 256.0 / 360.0);
  if (ahi >= 0.0)
    a1 = ROUND(ahi * 256.0 / 360.0);
  else
    a1 = ROUND((ahi + 360.0) * 256.0 / 360.0);
  if (mod180 > 0)
  {
    a0 &= 0x7F;
    a1 &= 0x7F;
  }

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *d++ = 0;
    for (x = rw - 2; x > 0; x--, d++, a++, b++, s++)
    {
      // get convolution results 
      dy = (a[0] + (a[1] << 1) + a[2] - b[0] - (b[1] << 1) - b[2]) >> 2;
      dx = (a[2] + (s[2] << 1) + b[2] - a[0] - (s[0] << 1) - b[0]) >> 2;

      // compute angle from 0 - 255 (or 0 - 128)
      if ((dx >= -128) && (dx < 128) && (dy >= -128) && (dy < 128))
        ang = arct[dy + 128][dx + 128];
      else
        ang = arct[(dy + 256) >> 1][(dx + 256) >> 1];
      ang >>= 8;
      if (mod180 > 0)
        ang &= 0x7F;

      // see if angle outside desired range
      // might be able to avoid arctan by using slopes instead
      if (((a1 >= a0) && ((ang < a0) || (ang > a1))) ||
          ((a1 <  a0) &&  (ang < a0) && (ang > a1)))
      {
        *d = 0;
        continue;
      }

      // record magnitude for accepted points
      dy = abs(dy);
      dx = abs(dx);
      val = (sf * root[dy][dx]) >> 16;
      *d = (UC8) __min(val, 255);
    }
    *d = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, d++)
    *d = 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Primitive Texture                            //
///////////////////////////////////////////////////////////////////////////

//= Like RawSobel4 but takes absolute value of convolution.
// comparing output magnitudes can tell approximate texture orientation

int jhcEdge::AbsSobel4 (jhcImg& xm, jhcImg& ym, 
                        jhcImg& d1m, jhcImg& d2m, const jhcImg& src, double sc) const 
{
  if (!src.Valid(1) || !src.SameFormat(xm) || !src.SameFormat(ym) ||
      !src.SameFormat(d1m) || !src.SameFormat(d2m))
    return Fatal("Bad images to jhcEdge::AbsSobel4");
  xm.CopyRoi(src);
  ym.CopyRoi(src);
  d1m.CopyRoi(src);
  d2m.CopyRoi(src);

  // general ROI case
  int x, y, val, val2, rw = src.RoiW(), rh = src.RoiH(), sf = ROUND(sc * 256.0);
  int dsk = xm.RoiSkip(), ssk = src.RoiSkip() + 2, ln = src.Line(); 
  const UC8 *a = src.RoiSrc(), *s = a + ln, *b = s + ln;
  UC8 *xv = xm.RoiDest(), *yv = ym.RoiDest();
  UC8 *d1v = d1m.RoiDest(), *d2v = d2m.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, xv++, yv++, d1v++, d2v++)
  {
    *xv  = 0;
    *yv  = 0;
    *d1v = 0;
    *d2v = 0;
  }
  xv  += dsk;
  yv  += dsk;
  d1v += dsk;
  d2v += dsk;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, xv += dsk, yv += dsk, d1v += dsk, d2v += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *xv++  = 0;
    *yv++  = 0;
    *d1v++ = 0;
    *d2v++ = 0;
    for (x = rw - 2; x > 0; x--, xv++, yv++, d1v++, d2v++, a++, s++, b++)
    {
      val = (a[2] + (s[2] << 1) + b[2]) - (a[0] + (s[0] << 1) + b[0]);
      val2 = (sf * abs(val)) >> 10;
      *xv = (UC8) __min(val2, 255);

      val = (a[0] + (a[1] << 1) + a[2]) - (b[0] + (b[1] << 1) + b[2]);
      val2 = (sf * abs(val)) >> 10;
      *yv = (UC8) __min(val2, 255);

      val = ((a[0] << 1) + a[1] + s[0]) - (s[2] + b[1] + (b[2] << 1));
      val2 = (sf * abs(val)) >> 10;
      *d1v = (UC8) __min(val2, 255);

      val = (a[1] + (a[2] << 1) + s[2]) - (s[0] + (b[0] << 1) + b[1]);
      val2 = (sf * abs(val)) >> 10;
      *d2v = (UC8) __min(val2, 255);
    }
    *xv++  = 0;
    *yv++  = 0;
    *d1v++ = 0;
    *d2v++ = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, xv++, yv++, d1v++, d2v++)
  {
    *xv  = 0;
    *yv  = 0;
    *d1v = 0;
    *d2v = 0;
  }
  return 1;
}


//= Figure out the dominant direction based on smoothed edges.
// returns 0 if below threshold, 64 major diagonal,   
// 128 horizontal, 192 minor diagonal, 255 vertical

int jhcEdge::DomDir (jhcImg& dest, const jhcImg& dx, const jhcImg& dy, 
                     const jhcImg& d1, const jhcImg& d2, int th) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(dx) || !dest.SameFormat(dy) ||
      !dest.SameFormat(d1) || !dest.SameFormat(d2))
    return Fatal("Bad images to jhcEdge::DomDir");
  dest.CopyRoi(dx);
  dest.MergeRoi(dy);
  dest.MergeRoi(d1);
  dest.MergeRoi(d2);

  // general ROI case
  int x, y, top;
  int rw = dest.RoiW(), rh = dest.RoiH(), sk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *vx = dx.RoiSrc(dest), *vy = dy.RoiSrc(dest);
  const UC8 *v1 = d1.RoiSrc(dest), *v2 = d2.RoiSrc(dest);

  // find and mark maximum at each pixel
  for (y = rh; y > 0; y--, d += sk, vx += sk, vy += sk, v1 += sk, v2 += sk)
    for (x = rw; x > 0; x--, d++, vx++, vy++, v1++, v2++)
    {
      top = __max(__max(*vx, *vy), __max(*v1, *v2));
      if (top < th)
        *d = 0;
      else if (*v1 == top)
        *d = 64;
      else if (*vx == top)
        *d = 128;
      else if (*v2 == top)
        *d = 192;
      else 
        *d = 255;
    }
  return 1;
}


//= Finds the top two directions over the specified threshold.
// normalizes so magnitude of these sum to 255, others zeroed
// useful for mixing together directional dependent image variants
// fills nej image with value vnej where no component is over th

int jhcEdge::DirMix (jhcImg& dx, jhcImg& dy, jhcImg& d1, jhcImg& d2, 
                     jhcImg& nej, int th, int vnej) const 
{
  jhcRoi dest;
  
  if (!dx.Valid(1) || !dx.SameFormat(dy) ||
      !dx.SameFormat(d1) || !dx.SameFormat(d2) || !dx.SameFormat(nej))
    return Fatal("Bad images to jhcEdge::DomMix");
  dest.CopyRoi(dx);
  dest.MergeRoi(dy);
  dest.MergeRoi(d1);
  dest.MergeRoi(d2);
  dx.CopyRoi(dest);
  dy.CopyRoi(dest);
  d1.CopyRoi(dest);
  d2.CopyRoi(dest);
  nej.CopyRoi(dest);

  // general ROI case
  int x, y, top, next, mix;
  int rw = dx.RoiW(), rh = dx.RoiH(), sk = dx.RoiSkip();
  UC8 *vx = dx.RoiDest(), *vy = dy.RoiDest();
  UC8 *v1 = d1.RoiDest(), *v2 = d2.RoiDest();
  UC8 *vn = nej.RoiDest();

  // find and mark maximum at each pixel
  for (y = rh; y > 0; y--, vx += sk, vy += sk, v1 += sk, v2 += sk, vn +=sk)
    for (x = rw; x > 0; x--, vx++, vy++, v1++, v2++, vn++)
    {
      // find maximum magnitude and make sure high enough 
      top = __max(__max(*vx, *vy), __max(*v1, *v2));
      if (top <= th)
      {
        *vx = 0;                
        *vy = 0;
        *v1 = 0;
        *v2 = 0;
        *vn = (UC8) vnej;
        continue;
      }
      mix = 255 * top;
      *vn = 0; 

      // check for D1 as dominant direction
      // make sure best neighboring direction above threshold
      if (*v1 == top)        
      {
        *v2 = 0;
        next = __max(*vx, *vy);  
        if (next <= th)
        {
          *vx = 0;
          *vy = 0;
          *v1 = 255;
          continue;
        }
        mix /= (top + next); 

        // see whether DX or DY is second most important
        if (*vx > *vy)       
        {
          *vy = 0;         
          *vx = (UC8)(255 - mix);
          *v1 = (UC8) mix;
        }
        else
        {
          *vx = 0;              
          *vy = (UC8)(255 - mix);
          *v1 = (UC8) mix;
        }
        continue;
      }

      // check for DX as dominant direction
      // make sure best neighboring direction above threshold
      if (*vx == top)        
      {
        *vy = 0;
        next = __max(*v1, *v2);  
        if (next <= th)
        {
          *v1 = 0;
          *v2 = 0;
          *vx = 255;
          continue;
        }
        mix /= (top + next); 

        // see whether D1 or D2 is second most important
        if (*v1 > *v2)       
        {
          *v2 = 0;         
          *v1 = (UC8)(255 - mix);
          *vx = (UC8) mix;
        }
        else
        {
          *v1 = 0;              
          *v2 = (UC8)(255 - mix);
          *vx = (UC8) mix;
        }
        continue;
      }

      // check for D2 as dominant direction
      // make sure best neighboring direction above threshold
      if (*v2 == top)        
      {
        *v1 = 0;
        next = __max(*vx, *vy);  
        if (next <= th)
        {
          *vx = 0;
          *vy = 0;
          *v2 = 255;
          continue;
        }
        mix /= (top + next); 

        // see whether DX or DY is second most important
        if (*vx > *vy)       
        {
          *vy = 0;         
          *vx = (UC8)(255 - mix);
          *v2 = (UC8) mix;
        }
        else
        {
          *vx = 0;              
          *vy = (UC8)(255 - mix);
          *v2 = (UC8) mix;
        }
        continue;
      }

      // if no other, then DY is  dominant direction
      // make sure best neighboring direction above threshold
      *vx = 0;
      next = __max(*v1, *v2);  
      if (next <= th)
      {
        *v1 = 0;
        *v2 = 0;
        *vy = 255;
        continue;
      }
      mix /= (top + next); 

      // see whether D1 or D2 is second most important
      if (*v1 > *v2)       
      {
        *v2 = 0;        
        *v1 = (UC8)(255 - mix);
        *vy = (UC8) mix;
      }
      else
      {
        *v1 = 0;              
        *v2 = (UC8)(255 - mix);
        *vy = (UC8) mix;
      }
    }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Edge Variants                                //
///////////////////////////////////////////////////////////////////////////

//= Computes three different kernels in three color fields.
// red gets Sobel vertical mask raw results
// green gets Sobel horizontal mask raw results
// blue gets 3x3 center surround results

int jhcEdge::TripleEdge (jhcImg& dest, const jhcImg& src) const 
{
  if (!dest.Valid(3) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcEdge::TripleEdge");
  dest.CopyRoi(src);

  // general ROI case
  int dx, dy,  cs;
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip() + 3, ssk = src.RoiSkip() + 2, ln = src.Line(); 
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;
  UC8 *d = dest.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, d += 3)
  {
    d[0] = 128;
    d[1] = 128;
    d[2] = 128;
  }
  d += dsk - 3;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, a += ssk, s += ssk, b += ssk)
  {
    d[0] = 128;
    d[1] = 128;
    d[2] = 128;
    d += 3;
    for (x = rw - 2; x > 0; x--, d += 3, a++, s++, b++) 
    {
      cs = (a[0] + a[1] + a[2]) + (s[0] - (s[1] << 3) + s[2]) + (b[0] + b[1] + b[2]);
      d[0] = (UC8)((cs + 2048) >> 4);
      dy = (a[0] + (a[1] << 1) + a[2]) - (b[0] + (b[1] << 1) + b[2]);
      d[1] = (UC8)((dy + 1024) >> 3);
      dx = (a[2] + (s[2] << 1) + b[2]) - (a[0] + (s[0] << 1) + b[0]);
      d[2] = (UC8)((dx + 1024) >> 3);
    }
    d[0] = 128;
    d[1] = 128;
    d[2] = 128;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, d += 3)
  {
    d[0] = 128;
    d[1] = 128;
    d[2] = 128;
  }
  return 1;
}


//= Gives unit vector components for all edges over threshold strength.

int jhcEdge::EdgeVect (jhcImg& unitx, jhcImg& unity, const jhcImg& src, int th) const 
{
  if (!src.Valid(1) || !src.SameFormat(unitx) || !src.SameFormat(unity))
    return Fatal("Bad images to jhcEdge::EdgeVect");
  unitx.CopyRoi(src);
  unity.CopyRoi(src);

  // general ROI case
  int val, mag, x, y, mx, my, rw = src.RoiW(), rh = src.RoiH();
  int ln = src.Line(), dsk = src.RoiSkip() + 1, ssk = dsk + 1;
  UC8 *dx = unitx.RoiDest(), *dy = unity.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, dx++, dy++)
  {
    *dx = 128;
    *dy = 128;
  }
  dx += dsk - 1;
  dy += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, dx += dsk, dy += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *dx++ = 128;
    *dy++ = 128;
    for (x = rw - 2; x > 0; x--, dx++, dy++, a++, s++, b++)
    {
      // get raw Sobel mask convolutions
      my = a[0] + (a[1] << 1) + a[2] - b[0] - (b[1] << 1) - b[2];
      mx = a[2] + (s[2] << 1) + b[2] - a[0] - (s[0] << 1) - b[0];

      // test magnitude of edge
      mag = (mx * mx + my * my) >> 4;
      if (mag < th)
      {
        *dx = 128;
        *dy = 128;
        continue;
      }

      // normalize edge vector components
      mag = root[abs(my) >> 2][abs(mx) >> 2] >> 6;
      val = (int)((127.0 * mx) / mag + 128.0);
      *dx = BOUND(val);
      val = (int)((127.0 * my) / mag + 128.0);
      *dy = BOUND(val);
    }
    *dx = 128;
    *dy = 128;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, dx++, dy++)
  {
    *dx = 128;
    *dy = 128;
  }
  return 1;
}


//= Returns angle (0 - 255) of edges above given threshold.
// also returns mask of where angle calculation is valid

int jhcEdge::DirMask (jhcImg& dir, jhcImg& mask, const jhcImg& src, int th) const 
{
  if (!src.Valid(1) || !src.SameFormat(dir) || src.SameImg(dir) ||
      !src.SameFormat(mask) || src.SameImg(mask))
    return Fatal("Bad images to jhcEdge::DirMask");
  dir.CopyRoi(src);
  mask.CopyRoi(src);

  // general ROI case
  int x, y, mx, my, val, rw = src.RoiW(), rh = src.RoiH();
  int ln = src.Line(), dsk = src.RoiSkip() + 1, ssk = dsk + 1;
  UC8 *d = dir.RoiDest(), *m = mask.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b + ln, *a = s + ln;

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  d += dsk - 1;
  m += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, m += dsk, d += dsk, a += ssk, s += ssk, b += ssk) 
  {
    *m++ = 0;
    *d++ = 0;
    for (x = rw - 2; x > 0; x--, m++, d++, a++, b++, s++)
    {
      // get raw Sobel mask convolutions
      my = (a[0] + (a[1] << 1) + a[2] - b[0] - (b[1] << 1) - b[2]) >> 2;
      mx = (a[2] + (s[2] << 1) + b[2] - a[0] - (s[0] << 1) - b[0]) >> 2;

      // test magnitude of edge
      val = mx * mx + my * my;
      if (val < th)
      {
        *d = 0;
        *m = 0;
        continue;
      }

      // compute angle from 0 - 255 
      if ((mx >= -128) && (mx < 128) && (my >= -128) && (my < 128))
        val = arct[my + 128][mx + 128];
      else
        val = arct[(my + 256) >> 1][(mx + 256) >> 1];
      *d = (UC8)(val >> 8);
      *m = 255;
    }
    *m = 0;
    *d = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Underlying Edge Functions                     //
///////////////////////////////////////////////////////////////////////////

//= Find root mean square value of components in image.
//   dest = sc * sqrt(dx * dx + dy * dy)
// assumes lifted values where 128 = zero

int jhcEdge::RMS (jhcImg &dest, const jhcImg& dx, const jhcImg& dy, double sc) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(dx) || !dest.SameFormat(dy))
    return Fatal("Bad images to jhcEdge::RMS");
  dest.CopyRoi(dx);
  dest.MergeRoi(dy);

  // general ROI case
  int x, y, xv, yv, val, sf = ROUND(sc * 256.0);
  int rw = dest.RoiW(), rh = dest.RoiH(), sk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *xval = dx.RoiSrc(dest), *yval = dy.RoiSrc(dest);

  // compute value for each pixel
  for (y = rh; y > 0; y--, d += sk, xval += sk, yval += sk) 
    for (x = rw; x > 0; x--, d++, xval++, yval++)
    {
      xv = abs(*xval - 128);
      yv = abs(*yval - 128);
      val = sf * root[xv][yv];
      *d = BOUND(val >> 16);
    }
  return 1;
}


//= Find angle corresponding to relative magnitude of components.
//   dest = sc * sqrt(dx * dx + dy * dy)
// assumes lifted values where 128 = zero
// angle encoded so that 256 = 360 degrees (255 = 358.6 degs)

int jhcEdge::Angle (jhcImg &dest, const jhcImg& dx, const jhcImg& dy) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(dx) || !dest.SameFormat(dy))
    return Fatal("Bad images to jhcEdge::Angle");
  dest.CopyRoi(dx);
  dest.MergeRoi(dy);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), sk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *xval = dx.RoiSrc(dest), *yval = dy.RoiSrc(dest);

  // compute value for each pixel
  for (y = rh; y > 0; y--, d += sk, xval += sk, yval += sk) 
    for (x = rw; x > 0; x--, d++, xval++, yval++)
      *d = (UC8)(arct[*yval][*xval] >> 8);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                             Bar Finding                               //
///////////////////////////////////////////////////////////////////////////

//= Finds thin, bright, bar-like features in image.
// use a negative scale factor to find dark lines
// uses masks:
// <pre>
//        -1 -2 -1          -1  2 -1
//  d2y =  2  4  2   d2x =  -2  4 -2
//        -1 -2 -1          -1  2 -1
// </pre>
// magnitude scaled (by 1/sqrt(2)) to fit in range [0 255]
// angle ranges from 0 degs (0) to 90 degs (255)

int jhcEdge::SobelBar (jhcImg& mag, jhcImg& dir, const jhcImg& src, double sc) const 
{
  if (!mag.Valid(1) || !mag.SameFormat(dir) || !mag.SameFormat(src))
    return Fatal("Bad images to jhcEdge::SobelBar");
  mag.CopyRoi(src);
  dir.CopyRoi(src);

  // general ROI case
  int val, ang, sf = ROUND(fabs(sc) * 256.0);
  int x, y, d2x, d2y, c2x, c2y, rw = mag.RoiW(), rh = mag.RoiH();
  int ln = src.Line(), ln2 = ln << 1, dsk = mag.RoiSkip() + 1, ssk = dsk + 1;
  const UC8 *s = src.RoiSrc();
  UC8 *m = mag.RoiDest(), *d = dir.RoiDest();

  // cannot compute value for bottom line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m = 0;
    *d = 0;
  }
  m += dsk - 1;
  d += dsk - 1;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, m += dsk, d += dsk, s += ssk) 
  {
    // left edge
    *m++ = 0;
    *d++ = 0;

    // bulk in middle
    for (x = rw - 2; x > 0; x--, m++, d++, s++)
    {
      // horizontally oriented mask
      d2y  = (s[ln] + (s[ln + 1] << 1) + s[ln + 2]) << 1;
      d2y -= (s[0] + (s[1] << 1) + s[2]) + (s[ln2] + (s[ln2 + 1] << 1) + s[ln2 + 2]);
      d2y >>= 3;

      // verticaly oriented mask
      d2x  = (s[1] + (s[ln + 1] << 1) + s[ln2 + 1]) << 1;
      d2x -= (s[0] + (s[ln] << 1) + s[ln2]) + (s[2] + (s[ln + 2] << 1) + s[ln2 + 2]);
      d2x >>= 3;

      // possibly look for dark bars instead
      if (sc > 0.0)
      {
        c2y = __max(0, d2y);
        c2x = __max(0, d2x);
      }
      else
      {
        c2y = __max(0, -d2y);
        c2x = __max(0, -d2x);
      }

      // figure out magnitude
      val = (sf * root[c2y][c2x]) >> 16;
      *m = (UC8) __min(val, 255);

      // figure out direction (preserve small difference for better accuracy)
      if (val <= 0)
        ang = 0;
      else if ((c2x < 128) && (c2y < 128))
        ang = (arct[128 + c2y][128 - c2x]) << 2;
      else
        ang = (arct[(256 + c2y) >> 1][(256 - c2x) >> 1]) << 2;
      *d = (UC8)(ang >> 8);
    }

    // right edge
    *m = 0;
    *d = 0;
  }

  // cannot compute value for top line of image
  for (x = rw; x > 0; x--, m++, d++)
  {
    *m  = 0;
    *d = 0;
  }
  return 1;
}


