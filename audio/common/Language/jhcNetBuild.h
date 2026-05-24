// jhcNetBuild.h : adds speech acts to language-derived semantic nets
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020-2026 Etaoin Systems
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

#pragma once

#include "jhcGlobal.h"

#include "Language/jhcGraphizer.h"          


//= Adds speech acts to language-derived semantic nets

class jhcNetBuild : public jhcGraphizer
{
// PRIVATE MEMBER VARIABLES
private:
  // max number of words to harvest in each class
  static const int wmax = 100; 

  // harvested words (allocated on heap not stack)
  char noun[wmax][40], adj[wmax][40], tag[wmax][40];
  char verb[wmax][40], mod[wmax][40], dir[wmax][40];
  int nn, na, nt, nv, nm, nd;

  // last ADD directive assembled
  char trim[500];
  jhcAliaDir *add;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  jhcNetBuild () {add = NULL;}

  // main functions
  int NameSaid (const char *alist, int mode =2) const;
  JSP_ACT Convert (const char *alist, const char *sent =NULL);
  void Summarize (FILE *log, const char *sent, int nt, JSP_ACT spact) const;

  // value range rules
  int AutoVals (const char *kern);

  // vocabulary generation
  int HarvestLex (const char *kern);


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  jhcAliaChain *feedback (JSP_ACT spact, const char *alist) const;

  // user responses
  void append_ynq (jhcAliaChain *main, jhcNodePool& pool) const; 
  void append_whq (jhcAliaChain *main, jhcNodePool& pool) const; 
  void append_exist (jhcAliaChain *main, jhcNodePool& pool) const;
  void append_find (jhcAliaChain *main, jhcNodePool& pool) const;
  jhcAliaChain *tell_step (const char *verb, jhcNodePool& pool) const;

  // speech acts
  JSP_ACT huh_tag () const;
  JSP_ACT hail_tag () const;
  JSP_ACT greet_tag () const;
  JSP_ACT farewell_tag () const;
  JSP_ACT unk_tag (const char *unk) const;
  JSP_ACT add_tag (JSP_ACT spact, const char *alist, const char *sent);
  JSP_ACT rev_tag (JSP_ACT spact, jhcAliaChain *main, const char *alist) const;
  JSP_ACT attn_tag (JSP_ACT spact, jhcAliaChain *main, const char *alist) const;
  const char *no_fluff (const char *sent, const char *alist);
  jhcAliaChain *build_tag (jhcNetNode **node, const char *fcn, const char *alist, int dest) const;
  jhcAliaChain *ack_meta (jhcNetNode *item) const;
  jhcAliaChain *exp_fail (jhcNetNode *plan) const;
  jhcAliaChain *ann_done (jhcNetNode *plan) const;

  // value range rules
  int range_rules (FILE *out, const char *cat, const char *lo, const char *hi, int nr) const;
  int value_rules (FILE *out, const char *cat, const char *val, int n) const;
  int exclude_rules (FILE *out, const char *lo, const char *hi, int nr) const;
  int mutex_rule (FILE *out, const char *val, const char *alt, int n) const;
  int opposite_rule (FILE *out, const char *v1, const char *v2, int n) const;
  int alias_rules (FILE *out, const char *cat, const char *val, const char *alt, int n) const;

  // vocabulary generation
  int scan_lex (const char *fname);
  void save_word (char (*list)[40], int& cnt, const char *term) const;
  int gram_cats (const char *fname, const char *label) const;


};

