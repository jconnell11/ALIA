// jhcNetRef.h : holds a fragment of network to be looked up in main memory
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#include "Action/jhcAliaChain.h"         // common robot

#include "Reasoning/jhcWorkMem.h"        // common audio
#include "Semantic/jhcSituation.h"


//= Holds a fragment of network to be looked up in main memory.

class jhcNetRef : public jhcSituation
{
// PRIVATE MEMBER VARIABLES
private:
  jhcNodePool *univ;
  const jhcNetNode *focus;
  jhcGraphlet *partial;
  jhcBindings win;
  int recent;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcNetRef ();
  jhcNetRef (jhcNodePool *u =NULL, double bmin =0.5);
  void RefMode (int mode) {refmode = mode;}

  // language interpretation
  jhcNetNode *FindMake (jhcNodePool& add, int find =0, jhcNetNode *f0 =NULL, 
                        double blf =-1.0, jhcAliaChain **skolem =NULL);
  jhcNetNode *LookUp (const jhcNetNode *old) const {return win.LookUp(old);}

  // language generation
  jhcNetNode *Main () const {return cond.Main();}
  int NumMatch (const jhcNodeList *wmem, double mth, int retract =0);
  jhcNetNode *BestMate () const {return win.LookUp(cond.Main());}

  // debugging
  void Print (int lvl =0) const 
    {jprintf("%*sNetRef =", lvl, ""); cond.Print(lvl + 2); jprintf("\n");}


// PRIVATE MEMBER FUNCTIONS
private:
  // language interpretation
  jhcNetNode *append_find (int n0, double blf, jhcAliaChain **skolem, int assume);

  // virtual override
  int match_found (jhcBindings *m, int& mc);


};


#endif  // once




