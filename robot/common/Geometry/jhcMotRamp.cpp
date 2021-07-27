// jhcMotRamp.cpp : trapezoidal velocity profiling for 3D vectors
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2016-2020 IBM Corporation
// Copyright 2021 Etaoin Systems 
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

#include "Interface/jhcMessage.h"

#include "Geometry/jhcMotRamp.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcMotRamp::jhcMotRamp () 
{
  cmd.SetSize(4);
  vel.SetSize(4);
  keep.SetSize(4);
  *rname = '\0';
  RampCfg(); 
  RampReset();
}


///////////////////////////////////////////////////////////////////////////
//                             Servo Control                             //
///////////////////////////////////////////////////////////////////////////

//= Give a motion control stop value based on current real value.
// move speed can be read by RampVel()

double jhcMotRamp::RampNext (double now, double tupd, double lead)
{
  jhcMatrix stop(4), loc(4);

  loc.SetVec3(now, 0.0, 0.0);
  RampNext(stop, loc, tupd, lead);
  return stop.X();
}


//= Give a motion control target stop position based on current real position.
// "tupd" is the time since last call, presumed to be approximate time to next call also
// special command of rt = 0 freezes control at pose where first issued (prevents drift)
// sets member variable "drem" with remaining distance to goal

void jhcMotRamp::RampNext (jhcMatrix& stop, const jhcMatrix& now, double tupd, double lead) 
{
  jhcMatrix dir(4);
  double amt;

  // make sure vectors are okay
  if (!stop.Vector(4) || !now.Vector(4) || (tupd <= 0.0))
    Fatal("Bad input to jhcMotRamp::RampNext");

  // check frozen state ("rt" will be from winning bid)
  if (rt != 0.0)
    ice = 0;
  else
  {
    if (ice <= 0) 
      keep.Copy(now);                  // remember initial pose
    cmd.Copy(keep);
    ice = 1;
  }

  // update velocity based on current position and accelerations
  drem = goal_dir(dir, now, tupd);
  alter_vel(dir, drem, tupd);

  // get stopping position along vector to goal (not along velocity)
  amt = sp * tupd * lead;
  amt = __min(amt, drem);
  stop.RelFrac3(now, dir, amt);
  if (done < 0)
    stop.CycNorm3();
}


//= Find vector in direction of target from current position.
// also monitors whether progress is being made (distance is decreasing)
// returns remaining distance to target (unsigned)

double jhcMotRamp::goal_dir (jhcMatrix& dir, const jhcMatrix& now, double tupd)
{
  double dist;

  // find direction to target and remaining distance 
  if (done < 0)
    dist = dir.RotDir3(cmd, now);
  else
    dist = dir.DirVec3(cmd, now);

  // check if making progress
  if ((d0 - dist) > fabs(done))
  {
    d0 = dist;
    stuck = 0.0;
  }
  else
  {
    d0 = __max(d0, dist);
    stuck += tupd;
  }
  return dist;
}


//= Change velocity so as to move closer to target.
// normally scales accelerations to give same trajectory regardless of rate
// makes sure that limited deceleration will cause stop at goal "cmd"
// <pre>
//        ^
//     sp |       +-----------
//        |     /
//        |   /
//        | /
//       -+------------------->
//                       dist
// </pre>
// if goal suddenly becomes closer, will cut speed instantly or by dmax (safer)
// sets member variable "sp" to scalar speed of new velocity vector
// sets member variable "rev" positive to indicate travel in wrong direction

void jhcMotRamp::alter_vel (const jhcMatrix& dir, double dist, double tupd) 
{
  double a, d, vmax, dot, vstop, v2;

  // compute commanded accelerations and cruise speed
  a = ((rt < 0.0) ? astd : rt * rt * astd);
  d = ((rt < 0.0) ? dstd : rt * rt * dstd);
  vmax = fabs(rt) * vstd;

  // if going wrong way decelerate toward zero
  if ((dot = dir.DotVec3(vel)) < 0.0)
    vel.AddFrac3(dir, d * tupd);
  else
    vel.AddFrac3(dir, a * tupd);

  // find resulting unsigned scalar speed
  if (done < 0)
    sp = vel.MaxAbs3();
  else
    sp = vel.LenVec3();

  // determine final deceleration limited target speed
  if (dot >= 0.0)
  {
    vstop = sqrt(2.0 * d * dist);
    vmax = __min(vstop, vmax);    
  }                           
  if (sp <= vmax)
    return;

  // scale velocity to be slower if needed
  if (dmax > 0.0)
  {
    v2 = sp - dmax * tupd;    
    vmax = __max(v2, vmax);
  }
  vel.ScaleVec3(vmax / sp);   
  sp = vmax;
}


//= Generate component-wise error vector between current and target positions.
// compares remembered last position and current command

void jhcMotRamp::RampErr (jhcMatrix& err, const jhcMatrix& loc, int abs) const
{
  if (!err.Vector(4))
    Fatal("Bad input to jhcMotRamp::RampErr");
  if (done < 0) 
    err.CycDiff3(loc, cmd); 
  else 
    err.DiffVec3(loc, cmd);
  if (abs > 0)
    err.Abs();
}


///////////////////////////////////////////////////////////////////////////
//                          Trajectory Queries                           //
///////////////////////////////////////////////////////////////////////////

//= compute a goal command value that can be reached at maximum deceleration.
// uses infinite deceleration when within "skid" of endpoint (to stop drift)
// sometimes a good default if no other commands - gradual vs sudden stop

double jhcMotRamp::SoftStop (double now, double skid, double rate) 
{
  jhcMatrix loc(4), stop(4);

  loc.SetVec3(now, 0.0, 0.0);
  SoftStop(stop, loc, skid, rate);
  return stop.X();
}


//= Compute a goal command vector that can be reached at maximum deceleration.
// uses infinite deceleration when within "skid" of endpoint (to stop drift)
// sometimes a good default if no other commands - gradual vs sudden stop

void jhcMotRamp::SoftStop (jhcMatrix& stop, const jhcMatrix& now, double skid, double rate) 
{
  jhcMatrix dir(4);
  double d = ((rate < 0.0) ? dstd : rate * rate * dstd), dist = 0.5 * sp * sp / d;

  // check arguments and simple cases
  if (!stop.Vector(4) || !now.Vector(4))
    Fatal("Bad input to jhcMotRamp::SoftStop");
  if (dist <= skid)
  {
    stop.Copy(now);
    return;
  }

  // set goal a short distance along current vector
  dir.UnitVec3(vel);
  stop.RelFrac3(now, dir, dist - skid);
  if (done < 0)
    stop.CycNorm3();
}


//= Estimate time (in secs) to move a certain distance using given rate.
// if rate < 0 then does not scale acceleration (for snappier response)
// assumes velocity starts and stops at zero

double jhcMotRamp::find_time (double dist, double rate) const
{
  double v, ad, t, r = fabs(rate);
  
  // limit velocity and possibly acceleration too
  v = fabs(rate) * vstd;
  ad = 2.0 * astd * dstd / (astd + dstd);
  if (rate > 0.0)
    ad *= (r * r);

  // see if triangular or trapezoidal profile
  if (dist <= (v * v / ad))
    t = 2.0 * sqrt(dist / ad);
  else
    t = (dist / v) + (v / ad);
  return t;
}


//= Pick a rate to move a certain distance in the given time.
// if secs < 0 then does not scale acceleration (for snappier response)

double jhcMotRamp::find_rate (double dist, double secs, double rmax) const
{
  double r, v, t2, ad = 2.0 * astd * dstd / (astd + dstd), t = fabs(secs);

  // simple case (wishful thinking)
  if (t == 0.0)
    return((secs > 0.0) ? rmax : -rmax);

  // see if both acceleration and velocity should be scaled
  if (secs > 0.0)
  {
    // check if triangular (covers distance at half peak velocity)
    v = 2.0 * dist / t;
    if (v > (rmax * vstd))
      r = ((dist / vstd) + (vstd / ad)) / t;     // trapezoidal 
    else
      r = v / sqrt(ad * dist);                   // triangular
  }
  else          
  {
    // compute time required for triangular profile to reach goal
    t2 = 2.0 * sqrt(dist / ad);
    v = 2.0 * dist / t2;

    // possibly compute cruise speed for trapezoidal profile instead
    if ((t2 < t) || (v > (rmax * vstd)))
      v = 0.5 * ad * t * (1.0 - sqrt(1.0 - (4.0 * dist / (ad * t * t))));
    r = v / vstd;
  }

  // clamp rate and set sign
  r = __min(r, rmax);
  return((secs > 0.0) ? r : -r);
}


//= Find difference between two vector values.

double jhcMotRamp::find_dist (const jhcMatrix& p2, const jhcMatrix& p1) const
{
  if (done < 0)
    return p2.RotDiff3(p1);
  return p2.PosDiff3(p1);
}


//= Find difference between two scalar values.

double jhcMotRamp::find_dist (double p2, double p1) const
{
  double d = p2 - p1;

  if (done < 0)
  {
    while (d > 180.0)
      d -= 360.0;
    while (d <= -180.0)
      d += 360.0;
  }
  return fabs(d);
}
