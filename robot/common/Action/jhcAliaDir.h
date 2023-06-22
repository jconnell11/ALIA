// jhcAliaDir.h : directive states what sort of thing to do in ALIA system
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

#ifndef _JHCALIADIR_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIADIR_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcTxtLine.h"          // common audio
#include "Semantic/jhcBindings.h"
#include "Semantic/jhcGraphlet.h"
#include "Semantic/jhcNetNode.h"
#include "Semantic/jhcNodePool.h"
#include "Semantic/jhcSituation.h"

#include "Action/jhcAliaChain.h"       // common robot


//= Various different kinds of directives.
// NOTE is a special bridge between declarative and procedural
// must remain consistent with "ktag" strings
//                  0          1          2          3          4          5          6          7      
enum JDIR_KIND {JDIR_NOTE, JDIR_DO,   JDIR_ANTE, JDIR_GATE, JDIR_PUNT, JDIR_GND,  JDIR_WAIT, JDIR_ACH, 
                JDIR_FIND, JDIR_BIND, JDIR_EACH, JDIR_ANY,  JDIR_CHK,  JDIR_ESC,  JDIR_ADD,  JDIR_EDIT, JDIR_MAX};
//                  8          9         10         11         12         13         14         15         16  


///////////////////////////////////////////////////////////////////////////

//= Directive states what sort of thing to do in ALIA system.
// handles selection of a method and orchestration of FSM states
// this is a first-class structure responsible for most of the work
// in particular is is responsible for choosing which operators to use
// <pre>
// NOTE: for asserting a new fact
//       tries all applicable handler operators one-by-one
//       operators can be matched with halo inferences
//       succeeds when first operator succeeds
//       can start whole operator selection multiple times
//       fails when exceeds persistence time budget
//
//   DO: for requesting action or stopping on-going action
//       calls ANTE and GATE before main if requesting
//       tries all applicable method operators one-by-one
//       succeeds when first operator succeeds 
//       fails when no more operators 
//       empty description succeeds immediately (no-op)
// 
// ANTE: preparing for an action or method selection
//       tries all applicable operators one-by-one
//       success or failure of operators irrelevant
//       progresses to GATE when no more operators
//       part of ANTE-GATE-DO automatic sequence
// 
// GATE: checking prohibitions on action
//       relevant operators end with PUNT or PASS
//       progresses to DO when first operator succeeds
//       part of ANTE-GATE-DO automatic sequence
// 
// PUNT: signal failure of triggering condition
//       forces current sequence to fail with no retry
//       never a trigger for operators
// 
//  GND: interface to kernel routines (including output)
//       done determined externally by kernel
//       returns success/fail
//       never a trigger for operators
// 
// WAIT: holds up execution until fact is matched
//       suceeds (cont) when fact is true
//       never a trigger for operators
// 
//  ACH: work towards making item true
//       tries all applicable operators one-by-one
//       succeeds when item true (new or old)
//       fails when no more operators   
// 
// FIND: bind description to known item
//       guess: wmem > ops > LTM (no assume)
//       succeeds (temporarily) when a binding found
//       backtracks to get new binding if later directives fail
//       fails when no more guesses or max of 3 tried
// 
// BIND: similar to FIND but will create new item if stuck
//       only used for referential phrases, never a command
//       triggers FIND operators
//
// EACH: similar to FIND but unlimited guesses
//       fails if no first binding, never backtracks
//       succeeds (alt) when no more bindings 
//       typically used for implicit "all" loops
//       triggers FIND operators
//
//  ANY: similar to EACH but succeeds (alt) if no bindings
//       triggers FIND operators
//
//  CHK: continue if assertion is newly known to be true/false
//       tries all applicable operators one-by-one
//       succeeds (cont) when item is definitely true
//       succeeds (alt) when item is definitely false 
//       fails when no more operators and no matching facts
// 
//  ESC: escape if assertion is newly known to be true/false
//       tries all applicable operators one-by-one
//       succeeds (cont) when item is definitely false
//       succeeds (cont) when no more operators and no matching facts
//       fails when item is definitely true
//       triggers CHK operators
//
//  ADD: accept new rule or operator into system
//       comes after speech act to allow rejection 
//       never a trigger for operators
//  
// EDIT: alter or replace operator responsible for action
//       follow-on directives replace action in new operator
//       negative action description always dings preference
//       positive boosts preference only if no follow-on
//       never a trigger for operators
// 
// </pre>

class jhcAliaDir : private jhcSituation
{
// PRIVATE MEMBER VARIABLES
private:
  static const int omax = 20;      /** Max number of operator choices.   */
  static const int hmax = 20;      /** Max non-return inhibit history.   */
  static const int smax = 20;      /** Max key variable substitutions.   */
  static const int emax = 20;      /** Maximum EACH/ANY instantiations.  */    
  static const int gmax = 3;       /** Maximum FIND/BIND guesses to try. */    

  // name associated with each "kind"
  static const char * const ktag[JDIR_MAX];

  // calling environment and scoping
//  jhcAliaChain *step;
  jhcNetNode *fact[smax], *val0[smax];
  int anum[smax];
  int nsub, inum0;

  // partial CHK and FIND control
  jhcGraphlet full;
  const jhcNetNode *focus;
  jhcNetNode *pron;
  int subset, find0, chk0, exc, recent;

  // choices for FIND directive
  jhcNetNode *guess[emax];
  jhcGraphlet hyp;
  int cand, cand0, fdbg;

  // already tried operators
  class jhcAliaOp *op0[hmax];
  jhcBindings m0[hmax];
  int result[hmax];
  int nri;

  // execution state
  jhcGraphlet ctx;
  int inst, verdict, wait, chk_state;

  // NOTE perseverance
  UL32 t1, fin;


// PUBLIC MEMBER VARIABLES
public:
  // basic configuration
  jhcGraphlet key; 
  JDIR_KIND kind;
  int root, own;

  // payload for ADD 
  class jhcAliaRule *new_rule;
  class jhcAliaOp *new_oper;

  // current matching progress
  class jhcAliaOp *op[omax];
  jhcBindings match[omax];
  int mc, anyops;

  // action currently in progress
  jhcAliaChain *meth;
  UL32 t0; 
  int sel;

  // control of procedural diagnostic messages
  int noisy;                    


jhcAliaChain *step;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaDir ();
  jhcAliaDir (JDIR_KIND k =JDIR_NOTE);
  JDIR_KIND Kind () const      {return kind;}
  bool IsNote () const         {return(kind == JDIR_NOTE);}
  bool IsFind () const         {return((kind == JDIR_FIND) || (kind == JDIR_BIND));}
  const char *KindTag () const {return ktag[kind];}
  jhcNetNode *KeyMain () const {return key.Main();}
  jhcNetNode *KeyAct () const  {return key.MainAct();}
  const char *KeyNick () const {return key.MainNick();}
  const char *KeyTag () const  {return key.MainTag();}
  int MaxOps () const          {return omax;}
  int MaxHist () const         {return hmax;}
  JDIR_KIND CvtKind (const char *name) const;
  const char *CvtTag (JDIR_KIND k) const;
  int MaxDepth (int cyc =1);
  int NumGoals (int leaf =0, int cyc =1);
  const jhcAliaChain *Method () const {return meth;}
  int NumGuess () const {return __max(cand, cand0);}
  int NumTries () const {return nri;}
  bool Assumed () const {return !hyp.Empty();}
  void HideAssume () 
    {jhcNetNode *n = hyp.Main(); if (n != NULL) n->SetBelief(0.0);}
  bool UserSelf () const  
    {return((cond.NumItems() == 1) && (cond.Main())->LexIn("me", "you"));}

  // building
  void Copy (const jhcAliaDir& ref);
  int CopyBind (jhcNodePool& pool, const jhcAliaDir& ref, jhcBindings& b, const jhcGraphlet *ctx =NULL);
  void SetKind (JDIR_KIND k2) {kind = k2;}
  int SetKind (const char *tag);
  void SetMethod (jhcAliaChain *ch) {meth = ch;}
  bool HasAlt () const {return((kind == JDIR_CHK) || (kind == JDIR_EACH) || (kind == JDIR_ANY));}
  bool Involves (const jhcNetNode *item) const;
  int RefDir (jhcNetNode *src, const char *slot, jhcNodePool& pool) const;
  void MarkSeeds ();

  // main functions
  int Start (jhcAliaChain *st);
  int Status ();
  int Stop ();

  // execution tracing
  int HaltActive (const jhcGraphlet& desc, const jhcAliaDir *skip =NULL, int halt =1);
  void FindCall (const jhcAliaDir **act, const jhcAliaDir **src, jhcBindings *d2a, 
                 const jhcGraphlet& desc, UL32& start, int done =1, const jhcAliaDir *prev =NULL, int cyc =1);
  class jhcAliaOp *LastOp () const     {return((sel < 0) ? NULL : op[sel]);}
  const jhcBindings *LastVars () const {return((sel < 0) ? NULL : &(match[sel]));}

  // file functions
  int Load (jhcNodePool& pool, jhcTxtLine& in); 
  int Save (FILE *out, int lvl =0, int detail =1) const;
  void Print (int lvl =0, int detail =1) {Save(stdout, lvl, detail);}
  void Print (const char *tag, int lvl =0, int detail =1);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  int reset ();

  // building
  void share_context (jhcNodePool& pool, const jhcGraphlet *ctx);

  // main functions
  void divorce_key (class jhcWorkMem *pool, int lvl);
  int chk_preempt ();
  int first_method ();
  int do_status (int res, int more);
  int next_method ();
  int report (int val);
  void alter_pref () const;
  void err_rule ();
  void halt_subgoal ();

  // variable scoping
  void subst_key (jhcBindings *sc);
  void revert_key ();

  // method selection
  int pick_method ();
  int match_ops (int& sel);
  int max_spec (int& sel);
  int wtd_rand (double wild) const;
  void get_context (jhcGraphlet *ctx, jhcNetNode *focus, const jhcBindings& b) const;

  // CHK control 
  void init_cond (int ver);
  int reduce_cond (const class jhcAliaOp *op, const jhcBindings& match);
  int seek_match ();
  int pat_confirm (const jhcGraphlet& desc, int chk);
  int complete_chk (jhcBindings *m, int& mc);

  // FIND control
  int me_you ();
  int seek_instance ();
  jhcNetNode *sat_criteria (const jhcGraphlet& desc, int exc, int after, int ltm);
  int complete_find (jhcBindings *m, int& mc);
  int filter_pron (jhcNetNode *mate);
  void pron_gender (jhcNetNode *mate, int note);
  class jhcAliaChain *chk_method ();
  int recall_assume ();
  int assume_found ();
  jhcNetNode *lift_key ();

  // jhcSituation override
  int match_found (jhcBindings *m, int& mc)
    {return((chkmode == 0) ? complete_find(m, mc) : complete_chk(m, mc));}
  

};


#endif  // once




