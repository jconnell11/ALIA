// jhcBindings.h : list of substitutions of one node for another
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2018 IBM Corporation
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

#ifndef _JHCBINDINGS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBINDINGS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Semantic/jhcNetNode.h"       // common audio
#include "Semantic/jhcNodeList.h"


//= List of substitutions of one node for another.
// this is an ordered list with push and pop functions

class jhcBindings
{
// PRIVATE MEMBER VARIABLES
private:
  static const int bmax = 20;          /** Maximum number of bindings. */

  // match keys and substitutions (now on heap)
  const jhcNetNode **key;
  jhcNetNode **sub;
  int nb;


// PUBLIC MEMBER VARIABLES
public:
  int expect;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcBindings ();
  jhcBindings (const jhcBindings *ref =NULL);
  void Clear () {nb = 0;}
  jhcBindings *Copy (const jhcBindings& ref);
  void Copy (const jhcBindings *ref) 
    {if (ref != NULL) Copy(*ref);}
  bool Complete () const 
    {return((expect > 0) && (nb >= expect));}
  bool Empty () const {return(nb <= 0);}

  // main functions
  jhcNetNode *LookUp (const jhcNetNode *k) const;
  const jhcNetNode *FindKey (const jhcNetNode *subst) const;
  int Bind (const jhcNetNode *k, jhcNetNode *subst);
  int TrimTo (int n);
  int Pop () {return TrimTo(nb - 1);}
  void RemFinal (jhcNetNode *key);

  // list functions
  int NumPairs () const {return nb;} 
  const jhcNetNode *GetKey (int i) const 
    {return(((i >= 0) && (i < nb)) ? key[i] : NULL);}
  jhcNetNode *GetSub (int i) const 
    {return(((i >= 0) && (i < nb)) ? sub[i] : NULL);}
  void SetSub (int i, jhcNetNode *n)
    {if ((i >= 0) && (i < nb)) sub[i] = n;}
  bool InKeys (const jhcNetNode *k) const;
  bool InSubs (const jhcNetNode *sub) const;
  int KeyMiss (const jhcNodeList& f) const;
  int SubstMiss (const jhcNodeList& f) const;

  // bulk functions
  bool Same (const jhcBindings& ref) const;
  void ReplaceSubs (const jhcBindings& alt);
  void CopyReplace (const jhcBindings& ref, const jhcBindings& alt)
    {Copy(ref); ReplaceSubs(alt);}
  void Print (int lvl =0, const char *prefix =NULL, int num =0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // list functions
  int index (const jhcNetNode *probe) const;


};


#endif  // once




