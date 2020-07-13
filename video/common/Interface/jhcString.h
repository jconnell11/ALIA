// jhcString.h : converts between char and WCHAR
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2019 IBM Corporation
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

#ifndef _JHCSTRING_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSTRING_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <string.h>


//= Converts between char and WCHAR.
// maintains both version and provides member access to each for use in function
// if member variables used directly, call W2C or C2W afterwards for consistency

class jhcString
{
// PUBLIC MEMBER VARIABLES
public:
  int len;
  char ch[500];
  wchar_t wch[500];


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcString ();
  jhcString (const char *val);
  jhcString (const wchar_t *val);
  int Len () const {return len;}

  // main functions
  void Set (const char *val, int n =0);
  void Set (const wchar_t *val, int n =0);
  void Terminate (int n);
  const char *W2C ();
  const wchar_t *C2W ();

  // get whatever version is needed for local MFC settings
  // reflect whatever was done to this version in other
#ifndef _UNICODE
  char *Txt () {return ch;}
  void Sync () {C2W();}
#else
  wchar_t *Txt () {return wch;}
  void Sync ()    {W2C();}
#endif


};


#endif  // once




