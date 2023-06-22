// jhcAliaTimer.cpp : simple timer function grounding for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023 Etaoin Systems
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

#include "Kernel/jhcAliaTimer.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaTimer::~jhcAliaTimer ()
{
}


//= Default constructor initializes certain values.

jhcAliaTimer::jhcAliaTimer ()
{
  strcpy_s(tag, "Timer");
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcAliaTimer::local_start (const jhcAliaDesc& desc, int i)
{
  JCMD_SET(time_delay);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcAliaTimer::local_status (const jhcAliaDesc& desc, int i)
{
  JCMD_CHK(time_delay);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Start delay timer and set some end time.
// returns 1 if okay, -1 for interpretation error

int jhcAliaTimer::time_delay0 (const jhcAliaDesc& desc, int i)
{
  camt[i] = find_secs(desc.Val("arg"));
  ct0[i] = jms_now();
  return 1;
}


//= Continue waiting for specified time to expire then succeed.
// returns 1 if done, 0 if still working, -1 for failure

int jhcAliaTimer::time_delay (const jhcAliaDesc& desc, int i)
{
  if (camt[i] > 0.0)
    if (jms_elapsed(ct0[i]) >= camt[i])
      return 1; 
  return 0;
}


//= Determine how many seconds to wait based on text description.

double jhcAliaTimer::find_secs (const jhcAliaDesc *amt) const
{
  const jhcAliaDesc *cnt, *hq;
  double secs = 20.0;                  // default for unknown amount

  // no argument means wait forever
  if (amt == NULL)
    return 0.0;

  // look for explicit time specification
  if (amt->LexIn("second", "minute"))
  {
    secs = ((amt->LexMatch("minute")) ? 60.0 : 1.0);
    if ((cnt = amt->Fact("cnt")) != NULL)
      secs *= atoi(cnt->Lex());
    return secs;
  }

  // check for qualitative time
  if (amt->Lex() != NULL)
    if (strstr(amt->Lex(), "little") != NULL)
      secs *= 0.5;

  // check for "little (while)" and "long (time)"
  if ((hq = amt->Fact("hq")) != NULL)
  {
    if (hq->LexMatch("little"))
      secs *= 0.5;
    else if (hq->LexMatch("long"))
      secs *= 3.0;
  }
  return secs;
}
