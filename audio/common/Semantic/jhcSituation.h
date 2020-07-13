// jhcSituation.h : semantic network description to be matched
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

class jhcSituation : public jhcNodePool
{
// PROTECTED MEMBER VARIABLES
protected: 
  static const int umax = 5;        /** Maximum number of caveats. */

  // MUST and MUST NOT descriptions
  jhcGraphlet cond;
  jhcGraphlet unless[umax];
  int nu;

  // restrict "you" and "me" 
  int refmode;


// PUBLIC MEMBER VARIABLES
public:
  // belief threshold
  double bth;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSituation ();
  jhcSituation ();

  // helpers for construction
  void CondPart ()  {BuildIn(&cond);}
  int UnlessPart () {if (nu >= umax) return 0; BuildIn(unless + nu); return ++nu;}
  void CmdHead (jhcNetNode *cmd) {cond.SetMain(cmd);}
  void PropHead () {cond.MainProp();}
  void UnlessHead () {if (nu > 0) unless[nu - 1].MainProp();}


// PROTECTED MEMBER FUNCTIONS
protected:
  // main functions
  int MatchGraph (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                  const jhcNodeList& f, const jhcNodeList *f2 =NULL, int tol =0);


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  int try_props (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                 const jhcNodeList& f, const jhcNodeList *f2, int tol);
  int try_args (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                const jhcNodeList& f, const jhcNodeList *f2, int tol);
  int try_bare (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                const jhcNodeList& f, const jhcNodeList *f2, int tol);
  int try_binding (const jhcNetNode *focus, jhcNetNode *mate, jhcBindings *m, int& mc, 
                   const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2, int tol);
  bool consistent (const jhcNetNode *mate, const jhcNetNode *focus, const jhcBindings *b, double th) const;


  // virtuals to override
  virtual int match_found (jhcBindings *m, int& mc) {return 1;}


};


#endif  // once




