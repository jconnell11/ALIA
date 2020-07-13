// jhcColor.h : color processing functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
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

#ifndef _JHCCOLOR_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCCOLOR_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Ways of manipulating primarily RGB data to emphasize different parts.

class jhcColor
{
// PRIVATE MEMBER VARIABLES
private:
  static int instances;
  static UL32 *norm8;   
  static UC8 *third, *blut, *glut, *rlut;
  static UC8 *rgsc, *bysc;  
  static int *rf2, *gf2, *bf2;
  static UC8 (*ratio)[256];
  static UC8 (*invtan2)[256];


// PUBLIC MEMBER FUNCTIONS
public:
  jhcColor ();
  ~jhcColor ();

  // color transformations
  int MaxBoost (jhcImg& dest, const jhcImg& src, int th =50) const;
  int MaxColor (jhcImg& dest, const jhcImg& src, double fmax =2.0) const;
  int ScaleRGB (jhcImg& dest, const jhcImg& src, double rsc, double gsc, double bsc) const;
  int ScaleRGB_16 (jhcImg& dest, const jhcImg& red, const jhcImg& grn, const jhcImg& blu, 
                   double rsc, double gsc, double bsc) const;
  int IsoLum (jhcImg& dest, const jhcImg& src) const;
  int HexColor (jhcImg& red, jhcImg& grn, jhcImg& blu, jhcImg& yel, 
                jhcImg& wht, jhcImg& blk, const jhcImg& src, int x2 =1) const; 

  // alternate color spaces
  int Redness (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int Blueness (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int ColorDiffs (jhcImg& rg, jhcImg& yb, const jhcImg& src) const;
  int ColorDiffsRC (jhcImg& rc, jhcImg& gm, const jhcImg& src) const;
  int RCDiff (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int GMDiff (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int Sat (jhcImg& dest, const jhcImg& src, int ith =50) const;
  int VectSat (jhcImg& dest, const jhcImg& src, int ith =50) const;
  int Hue (jhcImg& dest, const jhcImg& src, int sth =25, int ith =50, int def =0) const;
  int HueMask (jhcImg& dest, jhcImg& gate, const jhcImg& src, int sth =25, int ith =50) const;
  int MaskHSI (jhcImg& hue, jhcImg& sat, jhcImg& brite, const jhcImg& src, const jhcImg& mask) const;
  int RGBtoHSI (jhcImg& hue, jhcImg& sat, jhcImg& brite, const jhcImg& src) const;
  int RGBtoDiff (jhcImg& cdiff, const jhcImg& rgb) const;

  // region selection
  int SelectHSI (jhcImg& dest, const jhcImg& src, 
                 int hlo, int hhi, int slo, int shi, int ilo, int ihi) const;
  int SelectHSI (jhcImg& dest, 
                 const jhcImg& hue, const jhcImg& sat, const jhcImg& brite, const jhcImg& mask,
                 int hlo, int hhi, int slo, int shi, int ilo, int ihi) const;
  int SelectHCI (jhcImg& dest, const jhcImg& src, 
                 int hlo, int hhi, int clo, int chi, int ilo, int ihi) const;
  int ExactRGB (jhcImg& dest, const jhcImg& src, int r, int g, int b) const;
  int SoftHSI (jhcImg& dest, const jhcImg& src,                  
               int hlo, int hhi, int slo, int shi, int ilo, int ihi,
               int hdrop =10, int sdrop =20, int idrop =20) const;
  int SkinTone (jhcImg& dest, const jhcImg& src) const;
  int SkinTone2 (jhcImg& dest, const jhcImg& src) const;

  // assembly and disassembly
  int CopyMono (jhcImg& dest, const jhcImg& src) const;
  int SplitRGB (jhcImg& r, jhcImg& g, jhcImg& b, const jhcImg& src) const;
  int MergeRGB (jhcImg& dest, const jhcImg& r, const jhcImg& g, const jhcImg& b) const;
  int GetRed (jhcImg& dest, const jhcImg& src) const {return dest.CopyField(src, 2, 0);};  /** Extract red field.   */
  int GetGrn (jhcImg& dest, const jhcImg& src) const {return dest.CopyField(src, 1, 0);};  /** Extract green field. */
  int GetBlu (jhcImg& dest, const jhcImg& src) const {return dest.CopyField(src, 0, 0);};  /** Extract blue field.  */
  int PutRed (jhcImg& dest, const jhcImg& src) const {return dest.CopyField(src, 0, 2);};  /** Insert red field.    */
  int PutGrn (jhcImg& dest, const jhcImg& src) const {return dest.CopyField(src, 0, 1);};  /** Insert green field.  */
  int PutBlu (jhcImg& dest, const jhcImg& src) const {return dest.CopyField(src, 0, 0);};  /** Insert blue field.   */

  // raw camera color
  int DeBayer (jhcImg& dest, const jhcImg& src) const;
  int DeBayer_GB (jhcImg& dest, const jhcImg& src) const;
  int DeBayer16 (jhcImg& dest, const jhcImg& src, int off =0, int sh =0) const;
  int DeBayer16_GB (jhcImg& dest, const jhcImg& src, int off =0, int sh =0) const;
  int DeBayer16 (jhcImg& red, jhcImg& grn, jhcImg& blu, const jhcImg& src) const;
  int DeBayer16_GB (jhcImg& red, jhcImg& grn, jhcImg& blu, const jhcImg& src) const;
  int DeBayer16_GR (jhcImg& red, jhcImg& grn, jhcImg& blu, const jhcImg& src) const;

  // debugging functions
  void HueMap (jhcImg& map) const;
  void OppMap (jhcImg& map) const;
  void GexMap (jhcImg& map) const;


};


#endif




