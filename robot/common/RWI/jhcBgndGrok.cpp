// jhcBgndGrok.cpp : base class for pushing robot processing to background threads
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "Interface/jms_x.h"           // common video
#include "Interface/jprintf.h" 
#include "Interface/jtimer.h"          // for profiling

#include "RWI/jhcBgndGrok.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcBgndGrok::~jhcBgndGrok ()
{
  stop_digest();
  pthread_mutex_destroy(&rd_lock);
  sem_destroy(&digest_ask);
  sem_destroy(&digest_done);
  sem_destroy(&aux2_ask);
  sem_destroy(&aux2_done);
  sem_destroy(&aux3_ask);
  sem_destroy(&aux3_done);
  sem_destroy(&aux4_ask);
  sem_destroy(&aux4_done);
}


//= Default constructor initializes certain values.
// makes up thread control items 

jhcBgndGrok::jhcBgndGrok ()
{
  sem_init(&digest_ask,  0, 0);
  sem_init(&digest_done, 0, 0);
  sem_init(&aux2_ask,  0, 0);
  sem_init(&aux2_done, 0, 0);
  sem_init(&aux3_ask,  0, 0);
  sem_init(&aux3_done, 0, 0);
  sem_init(&aux4_ask,  0, 0);
  sem_init(&aux4_done, 0, 0);
  pthread_mutex_init(&rd_lock, NULL);
  accept = 1;
  digest_run  = 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Restart background processing loop.
// restarts background loop, which first generates a body Issue call
// NOTE: should call this base class version at end of derived Reset function

void jhcBgndGrok::Reset ()
{
  stop_digest();
  start_digest();
}


//= Call at end of main loop to stop background processing and robot motion.
// NOTE: should call this base class version at beginning of derived Stop function

void jhcBgndGrok::Stop ()
{
  stop_digest();
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Read and process all sensory information from robot.
// always blocks, inject commands only between this call and Issue
// returns 1 if okay, 0 or negative for error

int jhcBgndGrok::Update (UL32 resume)
{
  abstime_t one_sec;
      
  if (sem_timedwait(&digest_done, abstime_wait(&one_sec, 1000)) != 0)               
    return jprintf(">>> Never got background done in jhcBgndGrok::Update\n");
  jms_resume(resume);                  // enforce min wait (to simulate robot)
  return 1;
}


//= Take a snapshot of current commands and start sending them to robot.
// any commands set after this will likely be ignored (test Biddable)
// but sensor info is generally still stable for a while (test Readable)
// returns 1 if okay, 0 or negative for error

int jhcBgndGrok::Issue ()
{
  accept = 0;
  sem_post(&digest_ask);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Background Control Agent                        //
///////////////////////////////////////////////////////////////////////////

//= Start background sensor processing agents.

void jhcBgndGrok::start_digest ()
{
  // start all threads
  pthread_create(&aux4_fcn, NULL, aux4_backg, this);
  pthread_create(&aux3_fcn, NULL, aux3_backg, this);
  pthread_create(&aux2_fcn, NULL, aux2_backg, this);
  pthread_create(&digest_fcn, NULL, digest_backg, this);

  // wake up primary thread so it can see run flag
  accept = 1;
  digest_run = 1;
  sem_post(&digest_ask);
}


//= Cleanly exit background sensor processing agents.

void jhcBgndGrok::stop_digest ()
{
  abstime_t one_sec;

  // wait for end of last update (if any)
  if (digest_run <= 0) 
    return;
  sem_timedwait(&digest_done, abstime_wait(&one_sec, 1000));

  // politely ask primary thread to exit (automatically stops other thread)
  digest_run = 0;
  sem_post(&digest_ask);
  if (sem_timedwait(&digest_done, abstime_wait(&one_sec, 1000)) != 0)
    jprintf(">>> Never got done signal in jhcBgndGrok::stop_digest\n");
  accept = 1;

  // terminate and clean up background threads
  pthread_timedjoin_np(aux4_fcn,   0, abstime_wait(&one_sec, 1000));
  pthread_timedjoin_np(aux3_fcn,   0, abstime_wait(&one_sec, 1000));
  pthread_timedjoin_np(aux2_fcn,   0, abstime_wait(&one_sec, 1000));
  pthread_timedjoin_np(digest_fcn, 0, abstime_wait(&one_sec, 1000));
  pthread_detach(aux4_fcn);
  pthread_detach(aux3_fcn);
  pthread_detach(aux2_fcn);
  pthread_detach(digest_fcn);
}


//= Respond to requests to process sensory inputs (runs as up to 4 separate threads).
// this is where bulk of the work is done, essentially an interrupted Update:Issue cycle
// <pre>
//
// THREADS
//
//              +----------------------------------------------------------+  
//   digest_ask |                                                          | digest_done
//              +--> body_update --+--> umwelt ----+---X---> body_issue ---+
//                                 |               ^
//                        aux2_ask |               | aux2_done
//                                 +--> umwelt2 ---+                  
//                        aux3_ask |               | aux3_done
//                                 +--> umwelt3 ---+                  
//                        aux4_ask |               | aux4_done
//                                 +--> umwelt4 ---+                  
//
//
// TIMING
//                  :                +---------+                             :
//       accept ----:----------------+         +-----------------------------:------
//                  :                                                        :
//              ----:-----+                    +-----------------------+     :
//   digest_ask     :     +--------------------+                       +-----:------
//                  :                          ^                             :
//              ----+                +---------|-----------------------------+
//  digest_done     +----------------+         |                             +------
//                  :    f1          ^         |                   f2        :
//                  :     +--------+ |         |                       +-----:------
//      rd_lock ----:-----* vision +-|---------|--------------------*--+   vision
//                  :     ^          |         |                       ^     :
//                  :     |          |   +===+ |  +--+    +--+    +--+ |     :
//       ALIA   ----:-----|----------|---+   +-|--+  +----+  +----+  +-|-----:------
//       think      :     |          |     ^   |    ^       ^      ^   |     : 
//                  :     |          |     |   |     \      |     /  start   :
//                  :   frame        |    cmd  |      +-----+----+  delayed  :
//                  :   ready        |   cycle |            |       by lock  :
//               Update           Update       |      extra cycles         Update
//               called >>>>>>>>> returns    Issue                         called >>>
//
// </pre>
// cycle is started by setting digest_ask, digest_done set when cycle is complete 
// cleanly exit by setting digest_run = 0 and setting digest_ask then waiting on digest_done
// mutex rd_lock grabbed during image processing

int jhcBgndGrok::digest_loop ()
{
  abstime_t one_sec;

  while (1)
  {
    // sit around waiting for a request (digest_ask set by Issue)
    if (sem_wait(&digest_ask) != 0)
      break;

    // possibly exit for some reason 
    if (digest_run <= 0)
    {
      // request secondary processing thread (aux2) to stop also
      sem_post(&aux4_ask);      
      sem_post(&aux3_ask);      
      sem_post(&aux2_ask);      
      abstime_wait(&one_sec, 1000);
      if (sem_timedwait(&aux2_done, &one_sec) != 0)
        jprintf(">>> Secondary thread never stopped in jhcBgndGrok::digest_loop\n");  
      if (sem_timedwait(&aux3_done, &one_sec) != 0)
        jprintf(">>> Tertiary thread never stopped in jhcBgndGrok::digest_loop\n");  
      if (sem_timedwait(&aux4_done, &one_sec) != 0)
        jprintf(">>> Quaternary thread never stopped in jhcBgndGrok::digest_loop\n");  
      accept = 1;
      sem_post(&digest_done);
      return 1;   
    }

jtimer(1, "digest (issue update umwelts)");
    // END OF CYCLE - run local behaviors (if any) then send arbitrated commands to body 
    body_issue();

    // START OF CYCLE - request sensor lock then get new sensor data
    if (pthread_mutex_timedlock(&rd_lock, abstime_wait(&one_sec, 1000)) != 0)
      jprintf(">>> Never got image permission in jhcBgndGrok::digest_loop\n");  
    body_update();

jtimer(2, "image analysis (unwelts)");
    // do sensor processing in this thread and also in other thread (aux's)
    sem_post(&aux4_ask);
    sem_post(&aux3_ask);
    sem_post(&aux2_ask);
    abstime_wait(&one_sec, 1000);
    umwelt();
    if (sem_timedwait(&aux2_done, &one_sec) != 0)
      jprintf(">>> Secondary thread never finished in jhcBgndGrok::digest_loop\n");
    if (sem_timedwait(&aux3_done, &one_sec) != 0)
      jprintf(">>> Tertiary thread never finished in jhcBgndGrok::digest_loop\n");  
    if (sem_timedwait(&aux4_done, &one_sec) != 0)
      jprintf(">>> Quaternary thread never finished in jhcBgndGrok::digest_loop\n");    
jtimer_x(2);

    // signal that all sensor processing has completed
    pthread_mutex_unlock(&rd_lock);
    accept = 1;
    sem_post(&digest_done);
jtimer_x(1);
  }
  return 0;                                      // likely timeout
}


//= Secondary thread runs "umwelt2" in parallel with "umwelt" from main thread.
// cycle is started by setting aux2_ask, aux2_done set when cycle is complete 
// cleanly exit by setting digest_run = 0 and asserting aux2_ask then waiting on aux2_done
// assumes mutex rd_lock already grabbed inside other digest_loop() thread

int jhcBgndGrok::aux2_loop ()
{
  while (1)
  {
    // sit around waiting for a request (aux2_ask set by digest_loop)
    if (sem_wait(&aux2_ask) != 0)
      break;

    // possibly exit for some reason 
    if (digest_run <= 0)
    {
      sem_post(&aux2_done);
      return 1;   
    }

    // do secondary sensor processing in this thread
    umwelt2();

    // signal secondary processing is complete
    sem_post(&aux2_done);
  }
  return 0;                                      // likely timeout
}


//= Tertiary thread runs "umwelt3" in parallel with "umwelt" from main thread.
// cycle is started by setting aux3_ask, aux3_done set when cycle is complete 
// cleanly exit by setting digest_run = 0 and asserting aux3_ask then waiting on aux3_done
// assumes mutex rd_lock already grabbed inside other digest_loop() thread

int jhcBgndGrok::aux3_loop ()
{
  while (1)
  {
    // sit around waiting for a request (aux2_ask set by digest_loop)
    if (sem_wait(&aux3_ask) != 0)
      break;

    // possibly exit for some reason 
    if (digest_run <= 0)
    {
      sem_post(&aux3_done);
      return 1;   
    }

    // do secondary sensor processing in this thread
    umwelt3();

    // signal secondary processing is complete
    sem_post(&aux3_done);
  }
  return 0;                                      // likely timeout
}


//= Quaternary thread runs "umwelt4" in parallel with "umwelt" from main thread.
// cycle is started by setting aux4_ask, aux4_done set when cycle is complete 
// cleanly exit by setting digest_run = 0 and asserting aux4_ask then waiting on aux4_done
// assumes mutex rd_lock already grabbed inside other digest_loop() thread

int jhcBgndGrok::aux4_loop ()
{
  while (1)
  {
    // sit around waiting for a request (aux2_ask set by digest_loop)
    if (sem_wait(&aux4_ask) != 0)
      break;

    // possibly exit for some reason 
    if (digest_run <= 0)
    {
      sem_post(&aux4_done);
      return 1;   
    }

    // do secondary sensor processing in this thread
    umwelt4();

    // signal secondary processing is complete
    sem_post(&aux4_done);
  }
  return 0;                                      // likely timeout
}


///////////////////////////////////////////////////////////////////////////
//                         Intermediate Access                           //
///////////////////////////////////////////////////////////////////////////

//= See if background loops are accepting command settings.
// generally not allowed except between Update and Issue
// means fresh sensor data is available and actuator commands can be given
// NOTE: new sensor data accessible in this interval (no need for Readable/ReadDone)
// NOTE: used in grounding fcns for volunteered events and physical position updates

bool jhcBgndGrok::Accepting () const
{
  return accept;             // same as base class (just more doc here)
}


//= See if background loops will allow access to images and sensor data.
// always okay between Update and Issue and for a while afterward
// NOTE: used in grounding fcns for analyzing existing data (use Accepting for new data)
// NOTE: make sure to call ReadDone at end to allow data acquisition to continue

bool jhcBgndGrok::Readable ()
{
  return((pthread_mutex_trylock(&rd_lock) == 0) ? 1 : 0);
}


//= Signal that no more access of images or sensor data will occur.
// be sure to call this any time Readable has been granted
// returns argument always (for convenience)

int jhcBgndGrok::ReadDone (int rc)
{
  pthread_mutex_unlock(&rd_lock);      // for long interventions
  return rc;
}

