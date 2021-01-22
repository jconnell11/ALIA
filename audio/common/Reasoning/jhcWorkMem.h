// jhcWorkMem.h : main temporary semantic network for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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
  // main vs halo separation
  int nimbus, mode;
  jhcNetNode *self;          // fixed node representing the robot
  jhcNetNode *user;          // node for current person communicating

  // mood variables
  double skep;               // global condition belief threshold (skepticism)


// PROTECTED MEMBER VARIABLES
protected:
  jhcNodePool halo;          // expectations


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcWorkMem ();
  jhcWorkMem ();
  void Horizon () {nimbus = halo.LastLabel();}
  void SetMode (int exp =2) {mode = exp;}
  int WmemSize () const {return NodeCnt();}
  int HaloSize () const {return halo.NodeCnt();}

  // belief threshold
  double MinBlf () const {return skep;}
  void InitSkep (double v) {skep = v;}

  // conversation participants
  jhcNetNode *Robot () const {return self;}
  jhcNetNode *Human () const {return user;}
  jhcNetNode *ShiftUser (const char *name =NULL);
  jhcNetNode *SetUser (jhcNetNode *n);

  // list access (overrides virtual)
  jhcNetNode *NextNode (const jhcNetNode *prev =NULL) const;
  int Length () const {return NodeCnt();}
  bool Prohibited (const jhcNetNode *n) const;

  // halo functions
  void ClearHalo () {halo.PurgeAll();}
  void AssertHalo (const jhcGraphlet& pat, jhcBindings& b, jhcAliaRule *r =NULL);
  void MaxHalo (jhcNetNode *n);

  // truth maintenance
  void Endorse (const jhcGraphlet& key, int dbg =0);

  // debugging
  void PrintMain () const 
    {jprintf("\nWMEM (%d nodes) =", Length()); Print(2); jprintf("\n");}
  void PrintHalo () const 
    {jprintf("\nHALO (%d nodes) =", HaloSize()); halo.Print(2); jprintf("\n");}


// PROTECTED MEMBER FUNCTIONS
protected:
  // conversation participants
  void InitPeople (const char *rname);

  // halo functions
  void PromoteHalo (jhcBindings& h2m, const jhcBindings& b);

  // garbage collection
  int CleanMem (int dbg =0);


// PRIVATE MEMBER FUNCTIONS
private:
  // conversation participants
  void set_prons (int tru);

  // garbage collection
  void keep_from (jhcNetNode *anchor, int dbg);
  int rem_unmarked (int dbg);


};


#endif  // once




