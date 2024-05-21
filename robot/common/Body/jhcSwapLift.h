// jhcSwapLift.h : control interface for external robot forklift stage
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

#include <math.h>

#include "Body/jhcGenLift.h"           // common robot


//= Control interface for external robot forklift stage.
// no actual control code -- merely exchanges variable values

class jhcSwapLift : public jhcGenLift
{
// PRIVATE MEMBER VARIABLES
private:
  // sensor data
  double ht;                 // current height above floor

  // command info
  int llock;                 // current command importance
  double lstop;              // desired end height
  double lrate;              // desired motion speed


// PROTECTED MEMBER VARIABLES
protected:
  double ldone;              // motion endpoint tolerance


// PUBLIC MEMBER VARIABLES
public:
  // hardware status
  int lok;                             


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSwapLift ();
  jhcSwapLift ();
  int CommOK () {return lok;}
  double LiftTol () const {return ldone;}

  // configuration
  int Reset (int rpt =0);

  // core interaction
  int Status (float lvl);
  int Command (float& hdes, float& sp, int& bid);

  // current lift information
  double Height () const {return ht;}

  // goal conversion
  double LiftGoal (double dist) const {return(ht + dist);}

  // lift goal specification commands
  int LiftTarget (double high, double rate =1.0, int bid =10);

  // motion progress
  double LiftErr (double high, int abs =1) const 
    {return((abs > 0) ? fabs(ht - high) : ht - high);}


// PROTECTED MEMBER FUNCTIONS
protected:
  // configuration
  int def_cmd ();

};
