// jhcBgndGrok.h : base class for pushing robot processing to background threads
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2025 Etaoin Systems
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

#pragma once

#include "jhcGlobal.h"

#include "jhc_pthread.h"

#include "RWI/jhcGenGrok.h"             // common robot


//= Base class for pushing robot processing to background threads.
// main thread "digest" gets inputs, shares with "aux2", then sets outputs
// "aux2" can be a simple NOP if umwelt2 fcn is not overridden

class jhcBgndGrok : public jhcGenGrok
{
// PRIVATE MEMBER VARIABLES
private:
  sem_t digest_ask, aux2_ask, aux3_ask, aux4_ask;        // thread coordination 
  sem_t digest_done, aux2_done, aux3_done, aux4_done;                       
  pthread_t digest_fcn, aux2_fcn, aux3_fcn, aux4_fcn;    // background threads
  pthread_mutex_t rd_lock;                               // data access
  int digest_run;                                        // overall status


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcBgndGrok ();
  jhcBgndGrok ();

  // core interaction
  int Update (UL32 resume =0);
  int Issue ();

  // intermediate access
  bool Accepting () const;
  bool Readable ();
  int ReadDone (int rc =0);


// PROTECTED MEMBER FUNCTIONS
protected:
  // main functions
  void Reset ();
  void Stop ();

  // overriddables
  virtual void body_update () =0;      /** Request new sensor information from body. */  
  virtual void umwelt () =0;           /** Primary sensor processing commands.       */
  virtual void umwelt2 () {}           /** Secondary sensor processing commands.     */
  virtual void umwelt3 () {}           /** Tertiary sensor processing commands.      */
  virtual void umwelt4 () {}           /** Quaternary sensor processing commands.    */
  virtual void body_issue () =0;       /** Local behaviors and command transmission. */


// PRIVATE MEMBER FUNCTIONS
private:
  // background control agent
  int digest_loop ();
  int aux2_loop ();
  int aux3_loop ();
  int aux4_loop ();
  void start_digest ();
  void stop_digest ();

  // background threads (return 0 --either-> void * or int)
  static pthread_ret digest_backg (void *inst)
    {jhcBgndGrok *me = (jhcBgndGrok *) inst; me->digest_loop(); return 0;}
  static pthread_ret aux2_backg (void *inst)
    {jhcBgndGrok *me = (jhcBgndGrok *) inst; me->aux2_loop(); return 0;}
  static pthread_ret aux3_backg (void *inst)
    {jhcBgndGrok *me = (jhcBgndGrok *) inst; me->aux3_loop(); return 0;}
  static pthread_ret aux4_backg (void *inst)
    {jhcBgndGrok *me = (jhcBgndGrok *) inst; me->aux4_loop(); return 0;}


};
