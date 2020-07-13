// jhcNetBuild.h : adds speech acts to language-derived semantic nets
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

#ifndef _JHCNETBUILD_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNETBUILD_
/* CPPDOC_END_EXCLUDE */

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


// PUBLIC MEMBER FUNCTIONS
public:
  // main functions
  int NameSaid (const char *alist, int mode =2) const;
  int Convert (const char *alist);

  // utilities
  int HarvestLex (const char *kern);


// PRIVATE MEMBER FUNCTIONS
private:
  // speech acts
  int huh_tag () const;
  int hail_tag () const;
  int greet_tag () const;
  int farewell_tag () const;
  int add_tag (int rule, const char *alist) const;
  int attn_tag (const char *alist) const;
  jhcAliaChain *build_tag (jhcNetNode **node, const char *alist) const;
  void attn_args (jhcNetNode *input, const jhcAliaChain *bulk) const;

  // utilities
  int scan_lex (const char *fname);
  void save_word (char (*list)[40], int& cnt, const char *term) const;
  int gram_cats (const char *fname, const char *label) const;


};


#endif  // once




