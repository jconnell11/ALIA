// jhcMensCoord.h : language processing and perception for Manus robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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


#include "Body/jhcManusBody.h"         // common robot
#include "Grounding/jhcBasicAct.h"
#include "Grounding/jhcLocalSeq.h"
#include "Grounding/jhcSoundFcn.h"
#include "Grounding/jhcTargetVis.h"
#include "Manus/jhcManusRWI.h"

#include "Acoustic/jhcAliaSpeech.h"    // common audio


//= Language processing and perception for Manus robot.

class jhcMensCoord : public jhcAliaSpeech
{
// PRIVATE MEMBER VARIABLES
private:
  double ver;


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


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcMensCoord ();
  jhcMensCoord ();
  double Version () const {return ver;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  int Reset (int id =1);
  int Respond ();
  void Done (int status =1);


// PRIVATE MEMBER FUNCTIONS
private:


};


#endif  // once




