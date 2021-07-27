// jhcAliaRule.h : declarative implication in ALIA System 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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
// 
// each result fact has its own implied belief
//   if rule is matched > current threshold then input belief irrelevant
//
// adjustment of result beliefs:
//   when new fact added to main memory
//     if belief in halo < current threshold and correct then increment 
//     if belief in halo > current threshold and wrong then decrement 
// 
// alteration mostly performed in jhcActionTree::CompareHalo

class jhcAliaRule : public jhcSituation
{
friend class jhcAssocMem;              // collection
friend class jhcGraphizer;             // creation


// PRIVATE MEMBER VARIABLES
private:
  static const int hmax = 50;          /** Max halo instantiations. */

  // core information
  jhcGraphlet result;
  jhcAliaRule *next;
  int id, lvl;

  // run-time status
  jhcBindings hinst[hmax];
  int hyp[hmax];
  jhcWorkMem *wmem;
  int nh, show;


// PUBLIC MEMBER FUNCTIONS
public:
  // read only access
  int RuleNum () const {return id;}

  // main functions
  int AssertMatches (jhcWorkMem& f, double mth, int add =0, int noisy =0);
  void Inferred (jhcGraphlet& key, const jhcBindings& b) const;

  // halo consolidation
  void AddCombo (jhcBindings& m2c, const jhcAliaRule& step1, const jhcBindings& b1);
  void LinkCombo (jhcBindings& m2c, const jhcAliaRule& step2, const jhcBindings& b2);
  bool Identical (const jhcAliaRule& ref) const;
  bool Tautology ();

  // file functions
  int Load (jhcTxtLine& in);
  int Save (FILE *out, int detail =1) const;
  int Print (int detail =1) const {return Save(stdout, detail);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  ~jhcAliaRule ();
  jhcAliaRule ();

  // main functions (incl. virtual override)
  int match_found (jhcBindings *m, int& mc); 
  int same_result (const jhcBindings *m, int mc, int t0) const;
  bool result_uses (const jhcNetNode *key) const;
  void init_result (jhcBindings *b, int tval, int ver, int zero);

  // halo consolidation
  jhcNetNode *get_equiv (jhcBindings& m2c, const jhcNetNode *probe, int bcpy);
  void connect_args (jhcGraphlet& desc, const jhcBindings& m2c) const;
  bool same_struct (const jhcNetNode *focus, const jhcNetNode *mate) const;

  // file functions
  int load_clauses (jhcTxtLine& in);

int TryBinding0 (const jhcNetNode *focus, jhcNetNode *mate, jhcBindings *m, int& mc, 
                 const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2);
int try_bare0 (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
               const jhcNodeList& f, const jhcNodeList *f2);


};


#endif  // once




