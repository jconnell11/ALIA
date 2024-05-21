// jhcActionTree.h : holds attentional foci for ALIA system
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

#pragma once

#include "jhcGlobal.h"

#include <stdio.h>                     // needed for FILE

#include "Data/jhcParam.h"             // common video

#include "Action/jhcAliaChain.h"       // common robot
#include "Action/jhcAliaMood.h"

#include "Semantic/jhcBindings.h"      // common audio

#include "API/jhcAliaNote.h"           
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

class jhcActionTree : public jhcWorkMem, public jhcAliaNote
{
// PRIVATE MEMBER VARIABLES
private:
  static const int imax = 30;    /** Maximum number of foci. */

  // basic list of focus items and status
  jhcAliaChain *focus[imax];
  jhcGraphlet err[imax];
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

  // unique label for counting goals
  int req;

  // operator selection
  double pess;               // preference threshold (pessimism)
  double wild;               // respect for operator preference


// PRIVATE MEMBER PARAMETERS
private:
  // rule and operator adjustment parameters
  double cinc, cdec, pth0, pinc, pdec, fresh, wsc0;


// PUBLIC MEMBER VARIABLES
public:
  jhcParam aps;

  // surprise parameters
  double drill, dwell, calm;  
            
  // NOTE generation variables
  jhcGraphlet nkey;
  int blame;                 // record specific error messages     


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
  int MaxDepth ();
  int NumGoals (int leaf =0);

  // processing parameter bundles 
  int LoadCfg (const char *fname =NULL);
  int SaveCfg (const char *fname) const;

  // operator selection parameters
  double Wild () const       {return wild;}
  double RestWild () const   {return wsc0;}
  void SetWild (double w)    {wild = __max(0.25, __min(w, 1.0));}
  double MinPref () const    {return pess;}
  double RestPref () const   {return pth0;}
  void SetMinPref (double p) {pess = __max(0.1, __min(p, 1.0));}

  // rule and operator adjustment
  double AdjRuleConf (class jhcAliaRule *rule, double cf) const;
  double AdjOpPref (class jhcAliaOp *op, int up, int show =0) const;

  // list manipulation
  int NextFocus ();
  jhcAliaChain *FocusN (int n) const;
  jhcAliaChain *Current () const {return FocusN(svc);}
  jhcGraphlet *Error ();
  bool NeverRun (int n) const;
  int BaseBid (int n) const;
  void SetActive (const jhcAliaChain *f, int running =1);
  int ServiceWt (double pref);

  // list modification
  void ResetFoci (const char *rname =NULL);
  void ClrFoci ();
  int AddFocus (jhcAliaChain *f, double pref =1.0);
  void NoteSolo (jhcNetNode *n);
  void ClrFail () {if (svc >= 0) err[svc].Clear();}

  // maintenance
  int Update (int gc =1);

  // halo interaction 
  double CompareHalo (const jhcGraphlet& key, jhcAliaMood& mood);
  int ReifyRules (jhcBindings& b, int note =2);

  // execution tracing
  int HaltActive (const jhcGraphlet& desc, const class jhcAliaDir *skip, int bid);
  bool Recent (const jhcGraphlet& desc, int done =1) 
    {return(find_call(NULL, NULL, desc, done) != NULL);}
  class jhcAliaOp *Motive (const jhcGraphlet& desc, const jhcNetNode **step =NULL, jhcBindings *d2o =NULL);
  const class jhcAliaDir *FindFail () const;

  // external interface (jhcAliaNote virtuals)
  void StartNote ();
  void AddNode (jhcAliaDesc *item) 
    {nkey.AddItem(dynamic_cast<jhcNetNode *>(item));}
  jhcAliaDesc *NewObj (const char *kind, const char *word =NULL, double blf =1.0)
    {return MakeNode(kind, word, 0, blf, 0);} 
  jhcAliaDesc *NewAct (const char *verb =NULL, int neg =0, int done =0, double blf =1.0)
    {return MakeAct(verb, neg, blf, done);}
  jhcAliaDesc *NewProp (jhcAliaDesc *head, const char *role, const char *word,
                        int neg =0, double blf =1.0, int chk =0, int args =1)
    {return AddProp(dynamic_cast<jhcNetNode *>(head), role, word, neg, blf, chk, args);}
  jhcAliaDesc *NewDeg (jhcAliaDesc *head, const char *role, const char *word, const char *amt, 
                       int neg =0, double blf =1.0, int chk =0, int args =1)
    {return AddDeg(dynamic_cast<jhcNetNode *>(head), role, word, amt, neg, blf, chk, args);}
  void AddArg (jhcAliaDesc *head, const char *slot, jhcAliaDesc *val) const
    {if (head != NULL) (dynamic_cast<jhcNetNode *>(head))->AddArg(slot, dynamic_cast<jhcNetNode *>(val));}
  jhcAliaDesc *Resolve (jhcAliaDesc *focus);
  void Keep (jhcAliaDesc *obj) const
    {if (obj != NULL) (dynamic_cast<jhcNetNode*>(obj))->SetKeep(1);}
  void NewFound (jhcAliaDesc *obj) const;
  void GramTag (jhcAliaDesc *prop, int t) const 
    {if (prop != NULL) (dynamic_cast<jhcNetNode *>(prop))->tags = t;}
  jhcAliaDesc *Person (const char *name) const 
    {return FindName(name);}
  const char *Name (const jhcAliaDesc *obj, int i =0) const 
    {return((obj == NULL) ? NULL : (dynamic_cast<const jhcNetNode *>(obj))->Name(i, MinBlf()));}
  jhcAliaDesc *Self () const {return Robot();}
  jhcAliaDesc *User () const {return Human();}
  int VisAssoc (int vid, jhcAliaDesc *obj, int kind =0) 
    {return ExtLink(vid, dynamic_cast<jhcNetNode *>(obj), kind);}
  int VisID (const jhcAliaDesc *obj, int kind =0) const 
    {return ExtRef(dynamic_cast<const jhcNetNode *>(obj), kind);}
  jhcAliaDesc *NodeFor (int vid, int kind =0) const     
    {return ExtRef(vid, kind);}
  int VisEnum (int last, int kind =0) const     
    {return ExtEnum(last, kind);}
  int FinishNote (jhcAliaDesc *fail =NULL);

  // file functions
  int LoadFoci (const char *fname, int app =0);
  int SaveFoci (const char *fname);
  int SaveFoci (FILE *out);
  int PrintFoci () {return SaveFoci(stdout);}
    

// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int adj_params (const char *fname);

  // rule and operator adjustment
  double inc_conf (jhcAliaRule *r, double conf0) const;
  double dec_conf (jhcAliaRule *r, double conf0) const;

  // list modification
  int drop_oldest ();

  // halo interaction
  const jhcNetNode *halo_equiv (const jhcNetNode *n, const jhcNetNode *h0) const;
  jhcNetNode *pick_non_wmem (int& step, const jhcBindings& b, const jhcBindings& h2m, int stop) const;
  void promote_all (jhcBindings& h2m, const jhcBindings& b);
  int promote (jhcBindings& h2m, jhcNetNode *n);

  // maintenance
  int prune_foci ();
  void rem_compact (int n);

  // execution tracing
  const class jhcAliaDir *find_call (const class jhcAliaDir **act, jhcBindings *d2a, 
                                     const class jhcGraphlet& desc, int run);
  const jhcAliaDir *play_prob (const jhcAliaPlay *play) const;
  const jhcAliaDir *failed_dir (jhcAliaChain *start) const;


};

