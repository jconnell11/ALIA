// jhcWorkMem.h : main temporary semantic network for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#ifndef _JHCWORKMEM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCWORKMEM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Semantic/jhcBindings.h"      // common audio
#include "Semantic/jhcGraphlet.h"
#include "Semantic/jhcNetNode.h"
#include "Semantic/jhcNodePool.h"


//= Main temporary semantic network for ALIA system.
// lower 2 levels of memory: main > halo (attention on top)
// this holds all the facts directly linked to attention items
// main forms a basis for running halo rules to generate deductions
// main memory plus halo are used to match operator conditions

class jhcWorkMem : public jhcNodePool
{
// PRIVATE MEMBER VARIABLES
private:
  jhcNodePool halo;
  int src, mode;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcWorkMem ();
  jhcWorkMem ();
  void SetMode (int exp =1) {mode = exp;}
  int HaloMark () const {return src;} 

  // list access (overrides virtual)
  jhcNetNode *NextNode (const jhcNetNode *prev =NULL) const;
  int Length () const {return NodeCnt();}

  // halo functions
  void ClearHalo () {halo.PurgeAll(); src = 0;}
  int NumHalo () const {return src;}
  int AssertHalo (const jhcGraphlet& pat, jhcBindings& b, double conf, int tval =0);
  void PromoteHalo (jhcBindings& h2m, int s);
  void PrintHalo (int s =0, int lvl =0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // halo functions
  const jhcNetNode *args_bound (const jhcGraphlet& pat, const jhcBindings& b) const;
  jhcNetNode *main_equiv (const jhcNetNode *pn, const jhcBindings& b, double conf) const;
  void actualize_halo (int src) const;


};


#endif  // once




