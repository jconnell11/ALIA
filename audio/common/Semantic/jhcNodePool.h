// jhcNodePool.h : singly-linked list of semantic network nodes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "Parse/jhcTxtLine.h"          // common audio
#include "Semantic/jhcBindings.h"
#include "Semantic/jhcNetNode.h"
#include "Semantic/jhcNodeList.h"


//= Singly-linked list of semantic network nodes.
// most recently allocated nodes at head, numbers from tail
// whole list will be deleted when the pool is deleted
// only class that can read in (and create) graph structure
// generally a base class others are derived from

class jhcNodePool : public jhcNodeList
{
// PRIVATE MEMBER VARIABLES
private:
  class jhcGraphlet *acc;
  jhcNetNode *pool, *last;
  jhcNetNode **bucket, **cap;
  int *pop;
  int dn, psz, label, ncnt, nbins;
  int refnum, ref0, xarg, xadd, xdel, xmod, xltm;

  // translation while loading
  jhcNetNode **trans;
  char **surf;
  int tmax, nt;


// PROTECTED MEMBER VARIABLES
protected:
  // for special node nick names
  char sep0;

  // version useful for FIND directive
  int vis0, ltm0, ver; 


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcNodePool ();
  jhcNodePool (); 
  void MakeBins ();
  void NegID () {dn = 1; sep0 = '+';}
  int NodeMax () const   {return psz;}
  int LastLabel () const {return label;}
  int Version () const   {return ver;}
  int VisDef () const    {return vis0;}
  int BinCnt (int bin) const;
  int LexHash (const char *wd) const;

  // list functions
  void PurgeAll ();
  jhcNetNode *Pool (int bin =-1) const;
  jhcNetNode *Next (const jhcNetNode *ref, int bin =-1) const;
  jhcNetNode *NextPool (const jhcNetNode *ref) const;
  int NodeCnt (int hyp =1) const;
  int Changes ();
  void Dirty (int cnt =1) {xmod += cnt;}

  // main functions
  class jhcGraphlet *BuildIn (class jhcGraphlet& g) {return BuildIn(&g);}
  class jhcGraphlet *BuildIn (class jhcGraphlet *g) {class jhcGraphlet *old = acc; acc = g; return old;}
  class jhcGraphlet *Accum () const {return acc;}
  int Assert (const class jhcGraphlet& pat, jhcBindings& b, double conf =1.0, 
              int tval =0, const jhcNodeList *univ =NULL);
  jhcNetNode *SetGen (jhcNetNode *n, int v =0) const;
  int Refresh (jhcNetNode *n);
  void Refresh (const class jhcGraphlet& gr);
  int IncConvo ()   {return ++refnum;}
  void InitConvo () {ref0 = refnum;}
  int LocalConvo() const {return ref0;}

  // basic construction
  jhcNetNode *MakeNode (const char *kind, const char *word =NULL, int neg =0, double def =1.0, int done =0);
  jhcNetNode *CloneNode (const jhcNetNode& n, int bset =1);
  jhcNetNode *MakeAct (const char *word, int neg =0, double def =1.0, int done =0);
  jhcNetNode *CloneAct (const jhcNetNode *src, int neg =0);
  jhcNetNode *MakePoss (jhcNetNode *owner, jhcNetNode *obj, int neg =0, double def =1.0, int done =0);
  jhcNetNode *AddProp (jhcNetNode *head, const char *role, const char *word,
                       int neg =0, double def =1.0, int chk =0, int args =1);
  jhcNetNode *AddDeg (jhcNetNode *head, const char *role, const char *word, const char *amt, 
                      int neg =0, double def =1.0, int chk =0, int args =1);
  jhcNetNode *BuoyFor (jhcNetNode *deep);
  void SetLex (jhcNetNode *n, const char *txt);
  bool InPool (const jhcNetNode *n) const {return InList(n);}
  void MarkBelief (jhcNetNode *n, double blf) const 
    {if (n == NULL) return; n->SetBelief(blf); n->GenMax(ver);}

  // utilities
  jhcNetNode *FindNode (const char *desc, int make =0, int omit =0);
  jhcNetNode *Wash (const jhcNetNode *ref) const;

  // list access (overrides virtual)
  virtual jhcNetNode *NextNode (const jhcNetNode *prev =NULL, int bin =-1) const
    {return((prev == NULL) ? Pool(bin) : Next(prev, bin));}
  virtual int Length () const {return NodeCnt(1);}
  virtual bool InList (const jhcNetNode *n) const;
  virtual int NumBins () const {return nbins;}
  virtual int SameBin (const jhcNetNode& focus, const jhcBindings *b) const 
    {return((b != NULL) ? BinCnt(b->LexBin(focus)) : focus.Code());}
    
  // writing functions
  int Save (const char *fname, int lvl =0, int hyp =0) const;
  int Print (int lvl =0, int hyp =0) const
    {return sort_nodes(stdout, lvl, 0, hyp);}
  int SaveBin (const char *fname, int bin =-1, int imin =0) const;
  int PrintBin (int bin =-1, int imin =0) const
    {return save_bins(stdout, bin, imin);}

  // reading functions
  void ClrTrans (int n =100);
  int Load (const char *fname, int add =0, int nt =100);
  int Load (jhcTxtLine& in, int tru);
  int LoadGraph (class jhcGraphlet& g, jhcTxtLine& in, int tru =0);


// PROTECTED MEMBER FUNCTIONS
protected:
  // list editing
  int RemNode (jhcNetNode *n);

  // writing functions (used by jhcDeclMem)
  int save_bins (FILE *out, int bin, int imin) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void init_pool ();

  // main functions
  jhcNetNode *lookup_make (jhcNetNode *n, jhcBindings& b, const jhcNodeList *univ); 

  // basic construction
  jhcNetNode *create_node (const char *kind, int id, int chk, int omit, int rev);
  void add_to_list (int h, jhcNetNode *n, int rev);
  int parse_name (char *kind, const char *desc, int ssz);
  const char *extract_kind (char *kind, const char *desc, int ssz);
  void update_lex (jhcNetNode *n, const char *wd, int rev);
  void copy_mods (jhcNetNode *dest, const jhcNetNode *src);

  // list editing
  int rem_from_list (int h0, jhcNetNode *n);

  // writing functions
  int sort_nodes (FILE *out, int lvl, int imin, int hyp) const;

  // reading functions
  jhcNetNode *chk_topic (jhcNetNode *topic, jhcTxtLine& in, int tru);
  const char *link_name (char *arrow) const;
  int parse_arg (jhcNetNode *n,  const char *slot, jhcTxtLine& in, int tru);
  int get_str (jhcNetNode *item, jhcTxtLine& in);
  int get_lex (jhcNetNode *item, jhcTxtLine& in);
  int get_tags (UL32& tags, jhcTxtLine& in) const;
  jhcNetNode *find_trans (const char *name, int tru);


};

