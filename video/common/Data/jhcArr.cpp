// jhcArr.cpp : creating and accessing array whihc know their size
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "Interface/jhcMessage.h"

#include "Data/jhcArr.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor.

jhcArr::~jhcArr () 
{ 
  dealloc_arr();
}


//= Construct the shell for new array, but do not lock in size yet.

jhcArr::jhcArr ()
{
  init_arr();
  SetSize(1);           // to prevent errors with unsized arrays
}


//= Construct a new array with same size as reference array.

jhcArr::jhcArr (const jhcArr& ref)
{
  init_arr();
  SetSize(ref);
}


//= Construct a new array with same size as reference array pointed to.

jhcArr::jhcArr (const jhcArr *ref)
{
  init_arr();
  SetSize(*ref);
}


//= Construct a new array of specified size.
// if "no_init" is zero, array filled with zero automatically

jhcArr::jhcArr (int n, int no_init)
{
  init_arr();
  SetSize(n);
  if (no_init == 0)
    Fill(0);
}


//= Get rid of any array that has been created.

void jhcArr::dealloc_arr ()
{
  if (arr != NULL) 
    delete [] arr;
  init_arr();
}


//= Set up defaults for values.

void jhcArr::init_arr ()
{
  status = 1;
  len = 1;
  sz = 0;
  i0 = 0;
  i1 = 0;
  fn = 0;
  arr = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                         Size Specifications                           //
///////////////////////////////////////////////////////////////////////////

//= Set array size to match some other already existing array.

jhcArr *jhcArr::SetSize (const jhcArr& ref)
{
  return SetSize(ref.sz);
}


//= Allocate underlying array for values.

jhcArr *jhcArr::SetSize (int n)
{
  // sanity check
#ifdef _DEBUG
  if ((n <= 0) || (n > 10000000))
    Pause("jhcArr::SetSize - Trying to allocate an array of size %ld", n);
#endif

  // reallocate if different size requested
  if ((n > 0) && (n != sz))
  {
    dealloc_arr();
    arr = new int [n];
    if (arr == NULL)
      Fatal("jhcArr::SetSize - Buffer allocation [%d] failed!", n); 
    sz = n;
    i1 = sz;
  }
  return this;
}


//= Make sure array is some minimum size.
// uses existing array (if any) provided it is big enough

jhcArr *jhcArr::MinSize (int n)
{
  if (n > sz)
    return SetSize(n);
  return this;
}


//= Check that lengths match.

int jhcArr::SameSize (const jhcArr& ref) const
{
  if ((ref.sz <= 0) || (ref.sz != sz))
    return 0;
  return 1;
}


//= Returns 1 if test array is really this array.

int jhcArr::SameArr (const jhcArr& tst) const
{
  if (&tst == this)
    return 1;
  return 0;
}


//= Limit statistical computations (only) to a subrange.
// if value given is negative, no change is made to that value

void jhcArr::SetLimits (int start, int end)
{
  if (start >= 0)
    i0 = __min(start, __max(0, sz - 1));
  if (end >= 0)
    i1 = __min(end, sz);
}


//= Like SetLimits but takes a fractional specification.

void jhcArr::SetLims (double start, double end)
{
  SetLimits((int)(start * sz + 0.5), (int)(end * sz + 0.5));
}


//= Copy limits from some other array.

void jhcArr::CopyLims (const jhcArr& ref)
{
  SetLimits(ref.i0, ref.i1);
}


//= Resets to full range.

void jhcArr::MaxLims ()
{
  SetLims(0.0, 1.0);
}


///////////////////////////////////////////////////////////////////////////
//                          Simple Operations                            //
///////////////////////////////////////////////////////////////////////////

//= Fill all entries in array with some value.
// also rewinds scrolling data index for convenience

void jhcArr::Fill (int def)
{
  int i;

  for (i = 0; i < sz; i++)
    arr[i] = def;
  fn = 0;
}


//= Fill all entries below given position with some value.

void jhcArr::LeftFill (int pos, int val)
{
  int i, stop = __min(pos, sz);

  for (i = 0; i < stop; i++)
    arr[i] = val;
}


//= Fill bins from start to end (inclusive) with given value.

void jhcArr::FillSpan (int start, int end, int val)
{
  int i, lim = __min(sz - 1, end);

  for (i = __max(0, start); i <= lim; i++)
    arr[i] = val;
}


//= Fill bins strictly below start and above end with given value.

void jhcArr::Matte (int start, int end, int val)
{
  int i;

  for (i = 0; i <= start; i++)
    arr[i] = val;
  for (i = __max(0, end); i < sz; i++)
    arr[i] = val;
}


//= Fill all entries above given position with some value.

void jhcArr::RightFill (int pos, int val)
{
  int i;

  for (i = pos + 1; i < sz; i++)
    arr[i] = val;
}


//= Duplicate values from another sized array.

void jhcArr::Copy (const jhcArr& src)
{
  Copy(src.arr, src.sz);
}


//= Read in values from a normal array.

void jhcArr::Copy (const int vals[], int n)
{
  int i, lim = __min(sz, n);

  if (vals != arr)
    for (i = 0; i < lim; i++)
      arr[i] = vals[i];
}


//= Add new value at highest index, move all others down.
// ripple takes some time - consider Scroll instead

void jhcArr::Push (int val)
{
  int i, lim = sz - 1;

  for (i = 0; i < lim; i++)
    arr[i] = arr[i + 1];
  arr[lim] = val;    
}


//= Remove value at highest index, move all others up.
// fill lowest bin with value given - ripple takes some time

int jhcArr::Pop (int fill)
{
  int i, lim = sz - 1;
  int ans = arr[lim];

  for (i = lim; i > 0; i--)
    arr[i] = arr[i - 1];
  arr[0] = fill;
  return ans;
}


//= Move all values one direction or the other a certain amount.
// fill new bins with value given

void jhcArr::Shift (int amt, int fill)
{
  int i, lim;

  if (amt > 0)
  {
    for (i = sz - 1; i >= amt; i--)
      arr[i] = arr[i - amt];
    for (i = amt - 1; i >= 0; i--)
      arr[i] = fill;
  }
  else if (amt < 0)
  {
    lim = sz + amt; 
    for (i = 0; i < lim; i++)
      arr[i] = arr[i - amt];
    for (i = lim; i < sz; i++)
      arr[i] = fill;
  } 
}
  

//= Copy array with bins flipped around mid bin (which stays the same).
// basically does a reflection around the designated middle point

int jhcArr::FlipAround (const jhcArr& src, int mid)
{
  int i, j, m2 = mid << 1, lim = src.sz - 1;

  if (&src == this)
    Fatal("Bad image to jhcArr::FlipAround");

  for (i = 0; i < sz; i++)
  {
    j = m2 - i;
    if ((j >= 0) && (j <= lim))
      arr[i] = src.arr[j];
    else
      arr[i] = 0;
  }
  return 1;
}


//= Blows up lateral scale around a particular bin.
// performs linear interpolation for intermediate bins

void jhcArr::Zoom (const jhcArr& src, int mid, double factor)
{
  int i, base, lim = src.sz - 1;
  double hmix, step = 1.0 / factor, samp = mid - 0.5 * sz * step;

  if (&src == this)
    Fatal("Bad image to jhcArr::Zoom");

  Fill(0);
  for (i = 0; i < sz; i++)
  {
    base = (int) samp;
    hmix = samp - base;
    if ((base >= 0) && (base < lim) && (hmix >= 0.0))
      arr[i] = (int)((1.0 - hmix) * src.arr[base] + hmix * src.arr[base + 1] + 0.5);
    samp += step;
  }
}


//= Blows up lateral scale starting from a particular bin.
// performs linear interpolation for intermediate bins

void jhcArr::Magnify (const jhcArr& src, int bot, double factor)
{
  int i, base, lim = src.sz - 1;
  double hmix, step = 1.0 / factor, samp = bot;

  if (&src == this)
    Fatal("Bad image to jhcArr::Magnify");

  Fill(0);
  for (i = 0; i < sz; i++)
  {
    base = (int) samp;
    hmix = samp - base;
    if ((base >= 0) && (base < lim) && (hmix >= 0.0))
      arr[i] = (int)((1.0 - hmix) * src.arr[base] + hmix * src.arr[base + 1] + 0.5);
    samp += step;
  }
}


///////////////////////////////////////////////////////////////////////////
//                          Primitive Access                             //
///////////////////////////////////////////////////////////////////////////

//= Bounds checking read of data.

int jhcArr::ARefChk (int n, int def) const
{
  if ((n >= 0) && (n < sz))
    return aref0(n);
  return def; 
}


//= Bounds checking write of data.

int jhcArr::ASetChk (int n, int val)
{
  if ((n >= 0) && (n < sz))
  {
    aset0(n, val);
    return 1;
  }
  return 0; 
}


//= Bounds checking increment of data value.

int jhcArr::AIncChk (int n, int amt)
{
  if ((n >= 0) && (n < sz))
  {
    ainc0(n, amt);
    return 1;
  }
  return 0; 
}


//= Bounds checking read of data.
// complains if index is out of bounds

int jhcArr::ARefX (int n, int def) const
{
  if ((n >= 0) && (n < sz))
    return aref0(n);
  Pause("jhcArr::ARefX - %ld is beyond size %ld", n, sz); 
  return def; 
}


//= Bounds checking write of data.
// complains if index is out of bounds

int jhcArr::ASetX (int n, int val)
{
  if ((n >= 0) && (n < sz))
  {
    aset0(n, val);
    return 1;
  }
  Pause("jhcArr::ASetX - %ld is beyond size %ld", n, sz); 
  return 0; 
}


//= Bounds checking increment of data value.
// complains if index is out of bounds

int jhcArr::AIncX (int n, int amt)
{
  if ((n >= 0) && (n < sz))
  {
    ainc0(n, amt);
    return 1;
  }
  Pause("jhcArr::AIncX - %ld is beyond size %ld", n, sz); 
  return 0; 
}


//= Bounds checking maximum of data.
// complains if index is out of bounds

int jhcArr::AMaxX (int n, int val)
{
  if ((n >= 0) && (n < sz))
  {
    amax0(n, val);
    return 1;
  }
  Pause("jhcArr::AMaxX - %ld is beyond size %ld", n, sz); 
  return 0; 
}


//= Bounds checking minimum of data.
// complains if index is out of bounds

int jhcArr::AMinX (int n, int val)
{
  if ((n >= 0) && (n < sz))
  {
    amin0(n, val);
    return 1;
  }
  Pause("jhcArr::AMinX - %ld is beyond size %ld", n, sz); 
  return 0; 
}


///////////////////////////////////////////////////////////////////////////
//                         Statistics of Values                          //
///////////////////////////////////////////////////////////////////////////

//= Return average occupancy of each bin in array (or subrange).

double jhcArr::AvgVal (int all) const
{
  int i, lo = i0, hi = i1, sum = 0;

  if (all > 0)
  {
    lo = 0;
    hi = sz;
  }
  for (i = lo; i < hi; i++)
    sum += arr[i];
  if (sum == 0)
    return 0;  
  return(sum / (double)(hi - lo));
}


//= Return average occupancy of each bin in subrange of array.

double jhcArr::AvgRegion (int lo, int hi) const
{
  int i, start = __max(0, lo), stop = __min(hi, sz - 1), sum = 0;

  for (i = start; i <= stop; i++)
    sum += arr[i];
  if (sum == 0)
    return 0;  
  return(sum / (double)(stop - start + 1));
}


//= Find biggest value in array (or subrange).

int jhcArr::MaxVal (int all) const
{
  int i, lo = i0, hi = i1, top = arr[i0];

  if (all > 0)
  {
    lo = 0;
    hi = sz;
  }
  for (i = lo + 1; i < hi; i++)
    if (arr[i] > top)
      top = arr[i];
  return top;
}


//= Find biggest value in subrange of array.

int jhcArr::MaxRegion (int lo, int hi) const
{
  int i, start = __max(0, lo), stop = __min(hi, sz - 1), top = arr[start];

  for (i = start + 1; i <=stop; i++)
    if (arr[i] > top)
      top = arr[i];
  return top;
}


//= Find smallest value in array (or subrange).

int jhcArr::MinVal (int all) const
{
  int i, lo = i0, hi = i1, bot = arr[i0];

  if (all > 0)
  {
    lo = 0;
    hi = sz;
  }
  for (i = lo + 1; i < hi; i++)
    if (arr[i] < bot)
      bot = arr[i];
  return bot;
}


//= Find smallest value in subrange of array.

int jhcArr::MinRegion (int lo, int hi) const
{
  int i, start = __max(0, lo), stop = __min(hi, sz - 1), bot = arr[start];

  for (i = start + 1; i <= stop; i++)
    if (arr[i] < bot)
      bot = arr[i];
  return bot;
}


//= Find smallest value above zero in array (or subrange).

int jhcArr::MinNZ (int all) const
{
  int i, lo = i0, hi = i1, bot = 0;

  if (all > 0)
  {
    lo = 0;
    hi = sz;
  }
  for (i = lo; i < hi; i++)
    if (arr[i] > 0)
      if ((bot <= 0) || (arr[i] < bot))
        bot = arr[i];
  return bot;
}


//= Find the largest positive or negative value in array (or subrange).
// useful for scaling graphs

int jhcArr::MaxAbs (int all) const
{
  int i, lo = i0, hi = i1, top = arr[i0], bot = top;

  if (all > 0)
  {
    lo = 0;
    hi = sz;
  }
  for (i = lo + 1; i < hi; i++)
  {
    top = __max(top, arr[i]);
    bot = __min(bot, arr[i]);
  }
  return __max(top, -bot);
}


//= Return sum of all values in array (or subrange).

int jhcArr::SumAll (int all) const
{
  int i, lo = i0, hi = i1, sum = 0;
  
  if (all > 0)
  {
    lo = 0;
    hi = sz;
  }
  for (i = lo; i < hi; i++)
    sum += arr[i];
  return sum;
}


//= Returns sum of counts in selected area (inclusive).
// if lo bigger than hi, computes area strictly outside region
// can optionally scale bounds to range of 0 to vmax
// ignores index limits (if any)

int jhcArr::SumRegion (int lo, int hi, int vmax) const
{
  int i, bot = lo, top = hi, sum = 0;

  // compute region bounds
  if (vmax != 0)
  {
    bot = (int)(bot * sz / (double) vmax);
    top = (int)(top * sz / (double) vmax);
  }
  bot = __max(0, __min(bot, sz - 1));
  top = __max(0, __min(top, sz - 1));

  // sum either inside interval or outside it
  if (bot <= top)
    for (i = bot; i <= top; i++)
      sum += arr[i];
  else
  {
    for (i = 0; i < top; i++)
      sum += arr[i];
    for (i = bot + 1; i < sz; i++)
      sum += arr[i];
  }
  return sum;
}


//= Returns the number of bins that are above the threshold value.
// respects subrange settings (if any)

int jhcArr::CountOver (int val, int all) const
{
  int i, lo = i0, hi = i1, sum = 0;
  
  if (all > 0)
  {
    lo = 0;
    hi = sz;
  }
  for (i = lo; i < hi; i++)
    if (arr[i] > val)
      sum++;
  return sum;
}


///////////////////////////////////////////////////////////////////////////
//                       Finding particular bins                         //
///////////////////////////////////////////////////////////////////////////

//= Return the bin with the highest value in array (or subrange).
// if several are tied returns lowest index for bias 0, else highest

int jhcArr::MaxBin (int bias) const
{
  int i, win = i0, top = arr[i0];

  for (i = i0 + 1; i < i1; i++) 
    if ((arr[i] > top) || 
        ((arr[i] == top) && (bias > 0)))
    {
      win = i;
      top = arr[i];
    }
  return win;
}


//= Like other MaxBin but only considers certain portion of histogram.

int jhcArr::MaxBin (int lo, int hi, int bias) const
{
  int start = __min(lo, __max(0, sz - 1)), end = __min(hi, sz);
  int i, win = start, top = arr[start];

  for (i = start + 1; i < end; i++) 
    if ((arr[i] > top) || 
        ((arr[i] == top) && (bias > 0)))
    {
      win = i;
      top = arr[i];
    }
  return win;
}


//= Like other MaxBin but part described by fractional ranges.

int jhcArr::MaxBin (double lo, double hi, int bias) const
{
  int s = (int)(lo * sz + 0.5), e = (int)(hi * sz + 0.5);
  int start = __min(s, __max(0, sz - 1)), end = __min(e, sz);
  int i, win = start, top = arr[start];

  for (i = start + 1; i < end; i++) 
    if ((arr[i] > top) || 
        ((arr[i] == top) && (bias > 0)))
    {
      win = i;
      top = arr[i];
    }
  return win;
}


//= Return the bin with the lowest value in array (or subrange).
// if several are tied returns lowest index is bias 0, else highest

int jhcArr::MinBin (int bias) const
{
  int i, win = i0, bot = arr[i0];

  for (i = i0 + 1; i < i1; i++)
    if ((arr[i] < bot) ||
        ((arr[i] == bot) && (bias > 0)))
    {
      win = i;
      bot = arr[i];
    }
  return win;
}


//= Like other MinBin but only considers certain portion of histogram.

int jhcArr::MinBin (int lo, int hi, int bias) const
{
  int start = __min(lo, __max(0, sz - 1)), end = __min(hi, sz);
  int i, win = start, bot = arr[start];

  for (i = start + 1; i < end; i++)
    if ((arr[i] < bot) ||
        ((arr[i] == bot) && (bias > 0)))
    {
      win = i;
      bot = arr[i];
    }
  return win;
}


//= Like other MinBin but part described by fractional ranges.

int jhcArr::MinBin (double lo, double hi, int bias) const 
{
  int s = (int)(lo * sz + 0.5), e = (int)(hi * sz + 0.5);
  int start = __min(s, __max(0, sz - 1)), end = __min(e, sz);
  int i, win = start, bot = arr[start];

  for (i = start + 1; i < end; i++)
    if ((arr[i] < bot) ||
        ((arr[i] == bot) && (bias > 0)))
    {
      win = i;
      bot = arr[i];
    }
  return win;
}


//= Find highest bin index such that N counts are at or above this. 

int jhcArr::MaxBinN (int n) const
{
  int win, cnt = 0;

  for (win = i1 - 1; win >= 0; win--)
  {
    cnt += arr[win];
    if (cnt >= n)
      break;
  }
  return __max(0, win);
}


//= Find lowest bin index such that N counts are at or below this. 

int jhcArr::MinBinN (int n) const
{
  int win, cnt = 0;

  for (win = i0; win < i1; win++)
  {
    cnt += arr[win];
    if (cnt >= n)
      break;
  }
  return __min(win, i1 - 1);
}


//= Return the bin index closest to the centroid of the distribution.
// computation can be limited to a subrange using SetLims

int jhcArr::AvgBin () const
{
  int i, wts, wsum = 0;

  wts = SumAll();
  for (i = i0; i < i1; i++)
    wsum += (i * arr[i]);
  if (wts == 0)
    return 0;
  return((int)(wsum / (double) wts + 0.5));
}


//= Compute standard deviation in terms of bins (possibly in subrange).

int jhcArr::SDevBins () const
{
  return((int)(SDevFrac() + 0.5));
}


//= Compute standard deviation from mean bin (possibly in subrange).

double jhcArr::SDevFrac () const
{
  int i, w, wts = 0, wsum = 0, w2sum = 0;
  double ex, ex2;

  for (i = i0; i < i1; i++)
  {
    w = arr[i];
    wts += w;
    wsum += (w * i);
    w2sum += (w * i * i);
  }
  if (wts == 0)
    return 0;
  ex = wsum / (double) wts;
  ex2 = w2sum / (double) wts;
  return sqrt(ex2 - ex * ex);
}


//= Find the average deviation below some specified bin.
// basically a one-sided standard deviation

double jhcArr::SDevUnder (int mid) const
{
  int i, top = __min(mid, sz), cnt = 0, sum = 0;

  for (i = 0; i < top; i++)
  {
    sum += arr[i] * (mid - i) * (mid - i);
    cnt += arr[i];
  }
  if (cnt <= 0)
    return 0.0;
  return sqrt(sum / (double) cnt);
}


//= Find the average deviation above some specified bin.
// basically a one-sided standard deviation

double jhcArr::SDevOver (int mid) const
{
  int i, bot = __max(0, mid), cnt = 0, sum = 0;

  for (i = bot; i < sz; i++)
  {
    sum += arr[i] * (i - mid) * (i - mid);
    cnt += arr[i];
  }
  if (cnt <= 0)
    return 0.0;
  return sqrt(sum / (double) cnt);
}


//= Return the index of the bin just beyond that needed to
// capture "frac" of the mass at the lower end (or subrange).

int jhcArr::Percentile (double frac) const
{
  int i, targ, sum = 0;

  targ = (int)(frac * SumAll() + 0.5);
  for (i = i0; i < i1; i++)
  {
    sum += arr[i];
    if (sum >= targ)
    {
      if ((i > 0) && ((sum - targ) > (arr[i] >> 1)))
        return(i - 1);
      return i;
    }
  }
  return -1;
}


//= Return the bin index that splits the population in half.
// computation can be limited to a subrange

int jhcArr::MedianBin () const
{
  return Percentile(0.5);
}


//= Find middle of mass of distribution.

int jhcArr::Centroid () const
{
  int i, ans = 0, sum = 0, prod = 0;

  for (i = i0; i < i1; i++)
  {
    sum += arr[i];
    prod += (i * arr[i]);
  }
  if (sum > 0)
    ans = (int)(prod / (double) sum + 0.5);
  return ans;
}


//= Refine peak position to sub-pixel precision.
// works with normal or cyclic histograms

double jhcArr::SubPeak (int pk, int cyc) const
{
  double lf, rt, mid, best = pk;
  int n;

  // sanity check
  if (pk < 0)
    return 0.0;
  if (pk >= sz)
    return sz;
  mid = arr[pk];

  // find value in left adjacent bin
  n = pk - 1;
  if (n >= 0)
    lf = arr[n];  
  else if (cyc > 0)
    lf = arr[n + sz];
  else
    lf = 0.0;

  // find value in right adjacent bin
  n = pk + 1;
  if (n < sz)
    rt = arr[n];
  else if (cyc > 0)
    rt = arr[n - sz];
  else
    rt = 0.0;

  // use parabolic interpolation to get fractional bin center
  if ((lf <= mid) && (rt <= mid) && ((mid > lf) || (mid > rt)))
    best += (rt - lf) / (2.0 * mid - (lf + rt)); 

  // handle any wrap around 
  if (best < 0.0)
  {
    if (cyc > 0)
      return(best + sz);
    return 0.0;
  }
  if (best > sz)
  {
    if (cyc > 0)
      return(best - sz);
    return sz;
  }
  return best;
}


///////////////////////////////////////////////////////////////////////////
//                           Examining Peaks                             //
///////////////////////////////////////////////////////////////////////////

//= Return lowest index of bin below specified threshold.
// respects subrange specifications
// if force is positive, possibly returns end range

int jhcArr::FirstUnder (int th, int force) const
{
  int i;

  for (i = i0; i < i1; i++)
    if (arr[i] <= th)
      return i;
  if (force > 0)
    return(i1 - 1);
  return -1;
}


//= Return highest index of bin below specified threshold.

int jhcArr::LastUnder (int th, int force) const
{
  int i;

  for (i = i1 - 1; i >= i0; i--)
    if (arr[i] <= th)
      return i;
  if (force > 0)
    return i0;
  return -1;
}


//= Return highest index of bin in range below specified threshold.
// uses fractions of max bin number to define range

int jhcArr::LastUnder (int th, double lo, double hi, int force) const
{
  int s = (int)(lo * sz + 0.5), e = (int)(hi * sz + 0.5);
  int i, start = __min(s, __max(0, sz - 1)), end = __min(e, sz);

  for (i = end - 1; i >= start; i--)
    if (arr[i] <= th)
      return i;
  if (force > 0)
    return i0;
  return -1;
}


//= Return lowest index of bin above specified threshold.

int jhcArr::FirstOver (int th, int force) const
{
  int i;

  for (i = i0; i < i1; i++)
    if (arr[i] > th)
      return i;
  if (force > 0)
    return(i1 - 1);
  return -1;
}


//= Return highest index of bin above specified threshold.

int jhcArr::LastOver (int th, int force) const
{
  int i;

  for (i = i1 - 1; i >= i0; i--)
    if (arr[i] > th)
      return i;

  if (force > 0)
    return i0;
  return -1;
}


//= Look for closest bin preceeding peak which is frac of peak's value.

int jhcArr::PeakRise (int peak, double frac, int force) const
{
  int i, end = __min(peak, i1 - 1);
  int th = (int)(frac * ARef(peak) + 0.5);

  for (i = end; i >= i0; i--)
    if (arr[i] <= th)
      return i;
  if (force > 0)
    return i0;
  return -1;
}


//= Look for closest bin following peak less than frac of peak's value.
// stops looking when it reaches lo fraction of array size
// unlike other PeakRise the found bin is strictly less than peak

int jhcArr::PeakRiseLim (int peak, double frac, double lo, int force) const
{
  int s = (int)(lo * sz + 0.5), start = __min(s, __max(0, sz - 1));
  int i, th = (int)(frac * ARef(peak) + 0.5);

  for (i = peak - 1; i >= start; i--)
    if (arr[i] < th)
      return i;
  if (force > 0)
    return start;
  return -1;
}


//= Look for closest bin following peak which is frac of peak's value.

int jhcArr::PeakFall (int peak, double frac, int force) const
{
  int i, start = __max(i0, peak);
  int th = (int)(frac * ARef(peak) + 0.5);

  for (i = start; i < i1; i++)
    if (arr[i] <= th)
      return i;
  if (force > 0)
    return(i1 - 1);
  return -1;
}


//= Look for closest bin following peak less than frac of peak's value.
// stops looking when it reaches hi fraction of array size
// unlike other PeakFall the found bin is strictly less than peak

int jhcArr::PeakFallLim (int peak, double frac, double hi, int force) const
{
  int e = (int)(hi * sz + 0.5), end = __min(e, sz - 1);
  int i, th = (int)(frac * ARef(peak) + 0.5);

  for (i = peak + 1; i <= end; i++)
    if (arr[i] < th)
      return i;
  if (force > 0)
    return end;
  return -1;
}


//= Look for closest bin preceeding valley which is frac of valley's value.

int jhcArr::ValleyFall (int val, double frac, int force) const
{
  int i, end = __min(val, i1 - 1);
  int th = (int)(frac * ARef(val) + 0.5);

  for (i = end; i >= i0; i--)
    if (arr[i] >= th)
      return i;
  if (force > 0)
    return i0;
  return -1;
}


//= Look for closest bin following valley which is frac of valley's value.

int jhcArr::ValleyRise (int val, double frac, int force) const
{
  int i, start = __max(i0, val);
  int th = (int)(frac * ARef(val) + 0.5);

  for (i = start; i < i1; i++)
    if (arr[i] >= th)
      return i;
  if (force > 0)
    return(i1 - 1);
  return -1;
}


//= Look for closest bin preceding peak which is frac of peak's value.
// if any searched bin is greater, then take least found so far
// if come to end of search range, then report lowest dip
// if hump is non-negative then can never be more than this over peak
// if rise is non-negative then cannot rise more than this from lowest

int jhcArr::PeakLeft (int peak, double frac, int stop, double hump, double rise) const
{
  int i, end = i0;
  int orig = ARef(peak), ans = peak, best = orig;
  int th = (int)(frac * orig + 0.5);
  int top = (int)((1.0 + hump) * orig + 0.5);
  int chg = (int)(rise * orig + 0.5);

  if (stop >= 0) 
    end = __max(0, stop);
  for (i = peak; i >= end; i--)
  {
    if (arr[i] <= th)
      return i;
    if (arr[i] > top)
      return ans;
    if ((rise >= 0.0) && ((arr[i] - best) > chg))
      return ans;
    if (arr[i] < best)
    {
      best = arr[i];
      ans = i;
    }
  }
  return ans;
}


//= Look for closest bin following peak which is frac of peak's value.
// if any searched bin is greater, then take least found so far
// if come to end of search range, then report lowest dip
// if hump is non-negative then can never be more than this over peak
// if rise is non-negative then cannot rise more than this from lowest

int jhcArr::PeakRight (int peak, double frac, int stop, double hump, double rise) const
{
  int i, end = i1 - 1; 
  int orig = ARef(peak), ans = peak, best = orig;
  int th = (int)(frac * orig + 0.5);
  int top = (int)((1.0 + hump) * orig + 0.5);
  int chg = (int)(rise * orig + 0.5);

  if (stop >= 0) 
    end = __min(stop, sz - 1);
  for (i = peak; i <= end; i++)
  {
    if (arr[i] <= th)
      return i;
    if (arr[i] > top)
      return ans;
    if ((rise >= 0.0) && ((arr[i] - best) > chg))
      return ans;
    if (arr[i] < best)
    {
      best = arr[i];
      ans = i;
    }
  }
  return ans;
}


//= Descend to left until ascends by more than tol of peak.
// returns position of valley, -1 if no good valley found

int jhcArr::BoundLeft (int peak, double tol) const
{
  int i, bot = -1, val = arr[peak], th = ROUND(tol * val);

  for (i = peak - 1; i >= 0; i--)
    if ((arr[i] - val) > th)
      return bot;
    else if (arr[i] < val)
    {
      bot = i;
      val = arr[i];
      th = ROUND(tol * val);
    }
  return -1;
}


//= Descend to right until ascends by more than tol of peak.
// returns position of valley, -1 if no good valley found

int jhcArr::BoundRight (int peak, double tol) const
{
  int i, bot = -1, val = arr[peak], th = ROUND(tol * val);

  for (i = peak + 1; i < sz; i++)
    if ((arr[i] - val) > th)
      return bot;
    else if (arr[i] < val)
    {
      bot = i;
      val = arr[i];
    }
  return -1;
}


//= Return lowest index of a peak with reasonable definition.
// must be above "th" and fall to "tol" fraction of peak
// respects subrange specifications

int jhcArr::FirstSummit (int th, double tol) const
{
  int i, pos = i0, best = arr[pos];

  for (i = i0 + 1; i < i1; i++)
    if (arr[i] >= best)
    {
      best = arr[i];
      pos = i;
    }
    else if ((best >= th) && 
             (arr[i] < (int)(tol * best + 0.5)))
      return pos;
  return pos;
}


//= Return highest index of a peak with reasonable definition.
// must be above "th" and fall to "tol" fraction of peak
// respects subrange specifications

int jhcArr::LastSummit (int th, double tol) const
{
  int i, pos = i1 - 1, best = arr[pos];

  for (i = i1 - 1; i >= i0; i--)
    if (arr[i] >= best)
    {
      best = arr[i];
      pos = i;
    }
    else if ((best >= th) && 
             (arr[i] < (int)(tol * best + 0.5)))
      return pos;
  return pos;
}


//= Return lowest index of bin below specified threshold.
// stops at the preceeding valley if rise is too great
// respects subrange specifications

int jhcArr::FirstValley (int th, double tol) const
{
  int i, pos = i0, best = arr[pos];

  for (i = i0 + 1; i < i1; i++)
    if (arr[i] <= th)
      return i;
    else if (arr[i] <= best)
    {
      best = arr[i];
      pos = i;
    }
    else if (arr[i] > (int)(tol * best + 0.5))
      return pos;
  return pos;
}


//= Return highest index of bin below specified threshold.
// stops at the preceeding valley if rise is too great
// respects subrange specifications

int jhcArr::LastValley (int th, double tol) const
{
  int i, pos = i1 - 1, best = arr[pos];

  for (i = i1 - 1; i >= i0; i--)
    if (arr[i] <= th)
      return i;
    else if (arr[i] <= best)
    {
      best = arr[i];
      pos = i;
    }
    else if (arr[i] > (int)(tol * best + 0.5))
      return pos;
  return pos;
}


//= Starting at the given position climb and mark nearest peak.
// respects subrange specifications
// NOTE: returns the LARGER of the two adjacent peaks found

int jhcArr::NearestPeak (int pos) const
{
  int last, lf, rt;

  // look for monotonic peak to the right
  last = arr[pos];
  for (rt = pos + 1; rt < i1; rt++) 
    if (arr[rt] < last)
      break;
    else
      last = arr[rt];
  rt--;

  // look for monotonic peak to the left
  last = arr[pos];
  for (lf = pos - 1; lf >= i0; lf--) 
    if (arr[lf] < last)
      break;
    else
      last = arr[lf];
  lf++;

  // choose BIGGER of two
  if (arr[rt] > arr[lf])
    return rt;
  return lf;
}


//= Divide array into regions above th and find maximum in each.
// can optionally find only peaks below "pos" (any = 0)
// respects subrange specifications
// returns position of closest max or original reference position if no regions

int jhcArr::NearMassPeak (int pos, int th, int any) const
{
  int i, dist, best, top = 0, pk = -1, win = -1;

  // scan for contiguous regions over th
  for (i = i0; i <= i1; i++)
    if ((i < i1) && (arr[i] > th))
    {
      // possibly new region or better peak
      if ((pk < 0) || (arr[i] > top))
      {
        top = arr[i];        // always overwrites default
        pk = i;
      }
    }
    else if (pk >= 0) 
    {
      // possibly finished region so check if closest
      dist = abs(pos - pk);
      if ((any > 0) || (pos >= pk))
        if ((win < 0) || (dist < best))
        {
          best = dist;
          win = pk;
        }
      pk = -1;
    }
  return((win < 0) ? pos : win);
}


//= Looks to the left and right for the NEAREST adjacent peak.
// must decline to drop of starting peak value before next
// next peak must itself be over drop of starting point
// designed for peaks with deep gap between them

int jhcArr::AdjacentPeak (int pos, double drop) const
{
  int i, rt, lf, mode;
  int last, th = (int)(drop * ARef(pos) + 0.5);

  // descend to left, then rise monotonically
  mode = 0;
  for (i = pos - 1; i >= 0; i--)
    if ((mode <= 0) && (arr[i] <= th))
      mode = 1;
    else if ((mode > 0) && (arr[i] > th))
      break;
  last = arr[i];
  for (lf = i - 1; lf >= 0; lf--)
    if (arr[lf] < last)
      break;
    else
      last = arr[lf];
  lf++;
  lf = __max(0, lf);

  // descend to right, then rise monotonically
  mode = 0;
  for (i = pos + 1; i < sz; i++)
    if ((mode <= 0) && (arr[i] <= th))
      mode = 1;
    else if ((mode > 0) && (arr[i] > th))
      break;
  last = arr[i];
  for (rt = i + 1; rt < sz; rt++) 
    if (arr[rt] < last)
      break;
    else
      last = arr[rt];
  rt--;
  rt = __min(rt, sz - 1);

  // figure out which one is closer  
  if ((rt - pos) < (pos - lf))
    return rt;
  return lf;
}


//= Looks to the left and right for the BIGGEST adjacent peak.
// must decline to by drop fraction starting peak value before next
// only looks within +/- rng bins of given position
// designed for peaks with shallow gap between them
// returns -1 if no good option within range

int jhcArr::DualPeak (int pos, int rng, double dip) const
{
  int lo = __max(0, pos - rng), hi = __min(pos + rng, sz - 1);
  int last, val = arr[pos], th = (int)((1.0 - dip) * val + 0.5);
  int i, lf = lo, rt = hi, lmode = 0, rmode = 0;

  // correct for small fractions so a real drop happens
  if ((dip > 0.0) && (th == ARef(pos)))
    th--;

  // descend to left to bottom of valley, then rise monotonically
  last = val;
  for (i = pos - 1; i >= lo; i--)
  {
    if ((lmode <= 0) && (arr[i] <= th))
      lmode = 1;                               // initial dip found
    else if ((lmode > 0) && (arr[i] > last))   
    {
      lmode = 2;                               // turn around at valley
      break;
    }
    last = arr[i];
  }

  // rise monotonically to new peak (if valley found)
  if (lmode >= 2)
  {
    last = arr[i];
    for (lf = i - 1; lf >= lo; lf--)
    {
      if (arr[lf] < last)
      {
        lmode = 3;                             // mark success at crest
        break;
      }
      last = arr[lf];
    }
    lf++;
  }

  // descend to right, then rise monotonically
  last = val;
  for (i = pos + 1; i <= hi; i++)
  {
    if ((rmode <= 0) && (arr[i] <= th))
      rmode = 1;                               // initial dip found
    else if ((rmode > 0) && (arr[i] > last))   
    {
      rmode = 2;                               // turn around at valley
      break;
    }
    last = arr[i];
  }

  // rise monotonically to new peak (if valley found)
  if (rmode >= 2)
  {
    last = arr[i];
    for (rt = i + 1; rt <= hi; rt++) 
    {
      if (arr[rt] < last)
      {
        rmode = 3;                             // mark success at crest
        break;
      }
      last = arr[rt];
    }
    rt--;
  }

  // figure out which one is bigger (if both are valid)
  if ((lmode < 3) && (rmode < 3))
    return -1;
  if ((rmode >= 3) && (lmode < 3))
    return rt;
  if ((lmode >= 3) && (rmode < 3))
    return lf;
  if (arr[rt] > arr[lf])
    return rt;
  return lf;
}


//= Finds the highest peak not at limits of search range.
// unlike MaxBin peak must fall off on both sides before limits
// restricts search to some FRACTIONAL portion of whole array
// returns -1 if no true peak

int jhcArr::TruePeak (double lo, double hi, int bias) const
{
  int s = (int)(lo * sz + 0.5), e = (int)(hi * sz + 0.5);
  int start = __min(s, __max(0, sz - 1)), end = __min(e, sz - 1);
  int s2, e2, last;

  // find descending portion from left
  last = arr[start];
  for (s2 = start + 1; s2 <= end; s2++) 
    if (arr[s2] > last)
      break;
    else
      last = arr[s2];
  if (s2 > end)
    return -1;

  // find descending portion from right
  last = arr[end];
  for (e2 = end - 1; e2 >= s2; e2--) 
    if (arr[e2] > last)
      break;
    else
      last = arr[e2];
  if (e2 < s2)
    return -1;

  // find max between valleys at ends
  return MaxBin(s2, e2, bias);
}


//= Set values around identified peak to zero (non-return inhibition).
// looks for decrease of value to "drop" fraction of peak on each side

int jhcArr::ErasePeak (int pos, double drop)
{
  int i, th;

  // check for reasonable arguments
  if ((pos < 0) || (pos >= sz) || (drop < 0.0) || (drop > 1.0))
    return 0;
  th = ROUND(drop * arr[pos]);

  // erase left side
  for (i = pos; i >= 0; i--)
    if (arr[i] < th)
      break;
    else
      arr[i] = 0;

  // erase right side
  for (i = pos + 1; i < sz; i++)
    if (arr[i] < th)
      break;
    else
      arr[i] = 0;
  return 1;
}


//= Find best value within index limits (inclusive).
// returns -1 if all values are the same (no true max)

int jhcArr::TrueMax (int lo, int hi, int bias) const
{
  int end = ((lo <= hi) ? hi : hi + sz);
  int i, i0, v, any = 0, pk = lo, top = arr[lo]; 

  for (i = lo + 1; i <= end; i++)
  {
    i0 = i % sz;
    v = arr[i0];
    if ((any <= 0) && (v != top))
      any = 1;
    if ((v > top) || ((bias > 0) && (v == top)))
    {
      pk = i0;
      top = v;
    }
  }
  return((any > 0) ? pk : -1);
}


//= Finding rising and falling valleys bounding peak.
// descends until value rises by more than tol from valley
// assumes values wraparound from sz-1 to 0 in array
// returns 1 if successful, 0 if one side or the other not found

int jhcArr::CycBounds (int& lo, int& hi, int pk, double tol) const
{
  int i, i0, v, val, th, up = pk + sz, dn = pk + sz;

  // find valley to right  
  hi = -1;
  val = arr[pk];
  th = ROUND(tol * val);
  for (i = pk + 1; i < up; i++)
  {
    // handle index wraparound
    i0 = i % sz;
    v = arr[i0];

    // check for rise from valley
    if ((v - val) > th)          
      break;                  

    // see if new valley found
    if (v < val)
    {
      hi = i0;
      val = v;
      th = ROUND(tol * v);
    }
  }
  if (hi < 0)
    return 0;

  // find valley to left
  lo = -1;
  val = arr[pk];
  th = ROUND(tol * val);
  for (i = dn - 1; i > pk; i--)
  {
    // handle index wraparound
    i0 = i % sz;
    v = arr[i0];

    // check for rise from valley
    if ((v - val) > th)
      break;

    // see if new valley found
    if (v < val)
    {
      lo = i0;
      val = v;
      th = ROUND(tol * v);
    }
  }
  if (lo < 0)
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Functions of two arrays                         //
///////////////////////////////////////////////////////////////////////////

//= Compares two histograms bin by bin and returns sum of absolute values.
// if array are different sizes, returns -1 to signal error

int jhcArr::SumAbsDiff (const jhcArr& ref)
{
  int i;
  int diff, sum = 0;

  if (ref.sz != sz)
    return -1;
 
  for (i = 0; i < sz; i++)
  {
    diff = (ref.arr)[i] - arr[i];
    if (diff >= 0)
      sum += diff;
    else
      sum -= diff;
  }
  return sum;
}


//= Like SumAbsDiff but bin-wise sums (a - b)^2.

int jhcArr::SumSqrDiff (const jhcArr& ref)
{
  int i;
  int diff, sum = 0;

  if (ref.sz != sz)
    return -1;
 
  for (i = 0; i < sz; i++)
  {
    diff = (ref.arr)[i] - arr[i];
    if (diff != 0)
      sum += (int)(diff * diff + 0.5);
  }
  return sum;
}


//= Copies bins in selected area (inclusive).
// if lo bigger than hi, copies strictly outside region
// can optionally scale bounds to range of 0 to vmax

void jhcArr::CopyRegion (const jhcArr& src, int lo, int hi, int vmax)
{
  int i, bot = lo, top = hi, lim = __min(sz, src.sz) - 1;

  // compute region bounds
  if (vmax != 0)
  {
    bot = (int)(bot * sz / (double) vmax);
    top = (int)(top * sz / (double) vmax);
  }
  bot = __max(0, __min(bot, lim));
  top = __max(0, __min(top, lim));

  // copy values either inside interval or outside it
  if (bot <= top)
    for (i = bot; i <= top; i++)
      arr[i] = src.arr[i];
  else
  {
    for (i = 0; i < top; i++)
      arr[i] = src.arr[i];
    for (i = bot + 1; i < lim; i++)
      arr[i] = src.arr[i];
  }
}


///////////////////////////////////////////////////////////////////////////
//                          Combination Methods                          //
///////////////////////////////////////////////////////////////////////////

//= Fills self with the sum of two arrays added together bin-wise.

int jhcArr::Sum (const jhcArr& a, const jhcArr& b, double sc)
{
  int i; 

  if (!SameSize(a) || !SameSize(b))
    return Fatal("Bad array to jhcArr::Sum");

  if (sc == 1.0)
    for (i = 0; i < sz; i++)
      arr[i] = a.ARef(i) + b.ARef(i);
  else
    for (i = 0; i < sz; i++)
      arr[i] = (int)(sc * (a.ARef(i) + b.ARef(i)) + 0.5);
  return 1;
}


//= Mixes together fractions of two other arrays into self.

int jhcArr::WtdSum (const jhcArr& a, const jhcArr& b, double asc, double bsc)
{
  int i; 

  if (!SameSize(a) || !SameSize(b))
    return Fatal("Bad array to jhcArr::WtdSum");

  if ((asc == 1.0) && (bsc == 1.0))
    for (i = 0; i < sz; i++)
      arr[i] = a.ARef(i) + b.ARef(i);
  else
    for (i = 0; i < sz; i++)
      arr[i] = (int)(asc * a.ARef(i) + bsc * b.ARef(i) + 0.5);
  return 1;
}


//= Adds scaled version of other array to self.

int jhcArr::AddWtd (const jhcArr& a, double sc)
{
  int i; 

  if (!SameSize(a))
    return Fatal("Bad array to jhcArr::AddWtd");

  if (sc == 1.0)
    for (i = 0; i < sz; i++)
      arr[i] += a.ARef(i);
  else
    for (i = 0; i < sz; i++)
      arr[i] += (int)(sc * a.ARef(i) + 0.5);
  return 1;
}


//= Fills self with bin-wise difference of two arrays.

int jhcArr::Diff (const jhcArr& a, const jhcArr& b, double sc)
{
  int i; 

  if (!SameSize(a) || !SameSize(b))
    return Fatal("Bad array to jhcArr::Diff");

  if (sc == 1.0)
    for (i = 0; i < sz; i++)
      arr[i] = a.ARef(i) - b.ARef(i);
  else
    for (i = 0; i < sz; i++)
      arr[i] = (int)(sc * (a.ARef(i) - b.ARef(i)) + 0.5);
  return 1;
}


//= Fills self with positive bin-wise difference of two arrays.

int jhcArr::ClipDiff (const jhcArr& a, const jhcArr& b, double sc)
{
  int i; 
  int v;

  if (!SameSize(a) || !SameSize(b))
    return Fatal("Bad array to jhcArr::ClipDiff");

  if (sc == 1.0)
    for (i = 0; i < sz; i++)
    {
      v = a.ARef(i) - b.ARef(i);
      arr[i] = __max(0, v);
    }
  else
    for (i = 0; i < sz; i++)
    { 
      v = (int)(sc * (a.ARef(i) - b.ARef(i)) + 0.5);
      arr[i] = __max(0, v);
    }
  return 1;
}


//= Fills self with bin-wise absolute difference of two arrays.

int jhcArr::AbsDiff (const jhcArr& a, const jhcArr& b, double sc)
{
  int i; 
  
  if (!SameSize(a) || !SameSize(b))
    return Fatal("Bad array to jhcArr::AbsDiff");

  if (sc == 1.0)
    for (i = 0; i < sz; i++)
      arr[i] = abs(a.ARef(i) - b.ARef(i));
  else
    for (i = 0; i < sz; i++)
      arr[i] = (int)(sc * abs(a.ARef(i) - b.ARef(i)) + 0.5);
  return 1;
}


//= Fills self with bin-wise squared difference of two arrays.

int jhcArr::SqrDiff (const jhcArr& a, const jhcArr& b, double sc)
{
  int i; 
  int diff;
  
  if (!SameSize(a) || !SameSize(b))
    return Fatal("Bad array to jhcArr::AbsDiff");

  if (sc == 1.0)
    for (i = 0; i < sz; i++)
    {
      diff = a.ARef(i) - b.ARef(i);
      arr[i] = diff * diff;
    }
  else
    for (i = 0; i < sz; i++)
    {
      diff = a.ARef(i) - b.ARef(i);
      arr[i] = (int)(sc * diff * diff + 0.5);
    }
  return 1;
}


//= Takes bin-wise absolute difference weighted by fractional change.
// i.e. forgives some big differences if the underlying values are big
//   v = abs(a - b) * [abs(a - b) / max(a, b)]

int jhcArr::DualDiff (const jhcArr& a, const jhcArr& b, double sc)
{
  int i; 
  int diff, big;
  
  if (!SameSize(a) || !SameSize(b))
    return Fatal("Bad arrays to jhcArr::DualDiff");

  for (i = 0; i < sz; i++)
  {
    diff = abs(a.ARef(i) - b.ARef(i));
    big = __max(a.ARef(i), b.ARef(i));
    if (diff == 0)
      arr[i] = 0;
    else 
      arr[i] = (int)(sc * diff * diff / (double) big + 0.5);
  }
  return 1;
}


//= Shifts around a few bins and takes minimum.

int jhcArr::ShiftDiff (const jhcArr& a, const jhcArr& b, int rng)
{
  int i, j, top, bot;
  int diff, best;
  
  if (!SameSize(a) || !SameSize(b))
    return Fatal("Bad arrays to jhcArr::ShiftDiff");

  for (i = 0; i < sz; i++)
  {
    top = __min(i + rng, sz - 1);
    bot = __max(0, i - rng); 
    best = abs(a.ARef(bot) - b.ARef(bot));
    for (j = bot + 1; j <= top; j++)
    {
      diff = abs(a.ARef(j) - b.ARef(j));
      best = __min(diff, best);
    }
    arr[i] = best;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Statistical Update Methods                      //
///////////////////////////////////////////////////////////////////////////

//= Mix in new array with weight wt, current contents with weight (1.0 - wt).

int jhcArr::Blend (const jhcArr& src, double wt)
{
  int i; 
  
  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::MinFcn");

  for (i = 0; i < sz; i++)
    arr[i] += (int)(wt * (src.arr[i] - arr[i]) + 0.5);
  return 1;
}


//= Bin-wise multiply current values by values in source array.

int jhcArr::Mult (const jhcArr& src)
{
  int i; 
  
  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::Mult");

  for (i = 0; i < sz; i++)
    arr[i] *= src.arr[i];
  return 1;
}


//= Take bin-wise maximum of current array or supplied one.

int jhcArr::MaxFcn (const jhcArr& src)
{
  int i; 
  
  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::MaxFcn");

  for (i = 0; i < sz; i++)
    arr[i] = __max(arr[i], src.ARef(i));
  return 1;
}


//= Take bin-wise minimum of current array or supplied one.

int jhcArr::MinFcn (const jhcArr& src)
{
  int i; 
  
  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::MinFcn");

  for (i = 0; i < sz; i++)
    arr[i] = __min(arr[i], src.ARef(i));
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Modification                             //
///////////////////////////////////////////////////////////////////////////

//= Fill self by multiplying values in given array by a constant.

int jhcArr::Scale (const jhcArr& src, double sc)
{
  int i;
  int *s = src.arr;

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::Scale");
  if (sc == 1.0)
    for (i = 0; i < sz; i++)
      arr[i] = s[i];
  else
    for (i = 0; i < sz; i++)
      arr[i] = (int)(sc * s[i] + 0.5); 
  return 1;
}


//= Like Scale but uses integer operation for better speed.
// do not use for large scaling factors or high value bins

int jhcArr::ScaleFast (const jhcArr& src, double sc)
{
  int i, isc = (int)(65536.0 * sc + 0.5);
  int *s = src.arr;

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::ScaleFast");
  for (i = 0; i < sz; i++)
    arr[i] = (isc * s[i] + 32768) >> 16;
  return 1;
}


//= Fill self with some constant minus current value.

int jhcArr::Complement (const jhcArr& src, int top)
{
  int i;
  int *s = src.arr;

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::Complement");
  for (i = 0; i < sz; i++)
    arr[i] = top - s[i];
  return 1;
}


//= Add a constant value to all bins.

int jhcArr::Offset (const jhcArr& src, int inc)
{
  int i;
  int *s = src.arr;

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::Offset");
  for (i = 0; i < sz; i++)
    arr[i] = s[i] + inc;
  return 1;
}


//= Add a constant value to all bins.
// also works if input array is smaller 

int jhcArr::OffsetN (const jhcArr& src, int inc)
{
  int i, n = __min(sz, src.sz);
  int *s = src.arr;

  for (i = 0; i < n; i++)
    arr[i] = s[i] + inc;
  return 1;
}


//= Make all values under threshold be zero.

int jhcArr::Squelch (const jhcArr& src, int sub)
{
  int i, n = __min(sz, src.sz);
  int *s = src.arr;

  for (i = 0; i < n; i++)
    if (s[i] < sub)
      arr[i] = 0;
    else
      arr[i] = s[i];
  return 1;
}


//= Expand non-zero regions several bins to the left or right.
// converts adjacent zero bins to contain val instead of zero
// left and right values can be negative to shrink instead

int jhcArr::PadNZ (const jhcArr& src, int left, int right, int val)
{
  int i;
  int *s = src.arr;
  int in, out;

  // start by copying basic contents
  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::PadNZ");
  Copy(src);

  // left to right scan
  in = 0;
  out = 0;
  for (i = 0; i < sz; i++)
  {
    // measure distance into peak or into gap
    if (s[i] == 0)
    {
      in = 0;
      out++;
    }
    else
    {
      in++;
      out = 0;
    }

    // add to right sides or subtract from left
    if ((in > 0) && (in <= -left))
      arr[i] = 0;
    if ((out > 0) && (out <= right))
      arr[i] = val;
  }

  // right to left scan
  in = 0;
  out = 0;
  for (i = sz - 1; i >= 0; i--)
  {
    // measure distance into peak or into gap
    if (s[i] == 0)
    {
      in = 0;
      out++;
    }
    else
    {
      in++;
      out = 0;
    }

    // add to right sides or subtract from left
    if ((in > 0) && (in <= -right))
      arr[i] = 0;
    if ((out > 0) && (out <= left))
      arr[i] = val;
  }
  return 1;
}


//= Divide all elements by a constant.
// if cnt is zero, values are left unchanged

int jhcArr::NormBy (const jhcArr& src, int cnt, int total)
{
  int i;
  double sc;
  int *s = src.arr;

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::NormBy");
  if ((cnt <= 0) || (total < 0))
    return 0;
  sc = total / (double) cnt;
  for (i = 0; i < sz; i++)
    arr[i] = (int)(sc * s[i] + 0.5); 
  return 1;
}


//= Like NormBy by adds up all histogram bins itself.
// makes it so "SumAll" will return "total"

int jhcArr::Normalize (int total)
{
  return NormBy(*this, SumAll(), total);
}


//= Smooth array using [0.25 0.5 0.25] mask several times.
// values in first and last bins never changed unless cyclic specified

int jhcArr::Smooth (const jhcArr &src, int passes, int cyc)
{
  int j, first, last;
  int i, lf, mid, rt, sites = sz - 2;
  int *s, *d;

  if (!SameSize(src) || (sites <= 0))
    return Fatal("Bad array to jhcArr::Smooth");
  if (passes < 0)
    return 0;
  Copy(src);

  for (j = passes; j > 0; j--)
  {
    // special first and last bins for cyclic data
    if (cyc > 0)
    {
      first = (arr[sz - 1] + (arr[0]      << 1) + arr[1] + 2) >> 2;
      last  = (arr[sz - 2] + (arr[sz - 1] << 1) + arr[0] + 2) >> 2; 
    }

    // central part of smoothing
    d = arr + 1;
    s = arr;
    lf = *s++;
    mid = *s++;
    for (i = sites; i > 0; i--)
    {
      rt = *s++;
      *d++ = (lf + (mid << 1) + rt + 2) >> 2;
      lf = mid;
      mid = rt;
    }

    // copy precomputed cyclic values
    if (cyc > 0)
    {
      arr[0]      = first;
      arr[sz - 1] = last;
    }
  }
  return 1;
}


//= Replace each bin by an average of some region around bin.
// Wraps around if cyc is non-zero, else duplicates value at ends
// <pre>
// example:    sc = 5, n = 2, p = 3, sz = 10
//
//        *   *   *   *   *                               0   1   2
//   -3  -2  -1  [0   1   2   3   4   5   6   7   8   9] 10  11  12  
//    7   8   9                               *   *   *   *   *
// </pre>

int jhcArr::Boxcar (const jhcArr& src, int sc, int cyc)
{
  int i, j, n = sc / 2, p = sc - n, sum = 0;  

  if (SameArr(src) || !SameSize(src) || (sc <= 0) || (sc > sz))
    return Fatal("Bad array to jhcArr::Boxcar");
  if (sc == 1)
  {
    Copy(src);
    return 1;
  }

  // figure out initial value (use end of array if cyclic)
  for (i = -n; i < p; i++)
  {
    j = i;
    if (i < 0)
    {
      if (cyc <= 0)
        j = 0;
      else
        j = sz + i;
    }
    sum += src.arr[j];
  }
  arr[0] = sum / sc;

  // progressively form all other sums
  for (i = 1; i < sz; i++)
  {
    // subtract trailing edge
    j = i - n - 1;
    if (j < 0)
    {
      if (cyc <= 0)
        j = 0;
      else
        j += sz;
    } 
    sum -= src.arr[j];

    // add in leading edge
    j = i + p - 1;
    if (j >= sz)
    {
      if (cyc <= 0)
        j = sz - 1;
      else
        j -= sz;
    }
    sum += src.arr[j];
    arr[i] = sum / sc;
  }
  return 1;
}


//= Replace each bin by an average of values above zero around bin.
// Wraps around if cyc is non-zero, else duplicates value at ends

int jhcArr::BoxcarNZ (const jhcArr& src, int sc, int cyc)
{
  int i, j, n = sc / 2, p = sc - n, sum = 0, cnt = 0;  

  if (SameArr(src) || !SameSize(src) || (sc <= 0) || (sc > sz))
    return Fatal("Bad array to jhcArr::BoxcarNZ");
  if (sc == 1)
  {
    Copy(src);
    return 1;
  }
  Fill(0);

  // figure out initial value (use end of array if cyclic)
  for (i = -n; i < p; i++)
  {
    j = i;
    if (i < 0)
    {
      if (cyc <= 0)
        j = 0;
      else
        j = sz + i;
    }
    if (src.arr[j] > 0)
    {
      sum += src.arr[j];
      cnt++;
    }
  }
  if (cnt > 0)
    arr[0] = ROUND(sum / (double) cnt);

  // progressively form all other sums
  for (i = 1; i < sz; i++)
  {
    // subtract trailing edge
    j = i - n - 1;
    if (j < 0)
    {
      if (cyc <= 0)
        j = 0;
      else
        j += sz;
    } 
    if (src.arr[j] > 0)
    {
      sum -= src.arr[j];
      cnt--;
    }

    // add in leading edge
    j = i + p - 1;
    if (j >= sz)
    {
      if (cyc <= 0)
        j = sz - 1;
      else
        j -= sz;
    }
    if (src.arr[j] > 0)
    {
      sum += src.arr[j];
      cnt++;
    }

    // record average
    if (cnt > 0)
      arr[i] = ROUND(sum / (double) cnt);
  }
  return 1;
}


//= Linearly interpolate between valid points.
// invalid points are identified using the value "bad"

int jhcArr::Interpolate (const jhcArr& src, int bad)
{
  int i, j, any = 0, start = 0;
  double inc, sum;

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::Interpolate");
  Copy(src);

  // look for good values to anchor interpolation
  for (i = 0; i < sz; i++)
    if (arr[i] != bad)
    {
      // possibly fill gap
      if ((any > 0) && (start < (i - 1)))
      {
        inc = (arr[i] - arr[start]) / (double)(i - start);
        sum = arr[start] + inc;
        for (j = start + 1; j < i; j++, sum += inc)
          arr[j] = ROUND(sum);
      }

      // set up for possible gap next
      start = i;
      any = 1;
    }
  return 1;
}


//= Adjusts the bin size in a histogram by reapportioning counts.
// f is new width of bin (e.g. 1.7 causes histogram to shrink)

int jhcArr::BinScale (const jhcArr& src, double f)
{
  int sum, i, j = 0;
  double top = 0.0;

  if (&src == this)
    return Fatal("Bad array to jhcArr::BinScale");

  for (i = 0; i < sz; i++)
  {
    // add fraction of previous bin
    sum = 0;
    if ((i > 0) && (j < src.sz))
      sum += (int)((j + 1.0 - top) * (src.arr)[j] + 0.5); 
    top += f;

    // add an integral number of bin counts
    while (j < (int)(top))
    {
      if (j < src.sz)
        sum += (src.arr)[j];
      j++;
    }

    // add fraction of last bin
    if (j < src.sz)
      sum += (int)((top - j) * (src.arr)[j] + 0.5);
    arr[i] = sum;
  }
  return 1;
}


//= Set value to 1 if strictly over threshold, else zero.

int jhcArr::Thresh (const jhcArr& src, int th, int val)
{
  int i;

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::Thresh");

  for (i = 0; i < sz; i++)
    if ((src.arr)[i] > th)
      arr[i] = val;
    else
      arr[i] = 0;
  return 1;
}


//= Set value to 1 if strictly over threshold, else zero.
// also works if input array is smaller 

void jhcArr::ThreshN (const jhcArr& src, int th, int val)
{
  int i, n = __min(sz, src.sz);

  for (i = 0; i < n; i++)
    if ((src.arr)[i] > th)
      arr[i] = val;
    else
      arr[i] = 0;
}


//= Forces values strictly above threshold to over, others become under.

int jhcArr::OverUnder (const jhcArr& src, int th, int over, int under)
{
  int i;

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::OverUnder");

  for (i = 0; i < sz; i++)
    if ((src.arr)[i] > th)
      arr[i] = over;
    else
      arr[i] = under;
  return 1;
}


//= Makes maximum value be big and minimum value be sm.

int jhcArr::DualClamp (const jhcArr& src, int big, int sm)
{
  int i;

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::DualClamp");

  for (i = 0; i < sz; i++)
    arr[i] = __max(sm, __min((src.arr)[i], big));
  return 1;
}


//= Extracts bits specified in mask then downshift by some amount.

int jhcArr::BitField (const jhcArr& src, int mask, int shift)
{
  int i, sh = __max(0, shift);

  if (!SameSize(src))
    return Fatal("Bad array to jhcArr::BitFields");
  for (i = 0; i < sz; i++)
    arr[i] = ((src.arr)[i] & mask) >> sh;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            File Operations                            //
///////////////////////////////////////////////////////////////////////////

//= Read size and values from an ascii file.
// first line has tag "jhc " followed by number of samples (in ASCII)
// rest is concatenated values (in ASCII) generally separated by spaces
// for binary (all 1's and 0's) the tag is "JHC " and there are no spaces

int jhcArr::Read (const char *fname)
{
  FILE *in;
  char hdr[40];
  char c;
  int bin, v, i = 0, n = 1;

  // try to open file
  if (fopen_s(&in, fname, "r") != 0)
    return 0;

  while (1)
  {
    // check header for format and array size
    if (fgets(hdr, 40, in) == NULL)
      break;
    if (strlen(hdr) < 6)
      break;
    if (strncmp(hdr, "JHC ", 4) == 0)
      bin = 1;
    else if (strncmp(hdr, "jhc ", 4) == 0)
      bin = 0;
    else
      break;
    if (sscanf_s(hdr + 4, "%d", &n) != 1)
      break;
    SetSize(n);

    // read bulk of values as binary
    if (bin > 0)
    {
      Fill(0);
      for (i = 0; i < n; i++)
      {
        c = (char) fgetc(in);
        if (feof(in) || ferror(in))
          break;
        if (c != '0')
          arr[i] = 1;
      }
      break;
    }
    
    // read bulk of values as integers
    for (i = 0; i < n; i++)
      if (fscanf_s(in, "%d", &v) != 1)
        break;
      else
        arr[i] = v;
    break;
  }
  
  // clean up
  fclose(in);
  if (i < n)
    return 0;
  return 1;
}


//= Write out array size and values to a file.
// for binary arrays header is "JHC " and size followed by 1's and 0's
// for normal arrays header is "jhc " and size followed by values with spaces

int jhcArr::Write (const char *fname, int bin) const
{
  FILE *out;
  int i = 0;

  if (fopen_s(&out, fname, "w") != 0)
    return 0;

  if (bin > 0)
  {
    // write binary format
    if (fprintf(out, "JHC %d\n", sz) >= 0)
      for (i = 0; i < sz; i++)
        if (fputc(((arr[i] <= 0) ? '0' : '1'), out) == EOF)
          break;
  }
  else
  {     
    // write full format
    if (fprintf(out, "jhc %d\n", sz) >= 0)
      for (i = 0; i < sz; i++)
        if (fprintf(out, "%d ", arr[i]) < 0) 
          break;
  }

  // clean up 
  fclose(out);
  if (i < sz)
    return 0;
  return 1;  
}
