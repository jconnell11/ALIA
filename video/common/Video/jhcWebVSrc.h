// jhcWebVSrc.h : repeatedly reads a still image from a website
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2010-2016 IBM Corporation
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

#ifndef _JHCWEBVSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCWEBVSRC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <afxinet.h>                    // needed for internet functions

#include "Data/jhcImg.h"
#include "Data/jhcImgIO.h"
#include "Video/jhcVideoSrc.h"


//= Repeatedly reads a still image from a website.

class jhcWebVSrc : public jhcVideoSrc
{
// PRIVATE MEMBER VARIABLES
private:
  CInternetSession *s;
  jhcImgIO jio;
  char tmp[200];


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  ~jhcWebVSrc ();
  jhcWebVSrc (const char *name, int index =0);


// required functions for jhcVideoSrc base class
private:
  int iGet (jhcImg& dest, int *advance, int src);

private:
  int dealloc ();
  int copy_img (const char *dest);

};


#endif  // once




