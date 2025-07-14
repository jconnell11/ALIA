// jhcSwapNeck.h : control interface for external robot camera aiming
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

#include "Body/jhcGenNeck.h"           // common robot


//= Control interface for external robot camera aiming.
// no actual control code -- merely exchanges variable values
// provides support for double buffering (if desired)

class jhcSwapNeck : public jhcGenNeck
{
// PRIVATE MEMBER VARIABLES
private:
  // sensor data
  double pang, tang, rang;   // current camera angles
  double xcam, ycam, zcam;   // location of camera center
  double p0, t0;             // angles on previous cycle
  int stable;                // number of no-motion cycles

  // command info
  double pstop, tstop, gazex, gazey, gazez;      // desired angles or coords
  double prate, trate, grate;                    // desired rotation speed
  int plock, tlock, glock;                       // current command importance

  // double buffered input
  double pang0, tang0, rang0;   
  double xcam0, ycam0, zcam0;   

  // double buffered output
  double pstop0, tstop0, gazex0, gazey0, gazez0;
  double prate0, trate0, grate0;
  int plock0, tlock0, glock0;


// PUBLIC MEMBER VARIABLES
public:
  // hardware status
  int nok;                             


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSwapNeck ();
  jhcSwapNeck ();
  int CommOK () {return nok;}

  // configuration
  void Reset ();

  // data exchange
  void Status (float pan, float tilt, float cx, float cy, float cz);
  void PosCmd (float& xcmd, float& ycmd, float& zcmd, float& gvel, int& gbid);
  void DirCmd (float& pcmd, float& tcmd, float& pvel, float& tvel, int& pbid, int& tbid);

  // core interaction
  void Update ();
  void Issue ();       

  // current gaze information
  double Pan () const  {return pang;}
  double Tilt () const {return tang;}
  void HeadPose (jhcMatrix& pos, jhcMatrix& aim, double lift =0.0) const;
  bool Saccade (double plim =3.5, double tlim =1.0) const
    {return((fabs(pang - p0) > plim) || (fabs(tang - t0) > tlim));}
  int Stare () const {return stable;}

  // goal conversion
  void AimFor (double& p, double& t, const jhcMatrix& targ, double lift =0.0) const;

  // goal specifying commands for view
  int PanTarget (double pan, double rate =1.0, int bid =10);
  int TiltTarget (double tilt, double rate =1.0, int bid =10);
  int GazeTarget (double pan, double tilt, double p_rate =1.0, double t_rate =0.0, int bid =10);
  int GazeAt (const jhcMatrix& targ, double lift, double rate =1.0, int bid =10);

  // eliminate residual error
  int GazeFix (double pan, double tilt, double secs =0.5, int bid =10);
  int GazeFix (const jhcMatrix& targ, double lift, double secs =0.5, int bid =10);

  // motion progress
  double PanErr (double pan, int abs =1) const;
  double TiltErr (double tilt, int abs =1) const;
  double GazeErr (double pan, double tilt) const
    {return __max(PanErr(pan, 1), TiltErr(tilt, 1));}
  double GazeErr (const jhcMatrix& targ, double lift =0.0) const;


// PROTECTED MEMBER FUNCTIONS
protected:
  // configuration
  void def_cmd ();

  // motion progress
  double norm_ang (double degs) const;

};
