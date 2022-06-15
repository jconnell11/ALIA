// jhcNodePool.h : singly-linked list of semantic network nodes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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

#ifndef _JHCNODEPOOL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNODEPOOL_
/* CPPDOC_END_EXCLUDE */

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
  jhcNetNode *pool;
  jhcNetNode **bucket;
  int *pop;
  int dn, psz, label, ncnt, nbins;
  int rnum, arg, add, del, mod;

  // translation while loading
  jhcNetNode **trans;
  char **surf;
  int tmax, nt;


// PROTECTED MEMBER VARIABLES
protected:
  // version useful for FIND directive
  int vis0, ver; 


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcNodePool ();
  jhcNodePool (); 
  void MakeBins ();
  void NegID () {dn = 1;}
  int NodeMax () const   {return psz;}
  int LastLabel () const {return label;}
  int Version () const   {return ver;}
  int VisDef () const    {return vis0;}
  int BinCnt (int bin) const;

  // list functions
  void PurgeAll ();
  jhcNetNode *Pool (int bin =-1) const;
  jhcNetNode *Next (const jhcNetNode *ref, int bin =-1) const;
  int NodeCnt () const;
  int Changes ();
  void Dirty (int cnt =1) {mod += cnt;}

  // main functions
  jhcGraphlet *BuildIn (jhcGraphlet *g) {jhcGraphlet *old = acc; acc = g; return old;}
  jhcGraphlet *Accum () const {return acc;}
  int Assert (const jhcGraphlet& pat, jhcBindings& b, double conf =1.0, 
              int tval =0, const jhcNodeList *univ =NULL);
  jhcNetNode *SetGen (jhcNetNode *n, int v =0) const;
  jhcNetNode *MarkRef (jhcNetNode *n);
  int Refresh (jhcNetNode *n);
  void Refresh (const jhcGraphlet& gr);

  // basic construction
  jhcNetNode *MakeNode (const char *kind, const char *word =NULL, int neg =0, double def =1.0, int done =0);
  jhcNetNode *AddProp (jhcNetNode *head, const char *role, const char *word,
                       int neg =0, double def =1.0, int chk =0, int args =1);
  jhcNetNode *AddDeg (jhcNetNode *head, const char *role, const char *word, const char *amt, 
                      int neg =0, double def =1.0, int chk =0, int args =1);
  void SetLex (jhcNetNode *n, const char *txt);
  bool InPool (const jhcNetNode *n) const {return InList(n);}
  void MarkBelief (jhcNetNode *n, double blf) const 
    {if (n == NULL) return; n->SetBelief(blf); n->GenMax(ver);}

  // searching
  jhcNetNode *FindName (const char *full) const;
  jhcNetNode *FindNode (const char *desc, int make =0, int omit =0);
  jhcNetNode *Wash (const jhcNetNode *ref) const;

  // list access (overrides virtual)
  virtual jhcNetNode *NextNode (const jhcNetNode *prev =NULL, int bin =-1) const
    {return((prev == NULL) ? Pool(bin) : Next(prev, bin));}
  virtual int Length () const {return NodeCnt();}
  virtual bool InList (const jhcNetNode *n) const;
  virtual int NumBins () const {return nbins;}
  virtual int SameBin (const jhcNetNode& focus, const jhcBindings *b) const 
    {return((b != NULL) ? BinCnt(b->LexBin(focus)) : focus.Code());}
    
  // writing functions
  int Save (const char *fname, int lvl =0) const;
  int Print (int lvl =0) const
    {return save_nodes(stdout, lvl);}
  int SaveBin (const char *fname, int bin =-1, int lvl =0) const;
  int PrintBin (int bin =-1, int lvl =0) const
    {return save_bins(stdout, bin, lvl);}

  // reading functions
  void ClrTrans (int n =100);
  int Load (const char *fname, int add =0);
  int Load (jhcTxtLine& in, int tru =0);
  int LoadGraph (jhcGraphlet *g, jhcTxtLine& in, int tru =0);


// PROTECTED MEMBER FUNCTIONS
protected:
  // list editing
  int RemNode (jhcNetNode *n);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void init_pool ();

  // main functions
  jhcNetNode *lookup_make (jhcNetNode *n, jhcBindings& b, const jhcNodeList *univ); 

  // basic construction
  jhcNetNode *create_node (const char *kind, int id, int chk, int omit);
  int parse_name (char *kind, const char *desc, int ssz);
  const char *extract_kind (char *kind, const char *desc, int ssz);
  void update_lex (jhcNetNode *n, const char *wd);

  // writing functions
  int save_nodes (FILE *out, int lvl) const;
  int save_bins (FILE *out, int bin, int lvl) const;

  // reading functions
  jhcNetNode *chk_topic (jhcNetNode *topic, jhcTxtLine& in, int tru);
  const char *link_name (char *arrow) const;
  int parse_arg (jhcNetNode *n,  const char *slot, jhcTxtLine& in, int tru);
  int get_str (jhcNetNode *item, jhcTxtLine& in);
  int get_lex (jhcNetNode *item, jhcTxtLine& in);
  int get_tags (UL32& tags, jhcTxtLine& in) const;
  jhcNetNode *find_trans (const char *name, int tru);


};


#endif  // once




