// jhcSpRecoWeb.h : speech recognition using Microsoft Azure web service
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#ifndef _JHCSPRECOWEB_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSPRECOWEB_
/* CPPDOC_END_EXCLUDE */

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
// generally takes separate key string (kf) and geographic zone (area)
// if area is NULL, kf is file with credentials (defaults to "sp_reco_web.key")
// returns 1 if successful, 0 if cannot connect, -1 for bad credentials

extern "C" DEXP int reco_setup (const char *kf =NULL, const char *reg =NULL);


//= Fix a mis-recognized phrase by making it more likely (not super useful).
// can be called even when recognition is actively running

extern "C" DEXP int reco_prefer (const char *phrase);


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


//= Stop recognizing speech (can be restarted with reco_start).

extern "C" DEXP void reco_stop ();


//= Stop recognizing speech and clean up all objects and files.

extern "C" DEXP void reco_cleanup ();


///////////////////////////////////////////////////////////////////////////

#endif  // once
