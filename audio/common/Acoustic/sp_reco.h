// sp_reco.h : encapsulated functions for simple speech recognition
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2014 IBM Corporation
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

// The simple speech recognition interface automatically processes audio 
// (usually from a microphone) and generates best guesses as to the text.
// The system can be dynamically reconfigured while running for different 
// users and different expected topics. A typical program would look like:
//
//   reco_setup("ms_sp_reco.ini");
//   reco_user(usr);
//   reco_start(1, "reco_user1.log");
//   while (!_kbhit())
//   {
//     if (reco_status() >= 2)
//     {
//       cf = reco_heard(utterance, confs, 0);
//       printf("Confidence %d = %s\n", cf, utterance);
//     }
//     Sleep(100);
//   }
//   reco_cleanup();

///////////////////////////////////////////////////////////////////////////

#ifndef _SP_RECO_
#define _SP_RECO_


#include <stdlib.h>            // needed for NULL


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef SP_RECO_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


// link to library stub

#ifndef SP_RECO_EXPORTS
  #ifdef _DEBUG
    #pragma comment(lib, "sp_reco_d.lib")
  #else
    #pragma comment(lib, "sp_reco.lib")
  #endif
#endif


///////////////////////////////////////////////////////////////////////////
//                    Speech Recognition Configuration                   //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL.
// returns pointer to input string for convenience

extern "C" DEXP const char *reco_version (char *spec);


//= Loads all speech engine and input device parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int reco_setup (const char *cfg_file =NULL);


//= Fills string with description of audio source.
// returns pointer to input string for convenience

extern "C" DEXP const char *reco_input (char *spec);


//= Fills string with description of underlying speech recognition engine.
// returns pointer to input string for convenience

extern "C" DEXP const char *reco_engine (char *spec);


//= Start processing speech from the pre-designated audio source.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int reco_start (int level =0, const char *log_file =NULL);


//= Stop recognizing speech and clean up all objects and files.
// only call this at the end of a processing session (REQUIRED)

extern "C" DEXP void reco_cleanup ();


///////////////////////////////////////////////////////////////////////////
//                  Run-Time Recognition Modifications                   //
///////////////////////////////////////////////////////////////////////////

//= Provide a hint about user position to assist acoustic adaptation.
// angles are in degrees, distance is in inches relative to microphone

extern "C" DEXP void reco_loc_user (int azim, int elev =0, int dist =36);


//= Reconfigure the engine for a new primary user (acoustic model).
// this can be done without explicitly pausing the system
// if only one acoustic model is allowed, this overwrites previous
// can also be used for visual hints about the correct speaker ID
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int reco_add_user (const char *name);


//= Disable the acoustic model for a particular user.
// sometimes useful if a certain speaker is known to have left

extern "C" DEXP void reco_del_user (const char *name);


//= Disable all current acoustic models (users).
// this may also load up some background acoustic model instead

extern "C" DEXP void reco_clr_users ();


//= Give the ID strings associate with the current users (acoustic models).
// elements are separated with newline characters
// returns the count of elements in the string (separator = new line)

extern "C" DEXP int reco_list_users (char *list);


//= Enable a certain (or additional) language model (grammar / vocabulary).
// this can be done without explicitly pausing the system
// if only one langauge model is allowed, this overwrites previous
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int reco_add_model (const char *topic);


//= Disable a certain language model (grammar / vocabulary).

extern "C" DEXP void reco_del_model (const char *topic);


//= Disable all current language models (grammars / vocabularies).
// this may also load up some background language model instead

extern "C" DEXP void reco_clr_models ();


//= Generate a list of all currently enabled language models. 
// elements are separated with newline characters
// returns the count of elements in the string (separator = new line)

extern "C" DEXP int reco_list_models (char *list);


///////////////////////////////////////////////////////////////////////////
//                        Recognition Results                            //
///////////////////////////////////////////////////////////////////////////

//= Temporarily stop or resume processing speech (e.g. to free up CPU).
// also useful for half-duplex mode to ignore text-to-speech utterances
// the reco_start function automatically calls reco_listen(1) 

extern "C" DEXP void reco_listen (int doit =1);


//= Check to see if any utterances are ready for harvesting.
// returns 0 for silence, 1 for speech detected, 2 for ready, negative for some error

extern "C" DEXP int reco_status ();


//= Get the nth alternative for the last recognition result as a text string.
// assumes reco_status called immediately before this and returns 2
// also generates a string with the confidences (0-100) for each word
// and gives the confidence in this whole alternative (0-100)
// zero confidence generally means you have gotten to the end of the list
// returns confidence, 0 if nothing ready, -1 or less for some error

extern "C" DEXP int reco_heard (char *text, char *conf =NULL, int choice =0);


//= Get the phoneme string for the nth alternative of the last recognition result.
// phonemes are one or two characters separated by spaces with return between words

extern "C" DEXP void reco_phonetic (char *pseq, int choice =0);


//= Give ID string associated with the most likely speaker of the last utterance.
// if unknown then function returns 0 and clears string

extern "C" DEXP int reco_speaker (char *name);


///////////////////////////////////////////////////////////////////////////

#endif  // once
