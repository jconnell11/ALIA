// jhcGroup.h : pixel aggregation functions
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

#ifndef _JHCGROUP_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGROUP_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcArr.h"
#include "Data/jhcImg.h"


//= Pixel aggregation functions.
// NOTE: keeps private internal state so copies must be made for OpenMP

class jhcGroup
{
// PROTECTED MEMBER VARIABLES
protected:
  jhcArr areas;
  jhcImg tmp;


// PUBLIC MEMBER FUNCTIONS
public:
  // basic CC
  int CComps4 (jhcImg& dest, const jhcImg& src, int amin =0, int th =0, int label0 =0);
  int CComps8 (jhcImg& dest, const jhcImg& src, int amin =0, int th =0, int label0 =0);

  // CC variants
  int GComps4 (jhcImg& dest, const jhcImg& src, int amin =0, int diff =0, int bg =0);
  int GComps16 (jhcImg& dest, const jhcImg& src, int amin =0, int diff =0, int bg =0);
  int AComps4 (jhcImg& dest, const jhcImg& src, int amin =0, int diff =0, int bg =0);
  int AComps8 (jhcImg& dest, const jhcImg& src, int amin =0, int diff =0, int bg =0);
  int SiamCC (jhcImg& dest, const jhcImg& src, double arel =0.3, int amin =20, int th =0);

  // shape cleanup
  int EraseBlips (jhcImg& dest, const jhcImg& src, int amin =100, 
                  int th =0, jhcImg *vic =NULL);
  int Biggest (jhcImg& dest, const jhcImg& src, int th =0);
  int Tagged (jhcImg& dest, const jhcImg& src, int x, int y, int th =0);
  int RemSmall (jhcImg& dest, const jhcImg& src, double arel =0.5, int amin =0,
                 int th =0, jhcImg *vic =NULL);
  int RemSmallGray (jhcImg& dest, const jhcImg& src, double arel =0.5, int amin =0,
                    int diff =0, int bg =0, jhcImg *vic =NULL);
  int FillHoles (jhcImg& dest, const jhcImg& src, int hmax =100, 
                 int th =0, jhcImg *vic =NULL);
  int CleanUp (jhcImg& dest, const jhcImg& src, int th =128, int amin =20, 
               double arel =0.5, int hmax =100, jhcImg *vic =NULL);
  int PrunePatch (jhcImg& dest, const jhcImg& src, int th =128, int amin =20, 
                  double arel =0.5, double hrel =0.1, jhcImg *vic =NULL);

  // debugging graphics
  int MarkComp (jhcImg& dest, const jhcImg& marks, int n, int clr =1) const;
  int DrawBorder (jhcImg& dest, const jhcImg& marks, int n, int r =255, int g =255, int b =255) const;


// PROTECTED MEMBER FUNCTIONS
protected:
  // basic CC
  int scan_labels (jhcImg& dest, const jhcImg& src, int th);
  int scan_labels8 (jhcImg& dest, const jhcImg& src, int th);
  int merge_labels (int now, int old);
  int norm_labels (jhcImg& dest, int n, int amin, int label0 =0);

  // CC variants
  int scan_diff (jhcImg& dest, const jhcImg& src, int diff, int bg);
  int scan_diff16 (jhcImg& dest, const jhcImg& src, int diff, int bg);
  int scan_diff_a (jhcImg& dest, const jhcImg& src, int diff, int bg);
  int scan_diff_a8 (jhcImg& dest, const jhcImg& src, int diff, int bg);
  int scan_top (jhcImg& dest, const jhcImg& src, double arel, int amin, int th);
  int merge_horiz (int now, int old, double arel, int amin);

  // shape cleanup
  int keep_labels (jhcImg& dest, const jhcImg& marks, int n, int px, int py);
  int thresh_labels (jhcImg& dest, const jhcImg& marks, int n, 
                     int amin, double arel, int inv);
  int scan_dual (jhcImg& dest, const jhcImg& src, int th);
  int thresh_dual (jhcImg& dest, const jhcImg& marks, int n, 
                   int amin, double arel, int hmax, double hrel);

  // debugging graphics
  bool chk_around (const US16 *s, int sln2, int n) const;


};


#endif




