// jhcArea.cpp : computes properties of pixel regions
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

#include <math.h>
#include <basetsd.h>                 // for __int64 type
#include "Interface/jhcMessage.h"

#include "Processing/jhcArea.h"

// Note: many functions migrated to jhcRuns and jhcDist


///////////////////////////////////////////////////////////////////////////
//                         Simple Dispatch Forms                         //
///////////////////////////////////////////////////////////////////////////

//= Return local averages in box or diamond areas around each pixel.
// adjusts diagonal dimensions to give same box size

int jhcArea::BoxAvgX (jhcImg& dest, const jhcImg& src, int w1, int h2, 
                      double sc, int diag)
{
  int wd, hd = 0;

  if (diag <= 0)
    return BoxAvg(dest, src, w1, h2, sc);
  wd = 2 * ROUND(0.5 * 0.7071 * (w1 - 1)) + 1;
  if (h2 > 0)
    hd = 2 * ROUND(0.5 * 0.7071 * (h2 - 1)) + 1;
  return DBoxAvg(dest, src, wd, hd, sc);
}


//= Return average and standard deviation in boxes or diamonds around PxlSrc().
// adjusts diagonal dimensions to give same box size

int jhcArea::BoxAvgStdX (jhcImg& avg, jhcImg& std, const jhcImg& src, 
                         int w1, int h2, double dsc, int diag)
{
  int wd, hd = 0;

  if (diag <= 0)
    return BoxAvgStd(avg, std, src, w1, h2, dsc);
  wd = 2 * ROUND(0.5 * 0.7071 * (w1 - 1)) + 1;
  if (h2 > 0)
    hd = 2 * ROUND(0.5 * 0.7071 * (h2 - 1)) + 1;
  return DBoxAvgStd(avg, std, src, wd, hd, dsc);
}


//= Like BoxAvgStdX but returns inverse standard deviation (256 / sd).
// inverse standard deviations are clipped at 255 (e.g. for zero and one)
// adjusts diagonal dimensions to give same box size

int jhcArea::BoxAvgInvX (jhcImg& avg, jhcImg& isd, const jhcImg& src, 
                         int w1, int h2, double dsc, int diag)
{
  int wd, hd = 0;

  if (diag <= 0)
    return BoxAvgInv(avg, isd, src, w1, h2, dsc);
  wd = 2 * ROUND(0.5 * 0.7071 * (w1 - 1)) + 1;
  if (h2 > 0)
    hd = 2 * ROUND(0.5 * 0.7071 * (h2 - 1)) + 1;
  return DBoxAvgInv(avg, isd, src, wd, hd, dsc);
}


//= Returns averages of 16 bit values in box or diamond around each pixel.
// adjusts diagonal dimensions to give same box size

int jhcArea::BoxAvg16X (jhcImg& dest, const jhcImg& src, int w1, int h2, 
                        double sc, int diag)
{
  int wd, hd = 0;

  if (diag <= 0)
    return BoxAvg16(dest, src, w1, h2, sc);
  wd = 2 * ROUND(0.5 * 0.7071 * (w1 - 1)) + 1;
  if (h2 > 0)
    hd = 2 * ROUND(0.5 * 0.7071 * (h2 - 1)) + 1;
  return DBoxAvg16(dest, src, wd, hd, sc);
}


///////////////////////////////////////////////////////////////////////////
//                          Local Area Averages                          //
///////////////////////////////////////////////////////////////////////////

//= Replace each pixel by average of rectangular box around it.
// if ht = 0 height is reset to match width
// can multiply average by some scale factor and truncate result
// fast for masks less than 16 x 16, calls slower version for big masks
//
// NOTE: to get same result in ROI as would be obtained using the
// whole image, the ROI must be expanded by half the box size to
// get boundaries correct, e.g. GrowRoi(wid/2, ht/2), and the 
// input data must be valid over this expanded area
//
// about 0.5ms on 320x240 with 3.2GHz Xeon

int jhcArea::BoxAvg (jhcImg &dest, const jhcImg& src, int wid, int ht, 
                     double sc, jhcImg *vic)
{
  int dx = wid, dy = ((ht == 0) ? wid : ht), area = dx * dy;

  // check arguments
  if (dest.Valid(2))
    return BoxAvg16(dest, src, wid, ht, sc);
  if ((!dest.Valid(1) && !dest.Valid(3)) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::BoxAvg");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxAvg", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0) || (sc <= 0.0))
    return 0;
  if ((area == 1) && (sc == 1.0))
    return dest.CopyArr(src);
  if (dest.Valid(3))
    return BoxAvgRGB(dest, src, wid, ht, sc);
  if ((dx == 3) && (dy == 3) && !dest.SameImg(src))
    return BoxAvg3(dest, src, sc);
  dest.CopyRoi(src);
  if ((area * sc) >= 66051.0)
    return box_avg0(dest, src, dx, dy, sc, vic);

  // generic ROI case
  int i, x, y, wx, rsk4;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int nx = dx / 2, px = dx - nx, xlim = rw - px - 1;
  int ny = dy / 2, py = dy - ny, ylim = rh - py, nyp = ny + 1;
  UL32 norm = (UL32)(65536.0 * sc / area);
  UL32 sum, ej, roff4, line4 = dest.XDim();
  const UC8 *a0, *ahi, *alo, *aej;
  UC8 *d;
  UL32 *b, *b0, *blo, *bhi, *bsum;
  jhcImg *temp = vic;
  UL32 div[256];

  // use provided scratch array or make up new one  
  if ((vic == NULL) || !dest.SameSize(*vic, 4))
    temp = a4.SetSize(dest, 4);

  // set up pointer to start of region of interest in victim
  dest.RoiParams(&roff4, &rsk4, 4);
  rsk4 >>= 2;
  b0 = (UL32 *)(temp->PxlDest() + roff4);

  // precomputed "normalized" values for all intensities
  // such that final division by mask area is just a shift
  for (i = 0; i <= 255; i++)
    div[i] = norm * i;

  // PASS 1 vertical ==============================================
  // vertically smear from source into temp using running sum
  // keep rw running sums at the same time to preserve scan order
  a0 = src.PxlSrc() + src.RoiOff();

  // initialize running sums (in bsum), one per column in ROI
  // duplicate values at edge to handle mask overhang
  // this is exactly the answer for top row of PxlSrc()
  bsum = b0;
  ahi  = a0;
  for (x = rw; x > 0; x--)
    *bsum++ = nyp * div[*ahi++];
  ahi += rsk;
  for (y = 1; y < py; y++)
  {
    bsum = b0;
    for (x = rw; x > 0; x--, bsum++, ahi++)
      *bsum += div[*ahi];
    ahi += rsk;
  }

  // mask falls off top initially
  // take previous running sum (row above) and adjust by adding
  // source pixel from lower line and subtracting source pixel at top
  // nyp incremented since sum is updated BEFORE writing out, not after
  nyp++;
  bsum = b0;
  b    = b0 + line4;
  for (y = 1; y < nyp; y++)
  {
    alo = a0;
    for (x = rw; x > 0; x--)
      *b++ = *bsum++ + div[*ahi++] - div[*alo++];
    b    += rsk4;
    bsum += rsk4;
    ahi  += rsk;
  }
  alo += rsk;

  // mask is fully inside for most
  // get previous running sum, add bottom, subtract top
  for (y = nyp; y < ylim; y++)
  {
    for (x = rw; x > 0; x--)
      *b++ = *bsum++ + div[*ahi++] - div[*alo++];
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  += rsk;
  }

  // mask falls off bottom at end
  // get previous running sum, add edge, subtract top
  // for first center the add is legimately the last pixel
  aej = ahi;
  for (y = ylim; y < rh; y++)
  {
    for (x = rw; x > 0; x--)
      *b++ = *bsum++ + div[*ahi++] - div[*alo++];
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  =  aej;
  }

  // PASS 2 horizontal =========================================
  // horizontal smear from temp into local using running sum
  // adds in new right side, subtracts off old left side
  b = b0;
  d = dest.PxlDest() + dest.RoiOff();
  for (y = rh; y > 0; y--)
  {
    // initialize running sum 
    sum = 0;
    for (x = 0; x < dx; x++)
    {
      wx = x - nx;
      sum += *(b + __max(0, wx));
    }

    // mask falls off left initially
    ej = *b;
    bhi = b + px;
    for (x = 0; x < nx; x++)
    {
      if ((sum >> 24) != 0)
        *d++ = 255;
      else
        *d++ = (UC8)(sum >> 16);
      sum -= ej;
      sum += *bhi++;
    }

    // mask is fully inside for most
    blo = b;
    for (x = nx; x < xlim; x++)
    {
      if ((sum >> 24) != 0)
        *d++ = 255;
      else
        *d++ = (UC8)(sum >> 16);
      sum -= *blo++;
      sum += *bhi++;
    }

    // mask falls off right at end
    ej = *bhi;
    for (x = xlim; x < rw; x++)
    {
      if ((sum >> 24) != 0)
        *d++ = 255;
      else
        *d++ = (UC8)(sum >> 16);
      sum -= *blo++;
      sum += ej;
    }

    // move down to next line
    d += rsk;
    b += line4;
  }
  return 1;
}


//= Like BoxAvg but does normalization with multiply.
// works for larger mask sizes than base version
// about 1.4x slower than small mask version

int jhcArea::box_avg0 (jhcImg &dest, const jhcImg& src, int dx, int dy,
                      double sc, jhcImg *vic)
{
  // generic ROI case
  int x, y, wx, rsk4;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int nx = dx / 2, px = dx - nx, xlim = rw - px - 1;
  int ny = dy / 2, py = dy - ny, ylim = rh - py, nyp = ny + 1;
  unsigned __int64 norm = (unsigned __int64)(0x01000000 * sc / (dx * dy));
  UL32 sum, ej, val, roff4, line4 = dest.XDim();
  const UC8 *a0, *ahi, *alo, *aej;
  UC8 *d;
  UL32 *b, *b0, *blo, *bhi, *bsum;
  jhcImg *temp = vic;

  // use provided scratch array or make up new one  
  if ((vic == NULL) || !dest.SameSize(*vic, 4))
    temp = a4.SetSize(dest, 4);

  // set up pointer to start of region of interest in victim
  dest.RoiParams(&roff4, &rsk4, 4);
  rsk4 >>= 2;
  b0 = (UL32 *)(temp->PxlDest() + roff4);


  // PASS 1 vertical ==============================================
  // vertically smear from source into temp using running sum
  // keep rw running sums at the same time to preserve scan order
  a0 = src.PxlSrc() + src.RoiOff();

  // initialize running sums (in bsum), one per column in ROI
  // duplicate values at edge to handle mask overhang
  // this is exactly the answer for top row of PxlSrc()
  bsum = b0;
  ahi  = a0;
  for (x = rw; x > 0; x--)
    *bsum++ = nyp * (*ahi++);
  ahi += rsk;
  for (y = 1; y < py; y++)
  {
    bsum =  b0;
    for (x = rw; x > 0; x--)
      *bsum++ += *ahi++;
    ahi += rsk;
  }

  // mask falls off top initially
  // take previous running sum (row above) and adjust by adding
  // source pixel from lower line and subtracting source pixel at top
  // nyp incremented since sum is updated BEFORE writing out, not after
  nyp++;
  bsum = b0;
  b    = b0 + line4;
  for (y = 1; y < nyp; y++)
  {
    alo = a0;
    for (x = rw; x > 0; x--)
      *b++ = *bsum++ + *ahi++ - *alo++;
    b    += rsk4;
    bsum += rsk4;
    ahi  += rsk;
  }
  alo += rsk;

  // mask is fully inside for most
  // get previous running sum, add bottom, subtract top
  for (y = nyp; y < ylim; y++)
  {
    for (x = rw; x > 0; x--)
      *b++ = *bsum++ + *ahi++ - *alo++;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  += rsk;
  }

  // mask falls off bottom at end
  // get previous running sum, add edge, subtract top
  // for first center the add is legimately the last pixel
  aej = ahi;
  for (y = ylim; y < rh; y++)
  {
    for (x = rw; x > 0; x--)
      *b++ = *bsum++ + *ahi++ - *alo++;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  =  aej;
  }


  // PASS 2 horizontal =========================================
  // horizontal smear from temp into local using running sum
  // adds in new right side, subtracts off old left side
  b = b0;
  d = dest.PxlDest() + dest.RoiOff();
  for (y = rh; y > 0; y--)
  {
    // initialize running sum 
    sum = 0;
    for (x = 0; x < dx; x++)
    {
      wx = x - nx;
      sum += *(b + __max(0, wx));
    }

    // mask falls off left initially
    ej = *b;
    bhi = b + px;
    for (x = 0; x < nx; x++)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(255, val);
      *d++ = (UC8) val;
      sum -= ej;
      sum += *bhi++;
    }

    // mask is fully inside for most
    blo = b;
    for (x = nx; x < xlim; x++)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(255, val);
      *d++ = (UC8) val;
      sum -= *blo++;
      sum += *bhi++;
    }

    // mask falls off right at end
    ej = *bhi;
    for (x = xlim; x < rw; x++)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(255, val);
      *d++ = (UC8) val;
      sum -= *blo++;
      sum += ej;
    }

    // move down to next line
    d += rsk;
    b += line4;
  }
  return 1;
}


//= Do box averaging where size is a fraction of array width.
// Height also as fraction of array width but then rescaled by aspect
// if ht = 0.0 height is reset to match width (then aspect corrected)

int jhcArea::BoxAvg (jhcImg& dest, const jhcImg& src, double wf, double hf, 
                     double sc, jhcImg *vic)
{
  double wbox = wf * dest.XDim(), hbox = hf * dest.XDim();

  if (hf == 0.0)
    hbox = wbox;
  return(BoxAvg(dest, src, (int) wbox, (int)(hbox / dest.Ratio()), sc, vic));
}


//= Applies smoothing to all three color planes.

int jhcArea::BoxAvgRGB (jhcImg& dest, const jhcImg& src, 
                        int wid, int ht, double sc)
                     
{
  if (!dest.Valid(3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::BoxAvgRGB");
  if ((wid <= 0) || (ht < 0) || (sc <= 0.0))
    return 0;

  // make local array for field
  a1.SetSize(src, 1);
  
  // do red 
  a1.CopyField(src, 2, 0);
  BoxAvg(a1, a1, wid, ht);
  dest.CopyField(a1, 0, 2);

  // do green
  a1.CopyField(src, 1, 0);
  BoxAvg(a1, a1, wid, ht);
  dest.CopyField(a1, 0, 1);

  // do blue
  a1.CopyField(src, 0, 0);
  BoxAvg(a1, a1, wid, ht);
  dest.CopyField(a1, 0, 0);
  return 1;
}


//= Common morphology-like operation.
// might be 15% faster if integrated directly into BoxAvg core

int jhcArea::BoxThresh (jhcImg &dest, const jhcImg& src, int sc, 
                        int th, int over, int under)
{
  int ans;
  
  if ((sc == 3) && !dest.SameImg(src))
    return BoxThresh3(dest, src, th, over, under);
  ans = BoxAvg(dest, src, sc, sc, 1.0, NULL);
  if (ans < 1)
    return ans;
  thresh(dest, dest, th, over, under);
  return 1;
}

 
//= Take local 3x3 pixel averages around each pixel in source.
// 1.5x faster than standard BoxAvg

int jhcArea::BoxAvg3 (jhcImg& dest, const jhcImg& src, double sc)
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src)
      || (src.RoiW() <= 3) || (src.RoiH() <= 3))
    return Fatal("Bad images to jhcArea::BoxAvg3");
  dest.CopyRoi(src);

  // general ROI case
  int step = ROUND(sc * 65536.0 * 4.0 / 9.0), val = step >> 1;
  int x, y, sum, rw = dest.RoiW(), rh = dest.RoiH();
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = dsk + 1;
  UC8 *d = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b, *a = s + ln;
  UC8 ninth[575];

  // build scaling table
  for (sum = 0; sum < 575; sum++, val += step)
    ninth[sum] = (UC8)(val >> 16);

  // special computation for bottom line of image
  sum = (s[0] << 2) + ((s[1] + a[0]) << 1) + a[1];
  *d++ = ninth[sum >> 2];
  for (x = rw - 2; x > 0; x--, d++, s++, a++)
  {
    sum = ((s[0] + s[1] + s[2]) << 1) + a[0] + a[1] + a[2];
    *d = ninth[sum >> 2];
  }
  sum = (s[1] << 2) + ((a[1] + s[0]) << 1) + a[0];
  *d = ninth[sum >> 2];
  d += dsk;
  s += ssk;
  a += ssk;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, b += ssk, s += ssk, a += ssk) 
  {
    sum = ((b[0] + s[0] + a[0]) << 1) + b[1] + s[1] + a[1];
    *d++ = ninth[sum >> 2];
    for (x = rw - 2; x > 0; x--, d++, b++, s++, a++)
    {
      sum = b[0] + b[1] + b[2] + s[0] + s[1] + s[2] + a[0] + a[1] + a[2];
      *d = ninth[sum >> 2];
    }
    sum = b[0] + s[0] + a[0] + ((b[1] + s[1] + a[1]) << 1);
    *d = ninth[sum >> 2];
  }

  // special computation for top line of image
  sum = (s[0] << 2) + ((s[1] + b[0]) << 1) + b[1];
  *d++ = ninth[sum >> 2];
  for (x = rw - 2; x > 0; x--, d++, b++, s++)
  {
    sum = b[0] + b[1] + b[2] + ((s[0] + s[1] + s[2]) << 1);
    *d = ninth[sum >> 2];
  }
  sum = (s[1] << 2) + ((s[0] + b[1]) << 1) + b[0];
  *d = ninth[sum >> 2];
  return 1;
}

                
//= Take local 3x3 pixel average and mark responses above threshold.
// 1.4x faster than standard BoxThresh

int jhcArea::BoxThresh3 (jhcImg& dest, const jhcImg& src, int th, int over, int under)
{
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src) 
      || (src.RoiW() <= 3) || (src.RoiH() <= 3))
    return Fatal("Bad images to jhcArea::BoxThresh3");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, sum, th9 = 9 * th, rw = dest.RoiW(), rh = dest.RoiH();
  int ln = src.Line(), dsk = dest.RoiSkip() + 1, ssk = dsk + 1;
  UC8 *d = dest.RoiDest();
  const UC8 *b = src.RoiSrc(), *s = b, *a = s + ln;

  // special computation for bottom line of image
  sum = (s[0] << 2) + ((s[1] + a[0]) << 1) + a[1];
  if (sum <= th9)
    *d++ = (UC8) under;
  else
    *d++ = (UC8) over;
  for (x = rw - 2; x > 0; x--, d++, s++, a++)
  {
    sum = ((s[0] + s[1] + s[2]) << 1) + a[0] + a[1] + a[2];
    if (sum <= th9)
      *d = (UC8) under;
    else
      *d = (UC8) over;
  }
  sum = (s[1] << 2) + ((a[1] + s[0]) << 1) + a[0];
  if (sum <= th9)
    *d = (UC8) under;
  else
    *d = (UC8) over;
  d += dsk;
  s += ssk;
  a += ssk;

  // compute in interior (except for left and right border)
  for (y = rh - 2; y > 0; y--, d += dsk, b += ssk, s += ssk, a += ssk) 
  {
    sum = ((b[0] + s[0] + a[0]) << 1) + b[1] + s[1] + a[1];
    if (sum <= th9)
      *d++ = (UC8) under;
    else
      *d++ = (UC8) over;
    for (x = rw - 2; x > 0; x--, d++, b++, s++, a++)
    {
      sum = b[0] + b[1] + b[2] + s[0] + s[1] + s[2] + a[0] + a[1] + a[2];
      if (sum <= th9)
        *d = (UC8) under;
      else
        *d = (UC8) over;
    }
    sum = b[0] + s[0] + a[0] + ((b[1] + s[1] + a[1]) << 1);
    if (sum <= th9)
      *d = (UC8) under;
    else
      *d = (UC8) over;
  }

  // special computation for top line of image
  sum = (s[0] << 2) + ((s[1] + b[0]) << 1) + b[1];
  if (sum <= th9)
    *d++ = (UC8) under;
  else
    *d++ = (UC8) over;
  for (x = rw - 2; x > 0; x--, d++, b++, s++)
  {
    sum = b[0] + b[1] + b[2] + ((s[0] + s[1] + s[2]) << 1);
    if (sum <= th9)
      *d = (UC8) under;
    else
      *d = (UC8) over;
  }
  sum = (s[1] << 2) + ((s[0] + b[1]) << 1) + b[0];
  if (sum <= th9)
    *d = (UC8) under;
  else
    *d = (UC8) over;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Gated Area Averages                          //
///////////////////////////////////////////////////////////////////////////

//= Averages PxlSrc() in a box only where "gate" is at or above threshold.
// if ht = 0 height is reset to match width
// about 2.6x slower than best version of "BoxAvg" (1.8x slower than generic)
//   30% of the time is required for the division step in the output

int jhcArea::MaskBoxAvg (jhcImg &dest, const jhcImg& src, const jhcImg& gate, int th, 
                         int wid, int ht, jhcImg *vic, jhcImg *vic2)
{
  int ans, dx = wid, dy = ((ht == 0) ? wid : ht);

  // chack arguments
  if (!dest.Valid(1) || !dest.SameFormat(src) || !dest.SameFormat(gate))
    return Fatal("Bad images to jhcArea::MaskBoxAvg");
  dest.CopyRoi(src);  
  dest.MergeRoi(gate);
  if ((dx > dest.RoiW()) || (dy > dest.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::MaskBoxAvg", 
                 dx, dy, dest.RoiW(), dest.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0))
    return 0;
  if ((dx == 1) && (dy == 1) && (th <= 0))
  {
    ans = dest.CopyArr(src);
    dest.MergeRoi(gate);
    return ans;
  }

  // generic ROI case
  int x, y, rsk4;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int nx = dx / 2, px = dx - nx, xlim = rw - px;
  int ny = dy / 2, py = dy - ny, ylim = rh - py + 1, nyp = ny + 1;
  UC8 thv = BOUND(th);
  UL32 roff = dest.RoiOff();
  UL32 sum, cnt, roff4, line4 = dest.XDim();
  UC8 *a;
  const UC8 *a0, *ahi, *alo;
  const UC8 *g0, *ghi, *glo;
  UL32 *b, *b0, *blo, *bhi, *bsum;
  UL32 *c, *c0, *clo, *chi, *csum;
  jhcImg *temp = vic, *num = vic2;

  // use provided scratch array or make up new one  
  if ((vic == NULL) || !dest.SameSize(*vic, 4))
    temp = a4.SetSize(dest, 4);
  if ((vic2 == NULL) || !dest.SameSize(*vic2, 4))
    num = b4.SetSize(dest, 4);

  // set up pointer to start of region of interest in victim
  dest.RoiParams(&roff4, &rsk4, 4);
  rsk4 >>= 2;
  b0 = (UL32 *)(temp->PxlDest() + roff4);
  c0 = (UL32 *)(num->PxlDest()  + roff4);


  // PASS 1 vertical ===============================================
  // vertically smear from source into temp using running sum
  // keep rw running sums at the same time to preserve scan order
  a0 = src.PxlSrc() + roff;
  g0 = gate.PxlSrc() + roff;

  // initialize running sums (in bsum), one per column in ROI and
  //   also initialize count of PxlSrc() exceeding threshold (in csum)
  //   i.e. in each column sum up PxlSrc() within mask half-height of top
  // this computes exactly the answer for the top row of PxlSrc()
  // ahi and ghi point to start of line "py" at end of this stage
  bsum = b0;
  csum = c0;
  for (x = rw; x > 0; x--)         // zero the first row of sums
  {
    *bsum++ = 0;
    *csum++ = 0;
  }
  ahi  = a0;
  ghi  = g0;
  for (y = 0; y < py; y++)         // add non-masked PxlSrc() to sums
  {
    bsum = b0;
    csum = c0;
    for (x = rw; x > 0; x--)
    {
      if (*ghi++ > thv)
      {
        *csum += 1;
        *bsum += *ahi;
      }
      ahi++;
      bsum++;
      csum++;
    }
    ahi += rsk;
    ghi += rsk;
  }                                

  // mask falls off top initially
  // take previous running sum (row above) and adjust by adding
  //   source pixel from lower line and subtracting source pixel at top
  // nyp not incremented since edge values are not repeated in sum
  // ahi and ghi point to start of line "dy" at end of this stage
  bsum = b0;                      // points to beginning of first line
  csum = c0;
  b    = b0 + line4;              // points to beginning of second line
  c    = c0 + line4;
  for (y = 1; y < nyp; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b = *bsum++;               // init sum and count to vals from above
      *c = *csum++;
      if (*ghi++ > thv)           // add in head if above thresh
      {
        *c += 1;
        *b += *ahi;
      }
      ahi++;
      b++;
      c++;
    }
    b    += rsk4;                 // advance output and hi pointers
    c    += rsk4;
    bsum += rsk4;
    csum += rsk4;
    ahi  += rsk;
    ghi  += rsk;
  }

  // mask is fully inside for most
  // get previous running sum, add bottom, and subtract top
  // alo and glo point to start of line "ylim - nyp" at end of this stage
  alo = a0;
  glo = g0;
  for (y = nyp; y < ylim; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b = *bsum++;               // init sum and count to vals from above
      *c = *csum++;
      if (*glo++ > thv)           // subtract off tail if above thresh
      {
        *c -= 1;
        *b -= *alo;
      }
      alo++;
      if (*ghi++ > thv)           // add in head if above thresh
      {
        *c += 1;
        *b += *ahi;
      }
      ahi++;
      b++;
      c++;
    }
    b    += rsk4;                 // advance output, lo, and hi pointers
    c    += rsk4;
    bsum += rsk4;
    csum += rsk4;
    alo  += rsk;
    glo  += rsk;
    ahi  += rsk;
    ghi  += rsk;
  }

  // mask falls off bottom at end
  // get previous running sum and subtract top
  for (y = ylim; y < rh; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b = *bsum++;               // init sum and count to vals from above
      *c = *csum++;
      if (*glo++ > thv)           // subtract off tail if above thresh
      {
        *c -= 1;
        *b -= *alo;
      }
      alo++;
      b++;
      c++;
    }
    b    += rsk4;                 // advance output and lo pointers
    c    += rsk4;
    bsum += rsk4;
    csum += rsk4;
    alo  += rsk;
    glo  += rsk;
  }


  // PASS 2 horizontal ========================================
  // horizontal smear from temp into local using running sum
  // adds in new right side, subtracts off old left side
  // no inter need to test gate image since this info has already been 
  //   incorporated into the two temporaries: b (sum) and c (count)
  b = b0;
  c = c0;
  a = dest.PxlDest() + roff;
  for (y = rh; y > 0; y--)
  {
    // initialize running sum
    // bhi and chi point to pixel px in line afterwards
    bhi = b;
    chi = c;
    sum = 0;
    cnt = 0;
    for (x = 0; x < px; x++)
    {
      sum += *bhi++;
      cnt += *chi++;
    }

    // mask falls off left initially
    // bhi and chi point to pixel dx in line afterwards
    for (x = 0; x < nx; x++)
    {
      if (cnt <= 1) 
        *a++ = (UC8) sum;
      else
        *a++ = (UC8)(sum / cnt);
      sum += *bhi++;
      cnt += *chi++;
    }

    // mask is fully inside for most
    blo = b;
    clo = c;
    for (x = nx; x < xlim; x++)
    {
      if (cnt <= 1)
        *a++ = (UC8) sum;
      else 
        *a++ = (UC8)(sum / cnt);
      sum -= *blo++;
      sum += *bhi++;
      cnt -= *clo++;
      cnt += *chi++;
    }

    // mask falls off right at end
    for (x = xlim; x < rw; x++)
    {
      if (cnt <= 1)
        *a++ = (UC8) sum;
      else
        *a++ = (UC8)(sum / cnt);
      sum -= *blo++;
      cnt -= *clo++;
    }

    // move down to next line
    a += rsk;
    b += line4;
    c += line4;
  }
  return 1;
}


//= Do gated box averaging where size is a fraction of array width.
// Height also as fraction of array width but then rescaled by aspect
// if ht = 0.0 height is reset to match width (then aspect corrected)

int jhcArea::MaskBoxAvg (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, 
                         double wf, double hf, jhcImg *vic, jhcImg *vic2)
{
  double wbox = wf * dest.XDim(), hbox = hf * dest.XDim();

  if (hf == 0.0)
    hbox = wbox;
  return(MaskBoxAvg(dest, src, gate, th, 
                    (int) wbox, (int)(hbox / dest.Ratio()), vic, vic2));
}


//= Like NZBoxAvg but ignores pixel equal to "bg" instead of zero.
// Averages PxlSrc() in a box only where source PxlSrc() are non-zero
// if ht = 0 height is reset to match width
// about 10% slower than "MaskBoxAvg" (for some unknown reason)
// useful for signed values where zero corresponds to 128
// can optionally require "samps" or more samples to take average
// where no data are averaged, fills in with value "def"

int jhcArea::NotBoxAvg (jhcImg &dest, const jhcImg& src, int wid, int ht, 
                        int bg, int def, int samps)
{
  UL32 n = __max(1, samps);
  int dx = wid, dy = ((ht == 0) ? wid : ht);

  // check arguments
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::NotBoxAvg");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::NotBoxAvg", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0))
    return 0;
  if ((dx == 1) && (dy == 1))
    return dest.CopyArr(src);
  dest.CopyRoi(src);  

  // generic ROI case
  int x, y, rsk4;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int nx = dx / 2, px = dx - nx, xlim = rw - px;
  int ny = dy / 2, py = dy - ny, ylim = rh - py + 1, nyp = ny + 1;
  UC8 bdef = BOUND(def);
  UL32 roff = dest.RoiOff(); 
  UL32 sum, cnt, roff4, line4 = dest.XDim();
  UC8 *a;
  const UC8 *a0, *ahi, *alo;
  UL32 *b, *b0, *blo, *bhi, *bsum;
  UL32 *c, *c0, *clo, *chi, *csum;

  // set up pointer to start of region of interest in scratch arrays
  dest.RoiParams(&roff4, &rsk4, 4);
  rsk4 >>= 2;
  a4.SetSize(dest, 4);
  b4.SetSize(dest, 4);
  b0 = (UL32 *)(a4.PxlDest() + roff4);
  c0 = (UL32 *)(b4.PxlDest() + roff4);


  // PASS 1 vertical ===============================================
  // vertically smear from source into temp using running sum
  // keep rw running sums at the same time to preserve scan order
  a0 = src.PxlSrc() + roff;

  // initialize running sums (in bsum), one per column in ROI and
  //   also initialize count of PxlSrc() exceeding threshold (in csum)
  //   i.e. in each column sum up PxlSrc() within mask half-height of top
  // this computes exactly the answer for the top row of PxlSrc()
  // ahi and ghi point to start of line "py" at end of this stage
  bsum = b0;
  csum = c0;
  for (x = rw; x > 0; x--)         // zero the first row of sums
  {
    *bsum++ = 0;
    *csum++ = 0;
  }
  ahi  = a0;
  for (y = 0; y < py; y++)         // add non-masked PxlSrc() to sums
  {
    bsum = b0;
    csum = c0;
    for (x = rw; x > 0; x--)
    {
      if (*ahi != bg)
      {
        *bsum += *ahi;
        *csum += 1;
      }
      ahi++;
      bsum++;
      csum++;
    }
    ahi += rsk;
  }                                

  // mask falls off top initially
  // take previous running sum (row above) and adjust by adding
  //   source pixel from lower line and subtracting source pixel at top
  // nyp not incremented since edge values are not repeated in sum
  // ahi points to start of line "dy" at end of this stage
  bsum = b0;                      // points to beginning of first line
  csum = c0;
  b    = b0 + line4;              // points to beginning of second line
  c    = c0 + line4;
  for (y = 1; y < nyp; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b = *bsum++;               // init sum and count to vals from above
      *c = *csum++;
      if (*ahi != bg)             // add in head if not background
      {
        *b += *ahi;
        *c += 1;
      }
      ahi++;
      b++;
      c++;
    }
    ahi  += rsk;                  // advance output and hi pointers
    b    += rsk4;                 
    c    += rsk4;
    bsum += rsk4;
    csum += rsk4;
  }

  // convolution mask (box) is fully inside for most
  // get previous running sum, add bottom, and subtract top
  // alo points to start of line "ylim - nyp" at end of this stage
  alo = a0;
  for (y = nyp; y < ylim; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b = *bsum++;               // init sum and count to vals from above
      *c = *csum++;
      if (*alo != bg)             // subtract off tail if not background
      {
        *b -= *alo;
        *c -= 1;
      }
      alo++;
      if (*ahi != bg)             // add in head if not background
      {
        *b += *ahi;
        *c += 1;
      }
      ahi++;
      b++;
      c++;
    }
    b    += rsk4;                 // advance output, lo, and hi pointers
    c    += rsk4;
    bsum += rsk4;
    csum += rsk4;
    alo  += rsk;
    ahi  += rsk;
  }

  // mask falls off bottom at end
  // get previous running sum and subtract top
  for (y = ylim; y < rh; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b = *bsum++;               // init sum and count to vals from above
      *c = *csum++;
      if (*alo != bg)             // subtract off tail if not background
      {
        *b -= *alo;
        *c -= 1;
      }
      alo++;
      b++;
      c++;
    }
    b    += rsk4;                 // advance output and lo pointers
    c    += rsk4;
    bsum += rsk4;
    csum += rsk4;
    alo  += rsk;
  }


  // PASS 2 horizontal ========================================
  // horizontal smear from temp into local using running sum
  // adds in new right side, subtracts off old left side
  // no inter need to test gate image since this info has already been 
  //   incorporated into the two temporaries: b (sum) and c (count)
  b = b0;
  c = c0;
  a = dest.PxlDest() + roff;
  for (y = rh; y > 0; y--)
  {
    // initialize running sum
    // bhi and chi point to pixel px in line afterwards
    bhi = b;
    chi = c;
    sum = 0;
    cnt = 0;
    for (x = 0; x < px; x++)
    {
      sum += *bhi++;
      cnt += *chi++;
    }

    // mask falls off left initially
    // bhi and chi point to pixel dx in line afterwards
    for (x = 0; x < nx; x++)
    {
      if (cnt < n)
        *a++ = bdef;
      else if (cnt == 1)
        *a++ = (UC8) sum;
      else
        *a++ = (UC8)(sum / cnt);
      sum += *bhi++;
      cnt += *chi++;
    }

    // mask is fully inside for most
    blo = b;
    clo = c;
    for (x = nx; x < xlim; x++)
    {
      if (cnt < n)
        *a++ = bdef;
      else if (cnt == 1)
        *a++ = (UC8) sum;
      else 
        *a++ = (UC8)(sum / cnt);
      sum -= *blo++;
      sum += *bhi++;
      cnt -= *clo++;
      cnt += *chi++;
    }

    // mask falls off right at end
    for (x = xlim; x < rw; x++)
    {
      if (cnt < n)
        *a++ = bdef;
      else if (cnt == 1)
        *a++ = (UC8) sum;
      else
        *a++ = (UC8)(sum / cnt);
      sum -= *blo++;
      cnt -= *clo++;
    }

    // move down to next line
    a += rsk;
    b += line4;
    c += line4;
  }
  return 1;
}


//= Do non-zero box averaging where size is a fraction of array width.
// Height also as fraction of array width but then rescaled by aspect
// if ht = 0.0 height is reset to match width (then aspect corrected)

int jhcArea::NZBoxAvg (jhcImg& dest, const jhcImg& src, double wf, double hf, int samps)
{
  double wbox = wf * dest.XDim(), hbox = hf * dest.XDim();

  if (hf == 0.0)
    hbox = wbox;
  return(NZBoxAvg(dest, src, (int) wbox, (int)(hbox / dest.Ratio()), samps));
}


//= Averages PxlSrc() in a box only where source PxlSrc() are non-zero.

int jhcArea::NZBoxAvg (jhcImg &dest, const jhcImg& src, int wid, int ht, int samps)
{
  return NotBoxAvg(dest, src, wid, ht, 0, 0, samps);
}


//= Averages non-zero pixels in region then takes max with central pixel.
// typically used for filling gaps in depth map (e.g. jhcOverhead3D::Interpolate)

int jhcArea::NZBoxMax (jhcImg &dest, const jhcImg& src, int wid, int ht, int samps)
{
  UL32 n = __max(1, samps);
  int dx = wid, dy = ((ht == 0) ? wid : ht);

  // check arguments
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::NZBoxMax");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::NZBoxMax", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0))
    return 0;
  if ((dx == 1) && (dy == 1))
    return dest.CopyArr(src);
  dest.CopyRoi(src);  

  // generic ROI case
  int x, y, rsk4;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int nx = dx / 2, px = dx - nx, xlim = rw - px;
  int ny = dy / 2, py = dy - ny, ylim = rh - py + 1, nyp = ny + 1;
  UC8 avg;
  UL32 roff = dest.RoiOff(); 
  UL32 sum, cnt, roff4, line4 = dest.XDim();
  UC8 *a;
  const UC8 *m, *a0, *ahi, *alo;
  UL32 *b, *b0, *blo, *bhi, *bsum;
  UL32 *c, *c0, *clo, *chi, *csum;

  // set up pointer to start of region of interest in scratch arrays
  dest.RoiParams(&roff4, &rsk4, 4);
  rsk4 >>= 2;
  a4.SetSize(dest, 4);
  b4.SetSize(dest, 4);
  b0 = (UL32 *)(a4.PxlDest() + roff4);
  c0 = (UL32 *)(b4.PxlDest() + roff4);


  // PASS 1 vertical ===============================================
  // vertically smear from source into temp using running sum
  // keep rw running sums at the same time to preserve scan order
  a0 = src.PxlSrc() + roff;

  // initialize running sums (in bsum), one per column in ROI and
  //   also initialize count of PxlSrc() exceeding threshold (in csum)
  //   i.e. in each column sum up PxlSrc() within mask half-height of top
  // this computes exactly the answer for the top row of PxlSrc()
  // ahi and ghi point to start of line "py" at end of this stage
  bsum = b0;
  csum = c0;
  for (x = rw; x > 0; x--)         // zero the first row of sums
  {
    *bsum++ = 0;
    *csum++ = 0;
  }
  ahi  = a0;
  for (y = 0; y < py; y++)         // add non-masked PxlSrc() to sums
  {
    bsum = b0;
    csum = c0;
    for (x = rw; x > 0; x--)
    {
      if (*ahi > 0)
      {
        *bsum += *ahi;
        *csum += 1;
      }
      ahi++;
      bsum++;
      csum++;
    }
    ahi += rsk;
  }                                

  // mask falls off top initially
  // take previous running sum (row above) and adjust by adding
  //   source pixel from lower line and subtracting source pixel at top
  // nyp not incremented since edge values are not repeated in sum
  // ahi points to start of line "dy" at end of this stage
  bsum = b0;                      // points to beginning of first line
  csum = c0;
  b    = b0 + line4;              // points to beginning of second line
  c    = c0 + line4;
  for (y = 1; y < nyp; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b = *bsum++;               // init sum and count to vals from above
      *c = *csum++;
      if (*ahi > 0)               // add in head if not background
      {
        *b += *ahi;
        *c += 1;
      }
      ahi++;
      b++;
      c++;
    }
    ahi  += rsk;                  // advance output and hi pointers
    b    += rsk4;                 
    c    += rsk4;
    bsum += rsk4;
    csum += rsk4;
  }

  // convolution mask (box) is fully inside for most
  // get previous running sum, add bottom, and subtract top
  // alo points to start of line "ylim - nyp" at end of this stage
  alo = a0;
  for (y = nyp; y < ylim; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b = *bsum++;               // init sum and count to vals from above
      *c = *csum++;
      if (*alo > 0)               // subtract off tail if not background
      {
        *b -= *alo;
        *c -= 1;
      }
      alo++;
      if (*ahi > 0)               // add in head if not background
      {
        *b += *ahi;
        *c += 1;
      }
      ahi++;
      b++;
      c++;
    }
    b    += rsk4;                 // advance output, lo, and hi pointers
    c    += rsk4;
    bsum += rsk4;
    csum += rsk4;
    alo  += rsk;
    ahi  += rsk;
  }

  // mask falls off bottom at end
  // get previous running sum and subtract top
  for (y = ylim; y < rh; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b = *bsum++;               // init sum and count to vals from above
      *c = *csum++;
      if (*alo > 0)               // subtract off tail if not background
      {
        *b -= *alo;
        *c -= 1;
      }
      alo++;
      b++;
      c++;
    }
    b    += rsk4;                 // advance output and lo pointers
    c    += rsk4;
    bsum += rsk4;
    csum += rsk4;
    alo  += rsk;
  }


  // PASS 2 horizontal ========================================
  // horizontal smear from temp into local using running sum
  // adds in new right side, subtracts off old left side
  // no inter need to test gate image since this info has already been 
  //   incorporated into the two temporaries: b (sum) and c (count)
  b = b0;
  c = c0;
  a = dest.PxlDest() + roff;
  m = src.PxlSrc() + roff;          // original middle pixel
  for (y = rh; y > 0; y--)
  {
    // initialize running sum
    // bhi and chi point to pixel px in line afterwards
    bhi = b;
    chi = c;
    sum = 0;
    cnt = 0;
    for (x = 0; x < px; x++)
    {
      sum += *bhi++;
      cnt += *chi++;
    }

    // mask falls off left initially
    // bhi and chi point to pixel dx in line afterwards
    for (x = 0; x < nx; x++)
    {
      if (cnt < n)
        *a++ = *m;
      else if (cnt == 1)
        *a++ = __max(*m, (UC8) sum);
      else
      {
        avg = (UC8)(sum / cnt);
        *a++ = __max(*m, avg);
      }
      sum += *bhi++;
      cnt += *chi++;
      m++;
    }

    // mask is fully inside for most
    blo = b;
    clo = c;
    for (x = nx; x < xlim; x++)
    {
      if (cnt < n)
        *a++ = *m;
      else if (cnt == 1)
        *a++ = __max(*m, (UC8) sum);
      else 
      {
        avg = (UC8)(sum / cnt);
        *a++ = __max(*m, avg);
      }
      sum -= *blo++;
      sum += *bhi++;
      cnt -= *clo++;
      cnt += *chi++;
      m++;
    }

    // mask falls off right at end
    for (x = xlim; x < rw; x++)
    {
      if (cnt < n)
        *a++ = *m;
      else if (cnt == 1)
        *a++ = __max(*m, (UC8) sum);
      else
      {
        avg = (UC8)(sum / cnt);
        *a++ = __max(*m, avg);
      }
      sum -= *blo++;
      cnt -= *clo++;
      m++;
    }

    // move down to next line
    a += rsk;
    b += line4;
    c += line4;
    m += rsk;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                     Center Surround Operations                        //
///////////////////////////////////////////////////////////////////////////

//= Subtracts average in surround area (sw x sh) from center area (cw x ch).
// truncates and only shows positive values, can scale by some factor
// a negative scale factor flips meaning of center and surround

int jhcArea::ClipCS (jhcImg& dest, const jhcImg& src, 
                     int cw, int ch, int sw, int sh, double sc)
{
  const jhcImg *cnt = &src, *sur = &src;

  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::ClipCS");
  if ((sc == 0.0) || (cw <= 0) || (ch <= 0) || (sw <= 0) || (sh <= 0))
    return 0;
  dest.CopyRoi(src);

  // handle simplest case
  if ((cw == 1) && (ch == 1) && (sw == 1) && (sh == 1))
    return dest.FillArr(0);

  // determine where to write center and surround images
  if ((cw != 1) || (ch != 1))
  {
    cnt = a1.SetSize(src);
    BoxAvg(a1, src, cw, ch);
  }
  if ((sw != 1) || (sh != 1))
  {
    sur = b1.SetSize(src);
    BoxAvg(b1, src, sw, sh);
  }

  // combine the images
  if (sc < 0.0)
    cdiff(dest, *sur, *cnt, -sc);
  else 
    cdiff(dest, *cnt, *sur, sc);
  return 1;
}


//= Subtracts average in surround area (sw x sh) from center area (cw x ch).
// a difference of zero corresponds to 128, can scale around center value
// a negative scale factor flips meaning of center and surround

int jhcArea::LiftCS (jhcImg& dest, const jhcImg& src, 
                     int cw, int ch, int sw, int sh, double sc)
{
  const jhcImg *cnt = &src, *sur = &src;

  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::LiftCS");
  if ((sc == 0.0) || (cw <= 0) || (ch <= 0) || (sw <= 0) || (sh <= 0))
    return 0;
  dest.CopyRoi(src);

  // handle simplest case
  if ((cw == 1) && (ch == 1) && (sw == 1) && (sh == 1))
    return dest.FillArr(0);

  // determine where to write center and surround images
  if ((cw != 1) || (ch != 1))
  {
    cnt = a1.SetSize(src);
    BoxAvg(a1, src, cw, ch);
  }
  if ((sw != 1) || (sh != 1))
  {
    sur = b1.SetSize(src);
    BoxAvg(b1, src, sw, sh);
  }

  // combine the images
  if (sc < 0.0)
    ldiff(dest, *sur, *cnt, -sc);
  else 
    ldiff(dest, *cnt, *sur, sc);
  return 1;
}


//= Fills destination with the difference of two other images (all fields).
// scales difference by a small factor and clips result to between 0 and 255
// modified copy of jhcALU::ClipDiff

void jhcArea::cdiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const 
{
  int x, y, v, diff, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  int sum = -255 * f + 128;
  UC8 scaled[512];

  // compute arbitrarily scaled result for all possible differences
  for (diff = -255; diff <= 255; diff++)
  {
    v = (int)(sum >> 8);
    scaled[diff + 255] = BOUND(v);
    sum += f;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = scaled[(int)(*a++) - (int)(*b++) + 255];
    d += rsk;
    a += rsk;
    b += rsk;
  }
}


//= Fills destination with the difference of two other images (all fields).
// scales difference by a small factor and adds to 128 (to allow negatives)
// modified copy of jhcALU::LiftDiff

void jhcArea::ldiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const 
{
  int x, y, v, diff, f = ROUND(256.0 * sc);
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *a = imga.PxlSrc() + roff, *b = imgb.PxlSrc() + roff;
  int sum = -255 * f + 32768;
  UC8 scaled[512];

  // compute arbitrarily scaled result for all possible differences
  for (diff = -255; diff <= 255; diff++)
  {
    v = (int)(sum >> 8);
    scaled[diff + 255] = BOUND(v);
    sum += f;
  }

  // run lookup table over image
  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = scaled[(int)(*a++) - (int)(*b++) + 255];
    d += rsk;
    a += rsk;
    b += rsk;
  }
}


//= Normalize each pixel's intensity relative to a local patch around it.
// boosts contrast sort of like edge finding

int jhcArea::LocalAGC (jhcImg& dest, const jhcImg& src, int wid, double sc, int i0)
{
  // get local area average in dest
  if (!dest.Valid() || !dest.SameFormat(src) || (i0 <= 0))
    return Fatal("Bad images to jhcArea::LocalAGC");
  BoxAvg(dest, src, wid, wid);

  // general ROI case
  int r[256];
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dest);
  double sc16 = 32768.0 * sc;
  int i, x, y, v, rcnt = dest.RoiCnt(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int bot = ROUND(sc16 / i0), off = ROUND(32768.0 * (sc - 1.0) + 128.0);

  // build scaled division table
  for (i = 0; i <= i0; i++)
    r[i] = bot;
  for (i = i0 + 1; i <= 255; i++)
    r[i] = ROUND(sc16 / i);

  // apply table to all pixels in ROI
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rcnt; x > 0; x--, d++, s++)
    {
      v = (((*s) * r[*d]) - off) >> 8;
      *d = BOUND(v);
    }
  return 1;
}
 

///////////////////////////////////////////////////////////////////////////
//                       Statistical Operations                          //
///////////////////////////////////////////////////////////////////////////

//= Computes standard deviation in a rectangular area around each pixel.
// val = sqrt(E(x^2) - [E(x)]^2 = sqrt(sum(x^2) / n - [sum(x) / n]^2)
// can multiply result by a scaling factor - handles up to 257 x 257
// Note: does pixel squaring twice

int jhcArea::BoxStd (jhcImg &dest, const jhcImg& src, int wid, int ht, double sc) 
{
  int dx = wid, dy = ((ht == 0) ? wid : ht), area = dx * dy;

  // check arguments
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::BoxStd");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxStd", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0) || (sc <= 0.0))
    return 0;
  if (area > 66051)  // 32 bit overflow beyond (2^32 - 1) / (255^2)
    return 0;
  dest.CopyRoi(src);
  if ((area == 1) && (sc == 1.0))
    return dest.FillArr(0);

  // generic ROI case
  double nsc = sc / (double) area;
  UL32 sum, ssq, ej, ej2, val;
//  unsigned __int64 v64;
  int x, y, wx, rsk4; 
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  int nx = dx / 2, px = dx - nx, xlim = rw - px - 1;
  int ny = dy / 2, py = dy - ny, ylim = rh - py, nyp = ny + 1;
  UL32 roff4, line4 = dest.XDim();
  UC8 *a;
  const UC8 *a0, *ahi, *alo, *aej;
  UL32 *b, *b0, *blo, *bhi, *bsum;
  UL32 *c, *c0, *clo, *chi, *csum;

  // set up pointer to start of region of interest in victims
  a4.SetSize(dest, 4);
  b4.SetSize(dest, 4);
  dest.RoiParams(&roff4, &rsk4, 4);
  rsk4 >>= 2;
  b0 = (UL32 *)(a4.PxlDest() + roff4);
  c0 = (UL32 *)(b4.PxlDest() + roff4);


  // PASS 1 vertical ==============================================
  // vertically smear from source into temp using running sum
  // keep rw running sums at the same time to preserve scan order
  a0 = src.PxlSrc() + src.RoiOff();

  // initialize running sums (in bsum and csum), one per column in ROI
  // duplicate values at edge to handle mask overhang
  // this is exactly the answer for top row of PxlSrc()
  csum = c0;
  bsum = b0;
  ahi  = a0;
  for (x = rw; x > 0; x--)
  {
    val = nyp * (*ahi);
    *bsum++ = val;              // holds sum of values
    *csum++ = val * (*ahi);     // holds sum of squares
    ahi++;
  }
  ahi += rsk;
  for (y = 1; y < py; y++)
  {
    csum = c0;
    bsum = b0;
    for (x = rw; x > 0; x--)
    {
      *bsum++ += *ahi;
      *csum++ += ((*ahi) * (*ahi));
      ahi++;
    }
    ahi += rsk;
  }

  // mask falls off top initially
  // take previous running sum (row above) and adjust by adding
  // source pixel from lower line and subtracting source pixel at top
  // nyp incremented since sum is updated BEFORE writing out, not after
  nyp++;
  csum = c0;
  c    = c0 + line4;
  bsum = b0;
  b    = b0 + line4;
  for (y = 1; y < nyp; y++)
  {
    alo = a0;
    for (x = rw; x > 0; x--)
    {
      *b++ = *bsum++ - *alo + *ahi;
      *c++ = *csum++ - ((*alo) * (*alo)) + ((*ahi) * (*ahi));
      ahi++;
      alo++;
    }
    c    += rsk4;
    csum += rsk4;
    b    += rsk4;
    bsum += rsk4;
    ahi  += rsk;
  }
  alo += rsk;

  // mask is fully inside for most
  // get previous running sum, add bottom, subtract top
  for (y = nyp; y < ylim; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b++ = *bsum++ - *alo + *ahi;
      *c++ = *csum++ - ((*alo) * (*alo)) + ((*ahi) * (*ahi));
      ahi++;
      alo++;
    }
    c    += rsk4;
    csum += rsk4;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  += rsk;
  }

  // mask falls off bottom at end
  // get previous running sum, add edge, subtract top
  // for first center the add is legimately the last pixel
  aej = ahi;
  for (y = ylim; y < rh; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b++ = *bsum++ - *alo + *ahi;
      *c++ = *csum++ - ((*alo) * (*alo)) + ((*ahi) * (*ahi));
      ahi++;
      alo++;
    }
    c    += rsk4;
    csum += rsk4;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  =  aej;
  }


  // PASS 2 horizontal =========================================
  // horizontal smear from temp into local using running sum
  // adds in new right side, subtracts off old left side
  c = c0;
  b = b0;
  a = dest.PxlDest() + dest.RoiOff();
  for (y = rh; y > 0; y--)
  {
    // initialize running sum 
    ssq = 0;
    sum = 0;
    for (x = 0; x < dx; x++)
      if (x <= nx)
      {
        ssq += *c; 
        sum += *b; 
      }
      else
      {
        wx = x - nx;
        ssq += *(c + wx);
        sum += *(b + wx);
      }

    // mask falls off left initially
    ej2 = *c;
    ej  = *b;
    chi = c + px;
    bhi = b + px;
    for (x = 0; x < nx; x++)
    {
//      v64 = area * (unsigned __int64) ssq - sum * (unsigned __int64) sum;
//      val = (UL32) v64;
//      val = (UL32)(nsc * sqrt((double) val) + 0.5);
double fval = area * (double) ssq - sum * (double) sum;
val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *a++ = (UC8) val;
      ssq -= ej2;
      ssq += *chi++;
      sum -= ej;
      sum += *bhi++;
    }

    // mask is fully inside for most
    clo = c;
    blo = b;
    for (x = nx; x < xlim; x++)
    {

//      v64 = area * (unsigned __int64) ssq - sum * (unsigned __int64) sum;
//      val = (UL32) v64;
//      val = (UL32)(nsc * sqrt((double) val) + 0.5);
double fval = area * (double) ssq - sum * (double) sum;
val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *a++ = (UC8) val;
      ssq -= *clo++;
      ssq += *chi++;
      sum -= *blo++;
      sum += *bhi++;
    }

    // mask falls off right at end    
    ej2 = *chi;
    ej  = *bhi;
    for (x = xlim; x < rw; x++)
    {
//      v64 = area * (unsigned __int64) ssq - sum * (unsigned __int64) sum;
//      val = (UL32) v64;
//      val = (UL32)(nsc * sqrt((double) val) + 0.5);
double fval = area * (double) ssq - sum * (double) sum;
val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *a++ = (UC8) val;
      ssq -= *clo++;
      ssq += ej2;
      sum -= *blo++;
      sum += ej;
    }

    // move down to next line
    a += rsk;
    c += line4;
    b += line4;
  }
  return 1;
}


//= Like BoxStd but returns averages also in a second array.
// Note: does pixel squaring twice

int jhcArea::BoxAvgStd (jhcImg &avg, jhcImg& std, const jhcImg& src, 
                        int wid, int ht, double dsc)
{
  int dx = wid, dy = ((ht == 0) ? wid : ht), area = dx * dy;

  // check arguments
  if (!src.Valid(1) || !src.SameFormat(avg) || !src.SameFormat(std))
    return Fatal("Bad images to jhcArea::BoxAvgStd");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxAvgStd", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0) || (dsc <= 0.0))
    return 0;
  if (area > 66051)  // 32 bit overflow beyond (2^32 - 1) / (255^2)
    return 0;
  avg.CopyRoi(src);
  std.CopyRoi(src);
  if ((area == 1) && (dsc == 1.0))
  {
    std.FillArr(1);
    return avg.CopyArr(src);
  }

  // generic ROI case
  double fval, norm = 1.0 / (double) area, nsc = dsc * norm;
  UL32 sum, ssq, ej, ej2, val;
  int x, y, wx, rsk4; 
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  int nx = dx / 2, px = dx - nx, xlim = rw - px - 1;
  int ny = dy / 2, py = dy - ny, ylim = rh - py, nyp = ny + 1;
  UL32 roff4, line4 = src.XDim();
  UC8 *d, *a;
  const UC8 *a0, *ahi, *alo, *aej;
  UL32 *b, *b0, *blo, *bhi, *bsum;
  UL32 *c, *c0, *clo, *chi, *csum;

  // set up pointer to start of region of interest in victims
  a4.SetSize(src, 4);
  b4.SetSize(src, 4);
  src.RoiParams(&roff4, &rsk4, 4);
  rsk4 >>= 2;
  b0 = (UL32 *)(a4.PxlDest() + roff4);
  c0 = (UL32 *)(b4.PxlDest() + roff4);


  // PASS 1 vertical ==============================================
  // vertically smear from source into temp using running sum
  // keep rw running sums at the same time to preserve scan order
  a0 = src.PxlSrc() + src.RoiOff();

  // initialize running sums (in bsum and csum), one per column in ROI
  // duplicate values at edge to handle mask overhang
  // this is exactly the answer for top row of PxlSrc()
  csum = c0;
  bsum = b0;
  ahi  = a0;
  for (x = rw; x > 0; x--)
  {
    val = nyp * (*ahi);
    *bsum++ = val;              // holds sum of values
    *csum++ = val * (*ahi);     // holds sum of squares
    ahi++;
  }
  ahi += rsk;
  for (y = 1; y < py; y++)
  {
    csum = c0;
    bsum = b0;
    for (x = rw; x > 0; x--)
    {
      *bsum++ += *ahi;
      *csum++ += ((*ahi) * (*ahi));
      ahi++;
    }
    ahi += rsk;
  }

  // mask falls off top initially
  // take previous running sum (row above) and adjust by adding
  // source pixel from lower line and subtracting source pixel at top
  // nyp incremented since sum is updated BEFORE writing out, not after
  nyp++;
  csum = c0;
  c    = c0 + line4;
  bsum = b0;
  b    = b0 + line4;
  for (y = 1; y < nyp; y++)
  {
    alo = a0;
    for (x = rw; x > 0; x--)
    {
      *b++ = *bsum++ - *alo + *ahi;
      *c++ = *csum++ - ((*alo) * (*alo)) + ((*ahi) * (*ahi));
      ahi++;
      alo++;
    }
    c    += rsk4;
    csum += rsk4;
    b    += rsk4;
    bsum += rsk4;
    ahi  += rsk;
  }
  alo += rsk;

  // mask is fully inside for most
  // get previous running sum, add bottom, subtract top
  for (y = nyp; y < ylim; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b++ = *bsum++ - *alo + *ahi;
      *c++ = *csum++ - ((*alo) * (*alo)) + ((*ahi) * (*ahi));
      ahi++;
      alo++;
    }
    c    += rsk4;
    csum += rsk4;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  += rsk;
  }

  // mask falls off bottom at end
  // get previous running sum, add edge, subtract top
  // for first center the add is legimately the last pixel
  aej = ahi;
  for (y = ylim; y < rh; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b++ = *bsum++ - *alo + *ahi;
      *c++ = *csum++ - ((*alo) * (*alo)) + ((*ahi) * (*ahi));
      ahi++;
      alo++;
    }
    c    += rsk4;
    csum += rsk4;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  =  aej;
  }


  // PASS 2 horizontal =========================================
  // horizontal smear from temp into local using running sum
  // adds in new right side, subtracts off old left side
  c = c0;
  b = b0;
  a = avg.PxlDest() + avg.RoiOff();
  d = std.PxlDest() + std.RoiOff();
  for (y = rh; y > 0; y--)
  {
    // initialize running sum 
    ssq = 0;
    sum = 0;
    for (x = 0; x < dx; x++)
      if (x <= nx)
      {
        ssq += *c; 
        sum += *b; 
      }
      else
      {
        wx = x - nx;
        ssq += *(c + wx);
        sum += *(b + wx);
      }

    // mask falls off left initially
    ej2 = *c;
    ej  = *b;
    chi = c + px;
    bhi = b + px;
    for (x = 0; x < nx; x++)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *d++ = (UC8) val;
      *a++ = (UC8)(norm * sum + 0.5);
      ssq -= ej2;
      ssq += *chi++;
      sum -= ej;
      sum += *bhi++;
    }

    // mask is fully inside for most
    clo = c;
    blo = b;
    for (x = nx; x < xlim; x++)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *d++ = (UC8) val;
      *a++ = (UC8)(norm * sum + 0.5);
      ssq -= *clo++;
      ssq += *chi++;
      sum -= *blo++;
      sum += *bhi++;
    }

    // mask falls off right at end    
    ej2 = *chi;
    ej  = *bhi;
    for (x = xlim; x < rw; x++)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *d++ = (UC8) val;
      *a++ = (UC8)(norm * sum + 0.5);
      ssq -= *clo++;
      ssq += ej2;
      sum -= *blo++;
      sum += ej;
    }

    // move down to next line
    a += rsk;
    d += rsk;
    c += line4;
    b += line4;
  }
  return 1;
}


//= Like BoxAvgStd but returns inverse of standard deviation to allow.
// normalization by multiplication (isd = __min(255, 256 / (dsc * std)))
// Note: does pixel squaring twice

int jhcArea::BoxAvgInv (jhcImg &avg, jhcImg& isd, const jhcImg& src, 
                        int wid, int ht, double dsc)
{
  int dx = wid, dy = ((ht == 0) ? wid : ht), area = dx * dy;

  // check arguments
  if (!src.Valid(1) || !src.SameFormat(avg) || !src.SameFormat(isd))
    return Fatal("Bad images to jhcArea::BoxAvgInv");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxAvgInv", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0) || (dsc <= 0.0))
    return 0;
  if (area > 66051)  // 32 bit overflow beyond (2^32 - 1) / (255^2)
    return 0;
  avg.CopyRoi(src);
  isd.CopyRoi(src);
  if ((area == 1) && (dsc == 1.0))
  {
    isd.FillArr(255);
    return avg.CopyArr(src);
  }

  // generic ROI case
  double fval, norm = 1.0 / (double) area, nsc = 256.0 * area / dsc;
  UL32 sum, ssq, ej, ej2, val;
  int x, y, wx, rsk4; 
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  int nx = dx / 2, px = dx - nx, xlim = rw - px - 1;
  int ny = dy / 2, py = dy - ny, ylim = rh - py, nyp = ny + 1;
  UL32 roff4, line4 = src.XDim();
  UC8 *d, *a;
  const UC8 *a0, *ahi, *alo, *aej;
  UL32 *b, *b0, *blo, *bhi, *bsum;
  UL32 *c, *c0, *clo, *chi, *csum;

  // set up pointer to start of region of interest in victims
  a4.SetSize(src, 4);
  b4.SetSize(src, 4);
  src.RoiParams(&roff4, &rsk4, 4);
  rsk4 >>= 2;
  b0 = (UL32 *)(a4.PxlDest() + roff4);
  c0 = (UL32 *)(b4.PxlDest() + roff4);


  // PASS 1 vertical ==============================================
  // vertically smear from source into temp using running sum
  // keep rw running sums at the same time to preserve scan order
  a0 = src.PxlSrc() + src.RoiOff();

  // initialize running sums (in bsum and csum), one per column in ROI
  // duplicate values at edge to handle mask overhang
  // this is exactly the answer for top row of PxlSrc()
  csum = c0;
  bsum = b0;
  ahi  = a0;
  for (x = rw; x > 0; x--)
  {
    val = nyp * (*ahi);
    *bsum++ = val;              // holds sum of values
    *csum++ = val * (*ahi);     // holds sum of squares
    ahi++;
  }
  ahi += rsk;
  for (y = 1; y < py; y++)
  {
    csum = c0;
    bsum = b0;
    for (x = rw; x > 0; x--)
    {
      *bsum++ += *ahi;
      *csum++ += ((*ahi) * (*ahi));
      ahi++;
    }
    ahi += rsk;
  }

  // mask falls off top initially
  // take previous running sum (row above) and adjust by adding
  // source pixel from lower line and subtracting source pixel at top
  // nyp incremented since sum is updated BEFORE writing out, not after
  nyp++;
  csum = c0;
  c    = c0 + line4;
  bsum = b0;
  b    = b0 + line4;
  for (y = 1; y < nyp; y++)
  {
    alo = a0;
    for (x = rw; x > 0; x--)
    {
      *b++ = *bsum++ - *alo + *ahi;
      *c++ = *csum++ - ((*alo) * (*alo)) + ((*ahi) * (*ahi));
      ahi++;
      alo++;
    }
    c    += rsk4;
    csum += rsk4;
    b    += rsk4;
    bsum += rsk4;
    ahi  += rsk;
  }
  alo += rsk;

  // mask is fully inside for most
  // get previous running sum, add bottom, subtract top
  for (y = nyp; y < ylim; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b++ = *bsum++ - *alo + *ahi;
      *c++ = *csum++ - ((*alo) * (*alo)) + ((*ahi) * (*ahi));
      ahi++;
      alo++;
    }
    c    += rsk4;
    csum += rsk4;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  += rsk;
  }

  // mask falls off bottom at end
  // get previous running sum, add edge, subtract top
  // for first center the add is legimately the last pixel
  aej = ahi;
  for (y = ylim; y < rh; y++)
  {
    for (x = rw; x > 0; x--)
    {
      *b++ = *bsum++ - *alo + *ahi;
      *c++ = *csum++ - ((*alo) * (*alo)) + ((*ahi) * (*ahi));
      ahi++;
      alo++;
    }
    c    += rsk4;
    csum += rsk4;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk;
    ahi  =  aej;
  }


  // PASS 2 horizontal =========================================
  // horizontal smear from temp into local using running sum
  // adds in new right side, subtracts off old left side
  c = c0;
  b = b0;
  a = avg.PxlDest() + avg.RoiOff();
  d = isd.PxlDest() + isd.RoiOff();
  for (y = rh; y > 0; y--)
  {
    // initialize running sum 
    ssq = 0;
    sum = 0;
    for (x = 0; x < dx; x++)
      if (x <= nx)
      {
        ssq += *c; 
        sum += *b; 
      }
      else
      {
        wx = x - nx;
        ssq += *(c + wx);
        sum += *(b + wx);
      }

    // mask falls off left initially
    ej2 = *c;
    ej  = *b;
    chi = c + px;
    bhi = b + px;
    for (x = 0; x < nx; x++)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc / sqrt(fval) + 0.5);
      val = __min(255, val);
      *d++ = (UC8) val;
      *a++ = (UC8)(norm * sum + 0.5);
      ssq -= ej2;
      ssq += *chi++;
      sum -= ej;
      sum += *bhi++;
    }

    // mask is fully inside for most
    clo = c;
    blo = b;
    for (x = nx; x < xlim; x++)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc / sqrt(fval) + 0.5);
      val = __min(255, val);
      *d++ = (UC8) val;
      *a++ = (UC8)(norm * sum + 0.5);
      ssq -= *clo++;
      ssq += *chi++;
      sum -= *blo++;
      sum += *bhi++;
    }

    // mask falls off right at end    
    ej2 = *chi;
    ej  = *bhi;
    for (x = xlim; x < rw; x++)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc / sqrt(fval) + 0.5);
      val = __min(255, val);
      *d++ = (UC8) val;
      *a++ = (UC8)(norm * sum + 0.5);
      ssq -= *clo++;
      ssq += ej2;
      sum -= *blo++;
      sum += ej;
    }

    // move down to next line
    a += rsk;
    d += rsk;
    c += line4;
    b += line4;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Diagonal Area Averages                         //
///////////////////////////////////////////////////////////////////////////

//= Like BoxAvg but works uses diamond shaped region instead of a rectangle.
// w1 is the dimension aint the major diagonal (as displayed), h2 is minor
// not optimized for scan order access (already hairy enough!)
// NOTE: clamped output value to 255 as of March 2020

int jhcArea::DBoxAvg (jhcImg &dest, const jhcImg& src, int w1, int h2, double sc)
{
  int d1z = w1, d2z = ((h2 == 0) ? w1 : h2), area = d1z * d2z;
  int sz = __min(src.RoiW(), src.RoiH());

  // check arguments
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::DBoxAvg");
  if ((d1z >= sz) || (d2z >= sz))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::DBoxAvg", 
                 d1z, d2z, src.RoiW(), src.RoiH());

  // special cases
  if ((d1z <= 0) || (d2z <= 0) || (sc <= 0.0))
    return 0;
  if ((area == 1) && (sc == 1.0))
    return dest.CopyArr(src);
  dest.CopyRoi(src);
  a4.SetSize(dest, 4);

  // generic ROI case
  int i, x, y, v, step, step4, lim, lim2, stop;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int line = dest.Line(), line4 = a4.Line() >> 2;  // line4 = a4.XDim();
  int nd1 = d1z / 2, pd1 = d1z - nd1, nd1i = nd1 + 1;
  int nd2 = d2z / 2, pd2 = d2z - nd2, nd2i = nd2 + 1;
  UL32 sum, norm;
  const UC8 *s, *s2, *sst, *s0;
  UC8 *d, *dst, *d0;
  UL32 *m, *m2, *mst, *m0;
  UL32 div[256];

  // precomputed "normalized" values for all intensities
  // such that final division by mask area is just a shift
  norm = (UL32)(65536.0 * sc / area);
  for (i = 0; i <= 255; i++)
    div[i] = norm * i;


  // PASS 1 - MAJOR DIAGONAL \\\\ ================================
  // i increments aint diagonal scan line - up and left
  // start at lower right corner (as displayed) 
  m0 = (UL32 *)(a4.RoiDest()) + rw - 1;
  s0 = src.RoiSrc()  + rw - 1;
  step  = line  - 1;
  step4 = line4 - 1;

  // SW SCAN - scan bottom left half of image (as displayed)
  // -------------------------------------------------------
  mst = m0;
  sst = s0;
  for (x = rw; x > 0; x--)
  {
    m  = mst;             // where to write each sum
    s  = sst;             // leading edge pointer
    s2 = sst;             // trailing edge pointer
    lim  = __min(rh, x);  // how far before top or side 
    lim2 = lim - pd1;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd1 * div[*s];      
    i = 1;
    stop = __min(pd1, lim);
    while (i <= stop)
    {
      sum += div[*s];
      s += step;
      i++;
    }
    if (pd1 > lim)        // if mask is bigger than ROI
    {
      s -= step;          // point to top border pixel
      sum += (pd1 - lim) * div[*s];  
      s += step;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd1i, even when dual overhang
    i = 1;
    stop = __min(nd1i, lim2);
    while (i < stop)            
    {
      *m = sum;           // save previously computed running sum
      sum -= div[*s2];    // subtract bottom border copy
      sum += div[*s];     // add leading edge value
      s += step;          // advance leading edge but not trailing 
      m += step4;
      i++;
    }
    if (nd1i > lim2)      // if big mask falls off both sides of ROI
    {
      s -= step;
      stop = __min(nd1i, lim);
      while (i < stop)           
      {
        *m = sum;
        sum -= div[*s2];  // subtract off copy of bottom border pixel
        sum += div[*s];   // add in copy of top border pixel
        m += step4;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      *m = sum;
      sum -= div[*s2];    // subtract trailing, add leading, advance both
      sum += div[*s];
      s  += step;
      s2 += step;
      m  += step4;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      *m = sum;
      sum -= div[*s2];    // subtract trailing
      sum += div[*s];     // add copy of top border
      s2 += step;         // advance only trailing pointer
      m  += step4;
      i++;
    }

    // shift starting point over left by one diagonal for next loop
    sst--;                    
    mst--;
  }

  // NE SCAN - scan top right half of image (as displayed)
  // -------------------------------------------------------
  mst = m0 + line4;
  sst = s0 + line;
  for (y = rh - 1; y > 0; y--)
  {
    m  = mst;             // where to write each sum
    s  = sst;             // leading edge pointer
    s2 = sst;             // trailing edge pointer
    lim  = __min(rw, y);  // how far before top or side 
    lim2 = lim - pd1;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd1 * div[*s];      
    i = 1;
    stop = __min(pd1, lim);
    while (i <= stop)
    {
      sum += div[*s];
      s += step;
      i++;
    }
    if (pd1 > lim)        // if mask is bigger than ROI
    {
      s -= step;          // point to top border pixel
      sum += (pd1 - lim) * div[*s];  
      s += step;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with y = nyp, even when dual overhang
    i = 1;
    stop = __min(nd1i, lim2);
    while (i < stop)
    {
      *m = sum;           // save previously computed running sum
      sum -= div[*s2];    // subtract bottom border copy
      sum += div[*s];     // add leading edge value
      s += step;          // advance leading edge but not trailing 
      m += step4;
      i++;
    }
    if (nd1i > lim2)      // if big mask falls off both sides of ROI
    {
      s -= step;
      stop = __min(nd1i, lim);
      while (i < stop)
      {
        *m = sum;
        sum -= div[*s2];  // subtract off copy of bottom border pixel
        sum += div[*s];   // add in copy of top border pixel
        m += step4;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      *m = sum;
      sum -= div[*s2];    // subtract trailing, add leading, advance both
      sum += div[*s];
      s  += step;
      s2 += step;
      m  += step4;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      *m = sum;
      sum -= div[*s2];    // subtract trailing
      sum += div[*s];     // add copy of top border
      s2 += step;         // advance only trailing pointer
      m  += step4;
      i++;
    }

    // shift starting point up by one diagonal for next loop
    sst += line;                    
    mst += line4;
  }


  // PASS 2 - MINOR DIAGONAL //// ================================
  // i increments aint diagonal scan line - up and right
  // start at lower left corner (as displayed) 
  d0 = dest.RoiDest();
  m0 = (UL32 *)(a4.RoiDest());
  step  = line  + 1;
  step4 = line4 + 1;

  // SE SCAN - scan bottom right half of image (as displayed)
  // -------------------------------------------------------
  dst = d0;
  mst = m0;
  for (x = rw; x > 0; x--)
  {
    d  = dst;             // where to write each sum
    m  = mst;             // leading edge pointer
    m2 = mst;             // trailing edge pointer
    lim  = __min(rh, x);  // how far before top or side 
    lim2 = lim - pd2;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd2 * (*m);      
    i = 1;
    stop = __min(pd2, lim);
    while (i <= stop)
    {
      sum += (*m);
      m += step4;
      i++;
    }
    if (pd2 > lim)        // if mask is bigger than ROI
    {
      m -= step4;         // point to top border pixel
      sum += (pd2 - lim) * (*m);  
      m += step4;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd2i, even when dual overhang
    i = 1;
    stop = __min(nd2i, lim2);
    while (i < stop)            
    {
      v = sum >> 16;
      *d = (UC8) __min(v, 255);
      sum -= (*m2);       // subtract bottom border copy
      sum += (*m);        // add leading edge value
      m += step4;         // advance leading edge but not trailing 
      d += step;
      i++;
    }
    if (nd2i > lim2)      // if big mask falls off both sides of ROI
    {
      m -= step4;
      stop = __min(nd2i, lim);
      while (i < stop)           
      {
        v = sum >> 16;
        *d = (UC8) __min(v, 255);
        sum -= (*m2);     // subtract off copy of bottom border pixel
        sum += (*m);      // add in copy of top border pixel
        d += step;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      v = sum >> 16;
      *d = (UC8) __min(v, 255);
      sum -= (*m2);       // subtract trailing, add leading, advance both
      sum += (*m);
      m  += step4;
      m2 += step4;
      d  += step;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      v = sum >> 16;
      *d = (UC8) __min(v, 255);
      sum -= (*m2);       // subtract trailing
      sum += (*m);        // add copy of top border
      m2 += step4;        // advance only trailing pointer
      d  += step;
      i++;
    }

    // shift starting point over right by one diagonal for next loop
    mst++;                    
    dst++;
  }

  // NW SCAN - scan top left half of image (as displayed)
  // -------------------------------------------------------
  dst = d0 + line;
  mst = m0 + line4;
  for (y = rh - 1; y > 0; y--)
  {
    d  = dst;             // where to write each sum
    m  = mst;             // leading edge pointer
    m2 = mst;             // trailing edge pointer
    lim  = __min(rw, y);  // how far before top or side 
    lim2 = lim - pd2;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd2 * (*m);      
    i = 1;
    stop = __min(pd2, lim);
    while (i <= stop)
    {
      sum += (*m);
      m += step4;
      i++;
    }
    if (pd2 > lim)        // if mask is bigger than ROI
    {
      m -= step4;         // point to top border pixel
      sum += (pd2 - lim) * (*m);  
      m += step4;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd2i, even when dual overhang
    i = 1;
    stop = __min(nd2i, lim2);
    while (i < stop)
    {
      v = sum >> 16;
      *d = (UC8) __min(v, 255);
      sum -= (*m2);       // subtract bottom border copy
      sum += (*m);        // add leading edge value
      m += step4;         // advance leading edge but not trailing 
      d += step;
      i++;
    }
    if (nd2i > lim2)      // if big mask falls off both sides of ROI
    {
      m -= step4;
      stop = __min(nd2i, lim);
      while (i < stop)
      {
        v = sum >> 16;
        *d = (UC8) __min(v, 255);
        sum -= (*m2);     // subtract off copy of bottom border pixel
        sum += (*m);      // add in copy of top border pixel
        d += step;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      v = sum >> 16;
      *d = (UC8) __min(v, 255);
      sum -= (*m2);       // subtract trailing, add leading, advance both
      sum += (*m);
      m  += step4;
      m2 += step4;
      d  += step;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      v = sum >> 16;
      *d = (UC8) __min(v, 255);
      sum -= (*m2);       // subtract trailing
      sum += (*m);        // add copy of top border
      m2 += step4;        // advance only trailing pointer
      d  += step;
      i++;
    }

    // shift starting point up by one diagonal for next loop
    mst += line4;                    
    dst += line;
  }

  return 1;
}


//= Returns both diagonal box average and scaled standard deviation.
// Note: does pixel squaring twice

int jhcArea::DBoxAvgStd (jhcImg &avg, jhcImg& std, const jhcImg& src, 
                         int w1, int h2, double dsc)
{
  int d1z = w1, d2z = ((h2 == 0) ? w1 : h2), area = d1z * d2z;
  int sz = __min(src.RoiW(), src.RoiH());

  // check arguments
  if (!src.Valid(1) || !src.SameFormat(avg) || !src.SameFormat(std))
    return Fatal("Bad images to jhcArea::DBoxAvgStd");
  if ((d1z >= sz) || (d2z >= sz))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::DBoxAvgStd", 
                 d1z, d2z, src.RoiW(), src.RoiH());

  // special cases
  if ((d1z <= 0) || (d2z <= 0) || (dsc <= 0.0))
    return 0;
  if (area > 66051)  // 32 bit overflow beyond (2^32 - 1) / (255^2)
    return 0;
  avg.CopyRoi(src);
  std.CopyRoi(src);
  if ((area == 1) && (dsc == 1.0))
  {
    std.FillArr(0);
    return avg.CopyArr(src);
  }
  a4.SetSize(src, 4);
  b4.SetSize(src, 4);

  // generic ROI case
  int i, x, y, step, step4, lim, lim2, stop;
  int rw = src.RoiW(), rh = src.RoiH();
  int line = src.Line(), line4 = a4.Line() >> 2;  // line4 = a4.XDim();
  int nd1 = d1z / 2, pd1 = d1z - nd1, nd1i = nd1 + 1;
  int nd2 = d2z / 2, pd2 = d2z - nd2, nd2i = nd2 + 1;
  double fval, norm = 1.0 / (double) area, nsc = dsc * norm;
  UL32 sum, ssq, val;
  const UC8 *s, *s2, *sst, *s0;
  UC8 *a, *ast, *a0, *d, *dst, *d0;
  UL32 *m, *m2, *mst, *m0, *v, *v2, *vst, *v0;


  // PASS 1 - MAJOR DIAGONAL \\\\ ================================
  // i increments aint diagonal scan line - up and left
  // start at lower right corner (as displayed) 
  m0 = (UL32 *)(a4.RoiDest()) + rw - 1;
  v0 = (UL32 *)(b4.RoiDest()) + rw - 1;
  s0 = src.RoiSrc()  + rw - 1;
  step  = line  - 1;
  step4 = line4 - 1;

  // SW SCAN - scan bottom left half of image (as displayed)
  // -------------------------------------------------------
  mst = m0;
  vst = v0;
  sst = s0;
  for (x = rw; x > 0; x--)
  {
    m  = mst;             // where to write each sum
    v  = vst;
    s  = sst;             // leading edge pointer
    s2 = sst;             // trailing edge pointer
    lim  = __min(rh, x);  // how far before top or side 
    lim2 = lim - pd1;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd1 * (*s);    
    ssq = sum * (*s);  
    i = 1;
    stop = __min(pd1, lim);
    while (i <= stop)
    {
      sum += *s;
      ssq += (*s) * (*s);
      s += step;
      i++;
    }
    if (pd1 > lim)        // if mask is bigger than ROI
    {
      s -= step;          // point to top border pixel
      val = (pd1 - lim) * (*s);  
      sum += val;
      ssq += val * (*s);
      s += step;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd1i, even when dual overhang
    i = 1;
    stop = __min(nd1i, lim2);
    while (i < stop)            
    {
      *m = sum;           // save previously computed running sum
      *v = ssq;
      sum -= *s2;         // subtract bottom border copy
      ssq -= (*s2) * (*s2);
      sum += *s;          // add leading edge value
      ssq += (*s) * (*s);
      s += step;          // advance leading edge but not trailing 
      m += step4;
      v += step4;
      i++;
    }
    if (nd1i > lim2)      // if big mask falls off both sides of ROI
    {
      s -= step;
      stop = __min(nd1i, lim);
      while (i < stop)           
      {
        *m = sum;
        *v = ssq;
        sum -= *s2;       // subtract off copy of bottom border pixel
        ssq -= (*s2) * (*s2);
        sum += *s;        // add in copy of top border pixel
        ssq += (*s) * (*s);
        m += step4;
        v += step4;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      *m = sum;
      *v = ssq;
      sum -= *s2;         // subtract trailing, add leading, advance both
      ssq -= (*s2) * (*s2);
      sum += *s;
      ssq += (*s) * (*s);
      s  += step;
      s2 += step;
      m  += step4;
      v  += step4;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      *m = sum;
      *v = ssq;
      sum -= *s2;         // subtract trailing
      ssq -= (*s2) * (*s2);
      sum += *s;          // add copy of top border
      ssq += (*s) * (*s);
      s2 += step;         // advance only trailing pointer
      m  += step4;
      v  += step4;
      i++;
    }

    // shift starting point over left by one diagonal for next loop
    sst--;                    
    mst--;
    vst--;
  }

  // NE SCAN - scan top right half of image (as displayed)
  // -------------------------------------------------------
  mst = m0 + line4;
  vst = v0 + line4;
  sst = s0 + line;
  for (y = rh - 1; y > 0; y--)
  {
    m  = mst;             // where to write each sum
    v  = vst;
    s  = sst;             // leading edge pointer
    s2 = sst;             // trailing edge pointer
    lim  = __min(rw, y);  // how far before top or side 
    lim2 = lim - pd1;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd1 * (*s);    
    ssq = sum * (*s);
    i = 1;
    stop = __min(pd1, lim);
    while (i <= stop)
    {
      sum += *s;
      ssq += (*s) * (*s);
      s += step;
      i++;
    }
    if (pd1 > lim)        // if mask is bigger than ROI
    {
      s -= step;          // point to top border pixel
      val = (pd1 - lim) * (*s);  
      sum += val;
      ssq += val * (*s);
      s += step;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with y = nyp, even when dual overhang
    i = 1;
    stop = __min(nd1i, lim2);
    while (i < stop)
    {
      *m = sum;           // save previously computed running sum
      *v = ssq;
      sum -= *s2;         // subtract bottom border copy
      ssq -= (*s2) * (*s2);
      sum += *s;          // add leading edge value
      ssq += (*s) * (*s);
      s += step;          // advance leading edge but not trailing 
      m += step4;
      v += step4;
      i++;
    }
    if (nd1i > lim2)      // if big mask falls off both sides of ROI
    {
      s -= step;
      stop = __min(nd1i, lim);
      while (i < stop)
      {
        *m = sum;
        *v = ssq;
        sum -= *s2;       // subtract off copy of bottom border pixel
        ssq -= (*s2) * (*s2);
        sum += *s;        // add in copy of top border pixel
        ssq += (*s) * (*s);
        m += step4;
        v += step4;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      *m = sum;
      *v = ssq;
      sum -= *s2;         // subtract trailing, add leading, advance both
      ssq -= (*s2) * (*s2);
      sum += *s;
      ssq += (*s) * (*s);
      s  += step;
      s2 += step;
      m  += step4;
      v  += step4;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      *m = sum;
      *v = ssq;
      sum -= *s2;         // subtract trailing
      ssq -= (*s2) * (*s2);
      sum += *s;          // add copy of top border
      ssq += (*s) * (*s);
      s2 += step;         // advance only trailing pointer
      m  += step4;
      v  += step4;
      i++;
    }

    // shift starting point up by one diagonal for next loop
    sst += line;                    
    mst += line4;
    vst += line4;
  }


  // PASS 2 - MINOR DIAGONAL //// ================================
  // i increments aint diagonal scan line - up and right
  // start at lower left corner (as displayed) 
  a0 = avg.RoiDest();
  d0 = std.RoiDest();
  m0 = (UL32 *)(a4.RoiDest());
  v0 = (UL32 *)(b4.RoiDest());
  step  = line  + 1;
  step4 = line4 + 1;

  // SE SCAN - scan bottom right half of image (as displayed)
  // -------------------------------------------------------
  ast = a0;
  dst = d0;
  mst = m0;
  vst = v0;
  for (x = rw; x > 0; x--)
  {
    a  = ast;             // where to write each sum
    d  = dst;
    m  = mst;             // leading edge pointer
    v  = vst;
    m2 = mst;             // trailing edge pointer
    v2 = vst;
    lim  = __min(rh, x);  // how far before top or side 
    lim2 = lim - pd2;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd2 * (*m);    
    ssq = nd2 * (*v);  
    i = 1;
    stop = __min(pd2, lim);
    while (i <= stop)
    {
      sum += *m;
      ssq += *v;
      m += step4;
      v += step4;
      i++;
    }
    if (pd2 > lim)        // if mask is bigger than ROI
    {
      m -= step4;         // point to top border pixel
      v -= step4;
      sum += (pd2 - lim) * (*m);  
      ssq += (pd2 - lim) * (*v);  
      m += step4;
      v += step4;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd2i, even when dual overhang
    i = 1;
    stop = __min(nd2i, lim2);
    while (i < stop)            
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract bottom border copy
      ssq -= *v2;
      sum += *m;          // add leading edge value
      ssq += *v;
      m += step4;         // advance leading edge but not trailing 
      v += step4;
      a += step;
      d += step;
      i++;
    }
    if (nd2i > lim2)      // if big mask falls off both sides of ROI
    {
      m -= step4;
      v -= step4;
      stop = __min(nd2i, lim);
      while (i < stop)           
      {
        fval = area * (double) ssq - sum * (double) sum;
        val = (UL32)(nsc * sqrt(fval) + 0.5);
        val = __min(255, val);
        *d = (UC8) val;
        *a = (UC8)(norm * sum + 0.5);
        sum -= *m2;       // subtract off copy of bottom border pixel
        ssq -= *v2;
        sum += *m;        // add in copy of top border pixel
        ssq += *v;
        a += step;
        d += step;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract trailing, add leading, advance both
      ssq -= *v2;
      sum += *m;
      ssq += *v;
      m  += step4;
      v  += step4;
      m2 += step4;
      v2 += step4;
      a  += step;
      d  += step;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract trailing
      ssq -= *v2;
      sum += *m;          // add copy of top border
      ssq += *v;
      m2 += step4;        // advance only trailing pointer
      v2 += step4;
      a  += step;
      d  += step;
      i++;
    }

    // shift starting point over right by one diagonal for next loop
    mst++;                    
    vst++;                    
    ast++;
    dst++;
  }

  // NW SCAN - scan top left half of image (as displayed)
  // -------------------------------------------------------
  ast = a0 + line;
  dst = d0 + line;
  mst = m0 + line4;
  vst = v0 + line4;
  for (y = rh - 1; y > 0; y--)
  {
    a  = ast;             // where to write each sum
    d  = dst;
    m  = mst;             // leading edge pointer
    v  = vst;
    m2 = mst;             // trailing edge pointer
    v2 = vst;
    lim  = __min(rw, y);  // how far before top or side 
    lim2 = lim - pd2;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd2 * (*m);      
    ssq = nd2 * (*v);      
    i = 1;
    stop = __min(pd2, lim);
    while (i <= stop)
    {
      sum += *m;
      ssq += *v;
      m += step4;
      v += step4;
      i++;
    }
    if (pd2 > lim)        // if mask is bigger than ROI
    {
      m -= step4;         // point to top border pixel
      v -= step4;
      sum += (pd2 - lim) * (*m);  
      ssq += (pd2 - lim) * (*v);  
      m += step4;
      v += step4;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd2i, even when dual overhang
    i = 1;
    stop = __min(nd2i, lim2);
    while (i < stop)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract bottom border copy
      ssq -= *v2;
      sum += *m;          // add leading edge value
      ssq += *v;
      m += step4;         // advance leading edge but not trailing 
      v += step4;
      a += step;
      d += step;
      i++;
    }
    if (nd2i > lim2)      // if big mask falls off both sides of ROI
    {
      m -= step4;
      v -= step4;
      stop = __min(nd2i, lim);
      while (i < stop)
      {
        fval = area * (double) ssq - sum * (double) sum;
        val = (UL32)(nsc * sqrt(fval) + 0.5);
        val = __min(255, val);
        *d = (UC8) val;
        *a = (UC8)(norm * sum + 0.5);
        sum -= *m2;     // subtract off copy of bottom border pixel
        ssq -= *v2;
        sum += *m;      // add in copy of top border pixel
        ssq += *v;
        a += step;
        d += step;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract trailing, add leading, advance both
      ssq -= *v2;
      sum += *m;
      ssq += *v;
      m  += step4;
      v  += step4;
      m2 += step4;
      v2 += step4;
      a  += step;
      d  += step;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc * sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract trailing
      ssq -= *v2;
      sum += *m;          // add copy of top border
      ssq += *v;
      m2 += step4;        // advance only trailing pointer
      v2 += step4;
      a  += step;
      d  += step;
      i++;
    }

    // shift starting point up by one diagonal for next loop
    mst += line4;                    
    vst += line4;                    
    ast += line;
    dst += line;
  }

  return 1;
}


//= Like DBoxAvgStd but returns inverse of standard deviation to allow.
// normalization by multiplication (isd = __min(255, 256 / (dsc * std)))
// Note: does pixel squaring twice

int jhcArea::DBoxAvgInv (jhcImg &avg, jhcImg& std, const jhcImg& src, 
                         int w1, int h2, double dsc)
{
  int d1z = w1, d2z = ((h2 == 0) ? w1 : h2), area = d1z * d2z;
  int sz = __min(src.RoiW(), src.RoiH());

  // check arguments 
  if (!src.Valid(1) || !src.SameFormat(avg) || !src.SameFormat(std))
    return Fatal("Bad images to jhcArea::DBoxAvgInv");
  if ((d1z >= sz) || (d2z >= sz))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::DBoxAvgInv", 
                 d1z, d2z, src.RoiW(), src.RoiH());

  // special cases
  if ((d1z <= 0) || (d2z <= 0) || (dsc <= 0.0))
    return 0;
  if (area > 66051)  // 32 bit overflow beyond (2^32 - 1) / (255^2)
    return 0;
  avg.CopyRoi(src);
  std.CopyRoi(src);
  if ((area == 1) && (dsc == 1.0))
  {
    std.FillArr(0);
    return avg.CopyArr(src);
  }
  a4.SetSize(src, 4);
  b4.SetSize(src, 4);

  // generic ROI case
  int i, x, y, step, step4, lim, lim2, stop;
  int rw = src.RoiW(), rh = src.RoiH();
  int line = src.Line(), line4 = a4.Line() >> 2;  // line4 = a4.XDim();
  int nd1 = d1z / 2, pd1 = d1z - nd1, nd1i = nd1 + 1;
  int nd2 = d2z / 2, pd2 = d2z - nd2, nd2i = nd2 + 1;
  double fval, norm = 1.0 / (double) area, nsc = 256.0 * area / dsc;
  UL32 sum, ssq, val;
  const UC8 *s, *s2, *sst, *s0;
  UC8 *a, *ast, *a0, *d, *dst, *d0;
  UL32 *m, *m2, *mst, *m0, *v, *v2, *vst, *v0;


  // PASS 1 - MAJOR DIAGONAL \\\\ ================================
  // i increments aint diagonal scan line - up and left
  // start at lower right corner (as displayed) 
  m0 = (UL32 *)(a4.RoiDest()) + rw - 1;
  v0 = (UL32 *)(b4.RoiDest()) + rw - 1;
  s0 = src.RoiSrc()  + rw - 1;
  step  = line  - 1;
  step4 = line4 - 1;

  // SW SCAN - scan bottom left half of image (as displayed)
  // -------------------------------------------------------
  mst = m0;
  vst = v0;
  sst = s0;
  for (x = rw; x > 0; x--)
  {
    m  = mst;             // where to write each sum
    v  = vst;
    s  = sst;             // leading edge pointer
    s2 = sst;             // trailing edge pointer
    lim  = __min(rh, x);  // how far before top or side 
    lim2 = lim - pd1;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd1 * (*s);    
    ssq = sum * (*s);  
    i = 1;
    stop = __min(pd1, lim);
    while (i <= stop)
    {
      sum += *s;
      ssq += (*s) * (*s);
      s += step;
      i++;
    }
    if (pd1 > lim)        // if mask is bigger than ROI
    {
      s -= step;          // point to top border pixel
      val = (pd1 - lim) * (*s);  
      sum += val;
      ssq += val * (*s);
      s += step;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd1i, even when dual overhang
    i = 1;
    stop = __min(nd1i, lim2);
    while (i < stop)            
    {
      *m = sum;           // save previously computed running sum
      *v = ssq;
      sum -= *s2;         // subtract bottom border copy
      ssq -= (*s2) * (*s2);
      sum += *s;          // add leading edge value
      ssq += (*s) * (*s);
      s += step;          // advance leading edge but not trailing 
      m += step4;
      v += step4;
      i++;
    }
    if (nd1i > lim2)      // if big mask falls off both sides of ROI
    {
      s -= step;
      stop = __min(nd1i, lim);
      while (i < stop)           
      {
        *m = sum;
        *v = ssq;
        sum -= *s2;       // subtract off copy of bottom border pixel
        ssq -= (*s2) * (*s2);
        sum += *s;        // add in copy of top border pixel
        ssq += (*s) * (*s);
        m += step4;
        v += step4;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      *m = sum;
      *v = ssq;
      sum -= *s2;         // subtract trailing, add leading, advance both
      ssq -= (*s2) * (*s2);
      sum += *s;
      ssq += (*s) * (*s);
      s  += step;
      s2 += step;
      m  += step4;
      v  += step4;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      *m = sum;
      *v = ssq;
      sum -= *s2;         // subtract trailing
      ssq -= (*s2) * (*s2);
      sum += *s;          // add copy of top border
      ssq += (*s) * (*s);
      s2 += step;         // advance only trailing pointer
      m  += step4;
      v  += step4;
      i++;
    }

    // shift starting point over left by one diagonal for next loop
    sst--;                    
    mst--;
    vst--;
  }

  // NE SCAN - scan top right half of image (as displayed)
  // -------------------------------------------------------
  mst = m0 + line4;
  vst = v0 + line4;
  sst = s0 + line;
  for (y = rh - 1; y > 0; y--)
  {
    m  = mst;             // where to write each sum
    v  = vst;
    s  = sst;             // leading edge pointer
    s2 = sst;             // trailing edge pointer
    lim  = __min(rw, y);  // how far before top or side 
    lim2 = lim - pd1;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd1 * (*s);    
    ssq = sum * (*s);
    i = 1;
    stop = __min(pd1, lim);
    while (i <= stop)
    {
      sum += *s;
      ssq += (*s) * (*s);
      s += step;
      i++;
    }
    if (pd1 > lim)        // if mask is bigger than ROI
    {
      s -= step;          // point to top border pixel
      val = (pd1 - lim) * (*s);  
      sum += val;
      ssq += val * (*s);
      s += step;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with y = nyp, even when dual overhang
    i = 1;
    stop = __min(nd1i, lim2);
    while (i < stop)
    {
      *m = sum;           // save previously computed running sum
      *v = ssq;
      sum -= *s2;         // subtract bottom border copy
      ssq -= (*s2) * (*s2);
      sum += *s;          // add leading edge value
      ssq += (*s) * (*s);
      s += step;          // advance leading edge but not trailing 
      m += step4;
      v += step4;
      i++;
    }
    if (nd1i > lim2)      // if big mask falls off both sides of ROI
    {
      s -= step;
      stop = __min(nd1i, lim);
      while (i < stop)
      {
        *m = sum;
        *v = ssq;
        sum -= *s2;       // subtract off copy of bottom border pixel
        ssq -= (*s2) * (*s2);
        sum += *s;        // add in copy of top border pixel
        ssq += (*s) * (*s);
        m += step4;
        v += step4;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      *m = sum;
      *v = ssq;
      sum -= *s2;         // subtract trailing, add leading, advance both
      ssq -= (*s2) * (*s2);
      sum += *s;
      ssq += (*s) * (*s);
      s  += step;
      s2 += step;
      m  += step4;
      v  += step4;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      *m = sum;
      *v = ssq;
      sum -= *s2;         // subtract trailing
      ssq -= (*s2) * (*s2);
      sum += *s;          // add copy of top border
      ssq += (*s) * (*s);
      s2 += step;         // advance only trailing pointer
      m  += step4;
      v  += step4;
      i++;
    }

    // shift starting point up by one diagonal for next loop
    sst += line;                    
    mst += line4;
    vst += line4;
  }


  // PASS 2 - MINOR DIAGONAL //// ================================
  // i increments aint diagonal scan line - up and right
  // start at lower left corner (as displayed) 
  a0 = avg.RoiDest();
  d0 = std.RoiDest();
  m0 = (UL32 *)(a4.RoiDest());
  v0 = (UL32 *)(b4.RoiDest());
  step  = line  + 1;
  step4 = line4 + 1;

  // SE SCAN - scan bottom right half of image (as displayed)
  // -------------------------------------------------------
  ast = a0;
  dst = d0;
  mst = m0;
  vst = v0;
  for (x = rw; x > 0; x--)
  {
    a  = ast;             // where to write each sum
    d  = dst;
    m  = mst;             // leading edge pointer
    v  = vst;
    m2 = mst;             // trailing edge pointer
    v2 = vst;
    lim  = __min(rh, x);  // how far before top or side 
    lim2 = lim - pd2;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd2 * (*m);    
    ssq = nd2 * (*v);  
    i = 1;
    stop = __min(pd2, lim);
    while (i <= stop)
    {
      sum += *m;
      ssq += *v;
      m += step4;
      v += step4;
      i++;
    }
    if (pd2 > lim)        // if mask is bigger than ROI
    {
      m -= step4;         // point to top border pixel
      v -= step4;
      sum += (pd2 - lim) * (*m);  
      ssq += (pd2 - lim) * (*v);  
      m += step4;
      v += step4;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd2i, even when dual overhang
    i = 1;
    stop = __min(nd2i, lim2);
    while (i < stop)            
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc / sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract bottom border copy
      ssq -= *v2;
      sum += *m;          // add leading edge value
      ssq += *v;
      m += step4;         // advance leading edge but not trailing 
      v += step4;
      a += step;
      d += step;
      i++;
    }
    if (nd2i > lim2)      // if big mask falls off both sides of ROI
    {
      m -= step4;
      v -= step4;
      stop = __min(nd2i, lim);
      while (i < stop)           
      {
        fval = area * (double) ssq - sum * (double) sum;
        val = (UL32)(nsc / sqrt(fval) + 0.5);
        val = __min(255, val);
        *d = (UC8) val;
        *a = (UC8)(norm * sum + 0.5);
        sum -= *m2;       // subtract off copy of bottom border pixel
        ssq -= *v2;
        sum += *m;        // add in copy of top border pixel
        ssq += *v;
        a += step;
        d += step;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc / sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract trailing, add leading, advance both
      ssq -= *v2;
      sum += *m;
      ssq += *v;
      m  += step4;
      v  += step4;
      m2 += step4;
      v2 += step4;
      a  += step;
      d  += step;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc / sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract trailing
      ssq -= *v2;
      sum += *m;          // add copy of top border
      ssq += *v;
      m2 += step4;        // advance only trailing pointer
      v2 += step4;
      a  += step;
      d  += step;
      i++;
    }

    // shift starting point over right by one diagonal for next loop
    mst++;                    
    vst++;                    
    ast++;
    dst++;
  }

  // NW SCAN - scan top left half of image (as displayed)
  // -------------------------------------------------------
  ast = a0 + line;
  dst = d0 + line;
  mst = m0 + line4;
  vst = v0 + line4;
  for (y = rh - 1; y > 0; y--)
  {
    a  = ast;             // where to write each sum
    d  = dst;
    m  = mst;             // leading edge pointer
    v  = vst;
    m2 = mst;             // trailing edge pointer
    v2 = vst;
    lim  = __min(rw, y);  // how far before top or side 
    lim2 = lim - pd2;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd2 * (*m);      
    ssq = nd2 * (*v);      
    i = 1;
    stop = __min(pd2, lim);
    while (i <= stop)
    {
      sum += *m;
      ssq += *v;
      m += step4;
      v += step4;
      i++;
    }
    if (pd2 > lim)        // if mask is bigger than ROI
    {
      m -= step4;         // point to top border pixel
      v -= step4;
      sum += (pd2 - lim) * (*m);  
      ssq += (pd2 - lim) * (*v);  
      m += step4;
      v += step4;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd2i, even when dual overhang
    i = 1;
    stop = __min(nd2i, lim2);
    while (i < stop)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc / sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract bottom border copy
      ssq -= *v2;
      sum += *m;          // add leading edge value
      ssq += *v;
      m += step4;         // advance leading edge but not trailing 
      v += step4;
      a += step;
      d += step;
      i++;
    }
    if (nd2i > lim2)      // if big mask falls off both sides of ROI
    {
      m -= step4;
      v -= step4;
      stop = __min(nd2i, lim);
      while (i < stop)
      {
        fval = area * (double) ssq - sum * (double) sum;
        val = (UL32)(nsc / sqrt(fval) + 0.5);
        val = __min(255, val);
        *d = (UC8) val;
        *a = (UC8)(norm * sum + 0.5);
        sum -= *m2;     // subtract off copy of bottom border pixel
        ssq -= *v2;
        sum += *m;      // add in copy of top border pixel
        ssq += *v;
        a += step;
        d += step;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc / sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract trailing, add leading, advance both
      ssq -= *v2;
      sum += *m;
      ssq += *v;
      m  += step4;
      v  += step4;
      m2 += step4;
      v2 += step4;
      a  += step;
      d  += step;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      fval = area * (double) ssq - sum * (double) sum;
      val = (UL32)(nsc / sqrt(fval) + 0.5);
      val = __min(255, val);
      *d = (UC8) val;
      *a = (UC8)(norm * sum + 0.5);
      sum -= *m2;         // subtract trailing
      ssq -= *v2;
      sum += *m;          // add copy of top border
      ssq += *v;
      m2 += step4;        // advance only trailing pointer
      v2 += step4;
      a  += step;
      d  += step;
      i++;
    }

    // shift starting point up by one diagonal for next loop
    mst += line4;                    
    vst += line4;                    
    ast += line;
    dst += line;
  }

  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Two Byte Value Versions                         //
///////////////////////////////////////////////////////////////////////////

//= Return averages in a rectangular box around each pixel.
// pixel values (and averages) assumed to be 16 bits

int jhcArea::BoxAvg16 (jhcImg &dest, const jhcImg& src, int wid, int ht, double sc)
{
  int dx = wid, dy = ((ht == 0) ? wid : ht), area = dx * dy;

  // check arguments
  if (!dest.Valid(2) && !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::BoxAvg16");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxAvg16", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0) || (sc <= 0.0))
    return 0;
  if (area > 65537)  // limit for 32 bit area sum using 16 bit values
    return 0;
  if ((area == 1) && (sc == 1.0))
    return dest.CopyArr(src);
  dest.CopyRoi(src);

  // generic ROI case
  int x, y, wx, rsk4;
  int rw = dest.RoiW(), rh = dest.RoiH(), rsk2 = dest.RoiSkip() >> 1;
  int nx = dx / 2, px = dx - nx, xlim = rw - px - 1;
  int ny = dy / 2, py = dy - ny, ylim = rh - py, nyp = ny + 1;
  unsigned __int64 norm = (unsigned __int64)(0x01000000 * sc / area);
  UL32 sum, ej, val, roff4, line4 = dest.XDim();
  const US16 *a0, *ahi, *alo, *aej;
  US16 *a;
  UL32 *b, *b0, *blo, *bhi, *bsum;
  jhcImg *temp = a4.SetSize(dest, 4);

  // set up pointer to start of region of interest in victim
  dest.RoiParams(&roff4, &rsk4, 4);
  rsk4 >>= 2;
  b0 = (UL32 *)(temp->PxlDest() + roff4);


  // PASS 1 vertical ==============================================
  // vertically smear from source into temp using running sum
  // keep rw running sums at the same time to preserve scan order
  a0 = (const US16 *)(src.PxlSrc() + src.RoiOff());

  // initialize running sums (in bsum), one per column in ROI
  // duplicate values at edge to handle mask overhang
  // this is exactly the answer for top row of PxlSrc()
  bsum = b0;
  ahi  = a0;
  for (x = rw; x > 0; x--)
    *bsum++ = nyp * (*ahi++);
  ahi += rsk2;
  for (y = 1; y < py; y++)
  {
    bsum =  b0;
    for (x = rw; x > 0; x--)
      *bsum++ += *ahi++;
    ahi += rsk2;
  }

  // mask falls off top initially
  // take previous running sum (row above) and adjust by adding
  // source pixel from lower line and subtracting source pixel at top
  // nyp incremented since sum is updated BEFORE writing out, not after
  nyp++;
  bsum = b0;
  b    = b0 + line4;
  for (y = 1; y < nyp; y++)
  {
    alo = a0;
    for (x = rw; x > 0; x--)
      *b++ = *bsum++ + *ahi++ - *alo++;
    b    += rsk4;
    bsum += rsk4;
    ahi  += rsk2;
  }
  alo += rsk2;

  // mask is fully inside for most
  // get previous running sum, add bottom, subtract top
  for (y = nyp; y < ylim; y++)
  {
    for (x = rw; x > 0; x--)
      *b++ = *bsum++ + *ahi++ - *alo++;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk2;
    ahi  += rsk2;
  }

  // mask falls off bottom at end
  // get previous running sum, add edge, subtract top
  // for first center the add is legimately the last pixel
  aej = ahi;
  for (y = ylim; y < rh; y++)
  {
    for (x = rw; x > 0; x--)
      *b++ = *bsum++ + *ahi++ - *alo++;
    b    += rsk4;
    bsum += rsk4;
    alo  += rsk2;
    ahi  =  aej;
  }


  // PASS 2 horizontal =========================================
  // horizontal smear from temp into local using running sum
  // adds in new right side, subtracts off old left side
  b = b0;
  a = (US16 *)(dest.PxlDest() + dest.RoiOff());
  for (y = rh; y > 0; y--)
  {
    // initialize running sum 
    sum = 0;
    for (x = 0; x < dx; x++)
    {
      wx = x - nx;
      sum += *(b + __max(0, wx));
    }

    // mask falls off left initially
    ej = *b;
    bhi = b + px;
    for (x = 0; x < nx; x++)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(65535, val);
      *a++ = (US16) val;
      sum -= ej;
      sum += *bhi++;
    }

    // mask is fully inside for most
    blo = b;
    for (x = nx; x < xlim; x++)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(65535, val);
      *a++ = (US16) val;
      sum -= *blo++;
      sum += *bhi++;
    }

    // mask falls off right at end
    ej = *bhi;
    for (x = xlim; x < rw; x++)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(65535, val);
      *a++ = (US16) val;
      sum -= *blo++;
      sum += ej;
    }

    // move down to next line
    a += rsk2;
    b += line4;
  }
  return 1;
}


//= Return averages in a diamond-shaped box around each pixel.
// pixel values (and averages) assumed to be 16 bits

int jhcArea::DBoxAvg16 (jhcImg &dest, const jhcImg& src, int w1, int h2, double sc)
{
  int d1z = w1, d2z = ((h2 == 0) ? w1 : h2), area = d1z * d2z;
  int sz = __min(src.RoiW(), src.RoiH());

  // check arguments
  if (!dest.Valid(2) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::DBoxAvg16");
  if ((d1z >= sz) || (d2z >= sz))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::DBoxAvg16", 
                 d1z, d2z, src.RoiW(), src.RoiH());

  // special cases
  if ((d1z <= 0) || (d2z <= 0) || (sc <= 0.0))
    return 0;
  if (area > 65537)  // limit for 32 bit area sum using 16 bit values
    return 0;
  if ((area == 1) && (sc == 1.0))
    return dest.CopyArr(src);
  dest.CopyRoi(src);
  a4.SetSize(dest, 4);

  // generic ROI case
  int i, x, y, step2, step4, lim, lim2, stop;
  int rw = dest.RoiW(), rh = dest.RoiH();
  int line2 = dest.Line() >> 1, line4 = a4.Line() >> 2;  // line4 = a4.XDim();
  int nd1 = d1z / 2, pd1 = d1z - nd1, nd1i = nd1 + 1;
  int nd2 = d2z / 2, pd2 = d2z - nd2, nd2i = nd2 + 1;
  unsigned __int64 norm = (unsigned __int64)(0x01000000 * sc / area);
  UL32 sum, val;
  const US16 *s, *s2, *sst, *s0;
  US16 *d, *dst, *d0;
  UL32 *m, *m2, *mst, *m0;

  // PASS 1 - MAJOR DIAGONAL \\\\ ================================
  // i increments aint diagonal scan line - up and left
  // start at lower right corner (as displayed) 
  m0 = (UL32 *)(a4.RoiDest()) + rw - 1;
  s0 = (const US16 *)(src.RoiSrc()) + rw - 1;
  step2 = line2 - 1;
  step4 = line4 - 1;

  // SW SCAN - scan bottom left half of image (as displayed)
  // -------------------------------------------------------
  mst = m0;
  sst = s0;
  for (x = rw; x > 0; x--)
  {
    m  = mst;             // where to write each sum
    s  = sst;             // leading edge pointer
    s2 = sst;             // trailing edge pointer
    lim  = __min(rh, x);  // how far before top or side 
    lim2 = lim - pd1;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd1 * (*s);      
    i = 1;
    stop = __min(pd1, lim);
    while (i <= stop)
    {
      sum += *s;
      s += step2;
      i++;
    }
    if (pd1 > lim)        // if mask is bigger than ROI
    {
      s -= step2;         // point to top border pixel
      sum += (pd1 - lim) * (*s);  
      s += step2;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd1i, even when dual overhang
    i = 1;
    stop = __min(nd1i, lim2);
    while (i < stop)            
    {
      *m = sum;           // save previously computed running sum
      sum -= *s2;         // subtract bottom border copy
      sum += *s;          // add leading edge value
      s += step2;         // advance leading edge but not trailing 
      m += step4;
      i++;
    }
    if (nd1i > lim2)      // if big mask falls off both sides of ROI
    {
      s -= step2;
      stop = __min(nd1i, lim);
      while (i < stop)           
      {
        *m = sum;
        sum -= *s2;       // subtract off copy of bottom border pixel
        sum += *s;        // add in copy of top border pixel
        m += step4;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      *m = sum;
      sum -= *s2;         // subtract trailing, add leading, advance both
      sum += *s;
      s  += step2;
      s2 += step2;
      m  += step4;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      *m = sum;
      sum -= *s2;         // subtract trailing
      sum += *s;          // add copy of top border
      s2 += step2;        // advance only trailing pointer
      m  += step4;
      i++;
    }

    // shift starting point over left by one diagonal for next loop
    sst--;                    
    mst--;
  }

  // NE SCAN - scan top right half of image (as displayed)
  // -------------------------------------------------------
  mst = m0 + line4;
  sst = s0 + line2;
  for (y = rh - 1; y > 0; y--)
  {
    m  = mst;             // where to write each sum
    s  = sst;             // leading edge pointer
    s2 = sst;             // trailing edge pointer
    lim  = __min(rw, y);  // how far before top or side 
    lim2 = lim - pd1;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd1 * (*s);      
    i = 1;
    stop = __min(pd1, lim);
    while (i <= stop)
    {
      sum += *s;
      s += step2;
      i++;
    }
    if (pd1 > lim)        // if mask is bigger than ROI
    {
      s -= step2;         // point to top border pixel
      sum += (pd1 - lim) * (*s);  
      s += step2;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with y = nyp, even when dual overhang
    i = 1;
    stop = __min(nd1i, lim2);
    while (i < stop)
    {
      *m = sum;           // save previously computed running sum
      sum -= *s2;         // subtract bottom border copy
      sum += *s;          // add leading edge value
      s += step2;         // advance leading edge but not trailing 
      m += step4;
      i++;
    }
    if (nd1i > lim2)      // if big mask falls off both sides of ROI
    {
      s -= step2;
      stop = __min(nd1i, lim);
      while (i < stop)
      {
        *m = sum;
        sum -= *s2;       // subtract off copy of bottom border pixel
        sum += *s;        // add in copy of top border pixel
        m += step4;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      *m = sum;
      sum -= *s2;         // subtract trailing, add leading, advance both
      sum += *s;
      s  += step2;
      s2 += step2;
      m  += step4;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      *m = sum;
      sum -= *s2;         // subtract trailing
      sum += *s;          // add copy of top border
      s2 += step2;        // advance only trailing pointer
      m  += step4;
      i++;
    }

    // shift starting point up by one diagonal for next loop
    sst += line2;                    
    mst += line4;
  }


  // PASS 2 - MINOR DIAGONAL //// ================================
  // i increments aint diagonal scan line - up and right
  // start at lower left corner (as displayed) 
  d0 = (US16 *)(dest.RoiDest());
  m0 = (UL32 *)(a4.RoiDest());
  step2 = line2 + 1;
  step4 = line4 + 1;

  // SE SCAN - scan bottom right half of image (as displayed)
  // -------------------------------------------------------
  dst = d0;
  mst = m0;
  for (x = rw; x > 0; x--)
  {
    d  = dst;             // where to write each sum
    m  = mst;             // leading edge pointer
    m2 = mst;             // trailing edge pointer
    lim  = __min(rh, x);  // how far before top or side 
    lim2 = lim - pd2;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd2 * (*m);      
    i = 1;
    stop = __min(pd2, lim);
    while (i <= stop)
    {
      sum += (*m);
      m += step4;
      i++;
    }
    if (pd2 > lim)        // if mask is bigger than ROI
    {
      m -= step4;         // point to top border pixel
      sum += (pd2 - lim) * (*m);  
      m += step4;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd2i, even when dual overhang
    i = 1;
    stop = __min(nd2i, lim2);
    while (i < stop)            
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(65535, val);
      *d  = (US16) val;
      sum -= (*m2);       // subtract bottom border copy
      sum += (*m);        // add leading edge value
      m += step4;         // advance leading edge but not trailing 
      d += step2;
      i++;
    }
    if (nd2i > lim2)      // if big mask falls off both sides of ROI
    {
      m -= step4;
      stop = __min(nd2i, lim);
      while (i < stop)           
      {
        val = (UL32)((sum * norm) >> 24);
        val = __min(65535, val);
        *d  = (US16) val;
        sum -= (*m2);     // subtract off copy of bottom border pixel
        sum += (*m);      // add in copy of top border pixel
        d += step2;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(65535, val);
      *d  = (US16) val;
      sum -= (*m2);       // subtract trailing, add leading, advance both
      sum += (*m);
      m  += step4;
      m2 += step4;
      d  += step2;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(65535, val);
      *d  = (US16) val;
      sum -= (*m2);       // subtract trailing
      sum += (*m);        // add copy of top border
      m2 += step4;        // advance only trailing pointer
      d  += step2;
      i++;
    }

    // shift starting point over right by one diagonal for next loop
    mst++;                    
    dst++;
  }

  // NW SCAN - scan top left half of image (as displayed)
  // -------------------------------------------------------
  dst = d0 + line2;
  mst = m0 + line4;
  for (y = rh - 1; y > 0; y--)
  {
    d  = dst;             // where to write each sum
    m  = mst;             // leading edge pointer
    m2 = mst;             // trailing edge pointer
    lim  = __min(rw, y);  // how far before top or side 
    lim2 = lim - pd2;     // how far to mask overhang

    // setup initial value, duplicating for bottom overhang
    sum = nd2 * (*m);      
    i = 1;
    stop = __min(pd2, lim);
    while (i <= stop)
    {
      sum += (*m);
      m += step4;
      i++;
    }
    if (pd2 > lim)        // if mask is bigger than ROI
    {
      m -= step4;         // point to top border pixel
      sum += (pd2 - lim) * (*m);  
      m += step4;
    }

    // start scan - mask always falls off bottom at beginning
    // loop always stops with i = nd2i, even when dual overhang
    i = 1;
    stop = __min(nd2i, lim2);
    while (i < stop)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(65535, val);
      *d  = (US16) val;
      sum -= (*m2);       // subtract bottom border copy
      sum += (*m);        // add leading edge value
      m += step4;         // advance leading edge but not trailing 
      d += step2;
      i++;
    }
    if (nd2i > lim2)      // if big mask falls off both sides of ROI
    {
      m -= step4;
      stop = __min(nd2i, lim);
      while (i < stop)
      {
        val = (UL32)((sum * norm) >> 24);
        val = __min(65535, val);
        *d  = (US16) val;
        sum -= (*m2);     // subtract off copy of bottom border pixel
        sum += (*m);      // add in copy of top border pixel
        d += step2;
        i++;
      }
    }

    // continue - mask completely inside ROI for the majority of PxlSrc()
    // loop skipped if already beyond i = lim2 (e.g. dual overhang)
    while (i < lim2)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(65535, val);
      *d  = (US16) val;
      sum -= (*m2);       // subtract trailing, add leading, advance both
      sum += (*m);
      m  += step4;
      m2 += step4;
      d  += step2;
      i++;
    }

    // finish scan - mask is overhanging top or side now
    while (i <= lim)
    {
      val = (UL32)((sum * norm) >> 24);
      val = __min(65535, val);
      *d  = (US16) val;
      sum -= (*m2);       // subtract trailing
      sum += (*m);        // add copy of top border
      m2 += step4;        // advance only trailing pointer
      d  += step2;
      i++;
    }

    // shift starting point up by one diagonal for next loop
    mst += line4;                    
    dst += line2;
  }

  return 1;
}


//= Mutated version of standard threshold function.
// can do thresholding in-place, uses under value if less than th

void jhcArea::thresh (jhcImg& dest, const jhcImg& src, int th, int over, int under) const
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


///////////////////////////////////////////////////////////////////////////
//                           Max and Min Sweeps                          //
///////////////////////////////////////////////////////////////////////////

//= Finds the maximum value in a rectangular-shaped region.
// handles big masks more carefully than BoxAvg does

int jhcArea::BoxMax (jhcImg& dest, const jhcImg& src, int wid, int ht)
{
  int dx = wid, dy = ((ht == 0) ? wid : ht);

  // check arguments 
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::BoxMax");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxMax", 
                 dx, dy, src.RoiW(), src.RoiH());
   
  // special cases
  if ((dx <= 0) || (dy <= 0))
    return dest.FillArr(0);
  if ((dx == 1) && (dy == 1))
    return dest.CopyArr(src);
  dest.CopyRoi(src);
  if ((dx > 1) && (dy > 1))
  {
    a1.SetSize(dest);
    a1.CopyRoi(src);
  }

  // generic ROI case
  int x, y, i, ret, val;
  int rw = dest.RoiW(), rh = dest.RoiH(), ln = dest.Line();
  int nx = dx / 2, px = dx - nx, xdel = __min(nx + 1, rw), xadd = __max(0, rw - px);
  int xini = __min(px, rw), xlo = __min(xdel, xadd), xhi = __max(xdel, xadd);
  int ny = dy / 2, py = dy - ny, ydel = __min(ny + 1, rh), yadd = __max(0, rh - py);
  int yini = __min(py, rh), ylo = __min(ydel, yadd), yhi = __max(ydel, yadd), dyln = dy * ln;
  const UC8 *b, *f, *s0;
  UC8 *m, *m0;

  // PASS 1 vertical ==============================================
  // find maximum in vertical strip of height dy = [-ny 0 +py]
  // subtract trailing edge starting at ydel, add leading edge up to yadd 
  // y refers to location of output pixel in image a1 (m)
  if (dy > 1)
  {
    s0 = src.RoiSrc();
    if (dx > 1)
      m0 = a1.RoiDest();
    else
      m0 = dest.RoiDest();
    for (x = 0; x < rw; x++, m0++, s0++)
    {
      // start all pointers at base of column
      m = m0;
      b = s0;
      f = s0;

      // initialize value based on positive overlap of mask
      val = 0;
      for (i = 0; i < yini; i++, f += ln)
        if (*f > val)
          val = *f;

      // at beginning mask is not completely in image
      for (y = 0; y < ylo; y++, m += ln, f += ln)
      {
        *m = (UC8) val;
        if (*f >= val)
          val = *f;
      }

      if (yadd < ydel)
        // huge masks span whole image in middle
        for (y = ylo; y < yhi; y++, m += ln)
          *m = (UC8) val;
      else    
        // in middle look at leading and trailing edge of mask
        for (y = ylo; y < yhi; y++, m += ln, b += ln, f += ln)
        {
          *m = (UC8) val;

          // if leading edge is greater >= max then just set max 
          if (*f >= val)
            val = *f;
          else if ((val > 0) && (*b == val))
          {
            // if trailing edge was max, rescan for new max in mask
            val = 0;
            f -= dyln;
            for (i = 0; i < dy; i++, f += ln)
              if (*f > val)
                val = *f;
          }
        }

      // at end mask falls off image
      for (y = yhi; y < rh; y++, m += ln, b += ln)
      {
        *m = (UC8) val;

        if ((val > 0) && (*b == val))
        {
          // if trailing edge was max, rescan for new max in mask
          val = 0;
          ret = __min(dy, rh - (y - ny));
          f -= (ret * ln);
          for (i = 0; i < ret; i++, f += ln)
            if (*f > val)
              val = *f;
        }
      }
    }
  }

  // PASS 2 horizontal ============================================
  // find maximum in vertical strip of height dx = [-nx 0 +px]
  // subtract trailing edge starting at xdel, add leading edge up to xadd 
  // x refers to location of output pixel in image dest (m)
  if (dx > 1)
  {
    if (dy > 1)
      s0 = a1.RoiSrc();
    else
      s0 = src.RoiSrc();
    m0 = dest.RoiDest();
    for (y = 0; y < rh; y++, m0 += ln, s0 += ln)
    {
      // start all pointers at base of column
      m = m0;
      b = s0;
      f = s0;

      // initialize value based on positive overlap of mask
      val = 0;
      for (i = 0; i < xini; i++, f++)
        if (*f > val)
          val = *f;

      // at beginning mask is not completely in image
      for (x = 0; x < xlo; x++, m++, f++)
      {
        *m = (UC8) val;
        if (*f >= val)
          val = *f;
      }

      if (xadd < xdel)
        // huge masks span whole image in middle
        for (x = xlo; x < xhi; x++, m++)
          *m = (UC8) val;
      else    
        // in middle look at leading and trailing edge of mask
        for (x = xlo; x < xhi; x++, m++, b++, f++)
        {
          *m = (UC8) val;

          // if leading edge is greater >= max then just set max 
          if (*f >= val)
            val = *f;
          else if ((val > 0) && (*b == val))
          {
            // if trailing edge was max, rescan for new max in mask
            val = 0;
            f -= dx;
            for (i = 0; i < dx; i++, f++)
              if (*f > val)
                val = *f;
          }
        }

      // at end mask falls off image
      for (x = xhi; x < rw; x++, m++, b++)
      {
        *m = (UC8) val;

        if ((val > 0) && (*b == val))
        {
          // if trailing edge was max, rescan for new max in mask
          val = 0;
          ret = __min(dx, rw - (x - nx));
          f -= ret;
          for (i = 0; i < ret; i++, f++)
            if (*f > val)
              val = *f;
        }
      }
    }
  }
  return 1;
}


//= Finds the minimum non-zero value in a rectangular-shaped region.
// sets output to zero if mask does not overlap any non-zero pixels

int jhcArea::BoxMin (jhcImg& dest, const jhcImg& src, int wid, int ht)
{
  int dx = wid, dy = ((ht == 0) ? wid : ht);

  // check arguments
  if (!dest.Valid(1) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::BoxMin");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxMin", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0))
    return dest.FillArr(0);
  if ((dx == 1) && (dy == 1))
    return dest.CopyArr(src);
  dest.CopyRoi(src);
  if ((dx > 1) && (dy > 1))
  {
    a1.SetSize(dest);
    a1.CopyRoi(src);
  }

  // generic ROI case
  int x, y, i, ret, val;
  int rw = dest.RoiW(), rh = dest.RoiH(), ln = dest.Line();
  int nx = dx / 2, px = dx - nx, xdel = __min(nx + 1, rw), xadd = __max(0, rw - px);
  int xini = __min(px, rw), xlo = __min(xdel, xadd), xhi = __max(xdel, xadd);
  int ny = dy / 2, py = dy - ny, ydel = __min(ny + 1, rh), yadd = __max(0, rh - py);
  int yini = __min(py, rh), ylo = __min(ydel, yadd), yhi = __max(ydel, yadd), dyln = dy * ln;
  const UC8 *b, *f, *s0;
  UC8 *m, *m0;

  // PASS 1 vertical ==============================================
  // find minimum in vertical strip of height dy = [-ny 0 +py]
  // subtract trailing edge starting at ydel, add leading edge up to yadd 
  // y refers to location of output pixel in image a1 (m)
  if (dy > 1)
  {
    s0 = src.RoiSrc();
    if (dx > 1)
      m0 = a1.RoiDest();
    else
      m0 = dest.RoiDest();
    for (x = 0; x < rw; x++, m0++, s0++)
    {
      // start all pointers at base of column
      m = m0;
      b = s0;
      f = s0;

      // initialize value based on positive overlap of mask
      val = 0;
      for (i = 0; i < yini; i++, f += ln)
        if ((*f > 0) && ((*f < val) || (val <= 0)))
          val = *f;

      // at beginning mask is not completely in image
      for (y = 0; y < ylo; y++, m += ln, f += ln)
      {
        *m = (UC8) val;
        if ((*f > 0) && ((*f < val) || (val <= 0)))
          val = *f;
      }

      if (yadd < ydel)
        // huge masks span whole image in middle
        for (y = ylo; y < yhi; y++, m += ln)
          *m = (UC8) val;
      else    
        // in middle look at leading and trailing edge of mask
        for (y = ylo; y < yhi; y++, m += ln, b += ln, f += ln)
        {
          *m = (UC8) val;

          // if leading edge is greater >= max then just set max 
          if ((*f > 0) && ((*f < val) || (val <= 0)))
            val = *f;
          else if ((val > 0) && (*b == val))
          {
            // if trailing edge was max, rescan for new max in mask
            val = 0;
            f -= dyln;
            for (i = 0; i < dy; i++, f += ln)
              if ((*f > 0) && ((*f < val) || (val <= 0)))
                val = *f;
          }
        }

      // at end mask falls off image
      for (y = yhi; y < rh; y++, m += ln, b += ln)
      {
        *m = (UC8) val;

        if ((val > 0) && (*b == val))
        {
          // if trailing edge was max, rescan for new max in mask
          val = 0;
          ret = __min(dy, rh - (y - ny));
          f -= (ret * ln);
          for (i = 0; i < ret; i++, f += ln)
            if ((*f > 0) && ((*f < val) || (val <= 0)))
              val = *f;
        }
      }
    }
  }


  // PASS 2 horizontal ============================================
  // find minimum in vertical strip of height dx = [-nx 0 +px]
  // subtract trailing edge starting at xdel, add leading edge up to xadd 
  // x refers to location of output pixel in image dest (m)
  if (dx > 1)
  {
    if (dy > 1)
      s0 = a1.RoiSrc();
    else
      s0 = src.RoiSrc();
    m0 = dest.RoiDest();
    for (y = 0; y < rh; y++, m0 += ln, s0 += ln)
    {
      // start all pointers at base of column
      m = m0;
      b = s0;
      f = s0;

      // initialize value based on positive overlap of mask
      val = 0;
      for (i = 0; i < xini; i++, f++)
        if ((*f > 0) && ((*f < val) || (val <= 0)))
          val = *f;

      // at beginning mask is not completely in image
      for (x = 0; x < xlo; x++, m++, f++)
      {
        *m = (UC8) val;
        if ((*f > 0) && ((*f < val) || (val <= 0)))
          val = *f;
      }

      if (xadd < xdel)
        // huge masks span whole image in middle
        for (x = xlo; x < xhi; x++, m++)
          *m = (UC8) val;
      else    
        // in middle look at leading and trailing edge of mask
        for (x = xlo; x < xhi; x++, m++, b++, f++)
        {
          *m = (UC8) val;

          // if leading edge is greater >= max then just set max 
          if ((*f > 0) && ((*f < val) || (val <= 0)))
            val = *f;
          else if ((val > 0) && (*b == val))
          {
            // if trailing edge was max, rescan for new max in mask
            val = 0;
            f -= dx;
            for (i = 0; i < dx; i++, f++)
              if ((*f > 0) && ((*f < val) || (val <= 0)))
                val = *f;
          }
        }

      // at end mask falls off image
      for (x = xhi; x < rw; x++, m++, b++)
      {
        *m = (UC8) val;

        if ((val > 0) && (*b == val))
        {
          // if trailing edge was max, rescan for new max in mask
          val = 0;
          ret = __min(dx, rw - (x - nx));
          f -= ret;
          for (i = 0; i < ret; i++, f++)
            if ((*f > 0) && ((*f < val) || (val <= 0)))
              val = *f;
        }
      }
    }
  }
  return 1;
}


//= Finds the minimum non-zero 16 bit value in a rectangular-shaped region.
// sets output to zero if mask does not overlap any non-zero pixels

int jhcArea::BoxMin16 (jhcImg& dest, const jhcImg& src, int wid, int ht)
{
  int dx = wid, dy = ((ht == 0) ? wid : ht);

  // check arguments
  if (!dest.Valid(2) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcArea::BoxMin16");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxMin16", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0))
    return dest.FillArr(0);
  if ((dx == 1) && (dy == 1))
    return dest.CopyArr(src);
  dest.CopyRoi(src);
  if ((dx > 1) && (dy > 1))
  {
    a1.SetSize(dest);
    a1.CopyRoi(src);
  }

  // generic ROI case
  int x, y, i, ret, val;
  int rw = dest.RoiW(), rh = dest.RoiH(), ln2 = dest.Line() >> 1;
  int nx = dx / 2, px = dx - nx, xdel = __min(nx + 1, rw), xadd = __max(0, rw - px);
  int xini = __min(px, rw), xlo = __min(xdel, xadd), xhi = __max(xdel, xadd);
  int ny = dy / 2, py = dy - ny, ydel = __min(ny + 1, rh), yadd = __max(0, rh - py);
  int yini = __min(py, rh), ylo = __min(ydel, yadd), yhi = __max(ydel, yadd), dyln2 = dy * ln2;
  const US16 *b, *f, *s0;
  US16 *m, *m0;

  // PASS 1 vertical ==============================================
  // find minimum in vertical strip of height dy = [-ny 0 +py]
  // subtract trailing edge starting at ydel, add leading edge up to yadd 
  // y refers to location of output pixel in image a1 (m)
  if (dy > 1)
  {
    s0 = (const US16 *) src.RoiSrc();
    if (dx > 1)
      m0 = (US16 *) a1.RoiDest();
    else
      m0 = (US16 *) dest.RoiDest();
    for (x = 0; x < rw; x++, m0++, s0++)
    {
      // start all pointers at base of column
      m = m0;
      b = s0;
      f = s0;

      // initialize value based on positive overlap of mask
      val = 0;
      for (i = 0; i < yini; i++, f += ln2)
        if ((*f > 0) && ((*f < val) || (val <= 0)))
          val = *f;

      // at beginning mask is not completely in image
      for (y = 0; y < ylo; y++, m += ln2, f += ln2)
      {
        *m = (US16) val;
        if ((*f > 0) && ((*f < val) || (val <= 0)))
          val = *f;
      }

      if (yadd < ydel)
        // huge masks span whole image in middle
        for (y = ylo; y < yhi; y++, m += ln2)
          *m = (US16) val;
      else    
        // in middle look at leading and trailing edge of mask
        for (y = ylo; y < yhi; y++, m += ln2, b += ln2, f += ln2)
        {
          *m = (US16) val;

          // if leading edge is greater >= max then just set max 
          if ((*f > 0) && ((*f < val) || (val <= 0)))
            val = *f;
          else if ((val > 0) && (*b == val))
          {
            // if trailing edge was max, rescan for new max in mask
            val = 0;
            f -= dyln2;
            for (i = 0; i < dy; i++, f += ln2)
              if ((*f > 0) && ((*f < val) || (val <= 0)))
                val = *f;
          }
        }

      // at end mask falls off image
      for (y = yhi; y < rh; y++, m += ln2, b += ln2)
      {
        *m = (US16) val;

        if ((val > 0) && (*b == val))
        {
          // if trailing edge was max, rescan for new max in mask
          val = 0;
          ret = __min(dy, rh - (y - ny));
          f -= (ret * ln2);
          for (i = 0; i < ret; i++, f += ln2)
            if ((*f > 0) && ((*f < val) || (val <= 0)))
              val = *f;
        }
      }
    }
  }


  // PASS 2 horizontal ============================================
  // find minimum in vertical strip of height dx = [-nx 0 +px]
  // subtract trailing edge starting at xdel, add leading edge up to xadd 
  // x refers to location of output pixel in image dest (m)
  if (dx > 1)
  {
    if (dy > 1)
      s0 = (const US16 *) a1.RoiSrc();
    else
      s0 = (const US16 *) src.RoiSrc();
    m0 = (US16 *) dest.RoiDest();
    for (y = 0; y < rh; y++, m0 += ln2, s0 += ln2)
    {
      // start all pointers at base of column
      m = m0;
      b = s0;
      f = s0;

      // initialize value based on positive overlap of mask
      val = 0;
      for (i = 0; i < xini; i++, f++)
        if ((*f > 0) && ((*f < val) || (val <= 0)))
          val = *f;

      // at beginning mask is not completely in image
      for (x = 0; x < xlo; x++, m++, f++)
      {
        *m = (US16) val;
        if ((*f > 0) && ((*f < val) || (val <= 0)))
          val = *f;
      }

      if (xadd < xdel)
        // huge masks span whole image in middle
        for (x = xlo; x < xhi; x++, m++)
          *m = (US16) val;
      else    
        // in middle look at leading and trailing edge of mask
        for (x = xlo; x < xhi; x++, m++, b++, f++)
        {
          *m = (US16) val;

          // if leading edge is greater >= max then just set max 
          if ((*f > 0) && ((*f < val) || (val <= 0)))
            val = *f;
          else if ((val > 0) && (*b == val))
          {
            // if trailing edge was max, rescan for new max in mask
            val = 0;
            f -= dx;
            for (i = 0; i < dx; i++, f++)
              if ((*f > 0) && ((*f < val) || (val <= 0)))
                val = *f;
          }
        }

      // at end mask falls off image
      for (x = xhi; x < rw; x++, m++, b++)
      {
        *m = (US16) val;

        if ((val > 0) && (*b == val))
        {
          // if trailing edge was max, rescan for new max in mask
          val = 0;
          ret = __min(dx, rw - (x - nx));
          f -= ret;
          for (i = 0; i < ret; i++, f++)
            if ((*f > 0) && ((*f < val) || (val <= 0)))
              val = *f;
        }
      }
    }
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Rank Order Filtering                         //
///////////////////////////////////////////////////////////////////////////

//= Finds threshold such that some fraction of pixels in box are below value.
// if height is zero then copies width, picks halfway between boundary bins
// guarantees that AT LEAST frac of pixels in local area are AT OR ABOVE value
// conversely that AT MOST (1 - frac) of pixels in area are BELOW value 
// <pre>
// corner starting weights:
//
//   p4 | p4   p5         | x2 x1       dx = 3, nx = 1, px = 2
//      |                 |             dy = 5, ny = 2, py = 3
//   p2 | p2   p3         | x2 x1
//      |           ==>   | 
//   p0 | p0   p1         | x6 x3
//      +---------        +-------
//   p0   p0   p1   
//                         
//   p0   p0   p1 
//
//
// cut point finding:
//
//  index =    0  1  2  3  4  5  6
//   vals = [ 20 10  0  0  0 25 10 ]    with amt = 39 (1 - frac = 0.6) 
//               lo          hi           so mid = (1 + 5) / 2 = 3 
//                                        with sub = 30
// </pre>
// maintains "starter" histogram for each line by removing bottom row and adding top
// alters main histogram for each pixel by removing left column and adding right
// re-scanning columns is cheaper than combining partial histograms if dy < 256
// also uses incremental search from old threshold since usually a small move
// might be faster overall to divide into 8 border regions plus clip-free center
// 8.0ms for 3x3 (5.5x faster), 16.0ms for 9x9 (8.3x) on 320x240 with 3.2GHz Xeon

int jhcArea::BoxFracOver (jhcImg& dest, const jhcImg& src, int wid, int ht, double frac)
{
  int dx = wid, dy = ((ht == 0) ? wid : ht), area = dx * dy;

  // check arguments
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcArea::BoxFracOver");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxFracOver", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0) || (area <= 1) || (area > 65535))
    return 0;
  dest.CopyRoi(src);

  // generic ROI case
  int nx = dx / 2, px = dx - nx, nxp = nx + 1, pxm = px - 1;
  int ny = dy / 2, py = dy - ny, nyp = ny + 1;
  int rw = dest.RoiW(), rh = dest.RoiH(), ln = dest.Line();
  int xlim = rw - 1, ylim = rh - 1, rsk = ln - rw;
  int mid0 = 0, sub0 = 0, amt = area - ROUND(frac * area);
  int i, x, y, bot, top, lf, rt, now, mid, sub; 
  const UC8 *s00 = src.RoiSrc(), *s0 = s00, *s = s0;
  UC8 *d = dest.RoiDest();
 
  // clear line starting histogram
  for (i = 0; i < 256; i++)
    v0[i] = 0;

  // set histogram to be lower left corner with edge copy
  add_pel(s[0], v0, sub0, mid0, nxp * nyp);  // corner
  for (x = 1; x < px; x++)
    add_pel(s[x], v0, sub0, mid0, nyp);      // bottom row
  s += ln;
  for (y = 1; y < py; y++, s += ln)
  {
    add_pel(s[0], v0, sub0, mid0, nxp);      // left column
    for (x = 1; x < px; x++)
      add_pel(s[x], v0, sub0, mid0, 1);      // bulk
  }

  // compute cut point value for corner pixel
  mid_cut_up(mid0, sub0, v0, amt);

  // process each line of image
  for (y = 0; y < rh; y++, d += rsk)
  {
    // copy histogram and cut point for start of line
    for (i = 0; i < 256; i++)
      vals[i] = v0[i];
    sub = sub0;
    mid = mid0;
    *d++ = (UC8) mid;

    // clamp image source pointer to bottom border then
    bot = __max(y - ny, 0);
    s0 = s00 + bot * ln;

    // generate value for each pixel in line
    for (x = 1; x < rw; x++, d++)
    {
      // clamp inc/dec columns to side borders
      lf = __max(x - nxp, 0);
      rt = __min(x + pxm, xlim);
      s = s0;

      // adjust histogram contents and how many below threshold 
      for (i = -ny; i < py; i++)
      {
        // remove pixel from left column and add pixel from right column
        rem_pel(s[lf], vals, sub, mid, 1);
        add_pel(s[rt], vals, sub, mid, 1);

        // update source image line (repeats at top and bottom)
        now = y + i;
        if ((now >= 0) && (now < ylim))
          s += ln;
      }

      // compute new cut point and write to output
      if (sub <= amt)
        mid_cut_up(mid, sub, vals, amt);
      else if (mid > 0)
        mid_cut_dn(mid, sub, vals, amt);
      *d = (UC8) mid;
    }

    // adjust next line histogram by removing bottom row 
    rem_pel(s0[0], v0, sub0, mid0, nxp);
    for (x = 1; x < px; x++)
      rem_pel(s0[x], v0, sub0, mid0, 1);

    // clamp source image pointer to top border then add top
    top = __min(y + py, ylim);              
    s0 = s00 + top * ln;
    add_pel(s0[0], v0, sub0, mid0, nxp);
    for (x = 1; x < px; x++)
      add_pel(s0[x], v0, sub0, mid0, 1);

    // compute cut point for first pixel of next line
    if (sub0 <= amt)
      mid_cut_up(mid0, sub0, v0, amt);
    else if (mid0 > 0)
      mid_cut_dn(mid0, sub0, v0, amt);
  }
  return 1;
}


//= Decrement some histogram bin and adjust below threshold count.

void jhcArea::rem_pel (int v, US16 *hist, int& under, int cut, int wt) const  
{
  hist[v] -= (UC8) wt;
  if (v < cut)
    under -= wt;
}


//= Increment some histogram bin and adjust below threshold count.

void jhcArea::add_pel (int v, US16 *hist, int& under, int cut, int wt) const 
{
  hist[v] += (UC8) wt;
  if (v < cut)
    under += wt;
}


//= Adjust decision threshold upwards from previous value.

void jhcArea::mid_cut_up (int& cut, int& under, const US16 *hist, int th) const 
{
  int hi, lo = -1;

  // search histogram up from old cut point (always ok)
  for (hi = cut; hi < 256; hi++)
    if (hist[hi] > 0)
    {
      under += hist[hi];
      if (under > th)
        break;
      lo = hi;            
    }
  under -= hist[hi];        

  // try to find a non-zero lo bin (may fail)
  if (lo < 0)
    for (lo = cut - 1; lo >= 0; lo--)
      if (hist[lo] > 0)
        break;

  // compute new value based on brackets
  if (lo < 0)
    cut = hi;
  else
    cut = (lo + hi) >> 1;
  if (cut == lo)
    under -= hist[lo];
}


//= Adjust decision threshold downwards from previous value.

void jhcArea::mid_cut_dn (int& cut, int& under, const US16 *hist, int th) const 
{
  int lo, hi = 256;

  // search histogram down from old cut point 
  for (lo = cut - 1; lo >= 0; lo--)
    if (hist[lo] > 0)
    {
      if (under <= th)
        break;
      under -= hist[lo];   // go one past transition (may fail)
      hi = lo; 
    }

  // try to find a non-zero hi bin (may fail)
  if (hi > 255)
    for (hi = cut; hi < 256; hi++)
      if (hist[hi] > 0)
        break;

  // compute new value based on brackets
  if (lo < 0)
    cut = hi;
  else if (hi > 255)
    cut = lo;
  else
    cut = (lo + hi) >> 1;
  if (cut == lo)
    under -= hist[lo];
}


//= Finds threshold such that some fraction of pixels in box are above value.
// if height is zero then copies width, interpolates between boundary bins
// <pre>
// cut point finding:
//
//  index =    0  1  2  3  4  5  6
//   vals = [ 20 10  0  0  0 25 10 ]    with amt = 39 (frac = 0.6) 
//               lo          hi           so mid = 1 + (5 - 1) * (39 - 30) / 25 = 2 
//                                        with sub = 30
// </pre>

//  38.6 ms for 3x3 on 320x240 with 3.2GHz Xeon
// 119.9 ms for 9x9 on 320x240 with 3.2GHz Xeon

int jhcArea::BoxRankLin (jhcImg& dest, const jhcImg& src, int wid, int ht, double frac)
{
  int dx = wid, dy = ((ht == 0) ? wid : ht), area = dx * dy;

  // check arguments
  if (!dest.Valid(1) || !dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcArea::BoxRankLin");
  if ((dx > src.RoiW()) || (dy > src.RoiH()))
    return Fatal("Mask too big (%d %d) vs. (%d %d) in jhcArea::BoxRankLin", 
                 dx, dy, src.RoiW(), src.RoiH());

  // special cases
  if ((dx <= 0) || (dy <= 0) || (area <= 1) || (area > 65535))
    return 0;
  dest.CopyRoi(src);

  // generic ROI case
  int nx = dx / 2, px = dx - nx, ny = dy / 2, py = dy - ny;
  int rw = dest.RoiW(), rh = dest.RoiH(), ln = dest.Line();
  int xlim = rw - 1, ylim = rh - 1, rsk = ln - rw;
  int i, x, y, now, lo, hi, j, nowx; 
  int sub, amt = area - ROUND(frac * area);
  UC8 *d = dest.RoiDest();
 
  // process each line of image
  for (y = 0; y < rh; y++, d += rsk)
  {
    // generate value for each pixel in line
    for (x = 0; x < rw; x++, d++)
    {
      // clear histogram
      for (i = 0; i < 256; i++)
        vals[i] = 0;

      // form full histogram
      for (j = -ny; j < py; j++)
      {
        now = __max(__min(y + j, ylim), 0);
        for (i = -nx; i < px; i++)
        {
          nowx = __max(__min(x + i, xlim), 0);
          vals[src.ARef(nowx, now)] += 1;
        }
      }

      // compute new split point
      lo = -1;
      sub = 0;
      for (hi = 0; hi < 256; hi++)
        if (vals[hi] > 0)                    
        {
          sub += vals[hi];                  
          if (sub > amt)                 // find top filled bin
            break;
          lo = hi;                       // save bottom filled bin
        }
      if (lo < 0)
        *d = (UC8) hi;
      else
        *d = (UC8)((hi + lo) >> 1);
    }
  }
  return 1;
}


//= Median filtering using a square box.

int jhcArea::BoxMedian (jhcImg& dest, const jhcImg& src, int sc)
{
  return BoxRankLin(dest, src, sc, sc, 0.5);
}


///////////////////////////////////////////////////////////////////////////
//                              Tracking                                 //
///////////////////////////////////////////////////////////////////////////

//= Find the component label nearest the center of the search area.
// also binds the winning pixel in that blob
// returns 0 if nothing found in designated region

int jhcArea::NearestComp (int& wx, int &wy, const jhcRoi& area, const jhcImg& comps) const
{
  jhcRoi box;
  int cx = area.RoiMidX(), cy = area.RoiMidY(), ln = comps.Line() >> 1;
  int xlo, xhi, ylo, yhi, dx, dy, r2y, r2, best, mark = 0;
  const US16 *s, *s0;

  // check arguments and set defaults
  if (!comps.Valid(2))
    return Fatal("Bad input to jhcArea::NearestComp");
  wx = 0;
  wy = 0;

  // make sure search area is completely inside image
  box.CopyRoi(area);
  box.RoiClip(comps);
  xlo = box.RoiX() - cx;
  ylo = box.RoiY() - cy;
  xhi = box.RoiX2() - cx; 
  yhi = box.RoiY2() - cy;

  // check all lines within search region
  s0 = (const US16 *) comps.RoiSrc(box);
  for (dy = ylo; dy < yhi; dy++, s0 += ln)
  {
    // get y contribution to squared distance
    r2y = dy * dy;
    if ((mark != 0) && (r2y >= best))
      continue;

    // check pixels on this line
    s = s0;
    for (dx = xlo; dx < xhi; dx++, s++)
      if (*s != 0)
      {
        // get full squared distance if pixel labelled
        r2 = dx * dx + r2y;
        if ((mark == 0) || (r2 < best))
        {
          // record this label if closer than previous best
          best = r2;
          mark = *s;
          wx = dx;
          wy = dy;
        }
      }
  }

  // convert winning pixel position wrt center to full image coords
  wx += cx;
  wy += cy; 
  return mark;
}


//= Find the farthest point in the given component from the indicated center.
// the "comps" image can be an edited (gated) version of the connected components
// search will be constrained to "area" (e.g. component bounding box from jhcBBox)
// returns pixel distance to found point (assuming square pixels), negative for error

double jhcArea::ExtremePt (int& ex, int& ey, int cx, int cy, 
                           const jhcImg& comps, int mark, const jhcRoi& area) const
{
  jhcRoi box;
  int dx, dy, xlo, ylo, xhi, yhi, sk, r2y, r2, best = -1;
  const US16 *s;

  // check arguments 
  if (!comps.Valid(2))
    return Fatal("Bad input to jhcArea::ExtremePt");

  // make sure search area is completely inside image
  box.CopyRoi(area);
  box.RoiClip(comps);
  xlo = area.RoiX() - cx;
  ylo = area.RoiY() - cy;
  xhi = area.RoiX2() - cx;
  yhi = area.RoiY2() - cy;
  box.RoiClamp(ex, ey, cx, cy);

  // check all lines within search region
  s = (const US16 *) comps.RoiSrc(box);
  sk = comps.RoiSkip(box) >> 1;
  for (dy = ylo; dy < yhi; dy++, s += sk)
  {
    // get y contribution to squared distance
    r2y = dy * dy;
    for (dx = xlo; dx < xhi; dx++, s++)
      if (*s == mark)
      {
        // get full squared distance 
        r2 = dx * dx + r2y;
        if (r2 > best)
        {
          // remember farthest point relative to center
          ex = dx;
          ey = dy;
          best = r2;
        }
      }
   }

   // adjust point wrt center to point wrt image
   if (best < 0)
     return -1.0;
   ex += cx;
   ey += cy;
   return sqrt((double) best);
}       

