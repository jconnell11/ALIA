// jhcDist.cpp : spreading activation like space claiming 
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

#include <math.h>
#include "Interface/jhcMessage.h"

#include "Processing/jhcDist.h"


///////////////////////////////////////////////////////////////////////////
//                         Manhattan Distance                            //
///////////////////////////////////////////////////////////////////////////

//= Claim "empty" pixels for nearest "seed" based on Manhattan metric.
// when two seeds are equidistant, higher label dominates
// default version for labels of 16 bits (2 fields) and 16 bit distances

int jhcDist::Nearest (jhcImg& label, const jhcImg& seed, int bg, jhcImg *rng)
{
  jhcImg *dist = rng; 

  // check size and special cases, possibly make up scratch array
  if (label.Valid(1))
    return Nearest8(label, seed, bg, rng);
  if (!label.Valid(2) || !label.SameFormat(seed) || label.SameImg(seed))
    return Fatal("Bad images to jhcDist::Nearest");
  if ((dist != NULL) && !label.SameSize(*dist))
    return Fatal("Bad range image to jhcDist::Nearest");
  if (dist == NULL) 
    dist = a2.SetSize(seed);
  label.CopyRoi(seed);
  dist->CopyRoi(seed);

  // set up iteration variables
  int x, y, rw = seed.RoiW(), rh = seed.RoiH();
  US16 cnt, last, bval = (US16) bg;
  UL32 roff = seed.RoiOff(), rsk = seed.Line() >> 1;
  US16 *s1, *s = (US16 *)(seed.PxlSrc() + roff);
  US16 *d, *d0 = (US16 *)(dist->PxlSrc() + roff);
  US16 *n, *n0 = (US16 *)(label.PxlSrc() + roff);


  // PASS 1: find nearest neighbor and distance in horizontal direction
  d = d0;
  n = n0;
  for (y = rh; y > 0; y--)
  {
    // right wipe, extend last label and increment distance 
    s1 = s;
    cnt = 0;
    last = bval;
    for (x = rw; x > 0; x--)
    {
      if (last != bval)
        cnt++;
      if (*s != bval)         
      {
        last = *s;
        cnt = 0;
      }
      s++;
      *n++ = last;  
      *d++ = cnt;
    }

    // left wipe, extend last label if better and increment distance 
    cnt = 0;
    last = bval;
    for (x = rw; x > 0; x--)
    {
      n--;
      d--;
      if (last != bval)
        cnt++;
      if ((*n != bval) && (*d == 0))
      {
        last = *n;
        cnt = 0;
      }
      if ((last != bval) && 
          ((*n == bval) || (*d > cnt) || ((*d == cnt) && (last > *n))))
      {
        *n = last;
        *d = cnt;
      }
    }

    // set up for next line
    s = s1 + rsk;
    d += rsk;
    n += rsk;
  }


  // PASS2: find nearest neighbor and distance in vertical direction
  d = d0; 
  n = n0;
  for (x = rw; x > 0; x--)
  {
    // down wipe, extend last label if better and reset distance 
    cnt = 0;
    last = bval;
    for (y = rh; y > 0; y--)
    {
      if (last != bval)
        cnt++;
      if ((*n != bval) && ((last == bval) || (*d <= cnt)))  
      {
        last = *n;
        cnt = *d;
      }
      if ((last != bval) && 
          ((*n == bval) || (*d > cnt) || ((*d == cnt) && (last > *n))))
      {
        *n = last;
        *d = cnt;
      }
      n += rsk;
      d += rsk;
    }

    // up wipe, extend last label if better and reset distance 
    cnt = 0;
    last = bval;
    for (y = rh; y > 0; y--)
    {
      n -= rsk;
      d -= rsk;
      if (last != bval)
        cnt++;
      if ((*n != bval) && ((last == bval) || (*d <= cnt)))  
      {
        last = *n;
        cnt = *d;
      }
      if ((last != bval) && 
          ((*n == bval) || (*d > cnt) || ((*d == cnt) && (last > *n))))
      {
        *n = last;
        *d = cnt;
      }
    }

    // set up for next column
    n++;
    d++;
  }

  return 1;
}


//= Same as Nearest but specialized for 8 bit labels and distances (saturated).
// when two seeds are equidistant, higher label dominates

int jhcDist::Nearest8 (jhcImg& label, const jhcImg& seed, int bg, jhcImg *rng)
{
  jhcImg *dist = rng;

  // check image sizes, possibly make up scratch array
  if (!label.Valid(1) || !label.SameFormat(seed) || label.SameImg(seed))
    return Fatal("Bad images to jhcDist::Nearest8");
  if ((dist != NULL) && !label.SameSize(*dist))
    return Fatal("Bad range image to jhcDist::Nearest8");
  if (dist == NULL) 
    dist = a1.SetSize(seed);
  label.CopyRoi(seed);
  dist->CopyRoi(seed);

  // set up iteration variables
  int x, y, rw = seed.RoiW(), rh = seed.RoiH();
  UC8 cnt, last, bval = BOUND(bg);
  UL32 roff = seed.RoiOff(), rsk = seed.Line();
  const UC8 *s1, *s = seed.PxlSrc() + roff;
  UC8 *d, *d0 = dist->PxlDest() + roff;
  UC8 *n, *n0 = label.PxlDest() + roff;
  
  // PASS 1: find nearest neighbor and distance in horizontal direction
  d = d0;
  n = n0;
  for (y = rh; y > 0; y--)
  {
    // right wipe, extend last label and increment distance 
    s1 = s;
    cnt = 0;
    last = bval;
    for (x = rw; x > 0; x--)
    {
      if ((last != bval) && (cnt < 255))
        cnt++;
      if (*s != bval)         
      {
        last = *s;
        cnt = 0;
      }
      s++;
      *n++ = last;  
      *d++ = cnt;
    }

    // left wipe, extend last label if better and increment distance 
    cnt = 0;
    last = bval;
    for (x = rw; x > 0; x--)
    {
      n--;
      d--;
      if ((last != bval) && (cnt < 255))
        cnt++;
      if ((*n != bval) && (*d == 0))
      {
        last = *n;
        cnt = 0;
      }
      if ((last != bval) && 
          ((*n == bval) || (*d > cnt) || ((*d == cnt) && (last > *n))))
      {
        *n = last;
        *d = cnt;
      }
    }

    // set up for next line
    s = s1 + rsk;
    d += rsk;
    n += rsk;
  }


  // PASS2: find nearest neighbor and distance in vertical direction
  d = d0; 
  n = n0;
  for (x = rw; x > 0; x--)
  {
    // down wipe, extend last label if better and reset distance 
    cnt = 0;
    last = bval;
    for (y = rh; y > 0; y--)
    {
      if ((last != bval) && (cnt < 255))
        cnt++;
      if ((*n != bval) && ((last == bval) || (*d <= cnt)))  
      {
        last = *n;
        cnt = *d;
      }
      if ((last != bval) && 
          ((*n == bval) || (*d > cnt) || ((*d == cnt) && (last > *n))))
      {
        *n = last;
        *d = cnt;
      }
      n += rsk;
      d += rsk;
    }

    // up wipe, extend last label if better and reset distance 
    cnt = 0;
    last = bval;
    for (y = rh; y > 0; y--)
    {
      n -= rsk;
      d -= rsk;
      if ((last != bval) && (cnt < 255))
        cnt++;
      if ((*n != bval) && ((last == bval) || (*d <= cnt)))  
      {
        last = *n;
        cnt = *d;
      }
      if ((last != bval) && 
          ((*n == bval) || (*d > cnt) || ((*d == cnt) && (last > *n))))
      {
        *n = last;
        *d = cnt;
      }
    }

    // set up for next column
    n++;
    d++;
  }

  return 1;
}


//= Extend each blob over background regions by dmax at most.

int jhcDist::Expand (jhcImg& label, const jhcImg& seed, int dmax, int bg)
{
  if (!label.Valid(1, 2) || !label.SameFormat(seed) || (dmax < 0) || (dmax > 255))
    return Fatal("Bad range image to jhcDist::Expand");

  // get claims and ranges (exploit a1 and a2 from inside Nearest)
  Nearest(label, seed, bg);
  if (label.Valid(2))
  {
    a1.SetSize(seed); 
    a1.Sat8(a2);
  }

  // local variables
  int x, y, rw = label.RoiW(), rh = label.RoiH();
  int nsk = label.RoiSkip() >> 1, dsk = a1.RoiSkip(label);
  const UC8 *d = a1.RoiSrc(label);
  US16 *n = (US16 *) label.RoiDest();

  // invalidate any labels that are too far away
  for (y = rh; y > 0; y--, n += nsk, d += dsk)
    for (x = rw; x > 0; x--, n++, d++)
      if (*d > dmax)
        *n = (US16) bg;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Euclidean Distance                            //
///////////////////////////////////////////////////////////////////////////

//= Version of nearest seed claims using Euclidean distance.
// when two seeds are equidistant, higher label dominates
// default version for labels of 16 bits (2 fields) and 16 bit distances
// about 3x slower than Nearest (5x if distance is returned)

int jhcDist::Voronoi (jhcImg& label, const jhcImg& seed, int bg, jhcImg *rng, 
                      jhcImg *xrng, jhcImg *yrng, jhcImg *rng2)
{
  jhcImg *dist = rng, *xdist = xrng, *ydist = yrng, *sqdist = rng2; 

  // check image sizes, possibly make up scratch arrays
  if (label.Valid(1))
    return Voronoi8(label, seed, bg, rng, xrng, yrng, rng2);
  if (!label.Valid(2) || !label.SameFormat(seed) || label.SameImg(seed))
    return Fatal("Bad images to jhcDist::Voronoi");
  if (((dist != NULL) && !label.SameFormat(*dist)) ||
      ((xdist != NULL) && !label.SameFormat(*xdist)) ||
      ((ydist != NULL) && !label.SameFormat(*ydist)) ||
      ((sqdist != NULL) && !label.SameSize(*sqdist, 4)))
    return Fatal("Bad auxilliary images to jhcDist::Voronoi");
  if (xdist == NULL)
    xdist = a2.SetSize(seed);
  if (ydist == NULL)
    ydist = b2.SetSize(seed);
  if (sqdist == NULL)
    sqdist = a4.SetSize(seed, 4);
  label.CopyRoi(seed);
  xdist->CopyRoi(seed);
  ydist->CopyRoi(seed);
  sqdist->CopyRoi(seed);

  // set up iteration variables
  int x, y, rw = seed.RoiW(), rh = seed.RoiH();
  US16 dx, dy, last, bval = (US16) bg;
  UL32 dx2, dy2, r2, roff = seed.RoiOff();
  UL32 rsk = seed.Line() >> 1, dsk = sqdist->Line() >> 2;
  UL32 *dsq, *dsq0 = (UL32 *)(sqdist->PxlSrc() + sqdist->RoiOff());
  US16 *s1, *s = (US16 *)(seed.PxlSrc() + roff);
  US16 *n, *n0 = (US16 *)(label.PxlSrc() + roff);
  US16 *d, *xoff, *xoff0 = (US16 *)(xdist->PxlSrc() + roff);
  US16 *yoff, *yoff1, *yoff0 = (US16 *)(ydist->PxlSrc() + roff);

  // PASS 1: find nearest neighbor and distance in horizontal direction
  n = n0;
  xoff = xoff0;
  yoff = yoff0;
  dsq = dsq0;
  for (y = rh; y > 0; y--)
  {
    // right wipe, extend last label and increment distance 
    s1 = s;
    yoff1 = yoff;
    last = bval;
    for (x = rw; x > 0; x--)
    {
      if (*s != bval)         
      {
        dx = 0;
        last = *s;
        *n = last;  
        *xoff = 0;
        *yoff = 0;
        *dsq = 0;
      }
      else if (last != bval)
      {
        dx++;
        *n = last;  
        *xoff = dx;
        *yoff = 0;
        *dsq = dx * dx;
      }
      else
      {
        *n = bval;
        *xoff = 0;
        *yoff = 0;
        *dsq = 0;
      }
      s++;
      n++;
      xoff++;
      yoff++;
      dsq++;
    }

    // left wipe, extend last label if better and increment distance 
    last = bval;
    for (x = rw; x > 0; x--)
    {
      n--;
      xoff--;
      dsq--;
      if ((*n != bval) && (*xoff == 0))
      {
        dx = 0;
        last = *n;
      }
      else if (last != bval)
      {
        dx++;
        if ((*n == bval) || (*xoff > dx) || ((*xoff == dx) && (last > *n)))
        {
          *n = last;
          *xoff = dx;
          *dsq = dx * dx;
        }
      }
    }

    // set up for next line
    s = s1 + rsk;
    n += rsk;
    xoff += rsk;
    yoff = yoff1 + rsk;
    dsq += dsk;
  }


  // PASS2: find nearest neighbor and distance in vertical direction
  n = n0;
  xoff = xoff0;
  yoff = yoff0;
  dsq = dsq0; 
  for (x = rw; x > 0; x--)
  {
    // down wipe, extend last label if better and reset distance 
    last = bval;
    for (y = rh; y > 0; y--)
    {      
      if ((*n != bval) && ((last == bval) || (*dsq <= r2)))
      {
        last = *n;
        dx = *xoff;
        dx2 = dx * dx;
        dy = *yoff;
        r2 = *dsq;
      }
      else if (last != bval)
      {
        dy++;
        dy2 = dy * dy;
        r2 = dx2 + dy2;
      }
      if ((last != bval) && 
          ((*n == bval) || (*dsq > r2) || ((*dsq == r2) && (last > *n))))
      {
        *n = last;
        *xoff = dx;
        *yoff = dy;
        *dsq = r2;
      }
      n += rsk;
      xoff += rsk;
      yoff += rsk;
      dsq += dsk;
    }

    // up wipe, extend last label if better and reset distance 
    last = bval;
    for (y = rh; y > 0; y--)
    {
      n -= rsk;
      xoff -= rsk;
      yoff -= rsk;
      dsq -= dsk;
      if ((*n != bval) && ((last == bval) || (*dsq <= r2)))  
      {
        last = *n;
        dx = *xoff;
        dx2 = dx * dx;
        dy = *yoff;
        r2 = *dsq;
      }
      else if (last != bval)
      {
        dy++;
        dy2 = dy * dy;
        r2 = dx2 + dy2;
      }
      if ((last != bval) && 
          ((*n == bval) || (*dsq > r2) || ((*dsq == r2) && (last > *n))))
      {
        *n = last;
        *xoff = dx;
        *yoff = dy;
        *dsq = r2;
      }
    }

    // set up for next column
    n++;
    xoff++;
    yoff++;
    dsq++;
  }


  // PASS 3: find nearest neighbor and distance in horizontal direction again
  n = n0;
  xoff = xoff0;
  yoff = yoff0;
  dsq = dsq0; 
  for (y = rh; y > 0; y--)
  {
    // down right, extend last label if better and reset distance 
    last = bval;
    for (x = rw; x > 0; x--)
    {      
      if ((*n != bval) && ((last == bval) || (*dsq <= r2)))
      {
        last = *n;
        dx = *xoff;
        dy = *yoff;
        dy2 = dy * dy;
        r2 = *dsq;
      }
      else if (last != bval)
      {
        dx++;
        dx2 = dx * dx;
        r2 = dx2 + dy2;
      }
      if ((last != bval) && 
          ((*n == bval) || (*dsq > r2) || ((*dsq == r2) && (last > *n))))
      {
        *n = last;
        *xoff = dx;
        *yoff = dy;
        *dsq = r2;
      }
      n++;
      xoff++;
      yoff++;
      dsq++;
    }

    // left wipe, extend last label if better and reset distance 
    last = bval;
    for (x = rw; x > 0; x--)
    {
      n--;
      xoff--;
      yoff--;
      dsq--;
      if ((*n != bval) && ((last == bval) || (*dsq <= r2)))  
      {
        last = *n;
        dx = *xoff;
        dy = *yoff;
        dy2 = dy * dy;
        r2 = *dsq;
      }
      else if (last != bval)
      {
        dx++;
        dx2 = dx * dx;
        r2 = dx2 + dy2;
      }
      if ((last != bval) && 
          ((*n == bval) || (*dsq > r2) || ((*dsq == r2) && (last > *n))))
      {
        *n = last;
        *xoff = dx;
        *yoff = dy;
        *dsq = r2;
      }
    }

    // set up for next row
    n += rsk;
    xoff += rsk;
    yoff += rsk;
    dsq += dsk;
  }


  // PASS 4: fix up distances
  if (dist != NULL)
  {
    dist->CopyRoi(seed);
    rsk = dist->RoiSkip();
    dsk = sqdist->RoiSkip();
    d = (US16 *)(dist->PxlSrc() + roff);
    dsq = dsq0;
    for (y = rh; y > 0; y--)
    {
      for (x = rw; x > 0; x--)
        *d++ = (US16) ROUND(sqrt((double)(*dsq++)));
      d += rsk;
      dsq += dsk;
    }
  }

  return 1;
}


//= Same as Voronoi but specialized for 8 bit labels and distances (saturated).
// when two seeds are equidistant, higher label dominates
// about 3.4x slower than Nearest8 (5.3x if distance is returned)

int jhcDist::Voronoi8 (jhcImg& label, const jhcImg& seed, int bg, jhcImg *rng, 
                       jhcImg *xrng, jhcImg *yrng, jhcImg *rng2)
{
  jhcImg *dist = rng, *xdist = xrng, *ydist = yrng, *sqdist = rng2; 

  // check image sizes, possibly make up scratch arrays
  if (!label.Valid(1) || !label.SameFormat(seed) || label.SameImg(seed))
    return Fatal("Bad images to jhcDist::Voronoi8");
  if (((dist != NULL) && !label.SameFormat(*dist)) ||
      ((xdist != NULL) && !label.SameFormat(*xdist)) ||
      ((ydist != NULL) && !label.SameFormat(*ydist)) ||
      ((sqdist != NULL) && !label.SameSize(*sqdist, 2)))
    return Fatal("Bad auxilliary images to jhcDist::Voronoi8");
  if (xdist == NULL)
    xdist = a1.SetSize(seed);
  if (ydist == NULL)
    ydist = b1.SetSize(seed);
  if (sqdist == NULL)
    sqdist = a2.SetSize(seed, 2);
  label.CopyRoi(seed);
  xdist->CopyRoi(seed);
  ydist->CopyRoi(seed);
  sqdist->CopyRoi(seed);

  // set up iteration variables
  int x, y, rw = seed.RoiW(), rh = seed.RoiH();
  UC8 dx, dy, last, bval = BOUND(bg);
  US16 dx2, dy2;
  UL32 r2, roff = seed.RoiOff();
  UL32 rsk = seed.Line(), dsk = sqdist->Line() >> 1;
  US16 *dsq, *dsq0 = (US16 *)(sqdist->PxlSrc() + sqdist->RoiOff());
  const UC8 *s1, *s = seed.PxlSrc() + roff;
  UC8 *n, *n0 = label.PxlDest() + roff;
  UC8 *d, *xoff, *xoff0 = xdist->PxlDest() + roff;
  UC8 *yoff, *yoff1, *yoff0 = ydist->PxlDest() + roff;

  // PASS 1: find nearest neighbor and distance in horizontal direction
  n = n0;
  xoff = xoff0;
  yoff = yoff0;
  dsq = dsq0;
  for (y = rh; y > 0; y--)
  {
    // right wipe, extend last label and increment distance 
    s1 = s;
    yoff1 = yoff;
    last = bval;
    for (x = rw; x > 0; x--)
    {
      if (*s != bval)         
      {
        dx = 0;
        last = *s;
        *n = last;  
        *xoff = 0;
        *yoff = 0;
        *dsq = 0;
      }
      else if (last != bval)
      {
        if (dx < 255)
          dx++;
        *n = last;  
        *xoff = dx;
        *yoff = 0;
        *dsq = dx * dx;
      }
      else
      {
        *n = bval;
        *xoff = 0;
        *yoff = 0;
        *dsq = 0;
      }
      s++;
      n++;
      xoff++;
      yoff++;
      dsq++;
    }

    // left wipe, extend last label if better and increment distance 
    last = bval;
    for (x = rw; x > 0; x--)
    {
      n--;
      xoff--;
      dsq--;
      if ((*n != bval) && (*xoff == 0))
      {
        dx = 0;
        last = *n;
      }
      else if (last != bval)
      {
        if (dx < 255)
          dx++;
        if ((*n == bval) || (*xoff > dx) || ((*xoff == dx) && (last > *n)))
        {
          *n = last;
          *xoff = dx;
          *dsq = dx * dx;
        }
      }
    }

    // set up for next line
    s = s1 + rsk;
    n += rsk;
    xoff += rsk;
    yoff = yoff1 + rsk;
    dsq += dsk;
  }


  // PASS2: find nearest neighbor and distance in vertical direction
  n = n0;
  xoff = xoff0;
  yoff = yoff0;
  dsq = dsq0; 
  for (x = rw; x > 0; x--)
  {
    // down wipe, extend last label if better and reset distance 
    last = bval;
    for (y = rh; y > 0; y--)
    {      
      if ((*n != bval) && ((last == bval) || (*dsq <= r2)))
      {
        last = *n;
        dx = *xoff;
        dx2 = dx * dx;
        dy = *yoff;
        r2 = *dsq;
      }
      else if ((last != bval) && (dy < 255))
      {
        dy++;
        dy2 = dy * dy;
        r2 = dx2 + dy2;
      }
      if ((last != bval) && 
          ((*n == bval) || (*dsq > r2) || ((*dsq == r2) && (last > *n))))
      {
        *n = last;
        *xoff = dx;
        *yoff = dy;
        *dsq = (US16) __min(r2, 65535);
      }
      n += rsk;
      xoff += rsk;
      yoff += rsk;
      dsq += dsk;
    }

    // up wipe, extend last label if better and reset distance 
    last = bval;
    for (y = rh; y > 0; y--)
    {
      n -= rsk;
      xoff -= rsk;
      yoff -= rsk;
      dsq -= dsk;
      if ((*n != bval) && ((last == bval) || (*dsq <= r2)))  
      {
        last = *n;
        dx = *xoff;
        dx2 = dx * dx;
        dy = *yoff;
        r2 = *dsq;
      }
      else if ((last != bval) && (dy < 255))
      {
        dy++;
        dy2 = dy * dy;
        r2 = dx2 + dy2;
      }
      if ((last != bval) && 
          ((*n == bval) || (*dsq > r2) || ((*dsq == r2) && (last > *n))))
      {
        *n = last;
        *xoff = dx;
        *yoff = dy;
        *dsq = (US16) __min(r2, 65535);
      }
    }

    // set up for next column
    n++;
    xoff++;
    yoff++;
    dsq++;
  }


  // PASS 3: find nearest neighbor and distance in horizontal direction again
  n = n0;
  xoff = xoff0;
  yoff = yoff0;
  dsq = dsq0; 
  for (y = rh; y > 0; y--)
  {
    // down right, extend last label if better and reset distance 
    last = bval;
    for (x = rw; x > 0; x--)
    {      
      if ((*n != bval) && ((last == bval) || (*dsq <= r2)))
      {
        last = *n;
        dx = *xoff;
        dy = *yoff;
        dy2 = dy * dy;
        r2 = *dsq;
      }
      else if ((last != bval) && (dx < 255))
      {
        dx++;
        dx2 = dx * dx;
        r2 = dx2 + dy2;
      }
      if ((last != bval) && 
          ((*n == bval) || (*dsq > r2) || ((*dsq == r2) && (last > *n))))
      {
        *n = last;
        *xoff = dx;
        *yoff = dy;
        *dsq = (US16) __min(r2, 65535);
      }
      n++;
      xoff++;
      yoff++;
      dsq++;
    }

    // left wipe, extend last label if better and reset distance 
    last = bval;
    for (x = rw; x > 0; x--)
    {
      n--;
      xoff--;
      yoff--;
      dsq--;
      if ((*n != bval) && ((last == bval) || (*dsq <= r2)))  
      {
        last = *n;
        dx = *xoff;
        dy = *yoff;
        dy2 = dy * dy;
        r2 = *dsq;
      }
      else if ((last != bval) && (dx < 255))
      {
        dx++;
        dx2 = dx * dx;
        r2 = dx2 + dy2;
      }
      if ((last != bval) && 
          ((*n == bval) || (*dsq > r2) || ((*dsq == r2) && (last > *n))))
      {
        *n = last;
        *xoff = dx;
        *yoff = dy;
        *dsq = (US16) __min(r2, 65535);
      }
    }

    // set up for next row
    n += rsk;
    xoff += rsk;
    yoff += rsk;
    dsq += dsk;
  }


  // PASS 4: fix up distances
  if (dist != NULL)
  {
    dist->CopyRoi(seed);
    rsk = dist->RoiSkip();
    dsk = sqdist->RoiSkip();
    d = dist->PxlDest() + roff;
    dsq = dsq0;
    for (y = rh; y > 0; y--)
    {
      for (x = rw; x > 0; x--)
      {
        r2 = *dsq++;
        if (r2 >= 65025)
          *d++ = 255;
        else
          *d++ = (UC8) ROUND(sqrt((double) r2));
      }
      d += rsk;
      dsq += dsk;
    }
  }

  return 1;
}


