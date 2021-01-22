// jhcBackgRWI.h : base class for pushing robot processing to background threads
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#ifndef _JHCBACKGRWI_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBACKGRWI_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Base class for pushing robot processing to background threads.
// main thread "xchg" gets inputs, shares with "aux2", then sets outputs
// "aux2" can be a simple NOP if interpret2 fcn is not overridden

class jhcBackgRWI
{
// PRIVATE MEMBER VARIABLES
private:
  // background control agents
  void *rd_lock;                                           // data access mutex
  void *xchg_ask, *xchg_done, *aux2_ask, *aux2_done;       // thread control events
  void *xchg_fcn, *aux2_fcn;                               // thread functions
  int xchg_run;                                            // overall state


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcBackgRWI ();
  jhcBackgRWI ();

  // core interaction
  int Update (UL32 resume =0);
  int Issue ();

  // intermediate access
  bool Accepting ();
  bool Readable ();
  int ReadDone (int rc =0);


// PROTECTED MEMBER FUNCTIONS
protected:
  // main functions
  void Reset ();
  void Stop ();

  // overriddables
  virtual void body_update () =0;      /** Request new sensor information from body. */  
  virtual void interpret () {};        /** Main sensor processing commands.          */
  virtual void interpret2 () {};       /** Parallel sensor processing commands.      */
  virtual void body_issue () =0;       /** Local behaviors and command transmission. */


// PRIVATE MEMBER FUNCTIONS
private:
  // background control agent
  int xchg_loop ();
  int aux2_loop ();
  void start_xchg ();
  void stop_xchg ();

  // background threads
  static unsigned int __stdcall xchg_backg (void *inst)
    {jhcBackgRWI *me = (jhcBackgRWI *) inst; return me->xchg_loop();}
  static unsigned int __stdcall aux2_backg (void *inst)
    {jhcBackgRWI *me = (jhcBackgRWI *) inst; return me->aux2_loop();}


};


#endif  // once




