// jhcTimedFcns.cpp : stub for connecting grounded procedures to ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#include "Action/jhcTimedFcns.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// NOTE: items in list must be deleted elsewhere

jhcTimedFcns::~jhcTimedFcns ()
{
  dealloc();
}


//= Default constructor initializes certain values.
// can specify max number of command instance

jhcTimedFcns::jhcTimedFcns (int n)
{
  next = NULL;
  clr_ptrs();
  SetSize(n);
}


//= Null all pointers to instance date.

void jhcTimedFcns::clr_ptrs ()
{
  // call info
  cmd  = NULL;     // function name
  cbid = NULL;     // importance
  ct0  = NULL;     // starting time

  // goal and progress
  cpos = NULL;     // position vector
  cdir = NULL;     // direction vector
  camt = NULL;     // desired amount
  csp  = NULL;     // desired speed
  cerr = NULL;     // recent error
  cst  = NULL;     // current state

  // no commands yet
  nc = 0;
}


//= Specify max number of command instances to accomodate.

void jhcTimedFcns::SetSize (int n)
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
    cbid = new int [n];
    ct0  = new UL32 [n];

    // goal and progress
    cpos = new jhcMatrix [n];
    for (i = 0; i < n; i++)
      cpos[i].SetSize(4);              // most common
    cdir = new jhcMatrix [n];
    for (i = 0; i < n; i++)
      cdir[i].SetSize(4);              // most common
    camt = new double [n];
    csp  = new double [n];
    cerr = new double [n];
    cst  = new int [n];
  }

  // initialize bids (mark unused)
  for (i = 0; i < nc; i++)
    cbid[i] = 0;
}


//= Get rid of all old arrays.

void jhcTimedFcns::dealloc ()
{
  int i;

  // goal and progress
  delete [] cst;
  delete [] cerr;
  delete [] csp;
  delete [] camt;
  delete [] cdir;
  delete [] cpos;

  // call info
  delete [] ct0;
  delete [] cbid;
  if (cmd != NULL)
    for (i = nc - 1; i >= 0; i--)
      delete [] cmd[i];
  delete [] cmd;

  // clear pointers for safety
  clr_ptrs();
}


//= Tack another pool of functions onto tail of list.

void jhcTimedFcns::AddFcns (jhcAliaKernel *pool)
{
  if (pool == NULL)
    return;
  if (next != NULL)
    next->AddFcns(pool);
  else
    next = pool;
}


//= Kill all instance of all functions.
// automatically chains to "next" pool

void jhcTimedFcns::Reset (jhcAliaNote *attn)
{
  int i;

  local_reset(attn);                                
  for (i = 0; i < nc; i++)
    cbid[i] = 0;
  if (next != NULL) 
    next->Reset(attn);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Post any spontaneous observations to attention queue.
// automatically chains to "next" pool

void jhcTimedFcns::Volunteer ()
{
  local_volunteer();
  if (next != NULL)
    next->Volunteer();
}


//= Start a function using given importance bid.
// if local function then records bid and starting time
// returns new instance number (>= 0) if success, -1 for problem, -2 for unknown

int jhcTimedFcns::Start (const jhcAliaDesc *desc, int bid)
{
  int inst, rc;

  // sanity check
  if ((bid <= 0) || (desc == NULL) || (desc->Word() == NULL))
    return -1;

  // find a locally free entry based on cbid[] <= 0
  for (inst = 0; inst < nc; inst++)
    if (cbid[inst] <= 0)
      break;
  if (inst >= nc)
    return -1;

  // speculatively bind entry then see if function is in local pool
  cst[inst]  = 0;
  cbid[inst] = bid;
  ct0[inst]  = jms_now();
  strcpy_s(cmd[inst], 40, desc->Word());
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

int jhcTimedFcns::Status (const jhcAliaDesc *desc, int inst)
{
  int rc = -2;

  // sanity check (catches failure during start also)
  if ((inst < 0) || (desc == NULL) || (desc->Word() == NULL))
    return -1;

  // possibly get the status of some local instance
  if (cbid[inst] > 0)
    if (_stricmp(desc->Word(), cmd[inst]) == 0)
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

int jhcTimedFcns::Stop (const jhcAliaDesc *desc, int inst)
{
  int i, rc = -2;

  // stop all instances
  if (inst < 0)
  {
    // stop locally matching instances (NULL matches any function)
    for (i = 0; i < nc; i++)
      if (cbid[i] > 0)
        if ((desc == NULL) || (_stricmp(desc->Word(), cmd[i]) == 0))
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
    if (_stricmp(desc->Word(), cmd[inst]) == 0)
    {
      rc = local_stop(desc, inst);
      cbid[inst] = 0;
    }

  // otherwise pass on to next pool in list
  if (((rc <= -2) || (desc == NULL)) && (next != NULL))
    return next->Stop(desc, inst);
  return rc;
}


///////////////////////////////////////////////////////////////////////////
//                               Utilities                               //
///////////////////////////////////////////////////////////////////////////

//= Tests if command is making suitable progress given current target error.
// reads and updates member variables associated with instance
//   cerr[i]: error from target on last cycle (MUST be initialized!)
//    ct0[i]: counts how many cycles with minimal progress
//    cst[i]: 0 = set up target, 1 = wait for movement start, 2 = check if done
// NOTE: function only works in states 1 and 2 and changes cst[i]

bool jhcTimedFcns::Stuck (int i, double err, double prog, int start, int mid)
{
  int wait = ((cst[i] <= 1) ? start : mid);  // no motion timeout depends on state

  if ((cerr[i] - err) < prog)
    return(ct0[i]++ > (UL32) wait);
  cerr[i] = err;
  ct0[i]  = 0;                               // reset count once movement starts
  if (cst[i] == 1)                 
    cst[i] = 2;          
  return false;
}
