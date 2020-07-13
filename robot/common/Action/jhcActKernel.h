// jhcActKernel.h : provides grounding for some set of basic actions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2015 IBM Corporation
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

#ifndef _JHCACTKERNEL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCACTKERNEL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <string.h>


//= Convenient macro for defining action handlers in header file.
// convention is that name start with "x_" for use with jhcActSeq
// not truly needed -- could write these out in the normal way

#define JACT_DEF(fcn) int fcn (int start, double amt, double sp)


//= Convenient macros for building dispatchers (must have same variable names).
// derived class should override run_ctrl and fill with these statements
// NOTE: do not change name of arguments to run_ctrl function else macro breaks

#define JACT_RUN(fcn) if (_stricmp(act, #fcn) == 0) return fcn(start, amt, sp)
 

///////////////////////////////////////////////////////////////////////////

//= Interface class provides grounding for some set of basic actions.
// this is meant to contain single steps of an FSM (not a complete FSM)
// whole FSMs (and nested FSMs) should be implemented using jhcActSeq
//
// singly linked list of action sets, destructor does NOT delete other tail sets 
// if handler does not find function tag then should pass to next set (-2 if none)
// generally associated with a grammar fragment describing invocation patterns
// each grammar interpretation function has the standard format below:
// <pre>
//   int x_fcn (int start, double amt, double sp)
//   {
//     if (start > 0)
//       // intitialize with amt and sp
//     else (start == 0)
//       // continue running
//     else
//       // clean up
//   }
// </pre>
// "start": 1 = first invocation, 0 = continue running, -1 = shutdown
// "amt" and "sp" are factors applied to typical distance and velocity
// returns: 1 = success, 0 = running, -1 = failure

class jhcActKernel
{
// PRIVATE MEMBER VARIABLES
private:
  char last[80];
  jhcActKernel *next;


// PUBLIC MEMBER FUNCTIONS
public:
  jhcActKernel ();
  ~jhcActKernel ();
  void AddActions (jhcActKernel *end);
  void ClrAction ();
  int RunAction (const char *act =NULL, double amt =1.0, double sp =1.0);
  int Running (const char *tag) const {return((strcmp(tag, last) == 0) ? 1 : 0);}
  const char *Action () const {return last;}


// PROTECTED MEMBER FUNCTIONS
protected:
  virtual int run_ctrl (const char *act, int start, double amt, double sp); 
  int punt (const char *act, int start, double amt, double sp);


};


#endif  // once

