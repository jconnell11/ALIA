// alia_mind.cpp : text-based interface ALIA reasoning system
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

#include "Action/jhcAliaCore.h"        // common robot

#include "API/alia_txt.h"
#include "resource.h"


///////////////////////////////////////////////////////////////////////////
//                          Global Variables                             //
///////////////////////////////////////////////////////////////////////////

//= Temporary information string.

static char msg[200];


//= ALIA reasoner main class.

static jhcAliaCore core;


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
  return core.Defaults(fname);
}


//= Add the name of some person ("Kelly Smith") to recognition grammar.
// can also add under NAME category in "language/vocabulary.sgm" file
// should be called after alia_reset
// returns 1 if successful, 0 for problem

extern "C" DEXP int alia_vip (const char *dude)
{
  return core.AddName(dude);
}


//= Add a package of grounding functions to reasoning system.
// typically supported in KB0 by a set of operators, rules, and grammar terms
// should be called before alia_reset to allow loading of KB0 files
// returns 1 if successful, 0 for problem

extern "C" DEXP int alia_kernel (class jhcAliaKernel *fcns)
{
  if (fcns == NULL)
    return 0;
  (core.kern).AddFcns(*fcns);
  return 1;
}


//= Connect grounding kernels to real-world interface for body.
// also connects DLL kernels loaded through GND/kernels.lst file 
// should call after alia_kernel but before alia_reset

extern "C" DEXP void alia_body (void *soma)
{
  (core.kern).Platform(soma);
}


///////////////////////////////////////////////////////////////////////////
//                               Information                             //
///////////////////////////////////////////////////////////////////////////

//= Give string with version number of DLL and possibly other information.
// returns valid string always (never NULL)

extern "C" DEXP const char *alia_version ()
{
  sprintf_s(msg, "alia_txt v%4.2f", core.Version());
  return msg;
}


//= Provides access to a number of internal functions of the system.
// used to examine knowledge or inject new facts (like user name)
// for assertions do: StartNote + fact1 + fact2 + ... + FinishNote
// should be called after alia_reset to avoid memory clearing
// returns valid object always (never NULL)

extern "C" DEXP class jhcAliaNote *alia_note ()
{
  return &(core.atree);
}


///////////////////////////////////////////////////////////////////////////
//                           Main Functions                              //
///////////////////////////////////////////////////////////////////////////

//= Reset processing state at the start of a run.
// rname: robot name (like "Jim Jones"), can be NULL if desired
// quiet: 1 = no console printing (only log), 0 = copious status messages
// returns 1 if successful, 0 for error

extern "C" DEXP int alia_reset (const char *rname, int quiet)
{
  // possibly suppress all console printout
  if (quiet > 0)
    jprintf_log(1);
  jprintf_open();

  // no speech
  jprintf(1, core.noisy, "\nSPEECH -> OFF (text only)\n");
  jprintf(1, core.noisy, "=========================\n\n");

  // set basic grammar for core then clear state
  core.acc = 1;
  core.vol = 1;
  core.Reset(rname, 0);
  return 1;
}


//= Record current speeds of body and condition of battery.
// base and arm speeds are inches per second, battery is percentage
// this data is needed for computing boredom and tiredness

extern "C" DEXP void alia_motion (double base, double arm, int bat)
{
  (core.mood).Body(base, arm, bat);
}


//= Process any input and do reasoning using recent sensor data.
// generally call between sensor acquisition and command issuance
// returns string to communicate to user, NULL if none

extern "C" DEXP const char *alia_respond (const char *cmd)
{
  if (cmd != NULL)
    core.Interpret(cmd, 1, 0);
  core.RunAll(1);
  if (core.Response(msg) > 0)
    return msg;
  return NULL;
}


//= Think some more using sensor data already acquired.
// typically called after motion commands have been issued
// if pace > 0 then delays return until next sensor cycle

extern "C" DEXP void alia_daydream (int pace)
{
  core.DayDream();
  if (pace > 0)
    jms_resume(core.NextSense());
}


//= Stop processing and possibly save state at end of run.
// returns 1 if successful, 0 for error

extern "C" DEXP int alia_done (int save)
{
  core.Done(save);
  jprintf_close();
  return 1;
}


