// jhcAliaSpeech.h : speech and loop timing interface for ALIA reasoner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "Parse/jhcSpFix.h"            // common audio

#include "Action/jhcAliaCore.h"        // common robot


//= Speech and loop timing interface for ALIA reasoner
// largely funnels speech back and forth from reasoning engine

class jhcAliaSpeech : public jhcAliaCore
{
// PRIVATE MEMBER VARIABLES
private:
  // language I/O and verbal attention
  jhcSpFix fix;
  char raw[500], input[500], output[500], pend[500];
  char disp[500], tts[500];
  UL32 awake, conv, yack;
  int src, gate, perk;


// PUBLIC MEMBER VARIABLES
public:
  // externally settable I/O parameters
  jhcParam tps;
  double stretch, splag, wait, early;  
  int amode;                           


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaSpeech ();
  jhcAliaSpeech ();

  // speech data access
  const char *LastIn () const  {return echo;}  
  const char *LastOut () const {return disp;}
  const char *LastTTS () const {return tts;}

  // main functions
  int Reset (const char *rname =NULL, int prt =3);
  int SelectSrc (const char *msg, const char *reco =NULL);
  int UpdateAttn (int hear, int talk, int eye =0);
  void Consider ();


// PROTECTED MEMBER FUNCTIONS
protected:
  // processing parameters
  int time_params (const char *fname);


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  void blip_txt (int cutoff);


};





