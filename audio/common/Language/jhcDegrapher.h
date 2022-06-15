// jhcDegrapher.h : generates natural language string from network
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _JHCDEGRAPHER_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCDEGRAPHER_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Language/jhcNetRef.h"        // common audio
#include "Language/jhcMorphFcns.h"
#include "Reasoning/jhcActionTree.h"
#include "Semantic/jhcGraphlet.h"     


//= Generates natural language string from network.

class jhcDegrapher 
{
// PRIVATE MEMBER VARIABLES
private:
  char phrase[500];
  const jhcGraphlet *gr;
  const jhcMorphFcns *mf;
  jhcWorkMem *wmem;
  

// PUBLIC MEMBER VARIABLES
public:
  int dbg;                             


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcDegrapher ();
  jhcDegrapher ();
  void SetWords (const jhcMorphFcns *m) {mf = m;}
  void SetMem (jhcWorkMem *m) {wmem = m;}
  void SetMem (jhcAliaNote *m) 
    {SetMem(dynamic_cast<jhcWorkMem *>(m));}
 
  // formatted output
  const char *NameRef (const jhcNetNode *n) const
    {return(((n == NULL) || (wmem == NULL)) ? NULL : n->Name(0, wmem->MinBlf()));}
  const char *NodeRef (const jhcNetNode *n, int nom =1)
    {return node_ref(phrase, 500, n, nom, NULL);}
  const char *UserRef () const
    {return((wmem == NULL) ? NULL : NameRef(wmem->Human()));}

  // alternative input type
  const char *NameRef (const jhcAliaDesc *n) const
    {return NameRef(dynamic_cast<const jhcNetNode *>(n));}
  const char *NodeRef (const jhcAliaDesc *n, int nom =1)
    {return NodeRef(dynamic_cast<const jhcNetNode *>(n), nom);}

  // main functions
  const char *Generate (const jhcGraphlet& graph, jhcWorkMem& mem);


// PRIVATE MEMBER FUNCTIONS
private:
  // formatted output
  const char *node_ref (char *txt, int ssz, const jhcNetNode *n, int nom, const char *avoid) const;
  const char *pred_ref (char *txt, int ssz, const jhcNetNode *n, int inf) const;
  const char *full_pred (char *txt, int ssz, const jhcNetNode *n, int inf) const;

  // object reference
  const char *obj_ref (char *txt, int ssz, const jhcNetNode *n, int nom, const char *avoid) const;
  const char *pron_ref (char *txt, int ssz, const jhcNetNode *n, int nom) const;
  bool chk_prop (const jhcNetNode *n, const char *role, const char *label, 
                 const jhcGraphlet *desc) const;
  const char *name_ref (jhcNetRef& nr, const jhcNetNode *n) const;
  const char *add_kind (char *txt, int ssz, jhcNetRef& nr, const jhcNetNode *n, const char *avoid) const;
  const char *add_adj (char *txt, int ssz, jhcNetRef& nr, const jhcNetNode *n, const char *avoid) const;
  const char *hyp_ref (char *txt, int ssz, const jhcNetNode *n, const char *avoid) const;

  // utilities
  char *strcatf_s (char *txt, int ssz, const char *fmt, ...) const;

  // main functions
  int follow_args (jhcNetNode *n) const;
  const char *form_sent (char *txt, int ssz, const jhcNetNode *n, int top) const;
  const char *form_intj (char *txt, int ssz, const jhcNetNode *n) const;
  const char *form_conj (char *txt, int ssz, const jhcNetNode *n) const;

  const char *form_vp (char *txt, int ssz, const jhcNetNode *n) const;
  const char *form_verb (char *txt, int ssz, const jhcNetNode *n, UL32 tags) const;

  const char *form_np (char *txt, int ssz, const jhcNetNode *n) const;
  const char *form_ref (char *txt, int ssz, const jhcNetNode *n) const;
  const char *form_poss (char *txt, int ssz, const jhcNetNode *n) const;
  const char *form_det (char *txt, int ssz, const jhcNetNode *n) const;
  const char *form_adj (char *txt, int ssz, const jhcNetNode *n) const;
  const char *form_noun (char *txt, int ssz, const jhcNetNode *n, UL32 tags) const;

  const char *add_sp (char *txt, int ssz, const char *w, const char *suf =NULL) const;


};


#endif  // once




