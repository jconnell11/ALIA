// jhcNetNode.h : node in semantic network for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#ifndef _JHCNETNODE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNETNODE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>                     // needed for FILE and stdout
#include <string.h>
#include <math.h>

#include "Semantic/jhcAliaDesc.h"


//= Node in semantic network for ALIA system.
// holds pointers to arguments and properties of an item
// instances should only be created and deleted within a jhcNodePool!
// can also be tethered to some deeper node (e.g. WMEM to long-term DMEM)
//   "evt" field: 0 = state, 1 = completed event ("inv" tells success/failure)
//   "inv" field: 0 = positive assertion (success), 1 = negative assertion (failed)
//   "blf" field: pos = valid belief, 0 = hypothetical, neg = suppressed
//   "vis" field: 0 = hidden, 1 = eligible for matching 
//   "ref" field: last language I/O of node for pronouns (bigger = more recent)
//   "gen" field: cycle when node was added (use only new for CHK subset of FIND)
//   "top" field: which action focus (if any) node is associated with
//   "ltm" field: dependent on long term memory ghost fact in halo (1 = yes)
//  "keep" field: preserve node during garbage collection (1 = spread from this) 
// NOTE: probably should convert blf/blf0 to blf + "hyp" flag (cleaner)

class jhcNetNode : public jhcAliaDesc
{
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
  int na, np, na0, wrt;

  // long term memory linkage
  jhcNetNode *moor, *buoy;
  class jhcNodePool *home;

  // membership and id (largely for jhcNodePool)
  jhcNetNode *next;  
  int id, hash, gen, ref, vis, keep;


// PUBLIC MEMBER VARIABLES
public:
  // status and grammatical tags
  int top, mark, ltm;
  UL32 tags;

  // source of halo inference
  class jhcAliaRule *hrule;
  class jhcBindings *hbind;


// PUBLIC MEMBER FUNCTIONS
public:
  // special access for jhcNodePool only!
  ~jhcNetNode ();
  jhcNetNode (int num, class jhcNodePool *pool);
  jhcNetNode *NodeTail () const {return next;}
  void SetTail (jhcNetNode *n)  {next = n;}
  void SetConvo (int val)       {ref = val;}  
  void SetHash (int h)          {hash = h;}
  void SetWord (const char *wd) {strcpy_s(lex, wd);}

  // reading basic information
  const char *Kind () const    {return base;}              // copied from moor (static)
  const char *LexStr () const  {return lex;}               // copied from moor (static)
  const char *Nick () const    {return nick;}
  int Inst () const            {return id;}
  int Code () const            {return hash;}
  int Generation () const      {return gen;}
  const char *Literal () const {return((moor != NULL) ? moor->quote : quote);}

  // derived from basic information
  const char *Lex () const {return((*lex == '\0') ? NULL : lex);}
  int LexVar () const      {return((*lex == '*') ? 1 : 0);}
  bool Halo () const       {return(id < 0);}
  bool Visible () const    {return(vis > 0);}
  int LastConvo () const   {return ref;}
  bool String () const     {return(Literal() != NULL);}

  // reading negation and belief, etc.
  int Neg () const        {return inv;}                    // copied from moor
  int Done () const       {return evt;}                    // copied from moor
  double Default () const {return blf0;}                   // copied from moor
  double Belief () const  {return blf;}
  bool NounTag () const;
  bool VerbTag () const;

  // derived from negation and belief, etc.
  bool DefHyp () const {return(blf0 <= 0.0);}
  bool Hyp () const    {return(blf <= 0.0);}
  int Actual () const  {return((blf > 0.0) ? 1 : -1);}
  double Blf (double bth =1.0) const
    {return((bth > 0.0) ? blf : blf0);}
  bool Sure (double bth) const 
    {return((bth > 0.0) ? (blf >= bth) : (blf0 >= -bth));}

  // altering basic information 
  void Reveal (int doit =1) {vis = doit;}
  void TopMax (int tval)    {top = __max(top, tval);}
  void GenMax (int ver)     {if (ver > 0) gen = __max(gen, ver);}
  void MarkConvo ();
  void XferConvo (jhcNetNode *n) 
    {if ((n != NULL) && (n != this)) {ref = n->ref; n->ref = 0;}}
  void SetKind (const char *k, char sep ='\0');
  void SetString (const char *txt);

  // altering negation and belief, etc.
  void SetNeg (int val =1)  {inv = val;}
  void SetDone (int val =1) {evt = val;}
  void SetBelief (double val =1.0)  {blf = val; blf0 = val;}
  void SetDefault (double val =1.0) {blf0 = val;}
  void TmpBelief (double val =0.0)  {blf = val;}
  void Suppress ()                                         // can change moor
    {blf = -fabs(blf); if (moor != NULL) moor->blf = -fabs(moor->blf);}
  int Actualize (int ver);

  // argument functions
  int NumArgs () const   {return((moor != NULL) ? moor->na : na);}
  bool ArgsFull () const {return(NumArgs() >= amax);}
  bool ObjNode () const  {return(NumArgs() <= 0);}
  int Arity (int all =1) const;
  bool HypAny () const;
  jhcNetNode *Arg (int i =0) const;
  jhcNetNode *ArgSurf (int i =0) const {return Arg(i)->Surf();}
  const char *Slot (int i =0) const;
  bool SlotMatch (int i, const char *link) const
    {return((link != NULL) && (strcmp(Slot(i), link) == 0));}
  int NumVals (const char *slot) const;
  jhcNetNode *Val (const char *slot, int i =0) const;
  int AddArg (const char *slot, jhcNetNode *val);
  void SubstArg (int i, jhcNetNode *val);
  void RefreshArg (int i);

  // property functions
  int NumProps () const;
  bool PropsFull () const {return(np >= pmax);}            // only for surface nodes
  bool Naked () const     {return(NumProps() <= 0);}
  jhcNetNode *Prop (int i =0) const;
  jhcNetNode *PropSurf (int i =0) const {return Prop(i)->Surf();}
  const char *Role (int i =0) const;
  bool RoleMatch (int i, const char *link) const
    {return((link != NULL) && (strcmp(Role(i), link) == 0));}
  bool RoleIn (int i, const char *v1, const char *v2 =NULL, const char *v3 =NULL, 
              const char *v4 =NULL, const char *v5 =NULL, const char *v6 =NULL) const;
  jhcNetNode *PropMatch (int i, const char *role, double bth =0.0, int neg =0) const;
  int NumFacts (const char *role) const;
  jhcNetNode *Fact (const char *role, int n =0) const;

  // long term memory linkage
  jhcNetNode *Buoy () const {return buoy;}
  jhcNetNode *Moor () const {return moor;}
  jhcNetNode *Surf () {return((buoy != NULL) ? buoy : this);}
  jhcNetNode *Deep () {return((moor != NULL) ? moor : this);}
  bool Buoyed () const {return(buoy != NULL);}
  bool Moored () const {return(moor != NULL);}
  void MoorTo (jhcNetNode *deep);  
  bool Home (const class jhcNodePool *p) const {return(home == p);}
  void SetKeep (int val =1);
  int Keep () const;

  // simple matching
  bool HasVal (const char *slot, const jhcNetNode *val) const;
  bool HasFact (const jhcNetNode *fact, const char *role) const;
  bool SameArgs (const jhcNetNode *ref) const;
  bool SameArgs (const jhcNetNode *ref, const class jhcBindings *b) const;
  jhcNetNode *FindProp (const char *role, const char *word, int neg, double bth =0.5) const;
  jhcNetNode *FindArg (const char *slot, const char *word, int neg, double bth =0.5) const;
  bool HasSlot (const char *slot) const;
  bool AnySlot (const char *v1, const char *v2 =NULL, const char *v3 =NULL, 
                const char *v4 =NULL, const char *v5 =NULL, const char *v6 =NULL) const;
  bool SameSlots (const jhcNetNode *ref) const;  

  // predicate terms and reference names
  bool LexMatch (const char *txt) const 
    {return((txt != NULL) && (strcmp(lex, txt) == 0));}
  bool LexMatch (const jhcNetNode *n) const 
    {return((n != NULL) && LexMatch(n->lex));}
  bool LexIn (const char *v1, const char *v2 =NULL, const char *v3 =NULL, 
              const char *v4 =NULL, const char *v5 =NULL, const char *v6 =NULL) const;
  bool LexSame (const jhcNetNode *n) const
    {return((n != NULL) && (strcmp(lex, n->lex) == 0));}
  const char *Tag () const 
    {return((*lex != '\0') ? lex : nick);}
  const char *Name (int i =0, double bth =0.5) const;
  bool HasName (const char *word, int tru_only =0) const;
  const char *Label () const
    {const char *ans = Name(); if (ans != NULL) return ans; return Nick();}
    
  // writing functions
  void NodeSize (int& k, int& n) const;
  void TxtSizes (int& k, int& n, int& r) const;
  int Save (FILE *out, int lvl =0, int k =0, int n =0, int r =0, int detail =1, const class jhcGraphlet *acc =NULL) const;
  int Print (int lvl =0, int k =0, int n =0, int r =0, int detail =1) const
    {return Save(stdout, lvl, k, n, r, detail);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void rem_prop (const jhcNetNode *item);
  void rem_arg (const jhcNetNode *item);

  // argument functions
  const char *rem_num (char *trim, const char *sname, int ssz) const;

  // writing functions
  int save_tags (FILE *out, int lvl, int r, int detail) const;
  const char *bfmt (char *txt, double val) const;


};


#endif  // once




