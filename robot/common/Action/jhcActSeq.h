// jhcActSeq.h : holds sequence of steps for an action
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2012 IBM Corporation
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

#ifndef _JHCACTSEQ_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCACTSEQ_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Holds sequence of steps for an action.

class jhcActSeq
{
// PRIVATE MEMBER VARIABLES
private:
  char file[200];
  char verb[80];
  char step[10][80];
  double adv[10];
  int next, len;
  jhcActSeq *sub;


// PUBLIC MEMBER VARIABLES
public:


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcActSeq (); 
  ~jhcActSeq ();
  int Database (const char *fname); 
  int Length () const {return len;} 
  const char *Verb () const {return verb;}
  void Reset ();

  // new action
  void Start (const char *act, int clr =1);
  int AddStep (const char *act, double amt =1.0, int over =0);
  int ChkVerb (const char *act);
  int Save ();

  // old action
  int Load (const char *act);
  int NextStep (char *act, double& amt, int ssz);
  int Levels ();
  int StepNum (int level =0);
  int ActName (char *act, int level, int ssz);

  // old action - convenience
  template <size_t ssz>
    int NextStep (char (&act)[ssz], double& amt)
      {return NextStep(act, amt, ssz);}
  template <size_t ssz>
    int ActName (char (&act)[ssz], int level = 0)
      {return ActName(act, level, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  void clr_sub ();

  char *next_token (char **src, const char *delim);

};


#endif  // once




