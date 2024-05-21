// jhcSwapNeck.cpp : control interface for external robot camera aiming
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
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// "rpt" is generally level of disgnostic printouts desired
// returns 1 always

int jhcSwapNeck::Reset (int rpt)
{
  // previous gaze direction
  p0 = 0.0;
  t0 = 0.0;

  // current gaze direction
  pang = 0.0;
  tang = 0.0;

  // current camera location
  xcam = 0.0;
  ycam = 0.0;
  zcam = 0.0;
  return def_cmd();
}


//= Reset locks and specify default commands.
// returns 1 always for convenience

int jhcSwapNeck::def_cmd ()
{
  // pan command
  prate = 0.0;
  plock = 0;

  // tilt command
  trate = 0.0;
  tlock = 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Get new gaze angles from robot sensors (indirectly).
// p is pan angle (horizontal), t is tilt angle (vertical)
// cx, cy, cz is current location of camera optical center
// clears command priorites for next cycle
// override this function to directly query robot (if available) 
// returns 1 if sensors acquired, 0 or negative for problem

int jhcSwapNeck::Status (float p, float t, float cx, float cy, float cz)
{
  p0 = pang;
  t0 = tang;
  pang = p;
  tang = t;
  xcam = cx;
  ycam = cy;
  zcam = cz;
  return def_cmd();
}


//= Send angular command to robot actuators (indirectly).
// pan, tilt are the desired angles for the head
// pvel, tvel are the angular speeds wrt nominal
// pbid, tbid are the importance of the pan and tilt commands 
// override this function to directly drive robot (if available) 
// returns 1 if command sent, 0 or negative for problem

int jhcSwapNeck::Command (float& pan, float& tilt, float& pvel, float& tvel, int& pbid, int& tbid)
{
  // get aiming command and speed
  pan = (float) pstop;
  tilt = (float) tstop;
  pvel = (float) prate;
  tvel = (float) trate;

  // get command importance
  pbid = plock;
  tbid = tlock;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Current Information                          //
///////////////////////////////////////////////////////////////////////////

//= Compute position and true gazing angle of camera.
// Y points forward, X to right, Z is upwards (origin = wheel midpoint)
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

  if (!targ.Vector(4))
    Fatal("Bad input to jhcSwapNeck::AimFor");
  cam.SetVec3(xcam, ycam, zcam + lift);
  cam.PanTilt3(p, t, targ);
  p -= 90.0;                           // forward = 90 degs
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
// only approximate at start since head position changes
// does NOT coordinate pan and tilt speeds for straight arc

int jhcSwapNeck::GazeAt (const jhcMatrix& targ, double lift, double rate, int bid)
{
  double pan, tilt;

  AimFor(pan, tilt, targ, lift);
  return GazeTarget(pan, tilt, rate, rate, bid);
}


///////////////////////////////////////////////////////////////////////////
//                       Eliminate Residual Error                        //
///////////////////////////////////////////////////////////////////////////

//= Rotate to traverse some angle in a specific amount of time.
// pan and tilt moves should end at the same time

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
  double pan, tilt;

  AimFor(pan, tilt, targ, lift);
  return GazeFix(pan, tilt, secs, bid);
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
  double pan, tilt;

  AimFor(pan, tilt, targ, lift);
  return GazeErr(pan, tilt);      
}


