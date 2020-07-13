// jhcBackgRWI.h : base class for pushing robot processing to background thread
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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


//= Base class for pushing robot processing to background thread.

class jhcBackgRWI
{
// PRIVATE MEMBER VARIABLES
private:
  // background control agent
  void *rd_lock, *xchg_ask, *xchg_done, *xchg_fcn; 
  int xchg_run;


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
  virtual void body_issue () =0;       /** Local behaviors and command transmission. */
  virtual void body_update () =0;      /** Sensor reception and image processing.    */  


// PRIVATE MEMBER FUNCTIONS
private:
  // background control agent
  int xchg_loop ();
  void start_xchg ();
  void stop_xchg ();

  // background thread
  static unsigned int __stdcall xchg_backg (void *inst)
    {jhcBackgRWI *me = (jhcBackgRWI *) inst; return me->xchg_loop();}


};


#endif  // once




