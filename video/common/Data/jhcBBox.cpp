// jhcBBox.cpp : bounding box tracking
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2017 IBM Corporation
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

#include <stdlib.h>
#include "Interface/jhcMessage.h"
#include "Processing/jhcDraw.h"

#include "Data/jhcBBox.h"


///////////////////////////////////////////////////////////////////////////
//                        Construction and Copying                       //
///////////////////////////////////////////////////////////////////////////

//= Destructor cleans up.

jhcBBox::~jhcBBox ()
{
  DeallocBBox();
}


//= Default constructor.

jhcBBox::jhcBBox ()
{
  InitBBox();
}


//= Base size on the size of some other bound box list.

jhcBBox::jhcBBox (const jhcBBox& ref)
{
  InitBBox();
  SetSize(ref);
}


//= Make a list of a specific size.

jhcBBox::jhcBBox (int ni)
{
  InitBBox();
  SetSize(ni);
}


//= Make self same size as another.

void jhcBBox::SetSize (const jhcBBox& ref)
{
  SetSize(ref.total);
}


//= Allocate internal arrays of the correct size.

void jhcBBox::SetSize (int ni)
{
  int i;

  // sanity check
#ifdef _DEBUG
  if ((ni <= 0) || (ni > 100000))
    Pause("jhcBBox::SetSize - Trying to allocate %ld bound boxes!", ni);
#endif

  // check if current arrays can be reused
  if (ni != total)
  {
    if (total > 0)
      DeallocBBox();

    // do core allocation  
    status = new int [ni];
    count  = new int [ni];
    pixels = new int [ni];
    aux = new double [ni];
    vx  = new double [ni];
    vy  = new double [ni];
    vz  = new double [ni];

    // do ROI allocation
    xlo = new int [ni];
    xhi = new int [ni];
    ylo = new int [ni];
    yhi = new int [ni];
    items = new jhcRoi [ni];
    total = ni;

    // initialize everything
    for (i = 0; i < ni; i++)
    {
      status[i] = 0;
      count[i]  = 0;
      pixels[i] = 0;
      aux[i] = 0.0;
      vx[i]  = 0.0;
      vy[i]  = 0.0;
      vz[i]  = 0.0;
      xlo[i] = 0;
      xhi[i] = 0;
      ylo[i] = 0;
      yhi[i] = 0;
    }

    // check that it all succeeded
    if (ni > 0)
      if ((status == NULL) || (count == NULL) || (pixels == NULL) || (aux == NULL) || 
          (vx == NULL) || (vy == NULL) || (vz == NULL) || (items == NULL) ||
          (xlo == NULL) || (xhi == NULL) || (ylo == NULL) || (yhi == NULL))
        Fatal("jhcBBox::SetSize - Array allocation failed!");   
  }
  valid = 0;
}


//= Deallocates all structures.

void jhcBBox::DeallocBBox ()
{
  delete [] items;
  delete [] yhi;
  delete [] ylo;
  delete [] xhi;
  delete [] xlo;
  delete [] vz;
  delete [] vy;
  delete [] vx;
  delete [] aux;
  delete [] pixels;
  delete [] count;
  delete [] status;
  InitBBox();
}


//= Clear all structures.

void jhcBBox::InitBBox ()
{
  fxlo  = 0;
  fxhi  = 0;
  fylo  = 0;
  fyhi  = 0;
  total = 0;
  valid = 0;
  status = NULL;
  count  = NULL;
  pixels = NULL;
  aux    = NULL;
  vx     = NULL;
  vy     = NULL;
  vz     = NULL;
  xlo    = NULL;
  xhi    = NULL;
  ylo    = NULL;
  yhi    = NULL;
  items  = NULL;
}
  

///////////////////////////////////////////////////////////////////////////
//                            Raw Data Access                            //
///////////////////////////////////////////////////////////////////////////

//= Get the Roi structure for some entry.
// Have to be careful about dangling pointers (if BBox deleted).

jhcRoi *jhcBBox::GetRoi (int index) 
{
  if ((index < 0) || (index >= total))
    return NULL;
  return(items + index);
}


//= Get a const pointer to the ROI for some entry.

const jhcRoi *jhcBBox::ReadRoi (int index) const
{
  if ((index < 0) || (index >= total))
    return NULL;
  return(items + index);
}


//= Copy the parameters for some entry into a supplied ROI structure.

int jhcBBox::GetRoi (jhcRoi& dest, int index) const
{
  if ((index < 0) || (index >= total))
    return 0;
  dest.CopyRoi(items[index]);
  return 1;
}


//= Convenience function returns integer middle X of bounding box.

int jhcBBox::BoxMidX (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return ROUND(sc * items[index].RoiMidX());
}


//= Convenience function returns integer middle Y of bounding box.

int jhcBBox::BoxMidY (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return ROUND(sc * items[index].RoiMidY());
}


//= Convenience function returns floating point middle X of bounding box.

double jhcBBox::BoxAvgX (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return(sc * items[index].RoiAvgX());
}


//= Convenience function returns floating point middle Y of bounding box.

double jhcBBox::BoxAvgY (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return(sc * items[index].RoiAvgY());
}


//= Convenience function returns left X of bounding box.

double jhcBBox::BoxLf (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return(sc * items[index].RoiX());
}


//= Convenience function returns right X of bounding box.

double jhcBBox::BoxRt (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return(sc * items[index].RoiLimX());
}


//= Convenience function returns bottom Y of bounding box.

double jhcBBox::BoxBot (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return(sc * items[index].RoiY());
}


//= Convenience function returns top Y of bounding box.

double jhcBBox::BoxTop (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return(sc * items[index].RoiLimY());
}


//= Convenience function returns width of bounding box.

double jhcBBox::BoxW (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return(sc * items[index].RoiW());
}


//= Convenience function returns height of bounding box.

double jhcBBox::BoxH (int index, double sc) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return(sc * items[index].RoiH());
}


//= Get the real area associated with an entry.

int jhcBBox::PixelCnt (int index) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return pixels[index];
}


///////////////////////////////////////////////////////////////////////////
//                            List Manipulation                          //
///////////////////////////////////////////////////////////////////////////

//= Get the status associated with an entry.

int jhcBBox::GetStatus (int index) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return status[index];
}


//= Get the count associated with an entry.

int jhcBBox::GetCount (int index) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return count[index];
}


//= Get the auxilliary value associated with an entry.

double jhcBBox::GetAux (int index) const
{
  if ((index < 0) || (index >= total))
    return 0;
  return aux[index];
}


//= Get the velocities associated with an entry.
// Returns 0 if index is out of bounds (variable values unchanged).

int jhcBBox::GetSpeed (double *x, double *y, int index) const
{
  if ((index < 0) || (index >= total))
    return 0;
  *x = vx[index];
  *y = vy[index];
  return 1;
}


//= Get the zooming factor associated with an entry.
// Returns 0.0 if the index is out of bounds.

double jhcBBox::GetZoom (int index) const
{
  if ((index < 0) || (index >= total))
    return 0.0;
  return vz[index];
}


//= Set the status field for a particular item.
// updates "valid" to include this item (even if status zero)

int jhcBBox::SetStatus (int index, int val)
{
  if ((index < 0) || (index >= total))
    return 0;
  status[index] = val;
  valid = __max(valid, index + 1);
  return 1;
}


//= Set the count field for a particular item.

int jhcBBox::SetCount (int index, int val)
{
  if ((index < 0) || (index >= total))
    return 0;
  count[index] = val;
  return 1;
}


//= Set the auxilliary field for a particular item.

int jhcBBox::SetAux (int index, double val)
{
  if ((index < 0) || (index >= total))
    return 0;
  aux[index] = val;
  return 1;
}


//= Set the velocity fields for a particular item.

int jhcBBox::SetSpeed (int index, double x, double y)
{
  if ((index < 0) || (index >= total))
    return 0;
  vx[index] = x;
  vy[index] = y;
  return 1;
}


//= Set the predictive zooming factor for an item.

int jhcBBox::SetZoom (int index, double z)
{
  if ((index < 0) || (index >= total))
    return 0;
  vz[index] = z;
  return 1;
}


//= Initialize all fields of an entry.
// default velocity is no translation and no zoom (vz = 1)

int jhcBBox::ClearItem (int index)
{
  if ((index < 0) || (index >= total))
    return 0;

  status[index] = 0;
  count[index]  = 0;
  aux[index] = 0.0;
  vx[index]  = 0.0;
  vy[index]  = 0.0;
  vz[index]  = 1.0;
  items[index].ClearRoi();
  return 1;
}


//= Copy values from one list to another.
// expands "valid" range if necessary (even if status is zero)

int jhcBBox::CopyItem (int index, const jhcBBox& src, int si)
{
  if ((index < 0) || (index >= total) || (si < 0) || (si >= src.total))
    return 0;

  // copy current bounding box and original box (for PassHatched)
  items[index].CopyRoi((src.items)[si]);
  xlo[index] = (src.xlo)[si];
  xhi[index] = (src.xhi)[si];
  ylo[index] = (src.ylo)[si];
  yhi[index] = (src.yhi)[si];

  // copy velocity (for tracking)
  vx[index] = (src.vx)[si];
  vy[index] = (src.vy)[si];
  vz[index] = (src.vz)[si];

  // copy other indicators
  status[index] = (src.status)[si];
  count[index]  = (src.count)[si];
  aux[index]    = (src.aux)[si];

  // possibly extend current list size
  valid = __max(valid, index + 1);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Feature Extraction                           //
///////////////////////////////////////////////////////////////////////////

//= Fills bbox list with parameters based on segmented image.
// Ignores blobs labelled as zero (presumably the background).
// Records intial ranges of pixels in xlo, xhi, ylo, yhi variables
// Sets "valid" to reflect range of entries filled.
// unused indices have status "val0"
// returns total span of active boxes (scan 1 to n-1 checking status > 0)

int jhcBBox::FindBBox (const jhcImg& src, int val0)
{
  int i, x, y, ssk2 = src.RoiSkip() >> 1, last = -1;
  const US16 *s = (const US16 *) src.RoiSrc();

  // check for connected component image
  if (!src.Valid(2))
    return Fatal("Bad image to jhcBBox::FindBBox");

  // record overall limits of analysis region
  fxlo = src.RoiX();
  fxhi = src.RoiLimX();
  fylo = src.RoiY();
  fyhi = src.RoiLimY();

  // clear areas
  for (i = 0; i < total; i++)
    pixels[i] = 0;
  ResetAll(val0);

  // keep stretching bbox for each pixel encountered
  for (y = fylo; y <= fyhi; y++, s += ssk2)
    for (x = fxlo; x <= fxhi; x++, s++)
      if ((*s > 0) && (*s < total))
      {
        // increase box area
        i = *s;
        pixels[i] += 1;

        if (status[i] <= 0)          // used to check RoiArea()
        {
          // component not seen previously
          xlo[i] = x;
          xhi[i] = x;
          ylo[i] = y;
          yhi[i] = y;
          status[i] = 1;
          if (i > last)
            last = i;
        }
        else
        {
          // update statistics (scanning up)
          xlo[i] = __min(x, xlo[i]);
          xhi[i] = __max(x, xhi[i]);
          yhi[i] = __max(y, yhi[i]);
        }
      }

  // copy limits back to ROIs (1.8x faster than StretchRoi version)
  valid = last + 1;
  for (i = 1; i < valid; i++)
    if (status[i] > 0)
      items[i].SetRoi(xlo[i], ylo[i], xhi[i] - xlo[i] + 1, yhi[i] - ylo[i] + 1);
  return valid;
}


//= Scan component image from top stopping when box reasonably filled.
// unused indices have status -2, blobs with no heads are -1, heads are +1
// useful for locating heads of people, can artifical expand box around eyes

int jhcBBox::TopBoxes (const jhcImg& src, double f0, double f1, double mag) 
{
  int i, x, y, prev, dy, ssk2 = src.RoiW() + (src.Line() >> 1), last = -1;
  double mid, dy2;
  const US16 *s = (US16 *) src.RoiSrc(src.RoiX(), src.RoiLimY());

  // check for connected component image
  if (!src.Valid(2))
    return Fatal("Bad image to jhcBBox::TopBoxes");

  // record overall limits of analysis region 
  fxlo = src.RoiX();
  fxhi = src.RoiLimX();
  fylo = src.RoiY();
  fyhi = src.RoiLimY();

  // clear areas
  for (i = 0; i < total; i++)
    pixels[i] = 0;
  ResetAll(-2);

  // scan from the top down
  for (y = fyhi; y >= fylo; y--, s -= ssk2)
  {
    prev = 0;
    for (x = fxlo; x <= fxhi; x++, s++)
    {
      // see if in some unfinished component
      if ((*s > 0) && (*s < total))
      {
        // increase box area
        i = *s;
        pixels[i] += 1;

        if (status[i] < -1)
        {
          // component not seen previously
          xlo[i] = x;
          xhi[i] = x;
          ylo[i] = y;
          yhi[i] = y;
          status[i] = -1;
          if (i > last)
            last = i;
        }
        else if (status[i] < 1)
        {
          // update statistics (scanning down)
          xlo[i] = __min(x, xlo[i]);
          xhi[i] = __max(x, xhi[i]);
          ylo[i] = __min(y, ylo[i]);
        }
      }

      // check any ending component for head shape
      if ((prev > 0) && (prev < total) && (status[prev] == -1) && 
          ((*s != prev) || (x == fxhi)))
      {
        dy = yhi[prev] - y + 1;
        if ((pixels[prev] > ROUND(f0 * dy * dy)) && 
            (pixels[prev] < ROUND(f1 * dy * dy)))
        {
          // re-center bounding box on current line
          mid = 0.5 * (xlo[prev] + xhi[prev]);
          dy2 = mag * 0.5 * dy;
          xlo[prev] = ROUND(mid - dy2);
          xhi[prev] = ROUND(mid + dy2);
          ylo[prev] = ROUND(y - dy2);
          yhi[prev] = ROUND(y + dy2);
          status[prev] = 1;                  // mark as finished
        }
      }
      prev = *s;
    }
  }

  // copy limits back to ROIs (1.8x faster than StretchRoi version)
  valid = last + 1;
  for (i = 1; i < valid; i++)
    if (status[i] > 0)
      items[i].SetRoi(xlo[i], ylo[i], xhi[i] - xlo[i] + 1, yhi[i] - ylo[i] + 1);
  return 1;
}


//= See how many bboxes have status at or above some threshold.
// As a side-effect can shrink "valid" to end of current list

int jhcBBox::CountOver (int sth, int shrink)
{
  int i, last = -1, ans = 0;

  for (i = 1; i < valid; i++)
  {
    if (status[i] >= sth)
      ans++;
    if (status[i] > 0)
      last = i;
  }
  if (shrink > 0)
    valid = last + 1;
  return ans;
}


//= See how many bboxes have status at or above some threshold.

int jhcBBox::CountOver (int sth) const
{
  int i, last = -1, ans = 0;

  for (i = 1; i < valid; i++)
  {
    if (status[i] >= sth)
      ans++;
    if (status[i] > 0)
      last = i;
  }
  return ans;
}


//= Find the index of the nth entry above the given threshold.

int jhcBBox::IndexOver (int n, int sth) const
{
  int i, cnt = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth)
      if (cnt++ >= n)
        break;
  if (i >= valid)
    return -1;
  return i;
}


///////////////////////////////////////////////////////////////////////////
//                                Selection                              //
///////////////////////////////////////////////////////////////////////////

//= Find valid blob with largest bounding box and tell area.
// Can restrict to bboxes with status at or above specified value.

int jhcBBox::MaxAreaBB (int sth) const
{
  int i, val, best = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
    {
      val = items[i].RoiArea();
      if (val > best)
        best = val;
    }
  return best;
}


//= Find valid blob with largest bounding box and tell index.

int jhcBBox::MaxBB (int sth) const
{
  int i, val, win = -1, best = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
    {
      val = items[i].RoiArea();
      if (val > best)
      {
        best = val;
        win = i;
      }
    }
  return win;
}


//= Find valid blob with largest true area and tell index.

int jhcBBox::Biggest (int sth) const
{
  int i, val, best = 0, win = -1;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
    {
      val = pixels[i];
      if (val > best)
      {
        best = val;
        win = i;
      }
    }
  return win;
}


//= Find valid blob with smallest true area and tell index.

int jhcBBox::Smallest (int sth) const
{
  int i, val, best, win = -1;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
    {
      val = pixels[i];
      if ((win < 0) || (val < best))
      {
        best = val;
        win = i;
      }
    }
  return win;
}


//= Find valid box center closest to right side.
// Can restrict to bboxes with status at or above specified value.
// returns index of winner

int jhcBBox::RightBB (int sth) const
{
  int i, val, win = -1, best = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
      {
        val = items[i].RoiMidX();
        if ((win < 0) || (val > best))
        {
          best = val;
          win = i;
        }
      }
  return win;
}


//= Find valid box center closest to left side.
// Can restrict to bboxes with status at or above specified value.
// returns index of winner

int jhcBBox::LeftBB (int sth) const
{
  int i, val, win = -1, best = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
      {
        val = items[i].RoiMidX();
        if ((win < 0) || (val < best))
        {
          best = val;
          win = i;
        }
      }
  return win;
}


//= Find valid box center closest to top of image.
// Can restrict to bboxes with status at or above specified value.
// returns index of winner

int jhcBBox::TopBB (int sth) const
{
  int i, val, win = -1, best = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
      {
        val = items[i].RoiMidY();
        if ((win < 0) || (val > best))
        {
          best = val;
          win = i;
        }
      }
  return win;
}


//= Find valid box center closest to bottom of image.
// Can restrict to bboxes with status at or above specified value.
// returns index of winner

int jhcBBox::BottomBB (int sth) const
{
  int i, val, win = -1, best = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
      {
        val = items[i].RoiMidY();
        if ((win < 0) || (val < best))
        {
          best = val;
          win = i;
        }
      }
  return win;
}


//= Find valid box with lower edge closest to bottom of image.
// Can restrict to bboxes with status at or above specified value.
// returns index of winner

int jhcBBox::GapBottomBB (int sth) const
{
  int i, val, win = -1, best = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
      {
        val = items[i].RoiY();
        if ((win < 0) || (val < best))
        {
          best = val;
          win = i;
        }
      }
  return win;
}


//= Find valid box center closest to some point (corner, middle, etc.).
// Can restrict to bboxes with status at or above specified value.
// returns index of winner

int jhcBBox::ClosestBB (int x, int y, int sth) const
{
  int dx, dy, i, val, win = -1, best = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth) 
      {
        dx = items[i].RoiMidX() - x;
        dy = items[i].RoiMidY() - y;
        val = dx * dx + dy * dy;
        if ((win < 0) || (val < best))
        {
          best = val;
          win = i;
        }
      }
  return win;
}


//= For a given bbox, find element in list that matches "best" (yields
//   the greatest overlap area compared to the larger of the two areas).
// Also takes a threshold value for minimum acceptable overlap.

int jhcBBox::BestOverlap (const jhcRoi& target, double oth) const
{
  int i, winner = -1;
  int a, lap, ta = target.RoiArea();
  double val, best = 0.0; 
  jhcRoi *entry = items + 1;

  if (ta <= 0)
    return 0;

  for (i = 1; i < valid; i++, entry++)
    if (status[i] > 0)
    {
      a = entry->RoiArea();
      lap = entry->RoiOverlap(target);
      if (a > ta)
        val = lap / (double) a;
      else
        val = lap / (double) ta;
      if ((val >= oth) && (val > best))
      {
        best = val;
        winner = i;
      }
    }
  return winner;
}


///////////////////////////////////////////////////////////////////////////
//                              List Combination                         //
///////////////////////////////////////////////////////////////////////////

//= Copies basic data from one list to another (no allocation involved).
// Only copies from src's "valid" range, adjusts own "valid" as needed.

void jhcBBox::CopyAll (const jhcBBox& src)
{
  int i, common = __min(total, src.valid);

  if (this == &src)
    return;

  valid = 0;
  for (i = 0; i < common; i++)
    CopyItem(i, src, i);
  for (i = common; i < total; i++)
    status[i] = 0;
}


//= Copy new non-zero marked bboxes into list at lowest free slots.
// copies status, count, mark, velocities, AND initial ranges
// skips entry zero so boxes are never drawn with color zero.

void jhcBBox::AddItems (const jhcBBox& xtra)
{
  int i, j = 1, nx = xtra.valid;

  // find a valid new blob
  for (i = 1; i < nx; i++)
    if (xtra.status[i] > 0)
    {
      // look for a currently invalid entry
      while (status[j] > 0)
        if (++j >= total)
          return;
      CopyItem(j, xtra, i);
    }
}


//= Add velocity to old box center to get new box center.
// If zoom is non-zero, also adjusts size of box.
// does not change initial ranges (xlo, xhi, ylo, yhi)

void jhcBBox::ApplyVel (int zoom)
{
  int i;

  for (i = 1; i < valid; i++)
    if (status[i] > 0)
    {
      items[i].MoveRoi(ROUND(vx[i]), ROUND(vy[i]));
      if (zoom > 0)
        items[i].ResizeRoi(vz[i]);
    }
}


//= Updates velocities by mixing new displacement with old velocity.
// Assumes old set of boxes is indexed correspondingly.

void jhcBBox::ComputeVel (const jhcBBox& last, double mix)
{
  int i, common = __min(total, last.valid);
  double m1 = __min(mix, 1.0), m0 = 1.0 - m1;
  double nx, ny, nz;
  jhcRoi *entry, *old;

  for (i = 1; i < common; i++)
    if (status[i] > 0)
    {
      if (count[i] < 0)
      {
        // zero velocity if target lost
        vx[i] = 0.0;
        vy[i] = 0.0;
        vz[i] = 1.0;
      }
      else if (last.status[i] > 0)
      {
        // estimate new translation
        entry = items + i;
        old = last.items + i;
        nx = (double)(entry->RoiMidX() - old->RoiMidX());
        ny = (double)(entry->RoiMidY() - old->RoiMidY());

        // estimate new zoom
        if (old->RoiW() > 0) 
          nz = 0.5 * entry->RoiW() / (double) old->RoiW();
        else
          nz = 0.5;
        if (old->RoiH() > 0)
          nz += 0.5 * entry->RoiH() / (double) old->RoiH();
        else
          nz += 0.5;

        // mix with old velocities
        vx[i] = m0 * last.vx[i] + m1 * nx;
        vy[i] = m0 * last.vy[i] + m1 * ny;
        vz[i] = m0 * last.vz[i] + m1 * nz;
      }
    }
}
 

//= Mark as unusable any bbox with a significant overlap with one from list.
//   (overlap is measured with respect to blob that might be invalidated).

void jhcBBox::RemSimilar (const jhcBBox& known, double close)
{
  int i, j, nx = known.valid;
  jhcRoi *target, *entry = items + 1;

  for (i = 1; i < valid; i++, entry++)
    if (status[i] > 0)
    {
      target = known.items + 1;
      for (j = 1; j < nx; j++, target++)
        if (known.status[j] > 0) 
          if (entry->RoiOverlap(*target) >= (int)(close * entry->RoiArea()))
          {
            status[i] = 0;
            break;
          }
    }
}


//= Find closest match in new list for each bbox (overlap above threshold).
// Alter size and position of old bbox to be more like new bbox.
// Increment count as contiguous detections are added (e.g. 1, 2, 3,..)
// Decrement count each time no match is found (e.g. 3 goes to -1, -2,..)
// does not change initial ranges (xlo, xhi, ylo, yhi) 

void jhcBBox::MatchTo (jhcBBox& xtra, double close, double fmv, double fsz)
{
  int i, best;
  jhcRoi *entry = items + 1;

  for (i = 1; i < valid; i++, entry++)
    if (status[i] > 0)
      if ((best = xtra.BestOverlap(*entry, close)) < 0)
      {
        // if no match, start decrementing count
        if (count[i] > 0)
          count[i] = -1;
        else
          count[i] -= 1;
      }
      else
      {
        // update geometric parameters 
        entry->MorphRoi((xtra.items)[best], fmv, fsz, 1);

        // keep track of consecutive detections
        if (count[i] < 0)
          count[i] = 1;
        else
          count[i] += 1;

        // remove new blob from further matching (and leave linkage tag)
        (xtra.status)[best] = -i;
      }
}


///////////////////////////////////////////////////////////////////////////
//                          Value Alteration                             //
///////////////////////////////////////////////////////////////////////////

//= Sets all boxes invalid.
// Touches every entry in list to make sure (not just valid range).
// use AllStatus to set status flags without erasing boxes

void jhcBBox::ResetAll (int val0)
{
  int i;

  for (i = 0; i < total; i++)
    status[i] = val0;
  valid = 0;
}


//= Set the status of all box to some value (typically zero).

void jhcBBox::AllStatus (int val)
{
  int i;

  for (i = 1; i < valid; i++)
    status[i] = val;
}


//= Convert all entries with score old to have a score of now instead.

void jhcBBox::RemapStatus (int old, int now)
{
  int i;

  for (i = 1; i < valid; i++)
    if (status[i] == old)
      status[i] = now;
}


//= Take absolute valid of status field (negative might indicate linkage).

void jhcBBox::AbsStatus ()
{
  int i;

  for (i = 1; i < valid; i++)
    status[i] = abs(status[i]);
}


//= Set status to 1 for all boxes with status > sth (others are 0).

void jhcBBox::BinStatus (int sth, int val)
{
  int i;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth)
      status[i] = val;
    else
      status[i] = 0;
}


//= Checks count and marks as valid or invalid any bboxes that pass thresholds.
//   status = 1 means pending verification, status = 2 means just became valid 
//   status = 3 valid for a while, status = -1 means just became invalid

void jhcBBox::CheckCounts (int add, int del)
{
  int i;

  for (i = 1; i < valid; i++)
    if (status[i] < 0)
      status[i] = 0;              // clean up recently invalidated entries
    else if (status[i] > 0)
    {
      if (count[i] <= -del)
      {
        if (status[i] >= 2)
          status[i] = -1;         // valid blob becomes invalid
        else
          status[i] = 0;          // pending blob never makes it
      }
      else if (count[i] >= add)
      {
        if (status[i] == 1)
          status[i] = 2;          // pending blob just validated
        else if (status[i] == 2)  
          status[i] = 3;          // fully valid blob
      }
    }
}


//= Sets status to 1 if current box overlaps with initial box too much.
// also lowers tracking count to 1 if unhatched (count < 1 not affected)
// only examines if status is exactly EQUAL to schk (use with CheckCounts)
// useful in tracking to suppress objects that have never moved (just shrunk) 

void jhcBBox::PassHatched (int schk, double frac)
{
  jhcRoi b0;
  int i;
  
  for (i = 1; i < valid; i++)
    if (status[i] == schk)
    {
      // find overlap wrt smaller box
      b0.SetRoiLims(xlo[i], ylo[i], xhi[i], yhi[i]);
      if (b0.RoiLapSmall(items[i]) >= frac)
      {
        // bash status and count
        status[i] = 1;
        if (count[i] > 1)
          count[i] = 1;
      }
    } 
}


//= Makes all bounding boxes record image dimensions and adjust.

void jhcBBox::ClipAll (int xdim, int ydim)
{
  int i;

  for (i = 1; i < valid; i++)
    items[i].RoiClip(xdim, ydim);
}


//= Make bounding box have a particular height to width ratio in range spec'd.
// Useful for shaving neck and adding forehead in faces (does uniform change)

void jhcBBox::ShapeAll (double alo, double ahi)
{
  int i;
  int rw, rh, ry, dh0, dh1;
  jhcRoi *entry = items + 1;

  for (i = 1; i < valid; i++, entry++)
    if (status[i] > 0)
    {
      // figure out desired height bounds
      rw = entry->RoiW();
      dh1 = ROUND(ahi * rw);
      dh0 = ROUND(alo * rw);
  
      // symmetrically stretch box vertically to nearest limit 
      rh = entry->RoiH();
      ry = entry->RoiY();
      if (rh > dh1)
        entry->ResizeRoi(-1, dh1);
      else if (rh < dh0)
        entry->ResizeRoi(-1, dh0);
    }
}


///////////////////////////////////////////////////////////////////////////
//                              Elimination                              //
///////////////////////////////////////////////////////////////////////////

//= Mark as invalid all blobs except the given index.

void jhcBBox::KeepOnly (int focus)
{
  int i;

  if (focus > 0)
    for (i = 1; i < valid; i++)
      if (i != focus)
        status[i] = 0;
}


//= Remove bbox if it comes near the edge of the image.

void jhcBBox::RemBorder (int w, int h, int dl, int dr, int db, int dt, int sth, int bad)
{
  int i;
  jhcRoi region;

  // if no value given: copy left boundary to right, bottom to top
  if (dr < 0)
    dr = dl;
  if (db < 0) 
    db = dl;
  if (dt < 0)
    dt = db;
  region.SetRoi(dl, db, w - dr - dl, h - dt - db);

  // change status of bboxs that are outside spec
  for (i = 1; i < valid; i++)
    if ((status[i] > sth) && !region.RoiContains(items[i]))
      status[i] = bad;
}


//= Simplified version of RemBorder for uniform boundary.
// Uses clipping limits of Roi as edges of image.

void jhcBBox::RemBorder (const jhcImg &ref, int bd, int sth, int bad)
{
  RemBorder(ref.XDim(), ref.YDim(), bd, bd, bd, bd, sth, bad);
}


//= Remove components that cross outside or touch any edge of the region.

void jhcBBox::RemTouch (const jhcRoi& area)
{
  jhcRoi shrink;
  int i;

  shrink.SetRoi(area.RoiX() + 1, area.RoiY() + 1, area.RoiW() - 2, area.RoiH() - 2);
  for (i = 1; i < valid; i++)
    if ((status[i] > 0) && (shrink.RoiContains(items[i]) <= 0))
      status[i] = 0;
}


//= Mark as invalid any components under area specified.
// If limit is negative, invalidate bboxes over limit.
// Ignores boxes that touch more than max_ej image boundaries.
// Uses true pixel area, not area of bounding box.

void jhcBBox::PixelThresh (int ath, int max_ej, int sth, int bad)
{
  int i;

  if (ath >= 0)
  {
    for (i = 1; i < valid; i++)
      if ((status[i] > sth) && (pixels[i] < ath) && (bd_touch(items[i]) <= max_ej))
        status[i] = bad;
  }
  else
  {
    for (i = 1; i < valid; i++)
      if ((status[i] > sth) && (pixels[i] > -ath) && (bd_touch(items[i]) <= max_ej))
        status[i] = bad;
  }
}


//= Tells how many image boundaries a box touches.

int jhcBBox::bd_touch (const jhcRoi& box)
{
  int n = 0;

  if (box.RoiX() <= fxlo)
    n++;
  if (box.RoiLimX() >= fxhi)
    n++;
  if (box.RoiY() <= fylo)
    n++;
  if (box.RoiLimY() >= fyhi)
    n++;
  return n;
}


//= Mark as invalid any blobs under area specified.
// if limit is negative, invalidate blobs over limit 
// Uses true pixel area, not area of bounding box.

void jhcBBox::AreaThresh (int ath, int sth, int good, int bad)
{
  int over = good, under = bad;
  int i, val = ath;

  if (val < 0)
  {
    val = -ath;
    over = bad;
    under = good;
  }
  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (pixels[i] >= val)
        status[i] = over;
      else
        status[i] = under;
    }
}
   

//= Mark as invalid any bboxes under area specified.
// If limit is negative, invalidate bboxes over limit.

void jhcBBox::AreaThreshBB (int ath, int sth, int bad)
{
  int i;

  if (ath >= 0)
  {
    for (i = 1; i < valid; i++)
      if ((status[i] > sth) && (items[i].RoiArea() < ath))
        status[i] = bad;
  }
  else
  {
    for (i = 1; i < valid; i++)
      if ((status[i] > sth) && (items[i].RoiArea() > -ath))
        status[i] = bad;
  }
}


//= Marks as invalid any bboxes below aspect ratio specified.
// If limit is negative, invalidate bboxes above limit.

void jhcBBox::AspectThreshBB (double asp, int sth, int good, int bad)
{
  int i, over = good, under = bad;
  double val = asp;

  if (val < 0)
  {
    val = -asp;
    over = bad;
    under = good;
  }

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (items[i].RoiAspect() >= val)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Marks as invalid any boxes below fill ratio specified. 
// If limit is negative, invalidate bboxes above limit.

void jhcBBox::FillThreshBB (double fill, int sth, int good, int bad)
{
  int i, over = good, under = bad;
  double val = fill;

  if (val < 0)
  {
    val = -fill;
    over = bad;
    under = good;
  }

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (pixels[i] >= (val * items[i].RoiArea()))
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Marks as invalid any bboxes below width specified.
// If limit is negative, invalidate bboxes above limit.

void jhcBBox::WidthThreshBB (int wid, int sth, int good, int bad)
{
  int i, val = wid, over = good, under = bad;

  if (val < 0)
  {
    val = -wid;
    over = bad;
    under = good;
  }

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (items[i].RoiW() >= val)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Marks as invalid any bboxes below height specified.
// If limit is negative, invalidate bboxes above limit.

void jhcBBox::HeightThreshBB (int ht, int sth, int good, int bad)
{
  int i, val = ht, over = good, under = bad;

  if (val < 0)
  {
    val = -ht;
    over = bad;
    under = good;
  }

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (items[i].RoiH() >= val)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Marks as invalid any bboxes if below BOTH dimensions specified.

void jhcBBox::DimsThreshBB (int wid, int ht, int sth, int good, int bad)
{
  int i;

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
      if ((items[i].RoiW() < wid) && (items[i].RoiH() < ht))
        status[i] = bad;
}


//= Keep only boxes with centers inside the given area.
// if outside is positive, throw out boxes inside area instead

void jhcBBox::InsideThreshBB (const jhcRoi& area, int outside, int sth, int good, int bad)
{
  int i, hit = good, miss = bad;

  if (outside > 0)
  {
    hit = bad;
    miss = good;
  }

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (area.RoiContains(items[i].RoiMidX(), items[i].RoiMidY()) > 0)
        status[i] = hit;
      else
        status[i] = miss;
    }
}


//= Keep only boxes which overlap at least a little with the given area.
// if count is negative then throw out boxes with this much overlap

void jhcBBox::OverlapThreshBB (const jhcRoi& area, int cnt, int sth, int good, int bad)
{
  int i, val = cnt, hit = good, miss = bad;

  if (cnt < 0)
  {
    val = -cnt;
    hit = bad;
    miss = good;
  }

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (area.RoiOverlap(items[i]) > val)
        status[i] = hit;
      else
        status[i] = miss;
    }
}


//= Keep only boxes which are wholly within the given area.
// if outside is positive, throw out boxes contained in area instead

void jhcBBox::ContainThreshBB (const jhcRoi& area, int outside, int sth, int good, int bad)
{
  int i, hit = good, miss = bad;

  if (outside > 0)
  {
    hit = bad;
    miss = good;
  }

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (area.RoiContains(items[i]) > 0)
        status[i] = hit;
      else
        status[i] = miss;
    }
}


//= Removes bboxes whose bounding box top is below the given y coord.
// If ymin is negative, removes those above this threshold instead.

void jhcBBox::YTopThresh (int ymin, int sth, int good, int bad)
{
  int i, val = ymin, over = good, under = bad;

  if (val < 0)
  {
    val = -ymin;
    over = bad;
    under = good;
  }

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (items[i].RoiLimY() > val)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Removes bboxes whose bounding box bottom is below the given y coord.
// If ymin is negative, removes those above this threshold instead.

void jhcBBox::YBotThresh (int ymin, int sth, int good, int bad)
{
  int i, val = ymin, over = good, under = bad;

  if (val < 0)
  {
    val = -ymin;
    over = bad;
    under = good;
  }

  for (i = 1; i < valid; i++)
    if (status[i] > sth)
    {
      if (items[i].RoiY() > val)
        status[i] = over;
      else
        status[i] = under;
    }
}


//= Remove bboxes which hang over the top more than a certain percentage.

void jhcBBox::YClipThresh (int ytop, double maxfrac, int sth, int good, int bad)
{
  int ylim = ytop - 1;
  int i;

  for (i = 1; i < valid; i++)
    if ((status[i] > sth) &&
        ((items[i].RoiLimY() - ylim) > ROUND(maxfrac * items[i].RoiH())))
      status[i] = bad;
}


///////////////////////////////////////////////////////////////////////////
//                          Pixel Statistics                             //
///////////////////////////////////////////////////////////////////////////

//= Keep only elements with at least cnt pixels marked in mask image.
// bashes count member -- do not use with tracking
// similar to jhcBBox::Retain

int jhcBBox::OverlapMask (const jhcImg& labels, const jhcImg& marks, int cnt, int good, int bad)
{
  jhcRoi area;

  if (!labels.Valid(2) || !labels.SameSize(marks, 1))
    return Fatal("Bad image to jhcBBox::OverlapMask");
  area.CopyRoi(labels);
  area.MergeRoi(marks);  

  // local variables
  int i, x, y, rw = area.RoiW(), rh = area.RoiH();
  int ssk = labels.RoiSkip(area) >> 1, msk = marks.RoiSkip(area);
  const UC8 *m = marks.RoiSrc(area);
  const US16 *s = (const US16 *) labels.RoiSrc(area);

  // clear overlap area counts
  for (i = 0; i < valid; i++)
    count[i] = 0;

  // find overlap areas
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, s++, m++)
      if ((*m > 0) && (*s < valid))
        count[*s] += 1;        
    s += ssk;
    m += msk;
  }

  // set status based on area
  for (i = 1; i < valid; i++)
    if (count[i] >= cnt)
      status[i] = good;
    else
      status[i] = bad;
  return 1;
}


//= Keep only elements with at least cnt pixels in the specified area.
// bashes count member -- do not use with tracking

int jhcBBox::OverlapRoi (const jhcImg& labels, const jhcRoi& area, int cnt, int good, int bad)
{
  if (!labels.Valid(2))
    return Fatal("Bad image to jhcBBox::OverlapRoi");

  // local variables
  int i, x, y, rw = area.RoiW(), rh = area.RoiH(), skip = labels.RoiSkip(area) >> 1;
  const US16 *s = (const US16 *) labels.RoiSrc(area);

  // clear overlap area counts
  for (i = 0; i < valid; i++)
    count[i] = 0;

  // find overlap areas
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, s++)
      if (*s < valid)
        count[*s] += 1;        
    s += skip;
  }

  // set status based on area
  for (i = 1; i < valid; i++)
    if (count[i] >= cnt)
      status[i] = good;
    else
      status[i] = bad;
  return 1;
}


//= Returns element with the highest count of pixels marked in mask image.
// bashes count member -- do not use with tracking

int jhcBBox::OverlapBest (const jhcImg& labels, const jhcImg& marks, int sth)
{
  jhcRoi area;
  int win = 0;
  int most = 0;

  if (!labels.Valid(2) || !labels.SameSize(marks, 1))
    return Fatal("Bad image to jhcBBox::OverlapBest");
  area.CopyRoi(labels);
  area.MergeRoi(marks);  

  // local variables
  int i, x, y, rw = area.RoiW(), rh = area.RoiH();
  int ssk = labels.RoiSkip(area) >> 1, msk = marks.RoiSkip(area);
  const UC8 *m = marks.RoiSrc(area);
  const US16 *s = (const US16 *) labels.RoiSrc(area);

  // clear overlap area counts
  for (i = 0; i < valid; i++)
    count[i] = 0;

  // find overlap areas (irregardless of status)
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, s++, m++)
      if ((*m > 0) && (*s < valid))
        count[*s] += 1;        
    s += ssk;
    m += msk;
  }

  // find the item with the biggest overlap
  for (i = 1; i < valid; i++)
    if ((status[i] > sth) && (count[i] > most))
    {
      most = count[i];
      win = i;
    }
  return win;
}


///////////////////////////////////////////////////////////////////////////
//                            Region Tagging                             //
///////////////////////////////////////////////////////////////////////////

//= Mark as invalid all region labels which have a mark in them.
// typically used with seeds from connected component image

int jhcBBox::Poison (const jhcImg& labels, const jhcImg& marks)
{
  if (!labels.Valid(2) || !labels.SameFormat(marks))
    return Fatal("Bad images to jhcBBox::Poison");

  int x, y, rw = labels.RoiW(), rh = labels.RoiH(), rsk = labels.RoiSkip() >> 1;
  const US16 *n = (const US16 *) labels.RoiSrc();
  const US16 *m = (const US16 *) marks.RoiSrc(labels);

  for (y = rh; y > 0; y--, m += rsk, n += rsk)
    for (x = rw; x > 0; x--, m++, n++)
      if ((*m > 0) && (*n > 0) && (*n < valid))
        status[*n] = 0;
  return 1;
}


//= Mark as invalid all region labels which have a mark over threshold in them.
// a negative threshold removes all regions having a pixel under that value

int jhcBBox::PoisonOver (const jhcImg& labels, const jhcImg& marks, int th)
{
  if (!labels.Valid(2) || !labels.SameSize(marks, 1))
    return Fatal("Bad images to jhcBBox::PoisonOver");

  int x, y, rw = labels.RoiW(), rh = labels.RoiH();
  int lsk = labels.RoiSkip() >> 1, msk = marks.RoiSkip(labels);
  const US16 *n = (const US16 *) labels.RoiSrc();
  const UC8 *m = marks.RoiSrc(labels);

  if (th > 0)
  {
    for (y = rh; y > 0; y--, m += msk, n += lsk)
      for (x = rw; x > 0; x--, m++, n++)
        if ((*m > th) && (*n > 0) && (*n < valid))
          status[*n] = 0;
  }
  else
  {
    for (y = rh; y > 0; y--, m += msk, n += lsk)
      for (x = rw; x > 0; x--, m++, n++)
        if ((*m <=-th) && (*n > 0) && (*n < valid))
          status[*n] = 0;
  }
  return 1;
}


//= Mark as invalid all region labels which fall within the given area.

int jhcBBox::PoisonWithin (const jhcImg& labels, const jhcRoi& area)
{
  if (!labels.Valid(2))
    return Fatal("Bad images to jhcBBox::PoisonWithin");

  int x, y, rw = area.RoiW(), rh = area.RoiH(), rsk = labels.RoiSkip(area) >> 1;
  const US16 *n = (const US16 *) labels.RoiSrc(area);

  for (y = rh; y > 0; y--, n += rsk)
    for (x = rw; x > 0; x--, n++)
      if ((*n > 0) && (*n < valid))
        status[*n] = 0;
  return 1;
}


//= Keep only region labels which have a mark in them.
// typically used with seeds from connected component image

int jhcBBox::Retain (const jhcImg& labels, const jhcImg& marks)
{
  if (!labels.Valid(2) || !labels.SameFormat(marks))
    return Fatal("Bad images to jhcBBox::Retain");

  int i, s, x, y, rw = labels.RoiW(), rh = labels.RoiH(), rsk = labels.RoiSkip() >> 1;
  const US16 *n = (const US16 *) labels.RoiSrc();
  const US16 *m = (const US16 *) marks.RoiSrc(labels);

  // negate current markings
  for (i = 1; i < valid; i++)
  { 
    s = status[i];
    if (s > 0)
      status[i] = -s;
  }

  // re-invert markings on tagged items
  for (y = rh; y > 0; y--, m += rsk, n += rsk)
    for (x = rw; x > 0; x--, m++, n++)
      if ((*m > 0) && (*n > 0) && (*n < valid))
      {
        s = status[*n];
        if (s < 0)
          status[*n] = -s;
      }

  // erase blobs whose statuses are still negative
  for (i = 1; i < valid; i++)
  { 
    s = status[i];
    if (s < 0)
      status[i] = 0;
  }
  return 1;
}


//= Keep only region labels which have a mark over threshold in them.
// a negative threshold keeps all regions having a pixel under that value

int jhcBBox::RetainOver (const jhcImg& labels, const jhcImg& marks, int th)
{
  if (!labels.Valid(2) || !labels.SameSize(marks, 1))
    return Fatal("Bad images to jhcBBox::RetainOver");

  int i, s, x, y, rw = labels.RoiW(), rh = labels.RoiH();
  int lsk = labels.RoiSkip() >> 1, msk = marks.RoiSkip(labels);
  const US16 *n = (const US16 *) labels.RoiSrc();
  const UC8 *m = marks.RoiSrc(labels);

  // negate current markings
  for (i = 1; i < valid; i++)
  { 
    s = status[i];
    if (s > 0)
      status[i] = -s;
  }

  // re-invert markings on tagged items
  if (th > 0)
  {
    for (y = rh; y > 0; y--, m += msk, n += lsk)
      for (x = rw; x > 0; x--, m++, n++)
        if ((*m > th) && (*n > 0) && (*n < valid))
        {
          s = status[*n];
          if (s < 0)
            status[*n] = -s;
        }
  }
  else
  {
    for (y = rh; y > 0; y--, m += msk, n += lsk)
      for (x = rw; x > 0; x--, m++, n++)
        if ((*m < -th) && (*n > 0) && (*n < valid))
        {
          s = status[*n];
          if (s < 0)
            status[*n] = -s;
        }
  }

  // erase blobs whose statuses are still negative
  for (i = 1; i < valid; i++)
  { 
    s = status[i];
    if (s < 0)
      status[i] = 0;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Individual Components                         //
///////////////////////////////////////////////////////////////////////////

//= Like MarkRegions but create a binary mask covering a single blob.
// if only = 1 then marks non-selected pixels as zero
// NOTE: very similar to jhcGroup::MarkComp

int jhcBBox::MarkBlob (jhcImg& dest, const jhcImg& src, int index, int val, int only) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad images to jhcBBox::MarkBlob");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip() >> 1;
  UC8 bval = BOUND(val);
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const US16 *s = (const US16 *)(src.PxlSrc() + src.RoiOff());

  // selectively copy pixels 
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s++)
      if (*s == index)
        *d = bval;
      else if (only > 0)
        *d = 0;
  return 1;
}


//= Make binary mask for single blob and set ROI to just that area.
// can optionally grow ROI by certain amount around edges
// generally want to pre-fill dest with zero

int jhcBBox::TightMask (jhcImg& dest, const jhcImg& src, int index, int pad) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad images to jhcBBox::MarkBlob");

  // adjust destination ROI to match blob
  if ((index <= 0) || (index >= valid))
    return 0;
  dest.CopyRoi(*ReadRoi(index));
  dest.GrowRoi(pad, pad);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip(dest) >> 1;
  const US16 *s = (const US16 *) src.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // selectively copy pixels 
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s++)
      *d = ((*s == index) ? 255 : 0);
  return 1;
}


//= Make a binary mask of the lowest cnt pixels in the giben blob.
// also sets ROI of dest to just the mask pixels

int jhcBBox::LowestPels (jhcImg& dest, const jhcImg& src, int index, int cnt, int clr) const
{
  jhcRoi r;

  // check parameters
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad images to jhcBBox::LowestPels");
  if (GetRoi(r, index) <= 0)
    return 0;

  // possibly clear whole image
  if (clr > 0)
  {
    dest.MaxRoi();
    dest.FillArr(0);
  }
  if (cnt <= 0)
    return 1;

  // scan within component ROI
  int x, y, xmin = r.RoiX(), xmax = r.RoiLimX(), ymin = r.RoiY(), ymax = r.RoiLimY();
  int dsk = dest.RoiSkip(r), ssk2 = src.RoiSkip(r) >> 1, x0 = xmax, x1 = xmin, y1 = ymin, n = 0;
  const US16 *s = (const US16 *) src.RoiSrc(r);
  UC8 *d = dest.RoiDest(r);

  for (y = ymin; y <= ymax; y++, d += dsk, s += ssk2)
    for (x = xmin; x <= xmax; x++, d++, s++)
      if (*s != index)
        *d = 0;
      else
      {
        // mark pixel and adjust ROI
        *d = 255;
        x0 = __min(x0, x);
        x1 = __max(x1, x);
        y1 = __max(y1, y);

        // set image ROI at end
        if (++n < cnt)
          continue;
        dest.SetRoiLims(x0, ymin, x1, y1);
        return 1;
      } 
  return 0;                  // full blob marked
}


//= Make a binary mask of the highest cnt pixels in the giben blob.
// also sets ROI of dest to just the mask pixels

int jhcBBox::HighestPels (jhcImg& dest, const jhcImg& src, int index, int cnt, int clr) const
{
  jhcRoi r;

  // check parameters
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad images to jhcBBox::HighestPels");
  if (GetRoi(r, index) <= 0)
    return 0;

  // possibly clear whole image
  if (clr > 0)
  {
    dest.MaxRoi();
    dest.FillArr(0);
  }
  if (cnt <= 0)
    return 1;

  // scan within component ROI
  int x, y, xmin = r.RoiX(), xmax = r.RoiLimX(), ymin = r.RoiY(), ymax = r.RoiLimY();
  int dln = dest.Line(), sln2 = src.Line() >> 1, x0 = xmax, x1 = xmin, y0 = ymax, n = 0;
  const US16 *s, *s0 = (const US16 *) src.RoiSrc(xmin, ymax);
  UC8 *d, *d0 = dest.RoiDest(xmin, ymax);

  for (y = ymax; y >= ymin; y--, d0 -= dln, s0 -= sln2)
  {
    s = s0;
    d = d0;
    for (x = xmin; x <= xmax; x++, d++, s++)
      if (*s != index)
        *d = 0;
      else
      {
        // mark pixel and adjust ROI
        *d = 255;
        x0 = __min(x0, x);
        x1 = __max(x1, x);
        y0 = __min(y0, y);

        // set image ROI at end
        if (++n < cnt)
          continue;
        dest.SetRoiLims(x0, y0, x1, ymax);
        return 1;
      } 
  }
  return 0;                  // full blob marked
}


///////////////////////////////////////////////////////////////////////////
//                             Visualization                             //
///////////////////////////////////////////////////////////////////////////

//= Passes all parts of the image that fall into at least one bbox.
// Can restrict to bboxes with status at or above specified value.

int jhcBBox::OverGate (jhcImg& dest, const jhcImg& src, int sth) const
{
  jhcRoi r0;
  int i;

  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcBBox::OverGate");

  // copy parts specified by valid ROIs in list  
  for (i = 1; i < valid; i++)
    if (status[i] >= sth)
    {
      r0.CopyRoi(src);
      r0.MergeRoi(items[i]);
      dest.CopyArr(src, r0);
    }
      
  // restore original ROI
  dest.CopyRoi(src);
  return 1;
}


//= Draws filled rectangles for all bboxes as a monochrome mask.
// 8 bit pseudo-color of patch determined by index value
// Can restrict to bboxes with status at or above specified value.

int jhcBBox::DrawPatch (jhcImg& dest, int sth) const
{
  jhcDraw jd;
  int i;

  if (!dest.Valid(1))
    return Fatal("Bad image to jhcBBox::DrawPatch");

  dest.FillArr(0);
  for (i = 1; i < valid; i++)
    if (status[i] >= sth)
      jd.RectFill(dest, items[i], jd.ColorN(i));
  return 1;
}


//= Draws a rectangular frame for each bbox over top of another image.
// Can restrict to bboxes with status at or above specified value.
// color of box determined by index value
// returns number of boxes drawn
// NOTE: for tight contour around object see jhcGroup::DrawBorder

int jhcBBox::DrawOutline (jhcImg& dest, int sth, double mag) const
{
  jhcDraw jd;
  int i, x, y, w, h, n = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth)
    {
      items[i].RoiSpecs(&x, &y, &w, &h);
      jd.RectEmpty(dest, ROUND(mag * x), ROUND(mag * y), 
                   ROUND(mag * w), ROUND(mag * h), 3, -i);
      n++;
    }
  return n;
}


//= Draws a fixed color rectangular frame for each bbox over a color image.
// Can restrict to bboxes with status at or above specified value.
// returns number of boxes drawn
// NOTE: for tight contour around object see jhcGroup::DrawBorder

int jhcBBox::MarkOutline (jhcImg& dest, int sth, double mag, int t, int r, int g, int b) const
{
  jhcDraw jd;
  int i, x, y, w, h, n = 0;

  for (i = 1; i < valid; i++)
    if (status[i] >= sth)
    {
      items[i].RoiSpecs(&x, &y, &w, &h);
      jd.RectEmpty(dest, ROUND(mag * x), ROUND(mag * y), 
                   ROUND(mag * w), ROUND(mag * h), t, r, g, b);
      n++;
    }
  return n;
}


//= Draws a fixed color rectangular frame for each bbox with exactly the given status.
// returns number of boxes drawn
// NOTE: for tight contour around object see jhcGroup::DrawBorder

int jhcBBox::OnlyOutline (jhcImg& dest, int sth, double mag, int t, int r, int g, int b) const
{
  jhcDraw jd;
  int i, x, y, w, h, n = 0;

  for (i = 1; i < valid; i++)
    if (status[i] == sth)
    {
      items[i].RoiSpecs(&x, &y, &w, &h);
      jd.RectEmpty(dest, ROUND(mag * x), ROUND(mag * y), 
                   ROUND(mag * w), ROUND(mag * h), t, r, g, b);
      n++;
    }
  return n;
}
           
 
//= Passes pixels of original image that match valid bounding boxes.

int jhcBBox::ThreshValid (jhcImg& dest, const jhcImg& src, int sth, int mark) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad image to jhcBBox::ThreshValid");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip() >> 1;
  const US16 *s = (const US16 *) src.RoiSrc();
  UC8 *d = dest.RoiDest();

  // lookup status for each pixel
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d++, s++)
      if ((*s == 0) || (*s >= valid) || (status[*s] < sth))
        *d = 0;
      else
        *d = (UC8) mark;
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Marks pixels where original image that match valid bounding boxes.
// does not change pixels in other parts of image (use FillArr first)

int jhcBBox::ValidPixels (jhcImg& dest, const jhcImg& src, int mark, int key) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad image to jhcBBox::ValidPixels");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip() >> 1;
  const US16 *s = (const US16 *) src.RoiSrc();
  UC8 *d = dest.RoiDest();

  // lookup status for each pixel
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s++)
      if ((*s != 0) && (*s < valid) && (status[*s] == key))
        *d = (UC8) mark;
  return 1;
}


//= Only copy pixels corresponding to valid blobs.
// generates an edited version of connected components (16 bit image)

int jhcBBox::CopyRegions (jhcImg& dest, const jhcImg& src) const
{
  if (!dest.Valid(2) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcBBox::CopyRegions");
  dest.CopyRoi(src);

  // general ROI case
  int x, y;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk2 = dest.RoiSkip() >> 1;
  US16 index;
  UL32 roff = dest.RoiOff();
  US16 *d = (US16 *)(dest.PxlSrc() + roff);
  US16 *s = (US16 *)(src.PxlSrc() + roff);

  // selectively copy pixels 
  for (y = rh; y > 0; y--, d += rsk2, s += rsk2)
    for (x = rw; x > 0; x--, d++, s++)
    {
      index = *s;
      if ((index == 0) || (index >= total))
        *d = 0;
      else if (status[index] > 0)
        *d = index;
      else
        *d = 0;
    }
  return 1;
}


//= Only copy pixels corresponding to blobs with given status value.
// generates an edited version of connected components (16 bit image)

int jhcBBox::CopyOnly (jhcImg& dest, const jhcImg& src, int sth) const
{
  if (!dest.Valid(2) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcBBox::CopyOnly");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, index, rw = dest.RoiW(), rh = dest.RoiH(), rsk2 = dest.RoiSkip() >> 1;
  const US16 *s = (const US16 *) src.RoiSrc();
  US16 *d = (US16 *) dest.RoiSrc();

  // selectively copy pixels 
  for (y = rh; y > 0; y--, d += rsk2, s += rsk2)
    for (x = rw; x > 0; x--, d++, s++)
    {
      index = *s;
      if ((index <= 0) || (index >= total))
        *d = 0;
      else if (status[index] == sth)
        *d = (US16) index;
      else
        *d = 0;
    }
  return 1;
}


//= Like MarkRegions but only mark blobs above sth.

int jhcBBox::MarkOver (jhcImg& dest, const jhcImg& src, int sth, int val, int clr) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad images to jhcBBox::MarkOver");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip() >> 1;
  UC8 bval = BOUND(val);
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const US16 *s = (const US16 *)(src.PxlSrc() + src.RoiOff());

  // possibly clear image
  if (clr > 0)
    dest.FillArr(0);

  // selectively copy pixels 
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s++)
      if ((*s > 0) && (*s < total))
        if (status[*s] > sth)
          *d = bval;
  return 1;
}


//= Like MarkOver but only mark blobs with score between slo and shi (inclusive).

int jhcBBox::MarkRange (jhcImg& dest, const jhcImg& src, int slo, int shi, int val, int clr) const
{
  if (!dest.Valid(1) || !dest.SameSize(src, 2))
    return Fatal("Bad images to jhcBBox::MarkRange");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = src.RoiSkip() >> 1;
  UC8 bval = BOUND(val);
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  const US16 *s = (const US16 *)(src.PxlSrc() + src.RoiOff());

  // possibly clear image
  if (clr > 0)
    dest.FillArr(0);

  // selectively copy pixels 
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s++)
      if ((*s > 0) && (*s < total))
        if ((status[*s] >= slo) && (status[*s] <= shi))
          *d = bval;
  return 1;
}



