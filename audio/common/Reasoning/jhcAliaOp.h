// jhcAliaOp.h : advice on what to do given some stimulus or desire
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

#ifndef _JHCALIAOP_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIAOP_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcTxtLine.h"          // common audio
#include "Reasoning/jhcWorkMem.h"
#include "Semantic/jhcBindings.h"
#include "Semantic/jhcSituation.h"

#include "Action/jhcAliaChain.h"       // common robot
#include "Action/jhcAliaDir.h"


//= Advice on what to do given some stimulus.

class jhcAliaOp : public jhcSituation
{
friend class jhcProcMem;               // collection
friend class jhcGraphizer;             // creation


// PRIVATE MEMBER VARIABLES
private:
  // definition and list structure
  jhcAliaOp *next;
  JDIR_KIND kind; 
  int id, lvl;

  // matching state
  int first, tval, omax;


// PUBLIC MEMBER VARIABLES
public:
  // proposed action and desirability
  jhcAliaChain *meth;
  double pref;


// PUBLIC MEMBER FUNCTIONS
public:
  // read only functions
  int Inst () const {return id;}
  JDIR_KIND Kind () const {return kind;}
  const char *KindTag () const {jhcAliaDir dcvt; return dcvt.CvtTag(kind);}

  // main functions
  int FindMatches (jhcAliaDir& dir, const jhcWorkMem& f, double mth, int tol =0);
  bool SameEffect (const jhcBindings& b1, const jhcBindings& b2) const;

  // file functions
  int Load (jhcTxtLine& in); 
  int Save (FILE *out, int detail =2);
  int Print (int detail =0) {return Save(stdout, detail);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  ~jhcAliaOp ();
  jhcAliaOp (JDIR_KIND k =JDIR_NOTE);
 
  // main functions
  int try_mate (jhcNetNode *mate, jhcAliaDir& dir, const jhcWorkMem& f, int tol);

  // file functions
  int load_pattern (jhcTxtLine& in);

  // virtual override
  int match_found (jhcBindings *m, int& mc);


};


#endif  // once




