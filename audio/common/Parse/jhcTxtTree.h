// jhcTxtTree.h : nodes of a tree containing short strings
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013 IBM Corporation
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

#ifndef _JHCTXTTREE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTXTTREE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <string.h>

#include "Parse/jhcTxtSrc.h"


//= Nodes of a tree containing short strings.
// doubly-linked list with doubly linked hierarchy
// all branching is down and to the right to enforce tree-ness
// useful for organizing text into paragraphs, sentences, and words
// <pre>
//
//   [D]
//    |
//   [P]---------------------[P]-----------[P]
//    |                       |             |
//   [S]-----------[S]       [S]           [S]---[S]
//    |             |         |             |     |
//   [W]-[W]-[W]   [W]-[W]   [W]-[W]-[W]   [W]   [W]-[W]
//
// </pre>

class jhcTxtTree
{
// PRIVATE MEMBER VARIABLES
private:
  char txt[JTXT_MAX];
  jhcTxtTree *next, *prev, *child, *parent;

// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTxtTree (const char *tag =NULL);
  ~jhcTxtTree ();

  // content management
  const char *Text () const {return txt;}
  int Empty () const {return((*txt == '\0') ? 1 : 0);}
  void SetText (const char *tag) {strcpy(txt, ((tag != NULL) ? tag : ""));}
  void BuildText (const char *fmt, ...);
  bool Match (const char *probe, int caps =0) const;
  char *Linear (char *dest, int compact =0) const;
  void Linear (FILE *out, int compact =0) const;
  char *Fragment (char *dest, const jhcTxtTree *last =NULL, int compact =0) const;

  // basic navigation
  int Length () const;
  int Span (const jhcTxtTree *last) const;
  jhcTxtTree *Next () const {return next;}
  jhcTxtTree *Prev () const {return prev;}
  jhcTxtTree *Last ();
  jhcTxtTree *First ();
  jhcTxtTree *Bottom ();
  const jhcTxtTree *Last () const;
  const jhcTxtTree *First () const;
  const jhcTxtTree *Bottom () const;
  jhcTxtTree *SubList () const {return child;}
  jhcTxtTree *Pop () const;

  // enumeration and searching
  const jhcTxtTree *NextWord (int mv =1) const;
  const jhcTxtTree *NextSentence (int mv =1) const;
  jhcTxtTree *FindWord (const char *probe, int caps =0);
  const jhcTxtTree *FindPattern (const char *pattern) const;
  bool Satisfies (const char *pat) const;

  // construction
  jhcTxtTree *Append (const char *fmt =NULL, ...);
  jhcTxtTree *TackOn (const char *txt);
  jhcTxtTree *AddSub (const char *txt);
  int Truncate ();
  int ClrSub ();

  // hierarchy generation
  int FillDoc (jhcTxtSrc& src, int ndoc =1);
  int FillPara (jhcTxtSrc& src);
  int FillSent (jhcTxtSrc& src);

  // debugging
  void Print (int indent =0) const;
  int Save (const char *fname) const;


// PRIVATE MEMBER FUNCTIONS
private:
  bool sp_veto (const jhcTxtTree *t0, const jhcTxtTree *t, int compact) const;
  void save_n (FILE *out, int indent) const;

};


#endif  // once




