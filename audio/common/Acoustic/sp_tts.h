// sp_tts.h : encapsulated functions for simple text-to-speech capabilities
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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

// The text-to-speech interface automatically converts text strings into
// spoken output on some audio device. A typical program would look like:
//
//   tts_setup();
//   tts_start(1, "tts_log.txt");
//   while (1)
//   {
//     tts_say("The answer to 6 times 7 is %d.", 6 * 7);
//     tts_finish();
//     ...
//   }
//   tts_cleanup();

///////////////////////////////////////////////////////////////////////////

#ifndef _SP_TTS_
#define _SP_TTS_


#include <stdlib.h>            // needed for NULL


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef SP_TTS_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


// link to library stub (for DLL or static library)

#ifndef SP_TTS_EXPORTS
  #ifdef _DEBUG
    #pragma comment(lib, "sp_tts_d.lib")
  #else
    #pragma comment(lib, "sp_tts.lib")
  #endif
#endif


///////////////////////////////////////////////////////////////////////////
//                      Text-to-Speech Configuration                     //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL and possibly other information.
// returns pointer to input string for convenience

extern "C" DEXP const char *tts_version (char *spec);


//= Loads all voice and output device parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int tts_setup (const char *cfg_file =NULL);


//= Fills string with description of voice being used for output.
// returns pointer to input string for convenience

extern "C" DEXP const char *tts_voice (char *spec);


//= Fills string with description of output device being used.
// returns pointer to input string for convenience

extern "C" DEXP const char *tts_output (char *spec);


//= Start the text-to-speech system running and awaits input.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int tts_start (int level =0, const char *log_file =NULL);


//= Stop all speech output and clean up all objects and files.
// only call this at the end of a processing session (REQUIRED)

extern "C" DEXP void tts_cleanup ();


///////////////////////////////////////////////////////////////////////////
//                          Speaking Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Speak some message after filling in fields (like printf).
// queues utterance if already speaking, does not wait for completion
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int tts_say (const char *msg);


//= Tells if system has completed emitting utterance yet.
// will fill in given string (if any) with remainder of words to be spoken
// returns positive if talking, 0 if queue is empty, negative for some error

extern "C" DEXP int tts_status (char *rest =NULL);


//= Wait until system finishes speaking (BLOCKS).
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int tts_wait ();


//= Immediately terminate whatever is being said and anything queued.
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int tts_shutup ();


///////////////////////////////////////////////////////////////////////////

#endif  // once
