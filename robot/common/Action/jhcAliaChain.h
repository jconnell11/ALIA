// jhcAliaChain.h : sequence backbone for activities in an FSM chain
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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

#ifndef _JHCALIACHAIN_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIACHAIN_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <math.h>

#include "Parse/jhcTxtLine.h"          // common audio
#include "Semantic/jhcBindings.h"
#include "Semantic/jhcNodePool.h"


//= Sequence backbone for activities in an FSM chain.
// step is a variant which can be either jhcAliaDir or jhcAliaPlay
// basic operations use type-based dispatch (non-NULL member)
// better than using shared base class when copying whole graph
// NOTE: delete automatically cleans up rest of chain (or graph)
// this is the basic structure describing an FSM graph
           
class jhcAliaChain
{
// PRIVATE MEMBER VARIABLES
private:
  // calling environment
  class jhcAliaCore *core;
  int level;

  // variables from earlier FINDs
  jhcBindings scoping;
  jhcAliaChain *backstop;
  UL32 mt0;
  int spew, parse;

  // payload is one of two types
  class jhcAliaDir *d;
  class jhcAliaPlay *p;

  // deletion and serialization flags
  int cut, idx; 

  // linking via numbered steps
  int fnum, cnum, anum;

  // run status on last few cycles
  int prev, done;

  // last goal counting request
  int req;


// PUBLIC MEMBER VARIABLES
public:
  jhcAliaChain *fail, *cont, *alt;     // next step in chain (public for jhcGraphizer)
  class jhcAliaOp *avoid;
  int alt_fail, inum;                  // looping status


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaChain ();
  jhcAliaChain ();
  int Verdict () const  {return done;}
  int Level () const    {return level;}
  int LastReq () const  {return req;}  
  UL32 Time0 () const   {return mt0;}
  jhcBindings *Scope () {return &scoping;}
  class jhcAliaCore *Core () {return core;}
  class jhcActionTree *ATree ();
  bool Variations () const {return((cont == NULL) && (spew >= 2) && (backstop != NULL));} 
  bool Fallback () const;

  // configuration
  jhcAliaChain *BindDir (class jhcAliaDir *dir)
    {if ((p == NULL) && (d == NULL)) d = dir; return this;}
  jhcAliaChain *BindPlay (class jhcAliaPlay *play)
    {if ((p == NULL) && (d == NULL)) p = play; return this;}
  class jhcAliaDir *GetDir () const {return d;}
  class jhcAliaPlay *GetPlay () const {return p;}
  bool Empty () const 
    {return((d == NULL) && (p == NULL));}
  bool StepDir (int kind) const;
  void RefSteps (jhcNetNode *src, const char *slot, jhcNodePool& pool, int init =1);
  jhcAliaChain *StepN (int n);
  jhcAliaChain *Penult ();
  jhcAliaChain *Last ();
  jhcGraphlet *LastKey ();
  jhcAliaChain *Append (jhcAliaChain *tackon);
  int MaxDepth (int cyc =1);
  int NumGoals (int leaf =0, int cyc =1);
  void Enumerate ();

  // building
  jhcAliaChain *Instantiate (jhcNodePool& mem, jhcBindings& b, const jhcGraphlet *ctx =NULL);
  bool Involves (const jhcNetNode *item, int top =1);
  void MarkSeeds (int head =1);

  // main functions
  int Start (class jhcAliaCore *all, int lvl);
  int Start (jhcAliaChain *last);
  int Status ();
  void Stop ();
  int FindActive (const jhcGraphlet& desc, int halt);

  // file functions
  int Load (jhcNodePool& pool, jhcTxtLine& in, int play =0); 
  int Save (FILE *out, int lvl =0, int *step =NULL, int detail =1);
  int Print (int lvl =0, int detail =1)
    {return Save(stdout, lvl, NULL, detail);}
  int PrintStep (int lvl =0)
    {return Save(stdout, -(lvl + 1), NULL, 1);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void cut_loops ();

  // building
  jhcAliaChain *dup_self (int& node, jhcAliaChain *seen[], jhcNodePool& mem, 
                          jhcBindings& b, const jhcGraphlet *ctx);
  void clr_labels (int head);

  // main functions
  int start_payload (int lvl);

  // file reading
  int build_chain (jhcNodePool& pool, jhcAliaChain *label[], jhcAliaChain *fix[], int& n, jhcTxtLine& in);
  int get_payload (jhcNodePool& pool, jhcAliaChain *label[], jhcTxtLine& in);
  int link_graph (jhcAliaChain *fix[], int n, jhcAliaChain *label[]);

  // file writing
  int label_all (int *mark =NULL);
  void neg_jumps (int *step =NULL);


};


#endif  // once




