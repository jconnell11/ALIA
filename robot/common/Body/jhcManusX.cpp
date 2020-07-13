// jhcManusX.cpp : generic functions for physical or TAIS forklift robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#include <math.h>

#include "Body/jhcManusX.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcManusX::~jhcManusX ()
{
}


//= Default constructor initializes certain values.

jhcManusX::jhcManusX ()
{
  // image size and gripper sensing values
  frame.SetSize(640, 360, 3);
  wmin  = 1.2;
  wmax  = 2.3;
  wsm   = 1.0;
  wtol  = 0.1;

  // gripper progress monitoring
  wprog = 0.1;
  wstop = 5;

  // processing parameters and default values
  Defaults();
  clr_state();
}


//= Set size for image processing (even if no video source bound).

void jhcManusX::SetSize (int w, int h)
{
  frame.SetSize(w, h, 3);
}


///////////////////////////////////////////////////////////////////////////
//                             Motion Commands                           //
///////////////////////////////////////////////////////////////////////////

//= Ask for robot to move forward or backward at some speed.
// generally ignores values with magnitude less than 0.5 in/sec
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcManusX::MoveVel (double ips, int bid)
{
  if (bid <= mlock)
    return 0;
  mlock = bid;
  move = ips;
  return 1;
}


//= Ask for robot to turn CCW or CW at some speed.
// generally ignores values with magnitude less than 15 deg/sec
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcManusX::TurnVel (double dps, int bid)
{
  if (bid <= tlock)
    return 0;
  tlock = bid;
  turn = dps;
  return 1;
}


//= Ask robot to raise or lower gripper at some speed.
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcManusX::LiftVel (double ips, int bid)
{
  if (bid <= flock)
    return 0;
  flock = bid;
  fork = ips;
  return 1;
}


//= Ask robot to open or close the gripper (with force control).
// bid value must be greater than previous command to take effect
// grab( 1) = set width to min (close event)
// grab(-1) = set width to max (open event)
// grab( 0) = set width to width when stop occurs (still moves)
// can override a grab(-1) with a grab(0) to assure grip remain
// can issue grab(1) to apply force again if in danger of dropping  
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcManusX::Grab (int dir, int bid)
{
  if (bid <= glock)
    return 0;
  glock = bid;
  grip = dir;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Helper Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get ready for new control run of robot.
// typically called as part of Reset()

int jhcManusX::clr_state ()
{
  // no image yet
  frame.FillArr(0);
  got = 0;

  // fake sensor values and odometry
  ht   = 0.3;          // close to ground
  dist = 18.0;         // nothing sensed
  wid  = wmax;         // fully open
  wprev = wid;
  wcnt = 0;
  Zero();

  // no commands or communications (yet)
  cmd_defs();
  mok = -1;
  return mok;
}


//= Reset all command bids and set up defaults.
// typically called as part of Update()

int jhcManusX::cmd_defs ()
{
  // see if gripper has stopped moving
  wcnt++;
  if (fabs(wid - wprev) > wprog)
    wcnt = 0;
  wprev = wid;

  // commands 
  move = 0.0;
  turn = 0.0;
  fork = 0.0;
  grip = 0;

  // bids
  mlock = 0;
  tlock = 0;
  flock = 0;
  glock = 0;
  return mok;
}


//= Reset odometry so current direction is angle zero and path length zero.
// also resets Cartesian coordinates to (0, 0) and x axis points forward
// typically called as part of Zero() function

void jhcManusX::clr_odom ()
{
  trav = 0.0;
  head = 0.0;
  xpos = 0.0;
  ypos = 0.0;
}


//= Keep an angle in the range -180 to +180 degrees.

double jhcManusX::norm_ang (double degs) const
{
  double a = degs;

  while (a > 180.0)
    a -= 360.0;
  while (a <= -180.0)
    a += 360.0;
  return a;
}




