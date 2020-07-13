// jhcNetNode.h : node in semantic network for ALIA system
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

#ifndef _JHCNETNODE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNETNODE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>           // needed for FILE and stdout
#include <string.h>

#include "Semantic/jhcAliaDesc.h"


//= Node in semantic network for ALIA system.
// holds pointers to arguments and properties of an item
// essentially specialized version of jhcTripleNode and jhcTripleLink
// instances should only be created and deleted within a jhcNodePool
// terms associated with a predicate ("lex") now treated as properties
// "evt" field: 0 = state, 1 = completed event ("neg" tells success/failure)
// "inv" field: 0 = positive assertion (success), 1 = negative assertion (failed)

class jhcNetNode : public jhcAliaDesc
{
// only way to create and delete (mostly dtor and next)
friend class jhcNodePool;             

// PRIVATE MEMBER VARIABLES
private:
  static const int amax = 10;        /** Maximum arguments for a node.  */
  static const int pmax = 100;       /** Maximum properties for a node. */
  
  // utility
  char nick[40];

  // type or word, negation, and belief
  char base[40];
  char *quote;
  double blf, blf0;
  int inv, evt;

  // structural data
  char links[amax][20]; 
  int anum[pmax];      
  jhcNetNode *args[amax];    
  jhcNetNode *props[pmax]; 
  int na, np;

  // membership and id (largely for jhcNodePool)
  jhcNetNode *next;  
  int id, gen, ref;


// PUBLIC MEMBER VARIABLES
public:
  // status and grammatical tags
  int pod, top, keep, mark;
  UL32 tags;


// PUBLIC MEMBER FUNCTIONS
public:
  // basic information
  const char *Kind () const    {return base;}
  const char *Nick () const    {return nick;}
  const char *Literal () const {return quote;}
  int Inst () const            {return id;}
  bool Hyp () const            {return(blf <= 0.0);}
  int Generation () const      {return gen;}
  int LastRef () const         {return ref;}
  void TopMax (int tval)       {if (!ObjNode() && (top < tval)) top = tval;}

  // negation and belief and literals
  int Neg () const          {return inv;}
  int Done () const         {return evt;}
  double Belief () const    {return blf;}
  double Default () const   {return blf0;}
  void SetNeg (int val =1)  {inv = val;}
  void SetDone (int val =1) {evt = val;}
  void SetBelief (double val =1.0)  {blf = val;}
  void SetDefault (double val =1.0) {blf0 = val;}
  void SetString (const char *wds);
  bool NounTag () const;
  bool VerbTag () const;
  int Actualize (int ver);

  // argument functions
  int NumArgs () const      {return na;}
  bool ArgsFull () const    {return(na >= amax);}
  bool ObjNode () const     {return(na <= 0);}
  const char *Slot (int i =0) const
    {return(((i < 0) || (i >= na)) ? NULL : links[i]);}
  jhcNetNode *Arg (int i =0) const 
    {return(((i < 0) || (i >= na)) ? NULL : args[i]);}
  int NumVals (const char *slot) const;
  jhcNetNode *Val (const char *slot, int i =0) const;
  bool HasVal (const char *slot, const jhcNetNode *val) const;
  bool SameArgs (const jhcNetNode *ref) const;
  int AddArg (const char *slot, jhcNetNode *val);

  // property functions
  int NumProps () const     {return np;}
  bool PropsFull () const   {return(np >= pmax);}
  int NonLexCnt () const    {return(np - NumWords());} 
  const char *Role (int i =0) const
    {return(((i < 0) || (i >= np)) ? NULL : props[i]->Slot(anum[i]));}
  bool RoleMatch (int i, const char *link) const
    {return((link != NULL) && (strcmp(Role(i), link) == 0));}
  bool RoleIn (int i, const char *v1, const char *v2 =NULL, const char *v3 =NULL, 
              const char *v4 =NULL, const char *v5 =NULL, const char *v6 =NULL) const;
  jhcNetNode *NonLex (int i =0) const;
  jhcNetNode *Prop (int i =0) const
    {return(((i < 0) || (i >= np)) ? NULL : props[i]);}   
  int NumFacts (const char *role) const;
  jhcNetNode *Fact (const char *role, int i =0) const;
  bool HasFact (const jhcNetNode *prop, const char *role) const;

  // associated word functions
  bool LexNode () const;
  bool LexMatch (const jhcNetNode *ref) const;
  bool LexConflict (const jhcNetNode *ref) const;
  const char *LexBase (int i) const;
  int NumWords () const;
  const char *Word (int i =0, double bth =0.0) const;
  const char *Tag () const;  
  bool Blank () const {return(Word(0) == NULL);}
  bool HasWord (const char *word, int tru_only =0) const;
  bool WordIn (const char *v1, const char *v2 =NULL, const char *v3 =NULL, 
              const char *v4 =NULL, const char *v5 =NULL, const char *v6 =NULL) const;
  bool SameWords (const jhcNetNode *ref) const;
  bool SharedWord (const jhcNetNode *ref) const;

  // writing functions
  void NodeSize (int& k, int& n, int bind =0) const;
  void TxtSizes (int& k, int& n, int& r) const;
  int Save (FILE *out, int lvl =0, int k =0, int n =0, int r =0, const class jhcGraphlet *acc =NULL, int detail =0) const;
  int Print (int lvl =0, int k =0, int n =0, int r =0, const class jhcGraphlet *acc =NULL, int detail =0) const
    {return Save(stdout, lvl, k, n, r, acc, detail);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  ~jhcNetNode ();
  jhcNetNode ();
  void rem_prop (const jhcNetNode *item);
  void rem_arg (const jhcNetNode *item);

  // writing functions
  int save_tags (FILE *out, int lvl, int r, const jhcGraphlet *acc, int detail) const;
  int naked_lex (FILE *out, int lvl, int k, int n, int r, const jhcGraphlet *acc, int detail) const;


};


#endif  // once




