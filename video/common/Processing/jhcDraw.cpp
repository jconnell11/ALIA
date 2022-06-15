// jhcDraw.cpp : simple routines for drawing graphics shapes inside images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
// Copyright 2022 Etaoin Systems
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

#include "Processing/jhcDraw.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Constructor sets up some standard values.

jhcDraw::jhcDraw ()
{
  ej_clip = 0;
}


//= Set whether to smash out of range lines onto edge of image.
// ej_clip > 0 smashes, ej_clip <= 0 just does not draw them
// returns previous value, used by DrawLine and CircleEmpty

int jhcDraw::DrawClip (int doit)
{
  int c0 = ej_clip;

  ej_clip = doit;
  return c0;
}


///////////////////////////////////////////////////////////////////////////
//                           Color Shifting                              //
///////////////////////////////////////////////////////////////////////////

//= Method for choosing color to draw with based on a single number.
//   0 = black,   1 = red,  2 = green, 3 = yellow, 4 = blue, 
//   5 = magenta, 6 = aqua, 7 = white, 8 = black again, etc.

void jhcDraw::Color8 (UC8 *r, UC8 *g, UC8 *b, int i, int nf) const
{
  UC8 cols[8] = {0, 200, 128, 230, 50, 215, 70, 255};

  if (nf == 1)
    *r = cols[i & 0x07];
  else
  {
    *r = (UC8)(((i & 0x01) == 0) ? 0 : 255);
    *g = (UC8)(((i & 0x02) == 0) ? 0 : 255);
    *b = (UC8)(((i & 0x04) == 0) ? 0 : 255);
  }
}


//= Color is determined from a numeric index.

UC8 jhcDraw::ColorN (int i) const
{
  return((UC8)(((i << 5) & 0xE0) | ((i >> 3) & 0x1F)));
}


//= Copies field N of source but swaps upper and lower portion.
// Useful for displaying connected components since adjacent 
//   blobs will tend to come out in different color bands.

int jhcDraw::Scramble (jhcImg& dest, const jhcImg& src, int field) const 
{
  int v, nf = src.Fields();

  if (!dest.Valid(1) || !dest.SameSize(src))
    return Fatal("Bad images to jhcDraw::Scramble");
  if ((field < 0) || (field > nf))
    return 0;
  dest.CopyRoi(src);

  // general ROI case
  int i, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc() + field;
  UC8 scr[256];

  // build lookup table swapping bits
  scr[0] = 0;
  for (i = 1; i < 256; i++)
  {
    v = ((i % 14) + 1) << 4;    // no black or white
    v |= (i >> 4);
    scr[i] = v & 0xFF;
  }

  // apply table to image
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s += nf)
      *d = scr[*s];             // was ColorN(*s)
  return 1;
}


//= Generates distinct color for adjacent values in some field of input image.
// designed to overlay other graphics and only alters pixels where source value is non-zero

int jhcDraw::ScrambleNZ (jhcImg& dest, const jhcImg& src, int field) const 
{
  int v, nf = src.Fields();

  if (!dest.Valid(1) || !dest.SameSize(src))
    return Fatal("Bad images to jhcDraw::ScrambleNZ");
  if ((field < 0) || (field > nf))
    return 0;
  dest.CopyRoi(src);

  // general ROI case
  int i, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc() + field;
  UC8 scr[256];

  // build lookup table swapping bits
  scr[0] = 0;
  for (i = 1; i < 256; i++)
  {
    v = ((i % 14) + 1) << 4;    // no black or white
    v |= (i >> 4);
    scr[i] = v & 0xFF;
  }

  // apply table to image
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s += nf)
      if (*s != 0)
        *d = scr[*s];
  return 1;
}


//= Shows 8 discrete bands in an RGB image (like "show" with an argument of 2).
// for testing thresholds in a gray image

int jhcDraw::FalseColor (jhcImg& dest, const jhcImg& src) const 
{
  if (!dest.Valid(3) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcDraw::FalseColor");
  dest.CopyRoi(src);

  // general ROI case
  UC8 r[16] = { 70,   0,  72,   0,  30, 135,  34, 107,
                          50, 154, 205, 255, 255, 255, 255, 255};
  UC8 g[16] = { 70,   0,  61,   0, 144, 206, 139, 142,
                         205, 205, 133, 165,   0,   0, 255, 255};
  UC8 b[16] = { 70, 128, 139, 255, 255, 250,  34,  35,
                          50,  50,  63,   0,   0, 255,   0, 255};
  int v, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d += 3, s++)
      if (*s == 0)
	  {
        d[0] = 0;
        d[1] = 0;
        d[2] = 0;
	  }
	  else
      {
        v = (*s) >> 4;
        d[0] = b[v];
        d[1] = g[v];
        d[2] = r[v];
      }
  return 1;
}


//= Converts monochrome image to RGB color where bottom 3 bits choose color.

int jhcDraw::IndexColor (jhcImg& dest, const jhcImg& src) const 
{
  if (!dest.Valid(3) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcDraw::IndexColor");
  dest.CopyRoi(src);

  // general ROI case
  UC8 r[8] = {  0, 255,   0, 255,   0, 255,   0, 255};
  UC8 g[8] = {  0,   0, 255, 255,   0,   0, 255, 255};
  UC8 b[8] = {  0,   0,   0,   0, 255, 255, 255, 255};
  int v, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, s++, d += 3)
    {
      v = s[0] & 0x07;
      d[0] = b[v];
      d[1] = g[v];
      d[2] = r[v];
    }
    s += ssk;
    d += dsk;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Filled Shapes                               //
///////////////////////////////////////////////////////////////////////////

//= Draw an axis parallel filled rectangle into image.
// clips illegal values to yield reasonable result
// ignores destination ROI, bot is uppermost corner in display
// works for RGB or monochrome image (g and b ignored)
// if r is negative, picks RGB color based on magnitude
//   0 = black,  -1 = red,  -2 = green, -3 = yellow, -4 = blue, 
//  -5 = purple, -6 = aqua, -7 = white, -8 = black again, etc.

int jhcDraw::RectFill (jhcImg& dest, int left, int bot, int w, int h, int r, int g, int b) const 
{
  jhcRoi src(dest);

  src.SetRoi(left, bot, w, h);
  return RectFill(dest, src, r, g, b);
}


//= Like other RectFill but takes position and size in an array: {x, y, w, h}.

int jhcDraw::RectFill (jhcImg& dest, const int *specs, int r, int g, int b) const 
{
  jhcRoi src(dest);

  src.SetRoi(specs);
  return RectFill(dest, src, r, g, b);
}

      
//= Same as other RectFill but takes position and size from a ROI.
// source ROI's clipping range readjusted to match image

int jhcDraw::RectFill (jhcImg& dest, const jhcRoi& src, int r, int g, int b) const 
{
  int nf = dest.Fields();

  if (nf == 2)
    return RectFill_16(dest, src, r);
  if (!dest.Valid())
    return Fatal("Bad image to jhcDraw::RectFill");
  if ((nf != 1) && (nf != 3))
    return 0;

  // draw anywhere in image
  jhcRoi s2;
  int x, y, wid, ht, rsk;
  UC8 red = BOUND(r), grn = BOUND(g), blu = BOUND(b);
  UL32 roff;
  UC8 *d;

  // automatically choose drawing color
  if (r < 0) 
    Color8(&red, &grn, &blu, -r, nf);

  // find start and size of clipped rectangle
  s2.CopyRoi(src);
  s2.RoiClip(dest);
  wid = s2.RoiW();
  ht  = s2.RoiH();
  dest.RoiParams(&roff, &rsk, s2);
  d = dest.PxlDest() + roff;

  // write rectangle interior
  if (nf == 3)                         // RGB case
    for (y = ht; y > 0; y--)
    {
      for (x = wid; x > 0; x--)
      {
        d[0] = blu;
        d[1] = grn;
        d[2] = red;
        d += 3;
      }
      d += rsk;
    }
  else                                 // monochrome case
    for (y = ht; y > 0; y--)
    {
      for (x = wid; x > 0; x--)
        *d++ = red;
      d += rsk;
    }
  return 1;
}


//= Sets some block of pixels to a particular value in a 16 bit image.

int jhcDraw::RectFill_16 (jhcImg& dest, const jhcRoi& src, int val) const 
{
  jhcRoi s2;
  int x, y, wid, ht, rsk;
  US16 v = (US16) __max(0, __min(val, 65535));
  UL32 roff;
  US16 *d;

  if (!dest.Valid(2))
    return Fatal("Bad image to jhcDraw::RectFill_16");

  // find start and size of clipped rectangle
  s2.CopyRoi(src);
  s2.RoiClip(dest);
  wid = s2.RoiW();
  ht  = s2.RoiH();
  dest.RoiParams(&roff, &rsk, s2);
  rsk >>= 1;
  d = (US16 *)(dest.PxlDest() + roff);

  // write pixels
  for (y = ht; y > 0; y--, d += rsk)
    for (x = wid; x > 0; x--, d++)
      *d = v;
  return 1;
}


//= Draw a filled rectangle centered on given coordinates.

int jhcDraw::BlockCent (jhcImg& dest, int xc, int yc, int w, int h, int r, int g, int b) const
{
  return RectFill(dest, xc - (w / 2), yc - (h / 2), w, h, r, g, b);
}


//= Draw a filled rectangle centered on given coordinates tilted by some angle.
// can optionally set ROI of destination image tight around rectangle created
// checks of form: (x - xa) * (yb - ya) > (y - ya) * (xb - xa) for bounding line [xa ya]->[xb yb]
// computes sum = x * dy - y * dx - th where dx = xb - xa, dy = yb - ya, th = xa * dy - ya * dx 
// between 15-30x slower than BlockCent depending on angle
// NOTE: "w" is size along "degs" (since w = x dimension when degs = 0)

int jhcDraw::BlockRot (jhcImg& dest, double xc, double yc, double w, double h, double degs, int r, int g, int b, int set) const
{
  double d0 = fmod(degs, 180.0); 
  int ix = ROUND(xc), iy = ROUND(yc), iw = ROUND(w), ih = ROUND(h);

  // check for simple cases
  if (d0 == 0.0)
  {
    if (set > 0)
      dest.SetRoi(ix, iy, iw, ih);
    return BlockCent(dest, ix, iy, iw, ih, r, g, b);
  }
  if (d0 == 90.0)
  {
    if (set > 0)
      dest.SetRoi(ix, iy, ih, iw);
    return BlockCent(dest, ix, iy, ih, iw, r, g, b);
  }

  // general angle case
  double w2 = 0.5 * w, h2 = 0.5 * h, rads = D2R * degs, c = cos(rads), s = sin(rads);
  double w2c = w2 * c, w2s = w2 * s, h2c = h2 * c, h2s = h2 * s;
  double nwx = xc - w2c - h2s, nwy = yc - w2s + h2c, nex = xc + w2c - h2s, ney = yc + w2s + h2c;
  double swx = xc - w2c + h2s, swy = yc - w2s - h2c, sex = xc + w2c + h2s, sey = yc + w2s - h2c; 

  return FillPoly4(dest, nwx, nwy, nex, ney, sex, sey, swx, swy, r, g, b, set);
}


//= Draw a filled 4 sided polygon with vertexes in clockwise order.
// actual position of points does not matter (e.g. could start with SW) only their order
// counterclockwise order fills parts of bounding box (not whole image) outside of polygon
// can optionally set ROI of destination image tight around shape created

int jhcDraw::FillPoly4 (jhcImg& dest, double nwx, double nwy, double nex, double ney, 
                        double sex, double sey, double swx, double swy, int r, int g, int b, int set) const
{
  double v, dx1 = nex - nwx, dy1 = ney - nwy, dx2 = sex - swx, dy2 = sey - swy;
  double v2, dx3 = nex - sex, dy3 = ney - sey, dx4 = nwx - swx, dy4 = nwy - swy;
  int idx1 = ROUND(1024.0 * dx1), idx2 = ROUND(1024.0 * dx2);
  int idx3 = ROUND(1024.0 * dx3), idx4 = ROUND(1024.0 * dx4);
  int idy1 = ROUND(1024.0 * dy1), idy2 = ROUND(1024.0 * dy2);
  int idy3 = ROUND(1024.0 * dy3), idy4 = ROUND(1024.0 * dy4);
  int x, y, x0, x1, y0, y1, rw, rh, sk, f = dest.Fields();
  int is1_0, is2_0, is3_0, is4_0, isum1, isum2, isum3, isum4;
  UC8 red = BOUND(r), grn = BOUND(g), blu = BOUND(b);
  UC8 *d;

  // check for good inputs and automatically pick drawing color           
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::FillPoly4");
  if (r < 0)                                           
    Color8(&red, &grn, &blu, -r, dest.Fields());    

  // get scanning region limits
  v  = __min(nex, sex);
  v2 = __min(nwx, swx);
  v  = __min(v, v2); 
  x0 = (int) floor(v);
  x0 = __max(0, x0);
  v  = __min(ney, sey);
  v2 = __min(nwy, swy);
  v  = __min(v, v2); 
  y0 = (int) floor(v);
  y0 = __max(0, y0);
  v  = __max(nex, sex);
  v2 = __max(nwx, swx);
  v  = __max(v, v2); 
  x1 = (int) ceil(v);
  x1 = __min(x1, dest.XLim());
  v  = __max(ney, sey);
  v2 = __max(nwy, swy);
  v  = __max(v, v2); 
  y1 = (int) ceil(v);
  y1 = __min(y1, dest.YLim());
  rw = x1 - x0 + 1;
  rh = y1 - y0 + 1;

  // get starting values for sums (-th) wrt x0,y0 corner
  is1_0 = ROUND(1024.0 * ((nwy - y0) * dx1 - (nwx - x0) * dy1));
  is2_0 = ROUND(1024.0 * ((swy - y0) * dx2 - (swx - x0) * dy2));
  is3_0 = ROUND(1024.0 * ((sey - y0) * dx3 - (sex - x0) * dy3));
  is4_0 = ROUND(1024.0 * ((swy - y0) * dx4 - (swx - x0) * dy4));

  // scan all pixels in region and see if pixel inside all side lines
  d = dest.RoiDest(x0, y0);
  sk = dest.RoiSkip(rw);
  for (y = rh; y > 0; y--, d += sk, is1_0 -= idx1, is2_0 -= idx2, is3_0 -= idx3, is4_0 -= idx4)
  {
    isum1 = is1_0;
    isum2 = is2_0;
    isum3 = is3_0;
    isum4 = is4_0;
    if (f == 1)
    {
      // monochrome
      for (x = rw; x > 0; x--, d++, isum1 += idy1, isum2 += idy2, isum3 += idy3, isum4 += idy4)
        if ((isum1 > 0) && (isum2 < 0) && (isum3 < 0) && (isum4 > 0))
          *d = red;
    }
    else
      for (x = rw; x > 0; x--, d++, isum1 += idy1, isum2 += idy2, isum3 += idy3, isum4 += idy4)
        if ((isum1 > 0) && (isum2 < 0) && (isum3 < 0) && (isum4 > 0))
        {
          // color
          d[0] = blu;
          d[1] = grn;
          d[2] = red;
        }
  }

  // possibly set destination ROI
  if (set > 0)
    dest.SetRoi(x0, y0, rw, rh);
  return 1;
}


//= Specialization of RectFill to fill whole image.
// only changes pixels in ROI

int jhcDraw::Clear (jhcImg& dest, int r, int g, int b) const 
{
  return RectFill(dest, dest, r, g, b);
}


//= Fill in everything except ROI area with some color.

int jhcDraw::Matte (jhcImg& dest, const jhcRoi& src, int r, int g, int b) const 
{
  if (dest.Valid(3))
    return MatteRGB(dest, src, r, g, b);
  if (!dest.Valid(1))
    return Fatal("Bad image to jhcDraw::Matte");

  // general ROI case
  jhcRoi s2;
  int x, y, rx, ry, rw, rh, rest;
  int w = dest.XDim(), h = dest.YDim(), skip = dest.Skip();
  UC8 val = BOUND(r);
  UC8 *d = dest.PxlDest();

  // find start and size of clipped rectangle
  s2.CopyRoi(src);
  s2.RoiClip(dest);
  rx = s2.RoiX();
  ry = s2.RoiY();
  rw = s2.RoiW();
  rh = s2.RoiH();
  rest = w - rx - rw;

  // write bottom band
  for (y = ry; y > 0; y--)
  {
    for (x = w; x > 0; x--)
      *d++ = val;
    d += skip;
  }

  // write side bars on left and right
  for (y = rh; y > 0; y--)
  {
    for (x = rx; x > 0; x--)
      *d++ = val;
    d += rw;
    for (x = rest; x > 0; x--)
      *d++ = val;
    d += skip;
  }

  // write top band
  for (y = h - ry - rh; y > 0; y--)
  {
    for (x = w; x > 0; x--)
      *d++ = val;
    d += skip;
  }
  return 1;
}


//= Same as Matte but for RGB color images only.
// negative value for red causes function to choose auto-color 

int jhcDraw::MatteRGB (jhcImg& dest, const jhcRoi& src, int r, int g, int b) const 
{
  if (!dest.Valid(3))
    return Fatal("Bad image to jhcDraw::MatteRGB");

  // RGB color case 
  jhcRoi s2;
  int x, y, rx, ry, rw, rh, rw3, rest;
  int w = dest.XDim(), h = dest.YDim(), skip = dest.Skip();
  UC8 red = BOUND(r), grn = BOUND(g), blu = BOUND(b);
  UC8 *d = dest.PxlDest();

  // automatically choose drawing color
  if (r < 0)
    Color8(&red, &grn, &blu, -r, 3);

  // find start and size of clipped rectangle
  s2.CopyRoi(src);
  s2.RoiClip(dest);
  rx = s2.RoiX();
  ry = s2.RoiY();
  rw = s2.RoiW();
  rh = s2.RoiH();
  rw3 = 3 * rw;
  rest = w - rx - rw;

  // write bottom band
  for (y = ry; y > 0; y--)
  {
    for (x = w; x > 0; x--)
    {
      d[0] = blu;
      d[1] = grn;
      d[2] = red;
      d += 3;
    }
    d += skip;
  }

  // write side bars on left and right
  for (y = rh; y > 0; y--)
  {
    for (x = rx; x > 0; x--)
    {
      d[0] = blu;
      d[1] = grn;
      d[2] = red;
      d += 3;
    }
    d += rw3;
    for (x = rest; x > 0; x--)
    {
      d[0] = blu;
      d[1] = grn;
      d[2] = red;
      d += 3;
    }
    d += skip;
  }

  // write top band
  for (y = h - ry - rh; y > 0; y--)
  {
    for (x = w; x > 0; x--)
    {
      d[0] = blu;
      d[1] = grn;
      d[2] = red;
      d += 3;
    }
    d += skip;
  }
  return 1;
}


//= Draw a filled circle of a given size centered on a certain point.
// works with both RGB and monochrome images (g and b ignored)
// if r is negative, picks RGB color based on absolute value of r

int jhcDraw::CircleFill (jhcImg& dest, double xc, double yc, double radius, 
                         int r, int g, int b) const 
{
  double dx, dy, chord, rd = radius - 0.5, r2 = radius * radius;
  int px, nx, py, ny;
  UC8 red = BOUND(r), grn = BOUND(g), blu = BOUND(b);

  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::CircleFill");
  if (radius <= 0.0)
    return 1;

  // automatically pick drawing color
  if (r < 0)
    Color8(&red, &grn, &blu, -r, dest.Fields());

  // monochrome case
  if (dest.Valid(1))
  {
    for (dy = 0.0; dy <= rd; dy += 1.0)
    {
      chord = sqrt(r2 - dy * dy) - 0.5;
      ny = ROUND(yc - dy);
      py = ROUND(yc + dy);
      for (dx = 0.0; dx <= chord; dx += 1.0)
      {
        nx = ROUND(xc - dx);
        px = ROUND(xc + dx);
        dest.ASetOK(nx, ny, red);
        dest.ASetOK(px, ny, red);
        dest.ASetOK(nx, py, red);
        dest.ASetOK(px, py, red);
      }
    }
    return 1;
  }

  // color case
  for (dy = 0.0; dy <= rd; dy += 1.0)
  {
    chord = sqrt(r2 - dy * dy) - 0.5;
    ny = ROUND(yc - dy);
    py = ROUND(yc + dy);
    for (dx = 0.0; dx <= chord; dx += 1.0)
    {
      nx = ROUND(xc - dx);
      px = ROUND(xc + dx);
      dest.ASetColOK(nx, ny, red, grn, blu);
      dest.ASetColOK(px, ny, red, grn, blu);
      dest.ASetColOK(nx, py, red, grn, blu);
      dest.ASetColOK(px, py, red, grn, blu);
    }
  }
  return 1;
}


//= Fill image some fraction horizontally from left with given color.
// useful for bargraph displays, generally fill with background first

int jhcDraw::FillH (jhcImg& dest, double frac, int r, int g, int b) const
{
  // check arguments and special cases
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::FillH");
  if ((frac >= 1.0) && dest.Valid(3))
    return dest.FillRGB(r, g, b);
  if ((frac >= 1.0) && dest.Valid(1))
    return dest.FillArr(r);
  if (frac <= 0.0)
    return 1;

  // local variables
  UC8 *d, *d0 = dest.PxlDest();
  int x, y, fw = ROUND(frac * dest.XDim()), h = dest.YDim(), ln = dest.Line();

  if (dest.Valid(1))
    for (y = h; y > 0; y--, d0 += ln)
    {
      // monochrome
      d = d0;
      for (x = fw; x > 0; x--, d++)
        *d = (UC8) r;
    } 
  else
    for (y = h; y > 0; y--, d0 += ln)
    {
      // color
      d = d0;
      for (x = fw; x > 0; x--, d += 3)
      {
        d[0] = (UC8) b; 
        d[1] = (UC8) g; 
        d[2] = (UC8) r;
      }
    } 
  return 1;
}


//= Fill image some fraction vertically from bottom with given color.
// useful for bargraph displays, generally fill with background first

int jhcDraw::FillV (jhcImg& dest, double frac, int r, int g, int b) const
{
  // check arguments and special cases
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::FillV");
  if ((frac >= 1.0) && dest.Valid(3))
    return dest.FillRGB(r, g, b);
  if ((frac >= 1.0) && dest.Valid(1))
    return dest.FillArr(r);
  if (frac <= 0.0)
    return 1;

  // local variables
  UC8 *d = dest.PxlDest();
  int x, y, w = dest.XDim(), fh = ROUND(frac * dest.YDim()), sk = dest.Skip();

  if (dest.Valid(1))
    for (y = fh; y > 0; y--, d += sk)
      for (x = w; x > 0; x--, d++)
        *d = (UC8) r;
  else
    for (y = fh; y > 0; y--, d += sk)
      for (x = w; x > 0; x--, d += 3)
      {
        d[0] = (UC8) b; 
        d[1] = (UC8) g; 
        d[2] = (UC8) r;
      }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Outline Shapes                              //
///////////////////////////////////////////////////////////////////////////

//= Same as other RectEmpty but takes position and size from a ROI.

int jhcDraw::RectEmpty (jhcImg& dest, const jhcRoi& s, int t, int r, int g, int b) const 
{
  return RectEmpty(dest, s.RoiX(), s.RoiY(), s.RoiW(), s.RoiH(), t, r, g, b);
}


//= Like other RectEmpty but takes fractions of image dimensions.

int jhcDraw::RectEmpty (jhcImg& dest, double lfrac, double bfrac, double wfrac, double hfrac, 
                        int t, int r, int g, int b) const 
{
  int iw = dest.XDim(), ih = dest.YDim();

  return RectEmpty(dest, ROUND(lfrac * iw), ROUND(bfrac * ih), 
                         ROUND(wfrac * iw), ROUND(hfrac * ih), t, r, g, b);
}


//= Like other RectEmpty but takes position and size in an array: {x, y, w, h}.

int jhcDraw::RectEmpty (jhcImg& dest, const int *specs, int t, int r, int g, int b) const 
{
  return RectEmpty(dest, specs[0], specs[1], specs[2], specs[3], t, r, g, b);
}


//= Draws an axis parallel rectangle outline into image.
// lines draw with thickness t (outside is still w by h)
// (lx by) is southwest corner of rectangle, ignores image ROI
// works with both RGB and monochrome images (g and b ignored)
// if r is negative, picks RGB color based on magnitude

int jhcDraw::RectEmpty (jhcImg& dest, int lx, int by, int w, int h, 
                        int t, int r, int g, int b) const 
{
  int dsk, iw = dest.XDim(), ih = dest.YDim(), nf = dest.Fields();
  int x, lf0, lf1, rt0, rt1, x0 = lx - t, x1 = lx, x2 = lx + w, x3 = x2 + t;
  int y, bot0, bot1, top0, top1, y0 = by - t, y1 = by, y2 = by + h, y3 = y2 + t;
  UC8 red = BOUND(r), grn = BOUND(g), blu = BOUND(b);
  UC8 *d;
 
  // check argument and null cases
  if (!dest.Valid(1) && !dest.Valid(3))
    return Fatal("Bad image to jhcDraw::RectEmpty");
  if ((w <= 0) || (h <= 0) || (t == 0))                // t < 0 draws inside border
    return 1;

  // automatically choose drawing color
  if (r < 0)
    Color8(&red, &grn, &blu, -r, nf);

  // draw inside if thickness negative
  lf0  = __min(x0, x1);
  lf1  = __max(x0, x1);
  rt0  = __min(x2, x3);
  rt1  = __max(x2, x3);
  bot0 = __min(y0, y1);
  bot1 = __max(y0, y1);
  top0 = __min(y2, y3);
  top1 = __max(y2, y3);

  // clip coordinates to image 
  lf0  = __max(0, __min(lf0,  iw));
  lf1  = __max(0, __min(lf1,  iw));
  rt0  = __max(0, __min(rt0,  iw));
  rt1  = __max(0, __min(rt1,  iw));
  bot0 = __max(0, __min(bot0, ih));
  bot1 = __max(0, __min(bot1, ih));
  top0 = __max(0, __min(top0, ih));
  top1 = __max(0, __min(top1, ih));

  // handle very thick lines
  rt0  = __max(lf0,  rt0);
  top0 = __max(bot0, top0);  
  lf1  = __min(lf1,  rt1);
  bot1 = __min(bot1, top1);

  // draw bottom border 
  d = dest.RoiDest(lf0, bot0);
  dsk = dest.RoiSkip(rt1 - lf0);
  for (y = bot0; y < bot1; y++, d += dsk)
    for (x = lf0; x < rt1; x++)
      if (nf == 1)
        *d++ = red;
      else
      {
        d[0] = blu;
        d[1] = grn;
        d[2] = red;
        d += 3;
      }

  // draw left border in RGB
  d = dest.RoiDest(lf0, bot1);
  dsk = dest.RoiSkip(lf1 - lf0);
  for (y = bot1; y < top0; y++, d += dsk)
    for (x = lf0; x < lf1; x++)
      if (nf == 1)
        *d++ = red;
      else
      {
        d[0] = blu;
        d[1] = grn;
        d[2] = red;
        d += 3;
      }

  // draw right border 
  d = dest.RoiDest(rt0, bot1);
  dsk = dest.RoiSkip(rt1 - rt0);
  for (y = bot1; y < top0; y++, d += dsk)
    for (x = rt0; x < rt1; x++)
      if (nf == 1)
        *d++ = red;
      else
      {
        d[0] = blu;
        d[1] = grn;
        d[2] = red;
        d += 3;
      }

  // draw top border 
  d = dest.RoiDest(lf0, top0);
  dsk = dest.RoiSkip(rt1 - lf0);
  for (y = top0; y < top1; y++, d += dsk)
    for (x = lf0; x < rt1; x++)
      if (nf == 1)
        *d++ = red;
      else
      {
        d[0] = blu;
        d[1] = grn;
        d[2] = red;
        d += 3;
      }
  return 1;
}


//= Specialization of RectEmpty to draw a border around active part of image.
// generally want t < 0 to draw in from edges of image, t > 0 for outside ROI

int jhcDraw::Border (jhcImg& dest, int t, int r, int g, int b) const 
{
  return RectEmpty(dest, dest, t, r, g, b);
}


//= Only draws vertical borders (left and right sides).
// t > 0 draws inside image (unlike Border)

int jhcDraw::BorderV (jhcImg& dest, int t, int v) const 
{
  if (!dest.Valid(1))
    return Fatal("Bad image to jhcDraw::BorderV");

  // local variables
  int x, y, w = dest.XDim(), h = dest.YDim(), ln = dest.Line();
  UC8 *d;

  // left and right boundary
  d = dest.PxlDest();
  for (y = h; y > 0; y--)
  {
    for (x = 0; x < t; x++)
    {
      d[x] = (UC8) v;
      d[(w - 1) - x] = (UC8) v;
    }
    d += ln;
  }
  return 1;
}


//= Only draws horizontal borders (top and bottom sides).
// t > 0 draws inside image (unlike Border)

int jhcDraw::BorderH (jhcImg& dest, int t, int v) const 
{
  if (!dest.Valid(1))
    return Fatal("Bad image to jhcDraw::BorderH");

  // local variables
  int x, y, w = dest.XDim(), h = dest.YDim(), sk = dest.Skip();
  UC8 *d;

  // bottom boundary
  d = dest.PxlDest();
  for (y = 0; y < t; y++) 
  {
    for (x = w; x > 0; x--, d++)
      *d = (UC8) v;
    d += sk;
  }

  // top boundary
  d = dest.RoiDest(0, h - t);
  for (y = 0; y < t; y++) 
  {
    for (x = w; x > 0; x--, d++)
      *d = (UC8) v;
    d += sk;
  }
  return 1;
}


//= Copies image just inside border to outer border pixels.

int jhcDraw::BorderExt (jhcImg& dest) const 
{
  if (!dest.Valid())
    return Fatal("Bad image to jhcDraw::BorderExt");

  int x, y, i, ln = dest.Line();
  int w = dest.XDim(), h = dest.YDim(), f = dest.Fields();
  UC8 *d, *d2;

  // copy top and bottom lines
  d = dest.PxlDest();
  d2 = d + (h - 2) * ln;
  for (x = w * f; x > 0; x--)
  {
    d[0]   = d[ln];
    d2[ln] = d2[0];
    d++;
    d2++;
  }

  // copy side lines
  d = dest.PxlDest();
  d2 = d + (w - 2) * f;
  for (y = h; y > 0; y--)
    for (i = f; i > 0; i--)
    {
      d[0]  = d[f];
      d2[f] = d2[0];
      d  += ln;
      d2 += ln;
  }
  return 1;
}


//= Copy pixels n in from the edge all the way to the edge.
// useful for some BoxAvg operations to get correct repeating boundary
// similar to BorderExt but specialized to monochrome images

int jhcDraw::EdgeDup (jhcImg& dest, int n) const 
{
  if (!dest.Valid(1))
    return Fatal("Bad image to jhcDraw::EdgeDup");
  if (n <= 0)
    return 1;

  // local variables
  int i, x, y, w = dest.XDim(), h = dest.YDim(), ln = dest.Line();
  int xlim = w - 1, xsrc = xlim - n;
  const UC8 *s;
  UC8 *d;

  // bottom boundary
  s = dest.RoiSrc(0, n);
  d = dest.RoiDest(0, 0);
  for (i = n; i > 0; i--, d += ln)
    for (x = 0; x < w; x++)
      d[x] = s[x];

  // left and right boundary
  s = dest.RoiSrc(0, 0);
  d = dest.RoiDest(0, 0);
  for (y = h - n; y > 0; y--, s += ln, d += ln)
    for (i = 0; i < n; i++)
    {
      d[i]        = s[n];
      d[xlim - i] = s[xsrc];
    }

  // top boundary
  s = dest.RoiSrc(0, h - n - 1);
  d = dest.RoiDest(0, h - n);
  for (i = n; i > 0; i--, d += ln)
    for (x = 0; x < w; x++)
      d[x] = s[x];

  return 1;
}


//= Copy pixels n in from some edge all the way to the edge.
// side: 0 = left, 1 = top, 2 = right, 3 = bottom

int jhcDraw::SideDup (jhcImg& dest, int side, int n) const 
{
  if (!dest.Valid(1))
    return Fatal("Bad image to jhcDraw::SideDup");
  if (n <= 0)
    return 1;

  // local variables
  int i, x, y, w = dest.XDim(), h = dest.YDim(), ln = dest.Line();
  int xlim = w - 1, xsrc = xlim - n, e = side & 0x03;
  const UC8 *s;
  UC8 *d;

  // left boundary
  if (e == 0)
  {
    s = dest.RoiSrc(0, 0);
    d = dest.RoiDest(0, 0);
    for (y = h - n; y > 0; y--, s += ln, d += ln)
      for (i = 0; i < n; i++)
        d[i] = s[n];
    return 1;
  }
  
  // top boundary
  if (e == 1)
  {
    s = dest.RoiSrc(0, h - n - 1);
    d = dest.RoiDest(0, h - n);
    for (i = n; i > 0; i--, d += ln)
      for (x = 0; x < w; x++)
        d[x] = s[x];
    return 1;
  }

  // right boundary
  if (e == 2)
  {
    s = dest.RoiSrc(0, 0);
    d = dest.RoiDest(0, 0);
    for (y = h - n; y > 0; y--, s += ln, d += ln)
      for (i = 0; i < n; i++)
        d[xlim - i] = s[xsrc];
    return 1;
  }

  // bottom boundary
  s = dest.RoiSrc(0, n);
  d = dest.RoiDest(0, 0);
  for (i = n; i > 0; i--, d += ln)
    for (x = 0; x < w; x++)
      d[x] = s[x];
  return 1;
}


//= Draw border t pixels in from the specified image boundary.
// side: 0 = left, 1 = top, 2 = right, 3 = bottom

int jhcDraw::DrawSide (jhcImg& dest, int side, int t, int r, int g, int b) const 
{
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::DrawSide");

  // local variables (ignores ROI)
  int i, j, e = side & 0x03, t3 = 3 * t, n = dest.Fields();
  int w = dest.XDim(), h = dest.YDim(), ln = dest.Line(), sk = dest.Skip();
  UC8 rv = BOUND(r), gv = BOUND(g), bv = BOUND(b);
  UC8 *d = dest.PxlDest();

  // left
  if (e == 0)
  {
    if (n == 1)
      for (i = h; i > 0; i--, d += ln)
        for (j = 0; j < t; j++)
          d[j] = rv;
    else
      for (i = h; i > 0; i--, d += ln)
        for (j = 0; j < t3; j += 3)
        {
          d[j]     = bv;
          d[j + 1] = gv;
          d[j + 2] = rv;
        }
    return 1;
  }

  // top
  if (e == 1)
  {
    d = dest.RoiDest(0, h - t);
    if (n == 1)
      for (j = t; j > 0; j--, d += sk)
        for (i = w; i > 0; i--, d++)
          *d = rv;
    else
      for (j = t; j > 0; j--, d += sk)
        for (i = w; i > 0; i--, d += 3)
        {
          d[0] = bv;
          d[1] = gv;
          d[2] = rv;
        }
    return 1;
  }

  // right
  if (e == 2)
  {
    d = dest.RoiDest(w - t, 0);
    if (n == 1)
      for (i = h; i > 0; i--, d += ln)
        for (j = 0; j < t; j++)
          d[j] = rv;
    else
      for (i = h; i > 0; i--, d += ln)
        for (j = 0; j < t3; j += 3)
        {
          d[j]     = bv;
          d[j + 1] = gv;
          d[j + 2] = rv;
        }
    return 1;
  }
    
  // bottom
  if (n == 1)
    for (j = t; j > 0; j--, d += sk)
      for (i = w; i > 0; i--, d++)
        *d = rv;
  else
    for (j = t; j > 0; j--, d += sk)
      for (i = w; i > 0; i--, d += 3)
      {
        d[0] = bv;
        d[1] = gv;
        d[2] = rv;
      }
  return 1;
}


//= Draw a rectangular outline centered on given coordinates.
// can optionally rotate whole shape by given number of degrees
// NOTE: "w" is size along "degs" (since w = x dimension when degs = 0)

int jhcDraw::RectCent (jhcImg& dest, double xc, double yc, double w, double h, 
                       double degs, int t, int r, int g, int b) const 
{
  double eqv = fabs(fmod(degs, 180.0)), tol = 0.02;    // 0.5 pel shift over 1400 pel side

  // check argument validity and dispatch for easiest cases
  if (!dest.Valid(1) && !dest.Valid(3))
    return Fatal("Bad image to jhcDraw::RectCent");
  if (t < 0)
    return 0;
  if (eqv < tol)         
    return RectEmpty(dest, ROUND(xc - 0.5 * w), ROUND(yc - 0.5 * h), 
                     ROUND(w), ROUND(h), t, r, g, b);
  if (fabs(eqv - 90.0) < tol)
    return RectEmpty(dest, ROUND(xc - 0.5 * h), ROUND(yc - 0.5 * w), 
                     ROUND(h), ROUND(w), t, r, g, b);

  // draw four lines for sides of rectangle
  double rads = D2R * degs, hsa = 0.5 * sin(rads), hca = 0.5 * cos(rads);
  double ws = w * hsa, wc = w * hca, hs = h * hsa, hc = h * hca;
  int nwx = ROUND(xc - wc + hs), nwy = ROUND(yc - hc - ws);
  int nex = ROUND(xc + wc + hs), ney = ROUND(yc - hc + ws);
  int sex = ROUND(xc + wc - hs), sey = ROUND(yc + hc + ws);
  int swx = ROUND(xc - wc - hs), swy = ROUND(yc + hc - ws);

  DrawLine(dest, nwx, nwy, nex, ney, t, r, g, b);
  DrawLine(dest, nex, ney, sex, sey, t, r, g, b);  
  DrawLine(dest, sex, sey, swx, swy, t, r, g, b);  
  DrawLine(dest, swx, swy, nwx, nwy, t, r, g, b);  
  return 1;
}


//= Draws a centered outline rectangle but coords are fractions of image size.
// NOTE: "w" is size along "degs" (since w = x dimension when degs = 0)

int jhcDraw::RectFrac (jhcImg& dest, 
                       double xcf, double ycf, double wf, double hf, 
                       double degs, int t, int r, int g, int b) const 
{
  int iw = dest.XDim(), ih = dest.YDim();

  return RectCent(dest, xcf * iw, ycf * ih, wf * iw, hf * ih, 
                  degs, t, r, g, b);
}


//= Draw an empty circle of a given size and thickness centered on a certain point.
// works with both RGB and monochrome images (g and b ignored)
// if r is negative, picks RGB color based on absolute value of r
// if partly outside image, will draw flat side at boundary if ej_clip > 0

int jhcDraw::CircleEmpty (jhcImg& dest, double xc, double yc, double radius, 
                          int t, int r, int g, int b) const 
{
  double dx, dy, chord, ichord, rad = radius + 0.5 * t, irad = radius - 0.5 * t;
  double dy2, rd = rad - 0.5, r2 = rad * rad, ir2 = irad * irad;
  int px, nx, py, ny;
  UC8 red = BOUND(r), grn = BOUND(g), blu = BOUND(b);

  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::CircleEmpty");
  if (rad <= 0.0)
    return 1;

  // automatically pick drawing color
  if (r < 0)
    Color8(&red, &grn, &blu, -r, dest.Fields());

  // monochrome case
  if (dest.Valid(1))
  {
    for (dy = 0.0; dy <= rd; dy += 1.0)
    {
      dy2 = dy * dy;
      chord = sqrt(r2 - dy2) - 0.5;
      if ((ir2 <= dy2) || ((dy + 1.0) > rd))
        ichord = 0.0;
      else 
        ichord = sqrt(ir2 - dy2) - 0.5;
      ny = ROUND(yc - dy);
      py = ROUND(yc + dy);
      for (dx = ichord; dx <= chord; dx += 1.0)
      {
        nx = ROUND(xc - dx);
        px = ROUND(xc + dx);
        dest.ASetClip(nx, ny, red, ej_clip);
        dest.ASetClip(px, ny, red, ej_clip);
        dest.ASetClip(nx, py, red, ej_clip);
        dest.ASetClip(px, py, red, ej_clip);
      }
    }
    return 1;
  }

  // color case
  for (dy = 0.0; dy <= rd; dy += 1.0)
  {
    dy2 = dy * dy;
    chord = sqrt(r2 - dy2) - 0.5;
    if ((ir2 <= dy2) || ((dy + 1.0) > rd))
      ichord = 0.0;
    else 
      ichord = sqrt(ir2 - dy2) - 0.5;
    ny = ROUND(yc - dy);
    py = ROUND(yc + dy);
    for (dx = ichord; dx <= chord; dx += 1.0)
    {
      nx = ROUND(xc - dx);
      px = ROUND(xc + dx);
      dest.ASetColClip(nx, ny, red, grn, blu, ej_clip);
      dest.ASetColClip(px, ny, red, grn, blu, ej_clip);
      dest.ASetColClip(nx, py, red, grn, blu, ej_clip);
      dest.ASetColClip(px, py, red, grn, blu, ej_clip);
    }
  }
  return 1;
}


//= Draw an line approximation to some ellipse.
// works with both RGB and monochrome images (g and b ignored)
// if r is negative, picks RGB color based on absolute value of r
// ang is the direction of the len dimension (unlike RectCent)

int jhcDraw::EllipseEmpty (jhcImg& dest, double xc, double yc, double len, double wid, double ang, 
                           int t, int r, int g, int b) const
{
  double rads = D2R * ang, c = cos(rads), s = sin(rads);
  double maj = 0.5 * len, acb = maj * c, asb = maj * s;
  double min = 0.5 * wid, bcb = min * c, bsb = min * s;
  double th, ca, sa, x, y, px, py, step = 10.0;

  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::EllipseEmpty");
  if (t <= 0)
    return 0;

  for (th = 0.0; th <= 360.0; th += step)
  {
    // find ellipse point for this central angle
    rads = D2R * th;
    ca = cos(rads); 
    sa = sin(rads);
    x = xc + acb * ca - bsb * sa;
    y = yc + asb * ca + bcb * sa;

    // draw line from last point 
    if (th != 0.0)
      DrawLine(dest, px, py, x, y, t, r, g, b);
    px = x;
    py = y;
  }
  return 1;
}


//= Draw an empty diamond shaped outline.

int jhcDraw::Diamond (jhcImg& dest, double xc, double yc, int w, int h, int t, int r, int g, int b) const
{
  double x0 = xc - 0.5 * w, x1 = xc + 0.5 * w, y0 = yc - 0.5 * h, y1 = yc + 0.5 * h;

  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::Diamond");
  if (t <= 0)
    return 0;

  DrawLine(dest, x0, yc, xc, y1, t, r, g, b);
  DrawLine(dest, xc, y1, x1, yc, t, r, g, b);
  DrawLine(dest, x1, yc, xc, y0, t, r, g, b);
  DrawLine(dest, xc, y0, x0, yc, t, r, g, b);
  return 1;
}
                      

///////////////////////////////////////////////////////////////////////////
//                           Lines and Curves                            //
///////////////////////////////////////////////////////////////////////////

//= Draw a colored line on image, clips as needed (ignores ROI).
// works with both RGB and monochrome images (g and b ignored)
// if r is negative, picks RGB color based on absolute value of r
// draws circles at ends then cross-steps by half pixels at each location
// if outside image, will draw flat side at boundary if ej_clip > 0

int jhcDraw::DrawLine (jhcImg& dest, double x0, double y0, double x1, double y1,
                       int t, int r, int g, int b) const 
{
  double x, y, dx, dy, norm, xstep, ystep, slope;
  double x0t = x0, y0t = y0, x1t = x1, y1t = y1, t2 = 0.5 * t;
  UC8 red = BOUND(r), grn = BOUND(g), blu = BOUND(b);
  int i, w = dest.XDim(), h = dest.YDim();

  // check for good inputs
  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::DrawLine");

  // check for null cases (e.g. completely offscreen)
  if (t <= 0)
    return 0;
  if (ej_clip <= 0)
    if (((ROUND(x0 + t2) < 0)  && (ROUND(x1 + t2) < 0))  || 
        ((ROUND(y0 + t2) < 0)  && (ROUND(y1 + t2) < 0))  || 
        ((ROUND(x0 - t2) >= w) && (ROUND(x1 - t2) >= w)) ||
        ((ROUND(y0 - t2) >= h) && (ROUND(y1 - t2) >= h)))
      return 0;

  // automatically pick drawing color then draw end points
  if (r < 0)
    Color8(&red, &grn, &blu, -r, dest.Fields());
  CircleFill(dest, x0, y0, 0.5 * t, r, g, b); 
  CircleFill(dest, x1, y1, 0.5 * t, r, g, b); 

  // swap points to get y ascending
  if (y1 < y0)
  {
    x0t = x1;
    y0t = y1;
    x1t = x0;
    y1t = y0;
  }

  // figure out basic cross stepping parameters 
  dx = x1t - x0t;
  dy = y1t - y0t;
  norm = 0.5 / sqrt(dx * dx + dy * dy);
  xstep = -dy * norm;
  ystep = dx * norm; 

  // monochrome case
  if (dest.Valid(1))
  {
    // possibly step along the y axis
    if (dy > fabs(dx))      
    { 
      slope = 0.5 * dx / dy;
      for (x = x0t, y = y0t; y <= y1t; x += slope, y += 0.5)
        for (dx = 0.0, dy = 0.0, i = t; i > 0; i--, dx += xstep, dy += ystep)
        {
          dest.ASetClip(ROUND(x - dx), ROUND(y - dy), red, ej_clip);
          dest.ASetClip(ROUND(x + dx), ROUND(y + dy), red, ej_clip);
        }
      return 1;
    }

    // step along the x axis instead
    slope = 0.5 * dy / fabs(dx); 
    if (dx > 0.0)      
      for (x = x0t, y = y0t; x <= x1t; x += 0.5, y += slope)
        for (dx = 0.0, dy = 0.0, i = t; i > 0; i--, dx += xstep, dy += ystep)
        {
          dest.ASetClip(ROUND(x - dx), ROUND(y - dy), red, ej_clip);
          dest.ASetClip(ROUND(x + dx), ROUND(y + dy), red, ej_clip);
        }
    else           
      for (x = x0t, y = y0t; x >= x1t; x -= 0.5, y += slope)
        for (dx = 0.0, dy = 0.0, i = t; i > 0; i--, dx += xstep, dy += ystep)
        {
          dest.ASetClip(ROUND(x - dx), ROUND(y - dy), red, ej_clip); 
          dest.ASetClip(ROUND(x + dx), ROUND(y + dy), red, ej_clip); 
        }
    return 1;
  }

  // color case - possibly step along the y axis
  if (dy > fabs(dx))      
  { 
    slope = 0.5 * dx / dy;
    for (x = x0t, y = y0t; y <= y1t; x += slope, y += 0.5)
      for (dx = 0.0, dy = 0.0, i = t; i > 0; i--, dx += xstep, dy += ystep)
      {
        dest.ASetColClip(ROUND(x - dx), ROUND(y - dy), red, grn, blu, ej_clip);
        dest.ASetColClip(ROUND(x + dx), ROUND(y + dy), red, grn, blu, ej_clip);
      }
    return 1;
  }

  // color case - step along the x axis instead
  slope = 0.5 * dy / fabs(dx); 
  if (dx > 0.0)      
    for (x = x0t, y = y0t; x <= x1t; x += 0.5, y += slope)
      for (dx = 0.0, dy = 0.0, i = t; i > 0; i--, dx += xstep, dy += ystep)
      {
        dest.ASetColClip(ROUND(x - dx), ROUND(y - dy), red, grn, blu, ej_clip);
        dest.ASetColClip(ROUND(x + dx), ROUND(y + dy), red, grn, blu, ej_clip);
      }
  else           
    for (x = x0t, y = y0t; x >= x1t; x -= 0.5, y += slope)
      for (dx = 0.0, dy = 0.0, i = t; i > 0; i--, dx += xstep, dy += ystep)
      {
        dest.ASetColClip(ROUND(x - dx), ROUND(y - dy), red, grn, blu, ej_clip);
        dest.ASetColClip(ROUND(x + dx), ROUND(y + dy), red, grn, blu, ej_clip);
      }
  return 1;
}


//= Draw a ray from point (x0 y0) at some angle and extending some length.
// if len <= 0 then draws to edge of image

int jhcDraw::Ray (jhcImg& dest, double x0, double y0, double ang, double len, int t, int r, int g, int b) const
{
  double rads = D2R * ang, d = ((len > 0.0) ? len : dest.XDim() + dest.YDim());

  return DrawLine(dest, x0, y0, x0 + d * cos(rads), y0 + d * sin(rads), t, r, g, b);
}
                  

//= Connect a series of corner points with lines (n negative for open curve).
// takes number of points, line thickness, and line color

int jhcDraw::DrawCorners (jhcImg& dest, const double x[], const double y[], int pts, 
                          int t, int r, int g, int b) const 
{
  int i, n = abs(pts);

  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::DrawCorners");
  if ((pts > -2) && (pts < 3))
    return 0;

  for (i = 1; i < n; i++)
    DrawLine(dest, x[i - 1], y[i - 1], x[i], y[i], t, r, g, b); 
  if (pts > 0) 
    DrawLine(dest, x[n - 1], y[n - 1], x[0], y[0], t, r, g, b);
  return 1;
}


//= Interpret a series of image points as a closed polygon.
// takes number of points, line thickness, and line color

int jhcDraw::DrawPoly (jhcImg& dest, const int x[], const int y[], int pts, 
                          int t, int r, int g, int b) const 
{
  int i;

  if (!dest.Valid(1, 3))
    return Fatal("Bad image to jhcDraw::DrawPoly");
  if (pts < 3)
    return 0;

  for (i = 1; i < pts; i++)
    DrawLine(dest, x[i - 1], y[i - 1], x[i], y[i], t, r, g, b); 
  DrawLine(dest, x[pts - 1], y[pts - 1], x[0], y[0], t, r, g, b);
  return 1;
}


//= Draw an axis parallel cross centered on given location.

int jhcDraw::Cross (jhcImg& dest, double xc, double yc, int w, int h,
                    int t, int r, int g, int b) const 
{
  int x = ROUND(xc), y = ROUND(yc), hw = w / 2, hh = h / 2;

  if (!dest.Valid(1) && !dest.Valid(3))
    return Fatal("Bad image to jhcDraw::Cross");
  if (t < 0)
    return 0;

  DrawLine(dest, x - hw, y,      x + hw, y,      t, r, g, b);
  DrawLine(dest, x,      y - hh, x,      y + hh, t, r, g, b);
  return 1;
}


//= Draw a diagonal armed cross centered on given location.

int jhcDraw::XMark (jhcImg& dest, double xc, double yc, int sz,
                    int t, int r, int g, int b) const 
{
  int x = ROUND(xc), y = ROUND(yc), hsz = sz / 2;

  if (!dest.Valid(1) && !dest.Valid(3))
    return Fatal("Bad image to jhcDraw::XMark");
  if (t < 0)
    return 0;

  DrawLine(dest, x - hsz, y - hsz, x + hsz, y + hsz, t, r, g, b);
  DrawLine(dest, x - hsz, y + hsz, x + hsz, y - hsz, t, r, g, b);
  return 1;
}


//= Draw a cubic spline between curve points (x1 y1) and (x2 y2).
// preceding and following points used to determine slopes
// implements centripetal form of Catmull-Rom splines (no cusps)
// if r is negative, picks RGB color based on absolute value of r

int jhcDraw::Spline (jhcImg& dest, double x0, double y0, double x1, double y1, 
                     double x2, double y2, double x3, double y3,
                     int w, int r, int g, int b) const
{
  double d2, tsum, d12, t1, t2, tstep, t, x, y, px = x1, py = y1;
  int i, n;

  // divvy up "time" between points based on sqrt of distance (t3 = 1)
  d2 = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
  tsum = pow((double) d2, 0.25);
  t1 = tsum;
  d2 = (x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1);
  d12 = sqrt((double) d2);
  tsum += pow((double) d2, 0.25);
  t2 = tsum;
  d2 = (x3 - x2) * (x3 - x2) + (y3 - y2) * (y3 - y2);
  tsum += pow((double)d2, 0.25);
  t1 /= tsum;
  t2 /= tsum;

  // figure out how many line segments to use
  n = ROUND(d12 / 10.0);
  n = __max(3, n);
  tstep = (t2 - t1) / n;
  t = t1;

  // draw segmented approximation
  for (i = 0; i <= n; i++, t += tstep)
  {
    x = spline_mix(x0, x1, x2, x3, t1, t2, t);
    y = spline_mix(y0, y1, y2, y3, t1, t2, t);
    DrawLine(dest, px, py, x, y, w, r, g, b);
    px = x;
    py = y;
  }
  return 1;
}


//= Get interpolated value at "time" t between points v1 and v2.
// t1 and t2 are "times" at v1 and v2, while assuming t0 = 0 and t3 = 1
// uses Barry and Goldman's pyramid to compute mixed value
// <pre>
//
//   L01  =   v0 * (t1 - t) / (t1 - t0) +   v1 * (t - t0) / (t1 - t0)
//   L12  =   v1 * (t2 - t) / (t2 - t1) +   v2 * (t - t1) / (t2 - t1)
//   L23  =   v2 * (t3 - t) / (t3 - t2) +   v3 * (t - t2) / (t3 - t2)
//
//   L012 =  L01 * (t2 - t) / (t2 - t0) +  L12 * (t - t0) / (t2 - t0)
//   L123 =  L12 * (t3 - t) / (t3 - t1) +  L23 * (t - t1) / (t3 - t1)
//
//   C12  = L012 * (t2 - t) / (t2 - t1) + L123 * (t - t1) / (t2 - t1)
//
// which can be further simplified if t0 = 0 and t3 = 1
// </pre>

double jhcDraw::spline_mix (double v0, double v1, double v2, double v3, double t1, double t2, double t) const
{
  double L01 = (v0 * (t1 - t) + v1 * t) / t1;
  double L12 = (v1 * (t2 - t) + v2 * (t - t1)) / (t2 - t1);      
  double L23 = (v2 * (1.0 - t) + v3 * (t - t2)) / (1.0 - t2);
  double L012 = (L01 * (t2 - t) + L12 * t) / t2;
  double L123 = (L12 * (1.0 - t) + L23 * (t - t1)) / (1.0 - t1);

  return((L012 * (t2 - t) + L123 * (t - t1)) / (t2 - t1));
}


//= Draw a cubic spline between end point (x1 y1) and curve point (x2 y2).
// preceding and following points used to determine slopes
// uses square brush, always draws first point but skips last one

int jhcDraw::SplineStart (jhcImg& dest, double x1, double y1, double x2, double y2, double x3, double y3,
                          int w, int r, int g, int b) const
{
  return Spline(dest, 2.0 * x1 - x2, 2.0 * y1 - y2, x1, y1, x2, y2, x3, y3, w, r, g, b);
}


//= Draw a cubic spline between curve point (x1 y1) and end point (x2 y2).
// preceding and following points used to determine slopes
// uses square brush, always draws first and last point

int jhcDraw::SplineEnd (jhcImg& dest, double x0, double y0, double x1, double y1, double x2, double y2,
                        int w, int r, int g, int b) const
{
  return Spline(dest, x0, y0, x1, y1, x2, y2, 2.0 * x2 - x1, 2.0 * y2 - y1, w, r, g, b);
}


//= Draw a smooth contour with given series of spline control points.
// if n > 0 then spline is closed, n < 0 means leave it open

int jhcDraw::MultiSpline (jhcImg& dest, const int cx[], const int cy[], int n, 
                          int w, int r, int g, int b) const
{
  double x[4], y[4];
  int i, num = abs(n);

  // check arguments
  if (!dest.Valid(1) && !dest.Valid(3))
    return Fatal("Bad image to jhcDraw::MultiSpline");
  if (num < 2)
    return 0;

  // degenerate case of 2 points
  if (num == 2)
    return DrawLine(dest, cx[0], cy[0], cx[1], cy[1], w, r, g, b);

  // load up first three points into circular buffer
  for (i = 0; i < 3; i++)
  {
    x[i] = cx[i];
    y[i] = cy[i];
  }

  // draw first segment (pt 0-1) of possibly looped contour
  if (n <= 0)
    SplineStart(dest, x[0], y[0], x[1], y[1], x[2], y[2], w, r, g, b);
  else  
  {
    // use endpoint to constrain curve at start
    x[3] = cx[num - 1];
    y[3] = cy[num - 1];
    Spline(dest, x[3], y[3], x[0], y[0], x[1], y[1], x[2], y[2], w, r, g, b);
  }

  // draw bulk of segments in contour
  for (i = 3; i < num; i++)
  {
    // get new point to constrain far end of this segment (pt 1-2)
    x[3] = cx[i];
    y[3] = cy[i];
    Spline(dest, x[0], y[0], x[1], y[1], x[2], y[2], x[3], y[3], w, r, g, b);

    // shuffle old points down and advance to next segment
    x[0] = x[1];
    y[0] = y[1];
    x[1] = x[2];
    y[1] = y[2];
    x[2] = x[3];
    y[2] = y[3];
  }

  // draw segment ending in last point
  if (n <= 0)
    SplineEnd(dest, x[0], y[0], x[1], y[1], x[2], y[2], w, r, g, b);
  else
  {
    // close contour at end by adding one more segment
    Spline(dest, x[0], y[0], x[1], y[1], x[2], y[2], cx[0], cy[0], w, r, g, b);
    Spline(dest, x[1], y[1], x[2], y[2], cx[0], cy[0], cx[1], cy[1], w, r, g, b);
  }
  return 1;
}


//= Draw closed contour bounded by two open spline curves.

int jhcDraw::Ribbon (jhcImg& dest, const int cx[], const int cy[], int n, 
                     const int cx2[], const int cy2[], int n2, 
                     int w, int r, int g, int b) const
{
  int sx = cx[0], ex = cx[n - 1], sx2 = cx2[0], ex2 = cx2[n2 - 1];
  int sy = cy[0], ey = cy[n - 1], sy2 = cy2[0], ey2 = cy2[n2 - 1];
  int s_s2, s_e2, e_s2, e_e2;

  if (!dest.Valid(1) && !dest.Valid(3))
    return Fatal("Bad image to jhcDraw::Ribbon");

  // draw both curvy sides as open splines
  MultiSpline(dest, cx,  cy,  -n,  w, r, g, b);
  MultiSpline(dest, cx2, cy2, -n2, w, r, g, b);

  // figure out distances between pairs of endpoints
  s_s2 = (sx - sx2) * (sx - sx2) + (sy - sy2) * (sy - sy2);
  s_e2 = (sx - ex2) * (sx - ex2) + (sy - ey2) * (sy - ey2);
  e_s2 = (ex - sx2) * (ex - sx2) + (ey - sy2) * (ey - sy2);
  e_e2 = (ex - ex2) * (ex - ex2) + (ey - ey2) * (ey - ey2);

  // get reasonable connection order with straight lines
  if ((s_s2 + e_e2) <= (s_e2 + e_s2))
  {
    DrawLine(dest, sx, sy, sx2, sy2, w, r, g, b);
    DrawLine(dest, ex, ey, ex2, ey2, w, r, g, b);
  }
  else
  {
    DrawLine(dest, sx, sy, ex2, ey2, w, r, g, b);
    DrawLine(dest, ex, ey, sx2, sy2, w, r, g, b);
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                                 Masks                                 //
///////////////////////////////////////////////////////////////////////////

//= Draw a border in destination image based on edges of region above threshold in source.
// returns number of pixels drawn (a rough estimate of perimeter)

int jhcDraw::Outline (jhcImg& dest, const jhcImg& src, int th, int r, int g, int b) const
{
  if (!dest.Valid(1, 3) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcDraw::Outline");
  dest.MergeRoi(src);

  // general ROI case
  int sln = src.Line(), ssk = src.RoiSkip(dest);
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), cnt = 0;
  const UC8 *s = src.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // monochrome image marking
  if (dest.Valid(1) > 0)
  {
    for (y = rh; y > 0; y--, d += dsk, s += ssk)
      for (x = rw; x > 0; x--, d++, s++)
        if (*s > th)
          if ((y == rh) || (y == 1) || (x == rw) || (x == 1) || chk_hood(s, sln, th))
          {
            *d = (UC8) r;
            cnt++;
          }
    return cnt;
  }

  // color image marking
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d += 3, s++)
      if (*s > th)
        if ((y == rh) || (y == 1) || (x == rw) || (x == 1) || chk_hood(s, sln, th))
        {
          d[0] = (UC8) b;
          d[1] = (UC8) g;
          d[2] = (UC8) r;
          cnt++;
        }
  return cnt;
}


//= See if any 8 connected neighbors of pixel are at or below threshold.
// assumes image bounds checking already done so safe to retrieve pixels in all directions

bool jhcDraw::chk_hood (const UC8 *s, int sln, int th) const
{
  int dx, dy;

  for (dy = -sln; dy <= sln; dy += sln)
    for (dx = -1; dx <= 1; dx++)
      if (s[dy + dx] <= th)
        return true;
  return false;
}


//= Show parts of data over threshold as some color shading over monochrome image.
// typically used to show groundplane in green over top of monochrome input image
// ignores ROIs, always does full image

int jhcDraw::Emphasize (jhcImg& dest, const jhcImg& src, const jhcImg& data, int th, int dr, int dg, int db) const
{
  if (!dest.Valid(3) || !dest.SameSize(src, 1) || !dest.SameSize(data, 1))
    return Fatal("Bad images to jhcDraw::Emphasize");

  // full image case
  int amt = __max(dr, __max(dg, db)), vr2 = dr << 1, vg2 = dg << 1, vb2 = db << 1;
  int x, y, adj, w = dest.XDim(), h = dest.YDim(), dsk = dest.Skip(), ssk = src.Skip();
  const UC8 *s = src.PxlSrc(), *v = data.PxlSrc();
  UC8 *d = dest.PxlDest();

  for (y = h; y > 0; y--, d += dsk, s += ssk, v += ssk)
    for (x = w; x > 0; x--, d += 3, s++, v++)
      if (*v < th)
      {
        // monochrome copy
        d[0] = *s;
        d[1] = *s;
        d[2] = *s;
      }
      else
      {
        // dimmed plus emphasis amount
        adj = *s - amt;
        d[0] = BOUND(adj + vb2);
        d[1] = BOUND(adj + vg2);
        d[2] = BOUND(adj + vr2);
      }
  return 1;
}



