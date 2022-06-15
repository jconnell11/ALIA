// jhcRuns.cpp : processes images using four directions of run lengths
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#include "Processing/jhcRuns.h"


///////////////////////////////////////////////////////////////////////////
//                             Run Lengths                               //
///////////////////////////////////////////////////////////////////////////

//= Computes length of continuous horizontal runs of non-zero PxlSrc().
// writes whole strip with length found (possibly times a scale factor)
// adjusts for pixel aspect ratio based on source 
// can discard runs touching image borders (set bdok = 0)

int jhcRuns::RunsH (jhcImg& dest, const jhcImg& src, double sc, int bdok) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::RunsH");
  if (sc <= 0.0)
    return 0;
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, n, valid;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  double f = sc * src.Ratio();
  UC8 val;
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // scan image horizontal line by line
  for (y = rh; y > 0; y--)
  {
    n = 0;
    valid = bdok;
    for (x = rw; x > 0; x--)
    {
      if (*s++ > 0)              // inside a run
        n++;
      else                       
      { 
        if (n > 0)               // run just ended
        {
          val = 0;
          if (valid > 0)
            val = BOUND(f * n);
          while (n-- > 0)        // record length over strip
            *d++ = val;
          n = 0;
        }
        valid = 1;
        *d++ = 0;                // outside a run so write zero
      }
    }
    if (n > 0)                   // run continues to end of region
    {
      val = 0;
      if (bdok > 0)
        val = BOUND(f * n);
      while (n-- > 0)            // record length over strip
        *d++ = val;
    }
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Computes length of continuous vertical runs of non-zero PxlSrc().
// writes whole strip with length found (possibly times a scale factor)
// NOTE: source and destination images must be different
// about 25% slower than RunsH for violating scan order
// can discard runs touching image borders (set bdok = 0)

int jhcRuns::RunsV (jhcImg& dest, const jhcImg& src, double sc, int bdok) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcRuns::RunsV");
  if (sc <= 0.0)
    return 0;
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, n, valid, line = dest.Line(), rw = dest.RoiW(), rh = dest.RoiH();
  UC8 val;
  UC8 *d, *d0 = dest.RoiDest();
  const UC8 *s, *s0 = src.RoiSrc();

  // scan image vertically column by column
  for (x = rw; x > 0; x--)
  {
    n = 0;
    valid = bdok;
    s = s0++;                    // start at bottom of next column
    d = d0++;
    for (y = rh; y > 0; y--)
    {
      if (*s > 0)                // inside a run
        n++;
      else 
      {
        if (n > 0)               // run just ended
        {
          val = 0;
          if (valid > 0)
            val = BOUND(sc * n);
          while (n-- > 0)        // record length over strip
          {
            *d = val;
            d += line;
          } 
          n = 0;
        }
        valid = 1;
        *d = 0;                  // now outside run so write zero
        d += line;               
      }
      s += line;
    }
    if (n > 0)                   // run continues to end of region
    {
      val = 0;
      if (bdok > 0)
        val = BOUND(sc * n);
      while (n-- > 0)            // record length over strip
      {
        *d = val;
        d += line;
      }
    }
  }
  return 1;
}


//= Computes length of continuous NW-to-SE diagonal runs of non-zero PxlSrc().
// writes whole strip with length found (possibly times a scale factor)
// NOTE: source and destination images must be different
// about 25% slower than RunsH for violating scan order
// can discard runs touching image borders (set bdok = 0)

int jhcRuns::RunsD1 (jhcImg& dest, const jhcImg& src, double sc, int bdok) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcRuns::RunsD1");
  if (sc <= 0.0)
    return 0;
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, n, valid, line = dest.Line(), step = line - 1;
  int rw = dest.RoiW(), rh = dest.RoiH();
  UC8 val;
  double f = sc * sqrt(1.0 + src.Ratio());
  UC8 *d, *di, *d0 = dest.RoiDest() + rw - 1;
  const UC8 *s, *si, *s0 = src.RoiSrc() + rw - 1;

  // scan bottom half of image (SW corner)
  di = d0;
  si = s0;
  for (x = rw; x > 0; x--)
  {
    n = 0;
    valid = bdok;
    s = si--;                    // start at bottom of next diagonal
    d = di--;
    for (y = __min(rh, x); y > 0; y--)
    {
      if (*s > 0)                // inside a run
        n++;
      else 
      {
        if (n > 0)               // run just ended
        {
          val = 0;
          if (valid > 0)
            val = BOUND(f * n);
          while (n-- > 0)        // record length over strip
          {
            *d = val;
            d += step;
          }
          n = 0;
        }
        valid = 1;
        *d = 0;                  // now outside run so write zero
        d += step;               
      }
      s += step;
    }
    if (n > 0)                   // run continues to end of region
    {
      val = 0;
      if (bdok > 0)
        val = BOUND(f * n);
      while (n-- > 0)            // record length over strip
      {
        *d = val;
        d += step;
      }
    }
  }

  // scan top half of image (NE corner)
  di = d0 + line;
  si = s0 + line;
  for (y = rh - 1; y > 0; y--)
  {
    n = 0;
    valid = bdok;
    s = si;                    // start at bottom of next diagonal
    d = di;
    si += line;
    di += line;
    for (x = __min(rw, y); x > 0; x--)
    {
      if (*s > 0)                // inside a run
        n++;
      else 
      {
        if (n > 0)               // run just ended
        {
          val = 0;
          if (valid > 0)
            val = BOUND(f * n);
          while (n-- > 0)        // record length over strip
          {
            *d = val;
            d += step;
          }
          n = 0;
        }
        valid = 1;
        *d = 0;                  // now outside run so write zero
        d += step;               
      }
      s += step;
    }
    if (n > 0)                   // run continues to end of region
    {
      val = 0;
      if (bdok > 0)
        val = BOUND(f * n);
      while (n-- > 0)            // record length over strip
      {
        *d = val;
        d += step;
      }
    }
  }
  return 1;
}


//= Computes length of continuous NE-to-SW diagonal runs of non-zero PxlSrc().
// writes whole strip with length found (possibly times a scale factor)
// NOTE: source and destination images must be different
// about 25% slower than RunsH for violating scan order
// can discard runs touching image borders (set bdok = 0)

int jhcRuns::RunsD2 (jhcImg& dest, const jhcImg& src, double sc, int bdok) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcRuns::RunsD2");
  if (sc <= 0.0)
    return 0;
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, n, valid, line = dest.Line(), step = line + 1;
  int rw = dest.RoiW(), rh = dest.RoiH();
  UC8 val;
  double f = sc * sqrt(1.0 + src.Ratio());
  UC8 *d, *di, *d0 = dest.RoiDest();
  const UC8 *s, *si, *s0 = src.RoiSrc();

  // scan bottom half of image (SE corner)
  di = d0;
  si = s0;
  for (x = rw; x > 0; x--)
  {
    n = 0;
    valid = bdok;
    s = si++;                    // start at bottom of next diagonal
    d = di++;
    for (y = __min(rh, x); y > 0; y--)
    {
      if (*s > 0)                // inside a run
        n++;
      else 
      {
        if (n > 0)               // run just ended
        {
          val = 0;
          if (valid > 0)
            val = BOUND(f * n);
          while (n-- > 0)        // record length over strip
          {
            *d = val;
            d += step;
          }
          n = 0;
        }
        valid = 1;
        *d = 0;                  // now outside run so write zero
        d += step;               
      }
      s += step;
    }
    if (n > 0)                   // run continues to end of region
    {
      val = 0;
      if (bdok > 0)
        val = BOUND(f * n);
      while (n-- > 0)            // record length over strip
      {
        *d = val;
        d += step;
      }
    }
  }

  // scan top half of image (NW corner)
  di = d0 + line;
  si = s0 + line;
  for (y = rh - 1; y > 0; y--)
  {
    n = 0;
    valid = bdok;
    s = si;                    // start at bottom of next diagonal
    d = di;
    si += line;
    di += line;
    for (x = __min(rw, y); x > 0; x--)
    {
      if (*s > 0)                // inside a run
        n++;
      else 
      {
        if (n > 0)               // run just ended
        {
          val = 0;
          if (valid > 0)
            val = BOUND(f * n);
          while (n-- > 0)        // record length over strip
          {
            *d = val;
            d += step;
          }
          n = 0;
        }
        valid = 1;
        *d = 0;                  // now outside run so write zero
        d += step;               
      }
      s += step;
    }
    if (n > 0)                   // run continues to end of region
    {
      val = 0;
      if (bdok > 0)
        val = BOUND(f * n);
      while (n-- > 0)            // record length over strip
      {
        *d = val;
        d += step;
      }
    }
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Shape Properties                             //
///////////////////////////////////////////////////////////////////////////

//= Finds the minumum run length in all directions.

int jhcRuns::MinRun (jhcImg& dest, const jhcImg& src, double sc)  
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::MinRun");

  a1.SetSize(dest);
  b1.SetSize(dest);
  RunsH(b1, src, sc);
  RunsV(a1, src, sc);
  nzm(b1, b1, a1);
  RunsD1(a1, src, sc);
  nzm(b1, b1, a1);
  RunsD2(a1, src, sc);
  nzm(dest, b1, a1);
  return 1;
}


//= Combine two images to give pairwise minimum at every pixel.
// only considers PxlSrc() which have values greater than 0
// modified copy of jhcALU::NZMin

void jhcRuns::nzm (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const 
{
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);

  // compare PxlSrc() in ROI
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      if ((*b == 0) || ((*a != 0) && (*a < *b)))
        *d++ = *a;
      else 
        *d++ = *b;
      a++;
      b++;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
}


//= Finds the direction of the minumum run length.
// 0 = BG, 64 = D1, 128 = H, 192 = D2, 255 = V

int jhcRuns::MinDir (jhcImg& dest, const jhcImg& src, double sc) 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::MinDir");

  a1.SetSize(dest);
  b1.SetSize(dest);
  thresh(dest, src, 1, 64, 0);     // dest holds winner
  RunsD2(b1, src, sc);             // b1 holds minimum distance
  RunsV(a1, src, sc);
  min_mark(dest, b1, a1, 128);
  RunsD1(a1, src, sc);
  min_mark(dest, b1, a1, 192);
  RunsH(a1, src, sc);
  min_mark(dest, b1, a1, 255);
  return 1;
}


//= Marks output pixel as some value if width is less than minimum recorded.
// also updates the minimum to be new value if changed

void jhcRuns::min_mark (jhcImg& mark, jhcImg& narr, const jhcImg& wid, int val) const 
{
  narr.MergeRoi(mark);
  narr.MergeRoi(wid);
  mark.CopyRoi(narr);

  // general ROI case
  int x, y, rw = mark.RoiW(), rh = mark.RoiH(), rsk = mark.RoiSkip();
  UC8 bval = BOUND(val);
  const UC8 *s = wid.RoiSrc(mark);
  UC8 *d = mark.RoiDest(), *b = narr.RoiDest();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d++, b++, s++)
      if (*s < *b)
      {
        *b = *s;
        *d = bval;
      }
    d += rsk;
    b += rsk;
    s += rsk;
  }
}


//= Finds the minimum width based on two adjacent run directions.
// reports the bigger of the two run directions involved
// this is probably the best for pruning off spurs in binary masks

int jhcRuns::AdjMin (jhcImg& dest, const jhcImg& src, double sc)  
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::AdjMin");  

  // get runs in all directions
  dest.CopyRoi(src);
  a1.SetSize(dest);
  b1.SetSize(dest);
  c1.SetSize(dest);
  d1.SetSize(dest);
  RunsH(a1, src, sc);
  RunsV(b1, src, sc);
  RunsD2(c1, src, sc);
  RunsD1(d1, src, sc);

  // general ROI case
  int x, y, hd1, d1v, vd2, d2h, out;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(), *h = a1.RoiSrc(), *d = d1.RoiSrc();
  const UC8 *v = b1.RoiSrc(), *d2 = c1.RoiSrc();
  UC8 *m = dest.RoiDest();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, m++, s++, h++, d++, v++, d2++)
    {
      // ignore the background
      if (*s == 0)
      {
        *m = 0;
        continue;
      }

      // find the maximum of adjacent pairs
      hd1 = __max(*h, *d);
      d1v = __max(*d, *v);
      vd2 = __max(*v, *d2);
      d2h = __max(*d2, *h);

      // pick the smallest combination
      out = __min(hd1, d1v);
      out = __min(out, vd2);
      out = __min(out, d2h);
      *m = (UC8) out;
    }
    m  += rsk;
    s  += rsk;
    h  += rsk;
    d  += rsk;
    v  += rsk;
    d2 += rsk;
  }
  return 1;
}


//= Finds the minimum width based on two orthogonal run directions.
// reports the bigger of the two run directions involved

int jhcRuns::OrthoMin (jhcImg& dest, const jhcImg& src, double sc)  
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::OrthoMin");

  // get runs in all directions
  dest.CopyRoi(src);
  a1.SetSize(dest);
  b1.SetSize(dest);
  c1.SetSize(dest);
  d1.SetSize(dest);
  RunsH(a1, src, sc);
  RunsV(b1, src, sc);
  RunsD2(c1, src, sc);
  RunsD1(d1, src, sc);

  // general ROI case
  int x, y, hv, d1d2, out;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(), *h = a1.RoiSrc(), *d = d1.RoiSrc();
  const UC8 *v = b1.RoiSrc(), *d2 = c1.RoiSrc();
  UC8 *m = dest.RoiDest();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, m++, s++, h++, d++, v++, d2++)
    {
      // ignore the background
      if (*s == 0)
      {
        *m = 0;
        continue;
      }

      // find the maximum of orthogonal pairs then pick smallest
      hv   = __max(*h, *v);
      d1d2 = __max(*d, *d2);
      out = __min(hv, d1d2);
      *m = (UC8) out;
    }
    m  += rsk;
    s  += rsk;
    h  += rsk;
    d  += rsk;
    v  += rsk;
    d2 += rsk;
  }
  return 1;
}


//= Finds the minimum width based on two adjacent run directions.
// selects pair based on the maximum value in pair 
// reports the average of the two run directions involved

int jhcRuns::AdjAvg (jhcImg& dest, const jhcImg& src, double sc) 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::AdjAvg");  

  // get runs in all directions
  dest.CopyRoi(src);
  a1.SetSize(dest);
  b1.SetSize(dest);
  c1.SetSize(dest);
  d1.SetSize(dest);
  RunsH(a1, src, sc);
  RunsV(b1, src, sc);
  RunsD2(c1, src, sc);
  RunsD1(d1, src, sc);

  // general ROI case
  int x, y, hd1, d1v, vd2, d2h, out;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(), *h = a1.RoiSrc(), *d = d1.RoiSrc();
  const UC8 *v = b1.RoiSrc(), *d2 = c1.RoiSrc();
  UC8 *m = dest.RoiDest();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, m++, s++, h++, d++, v++, d2++)
    {
      // ignore the background
      if (*s == 0)
      {
        *m = 0;
        continue;
      }

      // find the maximum of adjacent pairs
      hd1 = __max(*h, *d);
      d1v = __max(*d, *v);
      vd2 = __max(*v, *d2);
      d2h = __max(*d2, *h);

      // pick the smallest combination then compute average
      // does not handle ties very intelligently
      out = __min(hd1, d1v);
      out = __min(out, vd2);
      out = __min(out, d2h);
      if (out == hd1)
        out = (*h + *d) >> 1;        
      else if (out == d1v)
        out = (*d + *v) >> 1;        
      else if (out == vd2)
        out = (*v + *d2) >> 1;        
      else
        out = (*d2 + *h) >> 1;        
      *m = (UC8) out;
    }
    m  += rsk;
    s  += rsk;
    h  += rsk;
    d  += rsk;
    v  += rsk;
    d2 += rsk;
  }
  return 1;
}


//= Finds the minimum width based on two orthogonal run directions.
// selects pair based on the maximum value in pair 
// reports the bigger of the two run directions involved

int jhcRuns::OrthoAvg (jhcImg& dest, const jhcImg& src, double sc) 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::OrthoAvg");

  // get runs in all directions
  dest.CopyRoi(src);
  a1.SetSize(dest);
  b1.SetSize(dest);
  c1.SetSize(dest);
  d1.SetSize(dest);
  RunsH(a1, src, sc);
  RunsV(b1, src, sc);
  RunsD2(c1, src, sc);
  RunsD1(d1, src, sc);

  // general ROI case
  int x, y, hv, d1d2, out;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc(), *h = a1.RoiSrc(), *d = d1.RoiSrc();
  const UC8 *v = b1.RoiSrc(), *d2 = c1.RoiSrc();
  UC8 *m = dest.RoiDest();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, m++, s++, h++, d++, v++, d2++)
    {
      // ignore the background
      if (*s == 0)
      {
        *m = 0;
        continue;
      }

      // find the maximum of orthogonal pairs
      hv   = __max(*h, *v);
      d1d2 = __max(*d, *d2);

      // report the average of the smallest pair
      // does not handle ties very intelligently
      if (hv < d1d2)
        out = (*h + *v) >> 1;
      else
        out = (*d + *d2) >> 1;
      *m = (UC8) out;
    }
    m  += rsk;
    s  += rsk;
    h  += rsk;
    d  += rsk;
    v  += rsk;
    d2 += rsk;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Region Bulking                                //
///////////////////////////////////////////////////////////////////////////

//= Closes foreground by filling interior runs in 4 directions.
// like a local convex hull - closes "bays" in contour
// will only do up to maxgap pixels (zero = any length)
// does not close between object and image boundary
// probably the most useful function in this class

int jhcRuns::Convexify (jhcImg& dest, const jhcImg& src, int maxgap) 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::Convexify");

  a1.SetSize(dest);
  b1.SetSize(dest);
  inv(a1, src);

  RunsH(b1, a1, 1.0, 0);
  cutoff(b1, maxgap);
  nzor(dest, src, b1);

  RunsV(b1, a1, 1.0, 0);
  cutoff(b1, maxgap);
  nzor(dest, dest, b1);

  RunsD1(b1, a1, 1.0, 0);
  cutoff(b1, maxgap);
  nzor(dest, dest, b1);

  RunsD2(b1, a1, 1.0, 0);
  cutoff(b1, maxgap);
  nzor(dest, dest, b1);

  return 1;
}


//= Invert image (0 swapped with 255).

void jhcRuns::inv (jhcImg& dest, const jhcImg& src) const 
{
  dest.CopyRoi(src);
  
  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // compare PxlSrc() in ROI
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      if (*s++ > 128) 
        *d++ = 0;
      else
        *d++ = 255;
    d += rsk;
    s += rsk;
  }
}


//= Zeroes pixels which have value greater than maxval.

void jhcRuns::cutoff (jhcImg& dest, int maxval) const 
{
  if (maxval <= 0)
    return;
 
  // general ROI case
  int x, y, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();

  // check all pixels
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--, d++)
      if (*d > maxval) 
        *d = 0;
    d += rsk;
  }
}


//= Combine two images using OR.
// give 255 at any pixel for which either is non-zero

void jhcRuns::nzor (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const 
{
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);

  // compare PxlSrc() in ROI
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
    {
      if ((*a > 0) || (*b > 0))
        *d++ = 255;
      else
        *d++ = 0;
      a++;
      b++;
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
}


//= Mutated version of standard threshold function.
// can do thresholding in-place, uses under value if less than th

void jhcRuns::thresh (jhcImg& dest, const jhcImg& src, int th, int over, int under) const 
{
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i;
  UC8 val = BOUND(th);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();
  UC8 ans[256];

  // create table for all answers
  for (i = 0; i < val; i++)
    ans[i] = (UC8) under;
  for (i = val; i <= 255; i++)
    ans[i] = (UC8) over;

  // apply table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--, d++, s++)
      *d = ans[*s];
    d += rsk;
    s += rsk;
  }
}


//= Simultaneously fill in short gaps in all connected components in 8 bit image.
// maxgap = 0 will fill any length, can still leave polygonal holes in objects
// use Fill8 and Fill16 to convert back and forth from 16 bit images

int jhcRuns::ConvexAll (jhcImg& dest, const jhcImg& src, int maxgap)
{
  c1.InitSize(dest, 255);
  return StripOutside(dest, c1, src, maxgap, 0);
}


//= Fill in short gaps in source where runs only pass through allowed gate regions.
// maxgap = 0 will fill any length, can still leave polygonal holes in source region
// can handle multiple components in source (i.e. pixel label matters, not all 255)
// no runs will pass through regions where gate is zero (but any positive value is okay)

int jhcRuns::ConvexClaim (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int maxgap)
{
  return StripOutside(dest, gate, src, maxgap, 0);
}


//= Fill in all horizontal gaps of less than maxgap wide.

int jhcRuns::ConvexH (jhcImg& dest, const jhcImg& src, int maxgap, int fill)
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::ConvexH");
  dest.CopyArr(src);

  // generic ROI case
  int x, y, n, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 fval = BOUND(fill);
  UC8 *d = dest.RoiDest();

  // scan image horizontal line by line
  for (y = rh; y > 0; y--, d += rsk)
  {
    n = -1;                                // initially unanchored
    for (x = rw; x > 0; x--, d++)
      if (*d > 0)                            
      {
        // see if just hit opposite side
        if ((n > 0) && (n <= maxgap))   
          while (n > 0)                    // back fill missing part
            d[-(n--)] = fval;                   
        n = 0;                             // mark as anchored    
      }
      else if (n >= 0)
        n++;                               // outside mask so measure
  }
  return 1;
}


//= Fill in all upward facing horizontal gaps of less than maxgap wide.
// like ConvexH but requires any filled run to be completely filled below
// useful for smoothing over occulsion shadows in depth projections

int jhcRuns::ConvexUp (jhcImg& dest, const jhcImg& src, int maxgap, int fill)
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::ConvexUp");
  dest.CopyArr(src);

  // generic ROI case
  int x, y, n, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip(), ln = dest.Line();
  UC8 fval = BOUND(fill);
  UC8 *d = dest.RoiDest();

  // scan image horizontal line by line
  for (y = rh; y > 0; y--, d += rsk)
  {
    n = -1;                                // initially unanchored
    for (x = rw; x > 0; x--, d++)
      if (*d > 0)                            
      {
        // see if just hit opposite side
        if ((n > 0) && (n <= maxgap))   
          while (n > 0)                    // back fill missing part
            d[-(n--)] = fval;                   
        n = 0;                             // mark as anchored    
      }
      else if (n >= 0)
        if ((y == rh) || (d[-ln] == 0))
          n = -1;                          // unsupported below
        else
          n++;                             // outside mask so measure
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Edge Filling                                //
///////////////////////////////////////////////////////////////////////////

//= Removes pixels of source unless run anchored by same label in bounds.
// destination pixels copy boundary labels not source pixels
// useful for removing shadows from difference images using edges
// can also be used to convexify 8-bit connected component images
// note that edges can gate component image to give labelled edges
// source and destination can be the same image

int jhcRuns::StripOutside (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun, int bdok)  
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(bnd))
    return Fatal("Bad images to jhcRuns::TrimOutside");

  // set up temporary images
  a1.SetSize(dest);
  b1.SetSize(dest);

  // keep pixel if bounded in any of the four directions
  KeepSpanH(a1, src, bnd, mrun, bdok);
  KeepSpanV(b1, src, bnd, mrun, bdok);
  nzm(a1, a1, b1);
  KeepSpanD1(b1, src, bnd, mrun, bdok);
  nzm(a1, a1, b1);
  KeepSpanD2(b1, src, bnd, mrun, bdok);
  nzm(dest, a1, b1);
  return 1;
}


//= Marks areas inside src than are bounded by edges in bnd.
// marks each region with the number of runs (x 64) passing through
// can alternately binarize above a number of runs if cnt is non-zero 
// source and destination can be the same image

int jhcRuns::InsideRuns (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun, int bdok, int cnt) 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(bnd))
    return Fatal("Bad images to jhcRuns::InsideRuns");

  // set up temporary images
  a1.SetSize(dest);
  b1.SetSize(dest);

  // keep pixel if bounded in any of the four directions
  KeepSpanH(a1, src, bnd, mrun, bdok);
  thresh(a1, a1, 1, 64, 0);
  KeepSpanV(b1, src, bnd, mrun, bdok);
  th_sum(a1, a1, b1, 1, 64);
  KeepSpanD1(b1, src, bnd, mrun, bdok);
  th_sum(a1, a1, b1, 1, 64);
  KeepSpanD2(b1, src, bnd, mrun, bdok);
  th_sum(dest, a1, b1, 1, 64);
  
  // see if final result should be binarized
  if (cnt > 0)
    thresh(dest, dest, 64 * cnt - 32, 255, 0);
  return 1;
}


//= Mutated version of jhcALU::ClipSum which thresholds its second argument.

void jhcRuns::th_sum (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, int bth, int mark) const 
{
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y, i, level = BOUND(bth), val = BOUND(mark);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);
  UC8 *d = dest.RoiDest();
  UC8 ans[256];

  // create table for all answers
  for (i = 0; i < level; i++)
    ans[i] = 0;
  for (i = level; i <= 255; i++)
    ans[i] = (UC8) val;

  // apply table to image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--, d++, a++, b++)
    {
      i = *a + ans[*b];
      *d = BOUND(i);
    }
    d += rsk;
    a += rsk;
    b += rsk;
  }
}


//= Keep horizontal runs anchored by same label in bounds.
// destination pixels copy boundary labels not source pixels
// can optionally interpret image borders as valid edges
// <pre>
//    B: 0 0 0 7 7 0 0 0 0 7 7 7 0 0 0 0
//    S: 0 0 1 1 1 1 1 1 1 1 1 1 1 1 1 0
//
//       | | | | | | | | | | | | | | | |
//       v v v v v v v v v v v v v v v v 
//
//    D: 0 0 0 7 7 7 7 7 7 7 7 7 0 0 0 0      
// </pre>

int jhcRuns::KeepSpanH (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun, int bdok) const 
{
  if (!dest.Valid(1) || dest.SameImg(src) || dest.SameImg(bnd) ||
      !dest.SameFormat(src) || !dest.SameFormat(bnd))
    return Fatal("Bad images to jhcRuns::KeepSpanH");
  dest.CopyRoi(src);
  dest.MergeRoi(bnd);

  // generic ROI case
  int x, y, n, mark, m0 = ((bdok > 0) ? -1 : 0);
  int rw = dest.RoiW(), rh = dest.RoiH(), sk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest), *b = bnd.RoiSrc(dest);

  // scan image horizontal line by line
  for (y = rh; y > 0; y--)
  {
    mark = m0;
    n = 0;
    for (x = rw; x > 0; x--, s++, b++)
    {
      if ((*s == 0) ||           // Case 1: no inter inside a source object
          ((mrun > 0) && (n > mrun)))
      {
        while (n-- > 0)          // erase any speculative run
          *d++ = 0;
        *d++ = 0;                // erase current pixel
        mark = 0;
        n = 0;
      }
      else if (*b > 0)           // Case 2: encountered an edge
      {
        if ((*b == mark) || (mark < 0))
          while (n-- > 0)        // if matched run in progress, mark all 
            *d++ = *b;
        else
          while (n-- > 0)        // else erase speculative run
            *d++ = 0;
        *d++ = *b;               // copy boundary to current pixel
        mark = *b;
        n = 0;
      }
      else                       // Case 3: middle of run
        n++;
    }

    // Case 4: image boundary reached
    if ((bdok > 0) && (mark > 0))
      while (n-- > 0)            // validate any run at image edge
        *d++ = (UC8) mark;
    else
      while (n-- > 0)            // erase any unbounded run at image edge
        *d++ = 0;
    d += sk;
    s += sk;
    b += sk;
  }
  return 1;
}


//= Keep vertical runs anchored by same label in bounds.
// destination pixels copy boundary labels not source pixels

int jhcRuns::KeepSpanV (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun, int bdok) const 
{
  if (!dest.Valid(1) || dest.SameImg(src) || dest.SameImg(bnd) ||
      !dest.SameFormat(src) || !dest.SameFormat(bnd))
    return Fatal("Bad images to jhcRuns::KeepSpanV");
  dest.CopyRoi(src);
  dest.MergeRoi(bnd);

  // generic ROI case
  int x, y, n, mark, m0 = ((bdok > 0) ? -1 : 0);
  int rw = dest.RoiW(), rh = dest.RoiH(), ln = dest.Line(), sk = rh * ln - 1;
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest), *b = bnd.RoiSrc(dest);

  // scan image vertical column by column
  for (x = rw; x > 0; x--)
  {
    mark = m0;
    n = 0;
    for (y = rh; y > 0; y--, s += ln, b += ln)
    {
      if ((*s == 0) ||           // Case 1: no inter inside a source object
          ((mrun > 0) && (n > mrun)))
      {
        while (n-- > 0)          // erase any speculative run
        {
          *d = 0;
          d += ln;
        }
        *d = 0;                  // erase current pixel
        d += ln;
        mark = 0;
        n = 0;
      }
      else if (*b > 0)           // Case 2: encountered an edge
      {
        if ((*b == mark) || (mark < 0))
          while (n-- > 0)        // if matched run in progress, mark all 
          {
            *d = *b;
            d += ln;
          }
        else
          while (n-- > 0)        // else erase speculative run
          {
            *d = 0;
            d += ln;
          }
        *d = *b;                 // copy boundary to current pixel
        d += ln;
        mark = *b;
        n = 0;
      }
      else                       // Case 3: middle of run
        n++;
    }

    // Case 4: image boundary reached
    if ((bdok > 0) && (mark > 0))
      while (n-- > 0)            // validate any run at image edge
      {
        *d = (UC8) mark;
        d += ln;
      }
    else
      while (n-- > 0)            // erase any unbounded run at image edge
      {
        *d = 0;
        d += ln;
      }
    d -= sk;
    s -= sk;
    b -= sk;
  }
  return 1;
}



//= Keep major diagonal runs anchored by same label in bounds.
// destination pixels copy boundary labels not source pixels

int jhcRuns::KeepSpanD1 (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun, int bdok) const 
{
  if (!dest.Valid(1) || dest.SameImg(src) || dest.SameImg(bnd) ||
      !dest.SameFormat(src) || !dest.SameFormat(bnd))
    return Fatal("Bad images to jhcRuns::KeepSpanD1");
  dest.CopyRoi(src);
  dest.MergeRoi(bnd);

  // generic ROI case
  int x, y, n, mark, m0 = ((bdok > 0) ? -1 : 0);
  int line = dest.Line(), step = line - 1, rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *di, *d0 = dest.RoiDest() + rw - 1;
  const UC8 *s, *si, *s0 = src.RoiSrc(dest) + rw - 1;
  const UC8 *b, *bi, *b0 = bnd.RoiSrc(dest) + rw - 1;

  // scan bottom half of image (SW corner)
  di = d0;
  si = s0;
  bi = b0;
  for (x = rw; x > 0; x--)
  {
    mark = m0;
    n = 0;
    s = si--;                    // start at bottom of next diagonal
    b = bi--;
    d = di--;
    for (y = __min(rh, x); y > 0; y--, s += step, b += step)
    {
      if ((*s == 0) ||           // Case 1: no inter inside a source object
          ((mrun > 0) && (n > mrun)))
      {
        while (n-- > 0)          // erase any speculative run
        {
          *d = 0;
          d += step;
        }
        *d = 0;                  // erase current pixel
        d += step;
        mark = 0;
        n = 0;
      }
      else if (*b > 0)           // Case 2: encountered an edge
      {
        if ((*b == mark) || (mark < 0))
          while (n-- > 0)        // if matched run in progress, mark all 
          {
            *d = *b;
            d += step;
          }
        else
          while (n-- > 0)        // else erase speculative run
          {
            *d = 0;
            d += step;
          }
        *d = *b;                 // copy boundary to current pixel
        d += step;
        mark = *b;
        n = 0;
      }
      else                       // Case 3: middle of run
        n++;
    }

    // Case 4: image boundary reached
    if ((bdok > 0) && (mark > 0))
      while (n-- > 0)            // validate any run at image edge
      {
        *d = (UC8) mark;
        d += step;
      }
    else
      while (n-- > 0)            // erase any unbounded run at image edge
      {
        *d = 0;
        d += step;
      }
  }

  // scan top half of image (NE corner)
  di = d0 + line;
  si = s0 + line;
  bi = b0 + line;
  for (y = rh - 1; y > 0; y--)
  {
    mark = m0;
    n = 0;
    s = si;                    // start at bottom of next diagonal
    b = bi;
    d = di;
    si += line;
    bi += line;
    di += line;
    for (x = __min(rw, y); x > 0; x--, s += step, b += step)
    {
      if ((*s == 0) ||           // Case 1: no inter inside a source object
          ((mrun > 0) && (n > mrun)))
      {
        while (n-- > 0)          // erase any speculative run
        {
          *d = 0;
          d += step;
        }
        *d = 0;                  // erase current pixel
        d += step;
        mark = 0;
        n = 0;
      }
      else if (*b > 0)           // Case 2: encountered an edge
      {
        if ((*b == mark) || (mark < 0))
          while (n-- > 0)        // if matched run in progress, mark all 
          {
            *d = *b;
            d += step;
          }
        else
          while (n-- > 0)        // else erase speculative run
          {
            *d = 0;
            d += step;
          }
        *d = *b;                 // copy boundary to current pixel
        d += step;
        mark = *b;
        n = 0;
      }
      else                       // Case 3: middle of run
        n++;
    }

    // Case 4: image boundary reached
    if ((bdok > 0) && (mark > 0))
      while (n-- > 0)            // validate any run at image edge
      {
        *d = (UC8) mark;
        d += step;
      }
    else
      while (n-- > 0)            // erase any unbounded run at image edge
      {
        *d = 0;
        d += step;
      }
  }
  return 1;
}


//= Keep minor diagonal runs anchored by same label in bounds.
// destination pixels copy boundary labels not source pixels

int jhcRuns::KeepSpanD2 (jhcImg& dest, const jhcImg& src, const jhcImg& bnd, int mrun, int bdok) const 
{
  if (!dest.Valid(1) || dest.SameImg(src) || dest.SameImg(bnd) ||
      !dest.SameFormat(src) || !dest.SameFormat(bnd))
    return Fatal("Bad images to jhcRuns::KeepSpanD2");
  dest.CopyRoi(src);
  dest.MergeRoi(bnd);

  // generic ROI case
  int x, y, n, mark, m0 = ((bdok > 0) ? -1 : 0);
  int line = dest.Line(), step = line + 1, rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *di, *d0 = dest.RoiDest();
  const UC8 *s, *si, *s0 = src.RoiSrc(dest);
  const UC8 *b, *bi, *b0 = bnd.RoiSrc(dest);

  // scan bottom half of image (SE corner)
  di = d0;
  si = s0;
  bi = b0;
  for (x = rw; x > 0; x--)
  {
    mark = m0;
    n = 0;
    s = si++;                    // start at bottom of next diagonal
    b = bi++;
    d = di++;
    for (y = __min(rh, x); y > 0; y--, s += step, b += step)
    {
      if ((*s == 0) ||           // Case 1: no inter inside a source object
          ((mrun > 0) && (n > mrun)))
      {
        while (n-- > 0)          // erase any speculative run
        {
          *d = 0;
          d += step;
        }
        *d = 0;                  // erase current pixel
        d += step;
        mark = 0;
        n = 0;
      }
      else if (*b > 0)           // Case 2: encountered an edge
      {
        if ((*b == mark) || (mark < 0))
          while (n-- > 0)        // if matched run in progress, mark all 
          {
            *d = *b;
            d += step;
          }
        else
          while (n-- > 0)        // else erase speculative run
          {
            *d = 0;
            d += step;
          }
        *d = *b;                 // copy boundary to current pixel
        d += step;
        mark = *b;
        n = 0;
      }
      else                       // Case 3: middle of run
        n++;
    }

    // Case 4: image boundary reached
    if ((bdok > 0) && (mark > 0))
      while (n-- > 0)            // validate any run at image edge
      {
        *d = (UC8) mark;
        d += step;
      }
    else
      while (n-- > 0)            // erase any unbounded run at image edge
      {
        *d = 0;
        d += step;
      }
  }

  // scan top half of image (NW corner)
  di = d0 + line;
  si = s0 + line;
  bi = b0 + line;
  for (y = rh - 1; y > 0; y--)
  {
    mark = m0;
    n = 0;
    s = si;                    // start at bottom of next diagonal
    b = bi;
    d = di;
    si += line;
    bi += line;
    di += line;
    for (x = __min(rw, y); x > 0; x--, s += step, b += step)
    {
      if ((*s == 0) ||           // Case 1: no inter inside a source object
          ((mrun > 0) && (n > mrun)))
      {
        while (n-- > 0)          // erase any speculative run
        {
          *d = 0;
          d += step;
        }
        *d = 0;                  // erase current pixel
        d += step;
        mark = 0;
        n = 0;
      }
      else if (*b > 0)           // Case 2: encountered an edge
      {
        if ((*b == mark) || (mark < 0))
          while (n-- > 0)        // if matched run in progress, mark all 
          {
            *d = *b;
            d += step;
          }
        else
          while (n-- > 0)        // else erase speculative run
          {
            *d = 0;
            d += step;
          }
        *d = *b;                 // copy boundary to current pixel
        d += step;
        mark = *b;
        n = 0;
      }
      else                       // Case 3: middle of run
        n++;
    }

    // Case 4: image boundary reached
    if ((bdok > 0) && (mark > 0))
      while (n-- > 0)            // validate any run at image edge
      {
        *d = (UC8) mark;
        d += step;
      }
    else
      while (n-- > 0)            // erase any unbounded run at image edge
      {
        *d = 0;
        d += step;
      }
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Region Claiming                              //
///////////////////////////////////////////////////////////////////////////

//= Label rims around seed objects with distance to nearest border of region.
// can optionally interpret image borders as valid edges also
// related to jhcDist::Nearest but does not have increasing halos around seed

int jhcRuns::BorderDist (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok) 
{
  if (!dest.Valid(1) || !dest.SameFormat(reg) || !dest.SameFormat(seed))
    return Fatal("Bad images to jhcRuns::BorderDist");

  a1.SetSize(dest);
  b1.SetSize(dest);
  ExtendH(b1, reg, seed, bdok);
  ExtendV(a1, reg, seed, bdok);
  nzm(b1, b1, a1);
  ExtendD1(a1, reg, seed, bdok);
  nzm(b1, b1, a1);
  ExtendD2(a1, reg, seed, bdok);
  nzm(dest, b1, a1);
  return 1;
}


//= Mark horizontal runs through region that start on seed and end outside.
// destination pixels are marked with run length from seed
// can optionally interpret image borders as valid edges
// <pre>
//    S: 0 0 0 F F F 0 0 F F F F 0 0 0 0
//    R: 0 0 F F F F F F F F F F F F F 0
//
//       | | | | | | | | | | | | | | | |
//       v v v v v v v v v v v v v v v v 
//
//    D: F F 1 0 0 0 F F 0 0 0 0 3 3 3 F      
// </pre>

int jhcRuns::ExtendH (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(reg) || !dest.SameFormat(seed))
    return Fatal("Bad images to jhcRuns::ExtendH");
  dest.CopyRoi(seed);
  dest.MergeRoi(reg);

  // generic ROI case
  UC8 val;
  int x, y, dir, n;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = seed.RoiSrc(dest), *r = reg.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // scan image horizontal line by line
  for (y = rh; y > 0; y--, s += rsk, r += rsk, d += rsk)
  {
    // possible start run looking for seed at boundary
    dir = 0;
    if (bdok > 0)
      dir = 1;
    n = 0;

    for (x = rw; x > 0; x--, s++, r++, d++)
      if (*s > 0)        // seed found
      {
        if (dir > 0)     // mark run from edge of region to seed
        {
          val = (UC8) __min(n, 255);
          d -= n;
          while (n-- > 0)
            *d++ = val;
        }
        *d = 0;          // known inside of object
        dir = -1;
        n = 0;
      }
      else if (*r == 0)  // outside region
      {
        if (dir < 0)     // mark run from seed to edge of region
        {
          val = (UC8) __min(n, 255);
          d -= n;
          while (n-- > 0)
            *d++ = val;
        }
        *d = 255;        // known outside of object
        dir = 1;
        n = 0;
      }
      else               // inside region
      {
        if (dir != 0)    // increase current run length
          n++;
        *d = 255;        // assume not in run or run will fail
      }

    // possible end run looking for edge at boundary
    if ((bdok > 0) && (dir < 0))
    {
      val = (UC8) __min(n, 255);
      d -= n;
      while (n-- > 0)
        *d++ = val;
    }
  }
  return 1;
}


//= Mark vertical runs through region that start on seed and end outside.
// destination pixels are marked with run length from seed
// can optionally interpret image borders as valid edges

int jhcRuns::ExtendV (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(reg) || !dest.SameFormat(seed))
    return Fatal("Bad images to jhcRuns::ExtendV");
  dest.CopyRoi(seed);
  dest.MergeRoi(reg);

  // generic ROI case
  UC8 val;
  int x, y, dir, n;
  int rw = dest.RoiW(), rh = dest.RoiH(), ln = dest.Line();
  const UC8 *s, *r, *s0 = seed.RoiSrc(dest), *r0 = reg.RoiSrc(dest);
  UC8 *d, *d0 = dest.RoiDest();

  // scan image horizontal line by line
  for (x = rw; x > 0; x--, s0++, r0++, d0++)
  {
    // possible start run looking for seed at boundary
    s = s0;
    r = r0;
    d = d0;
    dir = 0;
    if (bdok > 0)
      dir = 1;
    n = 0;

    for (y = rh; y > 0; y--, s += ln, r += ln, d += ln)
      if (*s > 0)        // seed found
      {
        if (dir > 0)     // mark run from edge of region to seed
        {
          val = (UC8) __min(n, 255);
          d -= (n * ln);
          while (n-- > 0)
          {
            *d = val;
            d += ln;
          }
        }
        *d = 0;          // known inside of object
        dir = -1;
        n = 0;
      }
      else if (*r == 0)  // outside region
      {
        if (dir < 0)     // mark run from seed to edge of region
        {
          val = (UC8) __min(n, 255);
          d -= (n * ln);
          while (n-- > 0)
          {
            *d = val;
            d += ln;
          }
        }
        *d = 255;        // known outside of object
        dir = 1;
        n = 0;
      }
      else               // inside region
      {
        if (dir != 0)    // increase current run length
          n++;
        *d = 255;        // assume not in run or run will fail
      }

    // possible end run looking for edge at boundary
    if ((bdok > 0) && (dir < 0))
    {
      val = (UC8) __min(n, 255);
      d -= (n * ln);
      while (n-- > 0)
      {
        *d = val;
        d += ln;
      }
    }
  }
  return 1;
}


//= Mark NW-to-SE diagonal runs through region that start on seed and end outside.
// destination pixels are marked with run length from seed
// can optionally interpret image borders as valid edges

int jhcRuns::ExtendD1 (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(reg) || !dest.SameFormat(seed))
    return Fatal("Bad images to jhcRuns::ExtendD1");
  dest.CopyRoi(seed);
  dest.MergeRoi(reg);

  // generic ROI case
  UC8 val;
  int x, y, dir, n, line = dest.Line(), step = line - 1;
  int rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *di, *d0 = dest.RoiDest() + rw - 1;
  const UC8 *s, *si, *s0 = seed.RoiSrc(dest) + rw - 1;
  const UC8 *r, *ri, *r0 = reg.RoiSrc(dest) + rw - 1;

  // scan bottom half of image (SW corner)
  si = s0;
  ri = r0;
  di = d0;
  for (x = rw; x > 0; x--, si--, ri--, di--)
  {
    // start at bottom of next diagonal
    s = si;
    r = ri;
    d = di;
    dir = 0;
    if (bdok > 0)
      dir = 1;
    n = 0;

    for (y = __min(rh, x); y > 0; y--, s += step, r += step, d += step)
      if (*s > 0)        // seed found
      {
        if (dir > 0)     // mark run from edge of region to seed
        {
          val = (UC8) __min(n, 255);
          d -= (n * step);
          while (n-- > 0)
          {
            *d = val;
            d += step;
          }
        }
        *d = 0;          // known inside of object
        dir = -1;
        n = 0;
      }
      else if (*r == 0)  // outside region
      {
        if (dir < 0)     // mark run from seed to edge of region
        {
          val = (UC8) __min(n, 255);
          d -= (n * step);
          while (n-- > 0)
          {
            *d = val;
            d += step;
          }
        }
        *d = 255;        // known outside of object
        dir = 1;
        n = 0;
      }
      else               // inside region
      {
        if (dir != 0)    // increase current run length
          n++;
        *d = 255;        // assume not in run or run will fail
      }

    // end of diagonal
    if ((bdok > 0) && (dir < 0))
    {
      val = (UC8) __min(n, 255);
      d -= (n * step);
      while (n-- > 0)
      {
        *d = val;
        d += step;
      }
    }
  }     

  // scan top half of image (NE corner)
  si = s0 + line;
  ri = r0 + line;
  di = d0 + line;
  for (y = rh - 1; y > 0; y--, si += line, ri += line, di += line)
  {
    // start at bottom of next diagonal
    s = si;                    
    r = ri;
    d = di;
    dir = 0;
    if (bdok > 0)
      dir = 1;
    n = 0;

    for (x = __min(rw, y); x > 0; x--, s += step, r += step, d += step)
      if (*s > 0)        // seed found
      {
        if (dir > 0)     // mark run from edge of region to seed
        {
          val = (UC8) __min(n, 255);
          d -= (n * step);
          while (n-- > 0)
          {
            *d = val;
            d += step;
          }
        }
        *d = 0;          // known inside of object
        dir = -1;
        n = 0;
      }
      else if (*r == 0)  // outside region
      {
        if (dir < 0)     // mark run from seed to edge of region
        {
          val = (UC8) __min(n, 255);
          d -= (n * step);
          while (n-- > 0)
          {
            *d = val;
            d += step;
          }
        }
        *d = 255;        // known outside of object
        dir = 1;
        n = 0;
      }
      else               // inside region
      {
        if (dir != 0)    // increase current run length
          n++;
        *d = 255;        // assume not in run or run will fail
      }

    // end of diagonal
    if ((bdok > 0) && (dir < 0))
    {
      val = (UC8) __min(n, 255);
      d -= (n * step);
      while (n-- > 0)
      {
        *d = val;
        d += step;
      }
    }
  }
  return 1;
}


//= Mark NE-to-SW diagonal runs through region that start on seed and end outside.
// destination pixels are marked with run length from seed
// can optionally interpret image borders as valid edges

int jhcRuns::ExtendD2 (jhcImg& dest, const jhcImg& reg, const jhcImg& seed, int bdok) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(reg) || !dest.SameFormat(seed))
    return Fatal("Bad images to jhcRuns::ExtendD1");
  dest.CopyRoi(seed);
  dest.MergeRoi(reg);

  // generic ROI case
  UC8 val;
  int x, y, dir, n, line = dest.Line(), step = line + 1;
  int rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *di, *d0 = dest.RoiDest();
  const UC8 *s, *si, *s0 = seed.RoiSrc(dest);
  const UC8 *r, *ri, *r0 = reg.RoiSrc(dest);

  // scan bottom half of image (SE corner)
  si = s0;
  ri = r0;
  di = d0;
  for (x = rw; x > 0; x--, si++, ri++, di++)
  {
    // start at bottom of next diagonal
    s = si;
    r = ri;
    d = di;
    dir = 0;
    if (bdok > 0)
      dir = 1;
    n = 0;

    for (y = __min(rh, x); y > 0; y--, s += step, r += step, d += step)
      if (*s > 0)        // seed found
      {
        if (dir > 0)     // mark run from edge of region to seed
        {
          val = (UC8) __min(n, 255);
          d -= (n * step);
          while (n-- > 0)
          {
            *d = val;
            d += step;
          }
        }
        *d = 0;          // known inside of object
        dir = -1;
        n = 0;
      }
      else if (*r == 0)  // outside region
      {
        if (dir < 0)     // mark run from seed to edge of region
        {
          val = (UC8) __min(n, 255);
          d -= (n * step);
          while (n-- > 0)
          {
            *d = val;
            d += step;
          }
        }
        *d = 255;        // known outside of object
        dir = 1;
        n = 0;
      }
      else               // inside region
      {
        if (dir != 0)    // increase current run length
          n++;
        *d = 255;        // assume not in run or run will fail
      }

    // end of diagonal
    if ((bdok > 0) && (dir < 0))
    {
      val = (UC8) __min(n, 255);
      d -= (n * step);
      while (n-- > 0)
      {
        *d = val;
        d += step;
      }
    }
  }     

  // scan top half of image (NW corner)
  si = s0 + line;
  ri = r0 + line;
  di = d0 + line;
  for (y = rh - 1; y > 0; y--, si += line, ri += line, di += line)
  {
    // start at bottom of next diagonal
    s = si;                    
    r = ri;
    d = di;
    dir = 0;
    if (bdok > 0)
      dir = 1;
    n = 0;

    for (x = __min(rw, y); x > 0; x--, s += step, r += step, d += step)
      if (*s > 0)        // seed found
      {
        if (dir > 0)     // mark run from edge of region to seed
        {
          val = (UC8) __min(n, 255);
          d -= (n * step);
          while (n-- > 0)
          {
            *d = val;
            d += step;
          }
        }
        *d = 0;          // known inside of object
        dir = -1;
        n = 0;
      }
      else if (*r == 0)  // outside region
      {
        if (dir < 0)     // mark run from seed to edge of region
        {
          val = (UC8) __min(n, 255);
          d -= (n * step);
          while (n-- > 0)
          {
            *d = val;
            d += step;
          }
        }
        *d = 255;        // known outside of object
        dir = 1;
        n = 0;
      }
      else               // inside region
      {
        if (dir != 0)    // increase current run length
          n++;
        *d = 255;        // assume not in run or run will fail
      }

    // end of diagonal
    if ((bdok > 0) && (dir < 0))
    {
      val = (UC8) __min(n, 255);
      d -= (n * step);
      while (n-- > 0)
      {
        *d = val;
        d += step;
      }
    }
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Gap Interpolation                            //
///////////////////////////////////////////////////////////////////////////

//= Linearly interpolate across small dropouts using best fill direction.
// always runs H and V gap filling, does D1 and D2 if diag is positive
// takes the smallest of the proposed fill values at each pixel

int jhcRuns::MinRamp (jhcImg& dest, const jhcImg& src, int maxgap, int diag) 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::MinRamp");
  if (maxgap <= 0)
    return dest.CopyArr(src);

  // get horizontal and vertical fills
  a1.SetSize(dest);
  b1.SetSize(dest);
  RampH(b1, src, maxgap);
  RampV(a1, src, maxgap);

  // possibly combine with diagonal fills
  if (diag > 0)
  {
    nzm(b1, b1, a1);
    RampD1(a1, src, maxgap);
    nzm(b1, b1, a1);
    RampD2(a1, src, maxgap);
  }

  // do final combination
  nzm(dest, b1, a1);
  return 1;
}


//= Linearly interpolate across small dropouts using average of fill directions.
// always runs H and V gap filling, does D1 and D2 if diag is positive
// conceptually takes the average of all the proposed fill values at each pixel
// Note: if 3 valid estimates, one will be overweighted (e.g. V if H is missing)
// effectively estimates using orthogonal runs then diagonal ones then averages

int jhcRuns::AvgRamp (jhcImg& dest, const jhcImg& src, int maxgap, int diag)  
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::AvgRamp");
  if (maxgap <= 0)
    return dest.CopyArr(src);

  // get horizontal and vertical fills
  a1.SetSize(dest);
  b1.SetSize(dest);
  RampH(b1, src, maxgap);
  RampV(a1, src, maxgap);

  // possibly combine with diagonal fills
  if (diag > 0)
  {
    nz_avg(a1, b1, a1);
    c1.SetSize(dest);
    RampD1(b1, src, maxgap);
    RampD2(c1, src, maxgap);
    nz_avg(b1, b1, c1);
  }

  // do final combination
  nz_avg(dest, b1, a1);
  return 1;
}


//= Linearly interpolate across horizontal gaps up to "maxgap" size.

int jhcRuns::RampH (jhcImg& dest, const jhcImg& src, int maxgap) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::RampH");
  if (maxgap <= 0)
    return dest.CopyArr(src);
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, j, run, v, inc;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // scan image horizontal line by line
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
  {
    v = 0;
    run = 0;
    for (x = rw; x > 0; x--, s++)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        inc = ((*s - v) << 16) / (run + 1);
        v = (v << 16) + 32768;
        for (j = run; j > 0; j--, d++, v += inc)
          *d = (UC8)(v >> 16);
      }
      else 
        for (j = run; j > 0; j--, d++)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d++;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d++)
        *d = 0;
    else
      for (j = rw; j > 0; j--, d++)
        *d = 0;
  }
  return 1;
}


//= Linearly interpolate across vertical gaps up to "maxgap" size.

int jhcRuns::RampV (jhcImg& dest, const jhcImg& src, int maxgap) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::RampV");
  if (maxgap <= 0)
    return dest.CopyArr(src);
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, j, run, v, inc, line = dest.Line(), rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *d0 = dest.RoiDest();
  const UC8 *s, *s0 = src.RoiSrc();

  // scan image vertically column by column
  for (x = rw; x > 0; x--, s0++, d0++)
  {
    s = s0;                      // start at bottom of next column
    d = d0;
    v = 0;
    run = 0;
    for (y = rh; y > 0; y--, s += line)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        inc = ((*s - v) << 16) / (run + 1);
        v = (v << 16) + 32768;
        for (j = run; j > 0; j--, d += line, v += inc)
          *d = (UC8)(v >> 16);
      }
      else 
        for (j = run; j > 0; j--, d += line)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += line;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += line)
        *d = 0;
    else
      for (j = rh; j > 0; j--, d += line)
        *d = 0;
  }
  return 1;
}


//= Linearly interpolate across major diagonal gaps up to "maxgap" size.

int jhcRuns::RampD1 (jhcImg& dest, const jhcImg& src, int maxgap) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::RampD1");
  if (maxgap <= 0)
    return dest.CopyArr(src);
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, j, run, v, inc, line = dest.Line(), step = line - 1;
  int rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *di, *d0 = dest.RoiDest() + rw - 1;
  const UC8 *s, *si, *s0 = src.RoiSrc() + rw - 1;

  // scan bottom half of image (SW corner)
  di = d0;
  si = s0;
  for (x = rw; x > 0; x--, si--, di--)
  {
    s = si;                      // start at bottom of next diagonal
    d = di;
    v = 0;
    run = 0;
    for (y = __min(rh, x); y > 0; y--, s += step)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        inc = ((*s - v) << 16) / (run + 1);
        v = (v << 16) + 32768;
        for (j = run; j > 0; j--, d += step, v += inc)
          *d = (UC8)(v >> 16);
      }
      else 
        for (j = run; j > 0; j--, d += step)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += step;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += step)
        *d = 0;
    else
      for (j = __min(rh, x); j > 0; j--, d += step)
        *d = 0;
  }

  // scan top half of image (NE corner)
  di = d0 + line;
  si = s0 + line;
  for (y = rh - 1; y > 0; y--, si += line, di += line)
  {
    s = si;                    // start at bottom of next diagonal
    d = di;
    v = 0;
    run = 0;
    for (x = __min(rw, y); x > 0; x--, s += step)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        inc = ((*s - v) << 16) / (run + 1);
        v = (v << 16) + 32768;
        for (j = run; j > 0; j--, d += step, v += inc)
          *d = (UC8)(v >> 16);
      }
      else 
        for (j = run; j > 0; j--, d += step)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += step;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += step)
        *d = 0;
    else
      for (j = __min(rw, y); j > 0; j--, d += step)
        *d = 0;
  }
  return 1;
}


//= Linearly interpolate across minor diagonal gaps up to "maxgap" size.

int jhcRuns::RampD2 (jhcImg& dest, const jhcImg& src, int maxgap) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::RampD2");
  if (maxgap <= 0)
    return dest.CopyArr(src);
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, j, run, v, inc, line = dest.Line(), step = line + 1;
  int rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *di, *d0 = dest.RoiDest();
  const UC8 *s, *si, *s0 = src.RoiSrc();

  // scan bottom half of image (SE corner)
  di = d0;
  si = s0;
  for (x = rw; x > 0; x--, si++, di++)
  {
    s = si;                      // start at bottom of next diagonal
    d = di;
    v = 0;
    run = 0;
    for (y = __min(rh, x); y > 0; y--, s += step)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        inc = ((*s - v) << 16) / (run + 1);
        v = (v << 16) + 32768;
        for (j = run; j > 0; j--, d += step, v += inc)
          *d = (UC8)(v >> 16);
      }
      else 
        for (j = run; j > 0; j--, d += step)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += step;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += step)
        *d = 0;
    else
      for (j = __min(rh, x); j > 0; j--, d += step)
        *d = 0;
  }

  // scan top half of image (NW corner)
  di = d0 + line;
  si = s0 + line;
  for (y = rh - 1; y > 0; y--, si += line, di += line)
  {
    s = si;                    // start at bottom of next diagonal
    d = di;
    v = 0;
    run = 0;
    for (x = __min(rw, y); x > 0; x--, s += step)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        inc = ((*s - v) << 16) / (run + 1);
        v = (v << 16) + 32768;
        for (j = run; j > 0; j--, d += step, v += inc)
          *d = (UC8)(v >> 16);
      }
      else 
        for (j = run; j > 0; j--, d += step)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += step;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += step)
        *d = 0;
    else
      for (j = __min(rw, y); j > 0; j--, d += step)
        *d = 0;
  }
  return 1;
}


//= Combine two images to give pairwise average at every pixel.
// only considers PxlSrc() which have values greater than 0

void jhcRuns::nz_avg (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb) const 
{
  dest.CopyRoi(imga);
  dest.MergeRoi(imgb);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *a = imga.RoiSrc(dest), *b = imgb.RoiSrc(dest);

  // compare PxlSrc() in ROI
  for (y = rh; y > 0; y--, d += rsk, a += rsk, b += rsk)
    for (x = rcnt; x > 0; x--, d++, a++, b++)
      if (*a == 0)
        *d = *b;
      else if (*b == 0)
        *d = *a;
      else
        *d = (*a + *b + 1) >> 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Gap Filling                                //
///////////////////////////////////////////////////////////////////////////

//= Fill all holes with the lowest surrounding value.
// always runs H and V gap filling, does D1 and D2 if diag is positive
// takes the smallest of the proposed fill values at each pixel
// useful for depth drop-outs in stereo range-finding

int jhcRuns::LowestAll (jhcImg& dest, const jhcImg& src, int maxgap, int diag)  
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::LowestAll");
  if (maxgap <= 0)
    return dest.CopyArr(src);

  // get horizontal and vertical fills
  a1.SetSize(dest);
  b1.SetSize(dest);
  LowestH(b1, src, maxgap);
  LowestV(a1, src, maxgap);

  // possibly combine with diagonal fills
  if (diag > 0)
  {
    nzm(b1, b1, a1);
    LowestD1(a1, src, maxgap);
    nzm(b1, b1, a1);
    LowestD2(a1, src, maxgap);
  }

  // do final combination
  nzm(dest, b1, a1);
  return 1;
}


//= For small horizontal gaps copy the lower of the bounding values.

int jhcRuns::LowestH (jhcImg& dest, const jhcImg& src, int maxgap) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::LowestH");
  if (maxgap <= 0)
    return dest.CopyArr(src);
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, j, run, v;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // scan image horizontal line by line
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
  {
    v = 0;
    run = 0;
    for (x = rw; x > 0; x--, s++)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with lowest side
      if ((v > 0) && (run <= maxgap))
      {
        v = __min(v, *s);
        for (j = run; j > 0; j--, d++)
          *d = (UC8) v;
      }
      else 
        for (j = run; j > 0; j--, d++)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d++;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d++)
        *d = 0;
    else
      for (j = rw; j > 0; j--, d++)
        *d = 0;
  }
  return 1;
}



//= For small vertical gaps copy the lower of the bounding values.

int jhcRuns::LowestV (jhcImg& dest, const jhcImg& src, int maxgap) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::LowestV");
  if (maxgap <= 0)
    return dest.CopyArr(src);
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, j, run, v, line = dest.Line(), rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *d0 = dest.RoiDest();
  const UC8 *s, *s0 = src.RoiSrc();

  // scan image vertically column by column
  for (x = rw; x > 0; x--, s0++, d0++)
  {
    s = s0;                      // start at bottom of next column
    d = d0;
    v = 0;
    run = 0;
    for (y = rh; y > 0; y--, s += line)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with lowest side
      if ((v > 0) && (run <= maxgap))
      {
        v = __min(v, *s);
        for (j = run; j > 0; j--, d += line)
          *d = (UC8) v;
      }
      else 
        for (j = run; j > 0; j--, d += line)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += line;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += line)
        *d = 0;
    else
      for (j = rh; j > 0; j--, d += line)
        *d = 0;
  }
  return 1;
}


//= For small major diagonal gaps copy the lower of the bounding values.

int jhcRuns::LowestD1 (jhcImg& dest, const jhcImg& src, int maxgap) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::LowestD1");
  if (maxgap <= 0)
    return dest.CopyArr(src);
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, j, run, v, line = dest.Line(), step = line - 1;
  int rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *di, *d0 = dest.RoiDest() + rw - 1;
  const UC8 *s, *si, *s0 = src.RoiSrc() + rw - 1;

  // scan bottom half of image (SW corner)
  di = d0;
  si = s0;
  for (x = rw; x > 0; x--, si--, di--)
  {
    s = si;                      // start at bottom of next diagonal
    d = di;
    v = 0;
    run = 0;
    for (y = __min(rh, x); y > 0; y--, s += step)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        v = __min(v, *s);
        for (j = run; j > 0; j--, d += step)
          *d = (UC8) v;
      }
      else 
        for (j = run; j > 0; j--, d += step)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += step;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += step)
        *d = 0;
    else
      for (j = __min(rh, x); j > 0; j--, d += step)
        *d = 0;
  }

  // scan top half of image (NE corner)
  di = d0 + line;
  si = s0 + line;
  for (y = rh - 1; y > 0; y--, si += line, di += line)
  {
    s = si;                    // start at bottom of next diagonal
    d = di;
    v = 0;
    run = 0;
    for (x = __min(rw, y); x > 0; x--, s += step)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        v = __min(v, *s);
        for (j = run; j > 0; j--, d += step)
          *d = (UC8) v;
      }
      else 
        for (j = run; j > 0; j--, d += step)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += step;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += step)
        *d = 0;
    else
      for (j = __min(rw, y); j > 0; j--, d += step)
        *d = 0;
  }
  return 1;
}

//= For small minor diagonal gaps copy the lower of the bounding values.

int jhcRuns::LowestD2 (jhcImg& dest, const jhcImg& src, int maxgap) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::LowestD2");
  if (maxgap <= 0)
    return dest.CopyArr(src);
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, j, run, v, line = dest.Line(), step = line + 1;
  int rw = dest.RoiW(), rh = dest.RoiH();
  UC8 *d, *di, *d0 = dest.RoiDest();
  const UC8 *s, *si, *s0 = src.RoiSrc();

  // scan bottom half of image (SE corner)
  di = d0;
  si = s0;
  for (x = rw; x > 0; x--, si++, di++)
  {
    s = si;                      // start at bottom of next diagonal
    d = di;
    v = 0;
    run = 0;
    for (y = __min(rh, x); y > 0; y--, s += step)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        v = __min(v, *s);
        for (j = run; j > 0; j--, d += step)
          *d = (UC8) v;
      }
      else 
        for (j = run; j > 0; j--, d += step)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += step;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += step)
        *d = 0;
    else
      for (j = __min(rh, x); j > 0; j--, d += step)
        *d = 0;
  }

  // scan top half of image (NW corner)
  di = d0 + line;
  si = s0 + line;
  for (y = rh - 1; y > 0; y--, si += line, di += line)
  {
    s = si;                    // start at bottom of next diagonal
    d = di;
    v = 0;
    run = 0;
    for (x = __min(rw, y); x > 0; x--, s += step)
    {
      // count the size of bounded gaps
      if (*s == 0)
      {
        run++;
        continue;
      }

      // if gap small enough then fill with ramp
      if ((v > 0) && (run <= maxgap))
      {
        v = __min(v, *s);
        for (j = run; j > 0; j--, d += step)
          *d = (UC8) v;
      }
      else 
        for (j = run; j > 0; j--, d += step)
          *d = 0;

      // no gap right now
      run = 0;
      v = *s;
      *d = (UC8) v;
      d += step;
    }

    // handle any final gap
    if (v > 0)
      for (j = run; j > 0; j--, d += step)
        *d = 0;
    else
      for (j = __min(rw, y); j > 0; j--, d += step)
        *d = 0;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Non-Maximum Suppression                        //
///////////////////////////////////////////////////////////////////////////

//= Thins response to peaks and valleys in horizontal direction.
// discards peak or valley if less than th from central 128
// marks middle of plateaus with 256-width and basins with width-1 
// <pre>
// with th = 20:
//
//   S:  128 130 150 150 140 170 120 110 115 100 100 100 110  
//   D:  128 128 128 254 128 255 128 128 128 128  2  128 128
//                    |       |       |           |
//                  peak    peak    valley      basin
//                 width=2         too weak    width=3
// </pre>

int jhcRuns::InflectH (jhcImg& dest, const jhcImg& src, int th) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::InflectH");
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, run;
  int rw = dest.RoiW() - 1, rh = dest.RoiH(), ln = dest.Line();
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();

  // scan image horizontally line by line
  for (y = rh; y > 0; y--, d += ln, s += ln)
  {
    run = 0;
    for (x = 0; x < rw; x++)
    {
      d[x] = 128;
      if (s[x + 1] < s[x])                         // going down
      {
        if ((run > 0) && ((s[x] - 128) >= th))
          d[x - (run >> 1)] = (UC8)(256 - run);    // end of peak or plateau
        run = -1;
      }
      else if (s[x + 1] > s[x])                    // going up
      {
        if ((run < 0) && ((128 - s[x]) >= th))
          d[x + (run >> 1)] = (UC8)(1 - run);      // end of valley or basin
        run = 1;
      }
      else if (run > 0)
        run++;
      else if (run < 0)
        run--;
    }
    d[x] = 128;
  }
  return 1;
}


//= Thins response to peaks and valleys in vertical direction.
// discards peak or valley if less than th from central 128
// marks middle of plateaus with 256-width and basins with width-1 

int jhcRuns::InflectV (jhcImg& dest, const jhcImg& src, int th) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::InflectV");
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, yln, run;
  int rw = dest.RoiW(), rh = dest.RoiH() - 1, ln = dest.Line();
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();

  // scan image vertically column by column
  for (x = rw; x > 0; x--, d++, s++)
  {
    run = 0;
    yln = 0;
    for (y = 0; y < rh; y++, yln += ln)
    {
      d[yln] = 128;
      if (s[yln + ln] < s[yln])                         // going down
      {
        if ((run > 0) && ((s[yln] - 128) >= th))
          d[yln - (run >> 1) * ln] = (UC8)(256 - run);  // end of peak or plateau
        run = -1;
      }
      else if (s[yln + ln] > s[yln])                    // going up
      {
        if ((run < 0) && ((128 - s[yln]) >= th))
          d[yln + (run >> 1) * ln] = (UC8)(1 - run);    // end of valley or basin
        run = 1;
      }
      else if (run > 0)
        run++;
      else if (run < 0)
        run--;
    }
    d[yln] = 128;
  }
  return 1;
}



//= Thins response to peaks and valleys in NW-to-SE diagonal direction.
// discards peak or valley if less than th from central 128
// marks middle of plateaus with 256-width and basins with width-1 

int jhcRuns::InflectD1 (jhcImg& dest, const jhcImg& src, int th) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcRuns::InflectD1");
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, stop, run;
  int rw = dest.RoiW(), rh = dest.RoiH(), ln = dest.Line(), step = ln - 1;
  const UC8 *s, *s0, *si = src.RoiSrc() + rw - 1;
  UC8 *d, *d0, *di = dest.RoiDest() + rw - 1;

  // scan bottom half of image (SW triangle) starting in SE corner
  d0 = di;
  s0 = si;
  for (x = rw; x > 0; x--, d0--, s0--)
  {
    run = 0;
    d = d0;
    s = s0;
    stop = __min(rh, x) - 1;
    for (y = 0; y < stop; y++, d += step, s += step)
    {
      d[0] = 128;
      if (s[step] < s[0])                                  // going down
      {
        if ((run > 0) && ((s[0] - 128) >= th))
          d0[(y - (run >> 1)) * step] = (UC8)(256 - run);  // end of peak or plateau
        run = -1;
      }
      else if (s[step] > s[0])                             // going up
      {
        if ((run < 0) && ((128 - s[0]) >= th))
          d0[(y + (run >> 1)) * step] = (UC8)(1 - run);    // end of valley or basin
        run = 1;
      }
      else if (run > 0)
        run++;
      else if (run < 0)
        run--;
    }
    d[0] = 128;
  }

  // scan top half of image (NE triangle) starting in SE corner
  d0 = di + ln;
  s0 = si + ln;
  for (y = rh - 1; y > 0; y--, d0 += ln, s0 += ln)
  {
    run = 0;
    d = d0;
    s = s0;
    stop = __min(rw, y) - 1;
    for (x = 0; x < stop; x++, d += step, s += step)
    {
      d[0] = 128;
      if (s[step] < s[0])                                  // going down
      {
        if ((run > 0) && ((s[0] - 128) >= th))
          d0[(x - (run >> 1)) * step] = (UC8)(256 - run);  // end of peak or plateau
        run = -1;
      }
      else if (s[step] > s[0])                             // going up
      {
        if ((run < 0) && ((128 - s[0]) >= th))
          d0[(x + (run >> 1)) * step] = (UC8)(1 - run);    // end of valley or basin
        run = 1;
      }
      else if (run > 0)
        run++;
      else if (run < 0)
        run--;
    }
    d[0] = 128;
  }
  return 1;
}



//= Thins response to peaks and valleys in NE-to-SW diagonal direction.
// discards peak or valley if less than th from central 128
// marks middle of plateaus with 256-width and basins with width-1 

int jhcRuns::InflectD2 (jhcImg& dest, const jhcImg& src, int th) const 
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcRuns::InflectD2");
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, stop, run;
  int rw = dest.RoiW(), rh = dest.RoiH(), ln = dest.Line(), step = ln + 1;
  const UC8 *s, *s0, *si = src.RoiSrc();
  UC8 *d, *d0, *di = dest.RoiDest();

  // scan bottom half of image (SE triangle) starting in NE corner
  d0 = di;
  s0 = si;
  for (x = rw; x > 0; x--, d0++, s0++)
  {
    run = 0;
    d = d0;
    s = s0;
    stop = __min(rh, x) - 1;
    for (y = 0; y < stop; y++, d += step, s += step)
    {
      d[0] = 128;
      if (s[step] < s[0])                                   // going down
      {
        if ((run > 0) && ((s[0] - 128) >= th))
          d0[(y - (run >> 1)) * step] = (UC8)(256 - run);   // end of peak or plateau
        run = -1;
      }
      else if (s[step] > s[0])                              // going up
      {
        if ((run < 0) && ((128 - s[0]) >= th))
          d0[(y + (run >> 1)) * step] = (UC8)(1 - run);     // end of valley or basin
        run = 1;
      }
      else if (run > 0)
        run++;
      else if (run < 0)
        run--;
    }
    d[0] = 128;
  }

  // scan top half of image (NW triangle) starting in NE corner
  d0 = di + ln;
  s0 = si + ln;
  for (y = rh - 1; y > 0; y--, d0 += ln, s0 += ln)
  {
    run = 0;
    d = d0;
    s = s0;
    stop = __min(rw, y) - 1;
    for (x = 0; x < stop; x++, d += step, s += step)
    {
      d[0] = 128;
      if (s[step] < s[0])                                   // going down
      {
        if ((run > 0) && ((s[0] - 128) >= th))
          d0[(x - (run >> 1)) * step] = (UC8)(256 - run);   // end of peak or plateau
        run = -1;
      }
      else if (s[step] > s[0])                              // going up
      {
        if ((run < 0) && ((128 - s[0]) >= th))
          d0[(x + (run >> 1)) * step] = (UC8)(1 - run);     // end of valley or basin
        run = 1;
      }
      else if (run > 0)
        run++;
      else if (run < 0)
        run--;
    }
    d[0] = 128;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Well Formed-ness                           //
///////////////////////////////////////////////////////////////////////////

//= Extend from region above threshold through okay regions until edge hit.
// if no edge found in some direction then keeps original thresholding
// helps when region defined by some criterion like same color
// intuititively region should not stop unless there is some reason
// boundary of image automatically treated as valid stopping points
// thresholds ej at 128, ignores ROIs and always does full image
// <pre>
// TH = 255, ok = 128
//
//                   ok  ok  ok  TH  TH  TH  ok  ok  ok  ok  ok  
//   S:   0  20 100 150 150 200 255 255 255 220 180 160 150 120 100  80  50   0   0
//   E:   0   0   0   0   0 180 200   0   0   0   0 255   0   0   0   0   0   0   0
//   D:   0   0   0   0   0 255 255 255 255 255 255 255   0   0   0   0   0   0   0
//                           .   X   X   X   .   .   .
//
// X = original threshold, . = extension
// </pre>
// returns binary mask of union of all directional runs plus seed area

int jhcRuns::StopAt (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(ej))
    return Fatal("Bad images to jhcRuns::StopAt");

  thresh(dest, src, th, 255, 0);
  StopAtH(dest, src, ej, th, ok);
  StopAtV(dest, src, ej, th, ok);
//  StopAtD1(dest, src, ej, th, ok);
//  StopAtD2(dest, src, ej, th, ok);
  return 1;
}


//= Extends mask in horizontal direction from th through ok until ej encountered.
// marks as 255 only the added fringe around original mask connecting it to edge
// generally dest should have thresholded original image
// always returns 1 for convenience

int jhcRuns::StopAtH (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(ej))
    return Fatal("Bad images to jhcRuns::StopAtH");

  // full image case
  int x, y, run, fill, w = dest.XDim(), h = dest.YDim(), ln = dest.Line();
  const UC8 *s = src.PxlSrc(), *e = ej.PxlSrc();
  UC8 *d = dest.PxlDest();

  for (y = 0; y < h; y++, d += ln, s += ln, e += ln)
  {
    // start from-lf
    run = -1;    

    // create runs through valid regions   
    for (x = 0; x < w; x++)
      if (s[x] < ok)
        run = 0;                 
      else if (s[x] >= th)
      {
        // possible end from-lf or gap, start to-rt
        if (run != 0)                              
          for (fill = x - abs(run) + 1; fill < x; fill++)
            d[fill] = 255;
        run = 1;                                   
      }
      else if (e[x] > 128)
      {
        // possible end to-rt, start from-lf
        if (run > 0)                        
          for (fill = x - run + 1; fill < x; fill++)
            d[fill] = 255;
        run = -1;                       
      }
      else 
      {  
        // continue run
        if (run > 0)
          run++;  
        else if (run < 0)
          run--;  
      }

    // finish any to-rt
    if (run > 0)
      for (fill = x - run + 1; fill < x; fill++)
        d[fill] = 255;
  }
  return 1;
}


//= Extends mask in vertical direction from th through ok until ej encountered.
// marks as 255 only the added fringe around original mask connecting it to edge
// generally dest should have thresholded original image
// always returns 1 for convenience

int jhcRuns::StopAtV (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(ej))
    return Fatal("Bad images to jhcRuns::StopAtV");

  // full image case
  int x, run, fill, w = dest.XDim(), h = dest.YDim();
  int yoff, ln = dest.Line(), top = h * ln;
  const UC8 *s = src.PxlSrc(), *e = ej.PxlSrc();
  UC8 *d = dest.PxlDest();

  for (x = 0; x < w; x++, d++, s++, e++)
  {
    // start from-bot
    run = -1;    

    // create runs through valid regions   
    for (yoff = 0; yoff < top; yoff += ln)
      if (s[yoff] < ok)
        run = 0;                 
      else if (s[yoff] >= th)
      {
        // possible end from-bot or gap, start to-top
        if (run != 0)                              
          for (fill = yoff - ln * (abs(run) - 1); fill < yoff; fill += ln)
            d[fill] = 255;
        run = 1;                                   
      }
      else if (e[yoff] > 128)
      {
        // possible end to-top, start from-bot
        if (run > 0)                        
          for (fill = yoff - ln * (run - 1); fill < yoff; fill += ln)
            d[fill] = 255;
        run = -1;                       
      }
      else 
      {  
        // continue run
        if (run > 0)
          run++;  
        else if (run < 0)
          run--;  
      }

    // finish any to-top
    if (run > 0)
      for (fill = yoff - ln * (run - 1); fill < yoff; fill += ln)
        d[fill] = 255;
  }
  return 1;
}


//= Extends mask in major diagonal direction from th through ok until ej encountered.
// marks as 255 only the added fringe around original mask connecting it to edge
// generally dest should have thresholded original image
// always returns 1 for convenience

int jhcRuns::StopAtD1 (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(ej))
    return Fatal("Bad images to jhcRuns::StopAtD1");

  // never finished because main StopAt was not useful

  return 1;
}


//= Extends mask in minor diagonal direction from th through ok until ej encountered.
// marks as 255 only the added fringe around original mask connecting it to edge
// generally dest should have thresholded original image
// always returns 1 for convenience

int jhcRuns::StopAtD2 (jhcImg& dest, const jhcImg& src, const jhcImg& ej, int th, int ok) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(ej))
    return Fatal("Bad images to jhcRuns::StopAtD2");

  // never finished because main StopAt was not useful

  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                             Object Bottoms                            //
///////////////////////////////////////////////////////////////////////////

//= Mark horizontal gaps relative source mask if they are narrow enough. 
// does not mark gaps between mask and sides of image
// useful for finding the bottoms of objects that extend above floor color region

int jhcRuns::SmallGapH (jhcImg& dest, const jhcImg& src, int wmax) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::SmallGapH");
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, valid, n, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();

  // scan image horizontal line by line
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
  {
    // no gaps start from left border
    valid = 0;
    n = 0;
    for (x = rw; x > 0; x--, s++)
    {
      // see if inside a gap
      if (*s < 128)         
        n++;
      else                       
      { 
        // see if run just ended
        if (n > 0)              
        {
          if ((valid > 0) && (n <= wmax))
            while (n-- > 0)      
              *d++ = 255;        // record length over strip
          else 
            while (n-- > 0)      
              *d++ = 0;          // clear strip
          n = 0;
        }

        // outside a gap so write zero here
        *d++ = 0;                
        valid = 1;
      }
    }

    // clear strip to right border
    if (n > 0)                   
      while (n-- > 0)            
        *d++ = 0;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Object Accomodation                           //
///////////////////////////////////////////////////////////////////////////

//= Mark centers where a wid by ht box will cover pixels all above threshold.
// since needs integer center coords, forces wid and ht to be odd

int jhcRuns::FitsBox (jhcImg& dest, const jhcImg& src, int wid, int ht, int th ) const
{
  FitsH(dest, src,  wid, th);
  FitsV(dest, dest, ht,  0); 
  return 1;
}


//= Mark centers where a horizontal span of wid will cover pixels all above threshold.
// since needs a integer center, forces wid to be odd

int jhcRuns::FitsH (jhcImg& dest, const jhcImg& src, int wid, int th) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::FitsH");
  dest.CopyRoi(src);

  // generic ROI case
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int x, y, run, odd = wid | 0x01, half = odd >> 1;
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--, d += rsk, s += rsk)
  {
    run = 0;
    for (x = rw; x > 0; x--, d++, s++)
    {
      if (*s <= th)
        run = 0;
      else if (++run >= odd)
        *(d - half) = 255;             // center of span (to left)
      *d = 0;                          // in case src = dest
    }  
  }
  return 1;
}


//= Mark centers where a vertical span of ht will cover pixels all above threshold.
// since needs a integer center, forces ht to be odd

int jhcRuns::FitsV (jhcImg& dest, const jhcImg& src, int ht, int th) const
{
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcRuns::FitsV");
  dest.CopyRoi(src);

  // generic ROI case
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int x, y, odd = ht | 0x01, half = (odd >> 1) * dest.Line();
  int *runs = new int[rw];
  const UC8 *s = src.RoiSrc();
  UC8 *d = dest.RoiDest();

  for (x = 0; x < rw; x++)            // initialize all columns
    runs[x] = 0;        
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = 0; x < rw; x++, d++, s++)
    {
      if (*s <= th)
        runs[x] = 0;
      else if (++(runs[x]) >= odd)
        *(d - half) = 255;             // center of span (below)
      *d = 0;                          // in case src = dest
    }  
  delete [] runs;                      // clean up
  return 1;
}
