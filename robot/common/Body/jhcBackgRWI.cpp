// jhcBackgRWI.cpp : base class for pushing robot processing to background threads
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

#include <windows.h>
#include <process.h>

#include "Interface/jms_x.h"           // common video

#include "Body/jhcBackgRWI.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcBackgRWI::~jhcBackgRWI ()
{
  // deallocate thread control items
  stop_xchg();
  CloseHandle((HANDLE) aux2_done);
  CloseHandle((HANDLE) aux2_ask);
  CloseHandle((HANDLE) xchg_done);
  CloseHandle((HANDLE) xchg_ask);

  // deallocate mutex (if needed)
  if ((HANDLE) rd_lock != INVALID_HANDLE_VALUE)
    CloseHandle((HANDLE) rd_lock);
}


//= Default constructor initializes certain values.
// makes up thread control items 

jhcBackgRWI::jhcBackgRWI ()
{
  // primary thread
  xchg_ask  = (void *) CreateEvent(NULL, TRUE,  FALSE, NULL);  // manual reset
  xchg_done = (void *) CreateEvent(NULL, FALSE, FALSE, NULL);  // auto-reset
  xchg_fcn  = NULL;

  // secondary thread
  aux2_ask  = (void *) CreateEvent(NULL, TRUE,  FALSE, NULL);   
  aux2_done = (void *) CreateEvent(NULL, FALSE, FALSE, NULL);  
  aux2_fcn  = NULL;

  // overall state
  rd_lock = INVALID_HANDLE_VALUE;
  xchg_run = 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Restart background processing loop.
// restarts background loop, which first generates a body Issue call
// NOTE: should call this base class version at end of derived Reset function

void jhcBackgRWI::Reset ()
{
  stop_xchg();
  start_xchg();
}


//= Call at end of main loop to stop background processing and robot motion.
// NOTE: should call this base class version at beginning of derived Stop function

void jhcBackgRWI::Stop ()
{
  stop_xchg();
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Read and process all sensory information from robot.
// always blocks, inject commands only between this call and Issue
// returns 1 if okay, 0 or negative for error

int jhcBackgRWI::Update (UL32 resume)
{
  if (WaitForSingleObject((HANDLE) xchg_done, 1000) != WAIT_OBJECT_0)
    return jprintf(">>> Never got background done in jhcBackgRWI::Update\n");
  jms_resume(resume);                  // enforce min wait (to simulate robot)
  return 1;
}


//= Take a snapshot of current commands and start sending them to robot.
// any commands set after this will likely be ignored (test Biddable)
// but sensor info is generally still stable for a while (test Readable)
// returns 1 if okay, 0 or negative for error

int jhcBackgRWI::Issue ()
{
  SetEvent((HANDLE) xchg_ask);         // start next cycle
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Background Control Agent                        //
///////////////////////////////////////////////////////////////////////////

//= Start background sensor processing agents.

void jhcBackgRWI::start_xchg ()
{
  // make sure mutex starts in known state
  if ((HANDLE) rd_lock != INVALID_HANDLE_VALUE)
    CloseHandle((HANDLE) rd_lock);
  rd_lock = (void *) CreateMutex(NULL, FALSE, NULL);

  // initialize primary and secondary thread control events
  ResetEvent((HANDLE) aux2_done);
  ResetEvent((HANDLE) aux2_ask);
  ResetEvent((HANDLE) xchg_done);
  SetEvent((HANDLE) xchg_ask);
  xchg_run = 1;

  // start both threads
  xchg_fcn = (void *) _beginthreadex(NULL, 0, xchg_backg, this, 0, NULL);
  aux2_fcn = (void *) _beginthreadex(NULL, 0, aux2_backg, this, 0, NULL);
}


//= Cleanly exit background sensor processing agents.

void jhcBackgRWI::stop_xchg ()
{
  HANDLE all[2] = {(HANDLE) xchg_fcn,  (HANDLE) aux2_fcn};

  if ((xchg_run <= 0) || (xchg_fcn == NULL) || (aux2_fcn == NULL))
   return;

  // wait for end of last update (if any)
  WaitForSingleObject((HANDLE) xchg_done, 1000);

  // politely ask primary thread to exit (automatically stops secondary thread)
  xchg_run = 0;
  SetEvent((HANDLE) xchg_ask);
  if (WaitForSingleObject((HANDLE) xchg_done, 1000) != WAIT_OBJECT_0)
    jprintf(">>> Never got done signal in jhcBackgRWI::stop_xchg\n");

  // clean up both background threads
  if (WaitForMultipleObjects(2, all, true, 1000) != WAIT_OBJECT_0)
    jprintf(">>> Never got thread terminations in jhcBackgRWI::stop_xchg\n");
  CloseHandle((HANDLE) xchg_fcn);
  CloseHandle((HANDLE) aux2_fcn);
  xchg_fcn = NULL;
  aux2_fcn = NULL;
}


//= Respond to requests to process sensory inputs (runs as two separate threads).
// this is where bulk of the work is done, essentially an interrupted Update:Issue cycle
// cleanly exit by setting xchg_run = 0 and asserting xchg_ask then waiting on xchg_done
// <pre>
//
// THREADS
//             +------------------------------------------------------------+    
//    xchg:    |                                                            |
//             +--> body_update --+--> interpret ---+---X---> body_issue ---+
//                                |                 |
//                                |                 |
//    aux2:                       +--> interpret2 --+                  
//
//
// TIMING
//                  :                                                        :
//               +--:----------------+         +-----------------------------:-------
//    xchg_ask --+  :                +---------+                             :
//                  :    f1          ^         ^                   f2        :
//                  :     +--------+ |         |                       +-----:-------
//     rd_lock -----:-----* vision +-|---------|--------------------*--+ vis : ion
//                  :     ^          |         |                       ^     :
//                  :     |          |   +===+ |  +--+    +--+    +--+ |     :
//      ALIA   -----:-----|----------|---+   +-|--+  +----+  +----+  +-|-----:-------
//      think       :     |          |     ^   |    ^       ^      ^   |     : 
//                  :     |          |     |   |     \      |     /  start   :
//                  :   frame        |    cmd  |      +-----+----+  delayed  :
//                  :   ready        |   cycle |            |       by lock  :
//               Update           Update       |      extra cycles         Update
//               called >>>>>>>>> returns    Issue                         called >>>
//
// </pre>
// cycle is started by setting xchg_ask which remains set throughout whole cycle
// mutex img_lock grabbed during image processing, xchg_done set when cycle is complete 

int jhcBackgRWI::xchg_loop ()
{
  // sit around waiting for a request (from Issue)
  while (WaitForSingleObject((HANDLE) xchg_ask, INFINITE) == WAIT_OBJECT_0)
  {
    // possibly exit for some reason 
    if (xchg_run <= 0)
    {
      // request secondary processing thread (aux2) to stop also
      SetEvent((HANDLE) aux2_ask);
      if (WaitForSingleObject((HANDLE) aux2_done, 1000) != WAIT_OBJECT_0)
        jprintf(">>> Secondary thread never stopped in jhcBackgRWI::xchg_loop\n");  
      SetEvent((HANDLE) xchg_done);
      return 1;   
    }

    // END OF CYCLE - run local behaviors (if any) then send arbitrated commands to body 
    body_issue();

    // START OF CYCLE - request sensor lock then get new sensor data
    if (WaitForSingleObject((HANDLE) rd_lock, 1000) != WAIT_OBJECT_0)
      jprintf(">>> Never got image permission in jhcBackgRWI::xchg_loop\n");  
    body_update();

    // do sensor processing in this thread and also in secondary thread (aux2)
    SetEvent((HANDLE) aux2_ask);
    interpret();
    if (WaitForSingleObject((HANDLE) aux2_done, 1000) != WAIT_OBJECT_0)
      jprintf(">>> Secondary thread never finished in jhcBackgRWI::xchg_loop\n");  

    // signal that all sensor processing has completed
    ReleaseMutex((HANDLE) rd_lock);
    ResetEvent((HANDLE) xchg_ask);               // manual reset
    SetEvent((HANDLE) xchg_done);                
  }
  return 0;                                      // likely timeout
}


//= Secondary thread runs "interpret2" in parallel with "interpret" from main thread.
// cleanly exit by setting xchg_run = 0 and asserting aux2_ask then waiting on aux2_done
// cycle is started by setting aux2_ask which remains set throughout whole cycle
// assumes mutex "rd_lock" already grabbed inside other "xchg_loop" thread

int jhcBackgRWI::aux2_loop ()
{
  // sit around waiting for a request (from xchg_loop)
  while (WaitForSingleObject((HANDLE) aux2_ask, INFINITE) == WAIT_OBJECT_0)
  {
    // possibly exit for some reason 
    if (xchg_run <= 0)
    {
      SetEvent((HANDLE) aux2_done);
      return 1;   
    }

    // do secondary sensor processing in this thread
    interpret2();

    // signal secondary processing is complete
    ResetEvent((HANDLE) aux2_ask);               // manual reset
    SetEvent((HANDLE) aux2_done);                
  }
  return 0;                                      // likely timeout
}


///////////////////////////////////////////////////////////////////////////
//                         Intermediate Access                           //
///////////////////////////////////////////////////////////////////////////

//= See if background loops are accepting command settings.
// generally not allowed except between Update and Issue
// sensor data accessible in this interval (no need for Readable/ReadDone)
// NOTE: use this to sync grounding functions to perception loop

bool jhcBackgRWI::Accepting ()
{
  return(WaitForSingleObject((HANDLE) xchg_ask, 0) != WAIT_OBJECT_0);
}


//= See if background loops will allow access to images and sensor data.
// always okay between Update and Issue and for a while afterward
// make sure to call ReadDone at end to allow new data to be acquired
// NOTE: most grounding functions sync'd to Accepting so this is not needed

bool jhcBackgRWI::Readable ()
{
  return(WaitForSingleObject((HANDLE) rd_lock, 0) == WAIT_OBJECT_0);
}


//= Signal that no more access of images or sensor data will occur.
// be sure to call this any time Readable has been granted
// returns argument always (for convenience)

int jhcBackgRWI::ReadDone (int rc)
{
  ReleaseMutex((HANDLE) rd_lock);
  return rc;
}

