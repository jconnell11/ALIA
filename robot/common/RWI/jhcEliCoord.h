// jhcEliCoord.h : top level parsing, learning, and control for ELI robot
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

#ifndef _JHCELICOORD_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCELICOORD_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"             // common video

#include "Acoustic/jhcAliaSpeech.h"    // common audio 
#include "Parse/jhcNameList.h"         

#include "Action/jhcAliaChart.h"       // common robot
#include "Body/jhcEliBody.h"           
#include "Grounding/jhcBallistic.h"
#include "Grounding/jhcManipulate.h"
#include "Grounding/jhcSceneVis.h"
#include "Grounding/jhcSocial.h"
#include "Grounding/jhcSupport.h"
#include "RWI/jhcEliGrok.h"


//= Top level parsing, learning, and control for ELI robot.
// <pre>
// class tree overview (+ = member, > = pointer):
//
//   EliCoord         
//     AliaSpeech
//       AliaCore            reasoning (see header file)
//       +SpeechX            TTS + local speech
//         SpTextMS
//         SpRecoMS
//         +TxtSrc           pronunciation corrections
//       +SpeechWeb
//         +TxtAssoc         recognition corrections      
//     +EliBody              robot hardware (see header file)
//     +EliGrok              runs body and sensors (see header file)
//     +Ballistic            net to basic movements
//       TimedFcns
//         AliaKernel
//     +Social               net to person interaction
//       TimedFcns
//         AliaKernel
//     +SceneVis             net to object perception
//       TimedFcns
//         AliaKernel
//     +Manipulate           net to object interaction
//       TimedFcns
//         AliaKernel
//     +Support              net to surface perception
//       TimedFcns
//         AliaKernel
//     +AliaGraph            graph display utilities
// 
// </pre>

class jhcEliCoord : public jhcAliaSpeech
{
// PRIVATE MEMBER VARIABLES
private:
  int alert, mech;


// PUBLIC MEMBER VARIABLES
public:
  // possibly shared components
  jhcEliBody body;           
  jhcEliGrok rwi;                     

  // face reco people
  jhcNameList vip;  

  // extra grounding kernels
  jhcBallistic ball; 
  jhcSocial soc;
  jhcSceneVis svis;
  jhcManipulate man;
  jhcSupport sup;

  // mood and statistics display
  jhcAliaChart disp;

  // kernel debugging messages
  jhcParam kps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  ~jhcEliCoord ();
  jhcEliCoord ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname);

  // main functions
  int SetPeople (const char *fname, int append =0, int wds =1);
  int BindVideo (jhcVideoSrc *v, int vnum =0);
  int Reset (int bmode =0);
  int Respond ();
  const jhcImg *View (int num =0);
  int Done (int face =0);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int kern_params (const char *fname);


};


#endif  // once




