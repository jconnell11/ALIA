// jhcAliaAttn.h : holds attentional foci for ALIA system
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

#ifndef _JHCALIAATTN_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIAATTN_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>                     // needed for FILE

#include "Action/jhcAliaChain.h"       // common robot

#include "Reasoning/jhcAliaNote.h"      
#include "Reasoning/jhcWorkMem.h"      


//= Holds attentional foci for ALIA system.
// composite 3 level memory: attention > main > halo
// focus array:
//   foci can be plays or directives (including NOTE)
//   items removed after being finished for some time
//   array is kept compacted so tail slots are always free
// running:
//   each foci has importance (wt) which determines priority
//   importance of NOTE comes from currently instantiated rule
//   all unfinished activities are run from newest to oldest

class jhcAliaAttn : public jhcWorkMem, public jhcAliaNote
{
// PRIVATE MEMBER VARIABLES
private:
  static const int imax = 50;    /** Maximum number of foci. */

  // basic list of focus items and status
  jhcAliaChain *focus[imax];
  int done[imax], mark[imax];
  int fill, chock;

  // importance for each item
  double wt[imax];
  int boost[imax];

  // timing for each item
  UL32 active[imax];
  UL32 now;                  

  // which focus has been selected
  int svc;

  // part for new NOTE focus
  jhcAliaChain *ch0;
  jhcAliaDir *dir0;


// PUBLIC MEMBER VARIABLES
public:
  jhcNetNode *self;             // fixed node representing the robot
  jhcNetNode *user;             // node for current person communicating

  int noisy;                    // controls diagnostic messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaAttn ();
  jhcAliaAttn ();
  int MaxFoci () const  {return imax;}
  UL32 TimeMS () const  {return now;}
  int NumFoci () const  {return fill;}
  int Inactive () const {return(fill - Active());}
  int Active () const;
 
  // list manipulation
  int NextFocus ();
  jhcAliaChain *FocusN (int n) const;
  bool NeverRun (int n) const;
  int BaseBid (int n) const;
  void SetActive (int n, int running =1);
  int ServiceWt (double pref);

  // list modification
  int ClrFoci (int init =0, const char *rname =NULL);
  jhcNetNode *ShiftUser (const char *name =NULL);
  jhcNetNode *SetUser (jhcNetNode *n);
  int AddFocus (jhcAliaChain *f, double pref =1.0);
  int ReifyRules (jhcBindings& b);

  // external interface (jhcAliaNote virtuals)
  void StartNote ();
  jhcAliaDesc *NewNode (const char *kind, const char *word =NULL, int neg =0, double blf =1.0)
    {return MakeNode(kind, word, neg, blf);}
  jhcAliaDesc *NewProp (jhcAliaDesc *head, const char *role, const char *word,
                        int neg =0, double blf =1.0, const char *kind =NULL)
    {return AddProp(dynamic_cast<jhcNetNode *>(head), role, word, neg, blf, kind);}
  void AddArg (jhcAliaDesc *head, const char *slot, jhcAliaDesc *val)
    {jhcNetNode *hd = dynamic_cast<jhcNetNode *>(head); hd->AddArg(slot, dynamic_cast<jhcNetNode *>(val));}
  void NewLex (jhcAliaDesc *head, const char *word, int neg =0, double blf =1.0) 
    {AddLex(dynamic_cast<jhcNetNode *>(head), word, neg, blf);}
  jhcAliaDesc *Person (const char *name) const {return FindName(name);}
  jhcAliaDesc *Self () const {return self;}
  jhcAliaDesc *User () const {return user;}
  int FinishNote (int keep =1);

  // maintenance
  int Update (int gc =1);

  // file functions
  int LoadFoci (const char *fname, int add =0);
  int SaveFoci (const char *fname);
  int SaveFoci (FILE *out);
  int PrintFoci () {return SaveFoci(stdout);}
    

// PRIVATE MEMBER FUNCTIONS
private:
  // list modification
  void set_prons (int tru);

  // maintenance
  int prune_foci ();
  void rem_compact (int n);
  int fluent_scan (int dbg);

  // garbage collection
  int clean_mem ();
  void keep_from (jhcNetNode *anchor, int dbg);
  int rem_unmarked (int dbg);


};


#endif  // once




