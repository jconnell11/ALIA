// jhcNetNode.h : node in semantic network for ALIA system
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

#ifndef _JHCNETNODE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNETNODE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>           // needed for FILE and stdout
#include <string.h>
#include <math.h>

#include "Semantic/jhcAliaDesc.h"


//= Node in semantic network for ALIA system.
// holds pointers to arguments and properties of an item
// instances should only be created and deleted within a jhcNodePool
// "evt" field: 0 = state, 1 = completed event ("neg" tells success/failure)
// "inv" field: 0 = positive assertion (success), 1 = negative assertion (failed)
// "blf" field: pos = valid belief, 0 = hypothetical, neg = hidden

class jhcNetNode : public jhcAliaDesc
{
// only way to create and delete (mostly dtor and next)
friend class jhcNodePool;             

// PRIVATE MEMBER VARIABLES
private:
  static const int amax = 10;        /** Maximum arguments for a node.  */
  static const int pmax = 100;       /** Maximum properties for a node. */
  
  // utility
  char nick[20];

  // type or word, negation, and belief
  char base[20], lex[40];
  char *quote;
  double blf, blf0;
  int inv, evt;

  // structural data
  char links[amax][20]; 
  int anum[pmax];      
  jhcNetNode *args[amax];    
  jhcNetNode *props[pmax]; 
  int na, np, na0;

  // membership and id (largely for jhcNodePool)
  jhcNetNode *next;  
  int id, hash, gen, ref, vis;


// PUBLIC MEMBER VARIABLES
public:
  // status and grammatical tags
  int top, keep, mark;
  UL32 tags;

  // source of halo inference
  class jhcAliaRule *hrule;
  class jhcBindings *hbind;


// PUBLIC MEMBER FUNCTIONS
public:
  // basic information
  const char *Kind () const     {return base;}
  const char *Lex () const      {return((*lex == '\0') ? NULL : lex);}
  const char *Nick () const     {return nick;}
  const char *Literal () const  {return quote;}
  int Inst () const             {return id;}
  bool Halo () const            {return(id < 0);}
  bool Hyp () const             {return(blf <= 0.0);}
  bool Visible () const         {return(vis > 0);}
  int Actual () const           {return((blf > 0.0) ? 1 : -1);}
  int Code () const             {return hash;}
  bool String () const          {return(quote != NULL);}
  int Generation () const       {return gen;}
  int LastRef () const          {return((vis > 0) ? ref : 0);}
  void XferRef (jhcNetNode *n)  {if (n != NULL) {ref = n->ref; n->ref = 0;}}
  void TopMax (int tval)        {top = __max(top, tval);}
  void SetKind (const char *k)  {strcpy_s(base, k);}
  void GenMax (int ver)         {if (ver > 0) gen = __max(gen, ver);}
  void Reveal ()                {vis = 1;}

  // negation and belief and literals
  int Neg () const          {return inv;}
  int Done () const         {return evt;}
  double Belief () const    {return blf;}
  double Default () const   {return blf0;}
  double Blf (double bth =1.0) const
    {return((bth > 0.0) ? blf : blf0);}
  bool Sure (double bth) const 
    {return((bth > 0.0) ? (blf >= bth) : (blf0 >= -bth));}
  void SetNeg (int val =1)  {inv = val;}
  void SetDone (int val =1) {evt = val;}
  void SetBelief (double val =1.0)  {blf = val; blf0 = val;}
  void SetDefault (double val =1.0) {blf0 = val;}
  void TmpBelief (double val =0.0)  {blf = val;}
  void Suppress () {blf = -fabs(blf);}
  void SetString (const char *wds);
  bool NounTag () const;
  bool VerbTag () const;
  int Actualize (int ver);

  // argument functions
  int NumArgs () const      {return na;}
  int Arity () const        {return na0;}
  bool ArgsFull () const    {return(na >= amax);}
  bool ObjNode () const     {return(na <= 0);}
  const char *Slot (int i =0) const
    {return(((i < 0) || (i >= na)) ? NULL : links[i]);}
  jhcNetNode *Arg (int i =0) const 
    {return(((i < 0) || (i >= na)) ? NULL : args[i]);}
  int NumVals (const char *slot) const;
  jhcNetNode *Val (const char *slot, int i =0) const;
  int AddArg (const char *slot, jhcNetNode *val);
  void SubstArg (int i, jhcNetNode *val);
//  void RefreshArg (int i);

  // property functions
  int NumProps () const   {return np;}
  bool PropsFull () const {return(np >= pmax);}
  const char *Role (int i =0) const
    {return(((i < 0) || (i >= np)) ? NULL : props[i]->Slot(anum[i]));}
  bool RoleMatch (int i, const char *link) const
    {return((link != NULL) && (strcmp(Role(i), link) == 0));}
  bool RoleIn (int i, const char *v1, const char *v2 =NULL, const char *v3 =NULL, 
              const char *v4 =NULL, const char *v5 =NULL, const char *v6 =NULL) const;
  jhcNetNode *Prop (int i =0) const
    {return(((i < 0) || (i >= np)) ? NULL : props[i]);}   
  jhcNetNode *PropMatch (int i, const char *role, double bth =0.0, int neg =0) const;
  int NumFacts (const char *role) const;
  jhcNetNode *Fact (const char *role, int n =0) const;

  // simple matching
  bool HasVal (const char *slot, const jhcNetNode *val) const;
  bool HasFact (const jhcNetNode *prop, const char *role) const;
  bool SameArgs (const jhcNetNode *ref) const;
  bool SameArgs (const jhcNetNode *ref, const class jhcBindings *b) const;
  jhcNetNode *FindProp (const char *role, const char *word, int neg, double bth =0.5) const;

  // predicate terms and reference names
  bool LexMatch (const char *txt) const 
    {return((txt != NULL) && (strcmp(lex, txt) == 0));}
  bool LexMatch (const jhcNetNode *n) const 
    {return((n != NULL) && LexMatch(n->lex));}
  bool LexIn (const char *v1, const char *v2 =NULL, const char *v3 =NULL, 
              const char *v4 =NULL, const char *v5 =NULL, const char *v6 =NULL) const;
  const char *Tag () const 
    {return((*lex != '\0') ? lex : nick);}
  const char *Name (int i =0, double bth =0.0) const;
  bool HasName (const char *word, int tru_only =0) const;
    
  // writing functions
  void NodeSize (int& k, int& n) const;
  void TxtSizes (int& k, int& n, int& r) const;
  int Save (FILE *out, int lvl =0, int k =0, int n =0, int r =0, int detail =1, const class jhcGraphlet *acc =NULL) const;
  int Print (int lvl =0, int k =0, int n =0, int r =0, int detail =1) const
    {return Save(stdout, lvl, k, n, r, detail);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  ~jhcNetNode ();
  jhcNetNode ();
  void rem_prop (const jhcNetNode *item);
  void rem_arg (const jhcNetNode *item);

  // writing functions
  int save_tags (FILE *out, int lvl, int r, int detail) const;
  const char *bfmt (char *txt, double val) const;


};


#endif  // once




