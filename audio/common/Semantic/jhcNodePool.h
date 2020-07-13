// jhcNodePool.h : singly-linked list of semantic network nodes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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
  int rnum, arg, add, del;

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
  int NodeCnt () const {return psz;}
  int LastLabel () const {return label;}
  int Version () const {return ver;}

  // list functions
  void PurgeAll ();
  jhcNetNode *Pool () const {return pool;}
  jhcNetNode *Next (const jhcNetNode *ref) const 
    {return((ref == NULL) ? pool : ref->next);}
  bool Changed ();

  // main functions
  void BuildIn (jhcGraphlet *g) {acc = g;}
  int Assert (const jhcGraphlet& pat, jhcBindings& b, double conf =1.0, int src =0, int tval =0);
  jhcNetNode *MarkRef (jhcNetNode *n)
    {if (n != NULL) n->ref = ++rnum; return n;}

  // basic construction
  jhcNetNode *MakeNode (const char *kind, const char *word =NULL, int neg =0, double def =1.0);
  jhcNetNode *AddProp (jhcNetNode *head, const char *role, const char *word,
                       int neg =0, double def =1.0, const char *kind =NULL);
  jhcNetNode *AddLex (jhcNetNode *head, const char *word, int neg =0, double blf =1.0);
  bool InPool (const jhcNetNode *n) const {return InList(n);}
  bool Recent (const jhcNetNode *n) const 
    {return((n != NULL) && (n->gen == ver));}
  void MarkBelief (jhcNetNode *n, double blf) const 
    {if (n == NULL) return; n->SetBelief(blf); n->gen = ver;}

  // searching
  jhcNetNode *FindName (const char *full) const;
  jhcNetNode *FindNode (const char *desc, int make =0);

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
  int Load (jhcTxtLine& in);
  int LoadGraph (jhcGraphlet *g, jhcTxtLine& in);


// PROTECTED MEMBER FUNCTIONS
protected:
  // list editing
  int RemNode (jhcNetNode *n);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void init_pool ();

  // main functions
  jhcNetNode *lookup_make (const jhcNetNode *n, jhcBindings& b, int src); 

  // basic construction
  jhcNetNode *create_node (const char *kind, int id);
  int parse_name (char *kind, const char *desc, int ssz);
  const char *extract_kind (char *kind, const char *desc, int ssz);

  // writing functions
  int save_nodes (FILE *out, int lvl) const;

  // reading functions
  jhcNetNode *chk_topic (jhcNetNode *topic, jhcTxtLine& in);
  const char *link_name (char *arrow) const;
  int parse_arg (jhcNetNode *n,  const char *slot, jhcTxtLine& in);
  int get_str (jhcNetNode *item, jhcTxtLine& in);
  int get_lex (jhcNetNode *item, jhcTxtLine& in);
  int get_tags (UL32& tags, jhcTxtLine& in) const;
  jhcNetNode *find_trans (const char *name);


};


#endif  // once




