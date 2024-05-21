// jhcSwapCoord.h : parsing, learning, and control for external robot
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

#include "Data/jhcParam.h"             // common video

#include "Acoustic/jhcAliaSpeech.h"    // common audio 

#include "Body/jhcSwapArm.h"           // common robot
#include "Body/jhcSwapBase.h"
#include "Body/jhcSwapLift.h"
#include "Body/jhcSwapNeck.h"
#include "Grounding/jhcBallistic.h"
#include "RWI/jhcSwapRWI.h"


//= Parsing, learning, and control for external robot.

class jhcSwapCoord : public jhcAliaSpeech
{
// PUBLIC MEMBER VARIABLES
public:
  // possibly shared components
  jhcSwapNeck neck0;
  jhcSwapArm arm0;
  jhcSwapLift lift0;
  jhcSwapBase base0;
  jhcSwapRWI rwi;                     

  // extra grounding kernels
  jhcBallistic ball; 

  // kernel debugging messages
  jhcParam kps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  ~jhcSwapCoord ();
  jhcSwapCoord ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname);

  // main functions
  int Reset (const char *dir =NULL, const char *rname =NULL, int silent =0);
  int Done (int face =0);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int kern_params (const char *fname);

};
