// jhcMotRamp.h : trapezoidal velocity profiling for 3D vectors
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

#ifndef _JHCMOTRAMP_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMOTRAMP_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Geometry/jhcMatrix.h"


//= Helper functions for trapezoidal velocity profiling for 3D vectors.
// No longer has system follow trajectory of idealized point as this led
// to herky-jerky motion. Now simply changes command speed based on distance 
// to target and current speed. Instead of an expected time to completion 
// there is a "no progress" indicator saying the motion is stuck (or done).
// Can also be used with angle sets or simple scalar values. If "done" < 0
// then treats vector coefficients as angles (i.e. mod 360 degrees).
// NOV 2019: Added velocity deadband, moved progress to jhcTimedFcns::Stuck
// MAY 2020: Added vector velocity to more closely match physical systems
// special command of rt = 0 freezes control at pose where first issued

class jhcMotRamp
{
// PRIVATE MEMBER VARIABLES
private:
  // state variables
  jhcMatrix vel;     /** Motion for ideal trajectory. */
  jhcMatrix keep;    /** Pose to use with freeze.     */
  double sp;         /** Speed along trajectory.      */
  int ice;

  // progress monitoring
  double drem;      /** Current distance from goal.    */
  double d0;        /** Last distance during progress. */
  double stuck;     /** Seconds since progress made.   */


// PUBLIC MEMBER VARIABLES
public:
  // debugging aid
  char rname[80];

  // profile characteristics
  double vstd;     /** Standard speed for moves at rate 1.  */
  double astd;     /** Standard acceleration for pursuit.   */
  double dstd;     /** Standard deceleration for goal area. */
  double dmax;     /** Maximum emergency deceleration.      */
  double done;     /** Mininum progress move (cyc if neg).  */ 

  // motion command
  jhcMatrix cmd;   /** Overall move goal position. */
  double rt;       /** Rate for current motion.    */


// PUBLIC MEMBER FUNCTIONS
public:
  // construction and configuration
  jhcMotRamp ();
  void RampCfg (double v =90.0, double a =180.0, double d =180.0, double tol =2.0, double e =0.0)
    {vstd = v; astd = a; dstd = d; done = tol; dmax = e;} 
  void RampReset () 
    {vel.Zero(); sp = 0.0; d0 = 0.0; stuck = 0.0; rt = 1.0; ice = 0;} 

  // goal specification
  void RampTarget (double val, double rate =1.0) 
    {cmd.SetVec3(val, 0.0, 0.0); rt = rate;}
  void RampTarget (const jhcMatrix& val, double rate =1.0) 
    {cmd.Copy(val); rt = rate;}
  void RampInc (double amt) 
    {cmd.VInc(0, amt); rt = 1.0;}

  // servo control
  double RampNext (double now, double tupd =0.033, double lead =3.0);
  void RampNext (jhcMatrix& stop, const jhcMatrix& now, double tupd =0.033, double lead =3.0);
  void RampErr (jhcMatrix& err, const jhcMatrix& loc, int abs =1) const;
  double RampDist (const jhcMatrix& loc) const {return find_dist(loc, cmd);}
  double RampDist (double loc) const {return find_dist(loc, cmd.X());}
  double RampDone () const {return stuck;}

  // trajectory queries
  double RampTime (double p2, double p1, double rate =1.0) const
    {return find_time(find_dist(p2, p1), rate);}
  double RampTime (const jhcMatrix &p2, const jhcMatrix& p1, double rate =1.0) const
    {return find_time(find_dist(p2, p1), rate);}
  double RampRate (double p2, double p1, double secs =0.5, double rmax =1.5) const
    {return find_rate(find_dist(p2, p1), secs, rmax);}
  double RampRate (const jhcMatrix &p2, const jhcMatrix& p1, double secs =0.5, double rmax =1.5) const
    {return find_rate(find_dist(p2, p1), secs, rmax);}
  void SoftStop (jhcMatrix& stop, const jhcMatrix& now, double skid =0.0, double rate =1.5);
  double SoftStop (double now, double skid =0.0, double rate =1.5);

  // read only state
  double RampVel (double dead =0.0) const {return((drem > dead) ? sp : 0.0);}
  double RampAxis (int i =0) const {return vel.VRefChk(i);}
  double RampCmd (int i =0) const  {return cmd.VRefChk(i);}
  int RampFrozen () const {return ice;}


// PRIVATE MEMBER FUNCTIONS
private:
  // trajectory generation
  double goal_dir (jhcMatrix& dir, const jhcMatrix& now, double tupd);
  void alter_vel (const jhcMatrix& dir, double dist, double tupd);
  
  // trajectory queries
  double find_time (double dist, double rate) const;
  double find_rate (double dist, double secs, double rmax) const;
  double find_dist (const jhcMatrix& p2, const jhcMatrix& p1) const;
  double find_dist (double p2, double p1) const;


};


#endif  // once




