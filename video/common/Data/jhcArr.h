// jhcArr.h : one dimensional integer array with size
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
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

#ifndef _JHCARR_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCARR_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include <stdlib.h>

#include "Interface/jhcMessage.h"

#pragma inline_depth(10)


//= One dimensional integer array with size.
// a useful primitive for things like histograms
// basically an array of ints with a fixed length
// more functions than would be provided by jhcList<int>

class jhcArr 
{
// PRIVATE MEMBER VARIABLES
private:
  int sz, i0, i1;
  int *arr;


// PUBLIC MEMBER VARIABLES
public:
  int status;  /** Whether the contents should be graphed.        */
  int len;     /** Mark a->len = n for arrays of jhcArr elements. */ 


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and destruction
  ~jhcArr ();
  jhcArr ();
  jhcArr (const jhcArr& ref);
  jhcArr (const jhcArr *ref);
  jhcArr (int n, int no_init =1);

  // size specification
  int Valid () const {return(__max(0, sz));};  /** Whether buffer has been allocated yet. */
  int Size ()  const {return sz;};             /** Number of entries in array.            */
  int Last ()  const {return(sz - 1);};        /** Index of last valid entry in array.    */
  const int *Vals () const {return arr;};      /** Read-only pointer to array of values.  */
  int *Data () {return arr;};                  /** Pointer to raw array of values.        */
  jhcArr *SetSize (const jhcArr& ref);
  jhcArr *SetSize (int n);
  jhcArr *InitSize (int n, int val =0) {SetSize(n); Fill(val); return this;}
  jhcArr *MinSize (int n);
  int SameSize (const jhcArr& ref) const;
  int SameArr (const jhcArr& tst) const;
  void SetLimits (int start, int end);
  void SetLims (double start, double end);
  void CopyLims (const jhcArr& ref);
  void MaxLims ();

  // simple operations
  void Fill (int val =0);
  void LeftFill (int pos, int val =0);
  void RightFill (int pos, int val =0);
  void FillSpan (int start, int end, int val =0);
  void Matte (int start, int end, int val =0);
  void Copy (const jhcArr& src);
  void Copy (const int vals[], int n);
  void Push (int val);
  int Pop (int fill =0);
  void Shift (int amt, int fill =0);
  int Tail () const {return arr[sz - 1];};
  int FlipAround (const jhcArr& src, int mid);
  void Zoom (const jhcArr& src, int mid, double factor); 
  void Magnify (const jhcArr& src, int bot, double factor); 

  // low-level accesss
  int ARefChk (int n, int def =-1) const;
  int ASetChk (int n, int val);
  int AIncChk (int n, int amt);
  int AMaxChk (int n, int val);
  int AMinChk (int n, int val);
  int ARefX (int n, int def =-1) const;
  int ASetX (int n, int val);
  int AIncX (int n, int amt);
  int AMaxX (int n, int val);
  int AMinX (int n, int val);

#ifndef _DEBUG
  int ARef (int n) const    {return aref0(n);}
  int ASet (int n, int val) {aset0(n, val); return 1;}
  int AInc (int n, int amt) {ainc0(n, amt); return 1;}
  int AMax (int n, int val) {amax0(n, val); return 1;}
  int AMin (int n, int val) {amin0(n, val); return 1;}
#else
  int ARef (int n) const    {return ARefX(n);}
  int ASet (int n, int val) {return ASetX(n, val);}
  int AInc (int n, int amt) {return AIncX(n, amt);}
  int AMax (int n, int val) {return AMaxX(n, val);}
  int AMin (int n, int val) {return AMinX(n, val);}
#endif

  // statistics
  double AvgVal (int all =0) const;
  double AvgRegion (int lo, int hi) const;
  int MaxVal (int all =0) const;
  int MaxRegion (int lo, int hi) const;
  int MinVal (int all =0) const;
  int MinRegion (int lo, int hi) const;
  int MinNZ (int all =0) const;
  int MaxAbs (int all =0) const;
  int SumAll (int all =0) const;
  int SumRegion (int lo, int hi, int vmax =0) const;
  int CountOver (int val =0, int all =0) const;

  // bin finding
  int MaxBin (int bias =0) const;
  int MaxBin (int lo, int hi, int bias =0) const;
  int MaxBin (double lo, double hi, int bias =0) const;
  int MinBin (int bias =0) const;
  int MinBin (int lo, int hi, int bias =0) const;
  int MinBin (double lo, double hi, int bias =0) const;
  int MaxBinN (int n) const;
  int MinBinN (int n) const;
  int AvgBin () const;
  int SDevBins () const;
  double SDevFrac () const;
  double SDevUnder (int mid) const;
  double SDevOver (int mid) const;
  int Percentile (double frac) const;
  int MedianBin () const;
  int Centroid () const;
  double SubPeak (int pk, int cyc =0) const;

  // examining peaks
  int FirstUnder (int th, int force =0) const;
  int LastUnder (int th, int force =0) const;
  int LastUnder (int th, double lo, double hi, int force =0) const;
  int FirstOver (int th, int force =0) const;
  int LastOver (int th, int force =0) const;
  int PeakRise (int peak, double frac, int force =1) const;
  int PeakRiseLim (int peak, double frac, double lo, int force =1) const;
  int PeakFall (int peak, double frac, int force =1) const;
  int PeakFallLim (int peak, double frac, double hi, int force =1) const;
  int ValleyFall (int val, double frac, int force =1) const;
  int ValleyRise (int val, double frac, int force =1) const;
  int PeakLeft (int peak, double frac, int stop =-1, double hump =0.0, double rise =-1.0) const;
  int PeakRight (int peak, double frac, int stop =-1, double hump =0.0, double rise =-1.0) const;
  int BoundLeft (int peak, double tol) const;
  int BoundRight (int peak, double tol) const;
  int FirstSummit (int th, double tol =0.9) const;
  int LastSummit (int th, double tol =0.9) const;
  int FirstValley (int th, double tol =1.2) const;
  int LastValley (int th, double tol =1.2) const;
  int NearestPeak (int pos) const;
  int NearMassPeak (int pos, int th =0) const;
  int AdjacentPeak (int pos, double drop =0.3) const;
  int DualPeak (int pos, int rng, double dip =0.1) const;
  int TruePeak (double lo, double hi, int bias =0) const;
  int ErasePeak (int pos, double drop =0.1);
  int TrueMax (int lo, int hi, int bias =0) const;
  int CycBounds (int& lo, int& hi, int pk, double tol) const;

  // two input functions
  int SumAbsDiff (const jhcArr& ref);
  int SumSqrDiff (const jhcArr& ref);
  void CopyRegion (const jhcArr& src, int lo, int hi, int vmax =0);

  // combining arrays
  int Sum (const jhcArr& a, const jhcArr& b, double sc =1.0);
  int WtdSum (const jhcArr& a, const jhcArr& b, double asc, double bsc =1.0);
  int AddWtd (const jhcArr& a, double sc =1.0);
  int Diff (const jhcArr& a, const jhcArr& b, double sc =1.0);
  int ClipDiff (const jhcArr& a, const jhcArr& b, double sc =1.0);
  int AbsDiff (const jhcArr& a, const jhcArr& b, double sc =1.0);
  int SqrDiff (const jhcArr& a, const jhcArr& b, double sc =1.0);
  int DualDiff (const jhcArr& a, const jhcArr& b, double sc =1.0);
  int ShiftDiff (const jhcArr& a, const jhcArr& b, int rng =1);

  // statistical updates
  int Blend (const jhcArr& src, double wt);
  int Mult (const jhcArr& src);
  int MaxFcn (const jhcArr& src);
  int MinFcn (const jhcArr& src);

  // modifications
  int Scale (const jhcArr& src, double sc);
  int Scale (double sc) {return Scale(*this, sc);}
  int ScaleFast (const jhcArr& src, double sc);
  int Complement (const jhcArr& src, int top =255);
  int Offset (const jhcArr& src, int inc =1);
  int OffsetN (const jhcArr& src, int inc =1);
  int Squelch (const jhcArr& src, int sub);
  int PadNZ (const jhcArr& src, int left, int right, int val =1);
  int NormBy (const jhcArr& src, int cnt, int total =255);
  int Normalize (int total =255);
  int Smooth (const jhcArr &src, int passes =1, int cyc = 0);
  int Smooth (int passes, int cyc =0) {return Smooth(*this, passes, cyc);};  /** Smooth in-place multiple times. */
  int Boxcar (const jhcArr &src, int sc, int cyc =0);
  int BoxcarNZ (const jhcArr &src, int sc, int cyc =0);
  int Interpolate (const jhcArr& src, int bad =0);
  int BinScale (const jhcArr& src, double f);
  int Thresh (const jhcArr& src, int th, int val =1);
  void ThreshN (const jhcArr& src, int th, int val =1);
  int OverUnder (const jhcArr& src, int th, int over =1, int under =0);
  int DualClamp (const jhcArr& src, int big =1, int sm =0);
  int BitField (const jhcArr& src, int mask, int shift =0);

  // file operations
  int Read (const char *fname);
  int Write (const char *fname, int bin =0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  void dealloc_arr ();
  void init_arr ();

  int aref0 (int n) const {return arr[n];}      /** Primitive write access to array values.  */
  void aset0 (int n, int val) {arr[n] = val;}   /** Primitive read access to array values.   */
  void ainc0 (int n, int amt) {arr[n] += amt;}  /** Primitive modify access to array values. */
  void amax0 (int n, int val) {arr[n] = __max(arr[n], val);}   /** Primitive max of array values.   */
  void amin0 (int n, int val) {arr[n] = __min(arr[n], val);}   /** Primitive min of array values.   */
};


#endif




