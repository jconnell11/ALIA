// alia_sp.cpp : speech-based interface ALIA reasoning system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023 Etaoin Systems
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

#include <windows.h>
#include <direct.h>                    // for _getcwd in Windows

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"

#include "Acoustic/jhcAliaSpeech.h"    // common ausio

#include "API/alia_sp.h"
#include "resource.h"


///////////////////////////////////////////////////////////////////////////
//                          Global Variables                             //
///////////////////////////////////////////////////////////////////////////

//= Temporary information string.

static char msg[200];


//= ALIA reasoner main class.

static jhcAliaSpeech asp;


///////////////////////////////////////////////////////////////////////////
//                      Initialization and Locking                       //
///////////////////////////////////////////////////////////////////////////

//= Do all system initializations like unpacking auxiliary files.
// make a new compressed folder in main then add all files needed
// then do Add Resource / Import ... / as RCDATA and assign IDR_AUX_FILES

BOOL init (HANDLE hModule)
{
  HMODULE mod = (HMODULE) hModule;
  HRSRC rsrc;
  HGLOBAL hres = NULL;
  char *data;
  FILE *out;
  DWORD sz, n;

  // try to locate embedded resource 
  rsrc = FindResource(mod, MAKEINTRESOURCE(IDR_AUX_FILES), RT_RCDATA);
  if (rsrc == NULL)
    return TRUE;
  if ((hres = LoadResource(mod, rsrc)) == NULL)
    return TRUE;

  // copy all data out to temporary file
  if (fopen_s(&out, "jhc_temp.zip", "wb") != 0)
    return TRUE;
  data = (char *) LockResource(hres);
  sz = SizeofResource(mod, rsrc);
  n = (int) fwrite(data, 1, sz, out);
  fclose(out);

  // attempt to extarct all files then clean up
  if (n == sz)
    system("tar -xkf jhc_temp.zip");
  remove("jhc_temp.zip");
  return TRUE;
}


//= Only allow DLL to be used for a while.
// more of an annoyance than any real security

BOOL APIENTRY DllMain (HANDLE hModule,
                       DWORD ul_reason_for_call, 
                       LPVOID lpReserved)
{
  int mon = 11, yr = 2023, smon = 6, syr = 2023, strict = 0; 
  char cwd[200];
  char *tail;

  // clean up on exit
  if (ul_reason_for_call == DLL_PROCESS_DETACH)
    return TRUE;
  if (ul_reason_for_call != DLL_PROCESS_ATTACH)
    return TRUE;

  // see if within valid time interval
  if (!jms_expired(mon, yr, smon, syr))    
    return init(hModule);
  Complain("%s\nExpired as of %d/%d\njconnell@alum.mit.edu",
           alia_version(), mon, yr);           

  // provide "backdoor" - override if directly in "jhc" directory
  _getcwd(cwd, 200);
  if ((tail = strrchr(cwd, '\\')) != NULL)
    if (strcmp(tail, "\\jhc") == 0)
      return init(hModule);
  
  // potentially prohibit use
  if (strict <= 0)
    return init(hModule);
  return FALSE;
}


///////////////////////////////////////////////////////////////////////////
//                              Configuration                            //
///////////////////////////////////////////////////////////////////////////

//= Loads all configuration and calibration data from a text file.
// if not called, default values will be used for all parameters
// returns 1 if successful, 0 for failure

extern "C" DEXP int alia_config (const char *fname)
{
  return asp.Defaults(fname);
}


//= Set up how agent should handle speech input, output, and noise rejection.
//   spin: 0 = only text, 1 = local Win10 recognition, 2 = online Azure speech
//   wake: 0 = always on, 1 = name at either end, 2 = name at front, 3 = name by itself
//    tts: 0 = silent, 1 = speak out loud
// for Azure speech need valid account info entered into sp_reco_web.key file
// for W10 speech acoustic model defaults to current one (check Control Panel)
// should be called before alia_reset

extern "C" DEXP void alia_ioctrl (int spin, int wake, int tts)
{
  asp.spin = spin;
  asp.amode = wake;
  asp.tts = tts;
}


//= Add a package of grounding functions to reasoning system.
// typically supported in KB0 by a set of operators, rules, and grammar terms
// should be called before alia_reset to allow loading of KB0 files
// returns 1 if successful, 0 for problem

extern "C" DEXP int alia_kernel (class jhcAliaKernel *fcns)
{
  if (fcns == NULL)
    return 0;
  (asp.kern).AddFcns(*fcns);
  return 1;
}


//= Connect grounding kernels to real-world interface for body.
// also connects DLL kernels loaded through GND/kernels.lst file 
// should call after alia_kernel but before alia_reset

extern "C" DEXP void alia_body (void *soma)
{
  (asp.kern).Platform(soma);
}


//= Add the name of some person ("Kelly Smith") to recognition grammar.
// can also add under NAME category in "language/vocabulary.sgm" file
// should be called after alia_reset
// returns 1 if successful, 0 for problem

extern "C" DEXP int alia_vip (const char *dude)
{
  return asp.AddName(dude);
}


///////////////////////////////////////////////////////////////////////////
//                               Information                             //
///////////////////////////////////////////////////////////////////////////

//= Give string with version number of DLL and possibly other information.
// returns valid string always (never NULL)

extern "C" DEXP const char *alia_version ()
{
  sprintf_s(msg, "alia_sp v%4.2f", asp.Version());
  return msg;
}


//= Tell whether speech recognition is currently paying attention to user.
// returns 1 if system is listening, 0 if ignoring noise (no wake)

extern "C" DEXP int alia_attn ()
{
  return asp.Attending();
}


//= Tell the most recent status of the language input system.
// returns 0 if nothing, 1 if receiving speech, 2 if valid input received

extern "C" DEXP int alia_reco ()
{
  return asp.SpeechRC();
}


//= Echo the most recent input to system from speech or text.

extern "C" DEXP const char *alia_input ()
{
  return asp.NewInput(1);
}


//= Tell if the system is speaking and what mouth shape to use.
// returns 0 for silence, 1-21 for basic viseme shapes, 100 if text blip

extern "C" DEXP int alia_mouth ()
{
  return asp.Viseme();
}


//= Provides access to a number of internal functions of the system.
// used to examine knowledge or inject new facts (like user name)
// for assertions do: StartNote + fact1 + fact2 + ... + FinishNote
// should be called after alia_reset to avoid memory clearing
// returns valid object always (never NULL)

extern "C" DEXP class jhcAliaNote *alia_note ()
{
  return &(asp.atree);
}


///////////////////////////////////////////////////////////////////////////
//                           Main Functions                              //
///////////////////////////////////////////////////////////////////////////

//= Reset processing state at the start of a run.
//   rname: robot name (like "Jim Jones"), can be NULL if desired
//   voice: which voice file text-to-speech system should use
//   quiet: 1 = no console printing (only log), 0 = copious status messages
// for voice use Control Panel / Speech Recognition / Advanced speech options
// on Text to Speech tab to see choices listed under Voice selection ("David")
// returns 1 if successful, 0 for error

extern "C" DEXP int alia_reset (const char *rname, const char *voice, int quiet)
{
  // possibly suppress all console printout
  if (quiet > 0)
    jprintf_log(1);
  jprintf_open();

  // set basic grammar for core and speech reco then clear state
  asp.acc = 1;
  asp.vol = 1;
  return asp.Reset(rname, voice, 0);
}


//= Record current speeds of body and condition of battery.
// base and arm speeds are inches per second, battery is percentage
// this data is needed for computing boredom and tiredness

extern "C" DEXP void alia_motion (double base, double arm, int bat)
{
  (asp.mood).Body(base, arm, bat);
}


//= Process any input and do reasoning using recent sensor data.
// force > 0 overrides any wake word gating to enable speech processing
// generally call between sensor acquisition and command issuance
// returns string to communicate to user, NULL if none

extern "C" DEXP const char *alia_respond (const char *cmd, int force)
{
  if (cmd != NULL)
    asp.Accept(cmd);
  asp.UpdateSpeech();
  asp.Respond(force);
  return asp.NewOutput();
}


//= Think some more using sensor data already acquired.
// typically called after motion commands have been issued
// if pace > 0 then delays return until next sensor cycle

extern "C" DEXP void alia_daydream (int pace)
{
  asp.DayDream();
  if (pace > 0)
    jms_resume(asp.NextSense());
}


//= Stop processing and possibly save state at end of run.
// returns 1 if successful, 0 for error

extern "C" DEXP int alia_done (int save)
{
  asp.Done(save);
  jprintf_close();
  return 1;
}


