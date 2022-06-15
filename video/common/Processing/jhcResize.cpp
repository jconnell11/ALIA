// jhcResize.cpp : ways of changing between image sizes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#include "Processing/jhcResize.h"


//////////////////////////////////////////////////////////////////////////////
//                     Basic creation and deletion                          //
//////////////////////////////////////////////////////////////////////////////

//= Destructor releases temporary image.

jhcResize::~jhcResize ()
{
  dealloc();
}


//= Constructor initializes values.

jhcResize::jhcResize ()
{
  temp = NULL;
  tsize = 0;
}


//= Make up a temporary array of 32 bit signed integers.

void jhcResize::alloc (int sz)
{
  if ((temp != NULL) && (sz <= tsize))
    return;
  dealloc();
  if ((temp = new int[sz]) == NULL)
    Fatal("jhcResize::alloc - Buffer allocation [%d] failed!", sz); 
  tsize = sz;
}


//= Get rid of any temporary array.

void jhcResize::dealloc ()
{
  delete [] temp;
  temp = NULL;
  tsize = 0;
}


///////////////////////////////////////////////////////////////////////////
//                          Sampling Methods                             //
///////////////////////////////////////////////////////////////////////////

//= Use sampling or smoothing based on argument.
//   0 = sample, 1 = smooth, 2 = stretch

int jhcResize::ForceSize (jhcImg& dest, const jhcImg& src, int style)
{
  if ((style == 2) && (src.Square() > 0) && (dest.Square() > 0))
    return Sample(dest, src);
  if (style == 2)
    return Interpolate(dest, src, 0, 0, src.XDim(), src.YDim());
  if ((style == 1) && (dest.XDim() <= src.XDim()) && (dest.YDim() <= src.YDim())) 
    return Smooth(dest, src);
  return Sample(dest, src);
}


//= Use source to fill array, either by decimating or duplicating pixels.
// non-integer resampling allows uneven duplication

int jhcResize::Sample (jhcImg& dest, const jhcImg& src) const 
{
  if (dest.SameFormat(src))
    return dest.CopyArr(src);
  return SampleN(dest, src);
}


//= Use source to fill array, either by decimating or duplicating pixels.
// integer sampling only -- can introduce dark bars

int jhcResize::SampleN (jhcImg& dest, const jhcImg& src) const 
{
  int nf = dest.Fields();

  // check arguments, detect special cases
  if (!dest.Valid() || !src.Valid(nf))
    return Fatal("Bad images to jhcResize::SampleN");
  if (dest.SameFormat(src))
    return dest.CopyArr(src);
  if (dest.Valid(1, 3) && (dest.XDim() == (2 * src.XDim())) && src.FullRoi())
    return Double(dest, src);
  if ((nf >= 1) && (nf <= 3) && (src.XDim() > dest.XDim()) && src.FullRoi())
    return Decimate(dest, src, src.XDim() / dest.XDim());

  // local variables
  int w = dest.XDim(), sw = src.XDim(), xf = w / sw;
  int h = dest.YDim(), sh = src.YDim(), yf = h / sh;
  double sx = (double) xf, sy = (double) yf;
  
  // set up destination ROI to match scaled version of source
  if (w < sw)
  {
    xf = sw / w;
    sx = 1.0 / xf;
  }
  if (h < sh)
  {
    yf = sh / h;
    sy = 1.0 / yf;
  }
  dest.CopyRoi(src);
  dest.ScaleRoi(sx, sy);

  // more local variables
  int x, y, i, j, f;
  int xskip = nf, yskip = src.Line(), dsk = dest.RoiSkip();
  int xdup = 1, ydup = 1, xcnt = dest.RoiW(), ycnt = dest.RoiH();
  const UC8 *s2, *s = src.RoiSrc(); 
  UC8 *d = dest.RoiDest(); 

  // determine pixel spacing and duplication parameters   
  if (w > sw)
  {
    xdup = xf;
    xcnt /= xf;
  }
  else
    xskip *= xf;
  if (h > sh)
  {
    ydup = yf;
    ycnt /= yf;
  }
  else
    yskip *= yf;

  // copy selected pixels of source 
  for (y = ycnt; y > 0; y--, s += yskip)
    for (j = ydup; j > 0; j--, d += dsk)
    {
      // copy source pixel, possibly into many destination pixels
      s2 = s;
      for (x = xcnt; x > 0; x--, s2 += xskip)
        for (i = xdup; i > 0; i--)
          for (f = 0; f < nf; f++, d++)
            *d = s2[f];
    }
  return 1;
}


//= Special monochrome case where destination image is twice size of source.
// always does whole image, does not respect or copy ROI
// might leave some destination pixels unfilled

int jhcResize::Double (jhcImg& dest, const jhcImg& src) const 
{
  int x, y, w, h, dln = dest.Line(), sln = src.Line();
  UC8 *d, *d0 = dest.PxlDest();
  const UC8 *s, *s0 = src.PxlSrc();

  // check arguments, detect special cases
  if (!dest.Valid(1, 3) || !src.Valid(dest.Fields()) || !src.FullRoi())
    return Fatal("Bad images to jhcResize::Double");
  if (dest.Valid(3))
    return DoubleRGB(dest, src);

  // figure out sizes and end skips
  w = dest.XDim() / 2;
  w = __min(w, src.XDim());
  h = dest.YDim() / 2;
  h = __min(h, src.YDim());
  
  // copy pixels
  for (y = 0; y < h; y++)
  {
    d = d0;
    s = s0;
    for (x = 0; x < w; x++, d += 2, s++)
    {
      d[0] = s[0];
      d[1] = s[0];
    }
    d0 += dln;
    d = d0;
    s = s0;
    for (x = 0; x < w; x++, d += 2, s++)
    {
      d[0] = s[0];
      d[1] = s[0];
    }
    d0 += dln;
    s0 += sln;
  }
  return 1;
}


//= Special color case where destination image is twice size of source.
// always does whole image, does not respect or copy ROI
// might leave some destination pixels unfilled

int jhcResize::DoubleRGB (jhcImg& dest, const jhcImg& src) const 
{
  int x, y, w, h, dln = dest.Line(), sln = src.Line();
  UC8 *d, *d0 = dest.PxlDest();
  const UC8 *s, *s0 = src.PxlSrc();

  // check arguments, detect special cases
  if (!dest.Valid(1, 3) || !src.Valid(dest.Fields()) || !src.FullRoi())
    return Fatal("Bad images to jhcResize::DoubleRGB");

  // figure out sizes and end skips
  w = dest.XDim() / 2;
  w = __min(w, src.XDim());
  h = dest.YDim() / 2;
  h = __min(h, src.YDim());

  // copy pixels
  for (y = 0; y < h; y++)
  {
    d = d0;
    s = s0;
    for (x = 0; x < w; x++, d += 6, s += 3)
    {
      d[0] = s[0];
      d[1] = s[1];
      d[2] = s[2];
      d[3] = s[0];
      d[4] = s[1];
      d[5] = s[2];
    }
    d0 += dln;
    d = d0;
    s = s0;
    for (x = 0; x < w; x++, d += 6, s += 3)
    {
      d[0] = s[0];
      d[1] = s[1];
      d[2] = s[2];
      d[3] = s[0];
      d[4] = s[1];
      d[5] = s[2];
    }
    d0 += dln;
    s0 += sln;
  }
  return 1;
}


//= Special sampling case where destination image is smaller than source.
// always does whole image, does not respect or copy ROI
// might leave some destination pixels unfilled

int jhcResize::Decimate (jhcImg& dest, const jhcImg& src, int factor) const 
{
  int x, y, w, h, f = dest.Fields(), dln = dest.Line(), sln = factor * src.Line();
  UC8 *d, *d0 = dest.PxlDest();
  const UC8 *s, *s0 = src.PxlSrc();

  // check arguments, detect special cases
  if ((f < 1) || (f > 3) || !dest.Valid() || !src.Valid(dest.Fields()) || 
      (factor <= 0) || !src.FullRoi())
    return Fatal("Bad images to jhcResize::Decimate");
  if ((factor == 1) && dest.SameFormat(src))
    return dest.CopyArr(src);
  if (dest.Valid(3))
    return DecimateRGB(dest, src, factor);
  if (dest.Valid(2))
    return Decimate_16(dest, src, factor);

  // figure out sizes and end skips
  w = src.XDim() / factor;
  w = __min(w, dest.XDim());
  h = src.YDim() / factor;
  h = __min(h, dest.YDim());
  
  // copy pixels for monochrome case
  for (y = 0; y < h; y++)
  {
    d = d0;
    s = s0;
    for (x = 0; x < w; x++, d++, s += factor)
      *d = *s;
    d0 += dln;
    s0 += sln;
  }
  return 1;
}


//= Special color case where destination image is smaller than source.
// always does whole image, does not respect or copy ROI
// might leave some destination pixels unfilled

int jhcResize::DecimateRGB (jhcImg& dest, const jhcImg& src, int factor) const 
{
  int x, y, w, h, f3 = 3 * factor;
  int dln = dest.Line(), sln = factor * src.Line();
  UC8 *d, *d0 = dest.PxlDest();
  const UC8 *s, *s0 = src.PxlSrc();

  // check arguments, detect special cases
  if (!dest.Valid(3) || !src.Valid(3) || (factor <= 0))
    return Fatal("Bad images to jhcResize::DecimateRGB");
  if ((factor == 1) && dest.SameFormat(src))
    return dest.CopyArr(src);

  // figure out sizes and end skips
  w = src.XDim() / factor;
  w = __min(w, dest.XDim());
  h = src.YDim() / factor;
  h = __min(h, dest.YDim());
  
  // copy pixels
  for (y = 0; y < h; y++)
  {
    d = d0;
    s = s0;
    for (x = 0; x < w; x++, d += 3, s += f3)
    {
      d[0] = s[0];
      d[1] = s[1];
      d[2] = s[2];
    }
    d0 += dln;
    s0 += sln;
  }
  return 1;
}


//= Sample 16 bit image source into smaller destination.
// always does whole image, does not respect or copy ROI
// might leave some destination pixels unfilled

int jhcResize::Decimate_16 (jhcImg& dest, const jhcImg& src, int factor) const 
{
  int x, y, w, h, dln2 = dest.Line() >> 1, sln2 = (factor * src.Line()) >> 1;
  US16 *d, *d0 = (US16 *) dest.PxlDest();
  const US16 *s, *s0 = (const US16 *) src.PxlSrc();

  // check arguments, detect special cases
  if (!dest.Valid(2) || !src.Valid(2) || (factor <= 0))
    return Fatal("Bad images to jhcResize::Decimate_16");
  if ((factor == 1) && dest.SameFormat(src))
    return dest.CopyArr(src);

  // figure out sizes and end skips
  w = src.XDim() / factor;
  w = __min(w, dest.XDim());
  h = src.YDim() / factor;
  h = __min(h, dest.YDim());
  
  // copy pixels
  for (y = h; y > 0; y--, d0 += dln2, s0 += sln2)
    for (x = w, d = d0, s = s0; x > 0; x--, d++, s += factor)
      *d = *s;
  return 1;
}


//= Get a version of source which is no larger than alternate.
// alternate image must be an integral multiple smaller than source
// useful for making displayable versions of potentially giant HD images
// returns a pointer to source if already okay, else samples it into alt

jhcImg *jhcResize::Smallest (jhcImg& alt, jhcImg& src) const
{
  if (alt.SameFormat(src) > 0)
    return &src;
  SampleN(alt, src);
  return &alt;
}


///////////////////////////////////////////////////////////////////////////
//                          Averaging Methods                            //
///////////////////////////////////////////////////////////////////////////

//= Use larger source to fill self, each pixel is interpolated from neighbors.
// non-integer sampling means some input pixels might never be used
// NOTE: does not smooth image first, thus aliasing is possible

int jhcResize::Smooth (jhcImg& dest, const jhcImg& src) 
{
  if (dest.SameFormat(src))
    return dest.CopyArr(src);
  return SmoothN(dest, src);
}


//= Use larger source to fill self, each pixel is an average over a block.
// integer sampling only -- can introduce black bars

int jhcResize::SmoothN (jhcImg& dest, const jhcImg& src)
{
  int sw = src.XDim(), sh = src.YDim();
  int w = dest.XDim(), h = dest.YDim(), nf = dest.Fields();

  // check arguments
  if (!dest.Valid() || !src.Valid(nf) || (sw < w) || (sh < h))
    return Fatal("Bad images to jhcResize::SmoothN");

  // check for straight copy
  if ((sw == w) && (sh == h))
    return dest.CopyArr(src);

  // special case of exactly half in each dimension
  if (DimScaled(sw, w, 2) && DimScaled(sh, h, 2) && ((nf == 1) || (nf == 3)))
  {
    if (nf == 3)
      return AvgHalfRGB(dest, src);
    return AvgHalf(dest, src);
  }

  // special case of exactly one third in each dimension
  if (DimScaled(sw, w, 3) && DimScaled(sh, h, 3) && ((nf == 1) || (nf == 3)))
  {
    if (nf == 3)
      return AvgThirdRGB(dest, src);
    return AvgThird(dest, src);
  }

  // everything else
  return generic_sm(dest, src);
}


//= Does arbitrary (and anisotropic) block smoothing.

int jhcResize::generic_sm (jhcImg& dest, const jhcImg& src) 
{
  // non-ROI -- does whole image
  int sw = src.XDim(), sh = src.YDim();
  int w = dest.XDim(), h = dest.YDim(), nf = dest.Fields();
  int x, y, i, j, f, xdup = sw / w, ydup = sh / h;
  int sk = dest.Skip(), ln = src.Line(), aline = w * nf;
  int cnt = xdup * ydup, hcnt = cnt / 2, asize = aline * h, k = asize;
  const UC8 *s2, *s = src.PxlSrc();
  UC8 *d = dest.PxlDest();
  int *a, *a2; 

  // clear temporary array 
  alloc(asize);
  a = temp;
  while (k-- > 0)
    *a++ = 0;

  // increment temporary sum array using blocks from source
  a = temp;
  for (y = h; y > 0; y--)
  {
    for (j = ydup; j > 0; j--)
    {
      a2 = a;
      s2 = s;
      for (x = w; x > 0; x--)
      {
        for (i = xdup; i > 0; i--)
          for (f = 0; f < nf; f++)
            *(a2 + f) += *s2++;
        a2 += nf;
      }
      s += ln;
    }
    a += aline;
  }

  // divide temporary array by counts and assign to self
  a = temp;
  for (y = h; y > 0; y--)
  {
    for (x = w; x > 0; x--)
      for (f = nf; f > 0; f--)
        *d++ = (UC8)((*a++ + hcnt) / cnt);
    d += sk;
  }

  return 1;
}


//= Check if the big value is approximately the small value scaled.

int jhcResize::DimScaled (int big, int sm, int sc) const 
{
  int ssm = sc * sm;

  if ((big >= ssm) && ((big - ssm) < sc))
    return 1;
  return 0;
}


//= Special version of SmoothN for generating monochrome half-sized images.

int jhcResize::AvgHalf (jhcImg& dest, const jhcImg& src) const 
{
  jhcRoi active;

  // check for proper scale factor
  if (!dest.Valid(1) || !src.Valid(1) || 
      !DimScaled(src.XDim(), dest.XDim(), 2) || 
      !DimScaled(src.YDim(), dest.YDim(), 2))
    return Fatal("Bad images to jhcResize::AvgHalf");
  active.CopyRoi(src);
  active.ScaleRoi(0.5);
  dest.CopyRoi(active);

  // general ROI case
  int sum, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int sln = src.Line(), ssk = 2 * sln - 2 * rw, dsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(), *sup = s + sln;

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d++, s += 2, sup += 2)
    {
      sum = s[0] + s[1] + sup[0] + sup[1];
      d[0] = (UC8)((sum + 2) >> 2);     // added rounding
    }
    d += dsk;
    s += ssk;
    sup += ssk;
  }
  return 1;
}


//= Special version of SmoothN for generating RGB half-sized images.

int jhcResize::AvgHalfRGB (jhcImg& dest, const jhcImg& src) const 
{
  jhcRoi active;

  // check for proper scale factor
  if (!dest.Valid(3) || !src.Valid(3) || 
      !DimScaled(src.XDim(), dest.XDim(), 2) || 
      !DimScaled(src.YDim(), dest.YDim(), 2))
    return Fatal("Bad images to jhcResize::AvgHalfRGB");
  active.CopyRoi(src);
  active.ScaleRoi(0.5);
  dest.CopyRoi(active);

  // general ROI case
  int sum, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int sln = src.Line(), ssk = 2 * sln - 6 * rw, dsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(), *sup = s + sln;

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d += 3, s += 6, sup += 6)
    {
      // blue
      sum = s[0] + s[3] + sup[0] + sup[3];
      d[0] = (UC8)((sum + 2) >> 2);  // added rounding

      // green
      sum = s[1] + s[4] + sup[1] + sup[4];
      d[1] = (UC8)((sum + 2) >> 2);

      // red
      sum = s[2] + s[5] + sup[2] + sup[5];
      d[2] = (UC8)((sum + 2) >> 2);
    }
    d += dsk;
    s += ssk;
    sup += ssk;
  }
  return 1;
}


//= Special version of SmoothN for generating monochrome third-sized images.

int jhcResize::AvgThird (jhcImg& dest, const jhcImg& src) const 
{
  jhcRoi active;

  // check for proper scale factor
  if (!dest.Valid(1) || !src.Valid(1) || 
      !DimScaled(src.XDim(), dest.XDim(), 3) || 
      !DimScaled(src.YDim(), dest.YDim(), 3))
    return Fatal("Bad images to jhcResize::AvgThird");
  active.CopyRoi(src);
  active.ScaleRoi(0.33333);
  dest.CopyRoi(active);

  // general ROI case
  int sum, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int sln = src.Line(), ssk = 3 * sln - 3 * rw, dsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(), *sup = s + sln, *shi = sup + sln;

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d++, s += 3, sup += 3, shi += 3)
    {
      sum  =   s[0] +   s[1] +   s[2];
      sum += sup[0] + sup[1] + sup[2];
      sum += shi[0] + shi[1] + shi[2];
      d[0] = (UC8)((7282 * sum + 32768) >> 16);  // 7282/65536 = 1/9
    }
    d += dsk;
    s += ssk;
    sup += ssk;
    shi += ssk;
  }
  return 1;
}

 
//= Special version of SmoothN for generating color third-sized images.

int jhcResize::AvgThirdRGB (jhcImg& dest, const jhcImg& src) const 
{
  jhcRoi active;

  // check for proper scale factor
  if (!dest.Valid(3) || !src.Valid(3)   || 
      !DimScaled(src.XDim(), dest.XDim(), 3) || 
      !DimScaled(src.YDim(), dest.YDim(), 3))
    return Fatal("Bad images to jhcResize::AvgThirdRGB");
  active.CopyRoi(src);
  active.ScaleRoi(0.33333);
  dest.CopyRoi(active);

  // general ROI case
  int sum, x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int sln = src.Line(), ssk = 3 * sln - 9 * rw, dsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(), *sup = s + sln, *shi = sup + sln;

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d += 3, s += 9, sup += 9, shi += 9)
    {
      // blue
      sum  =   s[0] +   s[3] +   s[6];
      sum += sup[0] + sup[3] + sup[6];
      sum += shi[0] + shi[3] + shi[6];
      d[0] = (UC8)((7282 * sum + 32768) >> 16);  

      // green
      sum  =   s[1] +   s[4] +   s[7];
      sum += sup[1] + sup[4] + sup[7];
      sum += shi[1] + shi[4] + shi[7];
      d[1] = (UC8)((7282 * sum + 32768) >> 16);  

      // red
      sum  =   s[2] +   s[5] +   s[8];
      sum += sup[2] + sup[5] + sup[8];
      sum += shi[2] + shi[5] + shi[8];
      d[2] = (UC8)((7282 * sum + 32768) >> 16);  
    }
    d += dsk;
    s += ssk;
    sup += ssk;
    shi += ssk;
  }
  return 1;
}


//= Fill half-sized 16 bit image with minimum of 4 neighbor pixels.
// ignores source ROI and always does full image
// makes more sense for depth images than Blocks_16

int jhcResize::MinHalf_16 (jhcImg& dest, const jhcImg& src) const
{
  int dw = dest.XDim(), dh = dest.YDim(), dsk2 = dest.Skip() >> 1;
  int x, y, lo, lo2, sln2 = src.Line() >> 1, ssk2 = (sln2 - dw) << 1;
  const US16 *s = (const US16 *) src.PxlSrc();
  UC8 *d = dest.PxlDest();

  // check for proper scale factor
  if (!dest.Valid(2) || !src.Valid(2) || 
      (dw != (src.XDim() >> 1)) || (dh != (src.YDim() >> 1)))
    return Fatal("Bad images to jhcResize::MinHalf_16");
  dest.MaxRoi();

  // full ROI case
  for (y = dh; y > 0; y--, d += dsk2, s += ssk2)
    for (x = dw; x > 0; x--, d++, s += 2)
    {
      lo  = __min(s[0], s[1]);
      lo2 = __min(s[sln2], s[sln2 + 1]);
      *d  = (UC8) __min(lo, lo2);
    }
  return 1;
}


//= Generate a new image by averaging blocks of pixels of size w x h.
// unlike SmoothN this allows a starting corner and explict block size
// fills in as much of destination image as possible, rest is unchanged

int jhcResize::Blocks (jhcImg& dest, const jhcImg& src, int sx, int sy, int bw, int bh) const 
{
  // check input
  if (src.Valid(2))
    return Blocks_16(dest, src, sx, sy, bw, bh);
  if (!dest.Valid(1) || !src.Valid(1))
    return Fatal("Bad images to jhcResize::Blocks");
  dest.MaxRoi();

  // check if there is any area to be sampled and simplest case
  if ((sx < 0) || (sx >= src.XDim()) || 
      (sy < 0) || (sy >= src.YDim()) || 
      (bw <= 0) || (bh <= 0))
    return 1;
  if ((sx == 0) && (sy == 0) && (bw == 1) && (bh == 1))
    return dest.CopyArr(src);

  // figure out pixel addresses for overlap of images
  int x, y, i, j, sum, norm = bw * bh, half = norm >> 1;
  int dw = dest.XDim(), dh = dest.YDim(), sw = src.XDim(), sh = src.YDim();
  int ex = sx + dw * bw, xlim = (__min(ex, sw) - sx) / bw;
  int ey = sy + dh * bh, ylim = (__min(ey, sh) - sy) / bh;
  int dln = dest.Line(), dsk = dln - xlim;
  int sln = src.Line(), ssk = bh * sln - bw * xlim;
  const UC8 *s, *s0 = src.RoiSrc(sx, sy);
  UC8 *d = dest.PxlDest();

  // accumulate pixels in blocks then normalize
  for (y = ylim; y > 0; y--, d += dsk, s0 += ssk)
    for (x = xlim; x > 0; x--, d++, s0 += bw)
    {
      s = s0;
      sum = half;
      for (j = 0; j < bh; j++, s += sln)
        for (i = 0; i < bw; i++)
          sum += s[i]; 
      *d = (UC8)(sum / norm);
    }
  return 1;
} 

 
//= Generate a new image by averaging blocks of 16 bit pixels of size w x h.
// fills in as much of destination image as possible, rest is unchanged
// not particularly sensible for depth images

int jhcResize::Blocks_16 (jhcImg& dest, const jhcImg& src, int sx, int sy, int bw, int bh) const 
{
  // check input
  if (!dest.Valid(2) || !src.Valid(2))
    return Fatal("Bad images to jhcResize::Blocks_16");
  dest.MaxRoi();

  // check if there is any area to be sampled and simplest case
  if ((sx < 0) || (sx >= src.XDim()) || 
      (sy < 0) || (sy >= src.YDim()) || 
      (bw <= 0) || (bh <= 0))
    return 1;
  if ((sx == 0) && (sy == 0) && (bw == 1) && (bh == 1))
    return dest.CopyArr(src);

  // figure out pixel addresses for overlap of images
  int x, y, i, j, sum, norm = bw * bh, half = norm >> 1;
  int dw = dest.XDim(), dh = dest.YDim(), sw = src.XDim(), sh = src.YDim();
  int ex = sx + dw * bw, xlim = (__min(ex, sw) - sx) / bw;
  int ey = sy + dh * bh, ylim = (__min(ey, sh) - sy) / bh;
  int dln = dest.Line() >> 1, dsk = dln - xlim;
  int sln = src.Line() >> 1, ssk = bh * sln - bw * xlim;
  const US16 *s, *s0 = (const US16 *) src.RoiSrc(sx, sy);
  US16 *d = (US16 *) dest.PxlDest();

  // accumulate pixels in blocks then normalize
  for (y = ylim; y > 0; y--, d += dsk, s0 += ssk)
    for (x = xlim; x > 0; x--, d++, s0 += bw)
    {
      s = s0;
      sum = half;
      for (j = 0; j < bh; j++, s += sln)
        for (i = 0; i < bw; i++)
          sum += s[i]; 
      *d = (US16)(sum / norm);
    }
  return 1;
} 


///////////////////////////////////////////////////////////////////////////
//                       Non-Integer Resizing                            //
///////////////////////////////////////////////////////////////////////////

//= Like below but takes spec in terms of a ROI.

int jhcResize::Interpolate (jhcImg& dest, const jhcImg &src, const jhcRoi& a) const 
{
  return Interpolate(dest, src, a.RoiX(), a.RoiY(), a.RoiW(), a.RoiH());
}


//= Fills dest with resampled source (no rotation).
// adjusts sampling parameters to fit completely in source (no black)
// NOTE: only uses nearest 4 pixels, so aliasing might occur
// ignores ROI specs in source and destination

int jhcResize::Interpolate (jhcImg& dest, const jhcImg &src, 
                            int ax, int ay, int aw, int ah) const 
{
  if (!dest.Valid(src.Fields()) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::Interpolate");

  // coerce area limits to fit inside source image
  int cx, cx2, cy, cy2, wid, ht, sw = src.XDim(), sh = src.YDim();
  cx  = __max(0, __min(ax,      sw - 1));
  cy  = __max(0, __min(ay,      sh - 1));
  cx2 = __max(0, __min(ax + aw, sw));
  cy2 = __max(0, __min(ay + ah, sh));
  wid = cx2 - cx;
  ht  = cy2 - cy; 

  // figure out step and mixing coefs for each destination x 
  int dx, sx, last = 0, dw = dest.XDim(), nf = dest.Fields();
  double xf, xsc = wid / (double) dw;
  int *lf, *rt, *xstep;

  lf    = (int *) new int[dw];          // allocate arrays (could do once)
  rt    = (int *) new int[dw];
  xstep = (int *) new int[dw];
  
  for (dx = 0; dx < dw; dx++)
  {
    xf = xsc * dx;                      // subpixel source position
    sx = (int) xf;
    xstep[dx] = nf * (sx - last);       // integer move since last
    last = sx;
    rt[dx] = ROUND(256.0 * (xf - sx));  // fractional part
    lf[dx] = 256 - rt[dx];
  }

  // mix appropriate source pixels for each destination pixel
  int dy, sy, i, dn, up;
  int dh = dest.YDim(), dsk = dest.Skip(), sln = src.Line();
  UL32 swf, sef, nwf, nef, v;
  double yf, ysc = ht / (double) dh;
  const UC8 *s, *s0 = src.PxlSrc() + (cy * sln) + (cx * nf); 
  UC8 *d = dest.PxlDest();

  last = 0;
  dn = 256;
  up = 0;
  for (dy = 0; dy < dh; dy++)
  {
    s = s0;
    for (dx = 0; dx < dw; dx++)
    {
      // advance to next source region
      s += xstep[dx];

      // figure out cascaded coefficients then mix all fields
      swf = dn * lf[dx];
      sef = dn * rt[dx];
      nwf = up * lf[dx];
      nef = up * rt[dx];
      for (i = 0; i < nf; i++)
      {
        v =  swf * s[i];
        v += sef * s[i + nf];  
        v += nwf * s[i + sln];  
        v += nef * s[i + sln + nf];  
        d[i] = (UC8)(v >> 16);
      }

      // advance to next destination pixel 
      d += nf;
    }

    // advance to next input neighborhood and refigure mixing
    d += dsk;
    yf = ysc * dy;
    sy = (int) yf;
    s0 += sln * (sy - last);        // integer part to advance
    last = sy;
    up = ROUND(256.0 * (yf - sy));  // fractional part
    dn = 256 - up;
  }

  // clean up
  delete [] xstep;
  delete [] rt;
  delete [] lf;
  return 1;
}


//= Like below but takes spec in terms of a ROI (e.g. src image).

int jhcResize::InterpolateNZ (jhcImg& dest, const jhcImg &src, const jhcRoi& a) const 
{
  return InterpolateNZ(dest, src, a.RoiX(), a.RoiY(), a.RoiW(), a.RoiH());
}


//= Fills dest with resampled source (no rotation) ignoring black pixels.
// adjusts sampling parameters to fit completely in source (no black)
// NOTE: only uses nearest 4 pixels, so aliasing might occur
// ignores ROI specs in source and destination

int jhcResize::InterpolateNZ (jhcImg& dest, const jhcImg &src, 
                            int ax, int ay, int aw, int ah) const 
{
  if (!dest.Valid(src.Fields()) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::InterpolateNZ");

  // coerce area limits to fit inside source image
  int cx, cx2, cy, cy2, wid, ht, sw = src.XDim(), sh = src.YDim();
  cx  = __max(0, __min(ax,      sw - 1));
  cy  = __max(0, __min(ay,      sh - 1));
  cx2 = __max(0, __min(ax + aw, sw));
  cy2 = __max(0, __min(ay + ah, sh));
  wid = cx2 - cx;
  ht  = cy2 - cy; 

  // figure out step and mixing coefs for each destination x 
  int dx, sx, last = 0, dw = dest.XDim(), nf = dest.Fields();
  double xf, xsc = wid / (double) dw;
  int *lf, *rt, *xstep;

  lf    = (int *) new int[dw];          // allocate arrays (could do once)
  rt    = (int *) new int[dw];
  xstep = (int *) new int[dw];
  
  for (dx = 0; dx < dw; dx++)
  {
    xf = xsc * dx;                      // subpixel source position
    sx = (int) xf;
    xstep[dx] = nf * (sx - last);       // integer move since last
    last = sx;
    rt[dx] = ROUND(256.0 * (xf - sx));  // fractional part
    lf[dx] = 256 - rt[dx];
  }

  // mix appropriate source pixels for each destination pixel
  int dy, sy, i, dn, up, vsw, vse, vnw, vne, def;
  int dh = dest.YDim(), dsk = dest.Skip(), sln = src.Line();
  UL32 swf, sef, nwf, nef, v;
  double yf, ysc = ht / (double) dh;
  int xlim = (int)(wid / xsc), ylim = (int)(ht / ysc);
  const UC8 *s, *s0 = src.PxlSrc() + (cy * sln) + (cx * nf); 
  UC8 *d = dest.PxlDest();

  last = 0;
  dn = 256;
  up = 0;
  for (dy = 0; dy < ylim; dy++)
  {
    s = s0;
    for (dx = 0; dx < xlim; dx++)
    {
      // advance to next source region
      s += xstep[dx];

      // figure out cascaded coefficients then mix all fields
      swf = dn * lf[dx];
      sef = dn * rt[dx];
      nwf = up * lf[dx];
      nef = up * rt[dx];
      for (i = 0; i < nf; i++)
      {
        // get basic 4 pixel values
        vsw = s[i];
        if (dx < (xlim - 1))
          vse = s[i + nf];  
        else
          vse = vsw;
        if (dy < (ylim - 1))
        {
          vnw = s[i + sln];  
          if (dx < (xlim - 1))
            vne = s[i + sln + nf];  
          else
            vne = vnw;
        }
        else
        {
          vnw = vsw;
          vne = vse;
        }

        // get reasonable non-zero values for all 4 pixels
        def = __max(__max(vsw, vse), __max(vnw, vne));
        if (vsw <= 0)
          vsw = def;
        if (vse <= 0)
          vse = def;
        if (vnw <= 0)
          vnw = def;
        if (vne <= 0)
          vne = def;

        // interpolate samples
        v =  swf * vsw;
        v += sef * vse;
        v += nwf * vnw;
        v += nef * vne;
        d[i] = (UC8)(v >> 16);
      }

      // advance to next destination pixel 
      d += nf;
    }

    // advance to next input neighborhood and refigure mixing
    d += dsk;
    yf = ysc * dy;
    sy = (int) yf;
    s0 += sln * (sy - last);        // integer part to advance
    last = sy;
    up = ROUND(256.0 * (yf - sy));  // fractional part
    dn = 256 - up;
  }

  // clean up
  delete [] xstep;
  delete [] rt;
  delete [] lf;
  return 1;
}


//= Copies magnified portion of source centered on (cx cy) to destination
// if clipped part of source is too small, part of dest is written to zero
// <pre>
// image has 9 different interpolation regions:
//
//         0 0 0 0 0 0 0 0 0 0 0 
//         0 0 0 C T T T T T C 0
//         0 0 0 L M M M M M R 0
//         0 0 0 L M M M M M R 0
//         0 0 0 L M M M M M R 0
//         0 0 0 L M M M M M R 0
//         0 0 0 C B B B B B C 0
//         0 0 0 0 0 0 0 0 0 0 0 
// </pre>

int jhcResize::Resample (jhcImg& dest, const jhcImg &src, double cx, double cy, double xsc, double ysc) const 
{
  // check for valid arguments
  if (dest.Valid(2))
    return Resample_16(dest, src, cx, cy, xsc, ysc);
  if (!dest.Valid() || !src.Valid(dest.Fields()) || src.SameImg(dest) || (xsc <= 0.0))
    return Fatal("Bad images to jhcResize::Resample");

  // local variables
  double magx = xsc, magy = ((ysc > 0.0) ? ysc : xsc);
  double dy0, dy1, dx0, dx1, sx0, sy0, sx, sy, xstep = 1.0 / magx, ystep = 1.0 / magy;
  int dw = dest.XDim(), dh = dest.YDim(), dsk = dest.Skip(), nf = dest.Fields();
  int sw = src.XDim(), sh = src.YDim(), sln = src.Line();
  int y0, y1, x0, x1, cnt0, cnt1, x0b, x1b, y0b, y1b, n, i, j, cnt = dw * nf;
  int x, y, ix, iy, up, dn, lf, rt, swf, sef, nwf, nef, v;
  UC8 *d = dest.PxlDest();
  const UC8 *s0, *s;
  int *pos, *mix;

  // determine destination y limits for interpolation (inclusive)
  dy0 = 0.5 * (dh - 1) - cy * magy;
  dy1 = dy0 + (sh - 2) * magy;           // was -1
  y0 = __max(0, (int) ceil(dy0));
  y1 = __min((int) floor(dy1), dh - 1); 

  // determine destination x limits for interpolation (inclusive)
  dx0 = 0.5 * (dw - 1) - cx * magx;
  dx1 = dx0 + (sw - 2) * magx;           // was -1
  x0 = __max(0, (int) ceil(dx0));
  x1 = __min((int) floor(dx1), dw - 1); 
  cnt0 = x0 * nf;
  cnt1 = (x1 + 1) * nf;

  // find border regions of image
  y0b = __max(0, y0 - ROUND(0.5 * magy));
  y1b = __min(y1 + ROUND(0.5 * magy), dh - 1);
  x0b = __max(0, x0 - ROUND(0.5 * magx));
  x1b = __min(x1 + ROUND(0.5 * magx), dw - 1);

  // allocate arrays for sampling control
  n = x1 - x0 + 1;
  pos = (int *) new int[n];
  mix = (int *) new int[n];

  // fill in x sampling position and mixing coefficients
  sx0 = cx + (x0 - 0.5 * (dw - 1)) * xstep;
  for (i = 0, sx = sx0; i < n; i++, sx += xstep)
  {
    ix = (int) floor(sx);
    pos[i] = ix * nf;
    mix[i] = ROUND(256.0 * (sx - ix));
  }

  // -----------------BOTTOM--------------------
  // possible blank lines at bottom of image
  for (y = 0; y < y0; y++, d += dsk)
    for (i = 0; i < cnt; i++, d++)
      *d = 0;

  // left corner pixel duplication

  // interpolation of border pixels

  // right corner pixel duplication

  // -----------------MIDDLE--------------------
  // valid portion for sampling
  sy0 = cy + (y0 - 0.5 * (dh - 1)) * ystep;
  for (y = y0, sy = sy0; y <= y1; y++, sy += ystep, d += dsk)
  {
    // possible blank pixels at beginning of line
    for (i = 0; i < cnt0; i++, d++)
      *d = 0;

    // get mixing coefficients for adjacent lines
    iy = (int) floor(sy);
    up = ROUND(256.0 * (sy - iy));
    dn = 256 - up;

    // left edge pixels

    // interpolate in the middle
    s0 = src.PxlSrc() + iy * sln;
    for (i = 0; i < n; i++, d += nf)
    { 
      // figure out cascaded coefficients 
      rt = mix[i];
      lf = 256 - rt;
      swf = dn * lf;
      sef = dn * rt;
      nwf = up * lf;
      nef = up * rt;

      // mix all fields
      s = s0 + pos[i];
      for (j = 0; j < nf; j++)
      {
        v =  swf * s[j];
        v += sef * s[j + nf];  
        v += nwf * s[j + sln];  
        v += nef * s[j + sln + nf];  
        d[j] = (UC8)(v >> 16);
      }
    }

    // right edge pixels

    // possible blank pixels at end of line
    for (i = cnt1; i < cnt; i++, d++)
      *d = 0;
  }

  // -------------------TOP---------------------
  // left corner pixel duplication

  // interpolation of border pixels

  // right corner pixel duplication

  // possible blank lines at top of image
  for (y = y1 + 1; y < dh; y++, d += dsk)
    for (x = 0; x < cnt; x++, d++)
      *d = 0;

  // clean up
  delete [] mix;
  delete [] pos;
  return 1;
}


//= Copies magnified portion of 16 bit source centered on (cx cy) to destination
// if clipped part of source is too small, part of dest is written to zero

int jhcResize::Resample_16 (jhcImg& dest, const jhcImg &src, double cx, double cy, double xsc, double ysc) const 
{
  // check for valid arguments
  if (!dest.Valid(2) || !src.Valid(dest.Fields()) || src.SameImg(dest) || (xsc <= 0.0))
    return Fatal("Bad images to jhcResize::Resample_16");

  // local variables
  double magx = xsc, magy = ((ysc > 0.0) ? ysc : xsc);
  double dy0, dy1, dx0, dx1, sx0, sy0, sx, sy, xstep = 1.0 / magx, ystep = 1.0 / magy;
  int dw = dest.XDim(), dh = dest.YDim(), dsk = dest.Skip() >> 1;
  int sw = src.XDim(), sh = src.YDim(), sln = src.Line() >> 1;
  int y0, y1, x0, x1, cnt0, cnt1, x0b, x1b, y0b, y1b, n, i; 
  int x, y, ix, iy, up, dn, lf, rt, swf, sef, nwf, nef, v;
  US16 *d = (US16 *) dest.PxlDest();
  const US16 *s0, *s;
  int *pos, *mix;

  // determine destination y limits for interpolation (inclusive)
  dy0 = 0.5 * (dh - 1) - cy * magy;
  dy1 = dy0 + (sh - 1) * magy;
  y0 = __max(0, (int) ceil(dy0));
  y1 = __min((int) floor(dy1), dh - 1); 

  // determine destination x limits for interpolation (inclusive)
  dx0 = 0.5 * (dw - 1) - cx * magx;
  dx1 = dx0 + (sw - 1) * magx;
  x0 = __max(0, (int) ceil(dx0));
  x1 = __min((int) floor(dx1), dw - 1); 
  cnt0 = x0;
  cnt1 = (x1 + 1);

  // find border regions of image
  y0b = __max(0, y0 - ROUND(0.5 * magy));
  y1b = __min(y1 + ROUND(0.5 * magy), dh - 1);
  x0b = __max(0, x0 - ROUND(0.5 * magx));
  x1b = __min(x1 + ROUND(0.5 * magx), dw - 1);

  // allocate arrays for sampling control
  n = x1 - x0 + 1;
  pos = (int *) new int[n];
  mix = (int *) new int[n];

  // fill in x sampling position and mixing coefficients
  sx0 = cx + (x0 - 0.5 * (dw - 1)) * xstep;
  for (i = 0, sx = sx0; i < n; i++, sx += xstep)
  {
    ix = (int) floor(sx);
    pos[i] = ix;
    mix[i] = ROUND(256.0 * (sx - ix));
  }

  // -----------------BOTTOM--------------------
  // possible blank lines at bottom of image
  for (y = 0; y < y0; y++, d += dsk)
    for (i = 0; i < dw; i++, d++)
      *d = 0;

  // left corner pixel duplication

  // interpolation of border pixels

  // right corner pixel duplication

  // -----------------MIDDLE--------------------
  // valid portion for sampling
  sy0 = cy + (y0 - 0.5 * (dh - 1)) * ystep;
  for (y = y0, sy = sy0; y <= y1; y++, sy += ystep, d += dsk)
  {
    // possible blank pixels at beginning of line
    for (i = 0; i < cnt0; i++, d++)
      *d = 0;

    // get mixing coefficients for adjacent lines
    iy = (int) floor(sy);
    up = ROUND(256.0 * (sy - iy));
    dn = 256 - up;

    // left edge pixels

    // interpolate in the middle
    s0 = ((US16 *) src.PxlSrc()) + iy * sln;
    for (i = 0; i < n; i++, d++)
    { 
      // figure out cascaded coefficients 
      rt = mix[i];
      lf = 256 - rt;
      swf = dn * lf;
      sef = dn * rt;
      nwf = up * lf;
      nef = up * rt;

      // mix fields
      s = s0 + pos[i];
      v =  swf * s[0];
      v += sef * s[1];  
      v += nwf * s[sln];  
      v += nef * s[sln + 1];  
      d[0] = (US16)(v >> 16);
    }

    // right edge pixels

    // possible blank pixels at end of line
    for (i = cnt1; i < dw; i++, d++)
      *d = 0;
  }

  // -------------------TOP---------------------
  // left corner pixel duplication

  // interpolation of border pixels

  // right corner pixel duplication

  // possible blank lines at top of image
  for (y = y1 + 1; y < dh; y++, d += dsk)
    for (x = 0; x < dw; x++, d++)
      *d = 0;

  // clean up
  delete [] mix;
  delete [] pos;
  return 1;
}


//= Completely fill destination image with full contents of source image.
// Note: only interpolates across 4 neighbors so aliasing might occur
// uses bilinear interpolation to suppress blocking artifacts
// borders of images are often duplicates of pixels one further in
// uses separate x and y scaling factors if conform is positive

int jhcResize::FillFrom (jhcImg& dest, const jhcImg& src, int conform) const 
{
  double magx = dest.XDim() / (double) src.XDim();
  double magy = dest.YDim() / (double) src.YDim();

  if (!dest.Valid(src.Fields())) 
    return Fatal("Bad images to jhcResize::FillFrom");
  if (dest.SameFormat(src))
    return dest.CopyArr(src);

  // scales with single factor so full source image will fit into destination
  if (conform <= 0)
  {
    magx = __min(magx, magy);
    magy = magx;
  }

  // do actual sampling and possibly patch borders
  if (dest.Valid(2))
  {
    Resample_16(dest, src, 0.5 * src.XLim(), 0.5 * src.YLim(), magx, magy);
    edge_dup_16(dest, ROUND(0.5 * magx), ROUND(0.5 * magy));
  }
  else
  {
    Resample(dest, src, 0.5 * src.XLim(), 0.5 * src.YLim(), magx, magy);
    edge_dup(dest, ROUND(0.5 * magx), ROUND(0.5 * magy));
  }
  return 1;
}


//= Copy pixels n in from the edge all the way to the edge.
// can copy differently on horizontal and vertical borders

void jhcResize::edge_dup (jhcImg& dest, int nx, int ny) const 
{
  int w = dest.XDim(), h = dest.YDim(), nf = dest.Fields(), ln = dest.Line();
  int i, j, x, y, cnt = nf * w, nxf = nf * nx, xsrc = nf * (w - 1) - nxf;
  const UC8 *s;
  UC8 *d, *lf0, *rt0, *lf, *rt;

  // bottom boundary
  if (ny > 0)
  {
    s = dest.RoiSrc(0, ny);
    d = dest.RoiDest(0, 0);
    for (i = ny; i > 0; i--, d += ln)
      for (x = 0; x < cnt; x++)
        d[x] = s[x];
  }

  // left and right boundary
  if (nx > 0)  
  {
    s = dest.RoiSrc(0, 0);
    lf0 = dest.RoiDest(0, 0);
    rt0 = dest.RoiDest(w - 1, 0);
    for (y = h; y > 0; y--, s += ln, lf0 += ln, rt0 += ln)
    {
      lf = lf0;
      rt = rt0;
      for (i = 0; i < nx; i++, lf += nf, rt -= nf)
        for (j = 0; j < nf; j++)
        {
          lf[j] = s[nxf + j];
          rt[j] = s[xsrc + j];
        }
    }
  }

  // top boundary
  if (ny > 0)
  {
    s = dest.RoiSrc(0, h - ny - 1);
    d = dest.RoiDest(0, h - ny);
    for (i = ny; i > 0; i--, d += ln)
      for (x = 0; x < cnt; x++)
        d[x] = s[x];
  }
}


//= Copy pixels n in from the edge all the way to the edge for a 16 bit image.
// can copy differently on horizontal and vertical borders

void jhcResize::edge_dup_16 (jhcImg& dest, int nx, int ny) const 
{
  int w = dest.XDim(), h = dest.YDim(), ln = dest.Line() >> 1;
  int i, x, y, xsrc = (w - 1) - nx;
  const US16 *s;
  US16 *d, *lf0, *rt0, *lf, *rt;

  // bottom boundary
  if (ny > 0)
  {
    s = (US16 *) dest.RoiSrc(0, ny);
    d = (US16 *) dest.RoiDest(0, 0);
    for (i = ny; i > 0; i--, d += ln)
      for (x = 0; x < w; x++)
        d[x] = s[x];
  }

  // left and right boundary
  if (nx > 0)  
  {
    s = (US16 *) dest.RoiSrc(0, 0);
    lf0 = (US16 *) dest.RoiDest(0, 0);
    rt0 = (US16 *) dest.RoiDest(w - 1, 0);
    for (y = h; y > 0; y--, s += ln, lf0 += ln, rt0 += ln)
    {
      lf = lf0;
      rt = rt0;
      for (i = 0; i < nx; i++, lf++, rt--)
      {
        *lf = s[nx];
        *rt = s[xsrc];
      }
    }
  }

  // top boundary
  if (ny > 0)
  {
    s = (US16 *) dest.RoiSrc(0, h - ny - 1);
    d = (US16 *) dest.RoiDest(0, h - ny);
    for (i = ny; i > 0; i--, d += ln)
      for (x = 0; x < w; x++)
        d[x] = s[x];
  }
}


//= Use bi-cubic convolution to give high-quality resampling of image.
// uses separate x and y scaling factors if conform is positive
// about 3x slower than bi-linear FillFrom

int jhcResize::Bicubic (jhcImg& dest, const jhcImg& src, int conform)
{
  int nf = src.Fields();

  if ((nf < 1) || (nf > 3) || !dest.Valid(nf)) 
    return Fatal("Bad images to jhcResize::Bicubic");
  dest.MaxRoi();
  if (dest.SameFormat(src))
    return dest.CopyArr(src);
  if (nf == 1)
    return Bicubic_BW(dest, src, conform);
  if (nf == 2)
    return Bicubic_16(dest, src, conform);
  return Bicubic_RGB(dest, src, conform);
}


//= Use bi-cubic convolution to give high-quality resampling of monochrome image.
// uses separate x and y scaling factors if conform is positive
// <pre>
//
// determine value of output pixel v between input pixels p1 and p2:
//
// source points:   [p0]           [p1]           [p2]           [p3]
//    dest point:     |              |--x--> [v]    |              |
//
//   v = a * x^3 + b * x^2 + c * x + d
//
// where:
//
//   a = 0.5 * (   -p0 + 3 * p1 - 3 * p2 + p3)
//   b = 0.5 * (2 * p0 - 5 * p1 + 4 * p2 - p3)
//   c = 0.5 * (-p0 + p2)
//   d = p1
//
// at far left side:   p0 = 2 * p1 - p2
// at far right side:  p3 = 2 * p2 - p1
//
// </pre>
// interpolation done in x direction first then repeated in y direction
// slower than bi-linear FillFrom, always samples whole image (ignores ROI)

int jhcResize::Bicubic_BW (jhcImg& dest, const jhcImg& src, int conform)
{
  double stepx, stepy, xf, yf;
  int dw = dest.XDim(), dh = dest.YDim(), sw = src.XDim(), sh = src.YDim();
  int dsk = dest.Skip(), sln = src.Line(), xlim = sw - 2, ylim = sh - 2;
  int x, y, v, xi, yi, xp, yp, dx, dy, p0, p1, p2, p3, a, b, c, d;
  int *t, *t0;
  const UC8 *s, *s0 = src.PxlSrc();
  UC8 *r = dest.PxlDest();

  // scales with single factor so full source image will fit into destination
  stepx = sw / (double)(dw + 1);
  stepy = sh / (double)(dh + 1);
  if (conform <= 0)
  {
    stepx = __max(stepx, stepy);
    stepy = stepx;
  }

  // set up temporary array (32 bit signed integer)
  alloc(sln * dh);

  // PASS 1 - do vertical interpolation of source into temporary array
  t0 = temp;
  for (x = 0; x < sw; x++, s0++, t0++)
  {
    // set up bottom side values (will be shuffled down)
    s = s0;
    t = t0;
    p2 = s[0];
    p3 = s[sln];
    p1 = (p2 << 1) - p3;
    s += (sln + sln);

    // start generating output pixels
    yp = -1;
    yf = 0.0;
    for (y = 0; y < dh; y++, t += sw, yf += stepy)
    {
      // see if moved into next gap between pixels
      yi = (int) yf;
      if (yp < yi)
      {
        // get new local window
        while (yp < yi)
        {
          // shuffle points
          p0 = p1;
          p1 = p2;
          p2 = p3;

          // check for top side
          if (yi < ylim)
            p3 = *s;
          else
            p3 = (p2 << 1) - p1;

          // advance source pointer
          s += sln;
          yp++;
        }

        // recompute coefficients
        a = -p0 + 3 * (p1 - p2) + p3;                     // a * 2
        b = ((p0 << 1) - 5 * p1 + (p2 << 2) - p3) << 8;   // b * 2^9
        c = (p2 - p0) << 8;                               // c * 2^9
        d = p1 << 8;                                      // d * 2^8
      }

      // compute new roughly 16 bit signed intermediate pixel values
      // Note: really should only be 0-65535 but much slower to clamp
      // v = a * y^3 + b * y^2 + c * y + d
      //   = ((a * y + b) * y + c) * y + d
      dy = ROUND(256.0 * (yf - yi));
      v = a * dy + b;               
      v = ((v * dy) >> 8) + c;
      v = ((v * dy) >> 9) + d;
      *t = v;
    }
  }

  // PASS 2 - do horizontal interpolation of temporary array into destination
  t0 = temp;
  for (y = 0; y < dh; y++, r += dsk, t0 += sw)
  {
    // set up far left side values (will be shuffled down)
    t = t0;
    p2 = t[0];
    p3 = t[1];
    p1 = (p2 << 1) - p3;
    t += 2;

    // start generating intermediate pixels
    xp = -1;
    xf = 0.0;
    for (x = 0; x < dw; x++, r++, xf += stepx)
    {
      // see if moved into next gap between pixels
      xi = (int) xf;
      if (xp < xi)
      {
        // get new local window
        while (xp < xi)
        {
          // shuffle points
          p0 = p1;
          p1 = p2;
          p2 = p3;

          // check for far right side
          if (xi < xlim)
            p3 = *t;
          else
            p3 = (p2 << 1) - p1;

          // advance source pointer
          t++;
          xp++;
        }

        // recompute coefficients (with pixels * 2^8)
        a = (-p0 + 3 * (p1 - p2) + p3) >> 8;        // a * 2
        b = (p0 << 1) - 5 * p1 + (p2 << 2) - p3;    // b * 2^9
        c = p2 - p0;                                // c * 2^9
        d = p1;                                     // d * 2^8
      }

      // compute new 8 bit unsigned output pixel values
      // v = a * x^3 + b * x^2 + c * x + d
      //   = ((a * x + b) * x + c) * x + d
      dx = ROUND(256.0 * (xf - xi));
      v = a * dx + b;               
      v = ((v * dx) >> 8) + c;
      v = ((v * dx) >> 9) + d;
      *r = BOUND(v >> 8);
    }
  }
  return 1;
}


//= Use bi-cubic convolution to give high-quality resampling of 16 bit image.
// uses separate x and y scaling factors if conform is positive

int jhcResize::Bicubic_16 (jhcImg& dest, const jhcImg& src, int conform)
{
  double stepx, stepy, xf, yf;
  int dw = dest.XDim(), dh = dest.YDim(), sw = src.XDim(), sh = src.YDim();
  int dsk = dest.Skip() >> 1, sln = src.Line() >> 1, xlim = sw - 2, ylim = sh - 2;
  int x, y, v, xi, yi, xp, yp, dx, dy, p0, p1, p2, p3, a, b, c, d;
  int *t, *t0;
  const US16 *s, *s0 = (const US16 *) src.PxlSrc();
  US16 *r = (US16 *) dest.PxlDest();

  // scales with single factor so full source image will fit into destination
  stepx = sw / (double)(dw + 1);
  stepy = sh / (double)(dh + 1);
  if (conform <= 0)
  {
    stepx = __max(stepx, stepy);
    stepy = stepx;
  }

  // set up temporary array (32 bit signed integer)
  alloc(sln * dh);

  // PASS 1 - do vertical interpolation of source into temporary array
  t0 = temp;
  for (x = 0; x < sw; x++, s0++, t0++)
  {
    // set up bottom side values (will be shuffled down)
    s = s0;
    t = t0;
    p2 = s[0];
    p3 = s[sln];
    p1 = (p2 << 1) - p3;
    s += (sln + sln);

    // start generating output pixels
    yp = -1;
    yf = 0.0;
    for (y = 0; y < dh; y++, t += sw, yf += stepy)
    {
      // see if moved into next gap between pixels
      yi = (int) yf;
      if (yp < yi)
      {
        // get new local window
        while (yp < yi)
        {
          // shuffle points
          p0 = p1;
          p1 = p2;
          p2 = p3;

          // check for top side
          if (yi < ylim)
            p3 = *s;
          else
            p3 = (p2 << 1) - p1;

          // advance source pointer
          s += sln;
          yp++;
        }

        // recompute coefficients
        a = -p0 + 3 * (p1 - p2) + p3;              // a * 2
        b = (p0 << 1) - 5 * p1 + (p2 << 2) - p3;   // b * 2
        c = p2 - p0;                               // c * 2
        d = p1;                                    // d
      }

      // compute new roughly 16 bit signed intermediate pixel values
      // Note: really should only be 0-65535 but much slower to clamp
      // v = a * y^3 + b * y^2 + c * y + d
      //   = ((a * y + b) * y + c) * y + d
      dy = ROUND(256.0 * (yf - yi));
      v = (a * dy + b) >> 8;               
      v = ((v * dy) >> 8) + c;
      v = ((v * dy) >> 9) + d;
      *t = v;
    }
  }

  // PASS 2 - do horizontal interpolation of temporary array into destination
  t0 = temp;
  for (y = 0; y < dh; y++, r += dsk, t0 += sw)
  {
    // set up far left side values (will be shuffled down)
    t = t0;
    p2 = t[0];
    p3 = t[1];
    p1 = (p2 << 1) - p3;
    t += 2;

    // start generating intermediate pixels
    xp = -1;
    xf = 0.0;
    for (x = 0; x < dw; x++, r++, xf += stepx)
    {
      // see if moved into next gap between pixels
      xi = (int) xf;
      if (xp < xi)
      {
        // get new local window
        while (xp < xi)
        {
          // shuffle points
          p0 = p1;
          p1 = p2;
          p2 = p3;

          // check for far right side
          if (xi < xlim)
            p3 = *t;
          else
            p3 = (p2 << 1) - p1;

          // advance source pointer
          t++;
          xp++;
        }

        // recompute coefficients (with pixels * 2^8)
        a = (-p0 + 3 * (p1 - p2) + p3) >> 8;        // a * 2
        b = (p0 << 1) - 5 * p1 + (p2 << 2) - p3;    // b * 2^9
        c = p2 - p0;                                // c * 2^9
        d = p1;                                     // d * 2^8
      }

      // compute new 16 bit unsigned output pixel values
      // v = a * x^3 + b * x^2 + c * x + d
      //   = ((a * x + b) * x + c) * x + d
      dx = ROUND(256.0 * (xf - xi));
      v = a * dx + b;               
      v = ((v * dx) >> 8) + c;
      v = ((v * dx) >> 9) + d;
      *r = (US16) __max(0, __min(v, 65535));
    }
  }
  return 1;
}


//= Use bi-cubic convolution to give high-quality resampling of color image.
// uses separate x and y scaling factors if conform is positive

int jhcResize::Bicubic_RGB (jhcImg& dest, const jhcImg& src, int conform)
{
  int p0[3], p1[3], p2[3], p3[3], a[3], b[3], c[3], d[3];
  double stepx, stepy, xf, yf;
  int dw = dest.XDim(), dh = dest.YDim(), sw = src.XDim(), sh = src.YDim();
  int dsk = dest.Skip(), sln = src.Line(), tln = 3 * sw, xlim = sw - 2, ylim = sh - 2;
  int x, y, f, v, xi, yi, xp, yp, dx, dy;
  int *t, *t0;
  const UC8 *s, *s0 = src.PxlSrc();
  UC8 *r = dest.PxlDest();

  // scales with single factor so full source image will fit into destination
  stepx = sw / (double)(dw + 1);
  stepy = sh / (double)(dh + 1);
  if (conform <= 0)
  {
    stepx = __max(stepx, stepy);
    stepy = stepx;
  }

  // set up temporary array (32 bit signed integer)
  alloc(sln * dh);

  // PASS 1 - do vertical interpolation of source into temporary array
  t0 = temp;
  for (x = 0; x < sw; x++, s0 += 3, t0 += 3)
  {
    // set up bottom side values (will be shuffled down)
    s = s0;
    t = t0;
    for (f = 0; f < 3; f++, s++)
    {
      p2[f] = s[0];
      p3[f] = s[sln];
      p1[f] = (p2[f] << 1) - p3[f];
    }
    s += (sln + sln - 3);

    // start generating output pixels
    yp = -1;
    yf = 0.0;
    for (y = 0; y < dh; y++, t += tln, yf += stepy)
    {
      // see if moved into next gap between pixels
      yi = (int) yf;
      if (yp < yi)
      {
        // get new local window
        while (yp < yi)
        {
          for (f = 0; f < 3; f++)
          {
            // shuffle points
            p0[f] = p1[f];
            p1[f] = p2[f];
            p2[f] = p3[f];

            // check for top side
            if (yi < ylim)
              p3[f] = s[f];
            else
              p3[f] = (p2[f] << 1) - p1[f];
          }

          // advance source pointer
          s += sln;
          yp++;
        }

        // recompute coefficients
        for (f = 0; f < 3; f++)
        {
          a[f] = -p0[f] + 3 * (p1[f] - p2[f]) + p3[f];                     // a * 2
          b[f] = ((p0[f] << 1) - 5 * p1[f] + (p2[f] << 2) - p3[f]) << 8;   // b * 2^9
          c[f] = (p2[f] - p0[f]) << 8;                                     // c * 2^9
          d[f] = p1[f] << 8;                                               // d * 2^8
        }
      }

      // compute new roughly 16 bit signed intermediate pixel values
      // Note: really should only be 0-65535 but much slower to clamp
      dy = ROUND(256.0 * (yf - yi));
      for (f = 0; f < 3; f++)
      {
        // v = a * y^3 + b * y^2 + c * y + d
        //   = ((a * y + b) * y + c) * y + d
        v = a[f] * dy + b[f];               
        v = ((v * dy) >> 8) + c[f];
        v = ((v * dy) >> 9) + d[f];
        t[f] = v;
      }
    }
  }

  // PASS 2 - do horizontal interpolation of temporary array into destination
  t0 = temp;
  for (y = 0; y < dh; y++, r += dsk, t0 += tln)
  {
    // set up far left side values (will be shuffled down)
    t = t0;
    for (f = 0; f < 3; f++, t++)
    {
      p2[f] = t[0];
      p3[f] = t[3];
      p1[f] = (p2[f] << 1) - p3[f];
    }
    t += 3;

    // start generating intermediate pixels
    xp = -1;
    xf = 0.0;
    for (x = 0; x < dw; x++, r += 3, xf += stepx)
    {
      // see if moved into next gap between pixels
      xi = (int) xf;
      if (xp < xi)
      {
        // get new local window
        while (xp < xi)
        {
          for (f = 0; f < 3; f++)
          {
            // shuffle points
            p0[f] = p1[f];
            p1[f] = p2[f];
            p2[f] = p3[f];

            // check for far right side
            if (xi < xlim)
              p3[f] = t[f];
            else
              p3[f] = (p2[f] << 1) - p1[f];
          }

          // advance source pointer
          t += 3;
          xp++;
        }

        // recompute coefficients (with pixels * 2^8)
        for (f = 0; f < 3; f++)
        {
          a[f] = (-p0[f] + 3 * (p1[f] - p2[f]) + p3[f]) >> 8;        // a * 2
          b[f] = (p0[f] << 1) - 5 * p1[f] + (p2[f] << 2) - p3[f];    // b * 2^9
          c[f] = p2[f] - p0[f];                                      // c * 2^9
          d[f] = p1[f];                                              // d * 2^8
        }
      }

      // compute new 8 bit unsigned output pixel values
      dx = ROUND(256.0 * (xf - xi));
      for (f = 0; f < 3; f++)
      {
        // v = a * x^3 + b * x^2 + c * x + d
        //   = ((a * x + b) * x + c) * x + d
        v = a[f] * dx + b[f];               
        v = ((v * dx) >> 8) + c[f];
        v = ((v * dx) >> 9) + d[f];
        r[f] = BOUND(v >> 8);
      }
    }
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Rigid Transforms                            //
///////////////////////////////////////////////////////////////////////////

//= Sample closest monochrome pixel after moving center then rotating.
// source point (px py) goes to dest point (cx cy) THEN rotate around (cx cy) 
// source steps along new axes are xsc and ysc, unknown source pixels get value def
// for final rotation A the sampling position is:
//   sx = [(dx - cx) * xsc * cos A] + [(dy - cy) * ysc * sin A] + px
//      =  dx * (xsc * cos A) + dy * (ysc * sin A) + [px - cx * (xsc * cos A) - cy * (ysc * sin A)]
//   sy = [-(dx - cx) * xsc * sin A] + [(dy - cy) * ysc * cos A] + py
//      = -dx * (xsc * sin A) + dy * (ysc * cos A) + [py + cx * (xsc * sin A) - cy * (ysc * cos A)] 
// where (sx sy) is the source pixel and (dx dy) is the destination pixel
// about 1.1ms for 50x100 dest with crop on 3.2GHz Pentium (2.3x faster than Mix)

int jhcResize::Rigid (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy,                    
                      double px, double py, int def, double xsc, double ysc) const
{
  double rads = D2R * degs, c = cos(rads), s = sin(rads);
  double xcos = xsc * c, ycos = ysc * c, xsin = xsc * s, ysin = ysc * s;
  int iyc = ROUND(65536.0 * ycos), iys = ROUND(65536.0 * ysin);
  int ixc = ROUND(65536.0 * xcos), ixs = ROUND(65536.0 * xsin);
  int isx, isx0 = ROUND(65536.0 * (px - cx * xcos - cy * ysin));
  int isy, isy0 = ROUND(65536.0 * (py + cx * xsin - cy * ycos));
  int w = dest.XDim(), h = dest.YDim(), dln = dest.Line();
  int x, y, ix, iy, sw = src.XDim(), sh = src.YDim();
  UC8 *d, *d0 = dest.PxlDest();

  if (!dest.Valid(1) || !src.Valid(1) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::Rigid");
  dest.FillArr(def);

  // copy closest source pixel
  for (y = h; y > 0; y--, d0 += dln, isx0 += iys, isy0 += iyc)
  {
    isx = isx0;
    isy = isy0;
    d = d0;
    for (x = w; x > 0; x--, d++, isx += ixc, isy -= ixs)
    {
      // make sure valid source pixel
      ix = (isx + 32768) >> 16;
      iy = (isy + 32768) >> 16;
      if ((ix >= 0) && (iy >= 0) && (ix < sw) && (iy < sh))
        *d = (UC8) src.ARef(ix, iy);
    }
  }
  return 1;
}


//= Sample closest color pixel after moving center then rotating.
// source point (px py) goes to dest point (cx cy) then rotate around (cx cy) 
// source steps along new axes are xsc and ysc, unknown source pixels get value (r g b)
// about 1.1ms for 50x100 dest with crop on 3.2GHz Pentium (2.3x faster than Mix)
// for linear interpolation rather than sampling use ExtRotateRGB

int jhcResize::RigidRGB (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy, 
                         double px, double py, int r, int g, int b, double xsc, double ysc) const
{
  double rads = D2R * degs, c = cos(rads), s = sin(rads);
  double xcos = xsc * c, ycos = ysc * c, xsin = xsc * s, ysin = ysc * s;
  double sx, sy, sx0 = px - cx * xcos - cy * ysin, sy0 = py + cx * xsin - cy * ycos;
  int w = dest.XDim(), h = dest.YDim(), dln = dest.Line();
  int x, y, ix, iy, sw = src.XDim(), sh = src.YDim(), sln = src.Line();
  const UC8 *p, *p0 = src.PxlSrc();
  UC8 *d, *d0 = dest.PxlDest();

  if (!dest.Valid(3) || !src.Valid(3) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::RigidRGB");

  // copy closest source pixel
  for (y = 0; y < h; y++, d0 += dln, sx0 += ysin, sy0 += ycos)
  {
    sx = sx0;
    sy = sy0;
    d = d0;
    for (x = 0; x < w; x++, d += 3, sx += xcos, sy -= xsin)
    {
      // make sure valid source pixel
      ix = (int)(sx);
      iy = (int)(sy);
      if ((ix >= 0) && (iy >= 0) && (ix < sw) && (iy < sh))
      {
        p = p0 + (ix + ix + ix) + iy * sln;
        d[0] = p[0];
        d[1] = p[1];
        d[2] = p[2];
      }
      else
      {
        d[0] = (UC8) b;
        d[1] = (UC8) g;
        d[2] = (UC8) r;
      }
    }
  }
  return 1;
}


//= Bilinear interpolate monochrome pixel after moving center then rotating.
// source point (px py) goes to dest point (cx cy) then rotate around (cx cy) 
// source steps along new axes are xsc and ysc, unknown source pixels get value def
// about 2.5ms for 50x100 dest with crop on 3.2GHz Pentium

int jhcResize::RigidMix (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy, 
                         double px, double py, int def, double xsc, double ysc) const
{
  double rads = D2R * degs, c = cos(rads), s = sin(rads);
  double xcos = xsc * c, ycos = ysc * c, xsin = xsc * s, ysin = ysc * s;
  double sx, sy, sx0 = px - cx * xcos - cy * ysin, sy0 = py + cx * xsin - cy * ycos;
  int sum, off, f, w = dest.XDim(), h = dest.YDim(), dln = dest.Line();
  int x, y, ix, iy, fx, fy, xlim = src.XLim(), ylim = src.YLim(), sln = src.Line();
  const UC8 *p0 = src.PxlSrc();
  UC8 *d, *d0 = dest.PxlDest();

  if (!dest.Valid(1) || !src.Valid(1) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::RigidMix");

  // shift sx and sy up by 8 bits
  sx0  *= 256.0;
  sy0  *= 256.0;
  xcos *= 256.0;
  ycos *= 256.0;
  xsin *= 256.0;
  ysin *= 256.0;

  // copy closest source pixel
  for (y = 0; y < h; y++, d0 += dln, sx0 += ysin, sy0 += ycos)
  {
    sx = sx0;
    sy = sy0;
    d = d0;
    for (x = 0; x < w; x++, d++, sx += xcos, sy -= xsin)
    {
      // get integer pixel address and fraction (for mixing)
      ix = ((int) sx) >> 8;
      iy = ((int) sy) >> 8;
      fx = (int)(sx - (ix << 8));
      fy = (int)(sy - (iy << 8));
      
      // find corner of lower row for 4 pixel patch in source
      off = ix + iy * sln;
      sum = 0;                         // truncate, not round

      // add in SW pixel if it exists
      f = (256 - fx) * (256 - fy);
      if ((ix >= 0) && (ix <= xlim) && (iy >= 0) && (iy <= ylim))
        sum += f * (*(p0 + off));
      else
        sum += f * def;

      // add in SE pixel if it exists
      f = fx * (256 - fy);
      if ((ix >= -1) && (ix < xlim) && (iy >= 0) && (iy <= ylim))
        sum += f * (*(p0 + off + 1));
      else
        sum += f * def;

      // adjust to upper row of 4 pixel patch in source
      off += sln;
      iy++;

      // add in NW pixel if it exists
      f = (256 - fx) * fy;
      if ((ix >= 0) && (ix <= xlim) && (iy >= 0) && (iy <= ylim))
        sum += f * (*(p0 + off));
      else
        sum += f * def;

      // add in NE pixel if it exists
      f = fx * fy;
      if ((ix >= -1) && (ix < xlim) && (iy >= 0) && (iy <= ylim))
        sum += f * (*(p0 + off + 1));
      else
        sum += f * def;

      // normalize to complete mix
      *d = (UC8)(sum >> 16);
    }
  }
  return 1;
}


//= Bilinear interpolate color pixel after moving center then rotating.
// source point (px py) goes to dest point (cx cy) then rotate around (cx cy) 
// source steps along new axes are xsc and ysc, unknown source pixels get value (r g b)
// about 2.5ms for 50x100 dest with crop on 3.2GHz Pentium

int jhcResize::RigidMixRGB (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy, 
                            double px, double py, int r, int g, int b, double xsc, double ysc) const
{
  double rads = D2R * degs, c = cos(rads), s = sin(rads);
  double xcos = xsc * c, ycos = ysc * c, xsin = xsc * s, ysin = ysc * s;
  double sx, sy, sx0 = px - cx * xcos - cy * ysin, sy0 = py + cx * xsin - cy * ycos;
  int rsum, gsum, bsum, f, w = dest.XDim(), h = dest.YDim(), dln = dest.Line();
  int x, y, ix, iy, fx, fy, off, xlim = src.XLim(), ylim = src.YLim(), sln = src.Line();
  const UC8 *p, *p0 = src.PxlSrc();
  UC8 *d, *d0 = dest.PxlDest();

  if (!dest.Valid(3) || !src.Valid(3) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::RigidMixRGB");

  // shift sx and sy up by 8 bits
  sx0  *= 256.0;
  sy0  *= 256.0;
  xcos *= 256.0;
  ycos *= 256.0;
  xsin *= 256.0;
  ysin *= 256.0;

  // copy closest source pixel
  for (y = 0; y < h; y++, d0 += dln, sx0 += ysin, sy0 += ycos)
  {
    sx = sx0;
    sy = sy0;
    d = d0;
    for (x = 0; x < w; x++, d += 3, sx += xcos, sy -= xsin)
    {
      // get integer pixel address and fraction (for mixing)
      ix = ((int) sx) >> 8;
      iy = ((int) sy) >> 8;
      fx = (int)(sx - (ix << 8));
      fy = (int)(sy - (iy << 8));
      
      // find corner of lower row for 4 pixel patch in source
      off = (ix + ix + ix) + iy * sln;
      bsum = 0;                        // truncate, not round
      gsum = 0;
      rsum = 0;

      // add in SW pixel if it exists
      f = (256 - fx) * (256 - fy);
      if ((ix >= 0) && (ix <= xlim) && (iy >= 0) && (iy <= ylim))
      {
        p = p0 + off;
        bsum += f * p[0];
        gsum += f * p[1];
        rsum += f * p[2];
      }
      else
      {
        bsum += f * b;
        gsum += f * g;
        rsum += f * r;
      }

      // add in SE pixel if it exists
      f = fx * (256 - fy);
      if ((ix >= -1) && (ix < xlim) && (iy >= 0) && (iy <= ylim))
      {
        p = p0 + off + 3;
        bsum += f * p[0];
        gsum += f * p[1];
        rsum += f * p[2];
      }
      else
      {
        bsum += f * b;
        gsum += f * g;
        rsum += f * r;
      }

      // adjust to upper row of 4 pixel patch in source
      off += sln;
      iy++;

      // add in NW pixel if it exists
      f = (256 - fx) * fy;
      if ((ix >= 0) && (ix <= xlim) && (iy >= 0) && (iy <= ylim))
      {
        p = p0 + off;
        bsum += f * p[0];
        gsum += f * p[1];
        rsum += f * p[2];
      }
      else
      {
        bsum += f * b;
        gsum += f * g;
        rsum += f * r;
      }

      // add in NE pixel if it exists
      f = fx * fy;
      if ((ix >= -1) && (ix < xlim) && (iy >= 0) && (iy <= ylim))
      {
        p = p0 + off + 3;
        bsum += f * p[0];
        gsum += f * p[1];
        rsum += f * p[2];
      }
      else
      {
        bsum += f * b;
        gsum += f * g;
        rsum += f * r;
      }

      // normalize to complete mix
      d[0] = (UC8)(bsum >> 16);
      d[1] = (UC8)(gsum >> 16);
      d[2] = (UC8)(rsum >> 16);
    }
  }
  return 1;
}


//= Bilinear interpolate monochrome pixel from non-zero neighbors after moving center then rotating.
// source point (px py) goes to dest point (cx cy) then rotate around (cx cy) 
// source steps along new axes are xsc and ysc

int jhcResize::RigidMixNZ (jhcImg& dest, const jhcImg& src, double degs, double cx, double cy, 
                           double px, double py, double xsc, double ysc) const
{
  double rads = D2R * degs, c = cos(rads), s = sin(rads);
  double xcos = xsc * c, ycos = ysc * c, xsin = xsc * s, ysin = ysc * s;
  double sx, sy, sx0 = px - cx * xcos - cy * ysin, sy0 = py + cx * xsin - cy * ycos;
  int sum, norm, v, off, f, w = dest.XDim(), h = dest.YDim(), dln = dest.Line();
  int x, y, ix, iy, fx, fy, xlim = src.XLim(), ylim = src.YLim(), sln = src.Line();
  const UC8 *p0 = src.PxlSrc();
  UC8 *d, *d0 = dest.PxlDest();

  if (!dest.Valid(1) || !src.Valid(1) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::RigidMixNZ");

  // shift sx and sy up by 8 bits
  sx0  *= 256.0;
  sy0  *= 256.0;
  xcos *= 256.0;
  ycos *= 256.0;
  xsin *= 256.0;
  ysin *= 256.0;

  // copy closest source pixel
  for (y = 0; y < h; y++, d0 += dln, sx0 += ysin, sy0 += ycos)
  {
    sx = sx0;
    sy = sy0;
    d = d0;
    for (x = 0; x < w; x++, d++, sx += xcos, sy -= xsin)
    {
      // get integer pixel address and fraction (for mixing)
      ix = ((int) sx) >> 8;
      iy = ((int) sy) >> 8;
      fx = (int)(sx - (ix << 8));
      fy = (int)(sy - (iy << 8));
      
      // find corner of lower row for 4 pixel patch in source
      off = ix + iy * sln;
      sum = 0;
      norm = 0;

      // add in SW pixel if it exists
      f = (256 - fx) * (256 - fy);
      if ((ix >= 0) && (ix <= xlim) && (iy >= 0) && (iy <= ylim))
        if ((v = *(p0 + off)) > 0)
        {
          sum += f * v;
          norm += f;
        }

      // add in SE pixel if it exists
      f = fx * (256 - fy);
      if ((ix >= -1) && (ix < xlim) && (iy >= 0) && (iy <= ylim))
        if ((v = *(p0 + off + 1)) > 0)
        {
          sum += f * v;
          norm += f;
        }

      // adjust to upper row of 4 pixel patch in source
      off += sln;
      iy++;

      // add in NW pixel if it exists
      f = (256 - fx) * fy;
      if ((ix >= 0) && (ix <= xlim) && (iy >= 0) && (iy <= ylim))
        if ((v = *(p0 + off)) > 0) 
        {
          sum += f * v;
          norm += f;
        }

      // add in NE pixel if it exists
      f = fx * fy;
      if ((ix >= -1) && (ix < xlim) && (iy >= 0) && (iy <= ylim))
        if ((v = *(p0 + off + 1)) > 0)
        {
          sum += f * v;
          norm += f;
        }

      // normalize to complete mix (truncate, not round)
      if (norm <= 0)
        *d = 0;
      else
        *d = (UC8)(sum / norm);
    }
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                      Compositing and Insertion                        //
///////////////////////////////////////////////////////////////////////////

//= Copy a (presumably) small image to some area of a larger image.
// ignores ROIs on source and destination

int jhcResize::Insert (jhcImg& dest, const jhcImg& src, int xoff, int yoff) const 
{
  int nf = dest.Fields(); 

  if (!dest.Valid() || !src.Valid(nf))
    return Fatal("Bad images to jhcResize::Insert");

  // general case
  int xlim = dest.XDim(), ylim = dest.YDim();
  int xlast = xoff + src.XDim(), ylast = yoff + src.YDim();
  int x1 = __max(0, __min( xoff, xlim)), y1 = __max(0, __min( yoff, ylim));
  int x2 = __max(0, __min(xlast, xlim)), y2 = __max(0, __min(ylast, ylim));
  int w = nf * (x2 - x1), h = y2 - y1;
  int x, y, ssk = src.Line() - w, dsk = dest.Line() - w ;
  const UC8 *s = src.PxlSrc();
  UC8 *d = dest.PxlDest() + x1 * nf + y1 * dest.Line();

  for (y = h; y > 0; y--)
  {
    for (x = w; x > 0; x--)
      *d++ = *s++;
    s += ssk;
    d += dsk;
  }
  return 1;
}


//= Fill up a small image from a (presumably) larger image.
// ignores ROIs on source and destination

int jhcResize::Extract (jhcImg& dest, const jhcImg& src, int xoff, int yoff) const 
{
  int nf = dest.Fields();

  if (!dest.Valid() || !src.Valid(nf))
    return Fatal("Bad images to jhcResize::Extract");

  // general case
  int sw = src.XDim(), sh = src.YDim();
  int dw = dest.XDim(), dh = dest.YDim();
  int dx0 = __max(0, -xoff), dy0 = __max(0, -yoff);
  int sx0 = __max(0, xoff), sy0 = __max(0, yoff);
  int sx1 = __min(xoff + dw, sw), sy1 = __min(yoff + dh, sh);
  int i, y, w = sx1 - sx0, h = sy1 - sy0, cnt = w * nf;
  int dsk = dest.Line() - cnt, ssk = src.Line() - cnt;
  const UC8 *s = src.PxlSrc() + sy0 * src.Line() + sx0 * nf;
  UC8 *d = dest.PxlDest() + dy0 * dest.Line() + dx0 * nf;

  for (y = h; y > 0; y--)
  {
    for (i = cnt; i > 0; i--)
      *d++ = *s++;
    d += dsk;
    s += ssk;
  }   
  return 1;
}


//= Fill destination with just one field from source starting at given corner.
// if only partial overlap then some pixels unchanged (use FillArr first)

int jhcResize::ExtField (jhcImg& dest, const jhcImg& src, int xoff, int yoff, int f) const 
{
  if (!dest.Valid(1) || !src.Valid(3))
    return Fatal("Bad images to jhcResize::ExtField");

  // general case
  int sw = src.XDim(), sh = src.YDim();
  int dw = dest.XDim(), dh = dest.YDim();
  int dx0 = __max(0, -xoff), dy0 = __max(0, -yoff);
  int sx0 = __max(0, xoff), sy0 = __max(0, yoff);
  int sx1 = __min(xoff + dw, sw), sy1 = __min(yoff + dh, sh);
  int x, y, w = sx1 - sx0, h = sy1 - sy0;
  int dsk = dest.RoiSkip(w), ssk = src.RoiSkip(w);
  const UC8 *s = src.RoiSrc(sx0, sy0) + __max(0, __min(f, 2));
  UC8 *d = dest.RoiDest(dx0, dy0);

  for (y = h; y > 0; y--, d += dsk, s += ssk)
    for (x = w; x > 0; x--, d++, s += 3)
      *d = *s;
  return 1;
}


//= Copy specified region to corner of destination region.
// rest of destination unaffected (might want to clear with FillArr)
// sets ROI of destination image as a side-effect

int jhcResize::CopyPart (jhcImg& dest, const jhcImg& src, 
                         int rx, int ry, int rw, int rh) const 
{
  if (!src.Valid() || !dest.Valid(src.Fields()) || src.SameImg(dest))
    return Fatal("Bad images to jhcResize::CopyPart");

  // figure out valid size to copy
  jhcRoi area;
  int x, y, dsk, ssk, sw = src.XDim(), sh = src.YDim();
  int rx1 = __min(rx, sw - 1),  ry1 = __min(ry, sh - 1);
  int rx2 = __min(rx + rw, sw), ry2 = __min(ry + rh, sh);
  int w = __min(rx2 - rx1, dest.XDim()), h = __min(ry2 - ry1, dest.YDim());
  int cnt = w * src.Fields();
  UC8 *d;
  const UC8 *s;

  // initialize pixel pointers and line stepping
  area.SetRoi(rx1, ry1, w, h);
  dest.SetRoi(0, 0, w, h);
  s = src.RoiSrc(area);
  d = dest.RoiDest();
  ssk = src.RoiSkip(area);
  dsk = dest.RoiSkip();

  // copy pixels in region
  for (y = h; y > 0; y--)
  {
    for (x = cnt; x > 0; x--)
      *d++ = *s++;
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Like other CopyPart but takes a ROI spec as input instead.

int jhcResize::CopyPart (jhcImg& dest, const jhcImg& src, const jhcRoi& a) const 
{
  return CopyPart(dest, src, a.RoiX(), a.RoiY(), a.RoiW(), a.RoiH());
}


//= Fills destination with rotated color patch from source around (cx cy).
// "ang" says how much "dest" vertical is rotated relative to "source" vertical
// does linear interpolation on color, to sample nearest instead use RigidRGB

int jhcResize::ExtRotateRGB (jhcImg& dest, const jhcImg& src, double cx, double cy, double ang) const
{
  double rads = D2R * ang, c = cos(rads), s = sin(rads);
  double dx0 = 0.5 * dest.XLim(), dy0 = 0.5 * dest.YLim();
  int c16 = ROUND(65536.0 * c), s16 = ROUND(65536.0 * s);
  int dx, dy, dw = dest.XDim(), dh = dest.YDim(), dsk = dest.Skip();
  int xlim16 = src.XLim() << 16, ylim16 = src.YLim() << 16, sln = src.Line();
  int sx0, sy0, sx, sy, ix, iy, fx, fy, cfx, cfy, sw, se, nw, ne;
  UC8 *d = dest.PxlDest(); 
  const UC8 *a;

  if (!src.Valid(3) || !dest.Valid(3) || src.SameImg(dest) ||
      (dw >= 327678) || (dh >= 32768) || (src.XLim() >= 32767) || (src.YLim() >= 32767))
    return Fatal("Bad images to jhcResize::ExtRotateRGB");

  // figure out sample position for every output pixel (16 bit fractional part)
  sx0 = ROUND(65536.0 * (cx - dx0 * c + dy0 * s));
  sy0 = ROUND(65536.0 * (cy - dx0 * s - dy0 * c));
  for (dy = dh; dy > 0; dy--, d += dsk, sx0 -= s16, sy0 += c16)
  {
    sx = sx0;
    sy = sy0;
    for (dx = dw; dx > 0; dx--, d += 3, sx += c16, sy += s16)
    {
      // make sure 4 nearest pixels are all inside source
      ix = sx & 0x7FFF0000;
      iy = sy & 0x7FFF0000;
      if ((ix < 0) || (ix >= xlim16) || (iy < 0) || (iy >= ylim16))
      {
        d[0] = 0;
        d[1] = 0;
        d[2] = 0;
        continue;
      }

      // figure out linear blending factors for nearby pixels (20 bits)
      fx  = (sx - ix + 32) >> 6;   
      cfx = 1024 - fx;
      fy  = (sy - iy + 32) >> 6;
      cfy = 1024 - fy;
      sw = cfx * cfy;      // base       
      se =  fx * cfy;      // right
      nw = cfx * fy;       // up
      ne =  fx * fy;       // up and right

      // do for all fields
      a = src.RoiSrc(ix >> 16, iy >> 16);
      d[0] = (UC8)((sw * a[0] + se * a[3] + nw * a[sln]     + ne * a[sln + 3] + 0x00080000) >> 20);
      d[1] = (UC8)((sw * a[1] + se * a[4] + nw * a[sln + 1] + ne * a[sln + 4] + 0x00080000) >> 20);
      d[2] = (UC8)((sw * a[2] + se * a[5] + nw * a[sln + 2] + ne * a[sln + 5] + 0x00080000) >> 20);
    } 
  }
  return 1;
}


//= Extract part of the src image around (cx cy) and possibly magnify.
// if clipped part of source is too small, part of dest is not written

int jhcResize::Zoom (jhcImg& dest, const jhcImg& src, int cx, int cy, int mag) const 
{
  // check for valid arguments
  if (!dest.Valid() || !src.Valid(dest.Fields()) || src.SameImg(dest) || (mag == 0))
    return Fatal("Bad images to jhcResize::Zoom");

  // local variables
  int w2 = dest.XDim() / mag, h2 = dest.YDim() / mag;
  int sln = src.Line(),  dln = dest.Line(), f = dest.Fields(); 
  int x, y, m, m2, i, x0a, y0a, x0, x1, y0, y1, cnt, dsk;
  const UC8 *s, *s0;
  UC8 *d;

  // figure out valid bounds in source
  x0a = cx - w2 / 2;
  y0a = cy - h2 / 2;
  x1 = x0a + w2;
  y1 = y0a + h2;
  x0 = __max(0, x0a);
  x1 = __min(x1, src.XDim());
  y0 = __max(0, y0a);
  y1 = __min(y1, src.YDim());
  cnt = (x1 - x0) * f;
  dsk = dln - mag * cnt;
  s0 = src.PxlSrc() + y0 * sln + x0 * f;
  d = dest.PxlDest() + (y0 - y0a) * mag * dln + (x0 - x0a) * mag * f;

  // duplicate portion of source image
  for (y = y0; y < y1; y++)
  {
    for (m2 = 0; m2 < mag; m2++)
    {
      s = s0;
      for (x = x0; x < x1; x++, s += f)
        for (m = 0; m < mag; m++, d += f)
          for (i = 0; i < f; i++)
            d[i] = s[i];
      d += dsk;
    }
    s0 += sln;
  }
  return 1;
}


//= Return pointer to an image which has no line padding.
// passes original image if already okay, else builds and returns alt image
// shrinks if needed by dropping pixels, required for some OpenCV conversions
// automatically sizes "alt" if needed, never changes "src"

jhcImg *jhcResize::Image4 (jhcImg& alt, jhcImg& src) const
{
  int w = src.XDim(), w4 = w & 0xFFFC;

  if (w == w4)
    return &src;
  alt.SetSize(w4, src.YDim(), src.Fields());
  Extract(alt, src, (w - w4) / 2, 0);
  return &alt;
}


///////////////////////////////////////////////////////////////////////////
//                         Simple Image Reshaping                        //
///////////////////////////////////////////////////////////////////////////

//= Reverses order of each line.
// generally dest must be different from source

int jhcResize::FlipH (jhcImg& dest, const jhcImg& src) const 
{
  if (dest.SameImg(src))
    return FlipH(dest);
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcResize::FlipH");
  dest.CopyRoi(src);

  // general ROI case
  int x, y, i;
  int f = dest.Fields(), rw = dest.RoiW(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = dsk + 2 * f * rw;
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff + f * rw;

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      s -= f;
      for (i = 0; i < f; i++)
        d[i] = s[i];
      d += f;
    }
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Reverse order of lines within same image.

int jhcResize::FlipH (jhcImg& dest) const 
{
  int x, y, i, v;
  int rw = dest.RoiW(), rh = dest.RoiH(), hw = rw / 2; 
  int f = dest.Fields(), line = dest.Line();
  UC8 *head, *tail, *h0, *t0, *r0 = dest.RoiDest(); 

  // start near middle of line
  h0 = r0 + f * hw;
  t0 = r0 + f * (rw - hw);
  for (y = rh; y > 0; y--)
  {
    head = h0;
    tail = t0;
    for (x = hw; x > 0; x--)
    {
      // swap pixel fields
      for (i = 0; i < f; i++)
      {
        v = tail[i]; 
        tail[i] = head[i];
        head[i] = (UC8) v;
      } 

      // move toward edges
      head -= f;
      tail += f;
    }
    h0 += line;
    t0 += line;
  }
  return 1;
}


//= Set destination size to match then reverse lines of source.
// if inv <= then just copies source

int jhcResize::Mirror (jhcImg& dest, const jhcImg& src, int rev) const
{
  dest.SetSize(src); 
  if (rev > 0) 
    return FlipH(dest, src); 
  dest.CopyArr(src);
  return 1;
}


//= Reverses order of columns.
// dest must be different from source

int jhcResize::FlipV (jhcImg& dest, const jhcImg& src) const 
{
  if (!dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::FlipV");
  dest.CopyRoi(src);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH();
  int dsk = dest.RoiSkip(), ssk = dsk - 2 * src.Line();
  UL32 roff = dest.RoiOff();
  UC8 *d = dest.PxlDest() + roff;
  const UC8 *s = src.PxlSrc() + roff + src.Line() * (rh - 1);

  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = *s++;
    d += dsk;
    s += ssk;
  }
  return 1;
}


//= Reverse order of columns within same image.

int jhcResize::FlipV (jhcImg& dest) const 
{
  int x, y, v;
  int line = dest.Line(), rh = dest.RoiH(), hh = rh / 2;
  UC8 *top, *bot;

  if (rh == 1)
    return 1;

  // start near middle of image
  bot = dest.RoiDest(0, hh - 1);
  top = dest.RoiDest(0, rh - hh);
  for (y = hh; y > 0; y--)
  {
    // swap horizontal lines
    for (x = 0; x < line; x++)
    {
      v = top[x];
      top[x] = bot[x];
      bot[x] = (UC8) v;
    }

    // advance line pointers
    top += line;
    bot -= line;
  }
  return 1;
}


//= Rotate image a full 180 degrees.
// dest must be different from source

int jhcResize::UpsideDown (jhcImg& dest, const jhcImg& src) const
{
  int w = src.XDim(), h = src.YDim(), nf = src.Fields();
  int x, y, f, ln = src.Line(), sk = src.Skip();
  const UC8 *s, *s0 = src.RoiSrc(w - 1, h - 1);
  UC8 *d = dest.PxlDest();

  if (!dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::UpsideDown");
  dest.FullRoi();

  for (y = h; y > 0; y--, d += sk, s0 -= ln)
  {
    s = s0;
    for (x = w; x > 0; x--, d += nf, s -= nf)
      for (f = 0; f < nf; f++)
        d[f] = s[f];  
  }
  return 1;
}


//= Rotate image a full 180 degrees in place.

int jhcResize::UpsideDown (jhcImg& dest) const
{
  int x, y, f, v, nf = dest.Fields(), sk = dest.Skip();
  int w = dest.XDim(), h = dest.YDim(), hw = w >> 1, hh = h >> 1;
  UC8 *top, *bot, *head, *tail;

  // BULK - start with horizontal lines flanking middle
  top = dest.RoiDest(0, h - hh);
  bot = dest.RoiDest(w - 1, hh - 1);
  for (y = hh; y > 0; y--, top += sk, bot -= sk)
    for (x = w; x > 0; x--, top += nf, bot -= nf)
      for (f = 0; f < nf; f++)
      {
        // swap pixel fields
        v = top[f]; 
        top[f] = bot[f];
        bot[f] = (UC8) v;
      } 

  // check for center horizontal line (odd height)
  if ((hh << 1) == h)
    return 1;

  // MIDDLE - start at opposite ends of center line
  head = dest.RoiDest(hw, hh);
  tail = dest.RoiDest(w - hw, hh);
  for (x = hw; x > 0; x--, head -= nf, tail += nf)
    for (f = 0; f < nf; f++)
    {
      // swap pixel fields
      v = tail[f]; 
      tail[f] = head[f];
      head[f] = (UC8) v;
    } 
  return 1;
}


//= Set destination size to match then rotate source 180 degrees.
// if inv <= then just copies source

int jhcResize::Invert (jhcImg& dest, const jhcImg& src, int inv) const
{
  dest.SetSize(src); 
  if (inv > 0) 
    return UpsideDown(dest, src); 
  dest.CopyArr(src);
  return 1;
}


//= Rotate image clockwise relative to screen display.

int jhcResize::RotateCW (jhcImg& dest, const jhcImg& src) const 
{
  int w = src.XDim(), h = src.YDim(), f = src.Fields();
  int rw = src.RoiH(), rh = src.RoiW();

  if (!dest.Valid(f) || dest.SameImg(src) || 
      (dest.XDim() != h) || (dest.YDim() != w))
    return Fatal("Bad images to jhcResize::RotateCW");
  dest.SetRoi(src.RoiY(), src.RoiX(), rw, rh);

  // general ROI case, write out in scan line order for dest
  int x, y, i, dsk = dest.RoiSkip();
  UL32 sln = src.Line();
  const UC8 *s, *s0 = src.RoiSrc() + f * (rh - 1);
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--)
  {
    s = s0;
    for (x = rw; x > 0; x--)
    {
      for (i = 0; i < f; i++)
        d[i] = s[i];
      d += f;
      s += sln;
    }
    d += dsk;
    s0 -= f;
  }
  return 1;
}


//= Rotate image counter-clockwise relative to screen display.

int jhcResize::RotateCCW (jhcImg& dest, const jhcImg& src) const 
{
  int w = src.XDim(), h = src.YDim(), f = src.Fields();
  int rw = src.RoiH(), rh = src.RoiW();

  if (!dest.Valid(f) || dest.SameImg(src) || 
      (dest.XDim() != h) || (dest.YDim() != w))
    return Fatal("Bad images to jhcResize::RotateCCW");
  dest.SetRoi(src.RoiY(), src.RoiX(), rw, rh);

  // general ROI case, write out in scan line order for dest
  int x, y, i, dsk = dest.RoiSkip();
  UL32 sln = src.Line();
  const UC8 *s, *s0 = src.RoiSrc() + sln * (rw - 1);
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--)
  {
    s = s0;
    for (x = rw; x > 0; x--)
    {
      for (i = 0; i < f; i++)
        d[i] = s[i];
      d += f;
      s -= sln;
    }
    d += dsk;
    s0 += f;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Subpixel Shifting                            //
///////////////////////////////////////////////////////////////////////////

//= Move image over a little in X and Y.
// leaves unmapped portions of dest at their original values

int jhcResize::Shift (jhcImg& dest, const jhcImg& src, int dx, int dy) const 
{
  if (!dest.SameFormat(src) || dest.SameImg(src))
    return Fatal("Bad images to jhcResize::Shift");

  // figure out new ROI for dest and corresponding clipped ROI for src
  jhcRoi dr;
  dr.CopyRoi(src);
  dr.MoveRoi(dx, dy);
  dest.CopyRoi(dr);
  dr.MoveRoi(-dx, -dy);

  // general ROI case
  int x, y;
  int rcnt = dest.RoiCnt(), rh = dest.RoiH(), sk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dr);

  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = *s++;
    d += sk;
    s += sk;
  }
  return 1; 
}


//= Shift an image in place inserting default value if no valid source for pixel.

int jhcResize::Shift (jhcImg& dest, int dx, int dy, int def) const
{
  int dw = abs(dx), dh = abs(dy), w = dest.XDim(), h = dest.YDim(), ln = dest.Line();
  int x, y, x0 = 0, y0 = 0, xinc = 1, yinc = ln, ndv = -dy * ln;
  UC8 *d, *d0;

  // check for special cases
  if (!dest.Valid(1))
    return Fatal("Bad image to jhcResize::Shift");
  if ((dw == 0) && (dh == 0))
    return 1;
  if ((dw >= w) || (dh >= h))
    return dest.FillArr(def);

  // figure out start and stepping directions
  if (dx > 0)
  {
    x0 = w - 1;              // right to left
    xinc = -1;
  }
  if (dy > 0)
  {
    y0 = h - 1;              // top to bottom
    yinc = -ln;
  }

  // scan image in some direction
  d0 = dest.RoiDest(x0, y0);      
  for (y = h - dh; y > 0; y--, d0 += yinc)
  {
    for (d = d0, x = w - dw; x > 0; x--, d += xinc)
      d[0] = d[ndv - dx];         
    for (x = dw; x > 0; x--, d += xinc)              
      d[0] = (UC8) def;                 
  }
  for (y = dh; y > 0; y--, d0 += yinc)           
    for (d = d0, x = w; x > 0; x--, d += xinc)
      d[0] = (UC8) def;                 
  return 1;
}


//= Move image over a little in X and Y and interpolates as needed.
// leaves unmapped portions of dest at their original values

int jhcResize::FracShift (jhcImg& dest, const jhcImg& src, double dx, double dy) const 
{
  if (!dest.SameFormat(src) || dest.SameImg(src) || !dest.Valid(1, 3))
    return Fatal("Bad images to jhcResize::FracShift");
  if (dest.Valid(3))
    return FracShift_RGB(dest, src, dx, dy);
  return FracShift_BW(dest, src, dx, dy);
}


//= Monochrome version of fractional shifting.

int jhcResize::FracShift_BW (jhcImg& dest, const jhcImg& src, double dx, double dy) const 
{
  jhcRoi dr;
  int xlo, ylo;

  // figure out reduced ROI (automatically clips to image)
  xlo = (int) floor(dx);
  ylo = (int) floor(dy);
  dr.SetRoi(src.RoiX(), src.RoiY(), src.RoiW() - 1, src.RoiH() - 1);
  dr.MoveRoi(xlo + 1, ylo + 1);
  dest.CopyRoi(dr);
  dr.CopyRoi(dest);
  dr.MoveRoi(-xlo - 1, -ylo - 1);

  // general ROI case
  int i, x, y, fx, fy, f00, f10, f01, f11, v00 = 0, v01 = 0, v10 = 0, v11 = 0;
  int rw = dest.RoiW(), rh = dest.RoiH(), sk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dr), *u = s + src.Line();
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

  // do interpolation, works for RGB also
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d++, s++, u++)
      d[0] = (UC8)((xf00[s[0]] + xf10[s[1]] + xf01[u[0]] + xf11[u[1]]) >> 16);
    d += sk;
    s += sk;
    u += sk;
  }
  return 1; 
}


//= Color version of fractional shifting.

int jhcResize::FracShift_RGB (jhcImg& dest, const jhcImg& src, double dx, double dy) const 
{
  jhcRoi dr;
  int xlo, ylo;

  // figure out reduced ROI (automatically clips to image)
  xlo = (int) floor(dx);
  ylo = (int) floor(dy);
  dr.SetRoi(src.RoiX(), src.RoiY(), src.RoiW() - 1, src.RoiH() - 1);
  dr.MoveRoi(xlo + 1, ylo + 1);
  dest.CopyRoi(dr);
  dr.CopyRoi(dest);
  dr.MoveRoi(-xlo - 1, -ylo - 1);

  // general ROI case
  int i, x, y, fx, fy, f00, f10, f01, f11, v00 = 0, v01 = 0, v10 = 0, v11 = 0;
  int rw = dest.RoiW(), rh = dest.RoiH(), sk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc(dr), *u = s + src.Line();
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

  // do interpolation, works for RGB also
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d += 3, s += 3, u += 3)
    {
      d[0] = (UC8)((xf00[s[0]] + xf10[s[3]] + xf01[u[0]] + xf11[u[3]]) >> 16);
      d[1] = (UC8)((xf00[s[1]] + xf10[s[4]] + xf01[u[1]] + xf11[u[4]]) >> 16);
      d[2] = (UC8)((xf00[s[2]] + xf10[s[5]] + xf01[u[2]] + xf11[u[5]]) >> 16);
    }
    d += sk;
    s += sk;
    u += sk;
  }
  return 1; 
}


//= Move image over a little in X and Y and interpolates as needed.
// leaves unmapped portions of dest at their original values
// can make a small image from a big one, but only with integer reduction

int jhcResize::FracSamp (jhcImg& dest, const jhcImg& src, double dx, double dy) const 
{
  if ((dest.Fields() != src.Fields()) || !dest.Valid(1, 3) ||
      (dest.XDim() > src.XDim()) || (dest.YDim() > src.YDim()))
    return Fatal("Bad images to jhcResize::FracSamp");
  if ((dx == 0.0) && (dy == 0.0) && dest.SameFormat(src))
    return dest.CopyArr(src);
  if ((dx == 0.0) && (dy == 0.0))
    return Sample(dest, src);
  if (dest.Valid(3))
    return FracSamp_RGB(dest, src, dx, dy);
  return FracSamp_BW(dest, src, dx, dy);
}


//= Monochrome version of fractional sampling.

int jhcResize::FracSamp_BW (jhcImg& dest, const jhcImg& src, double dx, double dy) const 
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

  // do interpolation, works for RGB also
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d++, s += step, u += step)
      d[0] = (UC8)((xf00[s[0]] + xf10[s[1]] + xf01[u[0]] + xf11[u[1]]) >> 16);
    d += dsk;
    s += ssk;
    u += ssk;
  }
  return 1; 
}


//= Color version of fractional sampling.

int jhcResize::FracSamp_RGB (jhcImg& dest, const jhcImg& src, double dx, double dy) const 
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

  // do interpolation, works for RGB also
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d += 3, s += inc, u += inc)
    {
      d[0] = (UC8)((xf00[s[0]] + xf10[s[3]] + xf01[u[0]] + xf11[u[3]]) >> 16);
      d[1] = (UC8)((xf00[s[1]] + xf10[s[4]] + xf01[u[1]] + xf11[u[4]]) >> 16);
      d[2] = (UC8)((xf00[s[2]] + xf10[s[5]] + xf01[u[2]] + xf11[u[5]]) >> 16);
    }
    d += dsk;
    s += ssk;
    u += ssk;
  }
  return 1; 
}


//= Move each line over a little in X and Y and interpolate as needed.
// leaves unmapped portions of dest at their original values

int jhcResize::LineShift (jhcImg& dest, const jhcImg& src, const double *vdx, double dy) const 
{
  if (!dest.SameFormat(src) || dest.SameImg(src) || !dest.Valid(1, 3) || (vdx == NULL))
    return Fatal("Bad images to jhcResize::LineShift");
  if (dest.Valid(3))
    return LineShift_RGB(dest, src, vdx, dy);
  return LineShift_BW(dest, src, vdx, dy);
}


//= Monochrome version of fractional line shifting.

int jhcResize::LineShift_BW (jhcImg& dest, const jhcImg& src, const double *vdx, double fdy) const 
{
  int fx, fy, f00, f10, f01, f11;
  int x, y, xhi, yhi, rx, ry, rw, rh, xmax, xmin;
  int xcnt, ycnt, xlim = dest.XLim(), ylim = dest.YLim(), ln = dest.Line();
  UC8 *d, *d0 = dest.PxlDest();
  const UC8 *u, *s, *s0 = src.PxlSrc();
  const double *f = vdx;

  // find vertical integral shift amount
  ycnt = ylim;
  yhi = (int) ceil(fdy);
  fy = ROUND(256.0 * (yhi - fdy));
  if (yhi > 0)
  {
    d0 += (yhi * ln);
    ycnt -= (yhi - 1);
  }
  else if (yhi < 0)
  {
    s0 += (-yhi * ln);
    f += -yhi;
    ycnt -= -yhi;
  }

  // do interpolation
  xmax = (int) ceil(*f);
  xmin = xmax;
  for (y = ycnt; y > 0; y--, f++)
  {
    // start at beginning of next line
    d = d0;
    s = s0;
    u = s0 + ln;

    // find horizontal integral shift amount
    xcnt = xlim;
    xhi = (int) ceil(*f);
    fx = ROUND(256.0 * (xhi - (*f)));
    if (xhi > 0)
    {
      d += xhi;
      xcnt -= (xhi - 1);
    }
    else if (xhi < 0)
    {
      s += -xhi;
      u += -xhi;
      xcnt -= -xhi;
    }

    // remember biggest and smallest value
    if (xhi > xmax)
      xmax = xhi;
    if (xhi < xmin)
      xmin = xhi;

    // figure out mixing coefficients
    f11 = fx * fy;
    f10 = fx * (256 - fy);
    f01 = (256 - fx) * fy;
    f00 = (256 - fx) * (256 - fy);

    // mix pixels in line
    for (x = xcnt; x > 0; x--, d++, s++, u++)
      *d = (UC8)((f00 * s[0] + f10 * s[1] + f01 * u[0] + f11 * u[1]) >> 16);

    // advance to next line
    d0 += ln;
    s0 += ln;
  }

  // figure out reduced ROI (automatically clips to image)
  src.RoiSpecs(&rx, &ry, &rw, &rh);
  dest.SetRoi(rx + xmax, ry + yhi, rw - (xmax - xmin), rh);
  return 1; 
}


//= Color version of fractional line shifting.

int jhcResize::LineShift_RGB (jhcImg& dest, const jhcImg& src, const double *vdx, double fdy) const 
{
  int fx, fy, f00, f10, f01, f11;
  int x, y, xhi, yhi, rx, ry, rw, rh, xmax, xmin;
  int xcnt, ycnt, xlim = dest.XLim(), ylim = dest.YLim(), ln = dest.Line();
  UC8 *d, *d0 = dest.PxlDest();
  const UC8 *u, *s, *s0 = src.PxlSrc();
  const double *f = vdx;

  // find vertical integral shift amount
  ycnt = ylim;
  yhi = (int) ceil(fdy);
  fy = ROUND(256.0 * (yhi - fdy));
  if (yhi > 0)
  {
    d0 += (yhi * ln);
    ycnt -= (yhi - 1);
  }
  else if (yhi < 0)
  {
    s0 += (-yhi * ln);
    f  += -yhi;
    ycnt -= -yhi;
  }

  // do interpolation
  xmax = (int) ceil(*f);
  xmin = xmax;
  for (y = ycnt; y > 0; y--, f++)
  {
    // start at beginning of next line
    d = d0;
    s = s0;
    u = s0 + ln;

    // find horizontal integral shift amount
    xcnt = xlim;
    xhi = (int) ceil(*f);
    fx = ROUND(256.0 * (xhi - (*f)));
    if (xhi > 0)
    {
      d += (3 * xhi);
      xcnt -= (xhi - 1);
    }
    else if (xhi < 0)
    {
      s += (3 * -xhi);
      u += (3 * -xhi);
      xcnt -= -xhi;
    }

    // remember biggest and smallest value
    if (xhi > xmax)
      xmax = xhi;
    if (xhi < xmin)
      xmin = xhi;

    // figure out mixing coefficients
    f11 = fx * fy;
    f10 = fx * (256 - fy);
    f01 = (256 - fx) * fy;
    f00 = (256 - fx) * (256 - fy);

    // mix pixels in line
    for (x = xcnt; x > 0; x--, d += 3, s += 3, u += 3)
    {
      d[0] = (UC8)((f00 * s[0] + f10 * s[3] + f01 * u[0] + f11 * u[3]) >> 16);
      d[1] = (UC8)((f00 * s[1] + f10 * s[4] + f01 * u[1] + f11 * u[4]) >> 16);
      d[2] = (UC8)((f00 * s[2] + f10 * s[5] + f01 * u[2] + f11 * u[5]) >> 16);
    }

    // advance to next line
    d0 += ln;
    s0 += ln;
  }

  // figure out reduced ROI (automatically clips to image)
  src.RoiSpecs(&rx, &ry, &rw, &rh);
  dest.SetRoi(rx + xmax, ry + yhi, rw - (xmax - xmin), rh);
  return 1; 
}


//= Move each line over a little in X and Y and interpolate as needed.
// leaves unmapped portions of dest at their original values
// can make a small image from a big one, but only with integer reduction

int jhcResize::LineSamp (jhcImg& dest, const jhcImg& src, const double *vdx, double dy) const 
{
  if ((dest.Fields() != src.Fields()) || !dest.Valid(1, 3) || (vdx == NULL) ||
      (dest.XDim() > src.XDim()) || (dest.YDim() > src.YDim()))
    return Fatal("Bad images to jhcResize::LineSamp");
  if (dest.Valid(3))
    return LineSamp_RGB(dest, src, vdx, dy);
  return LineSamp_BW(dest, src, vdx, dy);
}


//= Monochrome version of fractional line shifting.
// general transformation:
//   xs = step * (xd + 0.5) - 0.5 - dx
//   xd = (xs + 0.5 + dx) / step - 0.5 

int jhcResize::LineSamp_BW (jhcImg& dest, const jhcImg& src, const double *vdx, double fdy) const 
{
  int fx, fy, f00, f10, f01, f11;
  int x, y, xhi, yhi, rx, ry, rw, rh, rx2, ry2, xmax, xmin;
  int xcnt, ycnt, xlim = dest.XLim(), ylim = dest.YLim(), ln = dest.Line();
  int step = src.YDim() / dest.YDim(), sln = src.Line(), ssl = step * sln;
  double dx, dy, off = 0.5 * (step - 1);
  UC8 *d, *d0 = dest.PxlDest();
  const UC8 *s, *u, *s0 = src.PxlSrc();
  const double *f = vdx;

  // find vertical integral shift amount for first sample position
  ycnt = ylim;
  dy = fdy - off;
  yhi = (int) ceil(dy);
  fy = ROUND(256.0 * (yhi - dy));
  if (yhi > 0)
  {
    d0 += (((yhi / step) + 1) * ln);
    ycnt -= ((yhi / step) + 1);
  }
  else if (yhi < 0)
  {
    s0 += (-yhi * sln);
    f  += -yhi;
    ycnt -= (-yhi / step);
  }

  // do interpolation
  dx = *f - off;
  xmax = (int) ceil(dx);
  xmin = xmax;
  for (y = ycnt; y > 0; y--, f += step)
  {
    // start at beginning of next line
    d = d0;
    s = s0;
    u = s0 + sln;

    // find horizontal integral shift amount for first sample position
    xcnt = xlim;
    dx = *f - off;
    xhi = (int) ceil(dx);
    fx = ROUND(256.0 * (xhi - dx));
    if (xhi > 0)
    {
      d += ((xhi / step) + 1);
      xcnt -= ((xhi / step) + 1);
    }
    else
    {
      s += -xhi;
      u += -xhi;
      xcnt -= (-xhi / step);
    }

    // remember biggest and smallest value
    if (xhi > xmax)
      xmax = xhi;
    if (xhi < xmin)
      xmin = xhi;

    // figure out mixing coefficients
    f11 = fx * fy;
    f10 = fx * (256 - fy);
    f01 = (256 - fx) * fy;
    f00 = (256 - fx) * (256 - fy);

    // mix pixels in line
    for (x = xcnt; x > 0; x--, d++, s += step, u += step)
      *d = (UC8)((f00 * s[0] + f10 * s[1] + f01 * u[0] + f11 * u[1]) >> 16);

    // advance to next line
    d0 += ln;
    s0 += ssl;
  }

  // figure out reduced ROI (automatically clips to image)
  src.RoiSpecs(&rx, &ry, &rw, &rh);
  rx += xmax;                            // clipped version in big image
  ry += yhi;
  rw -= (xmax - xmin);
  rx2 = (rx + rw) / step;                // round down
  ry2 = (ry + rh) / step;
  rx = (rx + step - 1) / step;           // round up 
  ry = (ry + step - 1) / step;
  dest.SetRoi(rx, ry, rx2 - rx, ry2 - ry);
  return 1; 
}


//= Color version of fractional line shifting.

int jhcResize::LineSamp_RGB (jhcImg& dest, const jhcImg& src, const double *vdx, double fdy) const 
{
  int fx, fy, f00, f10, f01, f11;
  int x, y, xhi, yhi, rx, ry, rx2, ry2, rw, rh, xmax, xmin;
  int step = src.YDim() / dest.YDim(), sinc = 3 * step, sln = src.Line(), ssl = step * sln;
  int xcnt, ycnt, xlim = dest.XLim(), ylim = dest.YLim(), ln = dest.Line();
  double dx, dy, off = 0.5 * (step - 1);
  UC8 *d, *d0 = dest.PxlDest();
  const UC8 *s, *u, *s0 = src.PxlSrc();
  const double *f = vdx;

  // find vertical integral shift amount for first sample position
  ycnt = ylim;
  dy = fdy - off;
  yhi = (int) ceil(dy);
  fy = ROUND(256.0 * (yhi - dy));
  if (yhi > 0)
  {
    d0 += (((yhi / step) + 1) * ln);
    ycnt -= ((yhi / step) + 1);
  }
  else if (yhi < 0)
  {
    s0 += (-yhi * sln);
    f  += -yhi;
    ycnt -= (-yhi / step);
  }

  // do interpolation
  dx = *f - off;
  xmax = (int) ceil(dx);
  xmin = xmax;
  for (y = ycnt; y > 0; y--, f += step)
  {
    // start at beginning of next line
    d = d0;
    s = s0;
    u = s0 + sln;

    // find horizontal integral shift amount for first sample position
    xcnt = xlim;
    dx = *f - off;
    xhi = (int) ceil(dx);
    fx = ROUND(256.0 * (xhi - dx));
    if (xhi > 0)
    {
      d += (((xhi / step) + 1) * 3);
      xcnt -= ((xhi / step) + 1);
    }
    else
    {
      s += (-xhi * 3);
      u += (-xhi * 3);
      xcnt -= (-xhi / step);
    }

    // remember biggest and smallest value
    if (xhi > xmax)
      xmax = xhi;
    if (xhi < xmin)
      xmin = xhi;

    // figure out mixing coefficients
    f11 = fx * fy;
    f10 = fx * (256 - fy);
    f01 = (256 - fx) * fy;
    f00 = (256 - fx) * (256 - fy);

    // mix pixels in line
    for (x = xcnt; x > 0; x--, d += 3, s += sinc, u += sinc)
    {
      d[0] = (UC8)((f00 * s[0] + f10 * s[3] + f01 * u[0] + f11 * u[3]) >> 16);
      d[1] = (UC8)((f00 * s[1] + f10 * s[4] + f01 * u[1] + f11 * u[4]) >> 16);
      d[2] = (UC8)((f00 * s[2] + f10 * s[5] + f01 * u[2] + f11 * u[5]) >> 16);
    }

    // advance to next line
    d0 += ln;
    s0 += ssl;
  }

  // figure out reduced ROI (automatically clips to image)
  src.RoiSpecs(&rx, &ry, &rw, &rh);
  rx += xmax;                            // clipped version in big image
  ry += yhi;
  rw -= (xmax - xmin);
  rx2 = (rx + rw) / step;                // round down
  ry2 = (ry + rh) / step;
  rx = (rx + step - 1) / step;           // round up 
  ry = (ry + step - 1) / step;
  dest.SetRoi(rx, ry, rx2 - rx, ry2 - ry);
  return 1; 
}


///////////////////////////////////////////////////////////////////////////
//                         Four Panel Images                             //
///////////////////////////////////////////////////////////////////////////

//= Tell horizontal size of one panel.
// cv = total pixels to skip in middle, ev = pixels to skip at each border

int jhcResize::QuadX (const jhcImg& ref, int cv, int ev) const
{
  return((ref.XDim() - cv - 2 * ev) / 2);
}


//= Tell vertical size of one panel.
// ch = total pixels to skip in middle, eh = pixels to skip at each border

int jhcResize::QuadY (const jhcImg& ref, int ch, int eh) const
{
  return((ref.YDim() - ch - 2 * eh) / 2);
}


//= Set dimensions of an image to accept one panel of the target.

jhcImg *jhcResize::QuadSize (jhcImg& target, const jhcImg& src,  
                             int cv, int ch, int ev, int eh) const
{
  return target.SetSize(QuadX(src, cv, ev), QuadY(src, ch, eh), src.Fields());
}


//= Pull out one panel of 4 part image.
// quadrant 5 = zoom middle half
// panels labelled in DISPLAY order:   
// <pre>
//    1 2
//    3 4
// </pre>

int jhcResize::GetQuad (jhcImg& dest, const jhcImg& src, int n, 
                        int cv, int ch, int ev, int eh) const 
{
  int qw = QuadX(src, ch, eh), qh = QuadY(src, cv, ev);

  if (!dest.Valid() || (dest.Fields() != src.Fields()) ||
      (dest.XDim() != qw) || (dest.YDim() != qh))
    return Fatal("Bad images to jhcResize::GetQuad");
  if (n <= 0)
    return 0;

  switch (n)
  {
    case 1:
      return Extract(dest, src, ev,           eh + qh + ch);
    case 2:
      return Extract(dest, src, ev + qw + cv, eh + qh + ch);
    case 3:
      return Extract(dest, src, ev,           eh);
    case 4:
      return Extract(dest, src, ev + qw + cv, eh);
    default:
      return Extract(dest, src, ev + (qw / 2) + cv, eh + (qh / 2) + ch);
  }
}


//= Pull out one panel of 2 part image.
// panels labelled in DISPLAY order: 1 2

int jhcResize::GetHalf (jhcImg& dest, const jhcImg& src, int n, 
                        int cv, int ev, int eh) const 
{
  int hw = (src.XDim() - cv - 2 * ev) / 2;
  int hh = src.YDim() - 2 * eh;

  if (!dest.Valid() || (dest.Fields() != src.Fields()) ||
      (dest.XDim() != hw) || (dest.YDim() != hh))
    return Fatal("Bad images to jhcResize::GetHalf");
  if (n <= 0)
    return 0;
  if (n == 1)
    return Extract(dest, src, ev, eh);
  return Extract(dest, src, ev + hw + cv, eh);
}


///////////////////////////////////////////////////////////////////////////
//           Images Combined as Odd and Even NTSC Fields                 //
///////////////////////////////////////////////////////////////////////////

//= Put two images together to make "fake" interlaced source.
// even field is exposed before odd field

int jhcResize::MixOddEven (jhcImg& dest, const jhcImg& odd, const jhcImg& even) const 
{
  jhcRoi tmp;
  int specs[4];
  int w = dest.XDim(), h2 = dest.YDim() / 2, f = dest.Fields();

  // check arguments
  if (!dest.Valid() || !odd.SameFormat(w, h2, f) || !odd.SameFormat(even))
    return Fatal("Bad images to jhcResize::MixOddEven");

  // adjust output ROI
  tmp.CopyRoi(even);
  tmp.MergeRoi(odd);
  tmp.RoiSpecs(specs);
  specs[1] *= 2;
  specs[3] *= 2;
  dest.SetRoi(specs);

  // general ROI case
  int x, y, i, rw = dest.RoiW(), rh2 = dest.RoiH() / 2;
  int rx = dest.RoiX(), ry2 = dest.RoiY() / 2;
  int dsk = dest.RoiSkip(), ssk = odd.RoiSkip(rw);
  UC8 *d = dest.RoiDest();
  const UC8 *o = odd.RoiSrc(rx, ry2), *e = even.RoiSrc(rx, ry2);

  for (y = rh2; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      for (i = f; i > 0; i--)
        *d++ = *o++;
    d += dsk;
    o += ssk;
    for (x = rw; x > 0; x--)
      for (i = f; i > 0; i--)
        *d++ = *e++;
    d += dsk;
    e += ssk;
  }
  return 1;
}


//= Put two images together to make "fake" interlaced source of same size.
// even field is exposed before odd field

int jhcResize::MixOddEven2 (jhcImg& dest, const jhcImg& odd, const jhcImg& even) const 
{
  int x, y = 0, ln = dest.Line(), h = dest.YDim();
  const UC8 *a = odd.PxlSrc(), *b = even.PxlSrc() + ln;
  UC8 *d = dest.PxlDest();

  if (!dest.Valid() || !dest.SameFormat(odd) || !dest.SameFormat(even))
    return Fatal("Bad images to jhcResize::MixOddEven2");

  while (1)
  {
    // copy one line from odd image
    for (x = ln; x > 0; x--, d++, a++)
      *d = *a;
    if (++y >= h)
      break;

    // copy one line from even image
    for (x = ln; x > 0; x--, d++, b++)
      *d = *b;
    if (++y >= h)
      break;

    // move on to next pair of lines
    a += ln;
    b += ln;
  }
  return 1;
}


//= Split an odd/even line mixed image into two separate images.
// even field is exposed before odd field

int jhcResize::GetOddEven (jhcImg& odd, jhcImg& even, const jhcImg& src) const 
{
  int specs[4];
  int w = src.XDim(), h2 = src.YDim() / 2, f = src.Fields();

  // check arguments
  if (!src.Valid() || !odd.SameFormat(w, h2, f) || !odd.SameFormat(even))
    return Fatal("Bad images to jhcResize::GetOddEven");

  // adjust output ROIs
  src.RoiSpecs(specs);
  specs[1] /= 2;
  specs[3] /= 2;
  odd.SetRoi(specs);
  even.CopyRoi(odd);

  // general ROI case
  int x, y, i, rw = odd.RoiW(), rh2 = odd.RoiH(); 
  int ssk = src.RoiSkip(), dsk = odd.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *o = odd.RoiDest(), *e = even.RoiDest();

  for (y = rh2; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      for (i = f; i > 0; i--)
        *o++ = *s++;
    o += dsk;
    s += ssk;
    for (x = rw; x > 0; x--)
      for (i = f; i > 0; i--)
        *e++ = *s++;
    e += dsk;
    s += ssk;
  }
  return 1;
}


//= Like GetOddEven but subsample in the horizontal direction for same aspect.
// even field is exposed before odd field

int jhcResize::GetHalfOE (jhcImg& odd, jhcImg& even, const jhcImg& src) const 
{
  int specs[4];
  int w2 = src.XDim() / 2, h2 = src.YDim() / 2, f = src.Fields();

  // check arguments
  if (!src.Valid() || !odd.SameFormat(w2, h2, f) || !odd.SameFormat(even))
    return Fatal("Bad images to jhcResize::GetHalfOE");

  // adjust output ROIs
  src.RoiSpecs(specs);
  specs[0] /= 2;
  specs[1] /= 2;
  specs[2] /= 2;
  specs[3] /= 2;
  odd.SetRoi(specs);
  even.CopyRoi(odd);

  // general ROI case
  int x, y, i, rw2 = odd.RoiW(), rh2 = odd.RoiH(); 
  int ssk = src.RoiSkip(), dsk = odd.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *o = odd.RoiDest(), *e = even.RoiDest();

  for (y = rh2; y > 0; y--)
  {
    for (x = rw2; x > 0; x--)
    {
      for (i = f; i > 0; i--)
        *o++ = *s++;
      s += f;
    }
    o += dsk;
    s += ssk;
    for (x = rw2; x > 0; x--)
    {
      for (i = f; i > 0; i--)
        *e++ = *s++;
      s += f;
    }
    e += dsk;
    s += ssk;
  }
  return 1;
}


//= Like GetHalfOE but average horizontally adjacent pixels.
// even field is exposed before odd field

int jhcResize::GetAvgOE (jhcImg& odd, jhcImg& even, const jhcImg& src) const 
{
  int specs[4];
  int w2 = src.XDim() / 2, h2 = src.YDim() / 2;
  int f = src.Fields(), f2 = 2 * f;

  // check arguments
  if (!src.Valid() || !odd.SameFormat(w2, h2, f) || !odd.SameFormat(even))
    return Fatal("Bad images to jhcResize::GetAvgOE");

  // adjust output ROIs
  src.RoiSpecs(specs);
  specs[0] /= 2;
  specs[1] /= 2;
  specs[2] /= 2;
  specs[3] /= 2;
  odd.SetRoi(specs);
  even.CopyRoi(odd);

  // general ROI case
  int x, y, i, rw2 = odd.RoiW(), rh2 = odd.RoiH(); 
  int dsk = odd.RoiSkip(), ssk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();
  UC8 *o = odd.RoiDest(), *e = even.RoiDest();

  for (y = rh2; y > 0; y--)
  {
    for (x = rw2; x > 0; x--)
    {
      for (i = 0; i < f; i++)
        o[i] = (s[i] + s[i + f]) / 2;
      s += f2;
      o += f;
    }
    o += dsk;
    s += ssk;
    for (x = rw2; x > 0; x--)
    {
      for (i = 0; i < f; i++)
        e[i] = (s[i] + s[i + f]) / 2;
      s += f2;
      e += f;
    }
    e += dsk;
    s += ssk;
  }
  return 1;
}

