// jhcNetRef.h : holds a fragment of network to be looked up in main memory
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#ifndef _JHCNETREF_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNETREF_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Reasoning/jhcWorkMem.h"        // common audio
#include "Semantic/jhcSituation.h"


//= Holds a fragment of network to be looked up in main memory.

class jhcNetRef : public jhcSituation
{
// PRIVATE MEMBER VARIABLES
private:
  const jhcNetNode *focus;
  jhcBindings win;
  int recent;


// PUBLIC MEMBER VARIABLES
public:
  jhcNetNode *LookUp (const jhcNetNode *old) const {return win.LookUp(old);}


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcNetRef ();
  jhcNetRef ();
 
  // main functions
  jhcNetNode *FindMake (jhcNodePool& add, int find =0, jhcNetNode *f0 =NULL);

  // main functions (virtual override)
  int match_found (jhcBindings *m, int& mc);


// PRIVATE MEMBER FUNCTIONS
private:


};


#endif  // once




