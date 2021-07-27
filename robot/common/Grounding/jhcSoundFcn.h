// jhcSoundFcn.h : sound effect output for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _JHCSOUNDFCN_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSOUNDFCN_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Interface/jhcString.h"       // common video

#include "Action/jhcTimedFcns.h"       // common robot


//= Sound effect output for ALIA system.
// waits until audio output is available then waits until sound finished

class jhcSoundFcn : public jhcTimedFcns
{
// PUBLIC MEMBER VARIABLES
public:
  jhcString fname;
  char sdir[200];
  void *bg;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSoundFcn ();
  jhcSoundFcn ();


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  int local_start (const jhcAliaDesc *desc, int i);
  int local_status (const jhcAliaDesc *desc, int i);

  // on-demand functions
  JCMD_DEF(play_snd);

  // sound file functions
  int find_file (char *fn, const jhcAliaDesc *n, int ssz);

  // background thread
  static unsigned int __stdcall snd_backg (void *inst);


};


#endif  // once




