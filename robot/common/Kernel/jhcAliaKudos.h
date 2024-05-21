// jhcAliaKudos.h : user feedback on confidence and preference thresholds
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023-2024 Etaoin Systems
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

#include "Reasoning/jhcActionTree.h"   // common audio

#include "Action/jhcAliaMood.h"        // common robot

#include "Kernel/jhcStdKern.h"


//= User feedback on confidence and preference thresholds.

class jhcAliaKudos : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  jhcAliaMood *mood;
  jhcActionTree *atree;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaKudos ();
  jhcAliaKudos ();
  void BindMood (jhcAliaMood& m) {mood = &m;}

  // main functions
  JCMD_DEF(kudo_conf);
  JCMD_DEF(kudo_pref);


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  void local_reset (jhcAliaNote& top)
    {atree = dynamic_cast<jhcActionTree *>(&top);}
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);


};

