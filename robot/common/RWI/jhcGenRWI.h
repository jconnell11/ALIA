// jhcGenRWI.h : generic post-processed sensors and high level commands
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

#include "Body/jhcGenArm.h"            // common robot
#include "Body/jhcGenBase.h"
#include "Body/jhcGenLift.h"
#include "Body/jhcGenNeck.h"


//= Generic interface to post-processed sensors and high level commands.
// holds pointers to body components and input analysis packages
// these are the elements available to grounding kernels

class jhcGenRWI 
{
// PUBLIC MEMBER VARIABLES
public:
  // robot and subcomponents
  jhcGenBase *base;
  jhcGenNeck *neck;
  jhcGenLift *lift;
  jhcGenArm  *arm;

  // sensor analysis packages
/*
  jhcStare3D s3;                       // head finder using depth
  jhcFaceName fn;                      // face ID and gaze for heads
  jhcSpeaker tk;                       // sound location vs head
  jhcLocalOcc nav;                     // navigation obstacles
  jhcSurfObjs sobj;                    // depth-based object detection
  jhcTable tab;                        // supporting surfaces
*/  

// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  virtual ~jhcGenRWI () {};
  jhcGenRWI () {base = NULL, neck = NULL; lift = NULL; arm = NULL;}

  // runtime status
  virtual bool Ghost ()     {return false;}
  virtual bool Accepting () {return true;}

};




