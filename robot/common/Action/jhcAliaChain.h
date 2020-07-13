// jhcAliaChain.h : sequence backbone for activities in an FSM chain
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2018 IBM Corporation
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
  // deletion and serialization flags
  int cut, idx; 

  // payload is one of two types
  class jhcAliaDir *d;
  class jhcAliaPlay *p;

  // run status on last few cycles
  class jhcAliaCore *core;
  int prev, done;


// PUBLIC MEMBER VARIABLES
public:
  // next step in chain (public for jhcGraphizer)
  jhcAliaChain *cont;
  jhcAliaChain *alt;
  int alt_fail;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaChain ();
  jhcAliaChain ();

  // configuration
  void BindDir (class jhcAliaDir *dir)
    {if ((p == NULL) && (d == NULL)) d = dir;}
  void BindPlay (class jhcAliaPlay *play)
    {if ((p == NULL) && (d == NULL)) p = play;}
  class jhcAliaDir *GetDir () const {return d;}
  class jhcAliaPlay *GetPlay () const {return p;}
  bool Empty () const 
    {return((d == NULL) && (p == NULL));}
  jhcAliaChain *StepN (int n);

  // building
  jhcAliaChain *Instantiate (jhcNodePool& mem, jhcBindings& b, const jhcGraphlet *ctx =NULL);
  bool Involves (const jhcNetNode *item, int top =1);
  void MarkSeeds (int head =1);

  // main functions
  int Start (class jhcAliaCore& core);
  int Status ();
  int Stop ();

  // file functions
  int Load (jhcNodePool& pool, jhcTxtLine& in, int play =0); 
  int Save (FILE *out, int lvl =0, int *step =NULL, int detail =2);
  int Print (int lvl =0, int detail =0)
    {return Save(stdout, lvl, NULL, detail);}
  int PrintStep (int lvl =0)
    {return Save(stdout, -(lvl + 1), NULL, 0);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void cut_loops ();

  // building
  jhcAliaChain *dup_self (int& node, jhcAliaChain *seen[], jhcNodePool& mem, 
                          jhcBindings& b, const jhcGraphlet *ctx);
  void clr_labels (int head);

  // file reading
  int build_chain (jhcNodePool& pool, jhcAliaChain *label[], jhcAliaChain *fix[], int& n, jhcTxtLine& in);
  int get_payload (jhcNodePool& pool, jhcAliaChain *label[], jhcTxtLine& in);
  int link_graph (jhcAliaChain *fix[], int n, jhcAliaChain *label[]);

  // file writing
  int label_all (int *mark =NULL);
  void neg_jumps (int *step =NULL);


};


#endif  // once




