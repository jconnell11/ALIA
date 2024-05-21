// jhcEmotion.h : autonomous nagging and conscious acess to feelings
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

#include "API/jhcAliaNote.h"           // common audio

#include "Action/jhcAliaMood.h"        // common robot

#include "Kernel/jhcStdKern.h"


//= Autonomous nagging and conscious acess to feelings.
// largely keyed off emotion bits from jhcMood:
// [ surprised angry scared happy : unhappy bored lonely tired ]

class jhcEmotion : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  // external components
  jhcAliaMood *mood;
  jhcAliaNote *rpt;

  // pending emotion reports
  UL32 nag[3];
  UL32 onset;
  int first, yikes, reported;

  // timing parameters
  double delay[3];
  double urge[3];
  double suffer;


// PUBLIC MEMBER VARIABLES
public:
  // parameter sets for GUI
  jhcParam tps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcEmotion ();
  jhcEmotion ();
  void BindMood (jhcAliaMood& m) {mood = &m;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  JCMD_DEF(emo_test);
  JCMD_DEF(emo_list);
  int mood_bit (int& deg, const jhcAliaDesc *hq) const;
  void emo_assert (int bit, int detail);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int time_params (const char *fname);

  // overridden virtuals
  void local_reset (jhcAliaNote& top);
  void local_volunteer ();
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

  // event functions
  void wake_up ();
  void freak_out ();
  void mark_onset ();
  void auto_nag ();


};
