// jhcSwapBase.cpp : control interface for external robot mobile platform
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2026 Etaoin Systems
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
  Defaults();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for basic motion commands.

int jhcSwapBase::ctrl_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("base_ctrl", 0);
  ps->NextSpecF( &msp,    12.0, "Nomimal move (in/sec)");  
  ps->NextSpecF( &tsp,   120.0, "Nominal turn (deg/sec)");  
  ps->NextSpecF( &mdone,   0.5, "Close enough move (in)");  
  ps->NextSpecF( &tdone,   2.0, "Close enough turn (deg)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Initialize internal state for next run.

void jhcSwapBase::Reset () 
{
  Zero();
  Update();
  def_cmd();
  Issue();
}


//= Clear all odometry and speed information.

int jhcSwapBase::Zero ()
{
  // clear accumulated sums, map position, and step changes
  trav = 0.0;
  wind = 0.0;
  xmap = 0.0;
  ymap = 0.0;
  along = 0.0;
  ortho = 0.0;
  dr = 0.0;

  // clear speed estimates
//  ips = 0.0;
  dps = 0.0;
  tupd = 0;
  parked = 0.0;
  return 1;
}


//= Reset locks and specify default commands.
// returns 1 always for convenience

void jhcSwapBase::def_cmd ()
{
  // move command
  mdir  = 0.0;
  mrate = 0.0;
  mlock = 0;

  // turn command
  trate = 0.0;
  tlock = 0;
}


///////////////////////////////////////////////////////////////////////////
//                             Data Exchange                             //
///////////////////////////////////////////////////////////////////////////

//= Cache new odometric input from robot sensors (call Update to transfer).
// path is cumulative distance travelled, spin is cumulative rotation
// spin 0 points along map x axis, spin 90 points along map y axis
// mx, my is robot center in map

void jhcSwapBase::Status (float path, float spin, float mx, float my, float up, float ccw)
{
  trav0  = path;
  wind0  = spin;
  xmap0  = mx;
  ymap0  = my;
}


//= Report motion command for robot actuators (use Issue to refresh).
// dist, ang are absolute stop values for cumulative travel and windup
// mvel, rvel are motion rates relative to nominal speeds
// skew is CCW angle of motion relative to centerline (0 = forward)
// mbid and rbid are importance of move and turn commands

void jhcSwapBase::Command (float& dist, float& ang, float& skew, float& mvel, float& rvel, int& mbid, int& rbid)
{
  // get translation command (incl. direction)
  dist = (float) mstop0;
  skew = (float) mdir0;                
  mvel = (float) mrate0;
  
  // get rotation command
  ang  = (float) tstop0;
  rvel = (float) trate0;

  // get importance of commands
  mbid = mlock0;
  rbid = tlock0;
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Update odometric travel of body (load cache values with Status).
// automatically resets "lock" for new bids

void jhcSwapBase::Update ()
{
  double rmix = 1.0, swivel = 10.0;              // mix was 0.3, swivel was 2
  double dt = 0.0, dx = xmap0 - xmap, dy = ymap0 - ymap;
  double rads = D2R * wind, s0 = sin(rads), c0 = cos(rads);
  UL32 last = tupd;

  // incremental rotation since last update
  dr = wind0 - wind;
  if (dr > 180.0)
    dr -= 360.0;
  else if (dr <= -180.0)
    dr += 360.0;

  // incremental displacement (ortho is to right)
  along = dx * c0 + dy * s0;
  ortho = dx * s0 - dy * c0;

  // new total travel, total turn, and map position
  trav  = trav0;
  wind  = wind0;
  xmap  = xmap0;
  ymap  = ymap0;

  // mix new speed estimates into longer term averages
  tupd = jms_now();
  if (last != 0)
  {
    dt = jms_secs(tupd, last);
    dps += rmix * ((fabs(dr) / dt) - dps); 
  }

  // keep track of how many secs robot has not turned
  if (fabs(dps) <= swivel)
    parked = __max(0.0, parked) + dt;
  else
    parked = __min(0.0, parked) - dt;

  // set up for next cycle of command arbitration
  def_cmd();
}


//= Harvest final angle commands now that arbitration is done.
// caches "cmd" into "cmd0" for Command

void jhcSwapBase::Issue ()      
{
  // translation command
  mstop0 = mstop;
  mdir0  = mdir;                
  mrate0 = mrate;
  mlock0 = mlock;
  
  // rotation command
  tstop0 = tstop;
  trate0 = trate;
  tlock0 = tlock;
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
  if (bid < mlock)
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
  if (bid < tlock)
    return 0;
  tlock = bid;
  tstop = hd;
  trate = rate;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                             Smooth Sliding                            //
///////////////////////////////////////////////////////////////////////////

//= Move toward incremental tracel but linearly slow down when close.
// helps compensate for sensor lag during tracking

int jhcSwapBase::MoveSoft (double dist, double rate, int bid, double soft)
{
  double err = MoveErr(trav + dist), f = __min(err / soft, 1.0);

  return MoveAbsolute(trav + dist, f * rate, bid);
}


//= Rotate toward incremental angle but linearly slow down when close.
// helps compensate for sensor lag during tracking

int jhcSwapBase::TurnSoft (double ang, double rate, int bid, double soft)
{
  double err = TurnErr(wind + ang), f = __min(err / soft, 1.0);

  return TurnAbsolute(wind + ang, f * rate, bid);
}
