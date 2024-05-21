// jhcSwapLift.cpp : control interface for external robot forklift stage
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Etaoin Systems
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

#include "Body/jhcSwapLift.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// needed as a placeholder for derived class virtual destructor

jhcSwapLift::~jhcSwapLift ()
{
}


//= Default constructor initializes certain values.

jhcSwapLift::jhcSwapLift ()
{
  lok = 1;
  ldone = 0.5;
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                              Configuration                            //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// "rpt" is generally level of disgnostic printouts desired
// returns 1 always

int jhcSwapLift::Reset (int rpt) 
{
  ht = 0.0;
  return def_cmd();
}


//= Reset locks and specify default commands.
// returns 1 always for convenience

int jhcSwapLift::def_cmd ()
{
  lrate = 0.0;
  llock = 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Get new measured height from robot sensors (indirectly).
// lvl is current height of fork lift stage above floor
// clears command priorites for next cycle
// override this function to directly query robot (if available) 
// returns 1 if sensors acquired, 0 or negative for problem

int jhcSwapLift::Status (float lvl)
{
  ht = lvl;
  return def_cmd();
}


//= Send motion command to robot actuators (indirectly).
// hdes is desired height for lift stage 
// sp is motion speed wrt nominal, bid is the importance of cmd
// override this function to directly drive robot (if available) 
// returns 1 if command sent, 0 or negative for problem

int jhcSwapLift::Command (float& hdes, float& sp, int& bid)
{
  // get height command and speed
  hdes = (float) lstop;
  sp = (float) lrate;

  // get command importance
  bid = llock;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Goal Specification                          //
///////////////////////////////////////////////////////////////////////////

//= Move forklift stage to some absolute height above floor.
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapLift::LiftTarget (double high, double rate, int bid)
{
  if (bid <= llock)
    return 0;
  llock = bid;
  lstop = high;
  lrate = rate;
  return 1;
}

