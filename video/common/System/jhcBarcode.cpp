// jhcBarcode.cpp : reads barcodes centered in image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2007-2016 IBM Corporation
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

#include <stdlib.h>                  // for NULL

#include "jhcBarcode.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcBarcode::jhcBarcode ()
{
  // no size set yet
  w = 0;
  h = 0;
  f = 0;
  ln = 0;

  // no local arrays allocated yet
  proj = NULL;

  // set up processing parameters
  build_tables();
  FancyVersion();
}


//= Default values for processing parameters for still images.

void jhcBarcode::FancyVersion ()
{
  // stripe generation & result type
  steps = 9;    // number of offset slices
  off   = 40;   // spacing of slices (pels)
  cols  = 3;    // colors to try (G, R, B)
  dirs  = 4;    // directions to try (H, V, D1, D2)
  mode  = 3;    // do full pattern

  // code localization & bar finding
  sm    = 1;    // amount to smooth scan 
  dmax  = 20;   // barcode border uniformity
  wmin  = 15;   // barcode border minimum width
  bmin  = 200;  // overall barcode minimum width
  dbar  = 5;    // minimum slope for edge
  bdiff = 30;   // minimum change for edge 

  // bar pattern interpretation
  badj  = 1;    // automatic border adjustment
  eadj  = 1;    // automatically adjust edges
  est   = 2;    // method for estimating bit width
  pod   = 2;    // decoding mode for digits
  cvt   = 0;    // convert 6 digit to 10
}


//= Default values for processing parameters for cellphone video.
// these make the system operate like the original version of code

void jhcBarcode::VideoVersion ()
{
  // stripe generation & result type
  steps = 9;    // number of offset slices
  off   = 50;   // spacing of slices (pels)
  cols  = 3;    // colors to try (G, R, B)
  dirs  = 4;    // directions to try (H, V, D1, D2)
  mode  = 2;    // do partial pattern

  // code localization & bar finding
  sm    = 2;    // amount to smooth scan 
  dmax  = 20;   // barcode border uniformity
  wmin  = 15;   // barcode border minimum width
  bmin  = 200;  // overall barcode minimum width
  dbar  = 50;   // minimum slope for edge
  bdiff = 100;  // minimum change for edge 

  // bar pattern interpretation
  badj  = 0;    // automatic border adjustment
  eadj  = 0;    // automatically adjust edges
  est   = 1;    // method for estimating bit width
  pod   = 0;    // decoding mode for digits
  cvt   = 0;    // convert 6 digit to 10
}


//= Build lookup tables for convert YUV to RGB.
//
//   r = (c298           + 409 * e + 128) >> 8;
//   g = (c298 - 100 * d - 208 * e + 128) >> 8;
//   b = (c298 + 516 * d           + 128) >> 8;
//
// where:
//
//   c298 = 298 * (y - 16), 
//   d = u - 128 
//   e = v - 128

void jhcBarcode::build_tables ()
{
  int i, sum, sum2;

  // build Y contributions (same for all colors) with rounding
  for (i = 0, sum = -4640; i < 256; i++, sum += 298)
    ymult[i] = sum;   // 298 * (i - 16) + 128
  
  // build U and V tables for red (starts at -128)
  for (i = 0, sum = -52352; i < 256; i++, sum += 409)
  {
    umult[2][i] = 0;
    vmult[2][i] = sum;   // 409 * (i - 128)
  } 

  // build U and V tables for green (starts at -128)
  for (i = 0, sum = 12800, sum2 = 26624; i < 256; i++, sum -= 100, sum2 -= 208)
  {
    umult[1][i] = sum;   // -100 * (i - 128)
    vmult[1][i] = sum2;  // -208 * (i - 128)
  } 

  // build U and V tables for blue (starts at -128)
  for (i = 0, sum = -66048; i < 256; i++, sum += 516)
  {
    umult[0][i] = sum;   // 516 * (i - 128)
    vmult[0][i] = 0;
  } 
}


//= Default destructor does necessary cleanup.

jhcBarcode::~jhcBarcode ()
{
  dealloc();
}


//= Deallocate local arrays.

void jhcBarcode::dealloc ()
{
  delete [] proj;
  proj = NULL;
}


//= Set sizes of internal images directly.

void jhcBarcode::SetSize (int x, int y, int z)
{
  if ((x == w) && (y == h) && (z == f))
    return;

  // remember sizes and compute line length
  w = x; 
  h = y;
  f = z;
  ln = ((w * f) + 3) & 0xFFFC;

  // set projection slice sizes
  dealloc();
  proj = new int[__max(w, h)];
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Take best guess at barcode using scan pattern selected by mode variable.
// pat 1 is fast, 2 medium, 3 slow, others are default (e.g. 0, -4)
// a negative pat value specifies that the input is YV12 not RGB format

int jhcBarcode::GetCode (char *code, const unsigned char *src, int pat)
{
  int yv12 = ((pat < 0) ? 1 : 0);

  // specific pattern selected for RGB bitmap 
  if (pat == 1)
    return FastCode(code, src, 0);
  if (pat == 2)
    return FastCode(code, src, 0);
  if (pat == 3)
    return FastCode(code, src, 0);

  // specific pattern selected for YV12 bitmap
  if (pat == -1)
    return FastCode(code, src, 1);
  if (pat == -2)
    return FastCode(code, src, 1);
  if (pat == -3)
    return FastCode(code, src, 1);

  // otherwise use default pattern with RGB or YV12
  if (mode == 1)
    return FastCode(code, src, yv12);
  if (mode == 2)
    return HorizCode(code, src, yv12);
  return SlowCode(code, src, yv12);
}


//= Take best guess at barcode using multiple horizontal stripes through area.
// normally Microsoft DIB/BMP buffer in BGR order with line padded to 4 bytes
// alternate yv12 format is full-sized Y followed by half-sized U and half-size V
// always does 3 horizontal lines using green channel
// fills character string with full code when successful
// returns 1 if found and decoded, 0 if nothing seen anywhere or unreadable
// takes about 42us for 640x480 on a 1.7GHz PentiumM (4x faster than HorizCode)

int jhcBarcode::FastCode (char *code, const unsigned char *src, int yv12)
{
  int i, dy, mid = h / 2, lines = 3, move = 100;

  scans = 0;
  for (i = 0; i < lines; i++)
  {
    // search from the center then up and down by constant amounts
    dy = ((i + 1) >> 1) * move;
    if ((i & 0x01) != 0)
      dy = -dy;
    scans++;

    // as soon as one valid read happens stop searching
    if (SliceCode(code, src, 0, dy, 1, yv12) > 0)
      return 1;
  }
  return 0;
}


//= Take best guess at barcode using horizontal stripes and all colors.
// works adequately for 320x240 images from cell phone video
// normally Microsoft DIB/BMP buffer in BGR order with line padded to 4 bytes
// alternate yv12 format is full-sized Y followed by half-sized U and half-size V
// fills character string with full code when successful
// returns 1 if found and decoded, 0 if nothing seen anywhere or unreadable
// takes about 117us for 320x240 on a 1.7GHz PentiumM (-> 0.5ms @ 400MHz)

int jhcBarcode::HorizCode (char *code, const unsigned char *src, int yv12)
{
  int c, i, fld, dist;
  
  scans = 0;
  for (c = 0; c < 3; c++)                // multiple color fields
  {
    // try green then red then blue
    fld = ((c == 0) ? 1 : ((c == 1) ? 2 : 0));
      for (i = 0; i < 5; i++)            // multiple horizontal slices 
      {
        // search from the center then up and down by constant amounts
        dist = ((i + 1) >> 1) * off;
        if ((i & 0x01) != 0)
          dist = -dist;
        scans++;

        // as soon as one valid read happens stop searching
        if (SliceCode(code, src, 0, dist, fld, yv12) > 0)
          return 1;
      }
  }
  return 0;
}


//= Take best guess at barcode using multiple stripes and colors.
// normally Microsoft DIB/BMP buffer in BGR order with line padded to 4 bytes
// alternate yv12 format is full-sized Y followed by half-sized U and half-size V
// fills character string with full code when successful
// returns 1 if found and decoded, 0 if nothing seen anywhere or unreadable
// takes about 2.2ms for 640x480 on a 1.7GHz PentiumM (8x slower than HorizCode)

int jhcBarcode::SlowCode (char *code, const unsigned char *src, int yv12)
{
  int c, d, i, fld, dist;
  
  scans = 0;
  for (d = 0; d < dirs; d++)              // multiple slice orientations
    for (c = 0; c < __min(cols, f); c++)  // multiple color fields
    {
      // try green then red then blue
      fld = ((c == 0) ? 1 : ((c == 1) ? 2 : 0));
      for (i = 0; i < steps; i++)         // multiple slices in current direction
      {
        // search from the center then up and down by constant amounts
        dist = ((i + 1) >> 1) * off;
        if ((i & 0x01) != 0)
          dist = -dist;
        scans++;

        // as soon as one valid read happens stop searching
        if (SliceCode(code, src, d, dist, fld, yv12) > 0)
          return 1;
      }
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Image Analysis                           //
///////////////////////////////////////////////////////////////////////////

//= Interpret intensity pattern on a given stripe as a barcode.
// normally Microsoft DIB/BMP buffer in BGR order with line padded to 4 bytes
// alternate yv12 format is full-sized Y followed by half-sized U and half-size V
// can select color channel, offset of slice from center, and direction:
//   0 = horizontal, 1 = vertical, 2 = major diagonal, 3 = minor diagonal
// swaps horizontal and vertical if in yv12 mode (for cell phone)
// returns 1 if okay, 0 if confused, fills character string with full code

int jhcBarcode::SliceCode (char *code, const unsigned char *src, 
                           int dir, int cdist, int fld, int yv12)
{
  int i, sf = ((f == 1) ? 0 : fld);

  // remember calling parameters
  sdir = dir;
  soff = cdist;
  sfld = sf;

  // get a nice intensity profile
  if (yv12 <= 0)
  {
    // normal BGR bitmap image
    if (dir <= 0)
      slen = slice_h(proj, src, cdist, sf);
    else if (dir == 1)
      slen = slice_v(proj, src, cdist, sf);
    else if (dir == 2)
      slen = slice_d1(proj, src, cdist, sf);
    else 
      slen = slice_d2(proj, src, cdist, sf);
  }
  else
  {
    // special YUV encoded image
    if (dir <= 0)
      slen = slice_v_yuv(proj, src, cdist, sf);
    else if (dir == 1)
      slen = slice_h_yuv(proj, src, cdist, sf);
    else if (dir == 2)
      slen = slice_d1_yuv(proj, src, cdist, sf);
    else 
      slen = slice_d2_yuv(proj, src, cdist, sf);
  }

  // break intensity slice into bar edges
  ecnt = get_edges(ejs, i0, i1, proj, slen);

  // possibly fill in complete bit array at once then interpret  
  if (pod <= 0)
  {
    bcnt = bit_vect(bits, ejs, ecnt, bw16);
    return get_code(code, bits, bcnt);
  }

  // interpret each pair of bars separately
  bcnt = 0;
  for (i = 0; i < 200; i++)
    bits[i] = 0;
  return get_code(code, ejs, ecnt);
}


//= Returns line coordinates for active part of last slice analyzed.

void jhcBarcode::LastSlice (int& x0, int& y0, int& x1, int& y1)
{
  x0 = sx0 + i0 * sdx;
  y0 = sy0 + i0 * sdy;
  x1 = sx0 + i1 * sdx;
  y1 = sy0 + i1 * sdy;
}


///////////////////////////////////////////////////////////////////////////
//                            Stripe Extraction                          //
///////////////////////////////////////////////////////////////////////////

//= Generate a horizontal intensity slice of an image.
// cdist is distance of slice above (+) or below (-) image center
// can specify which color channel to use, assumes slice array is big enough

int jhcBarcode::slice_h (int *slice, const unsigned char *src, int cdist, int fld)
{
  int dx, dy = (h >> 1) + cdist;
  const unsigned char *s = src + dy * ln + fld;
  int *p = slice;

  // record slice geometry
  sx0 = 0;
  sy0 = dy;
  sdx = 1;
  sdy = 0;

  // collect pixels
  for (dx = 0; dx < w; dx++, p++, s += f)
    *p = *s;

  // clean up and return length
  smooth(slice, w, sm);
  return w;
}


//= Generate a vertical intensity slice of an image.
// cdist is distance of slice right (+) or left (-) of image center
// can specify which color channel to use, assumes slice array is big enough

int jhcBarcode::slice_v (int *slice, const unsigned char *src, int cdist, int fld)
{
  int dy, dx = (w >> 1) + cdist;
  const unsigned char *s = src + dx * f + fld;
  int *p = slice;  

  // record slice geometry
  sx0 = dx;
  sy0 = 0;
  sdx = 0;
  sdy = 1;

  // collect pixels
  for (dy = 0; dy < h; dy++, p++, s += ln)
    *p = *s;

  // clean up and return length
  smooth(slice, h, sm);
  return h;
}


//= Generate a major diagonal intensity slice of an image.
// cdist is distance of slice NE (+) or SW (-) of image center
// can specify which color channel to use, assumes slice array is big enough

int jhcBarcode::slice_d1 (int *slice, const unsigned char *src, int cdist, int fld)
{
  int i, skip = ln - f, coff = (cdist * 181) >> 8;      // adjust by 0.7071
  int xc = (w >> 1) + coff, yc = (h >> 1) + coff;
  int bot = __min(w - 1 - xc, yc), top = __min(xc, h - 1 - yc);
  int x0 = xc + bot, y0 = yc - bot, len = bot + top + 1;
  const unsigned char *s = src + y0 * ln + x0 * f + fld;
  int *p = slice;
  
  // record slice geometry
  sx0 = x0;
  sy0 = y0;
  sdx = -1;
  sdy = 1;

  // collect pixels
  for (i = 0; i < len; i++, p++, s += skip)
    *p = *s;

  // clean up and return length
  smooth(slice, len, sm);
  return len;
}


//= Generate a minor diagonal intensity slice of an image.
// cdist is distance of slice NW (+) or SE (-) of image center
// can specify which color channel to use, assumes slice array is big enough

int jhcBarcode::slice_d2 (int *slice, const unsigned char *src, int cdist, int fld)
{
  int i, skip = ln + f, coff = (cdist * 181) >> 8;      // adjust by 0.7071
  int xc = (w >> 1) - coff, yc = (h >> 1) + coff;
  int bot = __min(xc, yc), top = __min(w - 1 - xc, h - 1 - yc);
  int x0 = xc - bot, y0 = yc - bot, len = bot + top + 1;
  const unsigned char *s = src + y0 * ln + x0 * f + fld;
  int *p = slice;

  // record slice geometry
  sx0 = x0;
  sy0 = y0;
  sdx = 1;
  sdy = 1;

  // collect pixels
  for (i = 0; i < len; i++, p++, s += skip)
    *p = *s;

  // clean up and return length
  smooth(slice, h, sm);
  return h;
}


//= Normalize contrast and smooth intensity profile.

void jhcBarcode::smooth (int *slice, int len, int n)
{
  int xlim = len - 1, w25 = len / 4, w75 = 3 * w25;
  int i, x, lo, hi, sc, v, left, mid; 

  // find min and max for central portion
  lo = slice[w25];
  hi = slice[w25];
  for (x = w25 + 1; x <= w75; x++)
  {
    lo = __min(lo, slice[x]);
    hi = __max(hi, slice[x]);
  }

  // figure scaling to make min and max be 0 and 1000
  if (hi > lo)
  {
    sc = 256000 / (hi - lo);
    for (x = 0; x < len; x++)
    {
      v = (sc * (slice[x] - lo)) >> 8;
      slice[x] = __max(0, __min(v, 1000));
    }
  }

  // smooth array using [0.25 0.5 0.25] mask several times.
  // values in first and last bins never changed
  for (i = 0; i < n; i++)
  {
    left = slice[0];
    for (x = 1; x < xlim; x++)
    {
      mid = slice[x];
      slice[x] = (left + (mid << 1) + slice[x + 1] + 2) >> 2;
      left = mid;
    }
  }
}


///////////////////////////////////////////////////////////////////////////
//                               YUV Stripes                             //
///////////////////////////////////////////////////////////////////////////

//= Generate a horizontal intensity slice of a YV12 image.
// cdist is distance of slice above (+) or below (-) image center
// can specify which color channel to use, assumes slice array is big enough
// yv12 format is full-sized Y followed by half-sized U and half-size V
// forces dy to be even so aligned with U and V

int jhcBarcode::slice_h_yuv (int *slice, const unsigned char *src, int cdist, int fld)
{
  int val, base, w2 = w >> 1, h2 = h >> 1, sz = w * h, sz2 = w2 * h2;
  int dx, dy = (h2 + cdist) & 0xFFFE, off = dy * w, off2 = (dy >> 1) * w2;
  const unsigned char *y = src + off, *u = src + sz + off2, *v = u + sz2;
  int *p = slice;

  // record slice geometry
  sx0 = 0;
  sy0 = dy;
  sdx = 1;
  sdy = 0;

  // collect pixels using U and V values for two successive Y values
  // Y moves by 2 pixels, U and V move by one pixel (half-sized)
  for (dx = 0; dx < w; dx += 2, p += 2, y += 2, u += 1, v += 1)
  {
    base = umult[fld][*u] + vmult[fld][*v];
    val = (base + ymult[y[0]]) >> 8;
    p[0] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);
    val = (base + ymult[y[1]]) >> 8;
    p[1] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);
  }

  // clean up and return length
  smooth(slice, w, sm);
  return w;
}


//= Generate a vertical intensity slice of a YV12 image.
// cdist is distance of slice right (+) or left (-) of image center
// can specify which color channel to use, assumes slice array is big enough
// yv12 format is full-sized Y followed by half-sized U and half-size V
// forces dx to be even so aligned with U and V

int jhcBarcode::slice_v_yuv (int *slice, const unsigned char *src, int cdist, int fld)
{
  int val, base, w2 = w >> 1, ww = w << 1, sz = w * h, sz2 = w2 * (h >> 1);
  int dy, dx = (w2 + cdist) & 0xFFFE;
  const unsigned char *y = src + dx, *u = src + sz + (dx >> 1), *v = u + sz2;
  int *p = slice;
  
  // record slice geometry
  sx0 = dx;
  sy0 = 0;
  sdx = 0;
  sdy = 1;

  // collect pixels using U and V values for two successive Y values
  // Y moves by 2 full lines, U and V move by one half-sized line
  for (dy = 0; dy < h; dy += 2, p += 2, y += ww, u += w2, v += w2)
  {
    base = umult[fld][*u] + vmult[fld][*v];
    val = (base + ymult[y[0]]) >> 8;
    p[0] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);
    val = (base + ymult[y[w]]) >> 8;
    p[1] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);
  }

  // clean up and return length
  smooth(slice, h, sm);
  return h;
}


//= Generate a major diagonal intensity slice of a YV12 image.
// cdist is distance of slice NE (+) or SW (-) of image center (in RGB equiv)
// can specify which color channel to use, assumes slice array is big enough
// yv12 format is full-sized Y followed by half-sized U and half-size V
// forces x0 and y0 to be even so aligned with U and V

int jhcBarcode::slice_d1_yuv (int *slice, const unsigned char *src, int cdist, int fld)
{
  int w2 = w >> 1, h2 = h >> 1, skip = w + 1, sksk = skip << 1, skip2 = w2 + 1; 
  int coff = (cdist * 181) >> 8, xc = w2 + coff, yc = h2 - coff;  // adjust by 0.7071
  int i, val, base, bot = __min(xc, yc), top = __min(w - 1 - xc, h - 1 - yc);
  int x0 = (xc - bot) & 0xFFFE, y0 = (yc - bot) & 0xFFEE, len = bot + top + 1;
  int off = y0 * w + x0, off2 = (y0 >> 1) * w2 + (x0 >> 1);
  const unsigned char *y = src + off, *u = src + w * h + off2, *v = u + w2 * h2;
  int *p = slice;

  // record slice geometry
  sx0 = x0;
  sy0 = y0;
  sdx = 1;
  sdy = 1;

  // collect pixels using U and V values for two successive Y values
  // Y moves by 2 full lines plus 2 pixels, U and V by half-sized line plus one pixel
  for (i = 0; i < len; i += 2, p += 2, y += sksk, u += skip2, v += skip2)
  {
    base = umult[fld][*u] + vmult[fld][*v];
    val = (base + ymult[y[0]]) >> 8;
    p[0] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);
    val = (base + ymult[y[skip]]) >> 8;
    p[1] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);
  }

  // clean up and return length
  smooth(slice, h, sm);
  return h;
}


//= Generate a minor diagonal intensity slice of a YV12 image.
// cdist is distance of slice NW (+) or SE (-) of image center (in RGB equiv)
// can specify which color channel to use, assumes slice array is big enough
// yv12 format is full-sized Y followed by half-sized U and half-size V
// forces x0 and y0 to be even so aligned with U and V

int jhcBarcode::slice_d2_yuv (int *slice, const unsigned char *src, int cdist, int fld)
{
  int w2 = w >> 1, h2 = h >> 1, skip = w - 1, sksk = skip << 1, skip2 = w2 - 1; 
  int coff = (cdist * 181) >> 8, xc = w2 - coff, yc = h2 - coff;  // adjust by 0.7071
  int i, val, base, bot = __min(w - 2 - xc, yc), top = __min(xc, h - 1 - yc);
  int x0 = (xc + bot + 1) & 0xFFFE, y0 = (yc - bot) & 0xFFFE, len = bot + top + 1;
  int off = y0 * w + x0, off2 = (y0 >> 1) * w2 + (x0 >> 1);
  const unsigned char *y = src + off, *u = src + w * h + off2, *v = u + w2 * h2;
  int *p = slice;
  
  // record slice geometry
  sx0 = x0;
  sy0 = y0;
  sdx = -1;
  sdy = 1;

  // collect pixels using U and V values for two successive Y values
  // Y moves by 2 full lines less 2 pixels, U and V by half-sized line less one pixel
  for (i = 0; i < len; i += 2, p += 2, y += sksk, u += skip2, v += skip2)
  {
    base = umult[fld][*u] + vmult[fld][*v];
    val = (base + ymult[y[0]]) >> 8;
    p[0] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);
    val = (base + ymult[y[skip]]) >> 8;
    p[1] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);
  }

  // clean up and return length
  smooth(slice, len, sm);
  return len;
}


//= Convert YV12 image buffer to a RGB24 pixel buffer.
// for debugging primarily: does NOT rotate image but DOES bottom to top flip
// Note: output buffer needs to be twice the size of the input buffer

void jhcBarcode::YV12toRGB (unsigned char *bgr, const unsigned char *yuv)
{
  int dx, dy, f, val, base, sk = (ln << 1) + 3 * w;
  const unsigned char *y = yuv, *u = y + w * h, *v = u + (w >> 1) * (h >> 1);
  unsigned char *p = bgr + (h - 2) * ln;

  for (dy = 0; dy < h; dy += 2, p -= sk, y += w)
    for (dx = 0; dx < w; dx += 2, p += 6, y += 2, u++, v++)
      // decode 4 pixels with 3 color fields from 4 Y values and 1 U and 1 V
      //   ... B G R B G R ...  <==  ... Y Y ...  ... U ...  ... V ...
      //   ... B G R B G R ...       ... Y Y ...
      for (f = 0; f < 3; f++)
      {
        base = umult[f][*u] + vmult[f][*v];

        val = (base + ymult[y[w]]) >> 8;
        p[f] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);

        val = (base + ymult[y[w + 1]]) >> 8;
        p[3 + f] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);

        val = (base + ymult[y[0]]) >> 8;
        p[ln + f] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);

        val = (base + ymult[y[1]]) >> 8;
        p[ln + 3 + f] = ((val >= 0) ? ((val <= 255) ? val : 255) : 0);
      }
}


///////////////////////////////////////////////////////////////////////////
//                              Stripe Parsing                           //
///////////////////////////////////////////////////////////////////////////

//= Analyze intensity histogram as a string of UPC bits.
// finds start and end of barcode and generates an array of edge positions
// sets a number of global variables for bit width, edge count, etc.
// returns number of edges in array

int jhcBarcode::get_edges (int *marks, int& x0, int& x1, const int *slice, int len)
{
  int n, n2, t0, t1;

  // determine ends of barcode and edges of bars (ejs, ecnt, eth are global)
  // then record slice threshold and end indices
  find_limits(t0, t1, slice, len);
  n = mark_edges(marks, slice, t0, t1, bdiff, dbar);
  eth = bdiff;
  x0 = t0;
  x1 = t1;

  // get approximate bit width (bw16 is global) then
  // possibly refine overall barcode size based on start and end code
  // save limits of possible bar sizes (lo16, hi16 are global)
  bw16 = bit_width(marks, n);
  if (badj > 0)
    bw16 = trim_width(t0, t1, marks, n, bw16);  
  lo16 = (180 * bw16) >> 8;     // 0.7x estimate
  hi16 = (366 * bw16) >> 8;     // 1.4x estimate

  // see if refinement of threshold is desired but
  // copy trimmed barcode limits only if different edges found
  if (eadj <= 0)
    return n;
  n2 = search_edges(marks, n, proj, t0, t1);
  if (n2 != n)
  {
    x0 = t0;
    x1 = t1;
  }
  return n2;
}


//= Figure out the likely start and end band positions for interpreting barcode.
// returns width if okay, 0 if borders not found or limits not reasonable
 
int jhcBarcode::find_limits (int& x0, int& x1, const int *slice, int len)
{
  int x, flat, cent = len / 2;

  // set default span = nothing
  x0 = 0;
  x1 = 0;

  // look for first wide, low-slope region to the left of center
  flat = 0;
  for (x = cent - 1; x >= 0; x--)
    if (abs(slice[x] - slice[x + 1]) > dmax)
      flat = 0;
    else if (flat > wmin)
      break;
    else
      flat++;

  // record ending position if any  
  if (flat <= wmin)
    return 0;
  x0 = x + 1;
  x1 = x0;

  // look for first wide, low-slope region to the right of center
  flat = 0;
  for (x = cent + 1; x < len; x++)
    if (abs(slice[x] - slice[x - 1]) > dmax)
      flat = 0;
    else if (flat > wmin)
      break;
    else
      flat++;

  // record ending position if any  
  if (flat <= wmin)
    return 0;
  x1 = x - 1;
  return(x1 - x0);
}


//= Figure out bar edges given a gray scale slice and a range of interest.
// takes minimum change for edge (jump) and definition of sloping region (flat)
// max of 60 edges: (10 main + 2 aux) * 2 bars + 3 mark * 2 bars = 30 bars
// returns 0 if error, else number of edges found (typically 60, 48, or 34)

int jhcBarcode::mark_edges (int *marks, const int *slice, int x0, int x1, int jump, int flat)
{
  int trim = 0, x0p = x0 + trim, x1p = x1 - trim; 
  int x, i, step, diff, mid16, mid, base, gap;
  int slope, next = 0, pmark = 0, start = x0p, cnt = 0;

  // make sure barcode seems big enough
  if ((x1 - x0) < bmin)
    return 0;

  // look for peaks and troughs
  for (x = x0p; x <= x1p; x++)
  {
    // set next slope type
    step = slice[x + 1] - slice[x];
    slope = next;
    if (step > flat)
      next = 1;
    else  if (step < -flat)
      next = -1;
    else
      next = 0;

    // check for peak or trough (first ej must be trough)
    if (slope == 0)
      start = x;
    else if (((slope < 0) && (next > -1)) ||
             ((slope > 0) && (next < 1) && (cnt > 0)))
    {
      // check for minimum size and better than last of same sign (if any)
      diff = slice[x] - slice[start];
      if ((abs(diff) > jump) && 
          (((slope > 0) && (diff > pmark)) ||
           ((slope < 0) && (diff < pmark))))
      {
        // if same sign as last edge but stronger then overwrite
        if (((slope > 0) && (pmark > 0)) || 
            ((slope < 0) && (pmark < 0)))
          cnt--;

        // find pixel just before midpoint
        mid16 = (slice[x] + slice[start]) << 3;
        mid = mid16 >> 4;
        for (i = start; i < x; i++)
          if (((slope > 0) && (slice[i + 1] > mid)) ||
              ((slope < 0) && (slice[i + 1] < mid)))
            break;
 
        // refine position by adding in fractional interpolation (x16)
        // must start on a light area not a dark one
        base = slice[i];
        gap = slice[i + 1] - base;
        if (cnt >= 100)
          break;
        marks[cnt++] = (i << 4) + ((mid16 - (base << 4) + (gap >> 1)) / gap);
        pmark = diff;
      }

      // initiate next segment
      start = x;
    }
  }

  // last ej must be trough (for right to left reading)
  if (pmark < 0)
    cnt--; 
  return cnt;
}


//= Estimate the width of a single bar or space (x16 wrt edges).
//   UPC-A     = 60 edges (95 bits)
//   UPC-E + 2 = 48 edges (78 bits)
//   UPC-E     = 34 edges (51 bits)
// returns 256 * pixels or 0 if bad slice

int jhcBarcode::bit_width (const int *marks, int n)
{
  int wid16, i, div, span, v = 1, w0 = 100000, w1 = 100000;

  // EST 0: determine approximate number of bit samples 
  if (n >= 54)
    div = (n * 95) / 60;
  else if (n >= 41)
    div = (n * 78) / 48;
  else if (n >= 17)
    div = (n * 51) / 34;
  else
    return 0;

  // divide overall width by number of bits
  wid16 = (((marks[n - 1] - marks[0]) << 4) + (div >> 1)) / div;
  if (est <= 0)
    return wid16;

  // find smallest bar and space intervals 
  for (i = 1; i < n; i++)  
  {
    span = (marks[i] - marks[i - 1]) << 4;
    if (v > 0)
    {
      w1 = __min(w1, span);
      v = 0;
    }
    else
    {
      w0 = __min(w0, span);
      v = 1;
    }
  }

  // average in size of smallest bar or space 
  wid16 = (wid16 + __max(w0, w1)) >> 1;
  return wid16;
}
  

//= Finds valid start and end codes then recomputes expected bit width.
// note that both UPC-A and UPC-E have 101 patterns at each end
// can use previous estimate of width in order to find corect pattern
// returns 256 * pixels or 0 if bad slice

int jhcBarcode::trim_width (int& x0, int& x1, const int *marks, int n, int w0)
{
  int wid16 = 4, lo = 2, hi = 8;
  const int *e0, *e1;

  // make sure there are enough edges for a start and stop
  if (n < 8)
    return 0;

  // set limits for expected bar size
  if (w0 > 0)
  {
    wid16 = w0;
    lo = (180 * w0) >> 8;
    hi = (366 * w0) >> 8;  
  }

  // find edge for valid start code (or stop in reverse)
  e0 = edge_start(marks, n, 1, wid16, lo, hi);
  if (e0 == NULL)
    return 0;
  e0 -= 3;

  // find edge for valid stop code (or start in reverse)
  e1 = edge_start(marks, n, -1, wid16, lo, hi);
  if (e1 == NULL)
    return 0;
  e1 += 3;

  // get improved bit width estimate from trimmed size
  wid16 = bit_width(e0, (int)(e1 - e0 + 1));
  if (wid16 <= 0)
    return 0;

  // record new start and end positions in global variables
  x0 = ((*e0) >> 4) - wmin;
  x1 = ((*e1) >> 4) + wmin;
  return wid16;
}


//= Try a variety of threshold to get close to the proper number of edges.
// re-analyzes the intensity slice to give a better set of edges in array
// ignores UPC-E + 2 (48 edges) assuming that barcode limits have been refined
// saves new edges in "marks" array and returns length of collection

int jhcBarcode::search_edges (int *marks, int n, const int *slice, int x0, int x1)
{
  int n2, target = ((n > 45) ? 60 : 34), err = 0, th = 0, dth = 80, stop = 5;

  // see if any refinement needed
  if ((n == target) || (n == 0))
    return n;

  // do a binary search by varying the edge change threshold
  eth = 0;
  while (dth >= stop)
  {
    // raise or lower the threshold 
    if (err >= 0)
      eth += dth;    // fewer edges
    else 
      eth -= dth;    // more edges
    dth >>= 1;

    // see how many edges there are at current threshold
    n2 = mark_edges(marks, slice, x0, x1, eth, eth >> 2);
    err = n2 - target;
    if (err == 0)
      break;
  }

  // return exact match or most recent approximation
  // recompute bit width and limits
  bw16 = bit_width(marks, n2);
  lo16 = (180 * bw16) >> 8;     // 0.7x estimate
  hi16 = (366 * bw16) >> 8;     // 1.4x estimate
  return n2;
}


//= Convert a set of edge positions into a bit pattern in one shot.
// black bars correspond to ones, white spaces to zeroes
// takes as input estimate of width of single bit (in pixels x16)
// returns number of bits if succcessful, 0 if bad slice

int jhcBarcode::bit_vect (int *vals, const int *marks, int n, int wid)
{
  int i, j, span, run, bw8 = wid >> 1;
  int v = 1, extra = 0, end = n - 1, start = 0;

  // scan through pairs of edges and quantize
  for (i = 0; i < end; i++)  
  {
    // divide bar/space width into an integer number of bits
    span = ((marks[i + 1] - marks[i]) << 4) + extra;
    run = (span + bw8) / wid;
    run = __max(1, run);
    extra = (span - run * wid) >> 1;  // adjust edge a little 

    // write a sequence of zeroes or ones to bit array
    for (j = 0; j < run; j++)
      if ((start + j) >= 200)
        return 200; 
      else
        vals[start + j] = v; 

    // advance bit vector position and change polarity of bar
    start += run;
    if (v > 0)
      v = 0;
    else
      v = 1;
  }
  return start;
}


///////////////////////////////////////////////////////////////////////////
//                          UPC-A interpretation                         //
///////////////////////////////////////////////////////////////////////////

//= Turns bit pattern into UPC code string of digits (in ASCII).
// can save a little time if both = 0 (left ot right only)
// returns 1 if okay, 0 if contraints violated

int jhcBarcode::get_code (char *code, const int *vals, int n)
{
  int i, both = 1;
  char partial[20] = "xxxxxxxxxx";

  // set default for no valid read
  *code = '\0';

  if (((pod <= 0) && (n >= 87)) ||     // 87 bits if already parsed
      ((pod > 0)  && (n >= 54)))       // 54 edges if local parsing  
  {
    // try reading as a UPC-A code
    if ((parse_a(partial, vals, n, 1) <= 0) && (both > 0))
      if (parse_a(partial, vals, n, -1) <= 0)
        return 0;
  }
  else
  {
    // try reading as a UPC-E code (ignore +2 digits if any)
    partial[6] = '\0';
    if ((parse_e(partial, vals, n, 1) <= 0) && (both > 0))
      if (parse_e(partial, vals, n, -1) <= 0)
        return 0;
  }

  // copy full results to output
  if ((n < 87) && (cvt > 0))
    a_from_e(code, partial);       // expand 6 digit code to 10 digits
  else
    for (i = 0; i <= 10; i++)
      code[i] = partial[i];
  return 1;
}


//= Tries to interpret bit pattern as a UPC-A string of digits (in ASCII).
// UPC-A pattern: 101-z-L-L-L-L-L-01010-R-R-R-R-R-c-101
//   where z = 7 bit zero code; L, R = 7 bit digit; c = 7 bit checksum 
// returns 1 if okay, 0 if contraints violated

int jhcBarcode::parse_a (char *code, const int *vals, int n, int dir)
{
  int i, seq;
  const int *now, *last = vals + ((dir > 0) ? (n - 1) : 0);
  int pat[12];
 
  // see if special lattice decoding selected instead
  if (pod >= 2)
    return lattice_a(code, vals, n, dir);

  // check for proper start pattern 
  if ((now = find_start(vals, n, dir)) == NULL)
    return 0;

  // decode left half = number sys + 5 digits
  for (i = 0; i < 6; i++)
  {
    if ((now = get_pattern(seq, 7, now, last, 4)) == NULL)
      return 0;
    if ((pat[i] = valid_digit(seq, 0)) < 0)
      return 0;
  }

  // check for center divider
  if ((now = get_pattern(seq, 5, now, last, 5)) == NULL) 
    return 0;
  if (seq != 0x0A)
    return 0;

  // decode right half = 5 digits + checksum
  for (i = 6; i < 12; i++)
  {
    if ((now = get_pattern(seq, 7, now, last, -4)) == NULL)
      return 0;
    if ((pat[i] = valid_digit(~seq, 0)) < 0)
      return 0;
  }

  // check for terminator pattern 
  if ((now = get_pattern(seq, 3, now, last, -3)) == NULL)
    return 0;
  if ((seq & 0x07) != 0x05)
    return 0;

  // check for end of code or white afterward (not wide bar)
  if ((get_pattern(seq, 1, now, last, 0) != NULL) && (seq != 0x00))
    return 0;

  // verify barcode checksum and convert to string
  return correct_a(code, pat);
}


//= Get most likely interpretation of edges as a valid UPC-A code.
// returns 0 if no code seems reasonable

int jhcBarcode::lattice_a (char *code, const int *marks, int n, int dir)
{
  int i, bw = bw16, lo = lo16, hi = hi16, nvar = 3;
  const int *end, *now, *last = marks + ((dir > 0) ? (n - 1) : 0);

  // check for proper start pattern 
  if ((now = edge_start(marks, n, dir, bw, lo, hi)) == NULL)
    return 0;

  // grab potentials for numeric system and first 5 digits
  for (i = 0; i < 6; i++)
    if ((now = digit_prefs(lattice[i], nvar, now, last, 0)) == NULL)
      return 0;

  // look for central divider pattern
  if ((now = square_wave(now, last, 5, bw, lo, hi)) == NULL)
    return 0;

  // grab potentials for second 5 digits and checksum
  for (i = 6; i < 12; i++)
    if ((now = digit_prefs(lattice[i], nvar, now, last, 0)) == NULL)
      return 0;

  // make sure terminator exists then attempt complete decoding
  if ((end = square_wave(now, last, 3, bw, lo, hi)) == NULL)
    return 0;
  return best_path_a(code, lattice);
}


// Look through list of possible digits and find valid UPC-A sequence.
// tries most likely combinations first until checksum is valid
// only fixes one wrong digit (more would make checksum suspect)
// preference lattice has 12 entries with up to 3 choices and looks like:
// <pre>
//
//    cnt1   best1   alt1   ...  worst1   (number sys)
//    cnt2   best2   alt2   ...  worst2   (first digit)
//     :       :       :           :
//     :       :       :           :
//    cnt11  best11  alt11  ...  worst11  (tenth digit)
//    cnt12  best12  alt12  ...  worst12  (checksum)
//
// </pre>
// returns 0 if no code seems likely

int jhcBarcode::best_path_a (char *code, int all[][4])
{
  int i, v, any = 0;
  int pat[12];

  // initialize to best guesses for all digits then test pattern
  for (i = 0; i < 12; i++)
    pat[i] = all[i][1];
  if (correct_a(code, pat) > 0)
    return 1;

  // select digit position to alter 
  for (i = 0; i < 12; i++)          
  {
    // pick variant value to use then test altered pattern
    for (v = 2; v <= all[i][0]; v++)
    {
      pat[i] = all[i][v];
      if (correct_a(code, pat) > 0)
        if (any++ > 0)
          return 0;  // more than one non-best interpretation
    }

    // revert to best guess and select different position
    pat[i] = all[i][1];
  }
  return any;
}


//= See if pattern of digits is an acceptable UPC-A barcode.
// also generates output string from center portion of code

int jhcBarcode::correct_a (char *code, int *pat)
{
  int i, check = 0;

  // compute checksum for sequence 
  for (i = 0; i < 11; i++)
  {
    check += pat[i];
    if ((i & 0x01) == 0)
      check += (pat[i] << 1);  // weight 3 for number sys and digits 1, 3, 5, 7, 9
  }
  check %= 10;
  if (check > 0)
    check = 10 - check;

  // if it matches then assemble digit pattern into barcode string
  if (pat[11] != check)
    return 0;
  for (i = 0; i < 10; i++)
    code[i] = '0' + pat[i + 1];
  code[10] = '\0';
  return 1;
}


//= Interpret pattern of 7 bits as a UPC-A or UPC-E digit from 0 to 9.
// returns value (bit 4 set if even UPC-E encoding) or -1 if invalid code

int jhcBarcode::valid_digit (int seq, int ecode)
{
  int probe = seq & 0x7F;

  // UPC-A digits (and also plain UPC-E digits)
  switch (probe)
  {
    case 0x0D:
      return 0;
    case 0x19:
      return 1;
    case 0x13:
      return 2;
    case 0x3D:
      return 3;
    case 0x23:
      return 4;
    case 0x31:
      return 5;
    case 0x2F:
      return 6;
    case 0x3B:
      return 7;
    case 0x37:
      return 8;
    case 0x0B:
      return 9;
  }
  if (ecode <= 0)
    return -1;

  // UPC-E alternate digits
  switch (probe)
  {
    case 0x27:
      return 0x10;
    case 0x33:
      return 0x11;
    case 0x1B:
      return 0x12;
    case 0x21:
      return 0x13;
    case 0x1D:
      return 0x14;
    case 0x39:
      return 0x15;
    case 0x05:
      return 0x16;
    case 0x11:
      return 0x17;
    case 0x09:
      return 0x18;
    case 0x17:
      return 0x19;
  }
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                         UPC-E interpretation                          //
///////////////////////////////////////////////////////////////////////////

//= Tries to interpret bit pattern as a UPC-E string of digits (in ASCII).
// UPC-E pattern: 101-L-L'-L'-L-L-L'-010101
//   where L = 7 bit digit, L' = 7 bit digit with alternative coding
// check digit recovered from pattern of regular and alternative digits
// returns 1 if okay, 0 if contraints violated

int jhcBarcode::parse_e (char *code, const int *vals, int n, int dir)
{
  int i, seq;
  const int *now, *last = vals + ((dir > 0) ? (n - 1) : 0);
  int pat[6];
  
  // see if special lattice decoding selected instead
  if (pod >= 2)
    return lattice_e(code, vals, n, dir);

  // check for proper start pattern
  if ((now = find_start(vals, n, dir)) == NULL)
    return 0;

  // decode UPC-E as left half of a UPC-A code 
  for (i = 0; i < 6; i++)
  {
    if ((now = get_pattern(seq, 7, now, last, 4)) == NULL)
      return 0;
    if ((pat[i] = valid_digit(seq, 1)) < 0)
      return 0;
  }

  // check for terminator pattern 
  if ((now = get_pattern(seq, 6, now, last, 6)) == NULL)
    return 0;
  if (seq != 0x15)
    return 0;

  // check for end of code or white afterward (not wide bar)
  if ((get_pattern(seq, 1, now, last, 0) != NULL) && (seq != 0x00))
    return 0;

  // verify barcode checksum and convert to string
  return correct_e(code, pat);
}


//= Get most likely interpretation of edges as a valid UPC-E code.
// returns 0 if no code seems reasonable

int jhcBarcode::lattice_e (char *code, const int *marks, int n, int dir)
{
  int i, bw = bw16, lo = lo16, hi = hi16, nvar = 3;
  const int *end, *now, *last = marks + ((dir > 0) ? (n - 1) : 0);

  // check for proper start pattern 
  if ((now = edge_start(marks, n, dir, bw, lo, hi)) == NULL)
    return 0;

  // grab potentials for 6 digits
  for (i = 0; i < 6; i++)
    if ((now = digit_prefs(lattice[i], nvar, now, last, 1)) == NULL)
      return 0;
  
  // make sure terminator exists then attempt complete decoding
  if ((end = square_wave(now, last, 6, bw, lo, hi)) == NULL)
    return 0;
  return best_path_e(code, lattice);
}


// Look through list of possible digits and find valid UPC-E sequence.
// tries most likely combinations first until checksum is valid
// only fixes one wrong digit (more would make checksum suspect)
// preference lattice has 6 entries with up to 3 choices and looks like:
// <pre>
//
//    cnt1   best1   alt1   ...  worst1   (first digit)
//    cnt2   best2   alt2   ...  worst2   (second digit)
//    cnt3   best3   alt3   ...  worst3   (third digit)
//    cnt4   best4   alt4   ...  worst4   (fourth digit)
//    cnt5   best5   alt5   ...  worst5   (fifth digit)
//    cnt6   best6   alt6   ...  worst6   (sixth digit)
//
// </pre>
// returns 0 if no code seems likely

int jhcBarcode::best_path_e (char *code, int all[][4])
{
  int i, v, any = 0;
  int pat[6];

  // initialize to best guesses for all digits then test pattern
  for (i = 0; i < 6; i++)
    pat[i] = all[i][1];
  if (correct_e(code, pat) > 0)
    return 1;

  // select digit position to alter 
  for (i = 0; i < 6; i++)          
  {
    // pick variant value to use then test altered pattern
    for (v = 2; v <= all[i][0]; v++)
    {
      pat[i] = all[i][v];
      if (correct_e(code, pat) > 0)
        if (any++ > 0)
          return 0;  // more than one non-best interpretation
    }

    // revert to best guess and select different position
    pat[i] = all[i][1];
  }
  return any;
}


//= See if pattern of digits is an acceptable UPC-E barcode.
// also generates output string from center portion of code

int jhcBarcode::correct_e (char *code, int *pat)
{
  int i, check, parity = 0;
  int num[6];

  // compute parity sequence based on special coding of digits
  for (i = 0; i < 6; i++)
  {
    num[i] = pat[i] & 0x0F;
    parity <<= 1;
    if ((pat[i] & 0x10) != 0)
      parity |= 0x01;
  }
  if ((check = e_check(parity)) < 0)
    return 0;

  // if it matches then assemble digit pattern into barcode string
  if (check != a_check(num))
    return 0;;
  for (i = 0; i < 6; i++)
    code[i] = '0' + num[i];
  code[10] = '\0';
  return 1;
}


//= Looks up the appropriate checksum given a UPC-E parity pattern.
// in the parity pattern 1 means even while 0 means odd (num sys 0)

int jhcBarcode::e_check (int ppat)
{
  switch (ppat & 0x3F)
  {
    case 0x38:
      return 0;
    case 0x34:
      return 1;
    case 0x32:
      return 2;
    case 0x31:
      return 3;
    case 0x2C:
      return 4;
    case 0x26:
      return 5;
    case 0x23:
      return 6;
    case 0x2A:
      return 7;
    case 0x29:
      return 8;
    case 0x25:
      return 9;
  }
  return -1;
}


//= Recreates UPC-A code from UPC-E code then computes original check digit.
// reconstruction depends on the last digit of the UPC-E code
// <pre>
//
//   last       UPC-E digits         UPC-A wts
//    0-2 = [M1 M2 P3 P4 P5 M3] -> [1 3 3 1 3 1]
//     3  = [M1 M2 M3 P4 P5  3] -> [1 3 1 1 3 0]
//     4  = [M1 M2 M3 M4 P5  4] -> [1 3 1 3 3 0]
//    5-9 = [M1 M2 M3 M4 M5 P5] -> [1 3 1 3 1 3]
//
// </pre>
// UPC-A [M1 M2 M3 M4 M5 - P1 P2 P3 P4 P5] has wts [1 3 1 3 1 - 3 1 3 1 3]

int jhcBarcode::a_check (int *e_code)
{
  int check;

  // apply basic weight pattern of [1 3 1 1 1 1]
  check = e_code[0] + e_code[1] + e_code[2] + e_code[3] + e_code[4] + e_code[5];
  check += (e_code[1] << 1);

  // adjust for digits with weight 3 or 0
  if (e_code[5] <= 2)
    check += (e_code[2] + e_code[4]) << 1;        // adds [0 0 2 0 2 0] 
  else if (e_code[5] == 3)
    check += (e_code[4] << 1) - 3;                // adds [0 0 0 0 2 -1]
  else if (e_code[5] == 4)
    check += ((e_code[3] + e_code[4]) << 1) - 4;  // adds [0 0 0 2 2 -1]
  else
    check += (e_code[3] + e_code[5]) << 1;        // adds [0 0 0 2 0 2]

  // convert sum to check digit
  check %= 10;
  if (check > 0)
    check = 10 - check;
  return check;
}

  
//= Converts a UPC-E barcode into the equivalent UPC-A code.
// assumes a valid e_code and enough space for the a_code string
// see function a_check for documentation on scrambling patterns

void jhcBarcode::a_from_e (char *a_code, const char *e_code)
{
  int i;

  // some parts are the same no matter which scrambling
  // UPC-E [M1 M2 x x x x] -> UPC-A [M1 M2 0 0 0 - 0 0 0 0 0]
  a_code[0] = e_code[0];
  a_code[1] = e_code[1];
  for (i = 2; i < 10; i++)
    a_code[i] = '0';
  a_code[10] = '\0';

  // unscramble depending on last digit
  switch (e_code[5])
  {
    case '0':                 // UPC-E [x x P3 P4 P5 M3]
    case '1':
    case '2':
      a_code[7] = e_code[2];
      a_code[8] = e_code[3];
      a_code[9] = e_code[4];
      a_code[2] = e_code[5];
      break;

    case '3':                 // UPC-E [x x M3 P4 P5 x]
      a_code[2] = e_code[2];
      a_code[8] = e_code[3];
      a_code[9] = e_code[4];
      break;

    case '4':                 // UPC-E [x x M3 M4 P5 x]
      a_code[2] = e_code[2];
      a_code[2] = e_code[3];
      a_code[9] = e_code[4];
      break;

    default:                  // UPC-E [x x M3 M4 M5 P5]
      a_code[2] = e_code[2];
      a_code[3] = e_code[3];
      a_code[4] = e_code[4];
      a_code[9] = e_code[5];
  }
}


///////////////////////////////////////////////////////////////////////////
//                           Common bar parsing                          //
///////////////////////////////////////////////////////////////////////////

//= Looks for proper starting pattern either in bits or directly in edges.
// if pod > 0 then returns pointer into ejs, else pointer into bits 
// uses global values of bit width (lo16 and hi16)

const int *jhcBarcode::find_start (const int *vals, int n, int dir)
{
  resid = 0;
  if (pod <= 0)
    return bit_start(vals, n, dir);
  return edge_start(vals, n, dir, bw16, lo16, hi16);
}


//= Looks for start code of "101" followed by "0" in bit array.
// scans bit array in given direction (1 = left to right)
// returns pointer to value after start code, NULL if not found

const int *jhcBarcode::bit_start (const int *vals, int n, int dir)
{
  int i;

  // scan left to right
  if (dir > 0)
  {
    for (i = 3; i < n; i++)
      if ((vals[i - 3] == 1) && (vals[i - 2] == 0) && (vals[i - 1] == 1) && (vals[i] == 0))
        return(vals + i);
    return NULL;
  }

  // scan right to left
  for (i = n - 1; i >= 3; i--)
    if ((vals[i] == 1) && (vals[i - 1] == 0) && (vals[i - 2] == 1) && (vals[i - 3] == 0))
      return(vals + i - 3);
  return NULL;
}


//= Looks for start code of "101" followed by "0" in edge array.
// scans edge array in given direction (1 = left to right, -1 = right to left)
// takes estimate and limits on the possible widths of bars (pixels x16)
// gets each pair of bars and sees if evenly divides interval in three
// also checks that inferred bar width is close to estimated width
// <pre>
//
//     +----+    +----+       
//     |    |    |    |       est = (e3 - e0) / 3
//   --+    +----+    +--     0.7 * wid < est < 1.4 * wid
//     e0   e1   e2   e3      (e3 - e2) == (e2 - e1) == (e1 - e0)
//
// </pre>
// returns pointer to trailing edge of start pattern, NULL if not found

const int *jhcBarcode::edge_start (const int *marks, int n, int dir, int bw, int lo, int hi)
{
  int i, lim = n - 4, d2 = dir + dir; 
  const int *last = marks + ((dir > 0) ? (n - 1) : 0);
  const int *e, *m = marks + ((dir > 0) ? 0 : (n - 1));

  // step through edges until proper pattern found
  for (i = 0; i < lim; i += 2, m += d2)
    if ((e = square_wave(m, last, 3, bw, lo, hi)) != NULL)
      return e;
  return NULL;
}


//= Looks for pattern of "010101 ..." in edges where bars and spaces are equal.
// looks at a total of N intervals starting from the current point
// returns pointer to last transition of pattern if successful, else NULL

const int *jhcBarcode::square_wave (const int *marks, const int *last, int n, int bw, int lo, int hi)
{
  int i, span, extra = resid, dir = ((last > marks) ? 1 : -1), nd = n * dir;
  const int *m = marks;

  // make sure there are enough edges
  if (abs((int)(last - marks)) < n)
    return NULL;

  // check that the N stripes span N bit periods
  span = (abs(*(m + nd) - *m) << 4) + extra;
  if ((span < (n * lo)) || (span > (n * hi)))
    return NULL;

  // check that each bar or space is about 1 bit wide
  for (i = 0; i < n; i++, m += dir)
  {
    span = (abs(*(m + dir) - *m) << 4) + extra;
    if ((span < lo) || (span > hi))
      break;
    extra = (span - bw) >> 1;    
  }

  // return start of next bar and accumulated bit error
  if (i >= n)
  {
    resid = extra;
    return m;
  }
  return NULL;
}


//= Generates an N bit value starting at current location.
// for ejs takes k next bars and uses global bit size estimate (lo16 and hi16)
// returns pointer to next unused element in input array, NULL if error

const int *jhcBarcode::get_pattern (int& seq, int n, const int *vals, const int *last, int k)
{
  if (pod <= 0)
    return group_bits(seq, n, vals, last);
  return group_edges(seq, n, vals, last, k, bw16, lo16, hi16);
}


//= Assembles a set of N adjacent bits (big-endian) into a single value.
// direction inferred from relative order of now and last pointers
// returns pointer to next unused bit in input array, NULL if error

const int *jhcBarcode::group_bits (int& seq, int n, const int *vals, const int *last)
{
  int i, ans = 0, dir = ((last > vals) ? 1 : -1);
  const int *v = vals; 

  // make sure there are enough bits
  if (abs((int)(last - vals)) < (n - 1))
    return NULL;

  // shift bits into number from right
  seq = 0;
  for (i = 0; i < n; i++, v += dir)
  {
    seq <<= 1;
    if (*v > 0)
      seq |= 0x01;
  }
  return v;
} 


//= Assembles a set of K edges to yield N adjacent bits as a single value.
// direction inferred from relative order of now and last pointers
// <pre>
//
//         --+    +---------+    +--------------+       
//           |    |   bar   |    |     bar      |    
//   edges   +----+         +----+              +--
//           e0   e1        e2   e3             e4    ==> e4
// 
//     bit   | b0 | b1 | b2 | b3 | b4 | b5 | b6 |
//  
//     seq      0    1    1    0    1    1    1      ==> 0x37
//
// </pre>
// a negative number of transitions causes the answer to be inverted (7 bits)
// takes limits on expected bit width (lo and hi) in x16 pixels
// returns pointer to next unused edge in input array, NULL if error

const int *jhcBarcode::group_edges (int& seq, int n, const int *vals, const int *last, 
                                    int k, int bw, int lo, int hi)
{
  int kp = abs(k), dir = ((last > vals) ? 1 : -1);
  int i, j, span, run, w8, w16 = bw, b = 0, sum = 0, extra = resid;  
  const int *m = vals;

  // assume terminating blank space is always there
  seq = 0;
  if (kp == 0)
    return vals;

  // make sure there are enough edges for number of transititions requested
  if (abs((int)(last - vals)) < (kp - 1))
    return NULL;

  // EST 2: determine local bit width = pod / bits and check if reasonable
  if (est >= 2)
  {
    if (dir > 0)
      w16 = ((*(vals + kp) - *vals) << 4) / n;
    else
      w16 = ((*vals - *(vals - kp)) << 4) / n;
    if ((w16 < lo) || (w16 > hi))
      return NULL;
  }

  // scan through pairs of edges and quantize
  w8 = w16 >> 1;
  for (i = 0; i < kp; i++, m += dir)  
  {
    // divide bar/space width into an integer number of bits
    span = (abs(*(m + dir) - *m) << 4) + extra;
    run = (span + w8) / w16;
    run = __max(1, run);
    extra = (span - run * w16) >> 1;  // adjust edge a little 

    // write a sequence of zeroes or ones to output value
    if (b > 0)
    {
      for (j = 0; j < run; j++)
        seq = (seq << 1) | 0x01;
      b = 0;
    }
    else
    {
      seq <<= run;
      b = 1; 
    }
    sum += run;
  }
  resid = extra;    // save for next digit

  // see if sequence should be inverted (assume 7 bits max)
  if (sum != n)
    return NULL;
  if (k < 0)
    seq = (~seq) & 0x7F;
  return m;
} 


//= Get one or more interpretation for 2 bar UPC-A or UPC-E digit.
// direction inferred from relative order of now and last pointers
// does not carry over residual bit error from one digit to another
// "prefs" gets count followed by preferred digit interpretations (in order)
// returns NULL if no valid pattern seems reasonable, else next edge

const int *jhcBarcode::digit_prefs (int *prefs, int np, const int *marks, const int *last, int ecode)
{
  int alt[16] = {0x0, 0x8, 0x4, 0x2, 0x1,       // 0 or 1 spans changed
                 0xC, 0xA, 0x9, 0x6, 0x5, 0x3,  // 2 spans changed
                 0xE, 0xB, 0xA, 0x7, 0xF};      // 3 or 4 spans changed
  int w16 = bw16, dir = ((last > marks) ? 1 : -1);
  int i, j, seq, digit, end, start = resid, cnt = 0;

  // make sure there are enough edges
  if (abs((int)(last - marks)) < 4)
    return NULL;

/*
  // EST 2: determine local bit width = pod / bits and check if reasonable
  // Note: local bit width estimation seems to give poor results
  if (est >= 2)
  {
    w16 = (abs(*(marks + (dir << 2)) - *marks) << 4) / 7;
    if ((w16 < lo16) || (w16 > hi16))
      return NULL;
  }
*/

  // try each of the deviation patterns 
  for (i = 0; i < 16; i++)
  {
     // restore original leftover fractional bar and try decoding digit
     // save bit residual from best guess pattern (no perturbations)
     resid = start;
     seq = perturb_pat(marks, dir, w16, alt[i]);
     if (i == 0)
       end = resid;

     // check that bit sequence is a valid numeric digit
     if ((seq >= 0) && ((digit = valid_digit(seq, ecode)) >= 0))
     {
       // see if digit is already in list
       for (j = 1; j <= cnt; j++)
         if (prefs[j] == digit)
           break;

       // record new variant then stop if enough already
       if (j > cnt)
         prefs[++cnt] = digit; 
       if (cnt >= np)
         break;
     }
  }

  // record how many possibilities found overall
  if (cnt == 0)
    return NULL;
  prefs[0] = cnt;    

  // restore fraction leftover bar from best guess and advance edge pointer
  resid = end;
  return(marks + (dir << 2));
}


//= Try interpreting 4 edges as a 7 bit value with some variant rounding.
// attempts to alter the size of intervals where bits of "chg" are nonzero
// return -1 if bad parsing or same as original

int jhcBarcode::perturb_pat (const int *marks, int dir, int w16, int chg)
{
  int i, j, span, run, orig, w8 = w16 >> 1, w4 = w16 >> 2, w12 = w8 + w4;
  int seq = 0, sum = 0, b = 0, mask = 0x8, extra = resid;
  const int *m = marks;

  // look at 4 edges worth of data (= 2 bars)
  for (i = 0; i < 4; i++, mask >>= 1, m += dir)
  {
    // divide bar/space width into an integer number of bits
    span = (abs(*(m + dir) - *m) << 4) + extra;
    run = (span + w8) / w16;
    run = __max(1, run);

    // possibly try perturbing the width of this bar or space
    if ((chg & mask) != 0)
    {
      orig = run;
      run = (span + w4) / w16;     // imagine it smaller
      run = __max(1, run);
      if (run == orig)
      {
        run = (span + w12) / w16;  // imagine it bigger
        run = __max(1, run);
      }
    }
      
    // set up to adjust next edge a little
    extra = (span - run * w16) >> 1;  // adjust edge a little 
    sum += run;

    // write a sequence of zeroes or ones to output value
    if (b > 0)
    {
      for (j = 0; j < run; j++)
        seq = (seq << 1) | 0x01;
      b = 0;
    }
    else
    {
      seq <<= run;
      b = 1; 
    }
  }
  resid = extra;    // save for next digit

  // see if sequence is proper length
  if (sum != 7)
    return -1;
  return seq;
}

