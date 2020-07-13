// jhcBarcode.h : reads barcodes centered in image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2007-2016 IBM Corporation
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

#ifndef _JHCBARCODE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBARCODE_
/* CPPDOC_END_EXCLUDE */


//= Reads barcodes centered in image.
// there several bit width estimation modes:
//   est = 0 sets bit width based on number of edges in bar region
//   est = 1 adjust bit width based on minimum bar or gap size also
//   est = 2 does local estimate of bit width for each character (pod > 0)
// there are several interpretation modes:
//   pod = 0 creates a whole bit vector straight from the bar edges found
//   pod = 1 aligns decoding to pairs of bars to combat frame drift
//   pod = 2 decodes digits as a lattice of preferred possibilities
// system can also automatically refine barcode position and edge threshold

class jhcBarcode
{
friend class CUPCDoc;  // for debugging

// PRIVATE MEMBER VARIABLES
private:
  int w, h, f, ln, resid;
  int ejs[100], bits[200], lattice[12][4];
  int ymult[256], vmult[3][256], umult[3][256];
  int *proj;


// PUBLIC MEMBER VARIABLES
public:
  // intermediate processing state variables
  int scans, sdir, soff, sfld, sx0, sy0, sdx, sdy;
  int slen, i0, i1, ecnt, bcnt, bw16, lo16, hi16, eth;

  // control parameters
  int steps, off, cols, dirs, mode;
  int sm, dmax, wmin, bmin, dbar, bdiff;
  int badj, eadj, est, pod, cvt;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcBarcode ();
  ~jhcBarcode ();
  void SetSize (int x, int y, int f);
  void FancyVersion ();
  void VideoVersion ();

  // main functions
  int GetCode (char *code, const unsigned char *src, int pat =0);
  int FastCode (char *code, const unsigned char *src, int yv12 =0);
  int HorizCode (char *code, const unsigned char *src, int yv12 =0);
  int SlowCode (char *code, const unsigned char *src, int yv12 =0);

  // low level details and operation
  int SliceCode (char *code, const unsigned char *src, 
                 int dir, int cdist, int fld, int yv12 =0);
  void LastSlice (int& x0, int& y0, int& x1, int& y1);

// PRIVATE MEMBER FUNCTIONS
private:
  void build_tables ();
  void dealloc ();

  // normal image slicing
  int slice_h (int *slice, const unsigned char *src, int cdist, int fld);
  int slice_v (int *slice, const unsigned char *src, int cdist, int fld);
  int slice_d1 (int *slice, const unsigned char *src, int cdist, int fld);
  int slice_d2 (int *slice, const unsigned char *src, int cdist, int fld);
  void smooth (int *slice, int len, int n);

  // special YV12 image slicing
  int slice_h_yuv (int *slice, const unsigned char *src, int cdist, int fld);
  int slice_v_yuv (int *slice, const unsigned char *src, int cdist, int fld);
  int slice_d1_yuv (int *slice, const unsigned char *src, int cdist, int fld);
  int slice_d2_yuv (int *slice, const unsigned char *src, int cdist, int fld);
  void YV12toRGB (unsigned char *bgr, const unsigned char *yuv);

  // edge finding within slice
  int get_edges (int *marks, int& x0, int& x1, const int *slice, int len);
  int find_limits (int& x0, int& x1, const int *slice, int len);
  int bit_width (const int *marks, int n);
  int mark_edges (int *marks, const int *slice, int x0, int x1, int jump, int flat);
  int trim_width (int& x0, int& x1, const int *marks, int n, int w0);
  int search_edges (int *marks, int n, const int *slice, int x0, int x1);
  int bit_vect (int *vals, const int *marks, int n, int wid);

  // parsing UPC-A codes
  int get_code (char *code, const int *vals, int n);
  int parse_a (char *code, const int *vals, int n, int dir);
  int lattice_a (char *code, const int *marks, int n, int dir);
  int best_path_a (char *code, int all[][4]);
  int correct_a (char *code, int *pat);
  int valid_digit (int seq, int ecode);

  // parsing UPC-E codes
  int parse_e (char *code, const int *vals, int n, int dir);
  int lattice_e (char *code, const int *marks, int n, int dir);
  int best_path_e (char *code, int all[][4]);
  int correct_e (char *code, int *pat);
  int e_check (int ppat);
  int a_check (int *e_code);
  void a_from_e (char *a_code, const char *e_code);

  // interpreting small sets of edges or bits
  const int *find_start (const int *vals, int n, int dir);
  const int *bit_start (const int *vals, int n, int dir);
  const int *edge_start (const int *marks, int n, int dir, int bw, int lo, int hi);
  const int *square_wave (const int *marks, const int *last, int n, int bw, int lo, int hi);
  const int *get_pattern (int& seq, int n, const int *vals, const int *last, int k);
  const int *group_bits (int& seq, int n, const int *vals, const int *last);
  const int *group_edges (int& seq, int n, const int *vals, const int *last, 
                          int k, int bw, int lo, int hi);
  const int *digit_prefs (int *prefs, int np, const int *marks, const int *last, int ecode);
  int perturb_pat (const int *marks, int dir, int w16, int chg);

};


#endif  // once




