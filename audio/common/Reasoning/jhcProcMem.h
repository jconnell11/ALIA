// jhcProcMem.h : procedural memory for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#ifndef _JHCPROCMEM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCPROCMEM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>                     // needed for stdout

#include "Action/jhcAliaDir.h"         // common robot

#include "Reasoning/jhcAliaOp.h"       
#include "Reasoning/jhcWorkMem.h"


//= Procedural memory for ALIA system.
// has reactions to events as well as expansions for directives

class jhcProcMem
{
// PRIVATE MEMBER VARIABLES
private:
  // expansions for different kinds of directives
  jhcAliaOp *resp[JDIR_MAX];
  int np;


// PUBLIC MEMBER VARIABLES
public:
  int noisy;                    // controls diagnostic messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcProcMem ();
  jhcProcMem ();
  int NumOperators () const {return np;}

  // configuration
  int ClearOps () {return clear();}
  int Load (const char *fname, int add =0, int rpt =0, int level =1);
  int Save (const char *fname, int level =1) const;
  int Print (int level =1) const {return save_ops(stdout, level);}

  // list functions
  int AddOperator (jhcAliaOp *p, int ann =0);

  // main functions
  int FindOps (jhcAliaDir *dir, jhcWorkMem& wmem, double pth, double mth, int tol =0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  int clear ();
  void init_lists ();

  // configuration
  int save_ops (FILE *out, int level) const;


};


#endif  // once




