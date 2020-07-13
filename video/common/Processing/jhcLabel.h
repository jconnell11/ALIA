// jhcLabel.h : uses operating system to add captions to an image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2018 IBM Corporation
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

#ifndef _JHCLABEL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCLABEL_
/* CPPDOC_END_EXCLUDE */


#include "jhcGlobal.h"

#include <windows.h> 
#include <stdio.h>

#include "Data/jhcImg.h"


//= Uses operating system to add captions to an image.
// conceptually part of jhcDraw but has Windows dependency

class jhcLabel 
{
// PRIVATE MEMBER VARIABLES
private:
  HDC dc;
  HGDIOBJ bmap0, font0;
  HBITMAP bmap;
  HFONT font;
  LPBITMAPINFOHEADER hdr;
  jhcImg src;
  int sz, tw, th;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcLabel (int xmax =640, int ymax =480);
  ~jhcLabel ();

  // main functions
  int LabelLeft (jhcImg& dest, double x, double y, const char *msg, 
                 int ht =16, int r =255, int g =255, int b =255, int chk =0);
  int LabelRight (jhcImg& dest, double x, double y, const char *msg,
                  int ht =16, int r =255, int g =255, int b =255, int chk =0);
  int LabelOver (jhcImg& dest, double x, double y, const char *msg, 
                 int ht =16, int r =255, int g =255, int b =255, int chk =0);
  int LabelUnder (jhcImg& dest, double x, double y, const char *msg, 
                  int ht =16, int r =255, int g =255, int b =255, int chk =0);
  int LabelCenter (jhcImg& dest, double x, double y, const char *msg,
                   int ht =16, int r =255, int g =255, int b =255, int chk =0);
  int LabelRotate (jhcImg& dest, double x, double y, double degs, const char *msg,
                   int ht =16, int r =255, int g =255, int b =255);
  int LabelBox (jhcImg& dest, const jhcRoi& box, const char *msg, 
                int ht =16, int r =255, int g =255, int b =255, int gap =5);
  int LabelBox (jhcImg& dest, const jhcRoi& box, int n,
                int ht =16, int r =255, int g =255, int b =255, int gap =5)
    {char msg[80]; sprintf_s(msg, "%d", n); return LabelBox(dest, box, msg, ht, r, g, b, gap);}


// PRIVATE MEMBER FUNCTIONS
private:
  void make_header ();
  void set_font (int ht);
  void make_label (const char *txt, int ht, int just);
  int xfer_text (jhcImg& dest, int x, int y, int r, int g, int b, int chk);
  int xfer_text_rot (jhcImg& dest, double x, double y, double degs, int r, int g, int b);
  void cvt_col (int& red, int& grn, int& blu, const jhcImg& dest, int r, int g, int b) const;


};


#endif  // once




