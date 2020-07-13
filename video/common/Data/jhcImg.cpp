// jhcImg.cpp - basic image functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2020 IBM Corporation
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
#include <malloc.h>                      // needed for MMX aligned malloc

#include "Data/jhcBitMacros.h"
#include "Interface/jhcMessage.h"

#include "Data/jhcImg.h"


//////////////////////////////////////////////////////////////////////////////
//                     Basic creation and deletion                          //
//////////////////////////////////////////////////////////////////////////////

//= Destructor frees pixel array.

jhcImg::~jhcImg ()
{
  dealloc_img();
}


//= Basic constructor does not create pixel array.

jhcImg::jhcImg ()
{
  init_img(0);
  null_img();
  SetSize(0, 0, 0, 1.0);
}


//= Create new array of specific width, height, and number of fields.

jhcImg::jhcImg (int width, int height, int fields)
{
  init_img(0);
  null_img();
  SetSize(width, height, fields, 1.0);
}


//= Same as otehr constructor but takes single argument of width, height, fields.

jhcImg::jhcImg (const int specs[])
{
  init_img(0);
  null_img();
  SetSize(specs);
}


//= Base new instance on old instance but change the number of fields.
// if number of fields is zero, copies old number of fields

jhcImg::jhcImg (const jhcImg& ref, int fields) : jhcRoi()  // base constructor for g++
{
  init_img(0);
  null_img();
  SetSize(ref, fields);
}


//= Make image like old one but with dimensions scaled by some factor.

jhcImg::jhcImg (const jhcImg& ref, double f, int fields)
{
  int n = fields;

  if (n == 0)
    n = ref.nf;
  init_img(0);
  null_img();
  SetSize(ROUND(f * ref.w), ROUND(f * ref.h), n, ref.aspect);
}


//= Base new instance on old instance but make about hdes high.

jhcImg::jhcImg (const jhcImg& ref, int hdes, int fields)
{
  init_img(0);
  null_img();
  SetSize(ref, hdes, fields);
}


//= Initializes structure and conses up bit array.
// negative ht means scan top down (instead of bottom up)
// always initializes ROI to be whole image
// returns pointer to this image

jhcImg *jhcImg::SetSize (int wd, int ht, int fields, double a)
{
  // handle special cases
  if (a > 0.0)
    aspect = a;
  if ((Buffer != NULL) && (wd == w) && (ht == h) && (fields == nf))
    return this;
  if (wrap > 0)
  {
    Fatal("jhcImg::SetSize - Cannot change size of a wrapped array!");
    return this;
  }

  // record sizes and various other useful parameters
  record_sizes(wd, ht, fields);
#ifdef _DEBUG
  if ((bsize < 0) || (bsize > 32000000))   // sanity check
    Pause("jhcImg::SetSize - Trying to allocate a (%d %d) x %d image", w, h, nf);
#endif
  if (bsize == 0)
  {
    dealloc_img();
    return this;
  }

  // see if old pixel buffer can be re-used (vsz added in 2017)
  if ((vsz <= 0) || (bsize > asize))
  {
    // make up array to hold pixel values 
    dealloc_img();
    if (bsize > 0)
#ifdef JHC_MMX
      Buffer = (UC8 *) _mm_malloc(bsize, 16);  // needed for SSE2
#else
      Buffer = new UC8 [bsize];
#endif

    // check that allocation succeeded (record full buffer size in asize)
    if ((bsize > 0) && (Buffer == NULL))
    {
      record_sizes(0, 0, 0);
      Fatal("jhcImg::SetSize - Pixel buffer (%d %d) x %d allocation failed!", wd, ht, fields);
    }
    asize = bsize;
  }

  // clear all pixels when first allocated (added in 2013)
  memset(Buffer, 0, bsize);
  return this;
}


//= Sets image size based on parameters contained in an array (w, h, d).
// array must have at least 3 elements (but nothing ever checks this)

jhcImg *jhcImg::SetSize (const int specs[])
{
  return SetSize(specs[0], specs[1], specs[2], 1.0);
}


//= Like other SetSize but takes dimensions from another image.

jhcImg *jhcImg::SetSize (const jhcImg& ref, int fields)
{
  int n = fields;

  if (n == 0)
    n = ref.nf;
  return SetSize(ref.w, ref.h, n, ref.aspect);
}


//= A resizing function which scales both dimensions.
// if number of fields is zero, copies old number of fields

jhcImg *jhcImg::SetSize (const jhcImg& ref, double f, int fields)
{
  int n = fields;

  if (n == 0)
    n = ref.nf;
  SetSize(ROUND(f * ref.w), ROUND(f * ref.h), n, ref.aspect);
  return this;
}


//= Base new instance on old instance but change the number of fields.
// set up for resampling to give about hdes pixels high
// resulting image is always between 3/4 hdes and 4/3 hdes high

jhcImg *jhcImg::SetSize (const jhcImg& ref, int hdes, int fields)
{
  int f, rw = ref.w, rh = ref.h;

  if (rh > 0)
  {
    if (rh < hdes)
    {
      f = ROUND((double) hdes / rh);
      rw *= f;
      rh *= f;
    }
    else if (hdes > 0)
    {
      f = ROUND((double) rh / hdes);
      rw /= f;
      rh /= f;
    }
  }
  return SetSize(rw, rh, fields, ref.aspect);
}


//= Create image with default dimensions but fitting height constraint.
// set up for resampling to give about hdes pixels high
// resulting image is always between 3/4 hdes and 4/3 hdes high

jhcImg *jhcImg::AdjSize (int wd, int ht, int fields, int hdes, double a)
{
  int f, rw = wd, rh = ht;

  if (rh > 0)
  {
    if (rh < hdes)
    {
      f = ROUND((double) hdes / rh);
      rw *= f;
      rh *= f;
    }
    else if (hdes > 0)
    {
      f = ROUND((double) rh / hdes);
      rw /= f;
      rh /= f;
    }
  }
  return SetSize(rw, rh, fields, a);
}


//= Set size so that max dimension is EXACTLY nsz and shape matches reference.
// can EITHER expand or shrink versus reference

jhcImg *jhcImg::MaxSize (const jhcImg& ref, int nsz, int fields)
{
  int n = ((fields > 0) ? fields : ref.nf);

  if (ref.w > ref.h)
    return SetSize(nsz, ROUND(ref.h * nsz / (double) ref.w), n);
  return SetSize(ROUND(ref.w * nsz / (double) ref.h), nsz, n);
}


//= Set so min dimension is AT LEAST nsz and shape matches reference.
// can expand but NEVER shrinks versus reference

jhcImg *jhcImg::MinSize (const jhcImg& ref, int nsz, int fields)
{
  int n = ((fields > 0) ? fields : ref.nf);

  if ((ref.w >= nsz) && (ref.h >= nsz))
    return SetSize(ref.w, ref.h, n);
  return MaxSize(ref, nsz, n);
}


//= Like equivalent SetSize but forces square pixels (keeps height).
// takes height (and possibly depth) from another image.

jhcImg *jhcImg::SetSquare (const jhcImg& ref, int fields)
{
  int n = fields;

  if (n == 0)
    n = ref.nf;
  return SetSize(ROUND(ref.h * (4.0 / 3.0)), ref.h, n, 1.0);
}


//= Like equivalent SetSize but forces square pixels (keeps height).
// ignores given width

jhcImg *jhcImg::SetSquare (int wid, int ht, int fields)
{
  return SetSize(ROUND(ht * (4.0 / 3.0)), ht, fields, 1.0);
}


//= Like equivalent SetSize but forces square pixels (keeps height).
// base new instance on old instance but change the number of fields
// set up for resampling to give about hdes pixels high
// resulting image is always between 3/4 hdes and 4/3 hdes high

jhcImg *jhcImg::SetSquare (const jhcImg& ref, int hdes, int fields)
{
  int rh = ref.h;

  if (rh > 0)
  {
    if (rh < hdes)
      rh *= ROUND((double) hdes / rh);
    else if (hdes > 0)
      rh /= ROUND((double) rh / hdes);
  }
  return SetSize(ROUND(rh * (4.0 / 3.0)), rh, fields, 1.0);
}


//= Like equivalent SetSize but forces square pixels (keeps height).
// ignores given width

jhcImg *jhcImg::AdjSquare (int wid, int ht, int fields, int hdes)
{
  int rh = ht;

  if (rh > 0)
  {
    if (rh < hdes)
      rh *= ROUND((double) hdes / rh);
    else if (hdes > 0)
      rh /= ROUND((double) rh / hdes);
  }
  return SetSize(ROUND(rh * (4.0 / 3.0)), rh, fields, 1.0);
}


//= Have image "ingest" an external BMP buffer.
// when destructor called, this array is NOT deallocated
// more efficient than CopyArr for interface to other code

jhcImg *jhcImg::Wrap (UC8 *raw, int wid, int ht, int fields)
{
  int dw = wid, dh = ht, df = fields;

  // default size to current settings
  if (dw <= 0)
    dw = w;
  if (dh <= 0)
    dh = h;
  if (df <= 0)
    df = nf;

  // record (possibly new size) and replace internal buffer (always)
  record_sizes(dw, dh, df);
  dealloc_img();
  Buffer = raw;
  wrap = 1;
  return this;
}


//= Set up internal record of sizes, skips, etc.
// defines how to interpret buffer, not ACTUAL byte size (asize)

void jhcImg::record_sizes (int wd, int ht, int fields)
{
  w = abs(wd);
  h = abs(ht);
  nf = fields;

  MaxRoi();
  line_len = ((w * nf + 3) >> 2) << 2;
  end_skip = line_len - w * nf;
  bsize = line_len * h;
}


//= Clean up allocated memory.
// leave dimensions, size, and resizability untouched

void jhcImg::dealloc_img ()
{
#ifdef JHC_MMX
  // special alignment needed for SSE2 operations
  if ((wrap <= 0) && (Buffer != NULL))
    _mm_free(Buffer);
  if ((sep > 0) && (Stacked != NULL))
    _mm_free(Stacked);
#else
  // basic version, does not need processor pack
  if ((wrap <= 0) && (Buffer != NULL))
    delete [] Buffer;
  if ((sep > 0) && (Stacked != NULL))
    delete [] Stacked;
#endif
  init_img(vsz);
}


//= Set default values, but not any sizing parameters.

void jhcImg::init_img (int v0)
{
  status  = 1;
  wrap    = 0;
  sep     = 0;
  norm    = 1;

  Stacked = NULL;
  ssize   = 0;
  psize   = 0;
  sskip   = 0;
  sline   = 0;

  Buffer  = NULL;
  asize   = 0;
  vsz     = v0;
  aspect  = 1.0;
}


//= Set up initially with no buffer at all.

void jhcImg::null_img ()
{
  w = 0;
  h = 0;
  nf = 0;
  bsize = 0;
}


/////////////////////////////////////////////////////////////////////////////
//                   Different Color Format for MMX                        //
/////////////////////////////////////////////////////////////////////////////

//= Get normal pixel buffer for reading, indicate preferred color format.
// will only swizzle or de-swizzle only if RGB is stale
// generic replacement for old direct access "Pixel" variable

const UC8 *jhcImg::PxlSrc (int split)
{

return PxlDest(split);

  if (nf != 3)
    return Buffer;
  if (split > 0)
  {
    ForceSep(0);
    return Stacked;
  }
  ForceMix(0);
  return Buffer;
}


//= Get normal pixel buffer for writing, indicate preferred color format.
// will only swizzle or de-swizzle only if needed
// if this is a complete rewrite (FullRoi) then could skip de-swizzle

UC8 *jhcImg::PxlDest (int split)
{
  if (nf != 3)
    return Buffer;
  if (split > 0)
  {
    ForceSep(1);
    return Stacked;
  }
  ForceMix(1);
  return Buffer;
}


//= Full size of the buffer returned by PxlSrc or PxlDest.
// generic replacement for old "Size" function

int jhcImg::PxlSize (int split) const
{
  if (nf != 3)
    return bsize;
  if (split > 0)    // assumes separation was done successfully
    return ssize;
  return bsize;
}


//= Make sure a version of the image exists in BBB...GGG...RRR... format.
// can optionally invalidate normal copy of image

void jhcImg::ForceSep (int bad_norm)
{
  if (nf != 3)
    return;
  if (sep <= 0)
  {
    if (Stacked == NULL)
      alloc_rgb();
    swizzle(Stacked, Buffer);
    sep = 1;
  }
  if (bad_norm > 0)
    norm = 0;
}


//= Make sure a version of the image exists in BGRBGRBGR... format.
// can optionally invalidate separate color planes copy of image

void jhcImg::ForceMix (int bad_sep)
{
  if (nf != 3)
    return;
  if (norm <= 0)
  {
    deswizz(Buffer, Stacked);
    norm = 1;
  }
  if (bad_sep > 0)
    sep = 0;
}


//= De-interlace RGB values into separate contiguous color planes.
// Note: all images lines are guaranteed to be multiples of 4 bytes

void jhcImg::swizzle (UC8 *dest, const UC8 *src)
{
  int x, y, w4 = w >> 2;
  const UC8 *s = src;
  UL32 *d = (UL32 *) dest;
  UL32 fsz = psize >> 2;

  for (y = h; y > 0; y--)
    for (x = w4; x > 0; x--, d++, s += 12)
    {
      // extract from B1-R0:G0:B0, G2:B2-R1:G1, R3:G3:B3-R2 color like B3:B2:B1:B0
      d[0]         = MBYTE0(s[0]) | MBYTE1(s[3]) | MBYTE2(s[6]) | MBYTE3(s[9]);
      d[fsz]       = MBYTE0(s[1]) | MBYTE1(s[4]) | MBYTE2(s[7]) | MBYTE3(s[10]);
      d[fsz + fsz] = MBYTE0(s[2]) | MBYTE1(s[5]) | MBYTE2(s[8]) | MBYTE3(s[11]);
    }
}


//= Convert contiguous color planes into normal interlaced RGB values.
// Note: all images lines are guaranteed to be multiples of 4 bytes

void jhcImg::deswizz (UC8 *dest, const UC8 *src)
{
  int x, y, w4 = w >> 2;
  const UL32 *s = (const UL32 *) src;
  UL32 *d = (UL32 *) dest;
  UL32 bv, gv, rv, fsz = psize >> 2;

  for (y = h; y > 0; y--)
    for (x = w4; x > 0; x--, s++, d += 3)
    {
      // read in 4 pixels of each color like B3:B2:B1:B0
      bv = s[0];
      gv = s[fsz];
      rv = s[fsz + fsz];

      // mix so byte 0 = B1-R0:G0:B0, byte 1 = G2:B2-R1:G1, byte 2 = R3:G3:B3-R2
      d[0] = MBYTE0(BYTE0(bv)) | MBYTE1(BYTE0(gv)) | MBYTE2(BYTE0(rv)) | MBYTE3(BYTE1(bv));
      d[1] = MBYTE0(BYTE1(gv)) | MBYTE1(BYTE1(rv)) | MBYTE2(BYTE2(bv)) | MBYTE3(BYTE2(gv));
      d[2] = MBYTE0(BYTE2(rv)) | MBYTE1(BYTE3(bv)) | MBYTE2(BYTE3(gv)) | MBYTE3(BYTE3(rv));
    }
}


//= Make up an equivalent stacked RGB image of appropriate dimensions.

void jhcImg::alloc_rgb ()
{
  if (Stacked != NULL)
    return;

  // figure out sizes and strides
  sline = ((w + 3) >> 2) << 2;                 // force a multiple of 4
  sskip = sline - w;
  psize = ((sline * h + 15) >> 4) << 4;        // end on 16 byte boundary
  ssize = 3 * psize;

  // do actual allocation
  if (ssize > 0)
  {
#ifdef JHC_MMX
    Stacked = (UC8 *) _mm_malloc(ssize, 16);  // needed for SSE2
#else
    Stacked = new UC8 [ssize];
#endif
    if (Stacked == NULL)
      Fatal("jhcImg::alloc_rgb - Pixel buffer (%d %d) x 3 allocation failed!", w, h);
  }
}


/////////////////////////////////////////////////////////////////////////////
//                   Region Of Interest Manipulation                       //
/////////////////////////////////////////////////////////////////////////////

//= Copies location and areas sizes but does not change clipping (image size).

void jhcImg::CopyRoi (const jhcRoi& src)
{
  rx = src.RoiX();
  ry = src.RoiY();
  rw = src.RoiW();
  rh = src.RoiH();
  fix_roi();
}


//= Tells whether left and right edge of ROI are aligned on 4 byte boundaries.
// this condition can speed up some kinds of processing

int jhcImg::RoiMod4 () const
{
  if ((rx % 4 == 0) && (rw % 4 == 0))
    return 1;
  return 0;
}


//= Tells how many bytes are in each row of the ROI.

int jhcImg::RoiCnt () const
{
  return(rw * nf);
}


//= Tells how many bytes to skip over after right edge of ROI.
// to advance to left edge of ROI on next row

int jhcImg::RoiSkip (int wid) const
{
  int aw = __min(wid, w);

  if (aw <= 0)
    aw = rw;
  return(line_len - aw * nf);
}


//= Like other RoiSkip but use width of ROI from reference image.

int jhcImg::RoiSkip (const jhcRoi& ref) const
{
  return RoiSkip(ref.RoiW());
}


//= Tells how many bytes to advance from start of buffer to get to ROI.
// can use different range than recorded internally if cx or cy nonzero
// Note: used to use rx and ry if cx and cy were zero

int jhcImg::RoiOff (int cx, int cy) const
{
  int ex = __min(cx, w - 1), ey = __min(cy, h - 1);

  if (ex < 0)
    ex = rx;
  if (ey < 0)
    ey = ry;
  return((UL32) ey * line_len + ex * nf);
}


//= Like other RoiOff but uses ROI corner specified in reference item.

int jhcImg::RoiOff (const jhcRoi& ref) const
{
  return RoiOff(ref.RoiX(), ref.RoiY());
}


//= Pointer into pixel array at starting corner of ROI.
// generic replacement for old "RoiStart" function

const UC8 *jhcImg::RoiSrc (int cx, int cy) const
{
  return(PxlSrc() + RoiOff(cx, cy));
}


//= Pointer into pixel array at starting corner of ROI.

UC8 *jhcImg::RoiDest (int cx, int cy)
{
  return(PxlDest() + RoiOff(cx, cy));
}


//= Pointer into pixel array at starting corner matching ROI in reference image.

const UC8 *jhcImg::RoiSrc (const jhcRoi& ref) const
{
  return(PxlSrc() + RoiOff(ref));
}


//= Pointer into pixel array at starting corner matching ROI in reference image.

UC8 *jhcImg::RoiDest (const jhcRoi& ref)
{
  return(PxlDest() + RoiOff(ref));
}


//= Computes skip and offset to match ROI of destination image.
// useful for raw arrays, needs number of fields (bytes / pixel)

void jhcImg::RoiParams (UL32 *off, int *sk, int snf) const
{
  int sln = ((w * snf + 3) >> 2) << 2;

  *off = (UL32) ry * sln + rx * snf;
  *sk = sln - rw * snf;
}


//= Computes skip and offset for given ROI based on image sizes.

void jhcImg::RoiParams (UL32 *off, int *sk, const jhcRoi& src) const
{
  *off = (UL32) src.RoiY() * line_len + src.RoiX() * nf;
  *sk = line_len - src.RoiW() * nf;
}


//= Computes skip and offset for given corner and width.

void jhcImg::RoiParams (UL32 *off, int *sk, int rx, int ry, int rw) const
{
  *off = (UL32) ry * line_len + rx * nf;
  *sk = line_len - rw * nf;
}


//= Alters given ROI x parameters so that limits fall on 4 byte boundaries.
// lo, hi values: -1 expands to left if needed, +1 to right, 0 to closest
// makes rx and rw multiples of 4 (intentionally ignores case where nf is even)
// sometimes 32 bit aligned regions are faster to process

void jhcImg::RoiAdj_4 (int lo, int hi)
{
  int phase, cph, rx2 = rx + rw;

  // see if left ROI edge aligned with 32 bit boundary
  if ((phase = rx % 4) != 0)
  {
    cph = 4 - phase;
    if ((lo < 0) ||
        ((lo == 0) && (phase <= 2)) ||
        ((rx + cph) >= w))
      rx -= phase;
    else
      rx += cph;
  }

  // see if right ROI edge aligned with 32 bit boundary
  if ((phase = (rx + rw) % 4) != 0)
  {
    cph = 4 - phase;
    if ((hi < 0) ||
        ((hi == 0) && (phase < 2)) ||
        ((rx2 + cph) > w))
      rx2 -= phase;
    else
      rx2 += cph;
  }

  // recompute iteration parameters
  rw = rx2 - rx;
}


/////////////////////////////////////////////////////////////////////////////
//                             Simple tests                                //
/////////////////////////////////////////////////////////////////////////////

//= Array gets width, height, and nuber of fields (for creating other images).

int *jhcImg::Dims (int specs[]) const
{
  specs[0] = w;
  specs[1] = h;
  specs[2] = nf;
  return specs;
}


//= Sees if image has either one of two different field counts.

int jhcImg::Valid (int df1, int df2) const
{
  if (Buffer == NULL)
    return 0;
  if ((nf != df1) && (nf != df2))
    return 0;
  return 1;
}


//= Sees if current dimensions on image are reasonable.
// if df is non-zero, checks that image has that many fields

int jhcImg::Valid (int df) const
{
  if (Buffer == NULL)
    return 0;
  if ((df != 0) && (df != nf))
    return 0;
  return 1;
}


//= Returns 1 if test image is really this image.

int jhcImg::SameImg (const jhcImg& tst) const
{
  if (&tst == this)
    return 1;
  return 0;
}


//= Returns 1 if test image is really this image.

int jhcImg::SameImg0 (const jhcImg *tst) const
{
  if (tst == NULL)
    return 0;
  return SameImg(*tst);
}


//= See if two images have the same dimensions and scan direction.
// if df is non-zero then check that test image has that many fields

int jhcImg::SameSize (const jhcImg& tst, int df) const
{
  if (!Valid() || !tst.Valid() || (tst.w != w) || (tst.h != h))
    return 0;
  if ((df != 0) && (tst.nf != df))
    return 0;
  return 1;
}


//= Same as SameSize but returns OK if null pointer given.
// useful for checking sizes of optional image arguments

int jhcImg::SameSize0 (const jhcImg *tst, int df) const
{
  if (tst == NULL)
    return 1;
  return SameSize(*tst, df);
}


//= See if two images have same dimensions, scan order, and number of fields.

int jhcImg::SameFormat (const jhcImg& tst) const
{
  return SameSize(tst, nf);
}


//= Same as basic version but returns OK if null pointer given.
// useful for checking sizes of optional image arguments

int jhcImg::SameFormat0 (const jhcImg *tst) const
{
  if (tst == NULL)
    return 1;
  return SameSize(*tst, nf);
}


//= See if dimensions in specification array match the current image.

int jhcImg::SameFormat (const int specs[]) const
{
  return SameFormat(specs[0], specs[1], specs[2]);
}


//= Check image against an explict size and depth.
// set fields = 0 if any number is okay

int jhcImg::SameFormat (int width, int height, int fields) const
{
  if (!Valid() || (width != w) || (height != h) || ((fields > 0) && (fields != nf)))
    return 0;
  return 1;
}


//= Check if image has square pixels (assumes 4:3 aspect ratio)
// alternative to calling the Ratio function and testing

int jhcImg::Square () const
{
  if ((3 * w) == (4 * h))
    return 1;
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//                          Copying functions                              //
/////////////////////////////////////////////////////////////////////////////

//= Copy pixels to some other provided array.
// does not do any size or format checking
// dumps all pixels, not just those in the ROI

void jhcImg::DumpAll (UC8 *dest)
{
  if (!Valid())
    return;
  if (sep > 0)
    deswizz(dest, Stacked);
  else if (dest != Buffer)
    memcpy(dest, Buffer, bsize);
}


//= Like DumpAll but converts from bottom up to top down.
// does not do any size or format checking
// dumps all pixels, not just those in the ROI
// always dumps in interlaced RGB format

void jhcImg::DumpFlip (UC8 *dest) const
{
  const UC8 *s = Buffer + (h - 1) * line_len;
  UC8 *d = dest;
  int y;

  // make sure normal BGR format exists (if color)
  if (!Valid() || (norm <= 0) || (dest == Buffer))
    return;

  // read upside down
  for (y = h; y > 0; y--)
  {
    memcpy(d, s, line_len);
    s -= line_len;
    d += line_len;
  }
}


//= Loads up image with data from some other buffer.
// ignores ROI of image (hopefully there are enough src pixels)
// assumes source is not swizzled

void jhcImg::LoadAll (const UC8 *src)
{
  if (Valid() && (Buffer != src))
    memcpy(Buffer, src, bsize);
  norm = 1;
  sep = 0;
}


//= Like LoadAll but converts from top down to bottom up.
// ignores ROI of image (hopefully there are enough src pixels)
// assumes source is not swizzled

void jhcImg::LoadFlip (const UC8 *src)
{
  const UC8 *s = src;
  UC8 *d = Buffer + (h - 1) * line_len;
  int y;

  if (Valid() && (Buffer != src))  // cannot do in place
    for (y = h; y > 0; y--)
    {
      memcpy(d, s, line_len);
      s += line_len;
      d -= line_len;
    }
  norm = 1;
  sep = 0;
}


//= Copy all pixels of source image irrespective of ROI settings.
// does not alter ROI of receiving image
// forces image to match swizzle state of source

int jhcImg::LoadAll (const jhcImg& src)
{
  if (!SameFormat(src))
    return Fatal("Bad image to jhcImg::LoadAll");
  if (&src == this)
    return 1;
  if (src.sep > 0)
  {
    if (Stacked == NULL)
      alloc_rgb();
    memcpy(Stacked, src.Stacked, ssize);
    sep = 1;
    norm = 0;
  }
  else
  {
    memcpy(Buffer, src.Buffer, bsize);
    norm = 1;
    sep = 0;
  }
  return 1;
}


//= Copy image pointed to or clear image if NULL pointer given.

int jhcImg::CopyClr (const jhcImg *src, int def)
{
  if (src == NULL)
    return FillArr(def);
  return CopyArr(*src);
}


//= Copy contents of another image into this one.
// also sets ROI of this image (destination) to match source
// therefore only transfers pixels in ROI of other image (source)
// if ROI is maximum, preserves swizzle state of source

int jhcImg::CopyArr (const jhcImg& src)
{
  if (!SameFormat(src))
    return Fatal("Bad image to jhcImg::CopyArr");
  if (&src == this)
    return 1;
  CopyRoi(src);
  if (FullRoi())
    return LoadAll(src);
  return CopyArr(src.PxlSrc());
}


//= Useful variation of CopyArr for Windows DIBs.
// assumes receiving image is the correct size!
// assumes source is not swizzled

int jhcImg::CopyArr (const UC8 *src)
{
  if (!Valid())
    return 0;

  // quick cases
  if (FullRoi())
  {
    LoadAll(src);
    return 1;
  }
  if (RoiMod4() != 0)
    return copy_arr_4(src);

  // arbitrary ROI case
  int x, y, rcnt = RoiCnt(), rsk = RoiSkip();
  UL32 roff = RoiOff();
  const UC8 *s = src + roff;
  UC8 *d = PxlDest() + roff;

  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = *s++;
    d += rsk;
    s += rsk;
  }
  return 1;
}


//= Copy just a portion of the image that falls in specified area.
// ignores ROI of src image

int jhcImg::CopyArr (const jhcImg& src, const jhcRoi& area)
{
  if (!SameFormat(src))
    return Fatal("Bad image to jhcImg::CopyArr");
  if (&src == this)
    return 1;

  // local variables
  UC8 *d = RoiDest(area);
  const UC8 *s = src.RoiSrc(area);
  int rcnt = area.RoiW() * Fields(), rh = area.RoiH();
  int x, y, sk = src.RoiSkip(area);

  // copy inside specified region only
  for (y = 0; y < rh; y++, d += sk, s += sk)
    for (x = 0; x < rcnt; x++, d++, s++)
      *d = *s;
  CopyRoi(area);
  return 1;
}


//= Moves 32 bits at a time when x limits of ROI on 4 byte boundaries.

int jhcImg::copy_arr_4 (const UC8 *src)
{
  int x, y, rc4 = RoiCnt() >> 2, sk4 = RoiSkip() >> 2;
  UL32 roff = RoiOff();
  const UL32 *s = (UL32 *)(src + roff);
  UL32 *d = (UL32 *)(PxlDest() + roff);

  for (y = rh; y > 0; y--)
  {
    for (x = rc4; x > 0; x--)
      *d++ = *s++;
    d += sk4;
    s += sk4;
  }
  return 1;
}


//= Copy just one field of other array into some field of this one.
// also sets ROI of this image (destination) to match source
// therefore only transfers pixels in ROI of other image (source)

int jhcImg::CopyField (const jhcImg& src, int sfield, int dfield)
{
  if (!SameSize(src))
    return Fatal("Bad image to jhcImg::CopyField");
  if ((dfield < 0) || (dfield >= nf) ||
      (sfield < 0) || (sfield >= src.nf))
    return 0;
  if ((&src == this) && (sfield == dfield))
    return 1;
  CopyRoi(src);
/*
// figure out RGB to mono and others
  if ((sep > 0) && FullRoi())
  {
    if (src.sep > 0)
      memcpy(Stacked + dfield * psize, src.Stacked + sfield * psize, psize);
  }
*/
  return CopyField(src.PxlSrc(), sfield, src.nf, dfield);
}


//= Useful variation of CopyField for Windows DIBs.
// assumes receiving image is the correct size!
// assumes source is not swizzled

int jhcImg::CopyField (const UC8 *src, int sfield, int stotal, int dfield)
{
  int x, y, ssk, rsk = RoiSkip();
  UL32 soff;
  const UC8 *s = src + BYTEOFF(sfield, stotal);
  UC8 *d = PxlDest() + RoiOff() + BYTEOFF(dfield, nf);

  if (!Valid())
    return 0;
  RoiParams(&soff, &ssk, stotal);
  s += soff;
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      *d = *s;
      d += nf;
      s += stotal;
    }
    d += rsk;
    s += ssk;
  }
  return 1;
}


//= Force a multi-plane image into a monochrome image by clipping values to 255.
// adjust size of self to match dimensions of source but with a single plane
// usually more meaningful than just copying the bottom field
// assumes source is not swizzled

int jhcImg::Sat8 (const jhcImg& src)
{
  int x, y, dsk, sf = src.Fields(), ssk = src.Skip();
  const US16 *s16 = (const US16 *) src.PxlSrc();
  const UL32 *s32 = (const UL32 *) src.PxlSrc();
  UC8 *d;

  // does not work for RGB images
  if ((sf != 1) && (sf != 2) && (sf != 4))
    return 0;

  // make self correct size and test for simplest case
  SetSize(src, 1);
  if (sf == 1)
    return CopyArr(src);
  d = PxlDest();
  dsk = Skip();

  // copy 16 bit values with saturation
  if (sf == 2)
  {
    ssk >>= 1;
    for (y = src.YDim(); y > 0; y--, d += dsk, s16 += ssk)
      for (x = w; x > 0; x--, d++, s16++)
        *d = (UC8) __min(*s16, 255);
    return 1;
  }

  // copy 32 bit values with saturation
  ssk >>= 2;
  for (y = src.YDim(); y > 0; y--, d += dsk, s32 += ssk)
    for (x = w; x > 0; x--, d++, s32++)
      *d = (UC8) __min(*s32, 255);
  return 1;
}


//= Fill all fields of image with same value.
// only affects pixels in specified ROI (i.e. draws filled rectangle)
// if ROI is maximum, preserves swizzle state

int jhcImg::FillArr (int v)
{
  UC8 val = (UC8) v;

  if (!Valid())
    return 0;
  if (FullRoi())
  {
    if (sep > 0)
      memset(Stacked, val, ssize);
    else
      memset(Buffer, val, bsize);
    return 1;
  }
  if (RoiMod4() != 0)
    return fill_arr_4(val);

  // arbitrary ROI case
  int x, y, rcnt = RoiCnt(), rsk = RoiSkip();
  UC8 *d = PxlDest() + RoiOff();

  for (y = rh; y > 0; y--)
  {
    for (x = rcnt; x > 0; x--)
      *d++ = val;
    d += rsk;
  }
  return 1;
}


//= Quicker version for ROI lined up on long word boundaries.

int jhcImg::fill_arr_4 (UC8 val)
{
  int x, y, rc4 = RoiCnt() >> 2, sk4 = RoiSkip() >> 2;
  UL32 *d = (UL32 *)(PxlDest() + RoiOff());
  UL32 v4 = (val << 24) | (val << 16) | (val << 8) | val;

  for (y = rh; y > 0; y--)
  {
    for (x = rc4; x > 0; x--)
      *d++ = v4;
    d += sk4;
  }
  return 1;
}


//= Fill whole image with given value irrespective of ROI.

int jhcImg::FillAll (int v)
{
  jhcRoi orig(*this);

  MaxRoi();
  FillArr(v);
  CopyRoi(orig);
  return 1;
}


//= Fill just one field of array with particular value.
// if ROI is maximum, preserves swizzle state

int jhcImg::FillField (int v, int field)
{
  UC8 val = (UC8) v;

  if (!Valid() || (field < 0) || (field >= nf))
    return 0;
  if ((sep > 0) && FullRoi())
  {
    memset(Stacked + field * psize, val, psize);
    return 1;
  }

  // general ROI case
  int x, y, rsk = RoiSkip();
  UC8 *d = PxlDest() + RoiOff() + field;

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d += nf)
      *d = val;
    d += rsk;
  }
  return 1;
}


//= Fill color image with a solid color.
// if ROI is maximum, preserves swizzle state

int jhcImg::FillRGB (int r, int g, int b)
{
  UC8 rv = BOUND(r), gv = BOUND(g), bv = BOUND(b);

  if (!Valid(3))
    return 0;

  if ((gv == rv) && (bv == rv))
    return FillArr(rv);
  if ((sep > 0) && FullRoi())
  {
    memset(Stacked, bv, psize);
    memset(Stacked + psize, gv, psize);
    memset(Stacked + psize + psize, rv, psize);
    return 1;
  }

  // arbitrary ROI case
  int x, y, rsk = RoiSkip();
  UC8 *d = PxlDest() + RoiOff();

  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, d += 3)
    {
      d[0] = bv;
      d[1] = gv;
      d[2] = rv;
    }
    d += rsk;
  }
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                              Bounds Checking                            //
/////////////////////////////////////////////////////////////////////////////

//= See if requested address is inside image or not.

int jhcImg::InBounds (int x, int y, int f) const
{
  if ((x < 0) || (x >= w) || (y < 0) || (y >= h) || (f < 0) || (f >= nf))
    return 0;
  return 1;
}


//= Force coordinates to be within bounds of image.
// returns 1 if any changes made, 0 if initially OK

int jhcImg::ClipCoords (int *x, int *y, int *f) const
{
  int ans = 3;

  if (*x >= w)
    *x = w - 1;
  else if (*x < 0)
    *x = 0;
  else
    ans--;

  if (*y >= h)
    *y = h - 1;
  else if (*y < 0)
    *y = 0;
  else
    ans--;

  if (*f >= nf)
    *y = nf - 1;
  else if (*f < 0)
    *f = 0;
  else
    ans--;

  if (ans > 0)
    return 1;
  return 0;
}


//= Report size of image in format "(w h) x f").

const char *jhcImg::SizeTxt ()
{
  return SizeTxt(msg, sizeof msg);
}


//= Report size of image into externally supplied string.

const char *jhcImg::SizeTxt (char *ans, int ssz) const
{
  if (ans != NULL)
    sprintf_s(ans, ssz, "(%d %d) x %d", w, h, nf);
  return ans;
}


//= Check if pixel array exists and indices are valid.

int jhcImg::BoundChk (int x, int y, int f, const char *fcn) const
{
  if ((Buffer == NULL) || !InBounds(x, y, f))
  {
#ifdef _DEBUG
    Pause("jhcImg::BoundsChk - %s(%d, %d, %d) is beyond image (%d %d) x %d", fcn, x, y, f, w, h, nf);
#endif
    return 0;
  }
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                              Pixel access                               //
/////////////////////////////////////////////////////////////////////////////

//= Compute buffer location based on coordinates.
// if outside of image return NULL

UC8 *jhcImg::APtrChk (int x, int y, int f)
{
  if (!BoundChk(x, y, f, "APtr"))
    return NULL;
  return aptr0(x, y, f);
}


//= Get pixel value at location (x y) from field f.
// check that address is valid, if not return default value

int jhcImg::ARefChk (int x, int y, int f, int def) const
{
  if (!BoundChk(x, y, f, "ARef"))
    return def;
  return aref0(x, y, f);
}


//= Set pixel at location (x y) in field f to given value.
// returns 0 if address is invalid, 1 is write succeeds

int jhcImg::ASetChk (int x, int y, int f, int val)
{
  if (!BoundChk(x, y, f, "ASet"))
    return 0;
  if ((val < 0) || (val > 255))
  {
#ifdef _DEBUG
    Pause("jhcImg::ASetChk - %d out of range [0 255]", val);
#endif
    return 0;
  }

  aset0(x, y, f, val);
  return 1;
}


//= Get 16 bit pixel value at location (x y).
// check that address is valid, if not return default value

int jhcImg::ARefChk16 (int x, int y, int def) const
{
  if (!BoundChk(x, y, 0, "ARef16"))
    return def;
  return aref_16(x, y);
}


//= Set 16 bit pixel at location (x y) to given value.
// returns 0 if address is invalid, 1 is write succeeds

int jhcImg::ASetChk16 (int x, int y, int val)
{
  if (!BoundChk(x, y, 0, "ASet16"))
    return 0;
  if ((val < 0) || (val > 65535))
  {
#ifdef _DEBUG
    Pause("jhcImg::ASetChk16 - %d out of range [0 65535]", val);
#endif
    return 0;
  }

  aset_16(x, y, val);
  return 1;
}


//= Get 32 bit pixel value at location (x y).
// check that address is valid, if not return default value

UL32 jhcImg::ARefChk32 (int x, int y, int def) const
{
  if (!BoundChk(x, y, 0, "ARef32"))
    return def;
  return aref_32(x, y);
}


//= Set 32 bit pixel at location (x y) to given value.
// returns 0 if address is invalid, 1 is write succeeds

int jhcImg::ASetChk32 (int x, int y, UL32 val)
{
  if (!BoundChk(x, y, 0, "ASet32"))
    return 0;
  aset_32(x, y, val);
  return 1;
}


//= Get values of R, G, and B for pixel.
// check that address is valid, if not return default values

int jhcImg::ARefColChk (int *r, int *g, int *b, int x, int y,
                        int rdef, int gdef, int bdef) const
{
  *b = bdef;
  *g = gdef;
  *r = rdef;
  if (!BoundChk(x, y, 0, "ARefCol"))
    return 0;
  if (nf != 3)
  {
#ifdef _DEBUG
    Pause("jhcImg::ARefColChk - Image has %d fields", nf);
#endif
    return 0;
  }

  aref_col0(r, g, b, x, y);
  return 1;
}


//= Set some pixel to new RGB value.
// returns 0 if address is invalid, 1 is write succeeds

int jhcImg::ASetColChk (int x, int y, int r, int g, int b)
{
  if (!BoundChk(x, y, 0, "ASetCol"))
    return 0;
  if (nf != 3)
  {
#ifdef _DEBUG
    Pause("jhcImg::ASetColChk - Image has %d fields", nf);
#endif
    return 0;
  }
  if ((r < 0) || (r > 255) || (g < 0) || (g > 255) || (b < 0) || (b > 255))
  {
#ifdef _DEBUG
    Pause("jhcImg::ASetColChk - %d %d %d out of range [0 255]", r, g, b);
#endif
    return 0;
  }

  aset_col0(x, y, r, g, b);
  return 1;
}


//= Find buffer location referenced by coordinates.
// assumes write might happen

UC8 *jhcImg::aptr0 (int x, int y, int f)
{
  return(PxlDest() + y * line_len + x * nf + f);
}


//= Get value of some pixel.

int jhcImg::aref0 (int x, int y, int f) const
{
  const UC8 *loc = PxlSrc() + y * line_len + x * nf;

  return((int)(*(loc + f)));
}


//= Set some pixel to a new value.

void jhcImg::aset0 (int x, int y, int f, int val)
{
  *aptr0(x, y, f) = (UC8) val;
}


//= Get 16 bit value of pixel.

int jhcImg::aref_16 (int x, int y) const
{
  const US16 *loc = (US16 *)(PxlSrc() + y * line_len + x * nf);

  return *loc;
}


//= Set some pixel to new 16 bit value.

void jhcImg::aset_16 (int x, int y, int val)
{
  US16 *loc = (US16 *)(PxlSrc() + y * line_len + x * nf);

  *loc = (US16) val;
}


//= Get 32 bit value of pixel.

UL32 jhcImg::aref_32 (int x, int y) const
{
  const UL32 *loc = (UL32 *)(PxlSrc() + y * line_len + x * nf);

  return *loc;
}


//= Set some pixel to new 32 bit value.

void jhcImg::aset_32 (int x, int y, UL32 val)
{
  UL32 *loc = (UL32 *)(PxlSrc() + y * line_len + x * nf);

  *loc = val;
}


//= Get values of R, G, and B for pixel.

void jhcImg::aref_col0 (int *r, int *g, int *b, int x, int y) const
{
  const UC8 *loc = PxlSrc() + y * line_len + x * nf;

  *b = (int) loc[0];
  *g = (int) loc[1];
  *r = (int) loc[2];
}


//= Set some pixel to new RGB value.

void jhcImg::aset_col0 (int x, int y, int r, int g, int b)
{
  UC8 *loc = aptr0(x, y, 0);

  loc[0] = (UC8) b;
  loc[1] = (UC8) g;
  loc[2] = (UC8) r;
}


//= Set monochrome pixel if coordinates are reasonable.
// useful for graphics since no complaint if outside image
// only works with single field images (for speed)

void jhcImg::ASetOK (int x, int y, int val)
{
  if ((Buffer == NULL) || (nf != 1) || (x < 0) || (x >= w) || (y < 0) || (y >= h))
    return;
  *(PxlDest() + y * line_len + x) = (UC8) val;
}


//= Set color pixel if coordinates are reasonable.
// useful for graphics since no complaint if outside image

void jhcImg::ASetColOK (int x, int y, int r, int g, int b)
{
  UC8 *base;

  if ((Buffer == NULL) || (nf != 3) || (x < 0) || (x >= w) || (y < 0) || (y >= h))
    return;
  base = PxlDest() + y * line_len + x * 3;
  base[0] = (UC8) b;
  base[1] = (UC8) g;
  base[2] = (UC8) r;
}


//= Set monochrome pixel but check if it exceeds image bounds.
// if clip <= 0 then only draws point if inside image
// if clip > 0 then constrains x and/or y to be inside image

void jhcImg::ASetClip (int x, int y, int val, int clip)
{
  int xc, yc;

  if ((Buffer == NULL) || (nf != 1))
    return;
  xc = __max(0, __min(x, w - 1));
  yc = __max(0, __min(y, h - 1));
  if ((clip > 0) || ((x == xc) && (y == yc)))
    *(PxlDest() + yc * line_len + xc) = (UC8) val;
}


//= Set color pixel but check if it exceeds image bounds.
// if clip <= 0 then only draws point if inside image
// if clip > 0 then constrains x and/or y to be inside image

void jhcImg::ASetColClip (int x, int y, int r, int g, int b, int clip)
{
  UC8 *base;
  int xc, yc;

  if ((Buffer == NULL) || (nf != 3))
    return;
  xc = __max(0, __min(x, w - 1));
  yc = __max(0, __min(y, h - 1));
  if ((clip > 0) || ((x == xc) && (y == yc)))
  {
    base = PxlDest() + yc * line_len + xc * 3;
    base[0] = (UC8) b;
    base[1] = (UC8) g;
    base[2] = (UC8) r;
  }
}


