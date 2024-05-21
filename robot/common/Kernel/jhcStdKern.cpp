// jhcStdKern.cpp : stub for connecting grounded procedures to ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
// Copyright 2021-2023 Etaoin Systems
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

#include "Kernel/jhcStdKern.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// NOTE: items in list must be deleted elsewhere

jhcStdKern::~jhcStdKern ()
{
  dealloc();
}


//= Default constructor initializes certain values.
// can specify max number of command instance

jhcStdKern::jhcStdKern (int n)
{
  next = NULL;
  clr_ptrs();
  alloc(n);
}


//= Null all pointers to instance date.

void jhcStdKern::clr_ptrs ()
{
  // call info
  cmd   = NULL;    // function name
  cobj  = NULL;    // focal object
  cspot = NULL;    // destination 
  csp   = NULL;    // desired speed
  cbid  = NULL;    // importance

  // goal and progress
  camt  = NULL;    // desired amount
  caux  = NULL;    // auxilliary info
  cerr  = NULL;    // recent error
  cmode = NULL;    // command variant
  cst   = NULL;    // current state
  cst2  = NULL;    // current substate
  ccnt  = NULL;    // general counter
  ct0   = NULL;    // reference time

  // no commands yet
  nc = 0;
}


//= Create some extra command information and state variables.

void jhcStdKern::alloc (int n)
{
  int i;

  // see if larger size needed 
  if (n > nc)
  {
    // get rid of old data then record new size
    dealloc();
    nc = n;

    // call info
    cmd  = new char * [n];
    for (i = 0; i < n; i++)
      cmd[i] = new char [40];
    cobj  = new jhcAliaDesc * [n];
    cspot = new jhcAliaDesc * [n];
    csp  = new double [n];
    cbid = new int [n];

    // goal and progress
    camt  = new double [n];
    caux  = new double [n];
    cerr  = new double [n];
    cmode = new int [n];
    cst   = new int [n];
    cst2  = new int [n];
    ccnt  = new int [n];
    ct0   = new UL32 [n];
  }

  // initialize bids (mark unused)
  for (i = 0; i < nc; i++)
    cbid[i] = 0;
}


//= Get rid of all old arrays.

void jhcStdKern::dealloc ()
{
  int i;

  // goal and progress
  delete [] ct0;
  delete [] ccnt;
  delete [] cst2;
  delete [] cst;
  delete [] cmode;
  delete [] cerr;
  delete [] caux;
  delete [] camt;

  // call info
  delete [] cbid;
  delete [] csp;
  delete [] cspot;
  delete [] cobj;
  if (cmd != NULL)
    for (i = nc - 1; i >= 0; i--)
      delete [] cmd[i];
  delete [] cmd;

  // clear pointers for safety
  clr_ptrs();
}


//= Tack another pool of functions onto tail of list.

void jhcStdKern::AddFcns (jhcAliaKernel& pool)
{
  if (next != NULL)
    next->AddFcns(pool);
  else
    next = &pool;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Connect kernel to real-world sensors and actuators.
// automatically chains to "next" pool

void jhcStdKern::Platform (void *soma)
{
  local_platform(soma);
  if (next != NULL)
    next->Platform(soma);
}


//= Kill all instance of all functions.
// automatically chains to "next" pool

void jhcStdKern::Reset (jhcAliaNote& atree)
{
  int i;

  local_reset(atree);                                
  for (i = 0; i < nc; i++)
    cbid[i] = 0;
  if (next != NULL) 
    next->Reset(atree);
}


//= Post any spontaneous observations to attention queue.
// automatically chains to "next" pool

void jhcStdKern::Volunteer ()
{
  local_volunteer();
  if (next != NULL)
    next->Volunteer();
}


//= Start a function using given importance bid.
// if local function then records bid and starting time
// returns new instance number (>= 0) if success, -1 for problem, -2 for unknown

int jhcStdKern::Start (const jhcAliaDesc& desc, int bid)
{
  int inst, rc;

  // sanity check
  if ((bid <= 0) || (desc.Lex() == NULL))
    return -1;

  // find a locally free entry based on cbid[] <= 0
  for (inst = 0; inst < nc; inst++)
    if (cbid[inst] <= 0)
      break;
  if (inst >= nc)
    return -1;

  // initialize variables for new instance
  csp[inst]  = 1.0;                    // initial speed = 1
  cbid[inst] = bid;
  cst[inst]  = 0;                      // initial state = 0:0
  cst2[inst] = 0;
  ct0[inst]  = 0;                      // initial time  = 0 (was jms_now)

  // speculatively bind entry then see if function is in local pool
  strcpy_s(cmd[inst], 40, desc.Lex());
  if ((rc = local_start(desc, inst)) >= 0)
    return inst;

  // otherwise invalidate entry and try passing to some other pool  
  cbid[inst] = 0;
  if ((rc <= -2) && (next != NULL))
    return next->Start(desc, bid);
  return rc;
}


//= Check whether a function instance has completed yet.
// returns positive for done, 0 for still running, -1 for failure, -2 if unknown

int jhcStdKern::Status (const jhcAliaDesc& desc, int inst)
{
  int rc = -2;

  // sanity check (catches failure during start also)
  if ((inst < 0) || (desc.Lex() == NULL))
    return -1;

  // possibly get the status of some local instance
  if (cbid[inst] > 0)
    if (_stricmp(desc.Lex(), cmd[inst]) == 0)
      rc = local_status(desc, inst);

  // otherwise pass on to next pool in list
  if ((rc <= -2) && (next != NULL))
    return next->Status(desc, inst);
  if (rc > 0)
    cbid[inst] = 0;                            // free up slot when done
  return rc;
}


//= Stop a particular function instance (or all if negative).
// calling with NULL description causes all instances of everything to stop
// returns positive for convenience, -2 if unknown (courtesy call - should never wait)

int jhcStdKern::Stop (const jhcAliaDesc& desc, int inst)
{
  int i, rc = -2;

  // stop all instances
  if (inst < 0)
  {
    // stop locally matching instances (NULL matches any function)
    for (i = 0; i < nc; i++)
      if (cbid[i] > 0)
        if (desc.LexMatch(cmd[i]))
        {
          local_stop(desc, i);
          cbid[i] = 0;
        }

    // for safety pass on to other pools in list
    if (next != NULL)
      return next->Stop(desc, inst);
    return 1;
  }

  // stop a single local instance
  if (cbid[inst] > 0)
    if (desc.LexMatch(cmd[inst]))
    {
      rc = local_stop(desc, inst);
      cbid[inst] = 0;
    }

  // otherwise pass on to next pool in list
  if ((rc <= -2) && (next != NULL))
    return next->Stop(desc, inst);
  return rc;
}

