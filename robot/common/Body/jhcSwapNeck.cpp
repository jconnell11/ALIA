// jhcSwapNeck.cpp : control interface for external robot camera aiming
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

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"

#include "Body/jhcSwapNeck.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// needed as a placeholder for derived class virtual destructor

jhcSwapNeck::~jhcSwapNeck ()
{
}


//= Default constructor initializes certain values.

jhcSwapNeck::jhcSwapNeck ()
{
  nok = 1;
  Reset();
  p0 = 0.0;
  t0 = 0.0;
  stable = 1.0;
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcSwapNeck::Reset ()
{
  // previous gaze direction
  p0 = 0.0;
  t0 = 0.0;

  // speed estimate
  dps = 0.0;
  tupd = 0;
  stable = 0.0;

  // current gaze direction and camera location
  Status(0.0, 0.0, 0.0, 0.0, 0.0);
  Update();

  // desired neck angles
  def_cmd();
  Issue();  
}


//= Reset locks and specify default commands.

void jhcSwapNeck::def_cmd ()
{
  prate = 0.0;
  trate = 0.0;
  grate = 0.0;
  plock = 0;
  tlock = 0;
  glock = 0;
}


///////////////////////////////////////////////////////////////////////////
//                             Data Exchange                             //
///////////////////////////////////////////////////////////////////////////

//= Cache new gaze angles from robot sensors (call Update to transfer).
// p is pan angle (horizontal), t is tilt angle (vertical)
// pan is measured clockwise from overhead with pan = 0 as FORWARD
// cx, cy, cz is current location of camera optical center

void jhcSwapNeck::Status (float p, float t, float cx, float cy, float cz)
{
  pang0 = p;
  tang0 = t;
  xcam0 = cx;
  ycam0 = cy;
  zcam0 = cz;
}


//= Report target position command to set sensor orientation (use Issue to refresh).
// xcmd, ycmd, zcmd are the location to point the sensor at
// gvel is the angular speeds wrt nominal
// gbid is the importance of the various commands 

void jhcSwapNeck::PosCmd (float& xcmd, float& ycmd, float& zcmd, float& gvel, int& gbid)
{
  // get target location
  xcmd = (float) gazex0;
  ycmd = (float) gazey0;
  zcmd = (float) gazez0;

  // get speed and importance
  gvel = (float) grate0;
  gbid = glock0;
}


//= Report individual angular commands for sensor orientation (use Issue to refresh).
// pan, tilt are the desired orientation for the sensor
// pvel, tvel are the angular speeds wrt nominal
// pbid, tbid are the importance of the various commands 

void jhcSwapNeck::DirCmd (float& pcmd, float& tcmd, float& pvel, float& tvel, int& pbid, int& tbid)
{
  // get aiming command 
  pcmd = (float) pstop0;
  tcmd = (float) tstop0;

  // get speeds
  pvel = (float) prate0;
  tvel = (float) trate0;

  // get command importance
  pbid = plock0;
  tbid = tlock0;
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Update pan and tilt angles of the head (load cache values with Status).
// retrieves "data" from "data0", automatically resets "lock" for new bids

void jhcSwapNeck::Update ()
{
  double dr, dt = 0.0, rmix = 1.0, slow = 10.0;
  UL32 last = tupd;

  // save previous neck angles
  p0 = pang;
  t0 = tang;

  // copy new values from hardware
  pang = pang0;
  tang = tang0;
  xcam = xcam0;
  ycam = ycam0;
  zcam = zcam0;

  // mix new speed estimate into longer term average
  tupd = jms_now();
  if (last != 0)
  {
    dt = jms_secs(tupd, last);
    dr = fabs(pang - p0) + fabs(tang - t0);      // always positive
    dps += rmix * ((dr / dt) - dps); 
  }

  // update stable time
  if (dps <= slow)
    stable = __max(0.0, stable) + dt;
  else
    stable = __min(0.0, stable) - dt;

  // set up for new target arbitration
  def_cmd();
}


//= Harvest final angle commands now that arbitration is done.
// caches "cmd" into "cmd0" for Command

void jhcSwapNeck::Issue ()      
{
  // angles or coords
  pstop0 = pstop;
  tstop0 = tstop;
  gazex0 = gazex;
  gazey0 = gazey;
  gazez0 = gazez;

  // speeds
  prate0 = prate;
  trate0 = trate;
  grate0 = grate;

  // bids
  plock0 = plock;
  tlock0 = tlock;
  glock0 = glock;
}


///////////////////////////////////////////////////////////////////////////
//                          Current Information                          //
///////////////////////////////////////////////////////////////////////////

//= Compute position and true gazing angle of camera.
// Y points forward, X to right, Z is upwards (origin = wheel midpoint)
// pan is clockwise from overhead with forward = 0
// can be modified by height of bottom of lift stage separately

void jhcSwapNeck::HeadPose (jhcMatrix& pos, jhcMatrix& aim, double lift) const
{
  if (!pos.Vector(4) || !aim.Vector(4))
    Fatal("Bad input to jhcSwapNeck::HeadPose");
  pos.SetVec3(xcam, ycam, zcam + lift);
  aim.SetVec3(pang, tang, rang);
}


///////////////////////////////////////////////////////////////////////////
//                             Goal Conversion                           //
///////////////////////////////////////////////////////////////////////////

//= Compute pan and tilt angles to center given target in camera.
// Y points forward, X to right, Z is upwards (origin = wheel midpoint)
// camera can be modified by height of bottom of lift stage separately
// NOTE: do not cache angles because camera position changes with gaze

void jhcSwapNeck::AimFor (double& p, double& t, const jhcMatrix& targ, double lift) const
{
  jhcMatrix cam(4);

  cam.SetVec3(xcam, ycam, zcam + lift);
  cam.PanTilt3(p, t, targ);
  p -= 90.0;                           // adjust so forward -> pan = 0
  if (p <= -180.0)
    p += 360.0;
}


///////////////////////////////////////////////////////////////////////////
//                           Goal Specification                          //
///////////////////////////////////////////////////////////////////////////

//= Move neck laterally to some new azimuth direction.
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapNeck::PanTarget (double pan, double rate, int bid)
{
  if (bid < plock)
    return 0;
  plock = bid;
  pstop = pan;
  prate = rate;
  return 1;
}


//= Move neck vertically to some new elevation direction.
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapNeck::TiltTarget (double tilt, double rate, int bid)
{
  if (bid < tlock)
    return 0;
  tlock = bid;
  tstop = tilt;
  trate = rate;
  return 1;
}


//= Copy parameters for motion target pose and slew speed.
// bid value must be greater than previous command to take effect
// returns 1 if newly set (both parts), 0 if pre-empted by higher priority (perhaps partially)

int jhcSwapNeck::GazeTarget (double pan, double tilt, double rate, int bid)
{
  int pok, tok;

  pok = PanTarget(pan, rate, bid);
  tok = TiltTarget(tilt, rate, bid);
  return __min(pok, tok);
}


//= Set pan and tilt targets to look at given realworld position.
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapNeck::GazeAt (double wx, double wy, double wz, double lift, double rate, int bid)
{
  if (bid < glock)
    return 0;
  glock = bid;
  gazex = wx;
  gazey = wy;
  gazez = wz;
  grate = rate;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                             Smooth Sliding                            //
///////////////////////////////////////////////////////////////////////////

//= Slew toward goal pose but linearly slow down when close.
// helps compensate for sensor lag during tracking
// Note: path might be curvy rather than linear

int jhcSwapNeck::GazeSoft (double pan, double tilt, double rate, int bid, double soft)
{
  double pf = __min(PanErr(pan, 1) / soft, 1.0), tf =  __min(PanErr(pan, 1) / soft, 1.0); 
  int pok, tok;

  pok = PanTarget(pan, pf * rate, bid);
  tok = TiltTarget(tilt, tf * rate, bid);
  return __min(pok, tok);
}


//= Slew toward aiming at target position but linearly slow down when close.
// helps compensate for sensor lag during tracking

int jhcSwapNeck::GazeSoft (const jhcMatrix& targ, double lift, double rate, int bid, double soft)
{
  double err = GazeErr(targ, lift), f = __min(err / soft, 1.0);

  return jhcGenNeck::GazeAt(targ, lift, f * rate, bid);
}


///////////////////////////////////////////////////////////////////////////
//                            Motion Progress                            //
///////////////////////////////////////////////////////////////////////////

//= Return error (in degs) between current pan and goal angle.
// can optionally give absolute value

double jhcSwapNeck::PanErr (double pan, int abs) const   
{
  double err = norm_ang(Pan() - pan); 

  return((abs > 0) ? fabs(err) : err);
}


//= Return error (in degs) between current tilt and goal angle.
// can optionally give absolute value

double jhcSwapNeck::TiltErr (double tilt, int abs) const 
{
  double err = norm_ang(Tilt() - tilt); 
  
  return((abs > 0) ? fabs(err) : err);
}


//= Keep an angle in the range -180 to +180 degrees.

double jhcSwapNeck::norm_ang (double degs) const
{
  double a = degs;

  if (a > 180.0)
    a -= 360.0 * ROUND(a / 360.0);
  else if (a <= -180.0)
    a += 360.0 * ROUND(a / 360.0);
  return a;
}


//= Gives the max absolute pan or tilt error between current gaze and target position.

double jhcSwapNeck::GazeErr (const jhcMatrix& targ, double lift, int abs) const
{
  jhcMatrix diff(4);
  double cp, ct, big;

  if (!targ.Vector(4))
    Fatal("Bad input to jhcSwapNeck::GazeErr");

  // get difference vector from camera location
  diff.Copy(targ);
  diff.IncVec3(-xcam, -ycam, -(zcam + lift));

  // make y point outwards along camera optical axis
  diff.RotPan3(-pang);
  diff.RotTilt3(-tang);

  // resolve into angles RELATIVE to camera axis
  cp = R2D * atan2(diff.X(), diff.Y());
  ct = R2D * atan2(diff.Z(), diff.Y());
  big = ((fabs(cp) > fabs(ct)) ? cp : ct);
  return((abs > 0) ? fabs(big) : big);
}


