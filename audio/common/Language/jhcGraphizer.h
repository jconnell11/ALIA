// jhcGraphizer.h : turns parser alist into network structures
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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
// verb links:
//   "obj"   object affected
//   "dest"  location for action 
//   "src"   starting point for action
//   "cmd"   embedded action as object
//   "agt"   agent performing action
// property links:
//   "ako"   general noun class
//     "of"  for noun-noun modification ("baseball bat")
//     "wrt" for vague ownership (e.g. "my hand" = ako hand wrt me)
//   "hq"    general adjective class
//     "alt" for comparative adjectives ("than")
//   "loc"   spatial prepositional phrases 
//     "ref" anchor location for relation ("on the table")
//   "with"  generally tool used in action
//   "name"  proper name for object (e.g. "Dan")
//   "cnt"   how many of an object there are
//   "has"   descriptive part (e.g. "with a red top")
// modifier links:
//   "deg"  for properties (e.g "very")
//   "mod"  general adverb ("quickly")
//   "amt"  for command verbs (e.g. "far")
//   "dir"  for command verbs (e.g. "down")

class jhcGraphizer : protected jhcSlotVal
{
// PRIVATE MEMBER VARIABLES
private:
  // max length of intermediate association lists
  static const int amax = 1000; 

  // for resolving references
  jhcNodePool *univ;
  jhcAliaChain *skolem;
  int create, resolve;


// PROTECTED MEMBER VARIABLES
protected:
  class jhcAliaCore *core;

  // suggestions to add
  jhcAliaRule *rule;
  jhcAliaOp *oper;
  jhcAliaChain *bulk;                 


// PUBLIC MEMBER VARIABLES
public:
  // morphology (possibly shared)
  jhcMorphFcns mf;


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
  int append_ynq (jhcAliaChain *seq, jhcNodePool& pool) const;  
  int append_whq (jhcAliaChain *seq, jhcNodePool& pool) const;
  int append_exist (jhcAliaChain *seq, jhcNodePool& pool) const;
  int append_find (jhcAliaChain *seq, jhcNodePool& pool) const;
  jhcAliaChain *tell_step (const char *verb, jhcNodePool& pool) const;

  // rules
  int cvt_rule (const char *alist);
  bool build_fwd (jhcAliaRule& r, const char *alist);
  bool build_rev (jhcAliaRule& r, const char *alist);
  bool build_ifwd (jhcAliaRule& r, const char *alist);
  bool build_sfwd (jhcAliaRule& r, const char *alist);
  bool build_macro (jhcAliaRule& r, const char *alist) const;
  int build_graph (jhcGraphlet& gr, const char *alist, jhcNodePool& pool);

  // operators
  int cvt_op (const char *alist);
  jhcAliaOp *config_op (const char *alist) const;
  const char *kind_op (int& k, const char *alist, int veto) const;
  int build_sit (jhcSituation& sit, const char *alist, const char *ktag =NULL);
  double pref_val (const char *word) const;

  // command sequences
  jhcAliaChain *build_chain (const char *alist, jhcAliaChain *ult, jhcNodePool& pool);
  jhcAliaChain *dir_step (const char *kind) const;
  jhcNetNode *build_cmd (const char *head, const char *alist, jhcNodePool& pool);
  jhcNetNode *build_query (const char *alist, jhcNodePool& pool);
  void demote_bind () const;

  // action phrases
  jhcNetNode *build_do (const char *alist, jhcNodePool& pool);
  jhcNetNode *build_fact (const char **after, const char *alist, jhcNodePool& pool, 
                          jhcNetNode *subj =NULL, int pos =0);
  const char *act_deg (jhcNetNode *act, const char *amt, const char *alist, jhcNodePool& pool) const;
  int add_quote (jhcNetNode *v, const char *alist, jhcNodePool& pool) const;
  jhcNetNode *add_args (jhcNetNode *v, const char *alist, jhcNodePool& pool);
  void add_rels (jhcNetNode *act, const char *alist, jhcNodePool& pool);

  // object phrases
  jhcNetNode *build_obj (const char **after, const char *alist, jhcNodePool& pool, 
                         jhcNetNode *f0 =NULL, double blf =1.0, int qcnt =0);
  jhcNetNode *obj_owner (const char *alist, jhcNodePool& pool);
  jhcNetNode *ref_props (jhcNetNode *n, jhcNodePool& pool, const char *pron) const;
  bool match_any (const char *txt, const char *val, const char *val2, const char *val3 =NULL, const char *val4 =NULL) const;

  jhcNetNode *obj_deg (const char **after, jhcNetNode *obj, const char *deg, const char *alist, 
                       jhcNodePool& pool, int neg, double blf =1.0);
  jhcNetNode *obj_comp (const char **after, jhcNetNode *obj, const char *deg, const char *alist, 
                        jhcNodePool& pool, int neg, double blf =1.0);
  jhcNetNode *add_place (const char **after, jhcNetNode *obj, char *pair, const char *alist, 
                         jhcNodePool& pool, int neg =0, double blf =1.0);
  jhcNetNode *obj_has (const char **after, jhcNetNode *obj, const char *prep, const char *alist, 
                       jhcNodePool& pool, int neg =0, double blf =1.0);
  jhcNetNode *add_cop (const char **after, jhcNetNode *obj, const char *alist, jhcNodePool& pool, int pos =0);
  const char *nsuper_kind (char *kind, int ssz, const char *alist) const;
  double belief_val (const char *word) const;


};


#endif  // once




