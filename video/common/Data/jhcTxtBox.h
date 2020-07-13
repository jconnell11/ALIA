// jhcTxtBox.h : associates text string with a region of an image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2014 IBM Corporation
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

#ifndef _JHCTXTBOX_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTXTBOX_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcRoi.h"


//= Associates text string with a region of an image.
// contains useful post-processing for various OCR front ends

class jhcTxtBox 
{
// PRIVATE MEMBER VARIABLES
private:
  int total;           /** Maximum number of fragments in list. */
  int valid;           /** Size of list up to last valid box.   */
  char **text;         /** Text string associated with region.  */
  jhcRoi *area;        /** Size and location of text strings.   */
  int *mark;           /** Whether text is part of valid set.   */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTxtBox ();
  ~jhcTxtBox ();
  void SetSize (int n);
  void SetSize (const jhcTxtBox& ref) {SetSize(ref.total);}
  void ClearAll ();
  int ClearItem (int i);
  void CopyAll (const jhcTxtBox& src);

  // building list
  int AddItem (const char *txt, int x, int y, int w, int h, int chk =1);
  int SetItem (int i, const char *txt, int x, int y, int w, int h, int chk =1);
 
  // results
  int Size () const {return total;}    /** Maximum number of boxes allowed in list. */
  int Active () const {return valid;}  /** Current number of valid boxes in list.   */
  int ValidN (int n) const         {return  mark[__max(0, __min(n, total - 1))];}
  const char *TxtN (int n) const   {return  text[__max(0, __min(n, total - 1))];}
  const jhcRoi *BoxN (int n) const {return &area[__max(0, __min(n, total - 1))];}

  // restrictions
  int CountOver (int mth =1) const;
  int BoxHt (int hi =50, int lo =1);
  int MatchOnly (const char *choices, int alt =1);
  int AlphaOnly (const char *extra, int cap =0);
  int NumOnly (const char *extra);
  int AlnumOnly (const char *extra, int cap =0);
  int LengthOnly (int lo =3, int hi =0, int nopunc =0);
  void MarkN (int i, int val =0);
  int MarkAll (int val =1);
  bool Selected (int n) const;

  // box merging
  int MergeH (double gap =4.0, double shift =0.5, double sc =3.5);
  int MergeV (double gap =1.0, double overlap =0.8, double sc =2.0);

  // splitting
  void SplitSingle (double frac =0.7, int nchar =2);


// PRIVATE MEMBER FUNCTIONS
private:
  void dealloc ();

  int norm_label (char *txt, const char *src);
  void pure_ascii (char *dest, const char *src) const;
  void cvt_html (char *dest, const char *src) const;


};


#endif  // once




