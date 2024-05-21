// jhcAliaKudos.cpp : user feedback on confidence and preference thresholds
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023-2024 Etaoin Systems
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

#include "Kernel/jhcAliaKudos.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaKudos::~jhcAliaKudos ()
{
}


//= Default constructor initializes certain values.

jhcAliaKudos::jhcAliaKudos ()
{
  strcpy_s(tag, "Kudos");
  mood = NULL;
  atree = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcAliaKudos::local_start (const jhcAliaDesc& desc, int i)
{
  JCMD_SET(kudo_conf);
  JCMD_SET(kudo_pref);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcAliaKudos::local_status (const jhcAliaDesc& desc, int i)
{
  JCMD_CHK(kudo_conf);
  JCMD_CHK(kudo_pref);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Start trying to adjust belief threshold for user certification of inference.
// returns 1 if okay, -1 for interpretation error

int jhcAliaKudos::kudo_conf0 (const jhcAliaDesc& desc, int i)
{
  const jhcAliaDesc *hq = desc.Val("arg");

  if (mood == NULL)
    return -1;
  if (hq == NULL)
    return -1;
  cmode[i] = ((hq->Neg() > 0) ? -1 : 1);
  return 1;
}


//= Continue trying to adjust belief threshold for user certification of inference.
// returns 1 if done, 0 if still working, -1 for failure

int jhcAliaKudos::kudo_conf (const jhcAliaDesc& desc, int i)
{
  mood->UserConf(cmode[i]);
  return 1;
}


//= Start trying to adjust preference threshold for user approval of action.
// returns 1 if okay, -1 for interpretation error

int jhcAliaKudos::kudo_pref0 (const jhcAliaDesc& desc, int i)
{
  const jhcAliaDesc *hq = desc.Val("arg");

  if (mood == NULL)
    return -1;
  if (hq == NULL)
    return -1;
  cmode[i] = ((hq->Neg() > 0) ? -1 : 1);
  return 1;
}


//= Continue trying to adjust preference threshold for user approval of action.
// returns 1 if done, 0 if still working, -1 for failure

int jhcAliaKudos::kudo_pref (const jhcAliaDesc& desc, int i)
{
  mood->UserPref(cmode[i]);
  return 1;
}

