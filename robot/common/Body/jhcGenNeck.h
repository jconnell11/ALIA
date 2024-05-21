// jhcGenNeck.h : control interface for generic robot camera aiming
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

#pragma once

#include "jhcGlobal.h"

#include "Geometry/jhcMatrix.h"        // common robot


//= Control interface for generic robot camera aiming.
// these are the functions available to grounding kernels

class jhcGenNeck
{
// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  virtual ~jhcGenNeck () {}
  jhcGenNeck () {}
  virtual int CommOK () {return 1;}
  virtual void AimFor (double& p, double& t, const jhcMatrix& targ, double lift =0.0) const =0;

  // current gaze information
  virtual double Pan () const =0;
  virtual double Tilt () const =0;
  virtual void HeadPose (jhcMatrix& pos, jhcMatrix& aim, double lift =0.0) const =0;
  virtual bool Saccade (double plim =3.5, double tlim =1.0) const =0;

  // goal specifying commands for view
  virtual int PanTarget (double pan, double rate =1.0, int bid =10) =0;
  virtual int TiltTarget (double tilt, double rate =1.0, int bid =10) =0;
  virtual int GazeTarget (double pan, double tilt, double p_rate =1.0, double t_rate =0.0, int bid =10) =0;
  virtual int GazeAt (const jhcMatrix& targ, double lift, double rate =1.0, int bid =10) =0;

  // eliminate residual error
  virtual int GazeFix (double pan, double tilt, double secs =0.5, int bid =10) =0;
  virtual int GazeFix (const jhcMatrix& targ, double lift, double secs =0.5, int bid =10) =0;

  // profiled motion progress
  virtual double PanErr (double pan, int abs =1) const =0;
  virtual double TiltErr (double tilt, int abs =1) const =0;
  virtual double GazeErr (double pan, double tilt) const =0;
  virtual double GazeErr (const jhcMatrix& targ, double lift =0.0) const =0;

};
