// jhcNetBuild.h : adds speech acts to language-derived semantic nets
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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
  int Convert (const char *alist, const char *sent =NULL);
  void Summarize (FILE *log, const char *sent, int nt, int spact) const;

  // value range rules
  int AutoVals (const char *kern);

  // vocabulary generation
  int HarvestLex (const char *kern);


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  void intro_name (const char *alist) const;
  jhcAliaChain *feedback (int spact, const char *alist) const;

  // speech acts
  int huh_tag () const;
  int hail_tag () const;
  int greet_tag () const;
  int farewell_tag () const;
  int unk_tag (const char *unk) const;
  int add_tag (int spact, const char *alist, const char *sent);
  int rev_tag (int spact, const char *alist) const;
  int attn_tag (int spact, const char *alist) const;
  const char *no_fluff (const char *sent, const char *alist);
  jhcAliaChain *build_tag (jhcNetNode **node, const char *fcn, const char *alist, int dest) const;
  jhcAliaChain *ack_meta (jhcNetNode *item) const;
  jhcAliaChain *guard_plan (jhcAliaChain *steps, jhcNetNode *plan) const;
  jhcAliaChain *exp_fail (jhcNetNode *plan) const;

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

