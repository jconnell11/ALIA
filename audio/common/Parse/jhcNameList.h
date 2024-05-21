// jhcNameList.h : simple expansion of nickname to full name
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#ifndef _JHCNAMELIST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNAMELIST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Simple expansion of nickname to full name.
// for a more sophisticated version see jhcNamePool

class jhcNameList
{
// PRIVATE MEMBER VARIABLES
private:
 static const int pmax = 100;            /** Maximum number of people. */
 char full[pmax][80], first[pmax][80];    
 int nn;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcNameList ();
  jhcNameList ();
  int Names () const {return nn;}
 
  // main functions
  int Load (const char *fname, int append =0);
  const char *Full (int i) const 
    {return(((i < 0) || (i >= nn)) ? NULL : full[i]);}
  const char *First (int i) const
    {return(((i < 0) || (i >= nn)) ? NULL : first[i]);}
  const char *Canonical (const char *name) const;
  const char *LongName (const char *given) const;


};


#endif  // once




