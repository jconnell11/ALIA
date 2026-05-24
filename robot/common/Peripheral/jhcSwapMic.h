// jhcSwapMic.h : direction-of-arrival from ReSpeaker Mic Array V2.0
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2025-2026 Etaoin Systems
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

#include "Peripheral/jhcGenMic.h"


//= Direction-of-arrival from ReSpeaker Mic Array V2.0 (or others).
// only assumes 0 = no sound, 1-360 = direction (CCW from 360 front)
// Note: does NOT double-buffer raw direction input from robot body

class jhcSwapMic : public jhcGenMic
{
// PRIVATE MEMBER VARIABLES
private:
  int v0;                    // last speech recognition status


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSwapMic ();
  jhcSwapMic ();
 
  // main functions
  int Reset ();
  int Update (int voice =0);

  // sensor input
  void Smooth (int dir);
  void Sensor (int dir, double hpan, int sprc);


};
