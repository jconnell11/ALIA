// jhcActKernel.cpp : provides grounding for some set of basic actions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2015 IBM Corporation
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

#include <math.h>

#include "Action/jhcActKernel.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcActKernel::jhcActKernel ()
{
  *last = '\0';
  next = NULL;
}


//= Default destructor does necessary cleanup.
// tries to stop last action, beware dangling next pointers
// should delete chain of libraries in head to tail order
// "last" variable is empty except for at head of chain

jhcActKernel::~jhcActKernel ()
{
  ClrAction();
}


//= Add another set of controllers to chain of libraries.

void jhcActKernel::AddActions (jhcActKernel *end) 
{
  if (next == NULL) 
    next = end; 
  else 
    next->AddActions(end);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Cleanly stop anything that is running.
// should call this before destructor, can also use as an intializer

void jhcActKernel::ClrAction ()
{
  RunAction(NULL);
}


//= Pass authority to the named controller, automatically setting the start flag.
// also automatically terminates any previous controller and caches name 

int jhcActKernel::RunAction (const char *act, double amt, double sp)
{
  double sp2 = fabs(sp);
  int ans;

  // continue action if same name
  if ((act != NULL) && (strcmp(act, last) == 0))
  {
    if ((ans = run_ctrl(act, 0, amt, sp2)) != 0)
      *last = '\0';
    return ans;
  }

  // stop last action if different action runs now
  if (*last != '\0')
    run_ctrl(last, -1, 1.0, 1.0);
  *last = '\0';
  if (act != NULL)
    strcpy_s(last, act);

  // try to run new action (if any)
  if (*last != '\0')
  {
    if ((ans = run_ctrl(last, 1, amt, sp2)) != 0)
      *last = '\0';
    return ans;
  }
  return -2;
}


//= Derived classes should copy this and insert JACT_RUN statements for handlers.
// JACT_RUN macro takes advantage of local variables start, amt, and sp

int jhcActKernel::run_ctrl (const char *act, int start, double amt, double sp)
{
  // JACT_RUN(x_move);
  // JACT_RUN(x_turn);
  // JACT_RUN(x_grab);
  return punt(act, start, amt, sp);
}


//= Pass authority to next library, if any.
// this should be the last statement of every derived run_ctrl procedure

int jhcActKernel::punt (const char *act, int start, double amt, double sp)
{
  if (next != NULL)
    return next->run_ctrl(act, start, amt, sp);
  return -2;
}

