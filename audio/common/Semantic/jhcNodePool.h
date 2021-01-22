// jhcNodePool.h : singly-linked list of semantic network nodes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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
  int dn, psz, label;
  int rnum, arg, add, del, mod;

  // translation while loading
  jhcNetNode **trans;
  char **surf;
  int tmax, nt;


// PROTECTED MEMBER VARIABLES
protected:
  // useful for CHK directive
  int ver; 


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcNodePool ();
  jhcNodePool (); 
  void NegID () {dn = 1;}
  int NodeMax () const   {return psz;}
  int LastLabel () const {return label;}
  int Version () const   {return ver;}

  // list functions
  void PurgeAll ();
  jhcNetNode *Pool () const {return pool;}
  jhcNetNode *Next (const jhcNetNode *ref) const 
    {return((ref == NULL) ? pool : ref->next);}
  int NodeCnt () const;
  int Changes ();
  void Dirty (int cnt =1) {mod += cnt;}

  // main functions
  jhcGraphlet *BuildIn (jhcGraphlet *g) 
    {jhcGraphlet *old = acc; acc = g; return old;}
  jhcGraphlet *Accum () const {return acc;}
  int Assert (const jhcGraphlet& pat, jhcBindings& b, double conf =1.0, 
              int tval =0, const jhcNodeList *univ =NULL);
  jhcNetNode *MarkRef (jhcNetNode *n);

  // basic construction
  jhcNetNode *MakeNode (const char *kind, const char *word =NULL, int neg =0, double def =1.0, int done =0);
  jhcNetNode *AddProp (jhcNetNode *head, const char *role, const char *word,
                       int neg =0, double def =1.0, const char *kind =NULL);
  jhcNetNode *AddLex (jhcNetNode *head, const char *word, int neg =0, double blf =1.0);
  bool InPool (const jhcNetNode *n) const {return InList(n);}
  bool Recent (const jhcNetNode *n, int delay =0) const 
    {return((n != NULL) && ((ver - n->gen) <= delay));}
  void MarkBelief (jhcNetNode *n, double blf) const 
    {if (n == NULL) return; n->SetBelief(blf); n->gen = ver;}

  // searching
  jhcNetNode *FindName (const char *full) const;
  jhcNetNode *FindNode (const char *desc, int make =0);
  jhcNetNode *Wash (const jhcNetNode *ref) const;

  // list access (overrides virtual)
  virtual jhcNetNode *NextNode (const jhcNetNode *prev =NULL) const
    {return((prev == NULL) ? Pool() : Next(prev));}
  virtual int Length () const {return NodeCnt();}
  virtual bool InList (const jhcNetNode *n) const;

  // writing functions
  int Save (const char *fname, int lvl =0) const;
  int Print (int lvl =0) const
    {return save_nodes(stdout, lvl);}

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
  jhcNetNode *create_node (const char *kind, int id);
  int parse_name (char *kind, const char *desc, int ssz);
  const char *extract_kind (char *kind, const char *desc, int ssz);

  // writing functions
  int save_nodes (FILE *out, int lvl) const;

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




