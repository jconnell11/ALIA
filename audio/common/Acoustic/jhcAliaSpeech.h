// jhcAliaSpeech.h : speech and loop timing interface for ALIA reasoner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#ifndef _JHCALIASPEECH_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIASPEECH_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Acoustic/jhcSpeechX.h"       // common audio
#include "Acoustic/jhcSpeechWeb.h"

#include "Action/jhcAliaCore.h"        // common robot

#include "Data/jhcParam.h"             // common video
#include "Interface/jms_x.h"


//= Speech and loop timing interface for ALIA reasoner
// largely funnels speech back and forth from reasoning engine

class jhcAliaSpeech : public jhcAliaCore
{
// PRIVATE MEMBER VARIABLES
private:
  // speech input and TTS status
  char alist[1000];
  UL32 awake;

  // reasoning interface
  char lastin[500], input[500];
  char output[500], pend[500], delayed[500];
  UL32 yack;
  int done;

  // loop timing and cycle counts
  UL32 start, last, now;
  double rem;
  int sense, think;


// PROTECTED MEMBER VARIABLES
protected:
  // full speech controls
  jhcSpeechX sp;                       // includes standard TTS
  jhcSpeechWeb web;                    // dictation input only


// PUBLIC MEMBER VARIABLES
public:
  // configuration
  char gram[80], kdir[200], kdir2[200];
  int noisy;              

  // timing parameters
  jhcParam tps;
  double stretch, splag, wait, thz, shz;  

  // externally settable I/O parameters
  int spin, amode, tts, acc;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaSpeech ();
  jhcAliaSpeech ();
  int SenseCnt ()   const {return sense;}
  int ThoughtCnt () const {return think;}
  double Sensing () const 
    {return((sense <= 0) ? 0.0 : sense / jms_secs(last, start));}
  double Thinking () const 
    {return((think <= 0) ? 0.0 : think / jms_secs(last, start));}
  UL32 NextSense () const
    {return(start + ROUND((1000.0 * sense) / shz));}
  int Attending () const {return((awake != 0) ? 1 : 0);}
  int SpeechIn () const {return spin;}
  int SpeechRC () const;

  // main functions
  int Reset (const char *rname =NULL, const char *vname =NULL);
  void AddName (const char* name);
  int VoiceInit ();
  int UpdateSpeech ();
  int Respond (int alert =0);
  void DayDream ();
  void Listen () {if (spin > 0) sp.Update();}
  void Done ();

  // intercepted I/O
  bool Accept (const char *in, int quit =0);
  const char *NewInput (int clean =0);
  const char *NewOutput ();

  // debugging
  void SpeakError (const char *msg) 
    {sp.InitTTS(); sp.Say(msg); sp.Finish(5.0);}


// PROTECTED MEMBER FUNCTIONS
protected:
  // processing parameters
  int time_params (const char *fname);


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  void kern_gram ();
  void base_gram (const char *list);
  void self_name (const char *name);
  void xfer_input ();
  int syllables (const char *txt) const;
  const char *blip_txt (int cutoff);


};


#endif  // once




