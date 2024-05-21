// jhcGenBase.h : control interface for generic robot mobile platform
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


//= Control interface for generic robot mobile platform.
// these are the functions available to grounding kernels

class jhcGenBase
{
// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  virtual ~jhcGenBase () {}
  jhcGenBase () {}
  virtual int CommOK () {return 1;}
  virtual double MoveTol () const =0;
  virtual double TurnTol () const =0;

  // current position information
  virtual int Zero () =0;
  virtual double Travel () const =0;  
  virtual double WindUp () const =0;
  virtual double TravelRate () const =0;
  virtual int Static () const =0;

  // relative goal adjustment
  virtual double StepFwd () const =0;
  virtual double StepSide () const =0;
  double StepLeft () const {return -StepSide();}
  virtual double StepTurn () const =0;
  virtual void AdjustTarget (jhcMatrix& pos) const =0;
  virtual double AdjustAng (double& ang) const =0;

  // convert relative goal to absolute
  double MoveGoal (double dist) const {return(Travel() + dist);}
  double TurnGoal (double ang) const  {return(WindUp() + ang);}

  // motion goal specification commands
  virtual int MoveAbsolute (double tr, double rate =1.0, int bid =10, double skew =0.0) =0; 
  virtual int TurnAbsolute (double hd, double rate =1.0, int bid =10) =0;
  int MoveTarget (double dist, double rate =1.0, int bid =10, double skew =0.0)
    {return MoveAbsolute(MoveGoal(dist), rate, bid, skew);}
  int TurnTarget (double ang, double rate =1.0, int bid =10)
    {return TurnAbsolute(TurnGoal(ang), rate, bid);}

  // eliminate residual error
  virtual int TurnFix (double ang, double secs =0.5, double rmax =1.5, int bid =10) =0;

  // motion progress
  virtual double MoveErr (double mgoal) const =0;
  virtual double TurnErr (double tgoal) const =0;

};
