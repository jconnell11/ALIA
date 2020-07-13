// jhcFindPlane.cpp : finds flat areas in depth images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2016 IBM Corporation
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
#include "Processing/jhcDraw.h"

#include "Depth/jhcFindPlane.h"     // common robot

#include "Processing/jhcResize.h"
///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcFindPlane::~jhcFindPlane ()
{
  dealloc_pts();
}


//= Default constructor initializes certain values.

jhcFindPlane::jhcFindPlane ()
{
  // run silent
  noisy = 0;

  // assume standard image size
  ilim = 0;
  SetSize(640, 480);

  // load processing parameters
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for fitting lines and merging bands.
// Note: Kinect color hfov = 62.7 degs --> flen = 525

int jhcFindPlane::fit_params (const char *fname)
{
  jhcParam *ps = &fps;
  int ok;

  ps->SetTag("plane_fit", 0);
  ps->NextSpec4( &vstep,  4,   "Vertical seed sampling");
  ps->NextSpec4( &hstep,  4,   "Horizontal seed sampling");
  ps->NextSpec4( &pmin,   5,   "Min line points");
  ps->NextSpecF( &fmin,  90.0, "Min line fit (R^2)");
  ps->NextSpec4( &bmin,   3,   "Min band agreement");
  ps->NextSpecF( &dev,    1.8, "Max err increase");

  ps->NextSpecF( &htol,   8.0, "Height tolerance (pct)");  // was 3" @ 44.6"
  ps->NextSpecF( &atol,   5.0, "Angle tolerance (deg)");   // was 4
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Set sizes of internal images based on a reference image.

void jhcFindPlane::SetSize (const jhcImg& ref)
{
  SetSize(ref.XDim(), ref.YDim());
}


//= Set sizes of internal images directly.

void jhcFindPlane::SetSize (int x, int y)
{
  // remember size
  iw = x;
  ih = y;

  // possibly rebuild point arrays
  alloc_pts(__max(x, y));
}


///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Estimate camera pan (ha), tilt (va), and orthogonal distance (d) to surface.
// rotates image to get proper plane slant based on surface hint (vert = wall)
// returns 1 if valid estimate made, 0 if no good fit found

int jhcFindPlane::SurfaceData (double& ha, double& va, double& d, const jhcImg& d16, 
                               const jhcRoi *area, int vert)
{
  ha = 0.0;
  va = 0.0;
  d  = 0.0;

jhcImg dalt;
jhcResize rsz;
double e, t, r, h;

jprintf("\n");

  e = Fit3D(t, r, h, d16);
  if (e < 0.0)
    jprintf("Normal: failed\n");
  else
    jprintf("Normal: t = %3.1f, r = %3.1f, h = %3.1f, e = %3.1f\n", t, r, h, e);

dalt.SetSize(d16);
rsz.FlipV(dalt, d16);

  e = Fit3D(t, r, h, dalt);
  if (e < 0.0)
    jprintf("FlipV: failed\n");
  else
    jprintf("FlipV: t = %3.1f, r = %3.1f, h = %3.1f, e = %3.1f\n", t, r, h, e);

  return 1;
}

///////////////////////////////////////////////////////////////////////////
//                      Plane Fitting Across Bands                       //
///////////////////////////////////////////////////////////////////////////

//= Estimate vertical tilt of plane in image as well as height and roll of camera.
// ksc = scaling factor for distance, kf = focal length in pixels
// dir: 0 = scan bottom up, 1 = top down, 2 = from left, 3 = from right
// if dh > 0 then constrains surface to be within dh of h value given (also needs to know t)
// returns avg error of new estimate, negative if could not find plane
// takes about 0.2ms (V) or 0.3ms (H) for VGA 4x4 subsample on 1.6GHz i7
// NOTE: leaves t, r, and h unchanged if unable to find a valid plane

double jhcFindPlane::Fit3D (double& t, double& r, double& h, const jhcImg& d16, 
                           int dir, double dh, double ksc, double kf)
{
  double stats[14];
  double dsc = 0.25 * ksc / 25.4, finv = 1.0 / kf;
  int b;

  if (!d16.SameFormat(iw, ih, 2))
    return Fatal("Bad images to jhcFindPlane::Fit3D");
  jprintf(2, noisy, "\njhcFindPlane::Fit3D with dir = %d, dh = %3.1f\n", dir, dh);

  // get potential seed points 
  if (dir <= 0)
    vbot_bands(d16, dsc, finv);
  else if (dir == 1)
    vtop_bands(d16, dsc, finv);
  else if (dir == 2)
    hlf_bands(d16, dsc, finv);
  else
    hrt_bands(d16, dsc, finv);

  // try to fit lines within bands
  for (b = 0; b < bands; b++)
    line_fit(b, pick_start(b));

  // eliminate bad YZ lines 
  for (b = 0; b < bands; b++)
    if ((fit[b] < fmin) || (vpt[b] < pmin))   
      keep[b] = -1;

  // look for a big consistent set of bands
  if (form_clique(stats, keep) <= 0)
    return -1.0;

  // extract values from clique statistics
  err  = stats[10];
  ht   = stats[11];
  tilt = stats[12] - 90.0;            // 0 degs is camera horizontal
  roll = stats[13];

  // return through variables
  h = ht;
  t = tilt;
  r = roll;
  return err;
}


//= Find a consistent set of bands and set group flags.
// returns 1 if successful and updates statistics and group markings

int jhcFindPlane::form_clique (double s[], int group[]) const
{
  int mark[bands];
  double best;
  int b, base;

  while (1)
  {
    // initialize copy of valid bands
    for (b = 0; b < bands; b++)
      mark[b] = group[b];

    // find highest remaining camera estimate (if any)
    base = -1;
    for (b = 0; b < bands; b++)
      if ((mark[b] > 0) && ((base < 0) || (off[b] > best)))
      {
        best = off[b];
        base = b;
      }

    // initialize clique with this band
    if (base < 0)
      return 0;
    jprintf(2, noisy, "  starting with band %d\n", base);
    init_stats(s, base);
    mark[base] = 2;

    // if not enough similar bands then try a different base
    if (add_compatible(s, mark, base) >= bmin)
      break;
    group[base] = 0;
  }

  // copy markings back to final clique flags  
  for (b = 0; b < bands; b++)
    if ((mark[b] <= 0) && (group[b] > 0))
      group[b] = 0;
  return 1;
}


//= Add all bands compatible with base to clique and return number that agree.
// alters plane statistics ("s") and tells which bands are in clique ("mark") 

int jhcFindPlane::add_compatible (double s[], int mark[], int base) const
{
  double diff, best, off0 = off[base], ang0 = ang[base];
  int b, win, cnt = 1;

  // add all compatible bands while keeping count
  while (1)
  {
    // get next closest band in height (if any)
    win = -1;
    for (b = 0; b < bands; b++)
      if (mark[b] == 1) 
      {
        diff = fabs(off[b] - off0);
        if ((win < 0) || (diff < best))
        {
          best = diff;
          win = b;
        }
      }
    if (win < 0)
      break;

    // check if too far away from current plane (in pct)
    if (best > (off0 * htol / 100.0))
    {
      // no further selection will be better
      jprintf(2, noisy, "    reject band %d based on distance change %4.2f\n", win, best);
      for (b = 0; b < bands; b++)
        if (mark[b] == 1)
          mark[b] = 0;
      break;
    }

    // skip band if angle too different from current plane
    if (fabs(ang[win] - ang0) > atol)
    {
      jprintf(2, noisy, "    reject band %d based on angle change %4.2f\n", win, fabs(ang[win] - ang0));
      mark[win] = 0;
      continue;
    }

    // skip band if average planar error increases
    if (try_band(s, win) <= 0)
    {
      jprintf(2, noisy, "    reject band %d based on plane fit\n", win);
      mark[win] = 0;
      continue;
    }

    // add band to clique and record new plane offset and angle
    jprintf(2, noisy, "    >> added band %d\n", win);
    mark[win] = 2;
    off0 = s[11];
    ang0 = s[12];
    cnt++;
  }
  return cnt;
}


//= Try adding band to statistics and see if deviation gets better.
// returns 1 and updates "s" array of stats if band is okay

int jhcFindPlane::try_band (double s[], int b) const
{
  double s2[14];
  double dlim = dev * s[10];
  int i, n = pcnt[b];

  // make copy of original stats (only data needed)
  for (i = 0; i < 10; i++)
    s2[i] = s[i];

  // add all valid points in band to surface estimation
  for (i = 0; i < n; i++)
    if (ok[b][i] > 0)
      add_point(s2, cx[b][i], cy[b][i], cz[b][i]);

  // see if new deviation is not too much worse than before
  if (plane_err(s2) > dlim)
    return 0;

  // copy new stats back into input arrays (including analysis values)
  for (i = 0; i < 14; i++)
    s[i] = s2[i];
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                     Least Squares Plane Fitting                       //
///////////////////////////////////////////////////////////////////////////

//= Initialize statistics based on point in some band.
// returns orthogonal standard deviation

double jhcFindPlane::init_stats (double s[], int b) const
{
  int i, n = pcnt[b];

  // clear all values
  for (i = 0; i < 14; i++)
    s[i] = 0.0;

  // add in all valid points from this band
  for (i = 0; i < n; i++)
    if (ok[b][i] > 0)
      add_point(s, cx[b][i], cy[b][i], cz[b][i]);

  // compute planar fit
  return plane_err(s);
}


//= Add a point to the set of statistics.
// ignores upper level indices (std, a, b, c, ht, tilt, roll)

void jhcFindPlane::add_point (double s[], double x, double y, double z) const
{
  s[0] += x;        // Sx
  s[1] += y;        // Sy
  s[2] += z;        // Sz
  s[3] += x * x;    // Sxx
  s[4] += y * y;    // Syy
  s[5] += z * z;    // Szz
  s[6] += x * y;    // Sxy
  s[7] += x * z;    // Sxz
  s[8] += y * z;    // Syz
  s[9] += 1.0;      // num
}


//= Fit a plane to the given statistics.
// updates indices: 10 = std, 11 = a, 12 = b, 13 = c
// returns orthogonal standard deviation
//
// does a least-squares plane fit to the z coordinates of surface points.
// using statistic sums to fit plane to all points so far.
// returns average orthogonal distance of points from final plane
// <pre>
//
// plane equation:
//
//   A * x + B * y + C * z + D = 0
//   z = (-A/C) * x + (-B/C) * y - (D/C)
//     = a * x + b * y + c
// where:
//   a = -A/C, b = -B/C, c = -D/C
//
//
// residual error squared:
//
//   n * r^2 = sum[ (z - (ax + by + c))^2 ]
//           = sum[ z^2 - 2z(ax + by + c) + (ax + by + c)^2 ]
//           = sum[ z^2 - 2a(xz) - 2b(yz) - 2c(z) + (a^2x^2 + 2ax(by + c) + (by + c)^2) ] 
//           = sum[ z^2 - 2a(xz) - 2b(yz) - 2c(z) + a^2(x^2) + 2ab(xy) + 2ac(x) + b^2(y^2) + 2bc(y) + c^2 ]
//           = sum[ z^2 + a^2(x^2) + b^2(y^2) + 2[ab(xy) - a(xz) - b(yz) + c(a(x) + b(y) - z))] + c^2 ]   
//           = ( Szz + a^2 * Sxx + b^2 * Syy 
//               + 2 * [ab * Sxy - a * Sxz - b * Syz 
//                      + c * (a * Sx + b * Sy - Sz)] ) 
//             + n * c^2 
// where:
//   Si = sum[ i ], Sij = sum[ i * j ]
//
//
// residual error squared minimum wrt a, b, c:
//
//   d(n * r^2)/da = 0 = sum[ 2 * (z - (ax + by + c)) * (-x) ] 
//                   0 = -2 * sum[ z * x - (a * x * x + b * x * y + c * x) ]
//     --> Sxz = a * Sxx + b * Sxy + c * Sx       
//
//   d(n * r^2)/db = 0 = sum[ 2 * (z - (ax + by + c)) * (-y) ]
//                   0 = -2 * sum[ z * y - (a * x * y + b * y * y + c * y) ]
//     --> Syz = a * Sxy + b * Syy + c * Sy       
//
//   d(n * r^2)/dC = 0 = sum[ 2 * (z - (ax + by + c)) * -1 ] = 0
//                   0 = -2 * sum[ z - (a * x + b * y + c) ]
//     -->  Sz = a * Sx + b * Sy + n * c       
//
// in matrix form:
//
//   | Sxx  Sxy  Sx | | a |   | Sxz |
//   | Sxy  Syy  Sy | | b | = | Syz |
//   | Sx   Sy   n  | | c |   | Sz  |
//
//
// solving for a, b, c by inverting the matrix of moments:
//
//   | a |    | Sxx  Sxy  Sx |-1 | Sxz |
//   | b | =  | Sxy  Syy  Sy |   | Syz |
//   | c |    | Sx   Sy   n  |   | Sz  |
//
//   | a |     1    |  ( nSyy - SySy )  -( nSxy - SxSy )  ( SySxy - SxSyy ) | | Sxz |
//   | b | =  --- * | -( nSxy - SxSy )   ( nSxx - SxSx ) -( SySxx - SxSxy ) | | Syz |
//   | c |    det   |  (SySxy - SxSyy)  -(SySxx - SxSxy)  (SxxSyy - SxySxy) | | Sz  |
//
//   where det = Sxx * (nSyy - SySy) - Sxy * (nSxy - SxSy) + Sx * (SySxy - SxSyy)
//
// NOTE: direct inversion is about 7x faster than Gauss-Jordan elimination
// </pre>

double jhcFindPlane::plane_err (double s[]) const
{
  double Sx = s[0], Sy = s[1], Sz = s[2], Sxx = s[3], Syy = s[4], Szz = s[5];
  double Sxy = s[6], Sxz = s[7], Syz = s[8], num = s[9];  
  double m00 = num * Syy - Sy * Sy, m10 = Sx * Sy - num * Sxy, m20 = Sy * Sxy - Sx * Syy;
  double m01 = m10, m11 = num * Sxx - Sx * Sx, m21 = Sx * Sxy - Sy * Sxx;
  double m02 = m20, m12 = m21, m22 = Sxx * Syy - Sxy * Sxy;
  double idet = 1.0 / (Sxx * m00 - Sxy * (num * Sxy - Sx * Sy) + Sx * m20);
  double nr2, std, a, b, c, ht, tilt, roll;

  // solve for plane parameters
  a = idet * (Sxz * m00 + Syz * m10 + Sz * m20);
  b = idet * (Sxz * m01 + Syz * m11 + Sz * m21);
  c = idet * (Sxz * m02 + Syz * m12 + Sz * m22);

  // compute sum of squared errors up to constant
  nr2  = a * Sx + b * Sy - Sz;
  nr2 *= c;
  nr2 += a * b * Sxy - a * Sxz - b * Syz;
  nr2 *= 2.0;
  nr2 += Szz + a * a * Sxx + b * b * Syy;

  // divide by number of points, add offset, tip by viewing angle
  std = (nr2 / num) + c * c;
  std /= (a * a + b * b + 1.0);  
  std = sqrt(std);

  // record parameters for normal = (a0, b0, -1) in camera coords
  // camera is at (0 0 0) looking along negative z axis 
  ht   = c / sqrt(a * a + b * b + 1.0);
  tilt = R2D * atan2(sqrt(a * a + b * b), 1.0);
  roll = R2D * atan2(a, b);

  // bind values back to input array
  s[10] = std;
  s[11] = ht;
  s[12] = tilt;
  s[13] = roll;
  return std;
}


///////////////////////////////////////////////////////////////////////////
//                      Line Fitting Within Band                         //
///////////////////////////////////////////////////////////////////////////

//= Only keep points in range h +/- dh from camera given known tilt.

void jhcFindPlane::ht_gate (int b, double dh, double h, double t)
{
  double ph, h0 = h - dh, h1 = h + dh;
  double rads = D2R * t, c = cos(rads), s = sin(rads);
  int i;

  for (i = 0; i < pcnt[b]; i++)
  {
    ph = s * cz[b][i] + c * cy[b][i];
    if ((ph < h0) || (ph > h1))
      ok[b][i] = -2;
  }
}


//= Find starting bin containing index of lowest cy point (negative if bad)

int jhcFindPlane::pick_start (int b)
{
  int i, cm, first, any = 0;
  double low;

  // depth sort points
  if (sort_band(b) < 2)
    return -1;

  // find lowest cy in depth sorted array as starting point
  for (cm = 0; cm < 1000; cm++)
    if ((i = sort[cm]) >= 0)
      if ((any <= 0) || (cy[b][i] < low))
      {
        low = cy[b][i];
        first = cm;
        any = 1;
      }

  // mark unused close points as invalid
  for (cm = 0; cm < first; cm++)
    if ((i = sort[cm]) >= 0)
      ok[b][i] = 0;
  return first;
}


//= Do hash-like insertion sort points to get minimum cy at each discrete cz.
// "sort" array contains indices of original points
// returns number of points in sorting array

int jhcFindPlane::sort_band (int b)
{
  int i, cm, zlast = 0, n = 0;

  // clear true depth array by writing an invalid index
  if (pcnt[b] <= 0)
    return 0;
  for (cm = 0; cm < 1000; cm++)
    sort[cm] = -1;

  // check all properly sloped points in band
  for (i = 0; i < pcnt[b]; i++)
    if (ok[b][i] > 0)
    {
      // drop point if depth is not increasing from bottom of image
      if (iz[b][i] < zlast)
      {
        ok[b][i] = -1;
        continue;
      }
      zlast = iz[b][i];

      // if proper bin is already filled then advance to next bin
      cm = iz[b][i] / 40;
      while (sort[cm] >= 0)
        if (++cm >= 1000)
          break;

      // if no more empty bins then give up on rest of points
      if (cm >= 1000)
      {
        while (i < pcnt[b])
          ok[b][i++] = -1;
        break;
      }

      // insert point into array
      sort[cm] = i;
      n++;
    }   
  return n;
}


//= Try to fit a straight line to cy and cz assuming cx roughly constant.
// uses least squares on points in some band starting with lowest cy 
// saves "fit" (R^2 measure), "ang", and "off" for band

void jhcFindPlane::line_fit (int b, int first)
{
  double s[6];
  int cm, i, last, num, n = 0;
  double r2, m0, b0, mwin, bwin, best = 0.0, low = 0.0;

  // initialize return values
  fit[b] = 0.0;
  ang[b] = 0.0;
  off[b] = 0.0;
  vpt[b] = 0;
  if (first < 0)
    return;

  // clear statistics
  for (i = 0; i < 6; i++)
    s[i] = 0.0;

  // keep trying to add points to get a smoother line
  for (cm = first; cm < 1000; cm++)
    if ((i = sort[cm]) >= 0)
    {
      // see if fit improves with new point
      add_line(s, cy[b][i], cz[b][i]);
      num = (int) s[5];
      if (num < 3)
        last = cm;
      else
      {
        r2 = line_vals(m0, b0, s);
        if ((num < 10) || (r2 >= best))
        {
          // extend line estimate up through this point
          best = r2;
          mwin = m0;
          bwin = b0;
          last = cm;
          n = num;
        }
      }
    }

  // mark unused far points as invalid
  for (cm = last + 1; cm < 1000; cm++)
    if ((i = sort[cm]) >= 0)
      ok[b][i] = 0;

  // convert slope and intercept into offset and angle
  fit[b] = best;
  ang[b] = R2D * atan(mwin); 
  off[b] = bwin / sqrt(mwin * mwin + 1.0);
  vpt[b] = n;
}


//= Add point statistics for line fitting within band.

void jhcFindPlane::add_line (double s[], double y, double z) const
{
  s[0] += y;      // Sy
  s[1] += z;      // Sz
  s[2] += y * y;  // Syy
  s[3] += z * z;  // Szz
  s[4] += y * z;  // Syz
  s[5] += 1.0;    // num
}


//= Using current statistics find best fitting YZ line slope and intercept.
// returns 100 * R^2 Pearson correlation coefficient

double jhcFindPlane::line_vals (double& m, double& b, double s[]) const
{
  double Sy = s[0], Sz = s[1], Syy = s[2], Szz = s[3], Syz = s[4], num = s[5];
  double top  = num * Syz - Sy * Sz;
  double bot1 = num * Syy - Sy * Sy, bot2 = num * Szz - Sz * Sz;

  m = top / bot1;
  b = (Sz - m * Sy) / num;
  return(100.0 * m * top / bot2);
}


///////////////////////////////////////////////////////////////////////////
//                          Surface Seed Points                          //
///////////////////////////////////////////////////////////////////////////

//= Accumulate points from bottom up in a number of vertically oriented bands.
// respects ROI of image, subsamples using vstep and hstep
// takes about 0.12ms for VGA 4x4 subsample on 1.6GHz i7

void jhcFindPlane::vbot_bands (const jhcImg& d16, double dsc, double finv)
{
  int x0 = d16.RoiX(), y0 = d16.RoiY(), xlim = d16.RoiLimX(), ylim = d16.RoiLimY();
  int xlim2 = xlim - hstep, vln = vstep * (d16.Line() >> 1), bw = iw / bands;
  int x, y, b, intra, xwin, zwin, n;
  double d, hw = 0.5 * iw, hh = 0.5 * ih;
  const US16 *s, *r = (US16 *) d16.RoiSrc();

  // no points in any band yet
  for (b = 0; b < bands; b++)
  {
    pcnt[b] = 0;
    keep[b] = 1;
  }

  // collect points within image ROI (bands aligned to left edge)
  for (y = y0; y <= ylim; y += vstep, r += vln)
  {
    s = r;
    b = 0;
    intra = bw - 1;
    zwin = 1759;
    for (x = x0; x <= xlim; x += hstep, s += hstep, intra -= hstep)
    {     
      // find max 16 bit value (4xmm in 0.44-10m) along row
      if ((*s <= 40000) && (*s > zwin))  // pick closest to left if tie
      {
        zwin = *s;
        xwin = x;
      }

      // see if done with column segment (or at right edge)
      if ((intra <= 0) || (x > xlim2))
      {
        // check for valid depth as winner
        if (zwin >= 1760) 
        {
          // set up to save next point
          n = pcnt[b];
          pcnt[b] += 1;

          // save image-based coordinates (depth as 4xmm uncorrected)
          ok[b][n] = 1;
          iz[b][n] = zwin;
          iy[b][n] = y;
          ix[b][n] = xwin;

          // save real-world coordinates (in true inches) wrt camera
          d = dsc * zwin;
          cz[b][n] = d;
          d *= finv;
          cx[b][n] = d * (xwin - hw);
          cy[b][n] = d * (y - hh);
        }

        // move on to find max in next band to right
        b++;
        intra = bw - 1;
        zwin = 1759;
      }
    }
  }
}


//= Accumulate points from top down in a number of vertically oriented bands.
// respects ROI of image, subsamples using vstep and hstep
// takes about 0.12ms for VGA 4x4 subsample on 1.6GHz i7

void jhcFindPlane::vtop_bands (const jhcImg& d16, double dsc, double finv)
{
  int x0 = d16.RoiX(), y0 = d16.RoiY(), xlim = d16.RoiLimX(), ylim = d16.RoiLimY();
  int xlim2 = xlim - hstep, vln = vstep * (d16.Line() >> 1), bw = iw / bands;
  int x, y, b, intra, xwin, zwin, n;
  double d, hw = 0.5 * iw, hh = 0.5 * ih;
  const US16 *s, *r = (US16 *) d16.RoiSrc(x0, ylim);

  // no points in any band yet
  for (b = 0; b < bands; b++)
  {
    pcnt[b] = 0;
    keep[b] = 1;
  }

  // collect points within image ROI (bands aligned to left edge)
  for (y = ylim; y >= y0; y -= vstep, r -= vln)
  {
    s = r;
    b = 0;
    intra = bw - 1;
    zwin = 1759;
    for (x = x0; x <= xlim; x += hstep, s += hstep, intra -= hstep)
    {     
      // find max 16 bit value (4xmm in 0.44-10m) along row
      if ((*s <= 40000) && (*s > zwin))  // pick closest to left if tie
      {
        zwin = *s;
        xwin = x;
      }

      // see if done with column segment (or at right edge)
      if ((intra <= 0) || (x > xlim2))
      {
        // check for valid depth as winner
        if (zwin >= 1760) 
        {
          // set up to save next point
          n = pcnt[b];
          pcnt[b] += 1;

          // save image-based coordinates (depth as 4xmm uncorrected)
          ok[b][n] = 1;
          iz[b][n] = zwin;
          iy[b][n] = y;
          ix[b][n] = xwin;

          // save real-world coordinates (in true inches) wrt camera
          d = dsc * zwin;
          cz[b][n] = d;
          d *= finv;
          cx[b][n] = d * (xwin - hw);
          cy[b][n] = d * (hh - y);     // flip y for top down
        }

        // move on to find max in next band to right
        b++;
        intra = bw - 1;
        zwin = 1759;
      }
    }
  }
}


//= Accumulate points from left forward in a number of horizontally oriented bands.
// respects ROI of image, subsamples using vstep and hstep
// takes about 0.25ms for VGA 4x4 subsample on 1.6GHz i7

void jhcFindPlane::hlf_bands (const jhcImg& d16, double dsc, double finv)
{
  int x0 = d16.RoiX(), y0 = d16.RoiY(), xlim = d16.RoiLimX(), ylim = d16.RoiLimY();
  int ylim2 = ylim - vstep, vln = vstep * (d16.Line() >> 1), bh = ih / bands;
  int x, y, b, intra, ywin, zwin, n;
  double d, hw = 0.5 * iw, hh = 0.5 * ih;
  const US16 *s, *r = (US16 *) d16.RoiSrc();

  // no points in any band yet
  for (b = 0; b < bands; b++)
  {
    pcnt[b] = 0;
    keep[b] = 1;
  }

  // collect points within image ROI (bands aligned to bottom edge)
  for (x = x0; x <= xlim; x += hstep, r += hstep)
  {
    s = r;
    b = 0;
    intra = bh - 1;
    zwin = 1759;
    for (y = y0; y <= ylim; y += vstep, s += vln, intra -= vstep)
    {     
      // find max 16 bit value (4xmm in 0.44-10m) along column
      if ((*s <= 40000) && (*s > zwin))  // pick closest to bottom if tie
      {
        zwin = *s;
        ywin = y;
      }

      // see if done with row segment (or at top edge)
      if ((intra <= 0) || (y > ylim2))
      {
        // check for valid depth as winner
        if (zwin >= 1760) 
        {
          // set up to save next point
          n = pcnt[b];
          pcnt[b] += 1;

          // save image-based coordinates (depth as 4xmm uncorrected)
          ok[b][n] = 1;
          iz[b][n] = zwin;
          iy[b][n] = ywin;
          ix[b][n] = x;

          // save real-world coordinates (in true inches) wrt camera
          d = dsc * zwin;
          cz[b][n] = d;
          d *= finv;
          cx[b][n] = d * (ywin - hh);  // swap X and Y for sideways orientation
          cy[b][n] = d * (x - hw);
        }

        // move on to find max in next band above
        b++;
        intra = bh - 1;
        zwin = 1759;
      }
    }
  }
}


//= Accumulate points from right backward in a number of horizontally oriented bands.
// respects ROI of image, subsamples using vstep and hstep
// takes about 0.25ms for VGA 4x4 subsample on 1.6GHz i7

void jhcFindPlane::hrt_bands (const jhcImg& d16, double dsc, double finv)
{
  int x0 = d16.RoiX(), y0 = d16.RoiY(), xlim = d16.RoiLimX(), ylim = d16.RoiLimY();
  int ylim2 = ylim - vstep, vln = vstep * (d16.Line() >> 1), bh = ih / bands;
  int x, y, b, intra, ywin, zwin, n;
  double d, hw = 0.5 * iw, hh = 0.5 * ih;
  const US16 *s, *r = (US16 *) d16.RoiSrc(xlim, y0);

  // no points in any band yet
  for (b = 0; b < bands; b++)
  {
    pcnt[b] = 0;
    keep[b] = 1;
  }

  // collect points within image ROI (bands aligned to bottom edge)
  for (x = xlim; x >= x0; x -= hstep, r -= hstep)
  {
    s = r;
    b = 0;
    intra = bh - 1;
    zwin = 1759;
    for (y = y0; y <= ylim; y += vstep, s += vln, intra -= vstep)
    {     
      // find max 16 bit value (4xmm in 0.44-10m) along column
      if ((*s <= 40000) && (*s > zwin))  // pick closest to bottom if tie
      {
        zwin = *s;
        ywin = y;
      }

      // see if done with row segment (or at top edge)
      if ((intra <= 0) || (y > ylim2))
      {
        // check for valid depth as winner
        if (zwin >= 1760) 
        {
          // set up to save next point
          n = pcnt[b];
          pcnt[b] += 1;

          // save image-based coordinates (depth as 4xmm uncorrected)
          ok[b][n] = 1;
          iz[b][n] = zwin;
          iy[b][n] = ywin;
          ix[b][n] = x;

          // save real-world coordinates (in true inches) wrt camera
          d = dsc * zwin;
          cz[b][n] = d;
          d *= finv;
          cx[b][n] = d * (ywin - hh);  // swap X and Y for sideways orientation
          cy[b][n] = d * (hw - x);     // flip y for right backwards
        }

        // move on to find max in next band above
        b++;
        intra = bh - 1;
        zwin = 1759;
      }
    }
  }
}


//= Make up arrays for saving image seed points.

void jhcFindPlane::alloc_pts (int n)
{
  int b;

  // see if arrays big enough already
  if (n <= ilim)
    return;
  if (ilim > 0)
    dealloc_pts();

  for (b = 0; b < bands; b++)
  {
    // make new point valid array 
    ok[b] = new int[n];

    // make new image point arrays
    ix[b] = new int[n];
    iy[b] = new int[n];
    iz[b] = new int[n];

    // make new camera point arrays
    cx[b] = new double[n];
    cy[b] = new double[n];
    cz[b] = new double[n];

    // zero point count
    pcnt[b] = 0;
  }

  // remember max number of points in a band
  ilim = n;
}


//= Get rid of image seed points for surface.

void jhcFindPlane::dealloc_pts ()
{
  int b;

  if (ilim <= 0)
    return;

  for (b = bands - 1; b >= 0; b--)
  {
    // delete camera point arrays
    delete [] cz[b];
    delete [] cy[b];
    delete [] cx[b];

    // delete image point arrays
    delete [] iz[b];
    delete [] iy[b];
    delete [] ix[b];

    // delete point valid array
    delete [] ok[b];
  }
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Overlay points used to estimate ground plane.
// can optionally show rejected points of various types (detail = 2):
//   green  = valid ground
//   red    = invalid flat
//   yellow = bad fit
//   blue   = bad slope or height
// can optionally show all elements in white (detail = 3):
//   plus   = valid ground
//   X      = invalid flat
//   circle = bad fit
//   square = bad slope or height
// zero or negative number for detail just shows some band
// if dir >= then shows boundary of band(s)

int jhcFindPlane::Seeds (jhcImg& dest, int detail, int dir)
{
  jhcDraw dr;
  int i, b, n, x, y, bsz, w = dest.XDim(), h = dest.YDim();
  int b0 = 0, b1 = bands - 1, sz = 17, xsz = ROUND(0.7 * sz);
  double sc = w / (double) iw;

  if (!dest.SameFormat(ROUND(sc * iw), ROUND(sc * ih), 3))
    return Fatal("Bad images to jhcFindPlane::Seeds");

  // see if just one band to display
  if (detail <= 0)
  {
    b0 = -detail;
    b1 = -detail;
  }

  // possibly mark band boundaries
  if (dir >= 2)
  {
    // mark horizontal band(s)
    bsz = h / bands;
    for (b = b0; b <= b1; b++)
      dr.RectEmpty(dest, 0, abs(b) * bsz, w, bsz, 1, 255, 0, 255); 
  }
  else if (dir >= 0)
  {
    // mark vertical band(s)
    bsz = w / bands;
    for (b = b0; b <= b1; b++)
      dr.RectEmpty(dest, abs(b) * bsz, 0, bsz, h, 1, 255, 0, 255); 
  }

  // show points rejected for various reasons in color
  if ((detail == 2) || (detail <= 0))
    for (b = b0; b <= b1; b++)
    {
      n = pcnt[b];
      for (i = 0; i < n; i++)
      {
        x = ROUND(sc * ix[b][i]);
        y = ROUND(sc * iy[b][i]);
        if (ok[b][i] < 0)
          dr.Cross(dest, x, y, sz, sz, 1, 0, 0, 255);    // blue
        else if (ok[b][i] == 0)
          dr.Cross(dest, x, y, sz, sz, 1, 255, 255, 0);  // yellow
        else if (keep[b] <= 0)
          dr.Cross(dest, x, y, sz, sz, 1, 255, 0, 0);    // red
      }
    }

  // show points rejected for various reasons as white shapes
  if (detail >= 3) 
    for (b = b0; b <= b1; b++)
    {
      n = pcnt[b];
      for (i = 0; i < n; i++)
      {
        x = ROUND(sc * ix[b][i]);
        y = ROUND(sc * iy[b][i]);
        if (ok[b][i] < 0)
          dr.RectEmpty(dest, x, y, xsz, xsz, 1, 255, 255, 255);   // box
        else if (ok[b][i] == 0)
          dr.CircleEmpty(dest, x, y, sz >> 1, 1, 255, 255, 255);  // circle
        else if (keep[b] <= 0)
          dr.XMark(dest, x, y, xsz, 1, 255, 255, 255);            // X
      }
    }

  // draw basic ground points
  for (b = b0; b <= b1; b++)
    if (keep[b] > 0)
    {
      n = pcnt[b];
      for (i = 0; i < n; i++)
        if (ok[b][i] > 0)
        {
          x = ROUND(sc * ix[b][i]);
          y = ROUND(sc * iy[b][i]);
          if (detail >= 3)
            dr.Cross(dest, x, y, sz, sz, 1, 255, 255, 255);  // white
          else
            dr.Cross(dest, x, y, sz, sz, 1, 0, 255, 0);      // green
        }
    }
  return 1;
}
