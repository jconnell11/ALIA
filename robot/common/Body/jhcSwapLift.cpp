// jhcSwapLift.cpp : control interface for external robot forklift stage
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2025 Etaoin Systems
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

void jhcSwapLift::Reset ()
{
  Status(0.0);
  Update();
  def_cmd();
  Issue();
}


//= Reset locks and specify default commands.
// returns 1 always for convenience

void jhcSwapLift::def_cmd ()
{
  lrate = 0.0;
  llock = 0;
}


///////////////////////////////////////////////////////////////////////////
//                             Data Exchange                             //
///////////////////////////////////////////////////////////////////////////

//= Cache new gaze angles from robot sensors (call Update to transfer).
// lvl is current height of fork lift stage above floor

void jhcSwapLift::Status (float lvl)
{
  ht0 = lvl;
}


//= Report motion commands for robot actuator (use Issue to refresh).
// hdes is desired height for lift stage 
// sp is motion speed wrt nominal, bid is the importance of cmd

void jhcSwapLift::Command (float& hdes, float& sp, int& bid)
{
  hdes = (float) lstop0;
  sp = (float) lrate0;
  bid = llock0;
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Update height of the lift stage (load cache values with Status).
// retrieves "data" from "data0", automatically resets "lock" for new bids

void jhcSwapLift::Update ()
{
  ht = ht0;
  def_cmd();
}


//= Harvest final angle commands now that arbitration is done.
// caches "cmd" into "cmd0" for Command

void jhcSwapLift::Issue ()      
{
  lstop0 = lstop;
  lrate0 = lrate;
  llock0 = llock;
}


///////////////////////////////////////////////////////////////////////////
//                           Goal Specification                          //
///////////////////////////////////////////////////////////////////////////

//= Move forklift stage to some absolute height above floor.
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapLift::LiftTarget (double high, double rate, int bid)
{
  if (bid < llock)
    return 0;
  llock = bid;
  lstop = high;
  lrate = rate;
  return 1;
}

