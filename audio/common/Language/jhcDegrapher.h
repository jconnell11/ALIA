// jhcDegrapher.h : generates natural language string from network
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _JHCDEGRAPHER_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCDEGRAPHER_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Language/jhcNetRef.h"        // common audio
#include "Language/jhcMorphFcns.h"
#include "Reasoning/jhcActionTree.h"
#include "Semantic/jhcGraphlet.h"     

#include "Semantic/jhcSituation.h"


//= Generates natural language string from network.

class jhcDegrapher : private jhcSituation
{
// PRIVATE MEMBER VARIABLES
private:
  char phrase[500];
  const jhcMorphFcns *mf;
  jhcWorkMem *wmem;
  

// PUBLIC MEMBER VARIABLES
public:
  int noisy;                             


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcDegrapher ();
  jhcDegrapher ();
  void SetWords (const jhcMorphFcns& m) {mf = &m;}
  void SetMem (jhcWorkMem *m) {wmem = m;}
  void SetMem (jhcAliaNote& m) 
    {SetMem(dynamic_cast<jhcWorkMem *>(&m));}
 
  // formatted output
  const char *NameRef (jhcNetNode *n) const;
  const char *NodeRef (jhcNetNode *n, int nom =1)
    {return node_ref(phrase, 500, n, nom, NULL);}
  const char *UserRef () const
    {return((wmem == NULL) ? NULL : NameRef(wmem->Human()));}

  // alternative input type
  const char *NameRef (jhcAliaDesc *n) const
    {return NameRef(dynamic_cast<jhcNetNode *>(n));}
  const char *NodeRef (jhcAliaDesc *n, int nom =1)
    {return NodeRef(dynamic_cast<jhcNetNode *>(n), nom);}


// PRIVATE MEMBER FUNCTIONS
private:
  // formatted output
  const char *node_ref (char *txt, int ssz, jhcNetNode *n, int nom, const char *avoid);
  const char *pred_ref (char *txt, int ssz, jhcNetNode *n, int inf);
  const char *full_pred (char *txt, int ssz, const jhcNetNode *n, int inf);
  void list_conj (char *txt, int ssz, const jhcNetNode *multi) const;
  void inf_verb (char *txt, int ssz, const jhcNetNode *n) const;
  void agt_verb (char *txt, int ssz, const jhcNetNode *n);
  void copula (char *txt, int ssz, jhcNetNode *targ, const jhcNetNode *n);


  // object reference
  const char *obj_ref (char *txt, int ssz, jhcNetNode *n, int nom, const char *avoid);
  const char *pron_ref (char *txt, int ssz, const jhcNetNode *n, int nom) const;
  bool chk_prop (const jhcNetNode *n, const char *role, const char *label, 
                 const jhcGraphlet *desc) const;
  const char *name_ref (const jhcNetNode *n);
  const char *add_kind (char *txt, int ssz, const jhcNetNode *n, const char *avoid);
  const char *add_adj (char *txt, int ssz, const jhcNetNode *n, const char *avoid);
  const char *hyp_ref (char *txt, int ssz, const jhcNetNode *n, const char *avoid);

  // utilities
  char *strcatf_s (char *txt, int ssz, const char *fmt, ...) const;
  int num_match (int strip);

  // overrides
  int match_found (jhcBindings *m, int& mc) {return 1;}


};


#endif  // once




