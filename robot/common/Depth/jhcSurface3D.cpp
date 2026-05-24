// jhcSurface3D.cpp : interprets scene relative to a flat plane
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2020 IBM Corporation
// Copyright 2020-2025 Etaoin Systems
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

#include "Interface/jhcMessage.h"   // common vision

#include "Depth/jhcSurface3D.h"     // common robot


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcSurface3D::jhcSurface3D ()
{
  s2p.SetSize(4, 4);
  s2f.SetSize(4, 4);
  p2v.SetSize(4, 4);
  SetSize();
  SetCamera();
  SetView();
  SetFix();
  SetOptics();
  SetProject();
  choke = 1760;
}


//= Set sizes of internal images based on a reference image.

void jhcSurface3D::SetSize (const jhcImg& ref, int full)
{
  SetSize(ref.XDim(), ref.YDim(), full);
}


//= Set sizes of internal images directly.

void jhcSurface3D::SetSize (int x, int y, int full, double f, double sc)
{
  // save input size
  iw = x;
  ih = y;

  // make internal image for caching values
  hw = ((full > 0) ? x : x / 2);
  hh = ((full > 0) ? y : y / 2);
  wxyz.SetSize(hw, hh, 6);

  // possibly set optical parameters of sensor (for convenience)
  if (f > 0.0)
    SetOptics(f, sc);
}


///////////////////////////////////////////////////////////////////////////
//                         Local Plane Fitting                           //
///////////////////////////////////////////////////////////////////////////

//= Figure out camera parameters by fitting a horizontal plane to overhead height map.
// origin is at x = w/2 and y = yoff/ipp, generally (0 0) is middle of bottom
// input mapping: 0 = unknown, 1 = lower, 2 = zlo, 254 = zhi, 255 = higher
// only considers planar fit for points within +/- ztol of z0 
// often desirable to mask input depth image to only cover known flat area
// returns standard deviation of points in estimate to found plane (in inches)

double jhcSurface3D::CamCalib (double& t, double& r, double& h, const jhcImg& src, double z0, double ztol, 
                               double zlo, double zhi, double ipp, double yoff, const jhcRoi *area) 
{
  const jhcRoi *samp = ((area != NULL) ? area : &src);
  double cxm = cx + 0.5 * ipp * src.XDim(), cym = cy + yoff;
  double rads = D2R * (p0 + 90.0), cp = cos(rads), sp = sin(rads);
  double a, b, c, tz, ipz = (zhi - zlo) / 252.0, sc = 1.0 / ipz;
  int x0 = samp->RoiX(), x2 = samp->RoiX2(), y0 = samp->RoiY(), y2 = samp->RoiY2();
  int lo, hi, x, y, sk = src.RoiSkip(*samp);
  const UC8 *s = src.RoiSrc(*samp);

  if (!src.Valid(1))
    return Fatal("Bad images to jhcSurface3D::Calibrate");

  // compute height range for pixels to use in statistics
  lo = ROUND(sc * (z0 - ztol - zlo) + 2.0);
  hi = ROUND(sc * (z0 + ztol - zlo) + 2.0);
  lo = __max(2, __min(lo, 254));
  hi = __max(2, __min(hi, 254));

  // gather statistics using pixels close to expected surface
  ClrStats();
  for (y = y0; y < y2; y++, s += sk)
    for (x = x0; x < x2; x++, s++)
      if ((*s >= lo) && (*s <= hi))
        AddPoint(x, y, *s);

  // do actual plane fitting (a, b, and c in inches)
  Analyze(ipp, ipp, ipz);
  a = CoefX();
  b = CoefY();
  c = Offset();

  // figure orthogonal distance of camera location to plane
  tz = a * cxm + b * cym + c;
  h = z0 - (tz + zlo);

  // resolve surface normal wrt pan to get tilt and roll angles
  t = -R2D * atan(a * cp + b * sp);
  r = R2D * atan(-a * sp + b * cp);
  return RMS();
}


//= Does a least square fit of points along an image line to give a tilt angle.
// line start at pixel (x0 y0) and goes to (x1 y1), angle is atan(dz/dr) in degs
// can optionally trim line to start at f0 fraction of length and stop at f1
// primarily used to estimate angle of ELI robot forearm for calibration
// returns 360.0 if problem with estimate (normally -180 to +180)

double jhcSurface3D::LineTilt (const jhcImg& d16, double x0, double y0, double x1, double y1, double f0, double f1) const
{
  double dx = x1 - x0, dy = y1 - y0, len = sqrt(dx * dx + dy * dy);
  double ix = x0 + f0 * dx, iy = y0 + f0 * dy, ir = f0 * len, stop = f1 * len;
  double x, y, z, xref, yref, r, num, den, xstep = dx / len, ystep = dy / len; 
  double sr = 0.0, sz = 0.0, sr2 = 0.0, srz = 0.0; 
  int iz, n = 0;

  if (!d16.Valid(2))
    return Fatal("Bad images to jhcSurface3D::LineTilt");

  // get raw depth for all points on image line
  while (ir < stop)
  {
    iz = d16.ARefChk16(ROUND(ix), ROUND(iy), 0);
    if ((iz >= choke) && (iz <= 40000))
    {
      // find actual 3D coordinates and get planar distance from start
      WorldPt(x, y, z, ix, iy, iz);
      if (n <= 0)
      {
        xref = x;
        yref = y;
      }
      x -= xref;
      y -= yref;
      r = sqrt(x * x + y * y);

      // add to statistics
      sr  += r; 
      sz  += z;
      sr2 += r * r;
      srz += r * z;
      n++;
    }

    // advance to next sampling location
    ix += xstep;
    iy += ystep;
    ir += 1.0;
  }

  // compute angle of slope
  if (n < 2)
    return 360.0;
  num = n * srz - sr * sz;
  den = n * sr2 - sr * sr;
  if (den == 0.0)
    return 360.0;
  return R2D * atan(num / den);
}


///////////////////////////////////////////////////////////////////////////
//                         Standard Overhead Map                         //
///////////////////////////////////////////////////////////////////////////

//= Set up basic coordinate transform matrix for depth sensor pose to overhead map.
// NOTE: "s2p" results in 32768-centered 0.02" increments, not physical inches

void jhcSurface3D::RngToMap (double cpan, double ctilt, double croll, 
                             double x0, double y0, double z0, 
                             double pcal, double tcal, double rcal)
{
  double dsc = ksc / 101.6, finv = 2.0 * dsc / kf;    // 101.6 = 4 * 25.4 mm/in

  // check if cached image is full sized (not half)
  if (hw == iw)
    finv *= 0.5;

  // convert image coordinates to real distances (in inches)
  s2p.Magnification(finv, finv, -dsc);          

  // adjust for camera mounting errors
  s2p.RotateZ(rcal);
  s2p.RotateY(pcal);
  s2p.RotateX(tcal);

  // build up coordinate transform for surface
  s2p.RotateZ(croll);                  // view along -z axis
  s2p.RotateX(ctilt);                  // view along y axis
  s2p.RotateZ(cpan);
  s2p.Translate(x0, y0, z0);

  // change everything to inches x 50 and make zero be 32768
  s2p.Magnify(50.0);
  s2p.Translate(32768.0, 32768.0, 32768.0);
}


//= Set up basic coordinate transform matrix for overhead map to color camera pose.
// makes up z coordinate for each pixel which is in 0.25mm steps away from camera
// NOTE: "p2v" takes 32768-centered 0.02" increments, not physical inches

void jhcSurface3D::MapToCol (double cpan, double ctilt, double croll, 
                             double x0, double y0, double z0, double f, double sc,
                             double pcal, double tcal, double rcal)
{
  double psc = 101.6 / sc, fsc = (f * psc) / 2.0;          // 101.6 = 4 * 25.4 mm/in    

  // check if cached image is full sized (not half)
  if (hw == iw)
    fsc *= 2.0;

  // remove 32768 offset and change everything back to inches
  p2v.Translation(-32768.0, -32768.0, -32768.0);
  p2v.Magnify(0.02);

  // normalize positions for camera orientation
  p2v.Translate(-x0, -y0, -z0);
  p2v.RotateZ(-cpan);                  // view along y axis
  p2v.RotateX(-ctilt);                 // view along -z axis
  p2v.RotateZ(-croll);

  // adjust for camera mounting errors
  p2v.RotateX(-tcal);
  p2v.RotateY(-pcal);
  p2v.RotateZ(-rcal);

  // convert to depth sensor pixels and 0.25 mm steps
  p2v.Magnify(fsc, fsc, -psc);
}


//= Plot depth image as vacuform surface and cache xyz values for later use.
// generally need to call SetMap first to describe destination image

int jhcSurface3D::FloorMap0 (jhcImg& dest, const jhcImg& d16, int clr, 
                             double pan, double tilt, double roll, double xcam, double ycam, double zcam,
                             double pcal, double tcal, double rcal)
{
  SetCamera(xcam, ycam, zcam);
  CacheXYZ(d16, pan, tilt, roll);
  return Plane(dest, ipp, 0.0, z0, z1 - z0, zmax, 1);
}


//= Plot depth image as vacuform surface using given parameters.
// pixel values: 0 = less than z0, 1 = z0, 254 = z1, 255 = z1 to zmax
// usually want to call jhcLut::Replace(dest, 255, 0) on final map
// takes about 5.3ms at full VGA, about 1.3ms at SIF (full = 0) 
// about 20% faster than calling CacheXYZ then Plane
// generally need to call SetMap first to describe destination image
// Note: does NOT do CacheXYZ so functions like MapBack and Ground will not work

int jhcSurface3D::FloorMap (jhcImg& dest, const jhcImg& d16, int clr,
                            double pan, double tilt, double roll, 
                            double xcam, double ycam, double zcam,
                            double pcal, double tcal, double rcal)
{
  double a0, b0, c0, d0, a1, b1, c1, d1, a2, b2, c2, d2, abc0, abc1, abc2;
  int zlim = __min(ROUND(dmax * 101.6 / ksc), 40000);  // max = 10m (32.8')    
  int zcut = (int)(1.0 + 253.0 * (zmax - z0) / (z1 - z0));
  int dw = dest.XDim(), dh = dest.YDim(), ln2 = d16.Line();
  int x, y, ix, iy, iz, zstep = ((hw == iw) ? 1 : 2), zln = ((hw == iw) ? ln2 >> 1 : ln2);
  const US16 *z, *zrow = (const US16 *) d16.PxlSrc();
  UC8 *v;

  // check for proper sized input and clear output map
  if (!dest.Valid(1) || !d16.SameFormat(iw, ih, 2))
    return Fatal("Bad images to jhcSurface3D::FloorMap");
  if (clr > 0)
    dest.FillArr(0);

  // figure out transform (enables coordinate mapping functions)
  // 0 degs is camera horizontal
  RngToMap(pan, tilt + 90.0, roll, xcam, ycam, zcam, pcal, tcal, rcal);           

  // add in conversion to overhead map with x = 0 in middle and z0 -> 1
  s2f.Copy(s2p);
  s2f.Translate(-32768.0, -32768.0, -32768.0 - 50.0 * z0);
  s2f.Magnify(0.02 / ipp, 0.02 / ipp, 0.02 * 253.0 / (z1 - z0));
  s2f.Translate(0.5 * dest.XDim(), 0.0, 1.0);

  // extract factors: ix = (a0 * x + b0 * y + c0) * z + d0
  a0 = s2f.MRef(0, 0);
  b0 = s2f.MRef(1, 0);
  c0 = s2f.MRef(2, 0) - 0.5 * (a0 * (hw - 1) + b0 * (hh - 1));
  d0 = s2f.MRef(3, 0);

  // extract factors: iy = (a1 * x + b1 * y + c1) * z + d1
  a1 = s2f.MRef(0, 1);
  b1 = s2f.MRef(1, 1);
  c1 = s2f.MRef(2, 1) - 0.5 * (a1 * (hw - 1) + b1 * (hh - 1));
  d1 = s2f.MRef(3, 1);

  // extract factors: iz = (a2 * x + b2 * y + c2) * z + d2
  a2 = s2f.MRef(0, 2);
  b2 = s2f.MRef(1, 2);
  c2 = s2f.MRef(2, 2) - 0.5 * (a2 * (hw - 1) + b2 * (hh - 1));
  d2 = s2f.MRef(3, 2);

  // apply coordinate transform to depth pixels
  for (y = 0; y < hh; y++, zrow += zln)
  {
    // compute beginning values for row
    z = zrow;
    abc0 = b0 * y + c0;
    abc1 = b1 * y + c1;
    abc2 = b2 * y + c2;

    // remap pixels in a row of the depth image
    for (x = 0; x < hw; x++, z += zstep, abc0 += a0, abc1 += a1, abc2 += a2)
      if ((*z >= choke) && (*z <= zlim))
      {
        // find map location for pixel
        ix = (int)(abc0 * (*z) + d0);
        iy = (int)(abc1 * (*z) + d1);
        iz = (int)(abc2 * (*z) + d2);

        // convert z to appropriate pixel value
        if ((iz > 0) && (iz < zcut))
          if ((ix >= 0) && (ix < dw) && (iy >= 0) && (iy < dh))
          {
            // only change map pixel if height is greater than map
            iz = __min(iz, 255);       
            v = dest.APtr(ix, iy); 
            if (iz > *v)
              *v = (UC8) iz;                     // iz always positive
            else if (*v == 0)
              *v = 1;                            // clamp negative iz 
          }
      }
  }
  return 1;
}


//= Plot depth image as vacuform surface compensating for depth granularity.
// tries to mark every n'th map cell when long range reading encountered
// generally need to call SetMap first to describe destination image
// Note: does NOT do CacheXYZ so functions like MapBack and Ground will not work

int jhcSurface3D::FloorMap2 (jhcImg& dest, const jhcImg& d16, int clr,
                             double pan, double tilt, double roll, 
                             double xcam, double ycam, double zcam, int n,
                             double pcal, double tcal, double rcal)
{
  double a0, b0, c0, d0, a1, b1, c1, d1, a2, b2, c2, d2, abc0, abc1, abc2;
  double sc = 7.1e-7, sc2 = 0.5 * sc, grid = 101.6 * ipp, gr2 = 0.5 * grid + 0.5;
  int dev, top, alt, gstep = ROUND(n * grid), rth = ROUND(sqrt((n + 1) * grid / sc));
  int zlim = __min(ROUND(dmax * 101.6 / ksc), 40000);  // max = 10m (32.8')
  int zcut = (int)(1.0 + 253.0 * (zmax - z0) / (z1 - z0));
  int dw = dest.XDim(), dh = dest.YDim(), ln2 = d16.Line();
  int x, y, ix, iy, iz, zstep = ((hw == iw) ? 1 : 2), zln = ((hw == iw) ? ln2 >> 1 : ln2);
  const US16 *z, *zrow = (const US16 *) d16.PxlSrc();
  UC8 *v;

  // check for proper sized input and clear output map
  if (!dest.Valid(1) || !d16.SameFormat(iw, ih, 2))
    return Fatal("Bad images to jhcSurface3D::FloorMap2");
  if (n <= 0)
    return FloorMap(dest, d16, clr, pan, tilt, roll, xcam, ycam, zcam, pcal, tcal, rcal);           
  if (clr > 0)
    dest.FillArr(0);

  // figure out transform (enables coordinate mapping functions)
  // 0 degs is camera horizontal
  RngToMap(pan, tilt + 90.0, roll, xcam, ycam, zcam, pcal, tcal, rcal);                     

  // add in conversion to overhead map with x = 0 in middle and z0 -> 1
  s2f.Copy(s2p);
  s2f.Translate(-32768.0, -32768.0, -32768.0 - 50.0 * z0);
  s2f.Magnify(0.02 / ipp, 0.02 / ipp, 0.02 * 253.0 / (z1 - z0));
  s2f.Translate(0.5 * dest.XDim(), 0.0, 1.0);

  // extract factors: ix = (a0 * x + b0 * y + c0) * z + d0
  a0 = s2f.MRef(0, 0);
  b0 = s2f.MRef(1, 0);
  c0 = s2f.MRef(2, 0) - 0.5 * (a0 * (hw - 1) + b0 * (hh - 1));
  d0 = s2f.MRef(3, 0);

  // extract factors: iy = (a1 * x + b1 * y + c1) * z + d1
  a1 = s2f.MRef(0, 1);
  b1 = s2f.MRef(1, 1);
  c1 = s2f.MRef(2, 1) - 0.5 * (a1 * (hw - 1) + b1 * (hh - 1));
  d1 = s2f.MRef(3, 1);

  // extract factors: iz = (a2 * x + b2 * y + c2) * z + d2
  a2 = s2f.MRef(0, 2);
  b2 = s2f.MRef(1, 2);
  c2 = s2f.MRef(2, 2) - 0.5 * (a2 * (hw - 1) + b2 * (hh - 1));
  d2 = s2f.MRef(3, 2);

  // apply coordinate transform to depth pixels
  for (y = 0; y < hh; y++, zrow += zln)
  {
    // compute beginning values for row
    z = zrow;
    abc0 = b0 * y + c0;
    abc1 = b1 * y + c1;
    abc2 = b2 * y + c2;

    // remap pixels in a row of the depth image
    for (x = 0; x < hw; x++, z += zstep, abc0 += a0, abc1 += a1, abc2 += a2)
      if ((*z >= choke) && (*z <= zlim))
      {
        // STREAK - determine multi-fill range around nominal value
        dev = 0;
        if (*z >= rth)
          dev = (int)(sc2 * ((*z) * (*z)) - gr2);  
        top = *z + dev + 1;

        for (alt = *z - dev; alt < top; alt += gstep)
        {
          // find map location for pixel
          ix = (int)(abc0 * alt + d0);
          iy = (int)(abc1 * alt + d1);
          iz = (int)(abc2 * alt + d2);

          // convert z to appropriate pixel value
          if ((iz > 0) && (iz < zcut))
            if ((ix >= 0) && (ix < dw) && (iy >= 0) && (iy < dh))
            {
              // only change map pixel if height is greater than map
              iz = __min(iz, 255);       
              v = dest.APtr(ix, iy); 
              if (iz > *v)
                *v = (UC8) iz;                   // iz always positive
              else if (*v == 0)
                *v = 1;                          // clamp negative iz
            }
         }
       }
  }
  return 1;
}


//= Plot camera depth and color as height from some plane and color on that plane.
// generally need to call SetMap first to describe destination image
// right limbs of objects tend to bleed onto edge of surface shadow (Kinect 360)
// Note: does NOT do CacheXYZ so functions like MapBack and Ground will not work

int jhcSurface3D::FloorColor (jhcImg& rgb, jhcImg& hts, const jhcImg& col, const jhcImg& d16, int clr,
                              double pan, double tilt, double roll, double xcam, double ycam, double zcam,
                              double pcal, double tcal, double rcal)
{
  double a0, b0, c0, d0, a1, b1, c1, d1, a2, b2, c2, d2, abc0, abc1, abc2;
  int zlim = __min(ROUND(dmax * 101.6 / ksc), 40000);                          // max = 10m (32.8')    
  int zcut = (int)(1.0 + 253.0 * (zmax - z0) / (z1 - z0));
  int dw = hts.XDim(), dh = hts.YDim(), ln2 = d16.Line(), ln3 = col.Line();
  int zstep = ((hw == iw) ? 1 : 2), zln = ((hw == iw) ? ln2 >> 1 : ln2);
  int sstep = ((hw == iw) ? 3 : 6), sln = ((hw == iw) ? ln3: ln3 << 1);
  int x, y, ix, iy, iz;
  const US16 *z, *zrow = (const US16 *) d16.PxlSrc();
  const UC8 *s, *srow = col.PxlSrc();
  UC8 *h;

  // check for proper sized input and clear output images
  if (!rgb.Valid(3) || !rgb.SameSize(hts, 1) || !col.SameFormat(iw, ih, 3) || !d16.SameFormat(iw, ih, 2))
    return Fatal("Bad images to jhcSurface3D::FloorColor");
  if (clr > 0)
  {
    rgb.FillArr(0);
    hts.FillArr(0);
  }

  // figure out transform (enables coordinate mapping functions)
  // 0 degs is camera horizontal
  RngToMap(pan, tilt + 90.0, roll, xcam, ycam, zcam, pcal, tcal, rcal);           

  // add in conversion to overhead map with x = 0 in middle and z0 -> 1
  s2f.Copy(s2p);
  s2f.Translate(-32768.0, -32768.0, -32768.0 - 50.0 * z0);
  s2f.Magnify(0.02 / ipp, 0.02 / ipp, 0.02 * 253.0 / (z1 - z0));
  s2f.Translate(0.5 * hts.XDim(), 0.0, 1.0);

  // extract factors: ix = (a0 * x + b0 * y + c0) * z + d0
  a0 = s2f.MRef(0, 0);
  b0 = s2f.MRef(1, 0);
  c0 = s2f.MRef(2, 0) - 0.5 * (a0 * (hw - 1) + b0 * (hh - 1));
  d0 = s2f.MRef(3, 0);

  // extract factors: iy = (a1 * x + b1 * y + c1) * z + d1
  a1 = s2f.MRef(0, 1);
  b1 = s2f.MRef(1, 1);
  c1 = s2f.MRef(2, 1) - 0.5 * (a1 * (hw - 1) + b1 * (hh - 1));
  d1 = s2f.MRef(3, 1);

  // extract factors: iz = (a2 * x + b2 * y + c2) * z + d2
  a2 = s2f.MRef(0, 2);
  b2 = s2f.MRef(1, 2);
  c2 = s2f.MRef(2, 2) - 0.5 * (a2 * (hw - 1) + b2 * (hh - 1));
  d2 = s2f.MRef(3, 2);

  // apply coordinate transform to depth pixels
  for (y = 0; y < hh; y++, srow += sln, zrow += zln)
  {
    // compute beginning values for row
    s = srow;
    z = zrow;
    abc0 = b0 * y + c0;
    abc1 = b1 * y + c1;
    abc2 = b2 * y + c2;

    // remap pixels in a row of the depth image
    for (x = 0; x < hw; x++, s += sstep, z += zstep, abc0 += a0, abc1 += a1, abc2 += a2)
      if ((*z >= choke) && (*z <= zlim))
      {
        // find map location for pixel and check if in valid region
        iz = (int)(abc2 * (*z) + d2);
        if (iz >= zcut)                             // need iz < 0 sometimes
          continue;
        iy = (int)(abc1 * (*z) + d1);
        if ((iy < 0) || (iy >= dh))
          continue;
        ix = (int)(abc0 * (*z) + d0);
        if ((ix < 0) || (ix >= dw))
          continue;

        // only change output pixels only if height is greater than map
        iz = __min(iz, 255);       
        h = hts.APtr(ix, iy); 
        if (iz > *h)
        {
          *h = (UC8) iz;                           // iz always positive
          rgb.ASetCol(ix, iy, s[2], s[1], s[0]);   // copy color of top
        }
        else if (*h == 0)
        {
          *h = 1;                                  // clamp negative iz 
          rgb.ASetCol(ix, iy, s[2], s[1], s[0]);   // sample color of ditch
        }
      }
  }
  return 1;
}


//= Return real world height for map pixel of given value.
// cannot truly be sure for values outside [1 254]

double jhcSurface3D::FloorHt (int pixel) 
{
  if (pixel <= 0)
    return 0.0;
  if (pixel >= 255)
    return zmax;
  return(z0 + (pixel - 1) * (z1 - z0) / 253.0);
}


//= Determine pixel value corresponding to a particular real world height.
// values of 0 and 255 are largely meaningless

int jhcSurface3D::FloorPel (double ht)
{
  if (ht < z0)
    return 0;
  if (ht >= zmax)
    return 255;
  return (int)(1.0 + 253.0 * (ht - z0) / (z1 - z0));
}


///////////////////////////////////////////////////////////////////////////
//                           Height Analysis                             //
///////////////////////////////////////////////////////////////////////////

//= Change depth map into world coordinate of points given camera parameters.
// only compute coordinates for points less than dmax away (inches) 
// saves results in "wxyz" images = 2 bytes X + 2 bytes Y + 2 bytes Z
// all coordinates stored as 32768 + 50 * inches (to accomodate negatives)
// invalid pixels have x, y, and z set to zero (outside normal range)
// limited to about 54 feet in each direction relative to system origin
// takes about 3.2ms for SIF on 3.2GHz Pentium

int jhcSurface3D::CacheXYZ (const jhcImg& d16, double cpan, double ctilt, double croll, double dmax,
                            double pcal, double tcal, double rcal)
{
  int zlim = __min(ROUND(dmax * 101.6 / ksc), 40000);    // 101.6 = 4 * 25.4, max = 10m (32.8')
  int x, y, sk = wxyz.Skip() >> 1, ln2 = d16.Line();
  int zstep = ((hw == iw) ? 1 : 2), zln = ((hw == iw) ? ln2 >> 1 : ln2);
  double a0, b0, c0, d0, a1, b1, c1, d1, a2, b2, c2, d2, abc0, abc1, abc2;
  const US16 *z, *zrow = (US16 *) d16.PxlSrc();
  US16 *dxyz = (US16 *) wxyz.PxlDest();

  if (!d16.SameFormat(iw, ih, 2))
    return Fatal("Bad images to jhcSurface3D::CacheXYZ");

  // figure out mapping to use (no centering or perspective)
  RngToMap(cpan, ctilt + 90.0, croll, cx, cy, cz, pcal, tcal, rcal);   // 0 degs is camera horizontal

  // extract factors: ix = (a0 * x + b0 * y + c0) * z + d0
  a0 = s2p.MRef(0, 0);
  b0 = s2p.MRef(1, 0);
  c0 = s2p.MRef(2, 0) - 0.5 * (a0 * (hw - 1) + b0 * (hh - 1));
  d0 = s2p.MRef(3, 0);

  // extract factors: iy = (a1 * x + b1 * y + c1) * z + d1
  a1 = s2p.MRef(0, 1);
  b1 = s2p.MRef(1, 1);
  c1 = s2p.MRef(2, 1) - 0.5 * (a1 * (hw - 1) + b1 * (hh - 1));
  d1 = s2p.MRef(3, 1);

  // extract factors: iz = (a2 * x + b2 * y + c2) * z + d2
  a2 = s2p.MRef(0, 2);
  b2 = s2p.MRef(1, 2);
  c2 = s2p.MRef(2, 2) - 0.5 * (a2 * (hw - 1) + b2 * (hh - 1));
  d2 = s2p.MRef(3, 2);

  // apply coordinate transform to depth pixels
  for (y = 0; y < hh; y++, dxyz += sk, zrow += zln)
  {
    // compute beginning values for row
    z = zrow;
    abc0 = b0 * y + c0;
    abc1 = b1 * y + c1;
    abc2 = b2 * y + c2;

    // remap pixels in a row of the depth image
    for (x = 0; x < hw; x++, dxyz += 3, z += zstep, abc0 += a0, abc1 += a1, abc2 += a2)
      if ((*z >= choke) && (*z <= zlim))
      {
        // find world location for pixel
        dxyz[0] = (US16)(abc0 * (*z) + d0);
        dxyz[1] = (US16)(abc1 * (*z) + d1);
        dxyz[2] = (US16)(abc2 * (*z) + d2);
      }
      else
      {
        // invalid pixel
        dxyz[0] = 0;
        dxyz[1] = 0;
        dxyz[2] = 0;
      }
  }
  return 1;
}


//= Map image points onto surface found previously to give overhead view.
// always centers the origin in x direction of image and offset yoff inches forward
// must call CacheXYZ function first to fill internal "wxyz" image
// pixel = 128 + 127 * z / zrng, if pos > 0 then pixel = 255 * z / zrng instead
// 0 if invalid, 1 if very low (but >= zoff), 2-254 = valid, 255 if very high (but <= zmax)
// takes about 1.6ms on a 3.2GHz Pentium (for 144x180"/0.3)

int jhcSurface3D::Plane (jhcImg& dest, double ipp, double yoff, 
                         double zoff, double zrng, double zmax, int pos)
{
  int x, y, fx, fy, fz, w = dest.XDim(), h = dest.YDim(), psk = wxyz.Skip() >> 1; 
  int m = ROUND(4096.0 * 0.02 / ipp), x0 = ROUND(4096.0 * 0.5 * w), y0 = ROUND(4096.0 * yoff / ipp);
  int s = ROUND(4096.0 * 127.0 * 0.02 / zrng), z0 = 32768 + ROUND(zoff / 0.02);
  int off = 128, zhi = ROUND(50.0 * zmax + 32768.0);
  const US16 *pt = (US16 *) wxyz.PxlSrc();
  UC8 *d;

  if (!dest.Valid(1))
    return Fatal("Bad images to jhcSurface3D::Plane");
  dest.FillArr(0);
  if (pos > 0)
  {
    // positive only deviations from ground plane
    s = ROUND(4096.0 * 255.0 * 0.02 / zrng);
    off = 0;
  }
  for (y = 0; y < hh; y++, pt += psk)
    for (x = 0; x < hw; x++, pt += 3)
      if ((pt[2] > z0) && (pt[2] <= zhi))
      {
        // find floor position of pixel
        fx = (m * (pt[0] - 32768) + x0) >> 12;
        if ((fx < 0) || (fx >= w))
          continue;
        fy = (m * (pt[1] - 32768) + y0) >> 12;
        if ((fy < 0) || (fy >= h))
          continue;

        // find height value for pixel
        fz = ((s * (pt[2] - z0)) >> 12) + off;
        fz = __max(1, __min(fz, 255));

        // overwrite existing pixel if new value higher
        d = dest.APtr(fx, fy, 0);
        if (fz > *d)
          *d = (UC8) fz;
      }
  return 1;
}


//= Map image points in given height range onto presumed ground plane.
// always centers the origin in x direction of image and offset yoff inches forward
// must call CacheXYZ function first to fill internal "wxyz" image
// pixel = sum of inc for each hit in range, 0 otherwise
// takes about 0.3ms on a 3.2GHz Pentium (for 36-48" into 72x72"/0.3)

int jhcSurface3D::Slice (jhcImg& dest, double z0, double z1, double ipp, double yoff, int inc)
{
  int x, y, fx, fy, v, w = dest.XDim(), h = dest.YDim(), psk = wxyz.Skip() >> 1; 
  int m = ROUND(4096.0 * 0.02 / ipp);
  int x0 = ROUND(4096.0 * 0.5 * w), y0 = ROUND(4096.0 * yoff / ipp);
  int zlo = ROUND(50.0 * z0 + 32768.0), zhi = ROUND(50.0 * z1 + 32768.0);
  const US16 *pt = (US16 *) wxyz.PxlSrc();
  UC8 *d;

  if (!dest.Valid(1))
    return Fatal("Bad images to jhcSurface3D::Slice");
  dest.FillArr(0);
  zlo = __max(1, zlo);

  for (y = 0; y < hh; y++, pt += psk)
    for (x = 0; x < hw; x++, pt += 3)
      if ((pt[2] >= zlo) && (pt[2] <= zhi))
      {
        // find floor position of pixel
        fx = (m * (pt[0] - 32768) + x0) >> 12;
        if ((fx < 0) || (fx >= w))
          continue;
        fy = (m * (pt[1] - 32768) + y0) >> 12;
        if ((fy < 0) || (fy >= h))
          continue;

        // mark floor cell
        d = dest.APtr(fx, fy, 0);
        v = *d + inc;
        *d = (UC8) __min(v, 255);
//        dest.ASet(fx, fy, 0, mark);
      }
  return 1;
}


//= Create an image similar to input to show where ground plane features come from.
// assumes input centered in x direction of image and offset yoff inches forward
// takes a half-sized (SIF) destination image, input images has ipp inches per pixel
// only copies src pixels in z0 to z1 range (rest are left as "fill" value)
// must call CacheXYZ function first to fill internal "wxyz" image

int jhcSurface3D::MapBack (jhcImg& dest, const jhcImg& src, double z0, double z1, 
                           double ipp, double yoff, int fill)
{
  int x, y, fx, fy, w = src.XDim(), h = src.YDim();
  int  dsk = dest.Skip(), psk = wxyz.Skip() >> 1, m = ROUND(4096.0 * 0.02 / ipp);
  int x0 = ROUND(4096.0 * 0.5 * w), y0 = ROUND(4096.0 * yoff / ipp);
  int zlo = ROUND(50.0 * z0 + 32768.0), zhi = ROUND(50.0 * z1 + 32768.0);
  const US16 *pt = (US16 *) wxyz.PxlSrc();
  UC8 *d = dest.PxlDest();

  if (!dest.SameFormat(hw, hh, 1) || !src.Valid(1))
    return Fatal("Bad images to jhcSurface3D::MapBack");
  if (fill >= 0)
    dest.FillArr(fill);
  zlo = __max(1, zlo);

  for (y = 0; y < hh; y++, d += dsk, pt += psk)
    for (x = 0; x < hw; x++, d++, pt += 3)
      if ((pt[2] >= zlo) && (pt[2] <= zhi))
      {
        // find floor position of pixel
        fx = (m * (pt[0] - 32768) + x0) >> 12;
        if ((fx < 0) || (fx >= w))
          continue;
        fy = (m * (pt[1] - 32768) + y0) >> 12;
        if ((fy < 0) || (fy >= h))
          continue;

        // copy floor pixel
        *d = (UC8) src.ARef(fx, fy, 0);
      }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Reverse Mapping                            //
///////////////////////////////////////////////////////////////////////////

//= Create binary mask showing where overhead (map) blob comes from in sensor frontal view.
// recomputes map (x y) and ensures ht in range [over under] to lookup blob number in cc image 
// assumes mapping parameters (s2f) already set by one of main functions (e.g. FloorMap2)
// to make searching more efficient can supply frontal ROI by presetting dest
// mask only written inside this ROI so clear all of mask first if displaying
// adjusts mask ROI at exit to contain just marked pixels (add any border externally)
// does incremental back-projection without need for CacheXYZ to be valid
// useful for anaylzing color of objects (more pixels visible in frontal image)
// NOTE: projects map pixels to sensor, but overhead map view means vertical sides missed!
// returns 1 if okay, 0 if no valid pixels in mask

int jhcSurface3D::FrontMask (jhcImg& mask, const jhcImg& d16, double over, double under, const jhcImg& cc, int n)
{
  double a0, b0, c0, d0, a1, b1, c1, d1, a2, b2, c2, d2, abc0, abc1, abc2;
  int zlim = __min(ROUND(dmax * 101.6 / ksc), 40000);                          // max = 10m (32.8')    
  int zbot = (int)(1.0 + 253.0 * (over  - z0) / (z1 - z0));
  int zcut = (int)(1.0 + 253.0 * (under - z0) / (z1 - z0));
  int dw = cc.XDim(), dh = cc.YDim(), sk = mask.RoiSkip(), sk2 = d16.RoiSkip(mask) >> 1;
  int x0 = mask.RoiX(), xlim = mask.RoiLimX(), y0 = mask.RoiY(), ylim = mask.RoiLimY();
  int x, y, ix, iy, iz, lf = iw, rt = 0, bot = ih, top = 0;
  const US16 *z = (const US16 *) d16.RoiSrc(mask);
  UC8 *d = mask.RoiDest();

  // sanity check
  if (!mask.SameFormat(iw, ih, 1) || !mask.SameSize(d16, 2))
    return Fatal("Bad images to jhcSurface3D::FrontMask");
  mask.FillArr(0);

  // extract factors: ix = (a0 * x + b0 * y + c0) * z + d0
  a0 = s2f.MRef(0, 0);
  b0 = s2f.MRef(1, 0);
  c0 = s2f.MRef(2, 0) - 0.5 * (a0 * (iw - 1) + b0 * (ih - 1));
  d0 = s2f.MRef(3, 0);

  // extract factors: iy = (a1 * x + b1 * y + c1) * z + d1
  a1 = s2f.MRef(0, 1);
  b1 = s2f.MRef(1, 1);
  c1 = s2f.MRef(2, 1) - 0.5 * (a1 * (iw - 1) + b1 * (ih - 1));
  d1 = s2f.MRef(3, 1);

  // extract factors: iz = (a2 * x + b2 * y + c2) * z + d2
  a2 = s2f.MRef(0, 2);
  b2 = s2f.MRef(1, 2);
  c2 = s2f.MRef(2, 2) - 0.5 * (a2 * (iw - 1) + b2 * (ih - 1));
  d2 = s2f.MRef(3, 2);

  // adjust for left edge
  c0 += a0 * x0;
  c1 += a1 * x0;
  c2 += a2 * x0;

  // project pixels in ROI to find map pixel
  for (y = y0; y <= ylim; y++, d += sk, z += sk2)
  {
    // compute beginning values for this line (prevent round-off drift)
    abc0 = b0 * y + c0;
    abc1 = b1 * y + c1;
    abc2 = b2 * y + c2;

    // remap pixels in a row of the depth image
    for (x = x0; x <= xlim; x++, d++, z++, abc0 += a0, abc1 += a1, abc2 += a2)
      if ((*z >= choke) && (*z <= zlim))
      {
        // find map location for pixel and check if in valid map region
        iz = (int)(abc2 * (*z) + d2);
        if ((iz < zbot) || (iz >= zcut))
          continue;
        iy = (int)(abc1 * (*z) + d1);
        if ((iy < 0) || (iy >= dh))
          continue;
        ix = (int)(abc0 * (*z) + d0);
        if ((ix < 0) || (ix >= dw))
          continue;

        // see if component at this surface location matches desired
        if (cc.ARef16(ix, iy) == n)
        {
          // mark pixel and expand binary mask zone in output
          *d = 255;
          lf  = __min(lf, x);
          rt  = __max(rt, x);
          bot = __min(bot, y);
          top = __max(top, y);
        }
      }
  }

  // adjust output ROI so it encloses just mask pixels
  mask.SetRoiLims(lf, bot, rt, top);
  if (mask.RoiArea() <= 0) 
    return 0;
  return 1;
}


//= Create binary mask showing where overhead (map) blob comes from in alternate camera view.
// recomputes map (x y) and ensures ht in range [over under] to lookup blob number in cc image 
// assumes sensor -> map parameters (s2p) already set by one of main functions (e.g. FloorMap2)
// assumes map -> view parameters (p2v) already set by SetColorGeom() for alternate camera
// to make searching more efficient can supply sensor bounding box by presetting mask ROI (for d16!)
// adjusts mask ROI at exit to contain just marked pixels (add any border externally)
// NOTE: 3-way projection is better for object sides: sensor -> map + pixel validation -> view 
// returns 1 if okay, 0 if no valid pixels in mask

int jhcSurface3D::ViewMask (jhcImg& mask, const jhcImg& d16, double over, double under, const jhcImg& cc, int n)
{
  jhcMatrix s2v(4, 4);
  double a0, b0, c0, d0, a1, b1, c1, d1, a2, b2, c2, d2, abc0, abc1, abc2;
  double va0, vb0, vc0, vd0, va1, vb1, vc1, vd1, va2, vb2, vc2, vd2, vabc0, vabc1, vabc2;
  double vx, vy, vz, midx = 0.5 * (iw - 1), midy = 0.5 * (ih - 1);
  int zbot = (int)(1.0 + 253.0 * (over  - z0) / (z1 - z0));
  int zcut = (int)(1.0 + 253.0 * (under - z0) / (z1 - z0));
  int x0 = mask.RoiX(), xlim = mask.RoiLimX(), y0 = mask.RoiY(), ylim = mask.RoiLimY();
  int mw = cc.XDim(), mh = cc.YDim(), sk2 = d16.RoiSkip(mask) >> 1;
  int x, y, mx, my, mz, ix, iy, lf = iw, rt = 0, bot = ih, top = 0;
  const US16 *z = (const US16 *) d16.RoiSrc(mask);

  // sanity check (view must be same size as sensor to make loops work)
  if (!mask.SameFormat(iw, ih, 1) || !mask.SameSize(d16, 2))
    return Fatal("Bad images to jhcSurface3D::ViewMask");
  mask.FillArr(0);

  // SENSOR -> FLOOR transform -------------------------------------------
  // extract factors: mx = (a0 * x + b0 * y + c0) * z + d0
  a0 = s2f.MRef(0, 0);
  b0 = s2f.MRef(1, 0);
  c0 = s2f.MRef(2, 0) - a0 * midx - b0 * midy; 
  d0 = s2f.MRef(3, 0);

  // extract factors: my = (a1 * x + b1 * y + c1) * z + d1
  a1 = s2f.MRef(0, 1);
  b1 = s2f.MRef(1, 1);
  c1 = s2f.MRef(2, 1) - a1 * midx - b1 * midy;    
  d1 = s2f.MRef(3, 1);

  // extract factors: mz = (a2 * x + b2 * y + c2) * z + d2
  a2 = s2f.MRef(0, 2);
  b2 = s2f.MRef(1, 2);
  c2 = s2f.MRef(2, 2) - a2 * midx - b2 * midy;   
  d2 = s2f.MRef(3, 2);

  // adjust for left edge
  c0 += a0 * x0;
  c1 += a1 * x0;
  c2 += a2 * x0;

  // SENSOR -> MAP -> VIEW transform -------------------------------------
  s2v.MatMat(p2v, s2p);

  // extract factors: vx = (va0 * x + vb0 * y + vc0) * z + vd0
  va0 = s2v.MRef(0, 0);
  vb0 = s2v.MRef(1, 0);
  vc0 = s2v.MRef(2, 0) - va0 * midx - vb0 * midy;  
  vd0 = s2v.MRef(3, 0);

  // extract factors: vy = (va1 * x + vb1 * y + vc1) * z + vd1
  va1 = s2v.MRef(0, 1);
  vb1 = s2v.MRef(1, 1);
  vc1 = s2v.MRef(2, 1) - va1 * midx - vb1 * midy;  
  vd1 = s2v.MRef(3, 1);

  // extract factors: vz = (va2 * x + vb2 * y + vc2) * z + vd2
  va2 = s2v.MRef(0, 2);
  vb2 = s2v.MRef(1, 2);
  vc2 = s2v.MRef(2, 2) - va2 * midx - vb2 * midy;  
  vd2 = s2v.MRef(3, 2);

  // adjust for left edge
  vc0 += va0 * x0;
  vc1 += va1 * x0;
  vc2 += va2 * x0;

  // scan pixels in depth SENSOR ROI and find associated overhead map pixels
  for (y = y0; y <= ylim; y++, z += sk2)
  {
    // compute beginning values for this line (prevent round-off drift)
    abc0 = b0 * y + c0;
    abc1 = b1 * y + c1;
    abc2 = b2 * y + c2;

    // compute beginning values for sensor->view projection
    vabc0 = vb0 * y + vc0;
    vabc1 = vb1 * y + vc1;
    vabc2 = vb2 * y + vc2;

    // project all SENSOR pixels in a row to map image and view image
    for (x = x0; x <= xlim; x++, z++, abc0 += a0, abc1 += a1, abc2 += a2, vabc0 += va0, vabc1 += va1, vabc2 += va2)
//      if ((*z >= choke) && (*z <= zlim))
if (*z < 0xFFFF)
      {
        // find map location for SENSOR pixel and check if in valid region
        mz = (int)(abc2 * (*z) + d2);
        if ((mz < zbot) || (mz >= zcut))
          continue;
        my = (int)(abc1 * (*z) + d1);
        if ((my < 0) || (my >= mh))
          continue;
        mx = (int)(abc0 * (*z) + d0);
        if ((mx < 0) || (mx >= mw))
          continue;

        // see if component at this surface location matches desired
        if (cc.ARef16(mx, my) == n)
        {
          // find view pixel for original SENSOR pixel and check if in bounds
          vz = vabc2 * (*z) + vd2;
          if (vz < 0.0)
            continue;
          vy = vabc1 * (*z) + vd1;
          iy = ROUND(0.5 * (ih - 1) + (vy / vz));
          if ((iy < 0) || (iy >= ih))
            continue;
          vx = vabc0 * (*z) + vd0;
          ix = ROUND(0.5 * (iw - 1) + (vx / vz));
          if ((ix < 0) || (ix >= iw))
            continue;

          // mark pixel and expand cumulative non-zero zone for output
          mask.ASet(ix, iy, 0, 255);          
          lf  = __min(lf, ix);
          rt  = __max(rt, ix);
          bot = __min(bot, iy);
          top = __max(top, iy);
        }
      }
  }

  // adjust output ROI so it encloses just mask pixels
  mask.SetRoiLims(lf, bot, rt, top);
  if (mask.RoiArea() <= 0) 
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                     Coordinate Transformations                        //
///////////////////////////////////////////////////////////////////////////

//= Determines coordinate of an DEPTH image pixel in cached projection (not world).
// image coordinates are wrt original sized input, z is native 4x mm scale

void jhcSurface3D::ToCache (double& px, double& py, double& pz, 
                            double rx, double ry, double rz) const
{
  jhcMatrix img(4), proj(4);

  img.SetVec3((rx - 0.5 * (hw - 1)) * rz, (ry - 0.5 * (hh - 1)) * rz, rz);
  proj.MatVec(s2p, img);
  px = proj.X();
  py = proj.Y();
  pz = proj.Z();
}


//= Returns coordinates wrt original sized COLOR image of some point in projection (not world).
// all projected coordinates are in 0.02 inch steps with zero at 32768
// ix and iy are in pixels, iz is in 4 * mm (101.6 * in)

void jhcSurface3D::FromCache (double& cx, double& cy, double& cz, 
                              double px, double py, double pz) const
{
  jhcMatrix img(4), proj(4);

  proj.SetVec3(px, py, pz);
  img.MatVec(p2v, proj);
  cx = 0.5 * (hw - 1) + (img.X() / img.Z());
  cy = 0.5 * (hh - 1) + (img.Y() / img.Z());
  cz = img.Z();
} 


//= Get 3D world point (in inches) from full-sized DEPTH image and raw depth at current sensor pose.
// resulting RELATIVE world coords have sensor at origin and looking along y axis
// Note: set sc = 2.0 for hi-resolution Kinect images (1280 960)

void jhcSurface3D::WorldPt (double& wx, double& wy, double& wz, 
                            double rx, double ry, double rz, double sc) const
{
  double sc2 = ((hw < iw) ? 2.0 * sc : sc);

  ToCache(wx, wy, wz, rx / sc2, ry / sc2, rz);
  wx = 0.02 * (wx - 32768.0); 
  wy = 0.02 * (wy - 32768.0);
  wz = 0.02 * (wz - 32768.0);
}


//= Get 3D position vector from full-size DEPTH image and raw depth.
// resulting world coords have sensor at origin and looking along y axis
// Note: set sc = 2.0 for hi-resolution Kinect images (1280 960)

void jhcSurface3D::WorldPt (jhcMatrix& w, double rx, double ry, double rz, double sc) const
{
  double wx, wy, wz; 

  WorldPt(wx, wy, wz, rx, ry, rz, sc); 
  w.SetVec3(wx, wy, wz);
}


//= Get projection of 3D world point to COLOR image using current pose.
// assumes sensor is always at origin and looking along y axis
// Note: set sc = 2.0 for hi-resolution Kinect images (1280 960)
// returns non-scaled z coordinate (for use with WorldPt)

double jhcSurface3D::ImgPtZ (double& cx, double& cy, 
                             double wx, double wy, double wz, double sc) const
{
  double x, y, z, sc2 = ((hw < iw) ? 2.0 * sc : sc);
  
  FromCache(x, y, z, 32768.0 + 50.0 * wx, 32768.0 + 50.0 * wy, 32768.0 + 50.0 * wz);
  cx = sc2 * x;
  cy = sc2 * y; 
  return z;
}


//= Get projection of 3D world point (in inches) to full-sized COLOR image with current camera pose.
// assumes sensor is always at origin and looking along y axis
// Note: set sc = 2.0 for hi-resolution Kinect images (1280 960)
// returns 1 if point is in image, 0 if out of bounds

int jhcSurface3D::ImgPt (double& cx, double& cy, 
                         double wx, double wy, double wz, double sc) const
{
  double sc2 = ((hw < iw) ? 2.0 * sc : sc);
  
  ImgPtZ(cx, cy, wx, wy, wz, sc);
  if ((cx < 0) || (cx >= (sc2 * hw)) || (cy < 0) || (cy >= (sc2 * hh)))
    return 0;
  return 1;
}


//= Find equivalent bounding box in source image for an XZ oriented patch in 3D.
// xsz and zsz are the sizes in various directions centered on (wx wy wz)
// sc = 1 is for (640 480), use sc = 2 for hi-res Kinect images
// returns 1 if whole box is in image, 0 if out of bounds

int jhcSurface3D::ImgRect (jhcRoi& box, double wx, double wy, double wz, 
                           double xsz, double zsz, double sc) const
{
  double lf = 0.0, rt = 0.0, bot = 0.0, top = 0.0;
  double x, z, ix, iy;
  int i, j;

  // examine all 4 corners of plane
  x = wx - 0.5 * xsz;
  for (i = 0; i < 2; i++, x += xsz)
  {
    z = wz - 0.5 * zsz;
    for (j = 0; j < 2; j++, z += zsz)
    {
      // find image coordinates of corner
      ImgPt(ix, iy, x, wy, z, sc);
      if ((i == 0) && (j == 0))
      {
        // remember if first point tried
        lf = ix;
        rt = ix;
        bot = iy;
        top = iy;
      }       
      else
      {
        // add to bounds calculation
        lf = __min(lf, ix);
        rt = __max(rt, ix);
        bot = __min(bot, iy);
        top = __max(top, iy);
      }
    }
  }

  // set ROI with limits found and check if inside image
  box.SetRoiLims(ROUND(lf), ROUND(bot), ROUND(rt), ROUND(top));
  if ((lf < 0) || (rt >= hw) || (bot < 0) || (top >= hh))
    return 0;
  return 1;
}


//= Find equivalent bounding box in source image for a region in 3D.
// models head as axis-parallel rectangular solid in world coordinates
// xsz, ysz, zsz are the sizes in various directions centered on (wx wy wz)
// sc = 1 is for (640 480), use sc = 2 for hi-res Kinect images
// returns 1 if whole box is in image, 0 if out of bounds

int jhcSurface3D::ImgCube (jhcRoi& box, double wx, double wy, double wz, 
                           double xsz, double ysz, double zsz, double sc) const
{
  double lf = 0.0, rt = 0.0, bot = 0.0, top = 0.0;
  double x, y, z, ix, iy;
  int i, j, k;
  
  // examine all 8 corners of volume
  x = wx - 0.5 * xsz;
  for (i = 0; i < 2; i++, x += xsz)
  {
    y = wy - 0.5 * ysz;
    for (j = 0; j < 2; j++, y += ysz)
    {
      z = wz - 0.5 * zsz;
      for (k = 0; k < 2; k++, z += zsz)
      {
        // find image coordinates of corner
        ImgPt(ix, iy, x, y, z, sc);
        if ((i == 0) && (j == 0) && (k == 0))
        {
          // remember if first point tried
          lf = ix;
          rt = ix;
          bot = iy;
          top = iy;
        }       
        else
        {
          // add to bounds calculation
          lf = __min(lf, ix);
          rt = __max(rt, ix);
          bot = __min(bot, iy);
          top = __max(top, iy);
        }
      }
    }
  }

  // set ROI with limits found and check if inside image
  box.SetRoiLims(ROUND(lf), ROUND(bot), ROUND(rt), ROUND(top));
  if ((lf < 0) || (rt >= hw) || (bot < 0) || (top >= hh))
    return 0;
  return 1;
}


//= Find equivalent bounding box in source image for flat, oriented rectangular solid.
// ht is along z, len is at orientation ang in xy plane, wid is perpendicular to this
// sc = 1 is for (640 480), use sc = 2 for hi-res Kinect images
// returns 1 if whole box is in image, 0 if out of bounds

int jhcSurface3D::ImgPrism (jhcRoi& box, double wx, double wy, double wz, double ang,
                            double len, double wid, double ht, double sc) const
{
  double rads = D2R * ang, c = cos(rads), s = sin(rads);
  double idx = len * c, idy = len * s, jdx = -wid * s, jdy = wid * c;
  double lf = 0.0, rt = 0.0, bot = 0.0, top = 0.0;
  double x0, y0, x, y, z, ix, iy;
  int i, j, k;
  
  // examine all 8 corners of volume
  x0 = wx - 0.5 * idx;
  y0 = wy - 0.5 * idy;
  for (i = 0; i < 2; i++, x0 += idx, y0 += idy)    // planar position of 2 long ends
  {
    x = x0 - 0.5 * jdx;
    y = y0 - 0.5 * jdy;
    for (j = 0; j < 2; j++, x += jdx, y += jdy)    // position of 2 corners of end
    {
      z = wz - 0.5 * ht;
      for (k = 0; k < 2; k++, z += ht)             // 2 heights: on table and full
      {
        // find image coordinates of corner
        ImgPt(ix, iy, x, y, z, sc);
        if ((i == 0) && (j == 0) && (k == 0))
        {
          // if first point tried, just remember 
          lf = ix;
          rt = ix;
          bot = iy;
          top = iy;
        }       
        else
        {
          // else expand point set bounds
          lf = __min(lf, ix);
          rt = __max(rt, ix);
          bot = __min(bot, iy);
          top = __max(top, iy);
        }
      }
    }
  }

  // set ROI with limits found and check if inside image
  box.SetRoiLims(ROUND(lf), ROUND(bot), ROUND(rt), ROUND(top));
  if ((lf < 0) || (rt >= hw) || (bot < 0) || (top >= hh))
    return 0;
  return 1;
}


//= Find equivalent bounding box in source image for a sphere around point.
// diam is the diameter of the sphere centered on (wx wy wz)
// sc = 1 is for (640 480), use sc = 2 for hi-res Kinect images
// returns 1 if whole box is in image, 0 if out of bounds

int jhcSurface3D::ImgSphere (jhcRoi& box, double wx, double wy, double wz,
                             double diam, double sc) const
{
  double dx = wx - cx, dy = wy - cy, dz = wz - cz, r = 0.5 * diam; 
  double dist, dr, rcp, rsp, ct, st, rstcp, rstsp, rct, ix, iy, lf, rt, bot, top; 

  // find distance to point and parallel to floor plane
  dist = sqrt(dx * dx + dy * dy + dz * dz);
  dr = sqrt(dx * dx + dy * dy);

  // find horizontal perpendicular across sphere at viewing angle
  rcp = r * dx / dr;  
  rsp = r * dy / dr;  
  ImgPt(lf, iy, wx - rsp, wy + rcp, wz, sc);
  ImgPt(rt, iy, wx + rsp, wy - rcp, wz, sc);

  // find vertical perpendicular across sphere at viewing angle
  ct = dr / dist;
  st = dz / dist;
  rstcp = rcp * st;   
  rstsp = rsp * st;   
  rct = r * ct;
  ImgPt(ix, top, wx - rstcp, wy - rstsp, wz + rct, sc);
  ImgPt(ix, bot, wx + rstcp, wy + rstsp, wz - rct, sc);

  // set ROI with limits found and check if inside image
  box.SetRoiLims(ROUND(lf), ROUND(bot), ROUND(rt), ROUND(top));
  if ((lf < 0) || (rt >= hw) || (bot < 0) || (top >= hh))
    return 0;
  return 1;
}


//= Find equivalent bounding box in source image for an upright cylinder around point.
// diam is the diameter and zsz is the height of the cylinder centered on (wx wy wz)
// sc = 1 is for (640 480), use sc = 2 for hi-res Kinect images
// returns 1 if whole box is in image, 0 if out of bounds

int jhcSurface3D::ImgCylinder (jhcRoi& box, double wx, double wy, double wz,
                               double diam, double zsz, double sc) const
{
  double dx = wx - cx, dy = wy - cy, r = 0.5 * diam, hh = 0.5 * zsz;
  double dr, rcp, rsp, ix, iy1, iy2, lf, rt, bot, top; 

  // find distance parallel to floor plane
  dr = sqrt(dx * dx + dy * dy);

  // find horizontal across cylinder middle perpendicular to viewing angle
  rcp = r * dx / dr;  
  rsp = r * dy / dr;  
  ImgPt(lf, iy1, wx - rsp, wy + rcp, wz, sc);
  ImgPt(rt, iy2, wx + rsp, wy - rcp, wz, sc);

  // find top of front and back curved sides
  ImgPt(ix, iy1, wx - rcp, wy - rsp, wz + hh, sc);    
  ImgPt(ix, iy2, wx + rcp, wy + rsp, wz + hh, sc);    
  top = __max(iy1, iy2);

  // find bottom of front and back curved sides
  ImgPt(ix, iy1, wx - rcp, wy - rsp, wz - hh, sc);    
  ImgPt(ix, iy2, wx + rcp, wy + rsp, wz - hh, sc);    
  bot = __min(iy1, iy2);

  // set ROI with limits found and check if inside image
  box.SetRoiLims(ROUND(lf), ROUND(bot), ROUND(rt), ROUND(top));
  if ((lf < 0) || (rt >= hw) || (bot < 0) || (top >= hh))
    return 0;
  return 1;
}


//= Computes approximate number of pixels per inch around given world point.
// checks projection of sphere of diameter "test" to get scale

double jhcSurface3D::ImgScale (double wx, double wy, double wz, double sc, double test) const
{
  jhcRoi box;

  ImgSphere(box, wx, wy, wz, test, sc);
  return(box.RoiW() / test);
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Given the current plane description find height of each pixel.
// a point on the plane has output value 128 (or 0 if pos > 0)
// a positive deviation of "zrng" leads to 255 (zero if unknown)
// must call CacheXYZ function first to fill internal "wxyz" image

int jhcSurface3D::Heights (jhcImg& dest, double zoff, double zrng, int pos)
{
  int dsk = dest.Skip(), zsk = wxyz.Skip() >> 1; 
  int x, y, v, z0 = 32768 + ROUND(zoff / 0.02);
  double sc = 127.0 * 0.02 / zrng, off = 128.5;
  const US16 *z = (US16 *) wxyz.PxlSrc() + 2;
  UC8 *d = dest.PxlDest();

  if (!dest.SameFormat(hw, hh, 1))
    return Fatal("Bad images to jhcSurface3D::Heights");
  if (pos > 0)
  {
    // positive only deviations from ground plane
    sc = 255.0 * 0.02 / zrng;
    off = 0.5;
  }

  for (y = 0; y < hh; y++, d += dsk, z += zsk)
    for (x = 0; x < hw; x++, d++, z += 3)
      if (*z <= z0)
        *d = 0;
      else
      {
        v = (int)(sc * ((*z) - z0) + off);
        *d = (UC8) __max(1, __min(v, 255));
      }
  return 1;
}


//= Mark areas which are consistent with found plane (within +/- th).
// converts image to grayscale then shades some areas green
// must call CacheXYZ function first to fill internal "wxyz" image

int jhcSurface3D::Ground (jhcImg& dest, double th)
{
  int x, y, i, dsk = dest.Skip(), zsk = wxyz.Skip() >> 1;
  int zlo = ROUND(-50.0 * th + 32768.0), zhi = ROUND(50.0 * th + 32768.0);
  const US16 *z = (US16 *) wxyz.PxlSrc() + 2;
  UC8 *d = dest.PxlDest();

  if (!dest.SameFormat(hw, hh, 3))
    return Fatal("Bad images to jhcSurface3D::Ground");

  zlo = __max(1, zlo);
  for (y = 0; y < hh; y++, d += dsk, z += zsk)
    for (x = 0; x < hw; x++, d += 3, z += 3)
    {
      i = (d[0] + d[1] + d[2] + 2) >> 2;
      d[0] = (UC8) i;
      d[2] = (UC8) i;
      if ((*z >= zlo) && (*z <= zhi))
        d[1] = 255;
      else
        d[1] = (UC8) i;
    }
  return 1;
}     
