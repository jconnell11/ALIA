// jhcGenLift.h : control interface for generic robot forklift stage
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


//= Control interface for generic robot forklift stage.
// these are the functions available to grounding kernels

class jhcGenLift
{
// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  virtual ~jhcGenLift () {}
  jhcGenLift () {}
  virtual int CommOK () {return 1;}
  virtual double LiftTol () const =0;

  // current lift information
  virtual double Height () const =0;

  // convert relative goal to absolute
  virtual double LiftGoal (double dist) const =0; 

  // lift goal specification commands
  virtual int LiftTarget (double high, double rate =1.0, int bid =10) =0;
  int LiftShift (double dz, double rate =1.0, int bid =10) 
    {return LiftTarget(Height() + dz, rate, bid);}

  // profiled motion progress
  virtual double LiftErr (double high, int abs =1) const =0;

};
