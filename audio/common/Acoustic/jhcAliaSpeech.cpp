// jhcAliaSpeech.cpp : speech and loop timing interface for ALIA reasoner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#include <ctype.h>

#include "Acoustic/jhcAliaSpeech.h"

#include "Interface/jtimer.h"
///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaSpeech::~jhcAliaSpeech ()
{
}


//= Default constructor initializes certain values.

jhcAliaSpeech::jhcAliaSpeech ()
{
  time_params(NULL);         
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for overall control of timing.
// this should be called in Defaults and tps used in SaveVals

int jhcAliaSpeech::time_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("asp_time", 0);
  ps->NextSpecF( &stretch, 3.5, "Attention window (sec)");       // was 2.5 (local)
  ps->NextSpecF( &splag,   0.5, "Post speech delay (sec)");      // was 0.1 (local)
  ps->NextSpecF( &wait,    0.3, "Text out delay (sec)");         // was 0.15
  ps->NextSpecF( &early,   0.5, "Signal early turn off (sec)");
  ps->NextSpec4( &amode,   2,   "Wake (on, any, front, solo)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence and possibly suppress printouts.
// can also set up robot name in grammar as an attention word
// assumes base directory for configuration and log files already recorded
// returns 1 if okay, 0 or negative for error

int jhcAliaSpeech::Reset (const char *rname, int prt)
{
  // clear attention and TTS timing
  conv  = 0;
  awake = 0;
  yack  = 0;

  // clear internal language strings
  *raw    = '\0';
  *input  = '\0';
  *output = '\0';
  *pend   = '\0';

  // clear language I/O state
  src  = 0;
  gate = 0;
  perk = 0;

  // clear language outputs
  *disp = '\0';
  *tts  = '\0';
    
  // initialize underlying reasoning system and dump all name phrases 
  jhcAliaCore::Reset(rname, prt, 1);
  jprintf(" %3d name phrases for speech recognition\n", gr.SaveNames(wrt("config/all_names.txt")));   

  // initialize speech patches and announce start
  jprintf(1, noisy, " %3d recognition fixes from: misheard.map\n", fix.LoadFix(Dir(), 1));
  jprintf(1, noisy, " %3d re-spellings from: pronounce.map\n", fix.LoadPron(Dir(), 1));
  jprintf(1, noisy, "\n========================= START ==========================\n\n");
  return 1;
}


//= Choose either typing or speech recognition for input to reasoner.
// need to call before UpdateAttn() to supply correct "src" value 
// returns 2 for typing, 1 for speech, 0 for nothing

int jhcAliaSpeech::SelectSrc (const char *msg, const char *reco)
{
  *input = '\0';
  src = 0;
  if (msg != NULL)           // preferred over speech     
  {
    strcpy_s(input, msg);
    src = 2;
  }
  else if ((reco != NULL) && (*reco != '\0'))
  {
    strcpy_s(raw, reco);
    src = 1;
  }
  return src;
}


//= Decide whether to require verbal attention word or not.
// hear = incoming speech, talk = TTS busy, eye = direct gaze
// amode: -1 text, 0 not needed, 1 anywhere, 2 front, 3 by itself
// needs to be called after SelectSrc() for correct "src" value 
// returns 2 if listening, 1 if almost timed out, 0 if wake word needed

int jhcAliaSpeech::UpdateAttn (int hear, int talk, int eye)
{
  double gap;
  UL32 now = jms_now();

  // always refresh time since last interaction
  if ((hear > 0) || (talk > 0) || (eye > 0))
    conv = now;
  
  // renew time out if eye contact, name heard, or new typing
  if ((amode <= 0) || (eye > 0) || (perk >= 2) || (src == 2))
  {
    awake = now;
    gate = 2;                // green
  }
  else if (gate > 0)
  {
    // extend current interaction if still talking or user just said something
    if ((talk > 0) || (hear >= 2))
    {
      awake = now;
      gate = 2;              // green
    }
    else if (jms_secs(now, conv) > splag)  // long time since I/O
    {
      gap  = jms_secs(now, awake);         // time since interaction    
      if (gap > stretch) 
      {
        jprintf(1, noisy, "\n  ... timeout ... verbal attention off\n\n");
        gate = 0;            // red
      } 
      else if (gap > (stretch - early))
        gate = 1;            // yellow    
    }
  }

  // report result
  stat.Speech(hear, talk, gate);                 // for graphs
  return gate;
}


//= Generate actions in response to update sensory information.
// <pre>
// text I/O string transformations:
//
//   speech reco ---> raw               [tts] --------> speaking
//                     |                  ^
//                 fix |                  | re-spell
//                     v                  |
//   typing -------> input --> ALIA --> output --> pend
//                     |                             |
//       clean or mark |                       delay |
//                     v                             v
//                   [echo] ----> console          [disp] ----> console
// </pre>

void jhcAliaSpeech::Consider ()
{
  UL32 tcyc;
  int n;
jtimer(16, "Consider");

  // mark time that call was initiated
  tcyc = jms_now();

  // ingest user input, parse, and see if name was mentioned
  if (src == 1) 
    if (fix.FixUp(input, raw) > 0)
      jprintf(1, noisy, " { Corrected misheard: \"%s\" }\n", raw);
  *echo = '\0';
  perk = 0;
  if (*input != '\0')
    perk = Interpret(input, gate, ((src == 1) ? amode : -1));

  // ALIA main - generate body commands and linguistic output
jtimer(22, "RunAll(1)");
  RunAll(1);                           // think for one cycle
jtimer_x(22);
  Response(output);    
  fix.ReSpell(tts, output);            // start TTS immediately
  n = (int) strlen(output);
  mood.Speak(n);                   

  // generate console display string for reasoner output
  *disp = '\0';
  if (*pend != '\0')
  {
    // see if last output delayed long enough yet
    if (jms_secs(tcyc, yack) > wait)
      blip_txt(0);
    else if (n > 0)                    // interrupted
      blip_txt(1);
  }
  if (n > 0)
  {
    // delay any new output to allow later over-write
    yack = tcyc;              
    strcpy_s(pend, output);
    if (output[n - 1] == '?')          // open gate for user answer
      awake = tcyc;           
  }

  // tell DayDream when original call was made
  now = tcyc;
jtimer_x(16);
}


//= Possibly terminate message after first word by inserting ellipsis.
// copies "pend" string to console "disp" variable and erases "pend"

void jhcAliaSpeech::blip_txt (int cutoff)
{
  char *end;

  if (cutoff > 0)
  {
    if ((end = strchr(pend, ' ')) != NULL)
      *end = '\0';
    strcat_s(pend, " ...");
  }
  strcpy_s(disp, pend);
  *pend = '\0';
}

