// jhcSpRecoWeb.h : speech recognition using Microsoft Azure web service
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

#include <stdlib.h>            // needed for NULL


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifdef SPRECOWEB_EXPORTS
    #define DEXP __declspec(dllexport)
  #else
    #define DEXP __declspec(dllimport)
  #endif
#endif


// link to library stub

#ifndef SPRECOWEB_EXPORTS
  #pragma comment(lib, "sp_reco_web.lib")
#endif


///////////////////////////////////////////////////////////////////////////
//                             Main Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Gives string with version number of DLL.

extern "C" DEXP const char *reco_version ();


//= Configure system to process speech from default input source (microphone).
// reads required key and area from file:    config/sp_reco_web.key
// optionally reads special names from file: config/all_names.txt
// returns 1 + names if successful, 0 if cannot connect, neg for bad credentials

extern "C" DEXP int reco_setup ();


//= Start processing speech right now.
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int reco_start ();


//= Turn microphone on and off (e.g. to prevent TTS transcription).

extern "C" DEXP void reco_listen (int doit);


//= Check to see if any utterances are ready for harvesting (resets value).
// return: 2 new result, 1 speaking, 0 silence, -1 unintelligible, -2 lost connection 

extern "C" DEXP int reco_status ();


//= Gives text string of ongoing recognition process.

extern "C" DEXP const char *reco_partial ();


//= Gives text string of last full recognition result.

extern "C" DEXP const char *reco_heard ();


//= Add a particular name to grammar to increase likelihood of correct spelling.
// can be called even when recognition is actively running

extern "C" DEXP int reco_name (const char *name);


//= Stop recognizing speech (can be restarted with reco_start).

extern "C" DEXP void reco_stop ();


//= Stop recognizing speech and clean up all objects and files.

extern "C" DEXP void reco_cleanup ();
