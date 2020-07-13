// jhcDegrapher.h : generates natural language string from network
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include "Reasoning/jhcWorkMem.h"      // common audio
#include "Semantic/jhcGraphlet.h"      


//= Generates natural language string from network.

class jhcDegrapher
{
// PRIVATE MEMBER VARIABLES
private:
  const jhcWorkMem *wmem;
  const jhcGraphlet *gr;
  char out[500];


// PUBLIC MEMBER VARIABLES
public:


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcDegrapher ();
  jhcDegrapher ();
 
  // main functions
  const char *Generate (const jhcGraphlet& graph, const jhcWorkMem& mem);


// PRIVATE MEMBER FUNCTIONS
private:
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




