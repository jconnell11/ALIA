// jhcSpeechWeb.h : uses DLL to access Microsoft Azure Speech Services
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#include "Semantic/jhcTxtAssoc.h"


//= Uses DLL to access Microsoft Azure Speech Services.
// only allows one engine/session at a time (i.e. cannot handle two robots)
// project for sp_reco_web.dll is generally under code/deriv/sp_reco_web

class jhcSpeechWeb
{
// PRIVATE MEMBER VARIABLES
private:
  // recognition results and status
  char utt0[500], raw[500], utt[500];
  const char *rcv;
  int hear, mute, txtin, quit, dbg;

  // speech corrections
  jhcTxtAssoc canon;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSpeechWeb ();
  jhcSpeechWeb ();
  const char *Version () const;
  int LoadFix (const char *fname) {return canon.LoadList(fname);}
  int Fixes () const {return canon.TotalVals();}

  // main functions
  int Init (int partial =0);
  void Listen (int doit =1);
  void Inject (const char *txt, int stop =0);
  int Update (int tts =0);
  void Close ();

  // reognition status and results
  int Hearing () const {return hear;}
  const char *Heard () const {return utt;} 
  const char *LastIn () const {return rcv;}
  bool Escape () const {return(quit > 0);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr_state ();

  // main functions
  void fix_up (char *fix, int ssz, const char *orig) const;

};

