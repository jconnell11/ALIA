// jhcAviVSrc.h : specialization of VideoSrc to AVI files
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2019 IBM Corporation
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

#ifndef _JHCAVIVSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCAVIVSRC_
/* CPPDOC_END_EXCLUDE */


// order of includes is important

#include "jhcGlobal.h"

#include <windows.h>
#include <VfW.h>

#include "Video/jhcVideoSrc.h"


//= Wrapper around standard AVI functions and datastructures.
// Note: DirectShow has trouble with older uncompressed frames

class jhcAviVSrc : public jhcVideoSrc
{
// internal member variables
private:
  PAVIFILE pfile;                           
  PAVISTREAM pavi;
  PGETFRAME pgf;
  int pack;


// basic operations
public:
  ~jhcAviVSrc ();
  jhcAviVSrc (const char *filename, int index =0);


private:
  int CloseAvi (int last =0);
  void InitAvi (int first =1);
  int SetSource (const char *filename);

  int iGet (jhcImg& dest, int *advance, int src, int block);

};


/////////////////////////////////////////////////////////////////////////////

// part of mechanism for automatically associating class with file extensions

extern int jvreg_jhcAviVSrc;


#endif
