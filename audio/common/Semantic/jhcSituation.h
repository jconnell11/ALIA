// jhcSituation.h : semantic network description to be matched
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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

#ifndef _JHCSITUATION_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSITUATION_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Semantic/jhcBindings.h"      // common audio
#include "Semantic/jhcGraphlet.h"
#include "Semantic/jhcNodeList.h"
#include "Semantic/jhcNodePool.h"


//= Semantic network description to be matched.
// short term memory must match condition but no unless pieces
// handles 2-part (jhcAliaRule) and 3-part (jhcAliaOp) matching
// basically encapsulates subgraph isomorphism matcher
// only does FULL matches, partial matches can get combinatorial

class jhcSituation : public jhcNodePool
{
// PROTECTED MEMBER VARIABLES
protected: 
  static const int umax = 5;           /** Maximum number of caveats. */

  // MUST and MUST NOT descriptions
  jhcGraphlet cond;
  jhcGraphlet unless[umax];
  int nu;

  // ref = restrict "you" and "me", chk = ignore neg
  int refmode, chkmode;


// PUBLIC MEMBER VARIABLES
public:
  double bth;                          // belief threshold  
  int dbg;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSituation ();
  jhcSituation ();
  void Init (const jhcGraphlet& desc);
  const jhcGraphlet *Pattern () const {return &cond;}
  int NumPat () const {return cond.NumItems();}
  bool InPat (const jhcNetNode *n) const {return cond.InDesc(n);}

  // helpers for construction
  void BuildCond ()  {BuildIn(&cond);}
  int BuildUnless () {if (nu >= umax) return 0; BuildIn(unless + nu); return ++nu;}
  void CmdHead (jhcNetNode *cmd) {cond.SetMain(cmd);}
  void PropHead () {cond.MainProp();}
  void UnlessHead () {if (nu > 0) unless[nu - 1].MainProp();}

  // main functions
  int MatchGraph (jhcBindings *m, int& mc, const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2 =NULL);
  int TryBinding (const jhcNetNode *focus, jhcNetNode *mate, 
                  jhcBindings *m, int& mc, const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2 =NULL);
                  

// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  int try_props (jhcBindings *m, int& mc, const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2);
  int try_args (jhcBindings *m, int& mc, const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2);
  int try_bare (jhcBindings *m, int& mc, const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2);
  int try_hash (jhcBindings *m, int& mc, const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2);
  int consistent (const jhcNetNode *mate, const jhcNetNode *focus, const jhcGraphlet& pat, const jhcBindings *b, double th) const;

  // virtuals to override
  virtual int match_found (jhcBindings *m, int& mc) {return 1;}


};


#endif  // once




