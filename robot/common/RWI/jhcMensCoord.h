// jhcMensCoord.h : language processing and perception for Manus robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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

#ifndef _JHCMENSCOORD_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMENSCOORD_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Action/jhcAliaChart.h"       // common robot
#include "Body/jhcManusBody.h"         
#include "Grounding/jhcBasicAct.h"
#include "Grounding/jhcLocalSeq.h"
#include "Grounding/jhcSoundFcn.h"
#include "Grounding/jhcTargetVis.h"
#include "RWI/jhcManusRWI.h"

#include "Acoustic/jhcAliaSpeech.h"   


//= Language processing and perception for Manus robot.

class jhcMensCoord : public jhcAliaSpeech
{
// PRIVATE MEMBER VARIABLES
private:


// PUBLIC MEMBER VARIABLES
public:
  // possibly shared components
  jhcManusBody body;           
  jhcManusRWI rwi;                     

  // extra grounding kernels
  jhcBasicAct act;       
  jhcLocalSeq seq;
  jhcTargetVis vis;       
  jhcSoundFcn snd;

  // mood and statistics display
  jhcAliaChart disp;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcMensCoord ();
  jhcMensCoord ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname);

  // main functions
  int BindVideo (jhcVideoSrc *v, int vnum = 0);
  int Reset (int id =1);
  int Respond ();
  const jhcImg *View (int num =0);
  int Done (int status =1);


// PRIVATE MEMBER FUNCTIONS
private:


};


#endif  // once




