// jhcEliCoord.h : top level parsing, learning, and control for ELI robot
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

#ifndef _JHCELICOORD_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCELICOORD_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"             // common video

#include "Body/jhcEliBody.h"           // common robot
#include "Eli/jhcEliGrok.h"
#include "Grounding/jhcBallistic.h"
#include "Grounding/jhcSceneVis.h"
#include "Grounding/jhcSocial.h"

#include "Parse/jhcNameList.h"         // common audio
#include "Acoustic/jhcAliaSpeech.h"    


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
// 
// </pre>

class jhcEliCoord : public jhcAliaSpeech
{
// PRIVATE MEMBER VARIABLES
private:
  double ver;
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


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcEliCoord ();
  ~jhcEliCoord ();
  double Version () const {return ver;}

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname);

  // main functions
  int SetPeople (const char *fname, int append =0);
  int Reset (int bmode =0);
  int Respond ();
  void Done (int face =0, int status =1);


// PRIVATE MEMBER FUNCTIONS
private:
  // visual semantic linkage
  void check_user (int id);
  void wmem_heads ();


};


#endif  // once




