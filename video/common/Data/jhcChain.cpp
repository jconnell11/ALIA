// jhcChain.cpp : extract and process chains of edge pixels
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2007 IBM Corporation
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
#include "Processing/jhcDraw.h"
#include "Interface/jhcMessage.h"

#include "Data/jhcChain.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Destructor cleans up.

jhcChain::~jhcChain ()
{
  DeallocChain();
}


//= Default constructor, does not make any arrays yet.

jhcChain::jhcChain ()
{
  InitChain(1);
}


//= Make a new contour of the same size as the example given.

jhcChain::jhcChain (jhcChain& ref)
{
  InitChain(1);
  SetSize(ref);
}


//= Make a new contour with the specified number of entries.

jhcChain::jhcChain (int n)
{
  InitChain(1);
  SetSize(n);
}


//= Set number of entries based on number in some pre-existing structure.

jhcChain *jhcChain::SetSize (jhcChain& ref)
{
  return SetSize(ref.total);
}


//= Make up new elemental arrays of a given size.

jhcChain *jhcChain::SetSize (int n)
{
  // sanity check
#ifdef _DEBUG
  if ((n <= 0) || (n > 1000000))
    Pause("Trying to allocate a chain of %ld points!", n);
#endif

  // reallocate if different size requested
  if ((n > 0) && (n != total))
  {
    DeallocChain();
    kind = new int [n];
    xpos = new double [n];
    ypos = new double [n];
    link = new int [n];
    total = n;
  }
  valid = 0;
  return this;
}


//= Allocate temporary array for use in contour tracing.

void jhcChain::AboveSize (int x)
{
  // sanity check
#ifdef _DEBUG
  if ((x <= 0) || (x > 10000))
    Pause("Trying to allocate %ld above points for chain!", x);
#endif

  // reallocate if different size requested
  if ((x > 0) && (x != asz))
  {
    if (above != NULL)
      delete [] above;
    above = new int [x];
    asz = x;
  }
}


//= Clean up any allocated structures.

void jhcChain::DeallocChain ()
{
  if (kind != NULL)
    delete [] kind;
  if (xpos != NULL)
    delete [] xpos;
  if (ypos != NULL)
    delete [] ypos;
  if (link != NULL)
    delete [] link;
  if (above != NULL)
    delete [] above;
  InitChain(0);
}


//= Set up default values for things.

void jhcChain::InitChain (int first)
{
  if (first != 0)
    aspect = 1.0;
  traced = 0;
  total = 0;
  valid = 0;
  asz = 0;
  
  kind = NULL;
  xpos = NULL;
  ypos = NULL;
  link = NULL;
  above = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                            Member Access                              //
///////////////////////////////////////////////////////////////////////////

//= Get index of stored point above this one, -1 if out of bounds.
// bounds checked in debug mode

int jhcChain::AbovePt (int x)
{
#ifdef _DEBUG
  if ((above == NULL) || (x < 0) || (x >= asz))
  {
    Pause("Indexing above point %d in chain (size = %d)!", x, asz);
    return -1;
  }
#endif

  return above[x];
}


//= Save index for point above a given x coordinate.
// bounds checked in debug mode

void jhcChain::AboveSet (int x, int i)
{
#ifdef _DEBUG
  if ((above == NULL) || (x < 0) || (x >= asz))
    Pause("Indexing above point %d in chain (size = %d)!", x, asz);
#endif

  above[x] = i;
}


//= Returns marking for point.
// bounds checked in debug mode

int jhcChain::GetMark (int i) 
{
#ifdef _DEBUG
  return GetMarkChk(i);
#else
  return GetMark0(i);
#endif
}


//= Returns subpixel x coordinate for a point.
// bounds checked in debug mode

double jhcChain::GetX (int i) 
{
#ifdef _DEBUG
  return GetXChk(i);
#else
  return GetX0(i);
#endif
}


//= Returns subpixel y coordinate for a point.
// bounds checked in debug mode

double jhcChain::GetY (int i)
{
#ifdef _DEBUG
  return GetYChk(i);
#else
  return GetY0(i);
#endif
}


//= Returns index of next point linked to this one.
// bounds checked in debug mode

int jhcChain::GetLink (int i)
{
#ifdef _DEBUG
  return GetLinkChk(i);
#else
  return GetLink0(i);
#endif
}


//= Change mark stored with point.
// bounds checked in debug mode

void jhcChain::SetMark (int i, int val) 
{
#ifdef _DEBUG
  SetMarkChk(i, val);
#else
  SetMark0(i, val);
#endif
}


//= Change X coordinate of point, returns 0 if index out of bounds.
// bounds checked in debug mode

void jhcChain::SetX (int i, double val)
{
#ifdef _DEBUG
  SetXChk(i, val);
#else
  SetX0(i, val);
#endif
}


//= Change Y coordinate of point, returns 0 if index out of bounds.
// bounds checked in debug mode

void jhcChain::SetY (int i, double val)
{
#ifdef _DEBUG
  SetYChk(i, val);
#else
  SetY0(i, val);
#endif
}


//= Link this point ot some other, returns 0 if index is out of bounds.
// bounds checked in debug mode

void jhcChain::SetLink (int i, int val)
{
#ifdef _DEBUG
  SetLinkChk(i, val);
#else
  SetLink0(i, val);
#endif
}


//= Checks if index is valid.

int jhcChain::BoundChk (int i, char *fcn)
{
  if ((i < 0) || (i >= total))
  {
#ifdef _DEBUG
    Pause("%s(%ld) of chain beyond %ld!", fcn, i, total);
#endif     
    return 0;
  }
  return 1;
}


//= Returns marking for point, 0 if index out of bounds.

int jhcChain::GetMarkChk (int i, int def) 
{
  if (!BoundChk(i, "GetMark"))
    return def;
  return GetMark0(i);
}


//= Returns subpixel x coordinate for a point, -1 if index out of bounds.

double jhcChain::GetXChk (int i, double def) 
{
  if (!BoundChk(i, "GetX"))
    return def;
  return GetX0(i);
}


//= Returns subpixel y coordinate for a point, -1 if index out of bounds.

double jhcChain::GetYChk (int i, double def)
{
  if (!BoundChk(i, "GetY"))
    return def;
  return GetY0(i);
}


//= Returns index of next point linked to this one, -1 if out of bounds.

int jhcChain::GetLinkChk (int i, int def)
{
  if (!BoundChk(i, "GetLink"))
    return def;
  return GetLink0(i);
}


//= Change mark stored with point, returns 0 if index out of bounds.

int jhcChain::SetMarkChk (int i, int val) 
{
  if (!BoundChk(i, "SetMark"))
    return 0;
  SetMark0(i, val);
  return 1;
}


//= Change X coordinate of point, returns 0 if index out of bounds.

int jhcChain::SetXChk (int i, double val)
{
  if (!BoundChk(i, "SetX"))
    return 0;
  SetX0(i, val);
  return 1;
}


//= Change Y coordinate of point, returns 0 if index out of bounds.

int jhcChain::SetYChk (int i, double val)
{
  if (!BoundChk(i, "SetY"))
    return 0;
  SetY0(i, val);
  return 1;
}


//= Link this point ot some other, returns 0 if index is out of bounds.

int jhcChain::SetLinkChk (int i, int val)
{
  if (!BoundChk(i, "SetLink"))
    return 0;
  SetLink0(i, val);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Simple Manipulation                            //
///////////////////////////////////////////////////////////////////////////

//= Copy data contents of another chain (only in "valid" range).
//= Returns 0 if local arrays are too small.

int jhcChain::Copy (jhcChain& ref)
{
  int i, n = ref.valid;

  if (n > total)
    return 0;

  traced = ref.traced;
  aspect = ref.aspect;
  valid = n;

  for (i = 0; i < n; i++)
  {
    kind[i] = (ref.kind)[i];
    xpos[i] = (ref.xpos)[i];
    ypos[i] = (ref.ypos)[i];
    link[i] = (ref.link)[i];
  }
  return 1;
}


//= Appends a new point by linking it to previous one, returns new index.

int jhcChain::Append (double x, double y, int val)
{
  return AddPt(x, y, val, valid - 1);
}


//= Adds a new point to end of list, returns index for it.

int jhcChain::AddPt (double x, double y, int val, int next)
{
  int i;

  if (valid >= total)
  {
#ifdef _DEBUG
    Pause("More than %ld edge points in chain!", total);
#endif
    return -1;
  }
  i = valid++;
  kind[i] = val;
  xpos[i] = x;
  ypos[i] = y;
  link[i] = -1;
  if (next != -1)
    AddLink(i, next);
  return i;
}


//= Adds a link p1->p2 by setting p1's next pointer.

int jhcChain::AddLink (int p1, int p2)
{
  if ((p1 < 0) || (p2 < 0) || (p1 >= valid) || (p2 >= valid))
  {
#ifdef _DEBUG
    Pause("linking to non-existent node: %ld -> %ld!", p1, p2);
#endif
    return 0;
  }
  link[p1] = p2;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Basic Detection                             //
///////////////////////////////////////////////////////////////////////////

//= Takes a image, thresholds, then makes a list of contour points.
// Sets "valid" to number of contour points found (some doubled)
// Loop entries are:                                           
//   kind[i] = 2 if interior point, 1 if on border          
//      x[i] = x image coordinate                                 
//      y[i] = y image coordinate                                 
//   link[i] = index of next point                          
// All loops are guaranteed to be closed (i.e. no loose ends)

int jhcChain::FindPts (jhcImg& src, int th)
{
  if (!src.Valid(1))
    return Fatal("Bad image to jhcChain::FindPts");

  // general ROI case
  int nw, ne, sw, se; 
  int rx = src.RoiX(), ry = src.RoiY(), rsk = src.RoiSkip();
  int x, y, xlim = rx + src.RoiW(), ylim = ry + src.RoiH();
  int last = -1;
  UC8 thv = BOUND(th);
  const UC8 *p = src.PxlSrc() + src.RoiOff(), *s = p;

  // make temp array to index contour points above current line
  // mark that no points have been found yet
  aspect = src.Ratio();
  AboveSize(src.XDim() + 1);
  valid = 0;

  // handle first line specially, assume outside of ROI is all zero
  //   first pixel in line has SW of zero (from initial SE = 0)
  //   final pixel handled separately to ensure its SE is zero
  // "s" points to start of second line in ROI afterwards
  se = 0;
  for (x = rx; x < xlim; x++)
  {
    sw = se;
    se = ((*s++ > thv) ? 1 : 0);
    last = DoPattern(last, 0, 0, sw, se, x, ry, 1);
  }
  DoPattern(last, 0, 0, se, 0, xlim, ry, 1);  
  s += rsk;

  // scan through bulk of image looking for edges of thresholded regions
  // "p" points to start of last line in ROI afterwards
  for (y = ry + 1; y < ylim; y++)
  {
    // handle first point specially to set mark
    ne = ((*p++ > thv) ? 1 : 0);
    se = ((*s++ > thv) ? 1 : 0);
    last = DoPattern(-1, 0, ne, 0, se, rx, y, 1); 

    // for bulk of line shift pattern over and load new values
    // then add new contour pts and link as appropriate
    for (x = rx + 1; x < xlim; x++)
    {
      nw = ne;
      ne = ((*p++ > thv) ? 1 : 0);
      sw = se;
      se = ((*s++ > thv) ? 1 : 0);
      last = DoPattern(last, nw, ne, sw, se, x, y, 2);
    }

    // handle last point specially to set mark
    DoPattern(last, ne, 0, se, 0, xlim, y, 1); 
    s += rsk;
    p += rsk;
  }

  // handle last line specially, assume outside of ROI is all zero
  ne = 0;
  for (x = rx; x < xlim; x++)
  {
    nw = ne;
    ne = ((*p++ > thv) ? 1 : 0);
    last = DoPattern(last, nw, ne, 0, 0, x, ylim, 1);
  }
  DoPattern(last, ne, 0, 0, 0, xlim, ylim, 1);

  // mark as raw point list
  traced = 0;
  return 1;
}


//= Tests pattern of thresholded pixels to see what to do.
//   returns new value for west (i.e. one of the new points added)
// Examines four pixels around current contour point (aka origin)
//   contour points are defined to fall BETWEEN pixels
//             north
//          NW   |   NE
//     west -- origin        west, origin, north = contour pts
//          SW       SE
//   only binds contour pt at Origin to West and/or North neighbor         
//   patterns are composed of thresholded grayscale pixels
//   centers of diagonal patterns are assigned to both contours
// North and south are swapped for Windows DIBs (south at top)

int jhcChain::DoPattern (int west, int nw, int ne, int sw, int se, 
                          int x, int y, int mark)
{ 
  int saved = -1, origin = -1;

  // see if new region encountered
  if (nw == 0)
  {
    if (sw == 1)
    {
      // Basic pattern:  0   X 
      //                 --*
      //                 1   X                       
      // link a new contour point (*) to West neighbor 
      // save point index to propagate down (in case new pt added next)
      origin = AddPt(x, y, mark, west);
      saved = origin;

      if (ne == 1) 
      {
        // Basic pattern:  0 | 1              0 | 1
        //                 --+     or maybe     *
        //                 1   X              1   0
        // link North neighbor to newest contour point (+)   
        // for diagonal case, create a duplicate point (*)
        if (se == 0)
          origin = AddPt(x, y, mark, -1); 
        AddLink(AbovePt(x), origin);
      }
    }

    else if (ne == 1)
    {
      // Basic pattern:  0 | 1                   
      //                   *     
      //                 0   X                        
      // link North neighbor to new contour point   
      origin = AddPt(x, y, mark, -1);
      AddLink(AbovePt(x), origin);
    }

    else if (se == 1)
    // Basic pattern:  0   0                        
    //                   *
    //                 0   1                        
    // create new contour point with no links     
    origin = AddPt(x, y, mark, -1);
  }

  else  // nw == 1, see if old region ending
  {
    if (sw == 0)
    {
      // Basic pattern:  1   X
      //                 --*                     
      //                 0   X                     
      // link West neighbor to new contour point 
      origin = AddPt(x, y, mark, -1);
      AddLink(west, origin);
      if (ne == 0)
      {
        // Basic pattern:  1 | 0              1 | 0         
        //                 --+     or maybe     * 
        //                 0   X              0   1        
        // link new contour point to North neighbor    
        // for diagonal case, create a duplicate point        
        AddLink(origin, AbovePt(x));
        if (se != 0.0)
          origin = AddPt(x, y, mark, -1);
      }
    }

    else if (ne == 0)
      // Basic pattern:  1 | 0                        
      //                   *
      //                 1   X                        
      // link new contour point to North neighbor   
      origin = AddPt(x, y, mark, AbovePt(x));

    else if (se == 0)
      // Basic pattern:  1   1                        
      //                   *
      //                 1   0                        
      // create new contour point with no links     
      origin = AddPt(x, y, mark, -1);         
  }

  // record point indexes to propagate right and down (may be different)
  if (saved >= 0)
    AboveSet(x, saved);
  else if (origin >= 0)
    AboveSet(x, origin);
  return origin;
}


///////////////////////////////////////////////////////////////////////////
//                        Processing and Analysis                        //
///////////////////////////////////////////////////////////////////////////

//= Takes a set of linked contour points and makes a list of loops.
// Negates interior/border "kind" field in input chain ("pts")
// Can subsample original contour and throw out small loops.
// First pixel in each segment marked with type (1 = loop, 2 = open)
//   all other pixels in either kind of segment marked with zeroes

int jhcChain::Trace (jhcChain& raw, int samp, int size, int no_bd)
{
  int len, phase, do_ej = no_bd;
  int m, n = raw.valid, search = -1;
  int cstart, ppt, cpt; 
  int base, prev; 

  // can not perform operation in-place on same array
  if (&raw == this)
    return Fatal("Bad chain to jhcChain::Trace");

  // search is the place where loop tracing started
  // cstart is the first boundary pixel found on the loop
  aspect = raw.aspect;
  valid = 0;
  while (1)
  {
    // STEP 1 - trace all contours coming in from a boundary first
    // loop gives contour beginning, start is where to search next
    cstart = -1;
    if (do_ej > 0)
    {
      // start by looking for an unmarked boundary pixel 
      while (++search < n)
      {
        if (raw.GetMark(search) != 1) 
          continue;

        // now advance until non-border or already marked point found
        cpt = search;
        while (cpt >= 0)
        {
          m = raw.GetMark(cpt);
          if (m == 2)
            cstart = cpt;
          if (m != 1)
            break;

          // mark all boundary pixels seen so not examined again
          raw.SetMark(cpt, -1);
          cpt = raw.GetLink(cpt);
        }

        // stop searching as soon as first entry point found
        if (cstart >= 0)
          break;
      }

      // if no entry point found, switch mode and restart search
      if (cstart < 0)
      {
        do_ej = 0;
        search = -1;
      }
    }


    // STEP 2 - look for totally enclosed contours 
    // exit when no more beginning points can be found
    if (cstart < 0)
    {
      while (++search < n)
        if (raw.GetMark(search) == 2) 
        {
          cstart = search;
          break;
        }
    }
    if (cstart < 0)
      break;


    // STEP 3 - copy loop into new array
    prev = -1;
    base = valid;
    cpt = cstart;
    m = 1;
    len = 0;
    phase = 0;
    while (cpt >= 0)
    {
      // only record some pts, link each back to previous pt
      len++;
      if (--phase <= 0) 
      {
        prev = AddPt(raw.GetX(cpt), raw.GetY(cpt), 0, prev);
        phase = samp;
      }

      // mark point so not retraced then find next point
      // if back to beginning, link up and mark as a closed loop
      raw.SetMark(cpt, -m);
      ppt = cpt;
      cpt = raw.GetLink(cpt);
      if (cpt == cstart)
      {
        SetMark(base, 2);
        SetLink(base, prev); 
        break;
      }

      // otherwise get type of point and see if boudnary hit
      //   possibly mark loop type, record last point, then exit
      m = raw.GetMark(cpt);
      if ((no_bd != 0) && (m != 2))
      {
        SetMark(base, 1);
        if (ppt != cstart)
          AddPt(raw.GetX(ppt), raw.GetY(ppt), 0, prev); 
        break;
      }
    }

    // erase loops which are too small
    if (len < size)
      valid = base;
  }

  // mark as sorted point list
  traced = 1;
  return 1;
}


//= Moves each contour point "frac" (can be negative) of the.
//   way toward the centroid of the two adjacent points      
// Never moves endpoints of open loops.                     
// Assumes chains have been sorted and packed by "Trace". 

int jhcChain::Relax (double frac, int passes)
{
  int i;
  double hf = 0.5 * frac;
  int current, next, start;
  double x2, y2, lx, ly, nx, ny, cx = 0, cy = 0;

  if ((traced == 0) || (passes <= 0))
    return 0;

  // do smoothing several times if requested
  for (i = passes; i > 0; i--)
  {
    next = 0;
    while (next < valid)
    {
      // partially intialize window
      start = next;
      current = -1;
      nx = GetX(next);
      ny = GetY(next);

      while (1)
      {
        // if window of 3 points has been fully initialized then
        // move current point toward average of two flanking points
        if (current > start)
        {
          SetX(current, cx + hf * (lx - (2.0 * cx) + nx));
          SetY(current, cy + hf * (ly - (2.0 * cy) + ny));
        }
        else if (current == start)
        {
          // save unaltered coordinates of second point for closed contours
          x2 = nx;
          y2 = ny;
        }

        // shuffle old pointers to get new window 
        lx = cx;
        ly = cy;
        current = next;
        cx = nx;
        cy = ny;

        // read in new point, check if start of a new run is marked
        if (++next >= valid)
          break;
        if (GetMark(next) != 0)
          break;
        nx = GetX(next);
        ny = GetY(next);
      }

      // check if contour was closed 
      if ((GetMark(start) == 2) && (current >= start))
      {
        // smooth last point based on previous and first
        nx = GetX(start);
        ny = GetY(start);      
        SetX(current, cx + hf * (lx - (2.0 * cx) + nx));
        SetY(current, cy + hf * (ly - (2.0 * cy) + ny));

        // smooth first point based on last and second (saved)
        SetX(start, nx + hf * (cx - (2.0 * nx) + x2));
        SetY(start, ny + hf * (cy - (2.0 * ny) + y2));
      }
    }
  }
  return 1;
}


//= Computes 1/R for each contour segment and histograms results.
// Vote strength proportional to each contour segment length.                   
// Setup: <p0> ---A---> <p1> ---B---> <p2>
//   let B' be B rotated 90 degrees CCW, i.e. (dx dy) -> (-dy dx)
//     A dot B' = |A| |B'| cos(theta + 90) = sin(theta)
//   using approximation for A nearly parallel to B
//     A dot B' / |A| |B'| = sin(theta) = theta
//     1/R = K = d(theta) / d(s) = ( A dot B' / |A| |B'|) / |B'|
// Coordinates need to be floats and a lot of smoothing should be used.
// Assumes chains have been sorted and packed by "Trace". 
// fix for aspect?
int jhcChain::HistTurn (jhcArr& h, double rlo, int squash)
{
  int bin, top = h.Size() - 1; 
  int start, current, next = 0;
  double cx, cy, cldx, cldy, nx, ny, ncdx, ncdy;
  double off, sc, cl_invl, nc_invl, nc_len, k;

double lx, ly;
double cl_len;

  if (traced == 0) 
    return 0;

  // set up so 1/R increments bin = sc * (1/R) + off
  // flat segments show up at bin top/4, rlo shows up in bin "top"
  h.Fill(0);
  off = 0.25 * (top + 1);
  sc = (top - off) * rlo;

  while (next < valid)
  {
    // partially intialize window
    start = next;
    current = -1;
    nx = GetX(next);
    ny = GetY(next);

cx = 0.0;
cy = 0.0;

    // LOOP FOR N PTS
    while (1)
    {
      // if window of 3 points has been fully initialized then
      // compute inverse radius approximation and histogram 
      if ((current > start) && (nc_len > 0.0))
      {
        k = (cldy * ncdx - cldx * ncdy) * nc_invl * nc_invl * cl_invl;
        bin = ROUND(sc * k + off);
        if (squash != 0)
          bin = __max(0, __min(bin, top));

// Pause("(%f %f) -> (%f %f) -> (%f %f)\nlens = %f %f, curve = %f (rad = %f) @ %d", 
//       lx, ly, cx, cy, nx, ny, cl_len, nc_len, k, 1.0 / k, bin);

        if ((bin >= 0) && (bin <= top))
          h.AInc(bin, (int)(10.0 * nc_len + 0.5));
      }

      // shuffle old pointers to get new window 
  lx = cx;
  ly = cy;
  cl_len = nc_len;
      cldx = ncdx;
      cldy = ncdy;
      cl_invl = nc_invl;
      current = next;
      cx = nx;
      cy = ny;

      // read in new point, check if start of a new run is marked
      if (++next >= valid)
        break;
      if (GetMark(next) != 0)
        break;
      nx = GetX(next);
      ny = GetY(next);

      // compute difference from next to current and normalizing length
      ncdx = nx - cx;
      ncdy = ny - cy;
      nc_len = ncdx * ncdx + ncdy * ncdy;
      if (nc_len <= 0.0)
      {
        nc_len = 0.0;
        nc_invl = 100000.0;  // just a very big number
      }
      else
      {
        nc_len = sqrt(nc_len);
        nc_invl = 1.0 / nc_len;
      }
    }

    // FINAL PT - after finished tracing see if contour was closed 
    if ((GetMark(start) == 2) && (current >= start))
    {
      // compute difference from final point to starting point
      ncdx = GetX(start) - cx;
      ncdy = GetY(start) - cy;
      nc_len = ncdx * ncdx + ncdy * ncdy;
      if (nc_len > 0.0)
      {
        nc_len = sqrt(nc_len);
        nc_invl = 1.0 / nc_len;

        // compute inverse radius approximation and histogram 
        k = (cldy * ncdx - cldx * ncdy) * nc_invl * nc_invl * cl_invl;
        bin = ROUND(sc * k + off);
        if (squash != 0)
          bin = __max(0, __min(bin, top));
        if ((bin >= 0) && (bin <= top))
          h.AInc(bin, (int)(10.0 * nc_len + 0.5));
      }
    }
  }

  // normalize final histogram 
  h.Normalize(255);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Visualization                               //
///////////////////////////////////////////////////////////////////////////

//= Show points (but maybe not edges) on an image in specified color.
// if squash is non-zero, points outside image mapped to boundary
// shows points with marks greater than klim (or all if klim is zero)

int jhcChain::DrawPts (jhcImg& dest, int klim, int squash, int r, int g, int b)
{
  int i, k; 
  int x, y, f = 0, nf = dest.Fields();
  UC8 red = BOUND(r), grn = BOUND(g), blu = BOUND(b);
  jhcDraw gr;

  if (!dest.Valid(1) && !dest.Valid(3))
    return Fatal("Bad image to jhcChain::DrawPts");

  if ((r < 0) && (nf == 3))
    gr.Color8(&red, &grn, &blu, -r);

  for (i = 0; i < valid; i++)              
  {
    if ((k = GetMark(i)) < 0)
      k = -k;
    if (k >= klim)
    {
      x = ROUND(GetX(i));
      y = ROUND(GetY(i));
      if (dest.ClipCoords(&x, &y, &f) > 0)
        if (squash <= 0)
          continue;
      if (nf == 1)
        dest.ASet(x, y, 0, red);
      else
        dest.ASetCol(x, y, red, grn, blu);
    }
  }
  return 1;
}


//= Connect points by drawing lines on an image in specified color.
// can accept negative red to mean autoselect of color

int jhcChain::DrawSegs (jhcImg& dest, int r, int g, int b)
{
  int i, npt; 
  jhcDraw gr;

  for (i = 0; i < valid; i++)              
    if ((npt = GetLink(i)) >= 0)
      gr.DrawLine(dest, 
                  GetX(i), GetY(i), GetX(npt), GetY(npt), 
                  2, r, g, b);
  return 1;
}


