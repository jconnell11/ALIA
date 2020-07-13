// jhcShift.cpp : image and patch alignment via rigid shifting
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2003-2013 IBM Corporation
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

#include "Processing/jhcShift.h"


///////////////////////////////////////////////////////////////////////////
//                             Initialization                            //
///////////////////////////////////////////////////////////////////////////

// Done this way to make only one copy and keep it off the stack.
// Static member variables must be initialized at file scope.

int jhcShift::instances = 0;

UC8 *jhcShift::third = NULL;


//= Default constructor sets up some tables.

jhcShift::jhcShift ()
{
  int i;

  // state variables for CrispColor
  rdx = 0;
  rdy = 0;
  bdx = 0;
  bdy = 0;

  // see if initialization already done by another instance
  if (instances++ > 0)
    return;

  // allocate space for the table
  third = new UC8 [768];
  if (third == NULL)
    Fatal("Could not allocate table in jhcShift");

  // compute one third of all possible sums (AvgRGB)
  for (i = 0; i <= 765; i++)
    third[i] = (UC8)((i + 1) / 3);
}


//= Cleans up allocated tables if last instance in existence.

jhcShift::~jhcShift ()
{
  if (--instances > 0)
    return;

  // free array
  if (third != NULL)
    delete [] third;
  
  // reinitialize
  instances = 0;
  third = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                             Image Alignment                           //
///////////////////////////////////////////////////////////////////////////

//= Gets best image alignment which is useful for reducing camera jitter.
// takes only monochrome images and only looks where mask is < 128
// compares at ALL combinations of shifts in +/- xrng and +/- yrng 
// only compares one pixel every samp pixels in order to go faster
// ignores image ROIs: always compares all pixels (use mask if needed)
// number of times SAD is computed = 4 + (xhi - xlo + 1) * (yhi - ylo + 1)
//   for example searching +/- 8 in both directions -> 293 times!
// Note: shifts returned are amounts to correct src by (negated wrt older version)
// returns the average absolute mismatch

double jhcShift::AlignFull (double *xoff, double *yoff, const jhcImg& src, const jhcImg& ref, const jhcImg *mask,
                            int xlo, int xhi, int ylo, int yhi, int samp, int track) const 
{
  int dx, dy, xwin, ywin;
  double xt, yt, xmid = 0.5 * (xlo + xhi), ymid = 0.5 * (ylo + yhi);
  double sc, pbest, fbest, best = -1.0, dip = 0.0;

  if (!src.Valid(1) || !src.SameFormat(ref) || !src.SameFormat0(mask) ||
      (xlo > xhi) || (ylo > yhi) || (xoff == NULL) || (yoff == NULL))
    return Fatal("Bad inputs to jhcShift::AlignFull");

  // possibly shift preferred point for ties
  xt = -(*xoff);
  yt = -(*yoff);
  if ((track > 0) && (xt >= xlo) && (xt <= xhi) && (yt >= ylo) && (yt <= yhi))
  {
    xmid = xt;
    ymid = yt;
  }

  // find best overall shift
  for (dy = ylo; dy <= yhi; dy++)
    for (dx = xlo; dx <= xhi; dx++)
    {
      sc = offset_sad(dx, dy, samp, src, ref, mask);
      if ((best < 0.0) || 
          ((sc >= 0.0) && (sc < best)) ||
          ((sc >= 0.0) && (sc == best) && 
           (d2(dx, dy, xmid, ymid) < d2(xwin, ywin, xmid, ymid))))
      {
        best = sc;
        xwin = dx;
        ywin = dy;
      }
    }

  // refine position by recomputing neighbors and fitting parabola
  if ((xwin == xlo) || (xwin == xhi))
    *xoff = -xwin;
  else
  {
    pbest = offset_sad(xwin - 1, ywin, samp, src, ref, mask);
    fbest = offset_sad(xwin + 1, ywin, samp, src, ref, mask);
    *xoff = -parabolic(xwin, pbest, best, fbest, xmid, dip);
  }
  xwin = -ROUND(*xoff);
  if ((ywin == ylo) || (ywin == yhi))
    *yoff = -ywin;
  else
  {
    pbest = offset_sad(xwin, ywin - 1, samp, src, ref, mask);
    fbest = offset_sad(xwin, ywin + 1, samp, src, ref, mask);
    *yoff = -parabolic(ywin, pbest, best, fbest, ymid, dip);
  }
  return best;
}


//= Gets best image alignment which is useful for reducing camera jitter.
// takes only monochrome images and only looks where mask is < 128
// first shifts in range +/- xrng to get best, then +/- yrng to refine
// only compares one pixel every samp pixels in order to go faster
// ignores image ROIs: always compares all pixels (use mask if needed)
// number of times SAD is computed = (xhi - xlo + 1) + (yhi - ylo + 1)
//   for example searching +/- 8 in both directions -> 34 times
// Note: shifts returned are amounts to correct src by (negated wrt older version)
// returns the average absolute mismatch

double jhcShift::AlignCross (double *xoff, double *yoff, const jhcImg& src, const jhcImg& ref, const jhcImg *mask,
                             int xlo, int xhi, int ylo, int yhi, int samp, int track) const 
{
  double sc, last, pbest, fbest, best, dip = 0.0;
  double xt, yt, xmid = 0.5 * (xlo + xhi), ymid = 0.5 * (ylo + yhi);
  int dx, dy, xwin, ywin = ROUND(ymid);

  if (!src.Valid(1) || !src.SameFormat(ref) || !src.SameFormat0(mask) ||
      (xlo > xhi) || (ylo > yhi) || (xoff == NULL) || (yoff == NULL))
    return Fatal("Bad inputs to jhcShift::AlignCross");

  // possibly shift preferred point for ties
  xt = -(*xoff);
  yt = -(*yoff);
  if ((track > 0) && (xt >= xlo) && (xt <= xhi) && (yt >= ylo) && (yt <= yhi))
  {
    xmid = xt;
    ymid = yt;
  }

  // find best x shift
  last = -1.0;
  best = -1.0;
  for (dx = xlo; dx <= xhi; dx++)
  {
    sc = offset_sad(dx, ywin, samp, src, ref, mask);
    if ((best < 0.0) || 
        ((sc >= 0.0) && (sc < best)) ||
        ((sc >= 0.0) && (sc == best) && (fabs(dx - xmid) < fabs(xwin - xmid))))
    {
      best = sc;
      pbest = last;                // save previous score also
      xwin = dx;
    }
    else if (dx == (xwin + 1))
      fbest = sc;                  // save following score if last was winner
    last = sc;                     // save in case next is best
  }
  if ((xwin == xlo) || (xwin == xhi))
    *xoff = -xwin;
  else 
    *xoff = -parabolic(xwin, pbest, best, fbest, xmid, dip);
  xwin = -ROUND(*xoff);

  // find best y shift
  last = -1.0;
  best = -1.0;
  for (dy = ylo; dy <= yhi; dy++)
  {
    sc = offset_sad(xwin, dy, samp, src, ref, mask);
    if ((best < 0.0) || 
        ((sc >= 0.0) && (sc < best)) ||
        ((sc >= 0.0) && (sc == best) && (fabs(dy - ymid) < fabs(ywin - ymid))))
    {
      best = sc;
      pbest = last;                // save previous score also
      ywin = dy;
    }
    else if (dy == (ywin + 1))
      fbest = sc;                  // save following score if last was winner
    last = sc;                     // save in case next is best
  }
  if ((ywin == ylo) || (ywin == yhi))
    *yoff = -ywin;
  else
    *yoff = -parabolic(ywin, pbest, best, fbest, ymid, dip);

  return best;
}


//= Computes the squared distance between two points.

double jhcShift::d2 (double x1, double y1, double x2, double y2) const 
{
  return((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}


//= Do sub-pixel estimation by fitting parabola to adjacent scores.
// negative scores are invalid in which case no refinement is done
//   (y - v) = k * (x - c) ^ 2 where c is the dip location with score v
// form of equation taken from Numerical Recipes in C section 10.2

double jhcShift::parabolic (int loc, double before, double here, double after, 
                            double def, double dip) const 
{
  if ((before <= (here + dip)) && (after <= (here + dip)))
    return def;
  return(loc - (before - after) / (4.0 * here - 2.0 * (before + after)));
}

/*
// old version

int jhcShift::parabolic (double *ans, int loc, double before, double here, double after)
{
  double a = loc - 1, b = loc, c = loc + 1;
  double fa = before, fb = here, fc = after;
  double num, den;

  if ((before < 0.0) || (here < 0.0) || (after < 0.0))
    return 0;

  den = (b - a) * (fb - fc) - (b - c) * (fb - fa);
  if (den == 0.0) 
    return 0;

  num = (b - a) * (b - a) * (fb - fc) - (b - c) * (b - c) * (fb - fa);
  *ans = b - num / (2.0 * den);
  return 1;
}
*/


//= Computes global measure over image where mask < 128.
// returns average value of difference sampled every samp pixel on a grid
// ignores image ROIs: always compares all pixels (use mask if needed)

double jhcShift::offset_sad (int dx, int dy, int samp, 
                             const jhcImg& src, const jhcImg& ref, const jhcImg *mask) const 
{
  if (mask == NULL)
    return offset_sad_all(dx, dy, samp, src, ref);

  // local parameters
  int x, y, x0 = 0, xr = 0, y0 = 0, yr = 0, sad = 0, n = 0;
  int w = src.XDim(), ln = src.Line(), skip = samp * ln, size = src.PxlSize(); 
  const UC8 *s, *s0, *r, *r0, *m, *m0;

  // adjust all offsets to be positive
  if (dy < 0)
    yr = -dy * ln;
  else
    y0 = dy * ln;
  if (dx < 0)
    xr = -dx;
  else
    x0 = dx;

  // find starting point in images
  s0 =  src.PxlSrc() + y0 + x0;
  r0 =  ref.PxlSrc() + yr + xr;
  m0 = mask->PxlSrc() + yr + xr;

  // scan for average difference
  for (y = __max(y0, yr); y < size; y += skip)
  {
    s = s0;
    r = r0;
    m = m0;
    for (x = __max(x0, xr); x < w; x += samp)
    {
      if (*m < 128)
      {
        sad += abs((int) *s - *r);
        n++;
      }
      s += samp;
      r += samp;
      m += samp;          
    }
    s0 += skip;
    r0 += skip;
    m0 += skip;
  }

  // compute average value (if valid)
  if (n <= 0)
    return -1.0;
  return(sad / (double) n);
}


//= Computes global measure over image without any exclusions.
// returns average value of difference sampled every samp pixel on a grid
// ignores image ROIs: always compares all pixels 

double jhcShift::offset_sad_all (int dx, int dy, int samp, const jhcImg& src, const jhcImg& ref) const                              
{
  int x, y, x0 = 0, xr = 0, y0 = 0, yr = 0, sad = 0, n = 0;
  int w = src.XDim(), ln = src.Line(), skip = samp * ln, size = src.PxlSize(); 
  const UC8 *s, *s0, *r, *r0;

  // adjust all offsets to be positive
  if (dy < 0)
    yr = -dy * ln;
  else
    y0 = dy * ln;
  if (dx < 0)
    xr = -dx;
  else
    x0 = dx;

  // find starting point in images
  s0 =  src.PxlSrc() + y0 + x0;
  r0 =  ref.PxlSrc() + yr + xr;

  // scan for average difference
  for (y = __max(y0, yr); y < size; y += skip)
  {
    s = s0;
    r = r0;
    for (x = __max(x0, xr); x < w; x += samp)
    {
      sad += abs((int) *s - *r);
      n++;
      s += samp;
      r += samp;
    }
    s0 += skip;
    r0 += skip;
  }

  // compute average value (if valid)
  if (n <= 0)
    return -1.0;
  return(sad / (double) n);
}


///////////////////////////////////////////////////////////////////////////
//                          Line by Line Shifts                          //
///////////////////////////////////////////////////////////////////////////

//= Estimate line by line subpixel shifts to align source to reference.
// takes only monochrome images and only looks where mask is < 128
// returns vector of fractional horizontal shifts (fx size = image width)
// tests integer shifts between xlo and xhi with fixed vertical shift y0
// mode 0: compares every samp pixel, skips samp - 1 pixels between
// mode 1: compares all pixels in bands of 10 spaced evenly across image
// mode 2: compares all pixels in leftmost 1/samp of line
// useful for correcting "tearing" in analog NTSC signals with bad sync
// Note: shifts returned are amounts to correct src by (negated wrt older version)
// do actual correction with jhcResize::LineShift

int jhcShift::EstWobble (double *vdx, const jhcImg& src, const jhcImg& ref, const jhcImg& mask,                        
                         double fdx, double fdy, int wx, int samp, int sm, int mode) const 
{
  if (!src.Valid(1) || !src.SameFormat(ref) || !src.SameFormat(mask) || 
      (wx < 1) || (samp < 1) || (vdx == NULL))
    return Fatal("Bad inputs to jhcShift::EstWobble");

  // local variables
  double best, dip = 0.5;
  int xs, xr, xtop, dx, dy = ROUND(fdy), run = 4 * wx + 2;
  int i, j, win, xlo = ROUND(fdx) - wx, xhi = xlo + 2 * wx, n = 2 * wx + 1;
  int y, w = src.XDim(), h = src.YDim(), ylim = h - 1, ln = src.Line();
  const UC8 *s, *r, *m, *s0 = src.PxlSrc(), *r0 = ref.PxlSrc(), *m0 = mask.PxlSrc();
  double *f = vdx, *err = new double[n], *vdx2 = NULL;

  // default horizontal offset is in middle of range
  for (y = 0; y <= ylim; y++)
    f[y] = fdx;

  // adjust vertical offset to be positive
  if (dy < 0)
  {
    r0 += (-dy * ln);
    m0 += (-dy * ln); 
  }
  else
  {
    s0 += (dy * ln);
    f  += dy;
  }

  for (y = ylim - abs(dy); y > 0; y--, f++)
  {
    // compute average error for various line shifts 
    for (dx = xlo; dx <= xhi; dx++)
    {
      // adjust horizontal offset to be positive
      xs = 0;
      xr = 0;
      if (dx < 0)
        xr = -dx;
      else
        xs = dx;
      xtop = w - xs;

      // find starting point in images for this offset
      s = s0 + xs;
      r = r0 + xr;
      m = m0 + xr;

      // choose type of sampling for comparisoon
      i = dx - xlo;
      if ((mode <= 0) || (samp == 1))
        err[i] = line_sad0(s, r, m, xr, xtop, samp);
      else if (mode == 1)
        err[i] = line_sad1(s, r, m, xr, xtop, samp, run);
      else 
        err[i] = line_sad2(s, r, m, xr, xtop, samp, w);
    }

    // find lowest error position
    best = -1.0;
    for (i = 0; i < n; i++)
      if ((err[i] >= 0.0) && 
          ((best < 0.0) || 
           (err[i] < best) ||
           ((err[i] == best) && (abs(i - wx) < abs(win - wx))))) 
      {
        win = i;
        best = err[i];
      }

    // compute parabolic estimate of best horizontal shift
    if ((best >= 0.0) && (win > 0) && (win < n))
      *f = -parabolic(xlo + win, err[win - 1], best, err[win + 1], fdx, dip);

    // advance to next line
    s0 += ln;
    r0 += ln;
    m0 += ln;
  }

  // possibly smooth resulting shifts across adjacent lines
  if (sm > 0)
  {
    vdx2 = new double[h];
    for (j = 0; j < sm; j++)
    {
      // smooth values using 1 2 1 weighting (except ends)
      vdx2[0] = (2.0 * vdx[0] + vdx[1]) / 3.0;
      for (i = 1; i < ylim; i++)
        vdx2[i] = 0.25 * (vdx[i - 1] + 2.0 * vdx[i] + vdx[i + 1]);
      vdx2[ylim] = (vdx[ylim - 1] + 2.0 * vdx[ylim]) / 3.0;

      // copy back to result array
      for (i = 0; i < h; i++)
        vdx[i] = vdx2[i];
    }
    delete [] vdx2;
  }

  // deallocate temporary score array 
  delete [] err;
  return 1;
}


//= Compute SAD every samp pixels in line of reference image.
// ignores pixels where mask image is over 128

double jhcShift::line_sad0 (const UC8 *s0, const UC8 *r0, 
                            const UC8 *m0, int xr, int xtop, int samp) const                            
{
  int x, off = 0, sad = 0, n = 0;
  const UC8 *s = s0, *r = r0, *m = m0;

  // adjust so sampling happens at the same place in reference
  if ((xr % samp) > 0)
  {
    off = samp - (xr % samp);
    s += off;
    r += off;
    m += off;
  }

  // accumulate differences over valid area  
  for (x = xr + off; x < xtop; x += samp, s += samp, r += samp, m += samp)
    if (*m < 128)
    { 
      sad += abs((int) *s - *r);
      n++;
    }

  // compute average value (if valid)
  if (n <= 0)
    return -1.0;
  return(sad / (double) n);
}


// Computes SAD in bands of run pixels spaced evenly across line of reference image.
// ignores pixels where mask image is over 128

double jhcShift::line_sad1 (const UC8 *s0, const UC8 *r0, 
                            const UC8 *m0, int xr, int xtop, int samp, int run) const                            
{
  int x, off = 0, sad = 0, n = 0;
  int i, stride = run * samp, skip = stride - run;
  const UC8 *s = s0, *r = r0, *m = m0;

  // adjust so sampling starts with first full band in reference
  if (xr > 0)
  {
    off = stride * ((xr + samp) / samp);
    s += off;
    r += off;
    m += off;
  }

  for (x = xr + off; x < xtop; x += stride)
  {
    // compute SAD within band
    for (i = run; i > 0; i--, s++, r++, m++)
      if (*m < 128)
      { 
        sad += abs((int) *s - *r);
        n++;
      }

    // advance to next band
    s += skip;
    r += skip;
    m += skip;
  }

  // compute average value (if valid)
  if (n <= 0)
    return -1.0;
  return(sad / (double) n);
}


//= Compute SAD for first w / samp - xr pixels of reference image line.
// ignores pixels where mask image is over 128

double jhcShift::line_sad2 (const UC8 *s0, const UC8 *r0, 
                            const UC8 *m0, int xr, int xtop, int samp, int w) const                           
{
  int x, xtop2, sad = 0, n = 0;
  const UC8 *s = s0, *r = r0, *m = m0;

  // make sure never leaves valid area
  xtop2 = __min(w / samp, xtop);

  // accumulate differences over selected area
  for (x = xr; x < xtop2; x++, s++, r++, m++)
    if (*m < 128)
    { 
      sad += abs((int) *s - *r);
      n++;
    }

  // compute average value (if valid)
  if (n <= 0)
    return -1.0;
  return(sad / (double) n);
}


///////////////////////////////////////////////////////////////////////////
//                             Patch Finding                             //
///////////////////////////////////////////////////////////////////////////

//= Estimates the best match for an image fragment within another image.
// compares at ALL combinations of shifts in +/- xrng and +/- yrng 
// only compares one pixel every samp pixels in order to go faster
// ignores ROIs of src patch and ref image: always compares all pixels
// Note: shifts returned are amounts to correct src by (negated wrt older version)
// only works with monochrome, returns the average absolute mismatch

double jhcShift::PatchFull (double *xoff, double *yoff, const jhcImg& src, const jhcImg& ref, 
                            int xlo, int xhi, int ylo, int yhi, int samp, int track) const 
{
  int dx, dy, xwin, ywin;
  double xt, yt, xmid = 0.5 * (xlo + xhi), ymid = 0.5 * (ylo + yhi);
  double sc, pbest, fbest, best = -1.0, dip = 0.0;

  if (!src.Valid(1) || !src.SameFormat(ref) || 
      (xlo > xhi) || (ylo > yhi) || (xoff == NULL) || (yoff == NULL))
    return Fatal("Bad inputs to jhcShift::PatchFull");

  // possibly shift preferred point for ties
  xt = -(*xoff);
  yt = -(*yoff);
  if ((track > 0) && (xt >= xlo) && (xt <= xhi) && (yt >= ylo) && (yt <= yhi))
  {
    xmid = xt;
    ymid = yt;
  }

  // find best overall shift
  for (dy = ylo; dy <= yhi; dy++)
    for (dx = xlo; dx <= xhi; dx++)
    {
      sc = patch_sad(dx, dy, samp, src, ref);
      if ((best < 0.0) || 
          ((sc >= 0.0) && (sc < best)) ||
          ((sc >= 0.0) && (sc == best) && 
           (d2(dx, dy, xmid, ymid) < d2(xwin, ywin, xmid, ymid))))
      {
        best = sc;
        xwin = dx;
        ywin = dy;
      }
    }

  // refine position by recomputing neighbors and fitting parabola
  if ((xwin == xlo) || (xwin == xhi))
    *xoff = -xwin;
  else
  {
    pbest = patch_sad(xwin - 1, ywin, samp, src, ref);
    fbest = patch_sad(xwin + 1, ywin, samp, src, ref);
    *xoff = -parabolic(xwin, pbest, best, fbest, xmid, dip);
  }
  xwin = -ROUND(*xoff);
  if ((ywin == ylo) || (ywin == yhi))
    *yoff = -ywin;
  else
  {
    pbest = patch_sad(xwin, ywin - 1, samp, src, ref);
    fbest = patch_sad(xwin, ywin + 1, samp, src, ref);
    *yoff = -parabolic(ywin, pbest, best, fbest, ymid, dip);
  }

  return best;
}


//= Estimates the best match for an image fragment within another image.
// first shifts in range +/- xrng to get best, then +/- yrng to refine
// only compares one pixel every samp pixels in order to go faster
// ignores ROIs of src patch and ref image: always compares all pixels
// Note: shifts returned are amounts to correct src by (negated wrt older version)
// only works with monochrome, returns the average absolute mismatch

double jhcShift::PatchCross (double *xoff, double *yoff, const jhcImg& src, const jhcImg& ref, 
                             int xlo, int xhi, int ylo, int yhi, int samp, int track) const 
{
  double sc, last, pbest, fbest, best, dip = 0.0;
  double xt, yt, xmid = 0.5 * (xlo + xhi), ymid = 0.5 * (ylo + yhi);
  int dx, dy, xwin, ywin = ROUND(ymid);

  if (!src.Valid(1) || !src.SameFormat(ref) || 
      (xlo > xhi) || (ylo > yhi) || (xoff == NULL) || (yoff == NULL))
    return Fatal("Bad inputs to jhcShift::PatchCross");

  // possibly shift preferred point for ties
  xt = -(*xoff);
  yt = -(*yoff);
  if ((track > 0) && (xt >= xlo) && (xt <= xhi) && (yt >= ylo) && (yt <= yhi))
  {
    xmid = xt;
    ymid = yt;
  }

  // find best x shift
  last = -1.0;
  best = -1.0;
  for (dx = xlo; dx <= xhi; dx++)
  {
    sc = patch_sad(dx, ywin, samp, src, ref);
    if ((best < 0.0) || 
        ((sc >= 0.0) && (sc < best)) ||
        ((sc >= 0.0) && (sc == best) && (fabs(dx - xmid) < fabs(xwin - xmid))))
    {
      best = sc;
      pbest = last;                // save previous score also
      xwin = dx;
    }
    else if (dx == (xwin + 1))
      fbest = sc;                  // save following score if last was winner
    last = sc;                     // save in case next is best
  }
  if ((xwin == xlo) || (xwin == xhi))
    *xoff = -xwin;
  else
    *xoff = -parabolic(xwin, pbest, best, fbest, xmid, dip);
  xwin = -ROUND(*xoff);

  // find best y shift
  last = -1.0;
  best = -1.0;
  for (dy = ylo; dy <= yhi; dy++)
  {
    sc = patch_sad(xwin, dy, samp, src, ref);
    if ((best < 0.0) || 
        ((sc >= 0.0) && (sc < best)) ||
        ((sc >= 0.0) && (sc == best) && (fabs(dy - ymid) < fabs(ywin - ymid))))
    {
      best = sc;
      pbest = last;                // save previous score also
      ywin = dy;
    }
    else if (dy == (ywin + 1))
      fbest = sc;                  // save following score if last was winner
    last = sc;                     // save in case next is best
  }
  if ((ywin == ylo) || (ywin == yhi))
    *yoff = -ywin;
  else
    *yoff = -parabolic(ywin, pbest, best, fbest, ymid, dip);

  return best;
}


//= Computes difference for a reference patch at the given offset.
// ignores ROIs of src patch and ref image: always compares all pixels
// returns average value of difference sampled every samp pixel on a grid

double jhcShift::patch_sad (int dx, int dy, int samp, const jhcImg& src, const jhcImg& ref) const 
{
  int x, y, x0, x1, y0, y1, sad = 0, n = 0;
  int w = src.XDim(), h = src.YDim(), ssk = samp * src.Line();
  int rw = ref.XDim(), rh = ref.YDim(), rsk = samp * ref.Line();
  const UC8 *s, *s0, *r, *r0;

  // test for at least partial patch overlap
  if (!src.Valid(1) || !ref.Valid(1) ||
      ((dx + rw) < 0) || (dx > w) || ((dy + rh) < 0) || (dy > h))
    return -1.0;  

  // adjust all offsets for overlap
  x0 = __max(0, dx);
  x1 = __min(dx + rw, w);
  y0 = __max(0, dy);
  y1 = __min(dy + rh, h);
  s0 = src.RoiSrc(x0, y0);
  r0 = ref.RoiSrc(x0 - dx, y0 - dy);

  // scan for average difference
  for (y = y0; y < y1; y += samp)
  {
    s = s0;
    r = r0;
    for (x = x0; x < x1; x += samp)
    {
      sad += abs((int)(*s) - (int)(*r));
      n++;
      s += samp;
      r += samp;
    }
    s0 += ssk;
    r0 += rsk;
  }

  // compute average value (if valid)
  if (n <= 0)
    return -1.0;
  return(sad / (double) n);
}


///////////////////////////////////////////////////////////////////////////
//                         Undoing Camera Motion                         //
///////////////////////////////////////////////////////////////////////////

//= Finds best subpixel shift to align foreground and reference.
// takes +/- ranges as input, stores actual shift values in fdx and fdy

int jhcShift::FixShift (jhcImg& dest, const jhcImg& src, const jhcImg& ref, 
                        int dx, int dy, double *fdx, double *fdy) 
{
  const jhcImg *s = &src, *r = &ref;
  int xrng = abs(dx), yrng = abs(dy), samp = 4;
  double sx, sy;

  if (!src.Valid(1, 3) || !src.SameFormat(dest) || !src.SameFormat(ref) || src.SameImg(ref))
    return Fatal("Bad inputs to jhcShift::FixShift");

  // make monochrome versions of source and reference images
  if (src.Valid(3))
  {
    tmp.SetSize(src, 1);
    get_mono(tmp, src);
    s = &tmp;
    tmp2.SetSize(ref, 1);
    get_mono(tmp2, ref);
    r = &tmp2;
  }

  // set default if good shift not found (and for pretty borders)
  dest.CopyArr(src);                         

  // determine best offset and make sure it is within bounds 
  AlignCross(&sx, &sy, *s, *r, NULL, -xrng, xrng, -yrng, yrng, samp);
  if ((fabs(sx) > (xrng - 0.5)) || (fabs(sy) > (yrng - 0.5)))
  {
    if (fdx != NULL)
      *fdx = 0.0;
    if (fdy != NULL)
      *fdy = 0.0;
    return 0;
  }
  
  // shift source image the correct amount and optionally report values
  if (src.Valid(3))
    frac_samp3(dest, src, sx, sy);  
  else
    frac_samp(dest, src, sx, sy);  
  if (fdx != NULL)
    *fdx = sx;
  if (fdy != NULL)
    *fdy = sy;
  return 1;
}


//= Generate a grayscale image from a RGB image by averaging fields.
// essentially the same as jhcGray::MonoAvg

void jhcShift::get_mono (jhcImg& dest, const jhcImg& src) const 
{
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();
  
  // apply "global" lookup table to image (sum = b + g + r)
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
}


//= Move a monochrome image over a little in X and Y and interpolates as needed.
// leaves unmapped portions of dest at their original values
// essentially the same as jhcResize::FracSamp_BW

void jhcShift::frac_samp (jhcImg& dest, const jhcImg& src, double dx, double dy) const 
{
  jhcRoi dr;
  int xlo, ylo, step = src.YDim() / dest.YDim(), sm1 = step - 1;

  // figure out reduced ROI (automatically clips to image)
  xlo = (int) floor(dx);
  ylo = (int) floor(dy);
  dr.SetRoi(src.RoiX(), src.RoiY(), src.RoiW() - 1, src.RoiH() - 1);
  dr.MoveRoi(xlo + 1, ylo + 1);
  dest.SetRoi((dr.RoiX() + sm1) / step, (dr.RoiY() + sm1) / step, 
              dr.RoiW() / step, dr.RoiH() / step);
  dr.SetRoi(dest.RoiX() * step, dest.RoiY() * step, 
            dest.RoiW() * step, dest.RoiH() * step);
  dr.MoveRoi(-xlo - 1, -ylo - 1);

  // general ROI case
  int i, x, y, fx, fy, f00, f10, f01, f11, v00 = 0, v01 = 0, v10 = 0, v11 = 0;
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip();
  int sln = src.Line(), ssk = step * sln - rw * step;
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dr), *u = s + sln;
  int xf00[256], xf01[256], xf10[256], xf11[256];

  // figure out mixing coefficients
  fx = ROUND(256.0 * (dx - xlo));
  fy = ROUND(256.0 * (dy - ylo));
  f00 = fx * fy;
  f01 = fx * (256 - fy);
  f10 = (256 - fx) * fy;
  f11 = (256 - fx) * (256 - fy);

  // build scaling arrays
  for (i = 0; i < 256; i++)
  {
    xf00[i] = v00;
    xf01[i] = v01;
    xf10[i] = v10;
    xf11[i] = v11;
    v00 += f00;
    v01 += f01;
    v10 += f10;
    v11 += f11;
  }

  // do interpolation
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d++, s += step, u += step)
      d[0] = (xf00[s[0]] + xf10[s[1]] + xf01[u[0]] + xf11[u[1]]) >> 16;
    d += dsk;
    s += ssk;
    u += ssk;
  }
}


//= Move an RGB image over a little in X and Y and interpolates as needed.
// leaves unmapped portions of dest at their original values
// essentially the same as jhcResize::FracSamp_RGB

void jhcShift::frac_samp3 (jhcImg& dest, const jhcImg& src, double dx, double dy) const 
{
  jhcRoi dr;
  int xlo, ylo, step = src.YDim() / dest.YDim(), sm1 = step - 1;

  // figure out reduced ROI (automatically clips to image)
  xlo = (int) floor(dx);
  ylo = (int) floor(dy);
  dr.SetRoi(src.RoiX(), src.RoiY(), src.RoiW() - 1, src.RoiH() - 1);
  dr.MoveRoi(xlo + 1, ylo + 1);
  dest.SetRoi((dr.RoiX() + sm1) / step, (dr.RoiY() + sm1) / step, 
              dr.RoiW() / step, dr.RoiH() / step);
  dr.SetRoi(dest.RoiX() * step, dest.RoiY() * step, 
            dest.RoiW() * step, dest.RoiH() * step);
  dr.MoveRoi(-xlo - 1, -ylo - 1);

  // general ROI case
  int i, x, y, fx, fy, f00, f10, f01, f11, v00 = 0, v01 = 0, v10 = 0, v11 = 0;
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), inc = 3 * step;
  int sln = src.Line(), ssk = step * sln - rw * inc;
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dr), *u = s + sln;
  int xf00[256], xf01[256], xf10[256], xf11[256];

  // figure out mixing coefficients
  fx = ROUND(256.0 * (dx - xlo));
  fy = ROUND(256.0 * (dy - ylo));
  f00 = fx * fy;
  f01 = fx * (256 - fy);
  f10 = (256 - fx) * fy;
  f11 = (256 - fx) * (256 - fy);

  // build scaling arrays
  for (i = 0; i < 256; i++)
  {
    xf00[i] = v00;
    xf01[i] = v01;
    xf10[i] = v10;
    xf11[i] = v11;
    v00 += f00;
    v01 += f01;
    v10 += f10;
    v11 += f11;
  }

  // do interpolation in RGB 
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d += 3, s += inc, u += inc)
    {
      d[0] = (xf00[s[0]] + xf10[s[3]] + xf01[u[0]] + xf11[u[3]]) >> 16;
      d[1] = (xf00[s[1]] + xf10[s[4]] + xf01[u[1]] + xf11[u[4]]) >> 16;
      d[2] = (xf00[s[2]] + xf10[s[5]] + xf01[u[2]] + xf11[u[5]]) >> 16;
    }
    d += dsk;
    s += ssk;
    u += ssk;
  }
}


///////////////////////////////////////////////////////////////////////////
//                            Color Enhancement                          //
///////////////////////////////////////////////////////////////////////////

//= Attempts to better align color planes of an RGB color image.
// usually not necessary or even noticeable to human eye

int jhcShift::CrispColor (jhcImg& dest, const jhcImg& src, int rng, int samp)  
{
  if (!src.Valid(3) || !src.SameFormat(dest))
    return Fatal("Bad inputs to jhcShift::CrispColor");

  // use green field as reference 
  tmp.SetSize(src, 1);
  tmp2.SetSize(tmp);
  tmp3.SetSize(tmp);
  dest.CopyArr(src);
  tmp.CopyField(src, 1, 0);

  // determine optimal red shift and merge back into destination
  tmp2.CopyField(src, 2, 0);
  AlignCross(&rdx, &rdy, tmp2, tmp, NULL, -rng, rng, -rng, rng, samp);
  if ((fabs(rdx) > (rng - 0.5)) || (fabs(rdy) > (rng - 0.5)))
  {
    rdx = 0.0;
    rdy = 0.0;
  }
  else
  {
    frac_samp(tmp3, tmp2, rdx, rdy);
    dest.CopyField(tmp3, 0, 2);
  }
  
  // determine optimal blue shift and merge back into destination
  tmp2.CopyField(src, 0, 0);
  AlignCross(&bdx, &bdy, tmp2, tmp, NULL, -rng, rng, -rng, rng, samp);
  if ((fabs(bdx) > (rng - 0.5)) || (fabs(bdy) > (rng - 0.5)))
  {
    bdx = 0.0;
    bdy = 0.0;
  }
  else
  {
    frac_samp(tmp3, tmp2, bdx, bdy);
    dest.CopyField(tmp3, 0, 0);
  }
  return 1;
}


