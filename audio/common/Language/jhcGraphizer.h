// jhcGraphizer.h : turns parser alist into network structures
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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

#ifndef _JHCGRAPHIZER_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGRAPHIZER_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Language/jhcMorphFcns.h"     // common audio
#include "Reasoning/jhcAliaOp.h"
#include "Reasoning/jhcAliaRule.h"
#include "Semantic/jhcGraphlet.h"

#include "Parse/jhcSlotVal.h"          


//= Turns parser alist into network structures.
// set "dbg" in base class to 1 to see call sequence on input

class jhcGraphizer : protected jhcSlotVal
{
// PRIVATE MEMBER VARIABLES
private:
  // max length of intermediate association lists
  static const int amax = 1000; 


// PROTECTED MEMBER VARIABLES
protected:
  class jhcAliaCore *core;


// PUBLIC MEMBER VARIABLES
public:
  // morphology (possibly shared)
  jhcMorphFcns mf;

  // suggestions to add
  jhcAliaRule *rule;
  jhcAliaOp *oper;
  jhcAliaChain *bulk;                 


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcGraphizer ();
  jhcGraphizer ();
  void Bind (class jhcAliaCore *all) {core = all;}

  // main functions
  void ClearLast ();
  int Assemble (const char *alist);
  

// PRIVATE MEMBER FUNCTIONS
private:
  // attention items
  int cvt_attn (const char *alist);
  double belief_val (const char *word) const;

  // rules
  int cvt_rule (const char *alist);
  bool build_fwd (jhcAliaRule& r, const char *alist) const;
  bool build_rev (jhcAliaRule& r, const char *alist) const;
  bool build_ifwd (jhcAliaRule& r, const char *alist) const;
  bool build_sfwd (jhcAliaRule& r, const char *alist) const;
  bool build_macro (jhcAliaRule& r, const char *alist) const;
  int build_graph (jhcGraphlet& gr, const char *alist, jhcNodePool& pool) const;

  // operators
  int cvt_op (const char *alist);
  jhcAliaOp *config_op (const char *alist) const;
  const char *kind_op (int& k, const char *alist, int veto) const;

  double pref_val (const char *word) const;
  int build_sit (jhcSituation& sit, const char *alist, const char *ktag =NULL) const;

  // command sequences
  jhcAliaChain *build_chain (const char *alist, jhcAliaChain *final, jhcNodePool& pool) const;
  jhcAliaChain *dir_step (const char *kind) const;
  jhcNetNode *build_cmd (const char *head, const char *alist, jhcNodePool& pool) const;
  jhcNetNode *query_ako (const char *alist, jhcNodePool& pool) const;
  jhcNetNode *query_hq (const char *alist, jhcNodePool& pool) const;

  // action phrases
  jhcNetNode *build_do (const char *alist, jhcNodePool& pool) const;
  jhcNetNode *build_fact (const char **after, const char *alist, jhcNodePool& pool, 
                          jhcNetNode *subj =NULL, int pos =0) const;
  const char *act_deg (jhcNetNode *act, const char *amt, const char *alist, jhcNodePool& pool) const;
  int add_quote (jhcNetNode *v, const char *alist, jhcNodePool& pool) const;
  jhcNetNode *add_args (jhcNetNode *v, const char *alist, jhcNodePool& pool) const;
  void add_rels (jhcNetNode *act, const char *alist, jhcNodePool& pool) const;

  // object phrases
  jhcNetNode *build_obj (const char **after, const char *alist, jhcNodePool& pool, 
                         jhcNetNode *f0 =NULL, int neg =0, double blf =1.0) const;
  jhcNetNode *ref_props (jhcNetNode *n, jhcNodePool& pool, const char *pron, int neg) const;
  jhcNetNode *obj_deg (const char **after, jhcNetNode *obj, const char *deg, const char *alist, 
                       jhcNodePool& pool, int neg =0, double blf =1.0) const;
  jhcNetNode *add_place (const char **after, jhcNetNode *obj, char *pair, const char *alist, 
                         jhcNodePool& pool, int neg =0, double blf =1.0) const;
  jhcNetNode *obj_has (const char **after, jhcNetNode *obj, const char *prep, const char *alist, 
                       jhcNodePool& pool, int neg =0, double blf =1.0) const;
  jhcNetNode *add_cop (const char **after, jhcNetNode *obj, const char *alist, jhcNodePool& pool, int pos =0) const;

};


#endif  // once




