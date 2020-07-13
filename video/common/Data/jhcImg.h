// jhcImg.h : interface to the basic image class
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

#ifndef _JHCIMG_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCIMG_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include <stdlib.h>

#include "Data/jhcRoi.h"


//= Interface to the basic image class.
// The basic image class is derived from the ROI (Region of Interest) class.
// Handling iteration just within a ROI adds almost nothing to the processing
// time, yet limiting the number of pixels touched can yield a big speed-up.
//
// Pixels are 8 bit monochrome values or 24 bit BGR triples
// Values in reverse scan order = left to right but bottom up
// Line lengths are padded to multiples of 4 bytes (32 bit boundaries)
// Aspect is physical width of sensing pixel divided by length
// Region Of Interest (ROI) coordinates optionally restrict processing
//
// If you don't want to use any of the libraries you can just ignore
// most of the wrapper and only work directly with the Pixels buffer.
// This can even be cast to a different type, e.g.
// <pre>
//   jhcImage img(640, 480, 2);
//   US16 *rgb555 = (US16 *) img.Pixels;
//     ... program using rgb555 as the basic image ...
// </pre>
//
// Enhanced to allow schizophrenic dual-mode color image representation.
// Separated color planes is more efficient for MMX/SSE operations.
// <pre>
//   Buffer  = BGR.BGR.BGR.BGR...     (norm)
//   Stacked = BBBB...GGGG...RRRR...  (sep)
// </pre>
// Maintains cache clean bits "sep" and "norm" for which versions are valid.
// Pixel access broken into read and write functions to help maintain this.
// Note that normal and separated formats may both be valid at the same time.
// PrefRGB helps determine which format requires fewer conversions.
//
// If vsz > 0 then if size changed to smaller one, keeps larger buffer (no alloc).
// Useful if repeatedly extracting some varying size portion of a larger image.

class jhcImg : public jhcRoi
{
// PRIVATE MEMBER VARIABLES
private:
  int wrap, nf, end_skip, line_len;
  int norm, sep, sskip, sline;
  int bsize, asize;
  int ssize, psize;
  double aspect;
  char msg[20];
  UC8 *Buffer;   // actual bytes for pixels
  UC8 *Stacked;


// PUBLIC MEMBER VARIABLES
public:
  int status;  /** Whether image should be displayed.   */
  int vsz;     /** Whether to reuse buffer if possible. */


// PUBLIC MEMBER FUNCTIONS
public:
  // image creation and sizing
  ~jhcImg ();
  jhcImg ();
  jhcImg (int width, int height, int fields =1);
  jhcImg (const int specs[]);
  jhcImg (const jhcImg& ref, int fields =0);
  jhcImg (const jhcImg& ref, double factor, int fields =0);
  jhcImg (const jhcImg& ref, int hdes, int fields);

  jhcImg *SetSize (int wd, int ht, int fields =1, double a =0.0);
  jhcImg *SetSize (const int specs[]);
  jhcImg *SetSize (const jhcImg *ref, int fields =0) {return SetSize(*ref, fields);}
  jhcImg *SetSize (const jhcImg& ref, int fields =0);
  jhcImg *SetSize (const jhcImg& ref, double factor, int fields =0);
  jhcImg *SetSize (const jhcImg& ref, int hdes, int fields);
  void InitSize (const jhcImg& ref, int val =0) {SetSize(ref); FillArr(val);}
  jhcImg *Release () {return SetSize(0, 0, 0);}
  jhcImg *AdjSize (int wd, int ht, int fields, int hdes, double a =0.0);
  jhcImg *MaxSize (const jhcImg& ref, int nsz =480, int fields =0);
  jhcImg *MinSize (const jhcImg& ref, int nsz =480, int fields =0);

  jhcImg *SetSquare (const jhcImg& ref, int fields =0);
  jhcImg *SetSquare (const jhcImg& ref, int hdes, int fields);
  jhcImg *SetSquare (int wid, int ht, int fields);
  jhcImg *AdjSquare (int wid, int ht, int fields, int hdes);

  jhcImg *Wrap (UC8 *raw, int wid =0, int ht =0, int fields =0);
  void Clone (const jhcImg& ref) {SetSize(ref); if (ref.Valid()) CopyArr(ref);}  /** Copy size and content. */
  void Clone (const jhcImg *ref) {Clone(*ref);}

  // member variable access
  int *Dims (int specs[]) const;
  int Fields ()   const {return nf;}             /** Number of fields in the image.                */
  int Skip ()     const {return end_skip;}       /** Number of bytes to skip at the end of a line. */
  int Line ()     const {return line_len;}       /** Total number of bytes in a line incl. skip.   */
  int RowCnt ()   const {return(w * nf);}        /** Total number of bytes in active part of line. */
  int PxlCnt ()   const {return(w * h);}         /** Total number of pixels in image (not ROI).    */
  double Ratio () const {return aspect;}         /** Aspect ratio of pixels.                       */
  int SepRGB ()   const {return sep;}            /** Whether a valid separated RBG form exists.    */
  int MixRGB ()   const {return norm;}           /** Whether a valid interleaved RGB form exists.  */
  int PrefRGB ()  const {return(sep - norm);}    /** Whether it is easier to use separated form.   */
  int SepOff ()   const {return psize;}          /** Offset between color fields if separated.     */
  void SetRatio (double a) {aspect = a;}         /** Record the pixel aspect ratio.                */

  // extra Region of Interest (ROI) functions
  void RoiClip (int xmax, int ymax) const {};    /** Not allowed (overridden jhcRoi function). */
  void RoiClip (const jhcRoi& src) const {};     /** Not allowed (overridden jhcRoi function). */
  void CopyRoi (const jhcRoi& src);              // overridden
  int RoiMod4 () const;
  int RoiCnt () const;
  int RoiSkip (int wid =0) const;
  int RoiSkip (const jhcRoi& ref) const;
  int RoiOff (int cx =-1, int cy =-1) const;
  int RoiOff (const jhcRoi& ref) const;
  const UC8 *RoiSrc (int cx =-1, int cy =-1) const;
  UC8 *RoiDest (int cx =-1, int cy =-1);
  const UC8 *RoiSrc (const jhcRoi& ref) const;
  UC8 *RoiDest (const jhcRoi& ref);
  void RoiParams (UL32 *soff, int *ssk, int snf) const;
  void RoiParams (UL32 *soff, int *ssk, const jhcRoi& src) const;
  void RoiParams (UL32 *off, int *sk, int rx, int ry, int rw) const;
  void RoiAdj_4 (int lo =0, int hi =0);

  // basic operations
  int Valid (int df =0) const;
  int Valid (int df1, int df2) const;
  int SameImg (const jhcImg& tst) const;
  int SameImg0 (const jhcImg *tst) const;
  int SameSize (const jhcImg& tst, int df =0) const;
  int SameSize0 (const jhcImg *tst, int df =0) const;
  int SameFormat (const jhcImg& tst) const;
  int SameFormat0 (const jhcImg *tst) const;
  int SameFormat (const int specs[]) const;
  int SameFormat (int width, int height, int fields =0) const;
  int Square () const;

  const UC8 *PxlSrc () const {return Buffer;}  /** Ignore any potential swizzling. */
  UC8 *PxlDest () {return Buffer;}             /** Ignore any potential swizzling. */
  const UC8 *PxlSrc (int split);
  UC8 *PxlDest (int split);
  int PxlSize (int split =0) const;
  void ForceSep (int bad_norm =0);
  void ForceMix (int bad_sep =0);

  void DumpAll (UC8 *dest);
  void DumpFlip (UC8 *dest) const;
  void LoadAll (const UC8 *src);
  void LoadFlip (const UC8 *src);
  int LoadAll (const jhcImg& src);
  int CopyClr (const jhcImg *src, int def =0);
  int CopyArr (const jhcImg& src);
  int CopyArr (const jhcImg *src) {return CopyArr(*src);}
  int CopyArr (const UC8 *src);
  int CopyArr (const jhcImg& src, const jhcRoi& area);
  int CopyField (const jhcImg& src, int sfield, int dfield =0);
  int CopyField (const UC8 *src, int sfield, int stotal =3, int dfield =0);
  int Sat8 (const jhcImg& src);
  int FillArr (int v =0);
  int FillMax (int v =0) {MaxRoi(); return FillArr(v);}  /** Fill ALL pixels. */
  int FillAll (int v =0);
  int FillField (int v, int field =0);
  int FillRGB (int r, int g, int b);

  // bounds checking
  int InBounds (int x, int y, int f =0) const;
  int ClipCoords (int *x, int *y, int *f) const;
  const char *SizeTxt ();
  const char *SizeTxt (char *ans, int ssz) const;
  int BoundChk (int x, int y, int f =0, const char *fcn =NULL) const;

  // bounds - convenience
  template <size_t ssz>
    const char *SizeTxt (char (&ans)[ssz]) const
      {return SizeTxt(ans, ssz);}

  // pixel access
  UC8 *APtrChk (int x, int y, int f =0);
  int ARefChk (int x, int y, int f =0, int def =-1) const;
  int ASetChk (int x, int y, int f, int val);
  int ARefChk16 (int x, int y, int def =-1) const;
  int ASetChk16 (int x, int y, int val);
  UL32 ARefChk32 (int x, int y, int def =0) const;
  int ASetChk32 (int x, int y, UL32 val);
  int ARefColChk (int *r, int *g, int *b, int x, int y,
                  int rdef =0, int gdef =0, int bdef =0) const;
  int ASetColChk (int x, int y, int r, int g, int b);
  void ASetOK (int x, int y, int val);
  void ASetColOK (int x, int y, int r, int g, int b);
  void ASetClip (int x, int y, int val, int clip);
  void ASetColClip (int x, int y, int r, int g, int b, int clip);

#ifndef _DEBUG
  UC8 *APtr (int x, int y, int f =0)                        {return aptr0(x, y, f);}
  int ARef (int x, int y, int f =0) const                   {return aref0(x, y, f);}
  void ASet (int x, int y, int f, int val)                  {aset0(x, y, f, val);}
  int ARef16 (int x, int y) const                           {return aref_16(x, y);}
  void ASet16 (int x, int y, int val)                       {aset_16(x, y, val);}
  UL32 ARef32 (int x, int y) const                          {return aref_32(x, y);}
  void ASet32 (int x, int y, UL32 val)                      {aset_32(x, y, val);}
  void ARefCol (int *r, int *g, int *b, int x, int y) const {aref_col0(r, g, b, x, y);}
  void ASetCol (int x, int y, int r, int g, int b)          {aset_col0(x, y, r, g, b);}
#else
  UC8 *APtr (int x, int y, int f =0)                        {return APtrChk(x, y, f);}
  int ARef (int x, int y, int f =0) const                   {return ARefChk(x, y, f);}
  void ASet (int x, int y, int f, int val)                  {ASetChk(x, y, f, val);}
  int ARef16 (int x, int y) const                           {return ARefChk16(x, y);}
  void ASet16 (int x, int y, int val)                       {ASetChk16(x, y, val);}
  UL32 ARef32 (int x, int y) const                          {return ARefChk32(x, y);}
  void ASet32 (int x, int y, UL32 val)                      {ASetChk32(x, y, val);}
  void ARefCol (int *r, int *g, int *b, int x, int y) const {ARefColChk(r, g, b, x, y);}
  void ASetCol (int x, int y, int r, int g, int b)          {ASetColChk(x, y, r, g, b);}
#endif


// PRIVATE MEMBER FUNCTIONS
private:
  void record_sizes (int wd, int ht, int fields);
  void dealloc_img();
  void init_img (int v0);
  void null_img ();

  // fast helper functions for aligned ROIs
  void swizzle (UC8 *dest, const UC8 *src);
  void deswizz (UC8 *dest, const UC8 *src);
  void alloc_rgb ();

  // faster bulk moves
  int copy_arr_4 (const UC8 *src);
  int fill_arr_4 (UC8 val);

  // unchecked fast low-level access
  UC8 *aptr0 (int x, int y, int f =0);
  int aref0 (int x, int y, int f =0) const;
  void aset0 (int x, int y, int f, int val);
  int aref_16 (int x, int y) const;
  void aset_16 (int x, int y, int val);
  UL32 aref_32 (int x, int y) const;
  void aset_32 (int x, int y, UL32 val);
  void aref_col0 (int *r, int *g, int *b, int x, int y) const;
  void aset_col0 (int x, int y, int r, int g, int b);

};


#endif
