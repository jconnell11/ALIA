// jhcGraphizer.h : turns parser alist into network structures
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#pragma once

#include "jhcGlobal.h"

#include "Language/jhcMorphFcns.h"     // common audio
#include "Reasoning/jhcAliaOp.h"
#include "Reasoning/jhcAliaRule.h"
#include "Semantic/jhcGraphlet.h"
 
#include "Parse/jhcSlotVal.h"          


//= Turns parser alist into network structures.
// set "dbg" in base class to 1 to see call sequence on input
// verb argument links:
//   "obj"    object affected
//   "act"    embedded action as object
//   "agt"    agent performing action
// verb property links:
//   "fcn"    verb or class associated with action
//   "mod"    general adverb ("quickly")
//   "dir"    direction of action (e.g. "down")
//   "amt"    size of action (e.g. "far")
//   "src"    starting point for object or action (with "ref")
//   "dest"   location for action (with "ref")
// noun property links:
//   "name"   proper name for object (e.g. "Dan")
//   "ako"    general noun class
//     "of"   for noun-noun modification ("baseball bat")
//     "wrt"  for vague ownership (e.g. "my hand" = ako hand wrt me)
//   "hq"     general adjective class
//     "alt"  for comparative adjectives ("than")
//   "loc"    current spatial location
//     "ref"  anchor item for relation ("on the table")
//     "ref2" second anchor item ("between the salt and the pepper")
//   "cnt"    how many of an object there are
//   "has"    descriptive part (e.g. "with a red top")
// special property links:
//   "deg"    intensifier for adjective or adverb (e.g "very")

class jhcGraphizer : protected jhcSlotVal
{
// PRIVATE MEMBER VARIABLES
private:
  static const int amax = 1000;        /** Max length of intermediate alists. */
  static const int nest = 4;           /** Maximum nesting depth of loops.    */

  // for resolving references
  jhcNodePool *univ;
  jhcAliaChain *skolem;
  int create, resolve;

  // for implicit loops
  jhcAliaChain *multi;                  // "for" multi-step loop (if any)
  jhcAliaChain *root;                   // outermost looping EACH/ANY
  jhcAliaChain *loop;                   // innermost looping EACH/ANY


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

  // show subroutine calls
  int dbg;                


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcGraphizer ();
  jhcGraphizer ();
  void Bind (class jhcAliaCore *all) {core = all;}

  // main functions
  void ClearLast ();
  int Assemble (const char *alist);
  jhcAliaChain *TrySeq () const {return bulk;}


// PROTECTED MEMBER FUNCTIONS
protected:
  // utilities
  bool match_any (const char *txt, const char *val, const char *val2, const char *val3 =NULL,
                  const char *val4 =NULL, const char *val5 =NULL, const char *val6 =NULL) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // attention items
  int cvt_imm (const char *alist);
  int append_ynq (jhcAliaChain *seq, jhcNodePool& pool) const;  
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

  // operator revisions
  int cvt_rev (const char *alist);

  // command sequences
  jhcAliaChain *build_chain (const char *alist, jhcAliaChain *ult, jhcNodePool& pool);
  jhcAliaChain *play_step (int& mode, const char *alist, jhcNodePool& pool);
  jhcAliaChain *overt_loop (const char *alist, jhcNodePool& pool);
  jhcAliaChain *single_cmd (const char *entry, const char *alist, jhcNodePool& pool);
  jhcAliaChain *dir_step (const char *kind) const;
  jhcAliaChain *connect_loop (jhcAliaChain *ch0, jhcAliaChain *mini);
  void finish_loop (jhcAliaChain *ch, jhcAliaChain *ult);
  jhcNetNode *build_cmd (const char *head, const char *alist, jhcNodePool& pool);
  jhcNetNode *build_query (const char *alist, jhcNodePool& pool);
  void demote_bind () const;

  // action phrases
  jhcNetNode *build_ach (const char *alist, jhcNodePool& pool);
  jhcNetNode *build_do (const char *alist, jhcNodePool& pool);
  jhcNetNode *build_name (const char *alist, jhcNodePool& pool); 
  jhcNetNode *build_fact (const char **after, const char *alist, jhcNodePool& pool, 
                          jhcNetNode *subj =NULL, int pos =0);
  const char *act_deg (jhcNetNode *act, const char *amt, const char *alist, jhcNodePool& pool) const;
  const char *act_amt (jhcNetNode *act, const char *num, const char *alist, jhcNodePool& pool) const;
  int add_quote (jhcNetNode *v, const char *alist, jhcNodePool& pool) const;
  jhcNetNode *add_args (jhcNetNode *v, const char *alist, jhcNodePool& pool);
  void add_rels (jhcNetNode *act, const char *alist, jhcNodePool& pool);

  // object phrases
  jhcNetNode *build_obj (const char **after, const char *alist, jhcNodePool& pool, 
                         jhcNetNode *f0 =NULL, double blf =1.0, int qcnt =0);
  void obj_poss (jhcNetNode *obj, jhcNetNode *kind, const char *alist, jhcNodePool& pool);
  void obj_comp (jhcNetNode **fact, jhcNetNode *obj, const char *alist, jhcNodePool& pool);
  int setup_loop (const char *word);

  // basic object description
  jhcNetNode *obj_desc (jhcNetNode **last, jhcNetNode *obj, const char *alist, jhcNodePool& pool, double blf);
  jhcNetNode *ref_props (jhcNetNode *n, jhcNodePool& pool, const char *pron) const;
  jhcNetNode *adj_comp (const char **after, jhcNetNode *obj, const char *deg, const char *alist, 
                        jhcNodePool& pool, int neg, double blf =1.0);
  jhcNetNode *obj_deg (const char **after, jhcNetNode *obj, const char *deg, const char *alist, 
                       jhcNodePool& pool, int neg, double blf =1.0);
  jhcNetNode *obj_has (const char **after, jhcNetNode *obj, const char *prep, const char *alist, 
                       jhcNodePool& pool, int neg =0, double blf =1.0);
  jhcNetNode *add_place (const char **after, jhcNetNode *obj, char *pair, const char *alist, 
                         jhcNodePool& pool, int neg =0, double blf =1.0);

  // copula interpretation
  jhcNetNode *add_cop (const char **after, jhcNetNode *obj, const char *alist, jhcNodePool& pool, int pos =0);
  jhcNetNode *obj_owner (const char *alist, jhcNodePool& pool);
  double belief_val (const char *word) const;
  const char *nsuper_kind (char *kind, int ssz, const char *alist) const;

  // number strings
  const char *parse_int (char *cnt, const char *txt, int ssz) const;

  // utilities
  void call_list (int lvl, const char *fcn, const char *alist, int skip =0, const char *entry =NULL) const;


};

