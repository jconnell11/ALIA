// jhcGramRule.h : expansion of a non-terminal in CFG grammar
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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

#ifndef _JHCGRAMRULE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGRAMRULE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>                    // needed for FILE

#include "Parse/jhcGramStep.h"


//= Expansion of a non-terminal in CFG grammar.
// also used to record state in Early parser chart

class jhcGramRule
{
// PUBLIC MEMBER VARIABLES
public:
  // basic info
  char head[80];         /** Associated non-terminal (no <>).    */
  jhcGramStep *tail;     /** Full expansion sequence for rule.   */
  int status;            /** 1 = enable, 2 = top, neg = disable. */
  int level;             /** source of rule (base vs. added)     */
  int id;                /** Unique state ID (for debugging).    */

  // parse state
  jhcGramStep *dot;      /** Next expansion symbol (chart).      */
  int w0;                /** Starting word in sentence (chart).  */
  int wn;                /** Ending word in sentence (chart).    */
  
  // enumeration
  jhcGramRule *next;     /** Next rule or parse state (chart).   */
  int mark;              /** Convenience flag for enumeration.   */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcGramRule (); 
  ~jhcGramRule ();

  // main functions
  int CopyState (const jhcGramRule *r);
  int SameRule (const jhcGramRule *ref) const;
  
  // file functions
  void Topic (FILE *out) const;
  void Expand (FILE *out) const;

  // debugging
  void PrintRule (const char *tag =NULL) const;
  void PrintState (const char *tag =NULL) const;


// PRIVATE MEMBER FUNCTIONS
private:


};


#endif  // once




