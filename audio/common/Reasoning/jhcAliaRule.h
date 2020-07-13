// jhcAliaRule.h : declarative implication in ALIA System 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#ifndef _JHCALIARULE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIARULE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcTxtLine.h"          // common audio
#include "Reasoning/jhcWorkMem.h"
#include "Semantic/jhcGraphlet.h"
#include "Semantic/jhcSituation.h"


//= Declarative implication in ALIA System.

class jhcAliaRule : public jhcSituation
{
friend class jhcAssocMem;              // collection
friend class jhcGraphizer;             // creation


// PRIVATE MEMBER VARIABLES
private:
  jhcGraphlet result;
  jhcAliaRule *next;
  jhcWorkMem *wmem;
  double conf;
  int id, lvl, src0, src1, show;


// PUBLIC MEMBER FUNCTIONS
public:
  // read only access
  int RuleNum () const {return id;}
  double Confidence () const {return conf;}

  // source attribution
  bool Asserted (const jhcNetNode *n) const;

  // main functions
  int AssertMatches (jhcWorkMem& f, double mth, int dbg =0);

  // file functions
  int Load (jhcTxtLine& in);
  int Save (FILE *out, int detail =2) const;
  int Print (int detail =0) const {return Save(stdout, detail);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  ~jhcAliaRule ();
  jhcAliaRule ();

  // main functions (virtual override)
  int match_found (jhcBindings *m, int& mc);

  // file functions
  int load_clauses (jhcTxtLine& in);


};


#endif  // once




