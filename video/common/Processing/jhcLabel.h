// jhcLabel.h : add text captions to an image
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2018 IBM Corporation
// Copyright 2024-2026 Etaoin Systems
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

#pragma once

#include "jhcGlobal.h"

#include <stdio.h>

#include "Data/jhcImg.h"


//= Add text captions to an image.
// conceptually part of jhcDraw but has font dependency

class jhcLabel 
{
// PRIVATE MEMBER VARIABLES
private:
  // font information (shared by all instances)
  static int cw[96], ch[96];
  static UC8 *pels[96];
  static int font_users, full, line;

  // full label image
  jhcImg lab;
  int tw, th;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcLabel ();
  ~jhcLabel ();
  void SetDir (const char *path);

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
  int LabelSolid (jhcImg& dest, double x, double y, const char *msg,
                  int wht =0, int r =255, int g =255, int b =255);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and configuration
  int load_font (const char *fname);

  // main functions
  void blank_panel (jhcImg& dest, double x, double y, int wht, int pad) const;

  // label generation
  int make_label (const char *txt, int mag, int just);
  int set_size (const char *txt, int mag);
  const char *line_len (int& w, const char *txt) const;
  int insert_pels (int x0, int y0, int i, int mag);

  // pixel setting
  int xfer_text (jhcImg& dest, int x, int y, int r, int g, int b, int chk);
  int xfer_text_rot (jhcImg& dest, double x, double y, double degs, int r, int g, int b);
  void cvt_col (int& red, int& grn, int& blu, const jhcImg& dest, int r, int g, int b) const;

};

