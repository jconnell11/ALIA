// jhcCelTTS.h : interface to CEL text-to-speech system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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

#ifndef _JHCCELTTS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCCELTTS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Interface to CEL text-to-speech system.
// operates by doing a system call to the "curl" executable.
// needs cURL system from http://curl.haxx.se/download.html

class jhcCelTTS
{
// PUBLIC MEMBER VARIABLES
public:
  // parameters
  char iport[80], voice[80];


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcCelTTS ();
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  int Say (const char *msg) const;


// PRIVATE MEMBER FUNCTIONS
private:
  int tts_params (const char *fname);

};


#endif  // once




