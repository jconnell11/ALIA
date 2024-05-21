// jhcSwapBase.cpp : control interface for external robot mobile platform
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

#include "Interface/jms_x.h"           // common video

#include "Body/jhcSwapBase.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// needed as a placeholder for derived class virtual destructor

jhcSwapBase::~jhcSwapBase ()
{
}


//= Default constructor initializes certain values.

jhcSwapBase::jhcSwapBase ()
{
  bok = 1;
  msp = 12.0;                // nomimal move in/sec
  tsp = 120.0;               // nominal turn deg/sec
  mdone = 0.5;               // close enough in inches
  tdone = 2.0;               // close enough in degrees
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Initialize internal state for next run.
// "rpt" is generally level of disgnostic printouts desired
// returns 1 always

int jhcSwapBase::Reset (int rpt) 
{
  Zero();
  return def_cmd();
}


//= Clear all odometry and speed information.

int jhcSwapBase::Zero ()
{
  // clear step changes, map position, and accumulated sums
  along = 0.0;
  ortho = 0.0;
  dr = 0.0;
  xmap = 0.0;
  ymap = 0.0;
  trav = 0.0;
  head = 0.0;

  // clear speed estimates
  ips = 0.0;
  dps = 0.0;
  tupd = 0;
  parked = 0;
  return 1;
}


//= Reset locks and specify default commands.
// returns 1 always for convenience

int jhcSwapBase::def_cmd ()
{
  // move command
  mrate = 0.0;
  mlock = 0;

  // turn command
  trate = 0.0;
  tlock = 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Get new odometric input from robot sensors (indirectly).
// xmap, ymap is robot center in map, hmap is centerline CCW orientation
// heading 0 points along map x axis, heading 90 points along map y axis
// clears command priorites for next cycle
// override this function to directly query robot (if available) 
// returns 1 if sensors acquired, 0 or negative for problem

int jhcSwapBase::Status (float mx, float my, float mh)
{
  double mmix = 0.5, rmix = 0.3, scoot = 1.0, swivel = 2.0;
  double dx = mx - xmap, dy = my - ymap, dm = sqrt(dx * dx + dy * dy);
  double cyc, dt, rads = D2R * head, s0 = sin(rads), c0 = cos(rads);
  UL32 last = tupd;

  // incremental movement since last update
  along = dx * c0 + dy * s0;
  ortho = dx * s0 - dy * c0;
  dr = mh - head;
  cyc = 360.0 * ROUND(fabs(dr) / 360.0);
  dr += ((dr < 0.0) ? cyc : -cyc);

  // new map position, total turn, and total travel 
  xmap = mx;
  ymap = my;
  head += dr;
  trav += ((along >= 0.0) ? dm : -dm);

  // mix new speed estimates into longer term averages
  tupd = jms_now();
  if (last != 0)
  {
    dt = jms_secs(tupd, last);
    ips += mmix * ((dm / dt) - ips); 
    dps += rmix * ((dr / dt) - dps); 
  }

  // keep track of how many cycles robot has not moved
  if ((fabs(ips) >= scoot) || (fabs(dps) >= swivel))
    parked = __min(0, parked - 1);
  else
    parked = __max(1, parked + 1);

  // set up for next cycle of command arbitration
  return def_cmd();
}


//= Send motion command to robot actuators (indirectly).
// dist, ang are SIGNED incremental stop values wrt current pose
// mvel, rvel are motion rates relative to nominal speeds
// skew is CCW angle of motion relative to centerline (0 = forward)
// mbid and rbid are importance of move and turn commands
// override this function to directly drive robot (if available) 
// returns 1 if command sent, 0 or negative for problem

int jhcSwapBase::Command (float& dist, float& ang, float& skew, float& mvel, float& rvel, int& mbid, int& rbid)
{
  // get translation command (incl. direction)
  dist = (float)(mstop - trav);
  skew = (float) mdir;                
  mvel = (float) mrate;
  
  // get rotation command
  ang  = (float)(tstop - head);
  rvel = (float) trate;

  // get importance of commands
  mbid = mlock;
  rbid = tlock;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                      Relative Goal Adjustment                         //
///////////////////////////////////////////////////////////////////////////

//= Change a static target location (in place) based on recent motion of the base.
// assumes Y points along centerline, X is to right, origin at midpoint of wheels
// does not alter Z (no interaction with lift stage or arm)

void jhcSwapBase::AdjustTarget (jhcMatrix& pos) const
{
  double rads = D2R * dr, c = cos(rads), s = sin(rads);
  double nx = pos.X() - ortho, ny = pos.Y() - along;

  pos.SetX( nx * c + ny * s);
  pos.SetY(-nx * s + ny * c);
}


//= Change planar angle (e.g. object orientation) if base rotates.
// adjusts in place and returns new value for convenience

double jhcSwapBase::AdjustAng (double& ang) const
{
  double adj = ang - dr;

  if (adj > 180.0)
    adj -= 360.0;
  else if (adj <= -180.0)
    adj += 360.0;
  ang = adj;
  return ang;
}


///////////////////////////////////////////////////////////////////////////
//                           Goal Specification                          //
///////////////////////////////////////////////////////////////////////////

//= Drive until a particular cumulative path distance has been reached.
// rate is relative to normal moving speed, skew is angle relative to centerline
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapBase::MoveAbsolute (double tr, double rate, int bid, double skew) 
{
  if (bid <= mlock)
    return 0;
  mlock = bid;
  mstop = tr;
  mrate = rate;
  mdir  = skew;
  return 1;
}


//= Turn until a particular cumulative windup angle has been reached.
// rate is relative to normal turning speed
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority
// NOTE: can command multiple revolutions, e.g. hd = hd0 + 720

int jhcSwapBase::TurnAbsolute (double hd, double rate, int bid)
{
  if (bid <= tlock)
    return 0;
  tlock = bid;
  tstop = hd;
  trate = rate;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Eliminate Residual Error                        //
///////////////////////////////////////////////////////////////////////////

//= Rotate to traverse some angle in a specific amount of time.
// keeps the commanded rotation rate less than rmax

int jhcSwapBase::TurnFix (double ang, double secs, double rmax, int bid)
{
  double r = ang / (tsp * secs);

  return TurnAbsolute(head + ang, __min(r, rmax), bid);
}
