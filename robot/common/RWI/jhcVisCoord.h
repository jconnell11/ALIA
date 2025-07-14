// jhcVisCoord.h : language, perceptions, learning, and control for external robot
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

#include "Data/jhcImg.h"               // common video
#include "Data/jhcParam.h"      

#include "Acoustic/jhcAliaSpeech.h"    // common audio 

#include "Body/jhcSwapBody.h"          // common robot
#include "Grounding/jhcBallistic.h"
#include "Grounding/jhcManipulate.h"
#include "Grounding/jhcSceneVis.h"
#include "Grounding/jhcSocial.h"
#include "Grounding/jhcSupport.h"
#include "Peripheral/jhcSwapMic.h"
#include "RWI/jhcVisGrok.h"


//= Language, perceptions, learning, and control for external robot.
// Note: jhcSwapCoord used by alia_act for just actuator control

class jhcVisCoord : public jhcAliaSpeech
{
// PUBLIC MEMBER VARIABLES
public:
  // shared hardware components
  jhcSwapBody body0;
  jhcSwapMic mic0;

  // background vision processing
  jhcVisGrok rwi;                     

  // extra grounding kernels
  jhcBallistic ball; 
  jhcSceneVis svis;
  jhcSupport sup;
  jhcSocial soc;
  jhcManipulate man;

  // parameter sets
  jhcParam kps;

  // system has been reset
  int up;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  ~jhcVisCoord ();
  jhcVisCoord ();
  int Stare () const {return (rwi.fn).AnyGaze();}

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname);

  // main functions
  int Reset (const char *dir =NULL, const char *rname =NULL, int silent =0);
  void Respond ();
  int Done (int face =0, int batt =-1);

  // output images
  int GetView (void *pels, int fmt) const;
  int GetMap (void *pels, int fmt);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int kern_params (const char *fname);

  // output images
  int get_bgr_bot (void *pels, const jhcImg& src) const;
  int get_bgr_top (void *pels, const jhcImg& src) const;

};
