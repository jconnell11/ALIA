// jhcCelTTS.cpp : interface to CEL text-to-speech system
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

#include <stdio.h>
#include <process.h>

#include "Data/jhcParam.h"

#include "Acoustic/jhcCelTTS.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcCelTTS::jhcCelTTS ()
{
  Defaults();
}


//= Read all relevant defaults variable values from a file.

int jhcCelTTS::Defaults (const char *fname)
{
  jhcParam p;
  int ok = 1;

  ok &= p.LoadText(iport, fname, "tts_iport", "cel-audio2.watson.ibm.com:4567");
  ok &= p.LoadText(voice, fname, "tts_voice", "celia");
  return ok;
}


//= Write current processing variable values to a file.

int jhcCelTTS::SaveVals (const char *fname) const
{
  jhcParam p;
  int ok = 1;

  ok &= p.SaveText(fname, "tts_iport", iport);
  ok &= p.SaveText(fname, "tts_voice", voice);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Speak the given sentence (does not wait for completion).

int jhcCelTTS::Say (const char *msg) const
{
  char fcn[1000];

  sprintf_s(fcn, "curl --data \"{\\\"whatToSay\\\": \\\"%s\\\", \\\"voice\\\": \\\"%s\\\"}\" http://%s/tts", msg, voice, iport);
  return system(fcn);
}
