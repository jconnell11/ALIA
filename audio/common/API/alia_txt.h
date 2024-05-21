// alia_txt.h : text-based interface ALIA reasoning system
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

// Simple main loop:
//
//    alia_reset(NULL, 1);
//    while (1)
//    {
//      <-- update robot sensor data and get user input cmd --> 
//      msg = alia_respond(cmd);
//      <-- display agent msg and issue robot motion commands -->
//      alia_daydream(1);
//    }
//    alia_done(0);
//
// Setting user name:
// 
//    jhcAliaNote *rpt = alia_note();
//    rpt->StartNote();
//    rpt->AddProp(rpt->User(), "name", "Marvin");
//    rpt->FinishNote();
//    alia_vip("Marvin");

/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifdef __linux__

  #define DEXP               // nothing special needed for Linux shared lib

#else 

  // function declarations (possibly combined with other header files)
  #ifndef DEXP
    #ifdef ALIATXT_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #endif


  // link to library stub
  #ifndef ALIATXT_EXPORTS
    #pragma comment(lib, "alia_txt.lib")
  #endif

#endif


///////////////////////////////////////////////////////////////////////////
//                              Configuration                            //
///////////////////////////////////////////////////////////////////////////

//= Loads all configuration and calibration data from a text file.
// if not called, default values will be used for all parameters
// returns 1 if successful, 0 for failure

extern "C" DEXP int alia_config (const char *fname);


//= Add a package of grounding functions to reasoning system.
// typically supported in KB0 by a set of operators, rules, and grammar terms
// should be called before alia_reset to allow loading of KB0 files
// returns 1 if successful, 0 for problem

extern "C" DEXP int alia_kernel (class jhcAliaKernel *fcns);


//= Connect grounding kernels to real-world interface for body.
// also connects DLL kernels loaded through GND/kernels.lst file 
// should call after alia_kernel but before alia_reset

extern "C" DEXP void alia_body (void *soma);


//= Add the name of some person ("Kelly Smith") to recognition grammar.
// can also add under NAME category in "language/vocabulary.sgm" file
// should be called after alia_reset
// returns 1 if successful, 0 for problem

extern "C" DEXP int alia_vip (const char *dude);


///////////////////////////////////////////////////////////////////////////
//                               Information                             //
///////////////////////////////////////////////////////////////////////////

//= Give string with version number of DLL and possibly other information.
// returns valid string always (never NULL)

extern "C" DEXP const char *alia_version ();


//= Provides access to a number of internal functions of the system.
// used to examine knowledge or inject new facts (like user name)
// for assertions do: StartNote + fact1 + fact2 + ... + FinishNote
// should be called after alia_reset to avoid memory clearing
// returns valid object always (never NULL)

extern "C" DEXP class jhcAliaNote *alia_note ();


///////////////////////////////////////////////////////////////////////////
//                           Main Functions                              //
///////////////////////////////////////////////////////////////////////////

//= Reset processing state at the start of a run.
// dir: base directory for config, language, log, and KB subdirectories
// rname: robot name (like "Jim Jones"), can be NULL if desired
// quiet: 1 = no console printing (only log), 0 = copious status messages
// returns 1 if successful, 0 for error

extern "C" DEXP int alia_reset (const char *dir, const char *rname, int quiet);


//= Record current speeds of body and condition of battery.
// base and arm speeds are inches per second, battery is percentage
// this data is needed for computing boredom and tiredness

extern "C" DEXP void alia_motion (double base, double arm, int bat);


//= Process any input and do reasoning using recent sensor data.
// generally call between sensor acquisition and command issuance
// returns string to communicate to user, NULL if none

extern "C" DEXP const char *alia_respond (const char *cmd);


//= Think some more using sensor data already acquired.
// typically called after motion commands have been issued
// if pace > 0 then delays return until next sensor cycle

extern "C" DEXP void alia_daydream (int pace);


//= Stop processing and possibly save state at end of run.
// returns 1 if successful, 0 for error

extern "C" DEXP int alia_done (int save);


