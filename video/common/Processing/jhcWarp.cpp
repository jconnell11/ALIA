// jhcWarp.cpp : perform arbitrary remapping of image pixels
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2019 IBM Corporation
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

#include "Processing/jhcWarp.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcWarp::jhcWarp ()
{
  SrcSize(320, 240);
  DestSize(320, 240);
  Identity();
}


//= Set sizes of internal images based on a reference image.

void jhcWarp::DestSize (const jhcImg& ref)
{
  DestSize(ref.XDim(), ref.YDim());
}


//= Set sizes of internal images directly.

void jhcWarp::DestSize (int x, int y)
{
  dw = x;
  dh = y;
  off.SetSize(x, y, 4);
  mix.SetSize(x, y, 2);
}


//= Set sizes of expected input based on a reference image.
// Note: input image can be bigger or smaller than destination

void jhcWarp::SrcSize (const jhcImg& ref)
{
  SrcSize(ref.XDim(), ref.YDim(), ref.Fields());
}


//= Tell system size of input images to speed up calculation.
// Note: input image can be bigger or smaller than destination

void jhcWarp::SrcSize (int x, int y, int f)
{
  sw = x;
  sh = y;
  sln = (x * f + 3) & 0xFFFC;
  nf = f;
}


//= Set input and output images to the same size and clear mapping.

void jhcWarp::InitSize (int x, int y, int f)
{
  SrcSize(x, y, f);
  DestSize(x, y);
  ClrMap();
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Mark all output points as invalid sampling positions.

void jhcWarp::ClrMap ()
{
  UL32 *z = (UL32 *) off.PxlDest();
  int x, y;

  for (y = 0; y < dh; y++)
    for (x = 0; x < dw; x++, z++)
      *z = 0xFFFFFFFF;
}


//= Set up remapping array by telling sampling location for output pixel.
// typically would loop through all output pixels to define complete map
// returns 1 if okay, 0 if source outside image, -1 if destination outside

int jhcWarp::SetWarp (int xd, int yd, double xs, double ys)
{
  int ix, iy, fx, fy;

  // check for valid output pixel location
  if ((xd < 0) || (xd >= dw) || (yd < 0) || (yd >= dh))
    return -1;

  // check for valid input pixel location
  if ((xs < 0.0) || (xs >= (sw - 1.0)) || (ys < 0.0) || (ys >= (sh - 1.0)))
  {
    off.ASet32(xd, yd, 0xFFFFFFFF);
    return 0;
  }

  // get integer and fractional parts of sampling location
  ix = (int) xs;
  iy = (int) ys;
  off.ASet32(xd, yd, (UL32)(iy * sln + ix * nf));

  // save fractional interpolation coefficients
  fx = ROUND(256.0 * (xs - ix));
  fx = __min(fx, 255);
  fy = ROUND(256.0 * (ys - iy));
  fy = __min(fy, 255);
  mix.ASet16(xd, yd, (fx << 8) | fy);
  return 1;
}


//= Apply cached warping function to input image.
// can specify arbitrary color for pixels not found in source
// Note: input image can be bigger or smaller than destination

int jhcWarp::Warp (jhcImg& dest, const jhcImg& src, int r0, int g0, int b0) const
{
  if (!dest.SameFormat(dw, dh, nf) || !src.SameFormat(sw, sh, nf))
    return Fatal("Bad images to jhcWarp::Warp");
  if (nf == 1)
    map_mono(dest, src, r0);
  else
    map_color(dest, src, r0, g0, b0);
  return 1;
}


//= Resamples a monochrome image into a monochrome result.

void jhcWarp::map_mono (jhcImg& dest, const jhcImg& src, int v0) const
{
  int x, y, fx, fy, lo, hi, val;
  int msk = mix.Skip() >> 1, dsk = dest.Skip();
  UL32 zlim = src.Line() * src.YDim();
  const UL32 *z = (UL32 *) off.PxlSrc();
  const US16 *m = (US16 *) mix.PxlSrc();
  const UC8 *bot, *top, *s = src.PxlSrc();
  UC8 *d = dest.PxlDest();

  // see if integer sampling position is valid
  for (y = 0; y < dh; y++, d += dsk, m += msk)
    for (x = 0; x < dw; x++, d++, z++, m++)
      if (*z >= zlim)
        *d = (UC8) v0;
      else
      {
        // get coeffients and corner of quartet
        fx = (*m) >> 8;
        fy = (*m) & 0xFF;
        bot = s + (*z);
        top = bot + sln;

        // interpolate pixel
        lo  = (256 - fx) * bot[0] + fx * bot[1];
        hi  = (256 - fx) * top[0] + fx * top[1];
        val = (256 - fy) * lo + fy * hi;
        *d = (UC8)(val >> 16);
      }
}


//= Resamples an RGB image into an RGB result.

void jhcWarp::map_color (jhcImg& dest, const jhcImg& src, int r0, int g0, int b0) const
{
  int x, y, fx, fy, lo, hi, val;
  int msk = mix.Skip() >> 1, dsk = dest.Skip();
  UL32 zlim = src.Line() * src.YDim();
  const UL32 *z = (UL32 *) off.PxlSrc();
  const US16 *m = (US16 *) mix.PxlSrc();
  const UC8 *bot, *top, *s = src.PxlSrc();
  UC8 *d = dest.PxlDest();

  // see if integer sampling position is valid
  for (y = 0; y < dh; y++, d += dsk, m += msk)
    for (x = 0; x < dw; x++, d += 3, z++, m++)
      if (*z >= zlim)
      {
        // default value
        d[0] = (UC8) b0;
        d[1] = (UC8) g0;
        d[2] = (UC8) r0;
      }
      else
      {
        // get coeffients and corner of quartet
        fx = (*m) >> 8;
        fy = (*m) & 0xFF;
        bot = s + (*z);
        top = bot + sln;

        // interpolate blue pixel
        lo  = (256 - fx) * bot[0] + fx * bot[3];
        hi  = (256 - fx) * top[0] + fx * top[3];
        val = (256 - fy) * lo + fy * hi;
        d[0] = (UC8)(val >> 16);

        // interpolate green pixel
        lo  = (256 - fx) * bot[1] + fx * bot[4];
        hi  = (256 - fx) * top[1] + fx * top[4];
        val = (256 - fy) * lo + fy * hi;
        d[1] = (UC8)(val >> 16);

        // interpolate red pixel
        lo  = (256 - fx) * bot[2] + fx * bot[5];
        hi  = (256 - fx) * top[2] + fx * top[5];
        val = (256 - fy) * lo + fy * hi;
        d[2] = (UC8)(val >> 16);
      }
}


///////////////////////////////////////////////////////////////////////////
//                             Standard Variants                         //
///////////////////////////////////////////////////////////////////////////

//= Make output pixels identical to input pixels.

void jhcWarp::Identity ()
{
  double xsc = sw / (double) dw, ysc = sh / (double) dh;
  int x, y;

  for (y = 0; y < dh; y++)
    for (x = 0; x < dw; x++)
      SetWarp(x, y, x * xsc, y * ysc);
}  


//= Progressive zoom for more resolution around image center.
// falloff: dp/da = 1 / (k * a + 1) with k = 0.4251 for a in degs
// integral: p = (1/m) * log(k * a + 1) maps src angle to dest pixel
// inverted: a = (exp(m * p) - 1) / k maps dest pixel to src angle

void jhcWarp::LogZoom (double xc, double yc, double hfov)
{
  double kh = 0.4251, kv = kh / 0.75;                         // human log falloff coefs
  double khr = kh * R2D, kvr = kv * R2D;
  double ikh = D2R / kh, ikv = D2R / kv;
  double hhr = 0.5 * D2R * hfov, hvr = hhr * sh / sw;         // half H and V FOV angles (rads)
  double sx0 = 0.5 * (sw - 1), sy0 = 0.5 * (sh - 1);          // middle of src and dest (pels)
  double f = sx0 / tan(hhr);                                  // focal length of src (pels)
  double p0 = atan2((xc - 0.5) * (sw - 1), f);                // gaze point (rads)
  double t0 = atan2((yc - 0.5) * (sh - 1), f);         
  double pmin = fabs(hhr - p0), pmax = fabs(hhr + p0);        // max angles wrt gaze (rads)
  double tmin = fabs(hvr - t0), tmax = fabs(hvr + t0);   
  double xmin = -log(khr * pmin + 1.0);                       // max dest coords (pels)
  double xmax = log(khr * pmax + 1.0);  
  double ymin = -log(kvr * tmin + 1.0);
  double ymax = log(kvr * tmax + 1.0);
  double mx = (xmax - xmin) / dw, my = (ymax - ymin) / dh;    // dest scaling factors
  double dx0, dy0, sxa, sya, sx, sy; 
  int x, y;
/*
  // harmonized scaling factor
//  mx = __max(mx, my);
  mx = __min(mx, my);
  my = mx;
*/
  // destination centering
  dx0 = 0.5 * ((xmax + xmin) / mx + (dw - 1));         
  dy0 = 0.5 * ((ymax + ymin) / my + (dh - 1));

  // find source pixel for each dest pixel
  for (y = 0; y < dh; y++)
    for (x = 0; x < dw; x++)
    {
      // get sample angles (in rads) based on output pixel
      sxa = ikh * (exp(mx * fabs(x - dx0)) - 1.0);
      if (x < dx0)
        sxa = -sxa;
      sya = ikv * (exp(my * fabs(y - dy0)) - 1.0);
      if (y < dy0)
        sya = -sya;

      // convert sample angles to input pixel location
      sx = sx0 + f * tan(p0 + sxa);
      sy = sy0 + f * tan(t0 + sya);
      SetWarp(x, y, sx, sy);
    }
}


//= Rotate image by some amount around center of source.

void jhcWarp::Rotate (double degs)
{
  double xsc = sw / (double) dw, ysc = sh / (double) dh;
  double sx0 = 0.5 * (sw + 1), sy0 = 0.5 * (sh + 1); 
  double dx0 = 0.5 * (dw + 1), dy0 = 0.5 * (dh + 1); 
  double dx, dy, sx, sy, r = D2R * degs, c = cos(r), s = sin(r);
  int x, y;

  for (y = 0; y < dh; y++)
    for (x = 0; x < dw; x++)
    {
      dx = xsc * (x - dx0);
      dy = ysc * (y - dy0);
      sx = sx0 + c * dx + s * dy;
      sy = sy0 - s * dx + c * dy;
      SetWarp(x, y, sx, sy);
    }
}


//= Remove barrel distortion of lens given transform coefficients.
// coefs are wrt src which is 4:3 in true FOV aspect (not pels)
// assumes optical center is center of image

void jhcWarp::Flatten (double r2f, double r4f, double mag)
{
  double f2 = 1e-6 * r2f, f4 = 1e-12 * r4f;
  double dx0 = 0.5 * (dw - 1), dy0 = 0.5 * (dh - 1);
  double sx0 = 0.5 * (sw - 1), sy0 = 0.5 * (sh - 1);
  double xsc = sw / (dw * mag), ysc = xsc * 0.75 * sw / sh;
  double dx, dy, r2, r4, warp, wx, wy;
  int x, y;

  for (y = 0; y < dh; y++)
  {
    // get central offset adjusted for pixel aspect ratio
    dy = ysc * (y - dy0);
    for (x = 0; x < dw; x++)
    {
      // compute radial offset from center
      dx = xsc * (x - dx0);
      r2 = dx * dx + dy * dy;
      r4 = r2 * r2;

      // determine lens warped coordinates
      warp = 1.0 + f2 * r2 + f4 * r4;
      wx = sx0 + warp * dx;
      wy = sy0 + warp * dy;
      SetWarp(x, y, wx, wy);          
    }
  }
}


//= Remove lens distortion and de-rotate to vertical.
// similar to Flatten but no 4:3 assumption and adds rotation
// assumes optical center is center of image

void jhcWarp::Rectify (double r2f, double r4f, double mag, double degs)
{
  double f2 = 1e-6 * r2f, f4 = 1e-12 * r4f;
  double dx0 = 0.5 * (dw - 1), dy0 = 0.5 * (dh - 1);
  double sx0 = 0.5 * (sw - 1), sy0 = 0.5 * (sh - 1);
  double xsc = sw / (dw * mag), ysc = sh / (dh * mag);
  double r = D2R * degs, c = cos(r), s = sin(r);
  double dx, dy, r2, r4, warp, wx, wy, sx, sy;
  int x, y;

  for (y = 0; y < dh; y++)
  {
    // get central offset adjusted for pixel aspect ratio
    dy = ysc * (y - dy0);
    for (x = 0; x < dw; x++)
    {
      // compute radial offset from center
      dx = xsc * (x - dx0);
      r2 = dx * dx + dy * dy;
      r4 = r2 * r2;

      // determine lens warped coordinates
      warp = 1.0 + f2 * r2 + f4 * r4;
      wx = warp * dx;
      wy = warp * dy;

      // rotate sampling position
      sx = sx0 + c * wx + s * wy;
      sy = sy0 - s * wx + c * wy;
      SetWarp(x, y, sx, sy);          
    }
  }
}
