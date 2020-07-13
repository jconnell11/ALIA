// jhcAliaDir.h : directive states what sort of thing to do in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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
#include "Semantic/jhcNodePool.h"

#include "Action/jhcAliaChain.h"       // common robot


//= Various different kinds of directives.
// NOTE is a special bridge between declarative and procedural
// must remain consistent with "ktag" strings
//                  0          1          2         3          4          5
enum JDIR_KIND {JDIR_NOTE, JDIR_DO,  JDIR_ANTE, JDIR_POST, JDIR_PUNT, JDIR_FCN, 
                JDIR_CHK,  JDIR_ACH, JDIR_KEEP, JDIR_FIND, JDIR_ADD,  JDIR_MAX};
//                  6          7          8         9         10         11       


///////////////////////////////////////////////////////////////////////////

//= Directive states what sort of thing to do in ALIA system.
// handles selection of a method and orchestration of FSM states
// this is a first-class structure responsible for most of the work
// in particular is is responsible for choosing which operators to use
// <pre>
// NOTE: for asserting a new fact
//       try all applicable operators one-by-one
//       done (success) when no more operators
//       times out when done (can only be top-level)
//       can be matched with halo inferences
//
//   DO: for requesting an action
//       automatically calls ANTE before
//       try all applicable operators one-by-one
//       success when first operator succeeds (mark)
//       fail when no more operators (marked)
//       progress to DO when done (success/fail)
//       times out when done if top-level focus    
// 
// ANTE: preparing for an action
//       try all applicable operators one-by-one
//       progress to DO when no more operators
//       cannot be top-level focus
// 
// POST: cleaning up after an action
//       keys off DO marked result (success/fail)
//       try all applicable operators one-by-one
//       return DO result when no more operators
//       cannot be top-level focus
// 
// PUNT: signal failure of triggering condition
//       shortcircuits backtracking mechanism
//       cannot be top-level focus 
//
//  FCN: interface to kernel routines (including output)
//       done determined externally
//       returns success/fail
//       cannot be top-level focus 
// 
//  CHK: actively test if item true/false
//       try all applicable operators one-by-one
//       done (success) when item newly true/false (NOTE)
//       returns continue/alternate based on true/false
//       done (fail) when no more operators   
//       times out when done if top-level focus    
// 
//  ACH: work towards making item true
//       done (success) when item true (new or old)
//       try all applicable operators one-by-one
//       done (fail) when no more operators   
//       times out when done if top-level focus  
// 
// KEEP: prevent item from becoming false
//       done (fail) when item false (new or old)
//       try all applicable operators one-by-one
//       done (idle) when no more operators
//       never succeeds, can only be stopped
//       times out when done if top-level focus 
// 
// FIND: bind description to known item
//       try all applicable operators one-by-one
//       done (fail) when no more operators
//       done (temporarily) when a binding found
//       if successors fail, restart to get new binding
//       times out when done if top-level focus 
//
//  ADD: accept new rule or operator into system
//       allows delay to see if user is reliable
// </pre>

class jhcAliaDir 
{
// PRIVATE MEMBER VARIABLES
private:
  static const int omax = 20;      /** Max number of operator choices. */
  static const int hmax = 20;      /** Max non-return inhibit history. */
  static const int smax = 20;      /** Maximum FIND selections.        */

  // name associated with each "kind"
  static const char * const ktag[JDIR_MAX];

  // already tried operators
  class jhcAliaOp *op0[hmax];
  jhcBindings m0[hmax];
  int nri;

  // FIND node candidates 
  jhcNetNode *foci[smax];
  int cand;

  // execution state
  class jhcAliaCore *core;
  jhcAliaChain *meth;
  jhcAliaDir *punt;
  jhcGraphlet ctx;
  int inst, verdict;


// PUBLIC MEMBER VARIABLES
public:
  // configuration
  jhcGraphlet key; 
  JDIR_KIND kind;
  int root;

  // operators to choose from
  class jhcAliaOp *op[omax];
  jhcBindings match[omax];
  int mc;

  // value for nodes related to NOTE
  int own;

  // control of diagnostic messages
  int noisy;                    


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaDir ();
  jhcAliaDir (JDIR_KIND k =JDIR_NOTE);
  JDIR_KIND Kind () const      {return kind;}
  bool IsNote () const         {return(kind == JDIR_NOTE);}
  const char *KindTag () const {return ktag[kind];}
  int MaxOps () const  {return omax;}
  int MaxHist () const {return hmax;}
  JDIR_KIND CvtKind (const char *name) const;
  const char *CvtTag (JDIR_KIND k) const;

  // building
  void Copy (const jhcAliaDir& ref);
  int CopyBind (jhcNodePool& pool, const jhcAliaDir& ref, jhcBindings& b, const jhcGraphlet *ctx =NULL);
  int SetKind (const char *tag);
  bool HasAlt () const {return((kind == JDIR_CHK) || (kind == JDIR_FIND));}
  bool Involves (const jhcNetNode *item) const;
  void MarkSeeds ();

  // main functions
  int Start (class jhcAliaCore& all);
  int Status ();
  int Stop ();

  // file functions
  int Load (jhcNodePool& pool, jhcTxtLine& in); 
  int Save (FILE *out, int lvl =0, int detail =2);
  void Print (int lvl =0, int detail =0) {Save(stdout, lvl, detail);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  int reset ();

  // building
  void share_context (const jhcGraphlet *ctx) const;

  // main functions
  int first_method ();
  int do_status (int res);
  int next_method ();
  int report (int val);

  // method selection
  int pick_method ();
  void get_context (jhcGraphlet *ctx, jhcNetNode *focus, const jhcBindings& b) const;
  int match_ops (int& sel);
  int max_spec (int& sel);
  int wtd_rand (double wild) const;
  double std_factor (double lo, double avg, int cnt, double wild) const;
  int rand_best (double temp);


};


#endif  // once




