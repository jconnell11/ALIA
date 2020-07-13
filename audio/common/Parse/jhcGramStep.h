// jhcGramStep.h : terminal or non-terminal in a sequence for a CFG grammar
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

#ifndef _JHCGRAMSTEP_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGRAMSTEP_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Terminal or non-terminal in a sequence for a CFG grammar.

class jhcGramStep
{
// PUBLIC MEMBER VARIABLES
public:
  char symbol[80];            /** Terminal or non-terminal name.      */
  int non;                    /** Whether this is a non-terminal.     */
  class jhcGramRule *back;    /** Rule used for non-terminal (chart). */
  jhcGramStep *tail;          /** Next element in rule expansion.     */
  

// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcGramStep () {*symbol = '\0'; non = 0; back = NULL; tail = NULL;}


};


#endif  // once




