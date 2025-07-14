// jhcSwapBase.h : control interface for external robot mobile platform
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

#pragma once

#include "jhcGlobal.h"

#include <math.h>

#include "Data/jhcParam.h"             // common video

#include "Geometry/jhcMatrix.h"        // common robot

#include "Body/jhcGenBase.h"           


//= Control interface for external robot mobile platform.
// no actual control code -- merely exchanges variable values
// provides support for double buffering (if desired)

class jhcSwapBase : public jhcGenBase
{
// PRIVATE MEMBER VARIABLES
private:
  // sensor data
  double along, ortho, dr;   // motion changes on last cycle
  double xmap, ymap;         // current robot location in map
  double trav, wind;         // accumulated path and windup

  // speed estimation
  double ips, dps;           // smoothed travel and turn speeds
  UL32 tupd;                 // last time update was called
  int parked;                // how long the robot has stopped

  // command info
  double mstop, tstop;       // desired odometric end
  double mdir;               // angle of motion wrt centerline
  double mrate, trate;       // desired motion speed
  int mlock, tlock;          // current command importance

  // double buffered pose input
  double trav0, wind0, xmap0, ymap0;

  // double buffered command output
  double mstop0, tstop0, mdir0, mrate0, trate0;
  int mlock0, tlock0;


// PROTECTED MEMBER VARIABLES
protected:
  // motion control parameters
  double msp, tsp, mdone, tdone;        


// PUBLIC MEMBER VARIABLES
public:
  // motion control parameters
  jhcParam cps;

  // hardware status
  int bok;                             


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSwapBase ();
  jhcSwapBase ();
  int CommOK () {return bok;}
  double MoveTol () const {return mdone;}
  double TurnTol () const {return tdone;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL) {return ctrl_params(fname);}
  int SaveVals (const char *fname) const {return cps.SaveVals(fname);}

  // configuration
  void Reset ();
  int Zero ();

  // data exchange
  void Status (float path, float spin, float mx, float my); 
  void Command (float& dist, float& ang, float& skew, float& mvel, float& rvel, int& mbid, int& rbid);
  double TravelRate () const {return mrate0;}
               
  // core interaction
  void Update ();
  void Issue ();       

  // current position information
  double Travel () const {return trav;}  
  double WindUp () const {return wind;}
  int Static () const    {return parked;}

  // relative goal adjustment
  double StepFwd () const  {return along;}
  double StepSide () const {return ortho;}
  double StepTurn () const {return dr;}
  void AdjustTarget (jhcMatrix& pos) const;
  double AdjustAng (double& ang) const;

  // motion goal specification commands
  int MoveAbsolute (double tr, double rate =1.0, int bid =10, double skew =0.0); 
  int TurnAbsolute (double hd, double rate =1.0, int bid =10);

  // eliminate residual error
//  int MoveFix (double dist, double secs =0.5, double rmax =1.5, int bid =10)
//    {return MoveTarget(dist, MoveRate(dist, secs, rmax), bid);}
  int TurnFix (double ang, double secs =0.5, double rmax =1.5, int bid =10);

  // motion progress
  double MoveErr (double mgoal) const {return fabs(mgoal - trav);}
  double TurnErr (double tgoal) const {return fabs(tgoal - wind);}


// PROTECTED MEMBER FUNCTIONS
protected:
  // configuration
  void def_cmd ();


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int ctrl_params (const char *fname);

};
