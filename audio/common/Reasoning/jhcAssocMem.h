// jhcAssocMem.h : deductive rules for use in halo of ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#ifndef _JHCASSOCMEM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCASSOCMEM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>                     // needed for stdout

#include "Reasoning/jhcAliaRule.h"     
#include "Reasoning/jhcWorkMem.h"
#include "Semantic/jhcBindings.h"


//= Deductive rules for use in halo of ALIA system.
// holds a list of rules and applies them to jhcWorkMem to build halo

class jhcAssocMem
{
// PRIVATE MEMBER VARIABLES
private:
  // list of rules
  jhcAliaRule *rules;
  int nr;


// PUBLIC MEMBER VARIABLES
public:
  int noisy;                    // controls general diagnostic messages
  int detail;                   // show detailed matching for some rule


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAssocMem ();
  jhcAssocMem ();
  int NumRules () const {return nr;}
  int ClearRules () {return clear();}
 
  // list functions
  jhcAliaRule *RuleList () const {return rules;}
  jhcAliaRule *NextRule (jhcAliaRule *r) const 
    {return((r == NULL) ? rules : r->next);} 
  int AddRule (jhcAliaRule *r, int ann =0);
  void Remove (const jhcAliaRule *rem);

  // main functions
  int RefreshHalo (jhcWorkMem& wmem, int dbg =0) const;
  int Consolidate (const jhcBindings& b, int dbg =1);

  // file functions
  int Load (const char *base, int add =0, int rpt =0, int level =1);
  int Save (const char *base, int level =1) const;
  int Print (int level =1) const {return save_rules(stdout, level);}
  int Alterations (const char *base) const;
  int Overrides (const char *base);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  int clear ();

  // main functions
  int next_halo (jhcAliaRule **r, jhcBindings **b, jhcBindings& list, int start) const;

  // file functions
  int save_rules (FILE *out, int level) const;


};


#endif  // once




