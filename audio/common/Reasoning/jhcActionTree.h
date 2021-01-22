// jhcActionTree.h : holds attentional foci for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#ifndef _JHCACTIONTREE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCACTIONTREE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>                     // needed for FILE

#include "Action/jhcAliaChain.h"       // common robot

#include "Reasoning/jhcAliaNote.h"      
#include "Reasoning/jhcWorkMem.h"      
#include "Semantic/jhcBindings.h"


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

class jhcActionTree : public jhcWorkMem, public jhcAliaNote
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
  class jhcAliaDir *dir0;

  // mood variables
  double surp;     


// PUBLIC MEMBER VARIABLES
public:
  // surprise parameters
  double drill, dwell, calm;  

  // rule adjustment parameters
  double binc, bdec;    
            
  // control of diagnostic messages
  int noisy;                    


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcActionTree ();
  jhcActionTree ();
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
  int AddFocus (jhcAliaChain *f, double pref =1.0);

  // maintenance
  int Update (int gc =1);

  // halo interaction
  double Surprise () const {return surp;}
  double CompareHalo (const jhcGraphlet& key);
  int ReifyRules (jhcBindings& b, int note =1);

  // external interface (jhcAliaNote virtuals)
  void StartNote ();
  jhcAliaDesc *NewNode (const char *kind, const char *word =NULL, int neg =0, double blf =1.0, int done =0)
    {return MakeNode(kind, word, neg, blf, done);}
  jhcAliaDesc *NewProp (jhcAliaDesc *head, const char *role, const char *word,
                        int neg =0, double blf =1.0, const char *kind =NULL)
    {return AddProp(dynamic_cast<jhcNetNode *>(head), role, word, neg, blf, kind);}
  void AddArg (jhcAliaDesc *head, const char *slot, jhcAliaDesc *val)
    {jhcNetNode *hd = dynamic_cast<jhcNetNode *>(head); hd->AddArg(slot, dynamic_cast<jhcNetNode *>(val));}
  void NewLex (jhcAliaDesc *head, const char *word, int neg =0, double blf =1.0) 
    {AddLex(dynamic_cast<jhcNetNode *>(head), word, neg, blf);}
  jhcAliaDesc *Person (const char *name) const {return FindName(name);}
  jhcAliaDesc *Self () const {return Robot();}
  jhcAliaDesc *User () const {return Human();}
  int FinishNote (int keep =1);

  // file functions
  int LoadFoci (const char *fname, int app =0);
  int SaveFoci (const char *fname);
  int SaveFoci (FILE *out);
  int PrintFoci () {return SaveFoci(stdout);}
    

// PRIVATE MEMBER FUNCTIONS
private:
  // list modification
  int drop_oldest ();

  // halo interaction
  const jhcNetNode *halo_equiv (const jhcNetNode *n, const jhcNetNode *h0) const;
  jhcNetNode *pick_halo (const jhcBindings& b, const jhcBindings& h2m, int stop) const;

  // maintenance
  int prune_foci ();
  void rem_compact (int n);


};


#endif  // once




