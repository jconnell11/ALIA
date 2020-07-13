// jhcFileList.h : get file name strings one line at a time from text file
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013 IBM Corporation
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

#ifndef _JHCFILELIST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFILELIST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>

#include "Data/jhcName.h"


//= Get file name strings one line at a time from text file.
// takes an explicit text file of the form:
// <pre>
//
//    directory-name\*.extension
//    base-file-name1
//    base-file-name2
//    base-file-name3
//
// </pre> 
// here directory spec is relative to location of overall text file.
// if first file name is not "*" then >NO DEFAULTS< are assumed (intentionally)
// can also be used for normal text string lists (without * function)
// adapted from original jhcListVSrc code

class jhcFileList
{
// PRIVATE MEMBER VARIABLES
private:
  jhcName base;
  char time[20];
  FILE *in;
  long start;
  UL32 t0, tnow;
  int def, total, now;


// PUBLIC MEMBER VARIABLES
public:
  jhcName entry;              /** Last name read from list. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcFileList (const char *fname =NULL);
  ~jhcFileList ();
  int MaxBase ();
  int MaxName (); 
  const char *PadName (char *dest, const char *src, int n, int peel =0, int dsz =80) const;

  // main functions
  int ListOpen (const char *fname);
  bool ListClose ();
  int ListRewind ();
  const char *ListNext ();

  // status
  int ListNow () const {return now;}
  int ListCnt () const {return total;}
  double ListElapsed () const {return((tnow - t0) / 1000.0);}
  double ListProgress () const; 
  double ListRemaining () const;
  const char *ListTime ();

  // debugging
  int ListDups (const char *report);
  int RemDups (const char *clean, int all =0);
  int ListMiss (const char *report);


// PROTECTED MEMBER FUNCTIONS
protected:
  virtual int read_hdr () {return 1;}


};


#endif  // once




