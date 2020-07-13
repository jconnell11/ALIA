// jhcPatchProps.h : extracts semantic properties for image regions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _JHCPATCHPROPS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCPATCHPROPS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Data/jhcParam.h"
#include "Processing/jhcALU.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcColor.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcGray.h"
#include "Processing/jhcGroup.h"
#include "Processing/jhcHist.h"
#include "Processing/jhcEdge.h"
#include "Processing/jhcStats.h"
#include "Processing/jhcThresh.h"
#include "Processing/jhcVect.h"


//= Extracts semantic properties for image regions.

class jhcPatchProps : private jhcALU,   private jhcArea, private jhcColor, private jhcDraw,   private jhcEdge, private jhcGray, 
                      private jhcGroup, private jhcHist, private jhcStats, private jhcThresh, private jhcVect
{
friend class CMensEtDoc;  // for debugging

// PRIVATE MEMBER VARIABLES
private:
  static const int cmax = 9;    /** Maximum number of colors. */

  // interior of patch
  jhcImg shrink;
 
  // color properties
  jhcImg clip, hmsk, hue, wht, blk;
  jhcArr hhist;
  int cols[cmax], cvect[cmax];
  char cname[cmax][20];
  char main[80], mix[80];

  // texture properties
  jhcImg thumb, ej, ejv, hcc, vcc;
  double ftex;
  int nh, nv;


// PUBLIC MEMBER VARIABLES
public:
  // color extraction parameters
  jhcParam cps;
  int csm, cth, smin, imin, imax, white, dark;

  // qualitative color parameters
  jhcParam hps;
  int clim[6];

  // primary/secondary color parameters
  jhcParam nps;
  double cprime, cdom, csec, cmin;

  // stripedness parameters
  jhcParam sps;
  double tfill;
  int ejth, elen, nej;

  // size and width parameters
  jhcParam zps;
  double big, sm, wth, nth;

  // results of size and width
  double dim, wrel;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcPatchProps ();
  jhcPatchProps ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // configuration
  void SetSize (const jhcImg& ref) {SetSize(ref.XDim(), ref.YDim());}
  void SetSize (int x, int y);

  // color functions
  int FindColors (const jhcImg& mask, const jhcImg& src);
  const char *ColorN (int n) const;
  const char *AltColorN (int n) const;
  const char *KnownColor (int n) const;
  int QuantColor (jhcArr& dest) const;
  int MainColors (char *dest, int ssz) const;
  int AltColors (char *dest, int ssz) const;

  // color functions - convenience
  template <size_t ssz>
    int MainColors (char (&dest)[ssz]) const
      {return MainColors(dest, ssz);}
  template <size_t ssz>
    int AltColors (char (&dest)[ssz]) const
      {return AltColors(dest, ssz);}

  // texture functions
  int Striped (const jhcImg& mask, const jhcImg& mono);

  // size and shape functions
  int SizeClass (int area, double ppi);
  int WidthClass (double wx, double hy);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int cfind_params (const char *fname);
  int hue_params (const char *fname);
  int cname_params (const char *fname);
  int stripe_params (const char *fname);
  int size_params (const char *fname);

  // color functions
  void color_bins (const jhcImg& src, const jhcImg& gate);
  void qual_col ();

};


#endif  // once




