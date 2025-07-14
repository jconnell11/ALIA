// jhcSwapNeck.cpp : control interface for external robot camera aiming
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

#include "Interface/jhcMessage.h"      // common video

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
  stable = 30;
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
  stable = 0;

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
  double ndone = 1.0, atol = 5.0;

  // save previous neck angles
  p0 = pang;
  t0 = tang;

  // copy new values from hardware
  pang = pang0;
  tang = tang0;
  xcam = xcam0;
  ycam = ycam0;
  zcam = zcam0;

  // update stable count (extra test if pan implemented via base rotation)
  if ((fabs(pang - p0) <= ndone) && (fabs(tang - t0) <= ndone) &&
      ((plock <= 0) || (prate == 0.0) || (fabs(pstop) <= atol)))
    stable = __max(0, stable) + 1;
  else
    stable = __min(0, stable) - 1;

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
/*
  jhcMatrix diff(4);

  if (!targ.Vector(4))
    Fatal("Bad input to jhcSwapNeck::AimFor");

  // get difference vector from camera location
  diff.Copy(targ);
  diff.IncVec3(-xcam, -ycam, -(zcam + lift));

  // rotate difference by current viewing direction
  diff.RotPan3(-pang);
  diff.RotTilt3(-tang);

  // resolve into angles RELATIVE to camera axis
  t = diff.TiltVec3();
  p = diff.PanVec3(); 
*/
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
  if (bid <= plock)
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
  if (bid <= tlock)
    return 0;
  tlock = bid;
  tstop = tilt;
  trate = rate;
  return 1;
}


//= Copy parameters for motion target pose and slew speed.
// if tilt rate = 0 then copies pan rate
// bid value must be greater than previous command to take effect
// returns 1 if newly set (both parts), 0 if pre-empted by higher priority (perhaps partially)

int jhcSwapNeck::GazeTarget (double pan, double tilt, double p_rate, double t_rate, int bid)
{
  double r = ((t_rate != 0.0) ? t_rate : p_rate);
  int pok, tok;

  pok = PanTarget(pan, p_rate, bid);
  tok = TiltTarget(tilt, r, bid);
  return __min(pok, tok);
}


//= Set pan and tilt targets to look at given position.
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapNeck::GazeAt (const jhcMatrix& targ, double lift, double rate, int bid)
{
  if (bid <= glock)
    return 0;
  glock = bid;
  gazex = targ.X();
  gazey = targ.Y();
  gazez = targ.Z();
  grate = rate;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                       Eliminate Residual Error                        //
///////////////////////////////////////////////////////////////////////////

//= Rotate to traverse some angle in a specific amount of time.
// pan and tilt moves should end at the same time, assumes 90 dps max

int jhcSwapNeck::GazeFix (double pan, double tilt, double secs, int bid)
{
  double r, dps = 90.0, slew = dps * secs;
  double pr = fabs(pan - pang) / slew, tr = fabs(tilt - tang) / slew;

  r = __max(pr, tr);
  if (r > 1.0)
  {
    pr /= r;
    tr /= r;
  }
  return GazeTarget(pan, tilt, pr, tr, bid);
}


//= Move gaze toward target position reducing residual over given number of seconds.

int jhcSwapNeck::GazeFix (const jhcMatrix& targ, double lift, double secs, int bid)
{
  double r, dps = 90.0, slew = dps * secs;

  r = GazeErr(targ, lift) / slew;
  r = __min(r, 1.0);
  return GazeAt(targ, lift, r, bid);
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

double jhcSwapNeck::GazeErr (const jhcMatrix& targ, double lift) const
{
  jhcMatrix diff(4);
  double cp, ct;

  if (!targ.Vector(4))
    Fatal("Bad input to jhcSwapNeck::GazeErr");

  // get difference vector from camera location
  diff.Copy(targ);
  diff.IncVec3(-xcam, -ycam, -(zcam + lift));

  // make z point outwards along camera optical axis
  diff.RotPan3(-pang);
  diff.RotTilt3(tang - 90.0);

  // resolve into angles RELATIVE to camera axis
  cp = R2D * atan2(diff.X(), diff.Z());
  ct = R2D * atan2(diff.Y(), diff.Z());
  return __max(fabs(cp), fabs(ct)); 
}


