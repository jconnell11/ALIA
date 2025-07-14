// jhcSwapLift.h : control interface for external robot forklift stage
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

#include "Body/jhcGenLift.h"           // common robot


//= Control interface for external robot forklift stage.
// no actual control code -- merely exchanges variable values
// provides support for double buffering (if desired)

class jhcSwapLift : public jhcGenLift
{
// PRIVATE MEMBER VARIABLES
private:
  // sensor data
  double ht;                 // current height above floor

  // command info
  double lstop;              // desired end height
  double lrate;              // desired motion speed
  int llock;                 // current command importance

  // double buffered input
  double ht0;

  // double buffered output
  double lstop0, lrate0;
  int llock0;


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
  void Reset ();

  // data exchange
  void Status (float lvl);
  void Command (float& hdes, float& sp, int& bid);

  // core interaction
  void Update ();
  void Issue ();       

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
  void def_cmd ();

};
