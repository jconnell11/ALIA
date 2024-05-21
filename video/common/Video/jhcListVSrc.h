// jhcListVSrc.h : read a bunch of files to simulate a video stream
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
// Copyright 2023 Etaoin Systems
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

#ifndef _JHCLISTVSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCLISTVSRC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImgIO.h"
#include "Video/jhcVideoSrc.h"


//= Read a bunch of files to simulate a video stream.
// Takes a single file name and repeats, or a pattern such as "*.ras" .
// Can also take an explicit text file (extension ".txt") of the form:
// 
//    directory-name\*.extension
//    base-file-name1
//    base-file-name2
//    base-file-name3
// 
// here directory spec is relative to location of overall text file.
// if first file name is not "*" then no defaults are assumed

class jhcListVSrc : public jhcVideoSrc
{
protected:
  int def;
  float frt;
  char entry[500], list_name[500];
  jhcImgIO jio;
  FILE *list;
  jhcName iname;
  long backup;

public:
  ~jhcListVSrc ();
  jhcListVSrc (const char *name, int index =0);
  const char *FrameName (int idx_wid =-1, int full =0); 
  int FrameMatch (const char *tag);

protected:
  int repeat_img ();
  void kinect_geom ();
  void make_list ();
  void shift_dir (FILE *out, char *path);
  int read_list ();
  int reset_list ();
  int next_line ();

  int iSeek (int number);
  int iGet (jhcImg &dest, int *advance, int src, int block); 
  int iDual (jhcImg& dest, jhcImg& dest2);

};


/////////////////////////////////////////////////////////////////////////////

// part of mechanism for automatically associating class with file extensions

extern int jvreg_jhcListVSrc;


#endif




