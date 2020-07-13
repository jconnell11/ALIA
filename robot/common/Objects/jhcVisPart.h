// jhcVisPart.h : data about a portion of a visual object
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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

#ifndef _JHCVISPART_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCVISPART_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"
#include "Data/jhcImg.h"
#include "Processing/jhcTools.h"


//= Encapsulates data about a portion of a visual object.

class jhcVisPart : private jhcTools
{
// PRIVATE MEMBER VARIABLES
private:
  char main[80], mix[80];
  jhcVisPart *next;            // next item in list of parts


// PUBLIC MEMBER VARIABLES
public:
  // identification and selection status
  char name[40];
  int status;

  // numerical properties
  jhcArr hhist;
  int cols[9], cvect[9];
  int area, area2;
  double cx, cy;

  // iconic properties
  jhcImg crop;
  jhcImg mask, shrink, hue, hmsk, wht, blk;
  int rx, ry, rw, rh;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcVisPart ();
  ~jhcVisPart ();
  void IconSize (int x, int y);
  void Copy (jhcVisPart& src);
 
  // list traversal
  jhcVisPart *NextPart () {return next;}
  jhcVisPart *AddPart ()  {if (next == NULL) next = new jhcVisPart; return next;}

  // part characteristics
  const char *Color (int n);
  const char *AltColor (int n);

  // part analysis
  int Analyze (const int *clim);


// PRIVATE MEMBER FUNCTIONS
private:
  void color_bins (const jhcImg& src, const jhcImg& gate, const int *clim);
  void qual_col ();

};


#endif  // once




