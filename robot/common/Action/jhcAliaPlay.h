// jhcAliaPlay.h : play contains group of coordinated FSMs in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#ifndef _JHCALIAPLAY_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIAPLAY_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcTxtLine.h"          // common audio
#include "Semantic/jhcNodePool.h"

#include "Action/jhcAliaChain.h"       // common robot


//= Play contains group of coordinated FSMs in ALIA system.
//   main = things to accomplish before continuing (all must succeed)
//   guard = things to do while working on main activities (success irrelevant)
// runs all activities in guard list and main list on every cycle
//   if any activity (main or guard) fails, then whole play fails
//   order of lists reflects priorities of activities but
//   all guard activities are higher priority than any main activity
// a CHK directive expanded as a play does a logical AND of main activities
// <pre>
//   To check is something is an animal:
//     check if it moves and has fur       <-- first NO returns NO from play
//     while being careful of its teeth 
// </pre> 
// this is a second-class structure, an adjunct to graph structure of jhcAliaChain

class jhcAliaPlay
{
// PRIVATE MEMBER VARIABLES
private:
  static const int amax = 10;        /** Maximum activities in a set. */

  // sets of activities
  jhcAliaChain *main[amax], *guard[amax]; 
  int na, ng;

  // current state
  int status[amax], gstat[amax];
  int verdict;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaPlay ();
  jhcAliaPlay ();
  int AddReq (jhcAliaChain *act);
  int AddSimul (jhcAliaChain *act);
  void MarkSeeds ();
  int MaxDepth () const;
  int NumGoals (int leaf =0) const;

  // read only variables
  int NumReq () const   {return na;} 
  int NumSimul () const {return ng;} 
  jhcAliaChain *ReqN (int n) const   
    {return(((n >= 0) && (n < na)) ? main[n] : NULL);}
  jhcAliaChain *SimulN (int n) const 
    {return(((n >= 0) && (n < ng)) ? guard[n] : NULL);}

  // main functions
  int Start (jhcAliaCore *all, int lvl);
  int Status ();
  int Stop (int ans =-1);
  int FindActive (const jhcGraphlet& desc, int halt);

  // file functions
  int Load (jhcNodePool& pool, jhcTxtLine& in); 
  int Save (FILE *out, int lvl =0, int *step =NULL) const;
  int Print (int lvl =0, int *step =NULL) const
    {return Save(stdout, lvl, step);}
 

// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr ();

  // main functions
  int fail ();


};


#endif  // once




