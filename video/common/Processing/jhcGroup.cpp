// jhcGroup.cpp : connected components analysis, etc.
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
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

#include "Interface/jhcMessage.h"

#include "Processing/jhcGroup.h"


///////////////////////////////////////////////////////////////////////////
//                     Basic Connected Components                        //
///////////////////////////////////////////////////////////////////////////

//= Find 4 way connected regions in a binary image.
// Ignores pixels less than or equal to th if th positive, 
//   else ignores pixels greater than or equal to -th
// Can automatically eliminate regions below the area threshold given.
// Stores blob labels in a 2 field image, use Scramble to display.
// Starts labeling new fragments from value label0 + 1.
// Returns the number of blobs if successful, -1 if error.

int jhcGroup::CComps4 (jhcImg& dest, const jhcImg& src, int amin, int th, int label0)
{
  int n;

  if (!dest.Valid(2) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcGroup::CComps4");

  n = scan_labels(dest, src, th);
  n = norm_labels(dest, n, __max(1, amin), label0);
  return n;
}


//= Find 8 way connected regions in a binary image.
// Ignores pixels less than or equal to th if th positive, 
//   else ignores pixels greater than or equal to -th
// Can automatically eliminate regions below the area threshold given.
// Stores blob labels in a 2 field image, use Scramble to display.
// Starts labeling new fragments from value label0 + 1.
// Returns the number of blobs if successful, -1 if error.

int jhcGroup::CComps8 (jhcImg& dest, const jhcImg& src, int amin, int th, int label0)
{
  int n;

  if (!dest.Valid(2) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcGroup::CComps8");

  n = scan_labels8(dest, src, th);
  n = norm_labels(dest, n, __max(1, amin), label0);
  return n;
}


//= First pass of 4 connected labelling -- may generate several labels for a region.
// Ignores pixels less than or equal to th if th positive, 
//   else ignores pixels greater than or equal to -th.
// Returns number of labels used, global "areas" holds counts and linkages.

int jhcGroup::scan_labels (jhcImg& dest, const jhcImg& src, int th)
{
  dest.CopyRoi(src);

  // general ROI case
  int lim, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk2 = dest.RoiSkip() >> 1, ssk = src.RoiSkip();
  int label, maxblob, n = 0;
  const UC8 *s = src.RoiSrc();
  UC8 *d0 = dest.RoiDest();
  US16 *d = (US16 *) d0;
  const US16 *dlf = d - 1, *ddn = (US16 *)(d0 - dest.Line());

  // make and initialize area and pointer array
  maxblob = (rw * rh) / 2 + 2;
  if (areas.Size() < maxblob)
    areas.SetSize(maxblob);
  lim = areas.Last();
  areas.ASet(0, 0);    

  // do first pass over image labelling all foreground areas
  for (y = rh; y > 0; y--, d += dsk2, ddn += dsk2, dlf += dsk2, s += ssk)
    for (x = rw; x > 0; x--, d++, ddn++, dlf++, s++)
      if ((*s <= th) || ((th < 0) && (*s >= -th)))
        *d = 0;
      else
      {
        label = 0;

        // check left and below neighbors (if any) 
        if (y < rh) 
          if (*ddn != 0)
            label = merge_labels(label, *ddn);
        if (x < rw) 
          if (*dlf != 0)
            label = merge_labels(label, *dlf);

        // if no label copied then create new one, assign to pixel
        if ((label == 0) && (n < lim))
        {
          label = ++n;
          areas.ASet(label, 1);
        }
        *d = (US16) label;
      }
  return n;
}


//= First pass of 8 connected labelling -- may generate several labels for a region.
// Ignores pixels less than or equal to th if th positive, 
//   else ignores pixels greater than or equal to -th.
// Returns number of labels used, global "areas" holds counts and linkages.

int jhcGroup::scan_labels8 (jhcImg& dest, const jhcImg& src, int th)
{
  dest.CopyRoi(src);

  // general ROI case
  int lim, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk2 = dest.RoiSkip() >> 1, ssk = src.RoiSkip();
  int label, maxblob, n = 0;
  const UC8 *s = src.RoiSrc();
  UC8 *d0 = dest.RoiDest();
  US16 *d = (US16 *) d0;
  const US16 *dlf = d - 1, *ddn = (US16 *)(d0 - dest.Line() - 2);

  // make and initialize area and pointer array
  maxblob = (rw * rh) / 2 + 2;
  if (areas.Size() < maxblob)
    areas.SetSize(maxblob);
  lim = areas.Last();
  areas.ASet(0, 0);    

  // do first pass over image labelling all foreground areas
  for (y = rh; y > 0; y--, d += dsk2, ddn += dsk2, dlf += dsk2, s += ssk)
    for (x = rw; x > 0; x--, d++, ddn++, dlf++, s++)
      if ((*s <= th) || ((th < 0) && (*s >= -th)))
        *d = 0;
      else
      {
        label = 0;

        // check left, below, and diagonal neighbors (if any) 
        if (y < rh) 
		    {
          if (x < rw) 
            if (ddn[0] != 0)                            // diagonal left below
              label = merge_labels(label, ddn[0]);
          if (ddn[1] != 0)                              // directly below
            label = merge_labels(label, ddn[1]);
          if (x > 1)
            if (ddn[2] != 0)                            // diagonal right below
              label = merge_labels(label, ddn[2]);
		    }
        if (x < rw) 
          if (*dlf != 0)                                // directly to left
            label = merge_labels(label, *dlf);

        // if no label copied then create new one, assign to pixel
        if ((label == 0) && (n < lim))
        {
          label = ++n;
          areas.ASet(label, 1);
        }
        *d = (US16) label;
      }
  return n;
}


//= Merges two classes to yield one name and a combined area measure.
// Leaves a forwarding "pointer" by negating canonical index.    
// Returns the final canonical index.

int jhcGroup::merge_labels (int now, int old)
{
  int size, cnt = 0, base = old;
  
  // look up canonical name of old region (now is already canonical)
  // chase pointers to find base class, if several pointer then compress path
  while ((size = areas.ARef(base)) < 0)
  {
    base = -size;
    cnt++;
  }
  if (cnt > 1)
    areas.ASet(old, -base);

  // see if current pixel has no class or same class as old
  if (now == base)
    return base;
  if (now == 0)
  {
    areas.AInc(base, 1);
    return base;
  }

  // else merge areas into lowest numbered class, leave forwarding pointer
  if (now < base)
  {
    areas.AInc(now, areas.ARef(base));
    areas.ASet(base, -now);
    return now;
  }
  areas.AInc(base, areas.ARef(now));
  areas.ASet(now, -base);
  return base;
}


//= Keep only labels with lots of area, reassign names from label0 + 1 upward.  
// Assumes region info stored in global "areas" from first pass (scan_labels).
// Images now filled with consistent new labels for regions.

int jhcGroup::norm_labels (jhcImg& dest, int n, int amin, int label0)
{
  if (n >= areas.Size())
    return -2;

  // general ROI case
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk2 = dest.RoiSkip() >> 1;
  int x, y, i, label, old, cnt = label0;
  US16 *d = (US16 *) dest.RoiSrc();

  // convert from areas and pointers into canonical labels for regions
  for (i = 1; i <= n; i++)
  {
    if (areas.ARef(i) >= amin)     // if big enough assign new label
      areas.ASet(i, ++cnt);
    else if (areas.ARef(i) > 0)    // if too small then erase
      areas.ASet(i, 0);
  }

  // otherwise (negative means pointer) copy new label of canonical class
  // since this class is smallest, new label has already been assigned 
  for (i = 1; i <= n; i++)
    if (areas.ARef(i) < 0)
    {
      old = i;
      while ((label = areas.ARef(old)) < 0)
        old = -label;
      areas.ASet(i, label);
    }

  // scan image and convert class markers to canonical labels
  for (y = rh; y > 0; y--, d += dsk2)
    for (x = rw; x > 0; x--, d++)
      if (*d > 0)
        *d = (US16) areas.ARef(*d);
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                    Connected Components Variants                      //
///////////////////////////////////////////////////////////////////////////

//= Find 4 way connected regions in a gray scale image.
// ignores pixels with value bg, connects if abs diff less than or equal to th
// Can automatically eliminate regions below the area threshold given.
// Stores blob labels in a 2 field image, use Scramble to display.
// Returns the number of blobs if successful, -1 if error.

int jhcGroup::GComps4 (jhcImg& dest, const jhcImg& src, int amin, int diff, int bg)
{
  int n;

  if (!dest.Valid(2) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcGroup::GComps4");

  n = scan_diff(dest, src, diff, bg);
  n = norm_labels(dest, n, __max(1, amin));
  return n;
}


//= First pass of labelling -- may generate several labels for a region.
// Pixels are connected if their label is less than or equal to diff
// Returns number of labels used, global "areas" holds counts and linkages.

int jhcGroup::scan_diff (jhcImg& dest, const jhcImg& src, int diff, int bg)
{
  dest.CopyRoi(src);

  // general ROI case
  int lim, ej, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk2 = dest.RoiSkip() >> 1, ssk = src.RoiSkip();
  int label, maxblob, n = 0;
  const UC8 *s = src.RoiSrc(), *slf = s - 1, *sdn = s - src.Line();
  UC8 *d0 = dest.RoiDest();
  US16 *d = (US16 *) d0;
  const US16 *dlf = d - 1, *ddn = (US16 *)(d0 - dest.Line());

  // make and initialize area and pointer array
  maxblob = (rw * rh) / 2 + 2;
  if (areas.Size() < maxblob)
    areas.SetSize(maxblob);
  lim = areas.Last();
  areas.ASet(0, 0);  

  // do first pass over image labelling all foreground areas
  for (y = rh; y > 0; y--, d += dsk2, ddn += dsk2, dlf += dsk2, s += ssk, sdn += ssk, slf += ssk)
    for (x = rw; x > 0; x--, d++, ddn++, dlf++, s++, sdn++, slf++)
      if (*s == bg)
        *d = 0;
      else
      {
        label = 0;

        // check left and below neighbors (if any) 
        if ((y < rh) && (*ddn != 0))
        {
          ej = *s - *sdn;
          if (ej < 0)
            ej = -ej;
          if (ej <= diff)
            label = merge_labels(label, *ddn);
        }
        if ((x < rw) && (*dlf != 0))
        {
          ej = *s - *slf;
          if (ej < 0)
            ej = -ej;
          if (ej <= diff)
            label = merge_labels(label, *dlf);
        }

        // if no label copied then create new one, assign to pixel
        if ((label == 0) && (n < lim))
        {
          label = ++n;
          areas.ASet(label, 1);
        }
        *d = (US16) label;
      }
  return n;
}


//= Find 4 way connected regions in a (cyclic) angle image.
// ignores pixels with value bg (makes sure angle zero -> 1 instead)
// connects if cyclic absolute difference less than or equal to th
// Can automatically eliminate regions below the area threshold given.
// Stores blob labels in a 2 field image, use Scramble to display.
// Returns the number of blobs if successful, -1 if error.

int jhcGroup::AComps4 (jhcImg& dest, const jhcImg& src, int amin, int diff, int bg)
{
  int n;

  if (!dest.Valid(2) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcGroup::AComps4");

  n = scan_diff_a(dest, src, diff, bg);
  n = norm_labels(dest, n, __max(1, amin));
  return n;
}


//= Find 8 way connected regions in a (cyclic) angle image.
// ignores pixels with value bg (makes sure angle zero -> 1 instead)
// connects if cyclic absolute difference less than or equal to th
// Can automatically eliminate regions below the area threshold given.
// Stores blob labels in a 2 field image, use Scramble to display.
// Returns the number of blobs if successful, -1 if error.

int jhcGroup::AComps8 (jhcImg& dest, const jhcImg& src, int amin, int diff, int bg)
{
  int n;

  if (!dest.Valid(2) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcGroup::AComps8");

  n = scan_diff_a8(dest, src, diff, bg);
  n = norm_labels(dest, n, __max(1, amin));
  return n;
}


//= First pass 4 way labelling in cyclic image -- maybe several labels for a region.
// Pixels connected if labels are less than or equal to diff (with wraparound)
// Returns number of labels used, global "areas" holds counts and linkages.

int jhcGroup::scan_diff_a (jhcImg& dest, const jhcImg& src, int diff, int bg)
{
  dest.CopyRoi(src);

  // general ROI case
  int lim, ej, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk2 = dest.RoiSkip() >> 1, ssk = src.RoiSkip();
  int label, maxblob, n = 0;
  const UC8 *s = src.RoiSrc(), *slf = s - 1, *sdn = s - src.Line();
  UC8 *d0 = dest.RoiDest();
  US16 *d = (US16 *) d0;
  const US16 *dlf = d - 1, *ddn = (US16 *)(d0 - dest.Line());

  // make and initialize area and pointer array
  maxblob = (rw * rh) / 2 + 2;
  if (areas.Size() < maxblob)
    areas.SetSize(maxblob);
  lim = areas.Last();
  areas.ASet(0, 0);  

  // do first pass over image labelling all foreground areas
  for (y = rh; y > 0; y--, d += dsk2, ddn += dsk2, dlf += dsk2, s += ssk, sdn += ssk, slf += ssk)
    for (x = rw; x > 0; x--, d++, ddn++, dlf++, s++, sdn++, slf++)
      if (*s == bg)
        *d = 0;
      else
      {
        label = 0;

        // check left and below neighbors (if any) 
        if ((y < rh) && (*ddn != 0))
        {
          ej = *s - *sdn;
          if (ej <= -128)         // cyclic diff
            ej += 256;
          else if (ej > 128)
            ej -= 256;
          if (ej < 0)             // abs val
            ej = -ej;
          if (ej <= diff)
            label = merge_labels(label, *ddn);
        }
        if ((x < rw) && (*dlf != 0))
        {
          ej = *s - *slf;
          if (ej <= -128)         // cyclic diff
            ej += 256;
          else if (ej > 128)
            ej -= 256;
          if (ej < 0)             // abs val
            ej = -ej;
          if (ej <= diff)
            label = merge_labels(label, *dlf);
        }

        // if no label copied then create new one, assign to pixel
        if ((label == 0) && (n < lim))
        {
          label = ++n;
          areas.ASet(label, 1);
        }
        *d = (US16) label;
      }
  return n;
}


//= First pass 8 way labelling in cyclic image -- maybe several labels for a region.
// Pixels connected if labels are less than or equal to diff (with wraparound)
// Returns number of labels used, global "areas" holds counts and linkages.

int jhcGroup::scan_diff_a8 (jhcImg& dest, const jhcImg& src, int diff, int bg)
{
  dest.CopyRoi(src);

  // general ROI case
  int lim, ej, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk2 = dest.RoiSkip() >> 1, ssk = src.RoiSkip();
  int label, maxblob, n = 0;
  const UC8 *s = src.RoiSrc(), *slf = s - 1, *sdn = s - src.Line() - 1;
  UC8 *d0 = dest.RoiDest();
  US16 *d = (US16 *) d0;
  const US16 *dlf = d - 1, *ddn = (US16 *)(d0 - dest.Line() - 2);

  // make and initialize area and pointer array
  maxblob = (rw * rh) / 2 + 2;
  if (areas.Size() < maxblob)
    areas.SetSize(maxblob);
  lim = areas.Last();
  areas.ASet(0, 0);  

  // do first pass over image labelling all foreground areas
  for (y = rh; y > 0; y--, d += dsk2, ddn += dsk2, dlf += dsk2, s += ssk, sdn += ssk, slf += ssk)
    for (x = rw; x > 0; x--, d++, ddn++, dlf++, s++, sdn++, slf++)
      if (*s == bg)
        *d = 0;
      else
      {
        label = 0;

        // check left, below, and diagonal neighbors (if any) 
        if ((y < rh) && (x < rw) && (ddn[0] != 0))             // below left
        {
          ej = *s - sdn[0];
          if (ej <= -128)         // cyclic diff
            ej += 256;
          else if (ej > 128)
            ej -= 256;
          if (ej < 0)             // abs val
            ej = -ej;
          if (ej <= diff)
            label = merge_labels(label, ddn[0]);
        }
        if ((y < rh) && (ddn[1] != 0))                         // directly below
        {
          ej = *s - sdn[1];
          if (ej <= -128)         // cyclic diff
            ej += 256;
          else if (ej > 128)
            ej -= 256;
          if (ej < 0)             // abs val
            ej = -ej;
          if (ej <= diff)
            label = merge_labels(label, ddn[1]);
        }
        if ((y < rh) && (x > 1) && (ddn[2] != 0))              // below right
        {
          ej = *s - sdn[2];
          if (ej <= -128)         // cyclic diff
            ej += 256;
          else if (ej > 128)
            ej -= 256;
          if (ej < 0)             // abs val
            ej = -ej;
          if (ej <= diff)
            label = merge_labels(label, ddn[2]);
        }
        if ((x < rw) && (*dlf != 0))                           // directly left
        {
          ej = *s - *slf;
          if (ej <= -128)         // cyclic diff
            ej += 256;
          else if (ej > 128)
            ej -= 256;
          if (ej < 0)             // abs val
            ej = -ej;
          if (ej <= diff)
            label = merge_labels(label, *dlf);
        }

        // if no label copied then create new one, assign to pixel
        if ((label == 0) && (n < lim))
        {
          label = ++n;
          areas.ASet(label, 1);
        }
        *d = (US16) label;
      }
  return n;
}


//= Do a top-down connected component analysis where not all starts join up.
// at intersection smaller piece is absorbed if area < amin or area < arel * big
// otherwise seed continues to claim pixels leading to vertical divisions
// final regions < amin removed, relative comparison is disabled if arel <= 0.0
// useful for separating groups of people, originally in jhcHeadChop

int jhcGroup::SiamCC (jhcImg& dest, const jhcImg& src, double arel, int amin, int th)
{
  int n;

  if (!dest.Valid(2) || !dest.SameSize(src, 1))
    return Fatal("Bad images to jhcGroup::SiamCC");

  n = scan_top(dest, src, arel, amin, th);
  n = norm_labels(dest, n, __max(1, amin));
  return n;
}


//= First pass of labelling -- may generate several labels for a region.
// Ignores pixels less than or equal to th if th positive, 
//   else ignores pixels greater than or equal to -th.
// Returns number of labels used, global "areas" holds counts and linkages.

int jhcGroup::scan_top (jhcImg& dest, const jhcImg& src, double arel, int amin, int th)
{
  dest.CopyRoi(src);

  // general ROI case
  int lim, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk2 = rw + (dest.Line() >> 1), ssk = rw + src.Line();
  int label, maxblob, n = 0;
  const UC8 *s = src.RoiSrc(dest.RoiX(), dest.RoiLimY());
  UC8 *d0 = dest.RoiDest(dest.RoiX(), dest.RoiLimY());
  US16 *d = (US16 *) d0;
  const US16 *dlf = d - 1, *dup = (US16 *)(d0 + dest.Line());  // initially outside image

  // make and initialize area and pointer array
  maxblob = (rw * rh) / 2 + 2;
  if (areas.Size() < maxblob)
    areas.SetSize(maxblob);
  lim = areas.Last();
  areas.ASet(0, 0);    

  // do first pass over image labelling all foreground areas
  for (y = rh; y > 0; y--, d -= dsk2, dup -= dsk2, dlf -= dsk2, s -= ssk)
    for (x = rw; x > 0; x--, d++, dup++, dlf++, s++)
      if ((*s <= th) || ((th < 0) && (*s >= -th)))
        *d = 0;
      else
      {
        label = 0;

        // check above and left neighbors (if any) 
        if (y < rh) 
          if (*dup != 0)
            label = merge_labels(label, *dup);
        if (x < rw) 
          if (*dlf != 0)
            label = merge_horiz(label, *dlf, arel, amin);

        // if no label copied then create new one, assign to pixel
        if ((label == 0) && (n < lim))
        {
          label = ++n;
          areas.ASet(label, 1);
        }
        *d = (US16) label;
      }
  return n;
}


//= Merges two classes if small enough to yield one name and a combined area measure.
// Leaves a forwarding "pointer" by negating canonical index.    
// Returns the final canonical index.

int jhcGroup::merge_horiz (int now, int old, double arel, int amin)
{
  int size, anow, abase, alo, ahi;
  int cnt = 0, base = old;
  
  // look up canonical name of old region (now is already canonical)
  // chase pointers to find base class, if several pointer then compress path
  while ((size = areas.ARef(base)) < 0)
  {
    base = -size;
    cnt++;
  }
  if (cnt > 1)
    areas.ASet(old, -base);

  // see if current pixel has no class or same class as old
  if (now == base)
    return base;
  if (now == 0)
  {
    areas.AInc(base, 1);
    return base;
  }

  // refuse to absorb region if too big
  anow = areas.ARef(now);
  abase = areas.ARef(base);
  alo = __min(anow, abase);
  ahi = __max(anow, abase);
  if ((alo >= amin) && ((arel <= 0.0) || (alo >= (arel * ahi))))
    return now;

  // else merge areas into lowest numbered class, leave forwarding pointer
  if (now < base)
  {
    areas.AInc(now, areas.ARef(base));
    areas.ASet(base, -now);
    return now;
  }
  areas.AInc(base, areas.ARef(now));
  areas.ASet(now, -base);
  return base;
}


///////////////////////////////////////////////////////////////////////////
//                           Shape Cleanup                               //
///////////////////////////////////////////////////////////////////////////

//= Uses 4-connected components to remove areas less than amin pixels.
// Foreground = src pixels over th, or pixels under -th if th is negative.
// Can supply a similar-size 2 field image in vic to prevent allocation.

int jhcGroup::EraseBlips (jhcImg& dest, const jhcImg& src, int amin, int th, 
                          jhcImg *vic)
{
  int n;
  jhcImg *marks = vic;

  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcGroup::EraseBlips");
  if (amin <= 0)
    return 0;
  dest.CopyRoi(src);
  
  if ((vic == NULL) || !dest.SameSize(*vic, 2))
    marks = tmp.SetSize(dest, 2);
  marks->CopyRoi(src);

  n = scan_labels(*marks, src, th);
  thresh_labels(dest, *marks, n, amin, 0.0, 0);
  return 1;
}


//= Keep only labels with lots of area by marking these pixels as 255.
// If inv is greater then zero, then components marked 0 instead.
// Assumes region info stored in global "areas" from first pass (scan_labels).
// returns area of largest blob

int jhcGroup::thresh_labels (jhcImg& dest, const jhcImg& marks, int n, 
                             int amin, double arel, int inv)
{
  if (n >= areas.Size())
    return -2;
  dest.CopyRoi(marks);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk2 = marks.RoiSkip() >> 1;
  int i, label, old, win, big = amin, fg = 255, bg = 0;
  UC8 *d = dest.RoiDest();
  const US16 *s = (const US16 *) marks.RoiSrc();

  // determine labels to use
  if (inv > 0)
  {
    fg = 0;
    bg = 255;
  }

  // see if should be sensitive to relative area
  win = 0;
  for (i = 1; i <= n; i++)
    if (areas.ARef(i) > win)
      win = areas.ARef(i);   
  if (arel > 0.0)
  {
    big = (int)(arel * win + 0.5);
    big = __max(big, amin);
  }

  // convert from areas and pointers into canonical labels for regions
  for (i = 1; i <= n; i++)
  {
    if (areas.ARef(i) >= big)      // if big enough assign new label
      areas.ASet(i, fg);
    else if (areas.ARef(i) > 0)    // if too small then erase
      areas.ASet(i, bg);
  }

  // otherwise (negative means pointer) copy new label of canonical class
  // since this class is smallest, new label has already been assigned 
  for (i = 1; i <= n; i++)
    if (areas.ARef(i) < 0)
    {
      old = i;
      while ((label = areas.ARef(old)) < 0)
        old = -label;
      areas.ASet(i, label);
    }

  // scan image and convert class markers to foreground markings
  for (y = rh; y > 0; y--, d += dsk, s += ssk2)
    for (x = rw; x > 0; x--, d++, s++)
      if (*s == 0)
        *d = (UC8) bg;
      else
        *d = (UC8) areas.ARef(*s);
  return win;
}


//= Keeps just the biggest component in an image.
// foreground = src pixels over th, or pixels under -th if th is negative.
// returns area of single retained blob

int jhcGroup::Biggest (jhcImg& dest, const jhcImg& src, int th)
{
  return RemSmall(dest, src, 1.0, 0, th);
}


//= Keep only component that includes pixel (x y).
// returns area or 0 if none

int jhcGroup::Tagged (jhcImg& dest, const jhcImg& src, int x, int y, int th)
{
  int n;

  // sanity check
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcGroup::Tagged");
  dest.CopyRoi(src);
  if (!src.InBounds(x, y))
  {
    dest.FillArr(0);
    return 0;
  }

  // do connected components finding and select one
  tmp.SetSize(src, 2);
  tmp.CopyRoi(src);
  n = scan_labels(tmp, src, th);
  return keep_labels(dest, tmp, n, x, y);
}


//= Keep only label found at pixel (px py) and mark in output image.
// Assumes region info stored in global "areas" from first pass (scan_labels).
// returns area of retained blob (0 if none)

int jhcGroup::keep_labels (jhcImg& dest, const jhcImg& marks, int n, int px, int py)
{
  int rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk2 = marks.RoiSkip() >> 1;
  int win, label, i, base, x, y, cnt;
  const US16 *s = (const US16 *) marks.RoiSrc();
  UC8 *d = dest.RoiDest();

  // sanity check
  if (n >= areas.Size())
    return -2;

  // find canonical label for selected point and save component area
  if ((win = marks.ARef(px, py)) == 0)
  {
    dest.FillArr(0);
    return 0;
  }
  while ((label = areas.ARef(win)) < 0)
    win = -label;
  cnt = areas.ARef(win);

  // clear output mark for all raw labels which do not reduce to win
  for (i = 1; i <= n; i++)
    if ((base = -areas.ARef(i)) != 0)
    {
      if (base < 0)
        base = i;
      else
        while ((label = areas.ARef(base)) < 0)
          base = -label;
      areas.ASet(i, ((base == win) ? 255 : 0));      // determine output mark
    }

  // scan image and save only pixels from selected blob
  for (y = rh; y > 0; y--, d += dsk, s += ssk2)
    for (x = rw; x > 0; x--, d++, s++)
      if (*s == 0)
        *d = 0;
      else
        *d = (UC8) areas.ARef(*s);                   // emit output mark
  return cnt;
}


//= Removes blobs with area less than arel times biggest blob area.
// Also removes those with areas less than amin regardless.
// Foreground = src pixels over th, or pixels under -th if th is negative.
// Can supply a similar-size 2 field image in vic to prevent allocation.
// Returns area of largest blobs

int jhcGroup::RemSmall (jhcImg& dest, const jhcImg& src, double arel, int amin, 
                        int th, jhcImg *vic)
{
  int n;
  jhcImg *marks = vic;

  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcGroup::RemSmall");
  if (arel < 0)
    return 0;
  dest.CopyRoi(src);
  
  if ((vic == NULL) || !dest.SameSize(*vic, 2))
    marks = tmp.SetSize(dest, 2);
  marks->CopyRoi(src);

  n = scan_labels(*marks, src, th);
  return thresh_labels(dest, *marks, n, amin, arel, 0);
}


//= Removes gray-scale blobs with area less than arel times biggest blob area.
// Also removes those with areas less than amin regardless.
// ignores pixels with value bg, connects if abs diff less than or equal to th
// Can supply a similar-size 2 field image in vic to prevent allocation.
// Returns area of largest blobs

int jhcGroup::RemSmallGray (jhcImg& dest, const jhcImg& src, double arel, int amin, 
                            int diff, int bg, jhcImg *vic)
{
  int n;
  jhcImg *marks = vic;

  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcGroup::RemSmallGray");
  if (arel < 0)
    return 0;
  dest.CopyRoi(src);
  
  if ((vic == NULL) || !dest.SameSize(*vic, 2))
    marks = tmp.SetSize(dest, 2);
  marks->CopyRoi(src);

  n = scan_diff(*marks, src, diff, bg);
  return thresh_labels(dest, *marks, n, amin, arel, 0);
}


//= Uses 4-connected components to fill holes less than or equal hmax pixels.
// Foreground = src pixels over th, or pixels under -th if th is negative.
// Can supply a similar-size 2 field image in vic to prevent allocation.
// Similar to Complement, RemSmall, Complement except no arel

int jhcGroup::FillHoles (jhcImg& dest, const jhcImg& src, int hmax, int th, 
                         jhcImg *vic)
{
  int n;
  jhcImg *marks = vic;

  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcGroup::FillHoles");
  if (hmax <= 0)
    return 0;
  dest.CopyRoi(src);
  
  if ((vic == NULL) || !dest.SameSize(*vic, 2))
    marks = tmp.SetSize(dest, 2);
  marks->CopyRoi(src);

  n = scan_labels(*marks, src, -(th + 1));
  thresh_labels(dest, *marks, n, hmax + 1, 0.0, 1);
  return 1;
}


//= Pixels above threshold count as objects, rest is background.
// Removes objects smaller than amin, or less than arel times largest
// then fills holes smaller than hmax. NOTE: fg areas checked BEFORE fill.
// @deprecated !!! DOES NOT WORK QUITE RIGHT !!!
int jhcGroup::CleanUp (jhcImg& dest, const jhcImg& src, int th, 
                       int amin, double arel, int hmax, jhcImg *vic)
{
  int n;
  jhcImg *marks = vic;

  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcGroup::CleanUp");
  if (hmax <= 0)
    return 0;
  dest.CopyRoi(src);
  
  if ((vic == NULL) || !dest.SameSize(*vic, 2))
    marks = tmp.SetSize(dest, 2);
  marks->CopyRoi(src);

  n = scan_dual(*marks, src, th);
  thresh_dual(dest, *marks, n, amin, arel, hmax, 0.0);
  return 1;
}


//= First pass of labelling -- may generate several labels for a region.
// Computes components both for regions over th and less than or equal th.
// Uses odd labels for foreground, even for background.
// Returns number of labels used, global "areas" holds counts and linkages.

int jhcGroup::scan_dual (jhcImg& dest, const jhcImg& src, int th)
{
  dest.CopyRoi(src);

  // general ROI case
  int lim, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk2 = dest.RoiSkip() >> 1, ssk = src.RoiSkip();
  int label, maxblob, n = 0;
  const UC8 *s = src.RoiSrc();
  UC8 *d0 = dest.RoiDest();
  US16 *d = (US16 *) d0;
  const US16 *dlf = d - 1, *ddn = (US16 *)(d0 - dest.Line());

  // make and initialize area and pointer array
  maxblob = rw * rh;
  if (areas.Size() < maxblob)
    areas.SetSize(maxblob);
  lim = areas.Last();
  areas.ASet(0, 0);  

  // do first pass over image labelling all foreground areas
  for (y = rh; y > 0; y--, d += dsk2, ddn += dsk2, dlf += dsk2, s += ssk)
    for (x = rw; x > 0; x--, d++, ddn++, dlf++, s++)
    {
      label = 0;

      if (*s <= th)                  // BG case
      {    
        // check below and left neighbors (if any) 
        if ((y < rh) && ((*ddn & 0x01) == 0))
          label = merge_labels(label, *ddn);             
        if ((x < rw) && ((*dlf & 0x01) == 0))
          label = merge_labels(label, *dlf);             

        // if no label copied then create new one, assign to pixel
        // makes sure label is even for background
        if ((label == 0) && (n < lim))
        {
          label = ++n;
          if ((label & 0x01) != 0)
          {
            areas.ASet(label, 0);
            label = ++n;
          }
          areas.ASet(label, 1);          
        }
      }
      else                           // FG case
      {
        // check below and left neighbors (if any) 
        if ((y < rh) && ((*ddn & 0x01) != 0))
          label = merge_labels(label, *ddn);             
        if ((x < rw) && ((*dlf & 0x01) != 0))
          label = merge_labels(label, *dlf);             

        // if no label copied then create new one, assign to pixel
        // makes sure label is odd for foreground 
        if (label == 0)
        { 
          label = ++n;
          if ((label & 0x01) == 0)
          {
            areas.ASet(label, 0);
            label = ++n;
          }
          areas.ASet(label, 1);
        }
      }

      // set pixel label 
      *d = (US16) label;
    }
  return n;
}


//= Keep only labels with lots of area by marking these pixels as 255.
// Regions (odd) must be greater than amin in area and at least arel * biggest.
// Holes (even) with area smaller than hmax are removed by setting them to 255.
// Assumes region info stored in global "areas" from first pass (ScanLabels).
// Returns total number of blobs found.

int jhcGroup::thresh_dual (jhcImg& dest, const jhcImg& marks, int n, 
                           int amin, double arel, int hmax, double hrel)
{
  if (n >= areas.Size())
    return -2;
  dest.CopyRoi(marks);

  // general ROI case
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk2 = marks.RoiSkip() >> 1;
  int i, label, old, big, hbig, win, cnt = 0;
  UC8 *d = dest.RoiDest();
  const US16 *s = (US16 *) marks.RoiSrc();

  // find biggest unfilled blob (odd step = fg only)
  if ((arel > 0.0) || (hrel > 0.0))
  {
    win = amin;
    for (i = 1; i <= n; i += 2)
      if (areas.ARef(i) > win)
        win = areas.ARef(i);   
  }

  // adjust size threshold to be relative if requested
  big = amin;
  if (arel > 0.0)
  {
    big = (int)(arel * win + 0.5);
    big = __max(big, amin);
  }
  hbig = hmax;
  if (hrel > 0.0)
  {
    hbig = (int)(hrel * win + 0.5);
    hbig = __max(hbig, hmax);
  }

  // first pass
  // convert from areas and pointers into canonical labels for regions
  for (i = 1; i <= n; i++)
  {
    // handle odd entries (foreground regions)
    if (areas.ARef(i) >= big)      
    {
      areas.ASet(i, 255);          // if big enough assign new label
      cnt++;
    }
    else if (areas.ARef(i) > 0)
      areas.ASet(i, 0);            // if too small then erase

    // handle even entries (background regions)
    i++;
    if (areas.ARef(i) > hbig)
      areas.ASet(i, 0);            // if too big then keep as background
    else if (areas.ARef(i) > 0)
      areas.ASet(i, 255);          // if small enough then fill in 
  }

  // second pass
  // otherwise (negative means pointer) copy new label of canonical class
  // since this class is smallest, new label has already been assigned 
  for (i = 1; i <= n; i++)
    if (areas.ARef(i) < 0)
    {
      old = i;
      while ((label = areas.ARef(old)) < 0)
        old = -label;
      areas.ASet(i, label);
    }

  // scan image and convert class markers to foreground markings
  for (y = rh; y > 0; y--, d += dsk, s += ssk2)
    for (x = rw; x > 0; x--, d++, s++)
      *d = (UC8) areas.ARef(*s);
  return cnt;
}


//= Similar to CleanUp but fills holes below some fraction of biggest blob.
// @deprecated !!! DOES NOT WORK QUITE RIGHT !!!
int jhcGroup::PrunePatch (jhcImg& dest, const jhcImg& src, int th, 
                          int amin, double arel, double hrel, jhcImg *vic)
{
  int n;
  jhcImg *marks = vic;

  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcGroup::PrunePatch");
  if (amin <= 0)
    return 0;
  dest.CopyRoi(src);
  
  if ((vic == NULL) || !dest.SameSize(*vic, 2))
    marks = tmp.SetSize(dest, 2);
  marks->CopyRoi(src);

  n = scan_dual(*marks, src, th);
  thresh_dual(dest, *marks, n, amin, arel, 0, hrel);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Sets destination image to 255 wherever component n pixels are.
// clears image at beginning only if clr is positive (so can be used to append items)

int jhcGroup::MarkComp (jhcImg& dest, const jhcImg& marks, int n, int clr) const
{
  if (!dest.Valid(1) || !dest.SameSize(marks, 2))
    return Fatal("Bad images to jhcGroup::MarkComp");
  dest.CopyRoi(marks);
  if (clr > 0)
    dest.FillArr(0);
  if (n <= 0)
    return 1;

  // general ROI case
  int x, y, rw = marks.RoiW(), rh = marks.RoiH();
  int dsk = dest.RoiSkip(), ssk = marks.RoiSkip() >> 1;
  const US16 *s = (US16 *) marks.RoiSrc();
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--, s += ssk, d += dsk)
    for (x = rw; x > 0; x--, s++, d++)
      if (*s == n)
        *d = 255;
  return 1;
}


//= Draws border of some component over top the existing destination image.
// more efficient if bounding box of dest is set (e.g. from jhcBBox::GetRoi(n))
// if so, should generally be careful to call dest.MaxRoi() afterward
// returns number of border pixels drawn (rough measure of perimeter)

int jhcGroup::DrawBorder (jhcImg& dest, const jhcImg& marks, int n, int r, int g, int b) const
{
  if (!dest.Valid(1, 3) || !dest.SameSize(marks, 2))
    return Fatal("Bad images to jhcGroup::DrawBorder");
  if (n <= 0)
    return 0;
  dest.MergeRoi(marks);

  // general ROI case
  int sln2 = marks.Line() >> 1, ssk2 = marks.RoiSkip(dest) >> 1;
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), cnt = 0;
  const US16 *s = (US16 *) marks.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  // monochrome image marking
  if (dest.Valid(1) > 0)
  {
    for (y = rh; y > 0; y--, d += dsk, s += ssk2)
      for (x = rw; x > 0; x--, d++, s++)
        if (*s == n)
          if ((y == rh) || (y == 1) || (x == rw) || (x == 1) || chk_around(s, sln2, n))
          {
            *d = (UC8) r;
            cnt++;
          }
    return cnt;
  }

  // color image marking
  for (y = rh; y > 0; y--, d += dsk, s += ssk2)
    for (x = rw; x > 0; x--, d += 3, s++)
      if (*s == n)
        if ((y == rh) || (y == 1) || (x == rw) || (x == 1) || chk_around(s, sln2, n))
        {
          d[0] = (UC8) b;
          d[1] = (UC8) g;
          d[2] = (UC8) r;
          cnt++;
        }
  return cnt;
}


//= See if any 8 connected neighbors of pixel are background or a different component.
// assumes image bounds checking already done so safe to retrieve pixels in all directions

bool jhcGroup::chk_around (const US16 *s, int sln2, int n) const
{
  int dx, dy;

  for (dy = -sln2; dy <= sln2; dy += sln2)
    for (dx = -1; dx <= 1; dx++)
      if (s[dy + dx] != n)
        return true;
  return false;
}

