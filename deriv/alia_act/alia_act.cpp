// alia_act.cpp : external interface to ALIA speech utilities and reasoning
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Etaoin Systems
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

#ifndef __linux__
  #include <windows.h>                 // needs to be early in list
  #include <direct.h>                  // for _getcwd in Windows
#endif

#include "jhc_pthread.h"

#include "RWI/jhcSwapCoord.h"          // common robot

#include "Interface/jhcUserIO.h"       // common video

#include "API/alia_act.h"              // common audio

#include "resource.h"

#include "Interface/jtimer.h"
///////////////////////////////////////////////////////////////////////////
//                   Hidden Variables and Functions                      //
///////////////////////////////////////////////////////////////////////////

//= Main reasoning engine and console interaction.

static jhcSwapCoord sc;
static jhcUserIO io;


//= Background reasoning thread control.

static pthread_t mull;  
static int active = -1;      // not initialized yet


//= Hidden global speech strings.

static char sp_in[500], sp_out[500];


//= Background thread function runs several reasoning cycles.

static pthread_ret churn (void *dummy)      
{
UL64 start = jtimer_now();

jtimer(23, "churn");
  sc.Consider();
  sc.DayDream();
jtimer_x(23);

double ms = 1000.0 * jtimer_secs(start);
if (ms >= 10.0)
    jprintf("------------------- %5.2f ms think -------------------\n", ms);

  return 0;                  // can be void * or int
}


///////////////////////////////////////////////////////////////////////////
//                      Initialization and Locking                       //
///////////////////////////////////////////////////////////////////////////

#ifndef __linux__

//= Only allow DLL to be used for a while.
// more of an annoyance than any real security

BOOL APIENTRY DllMain (HANDLE hModule,
                       DWORD ul_reason_for_call, 
                       LPVOID lpReserved)
{
  int mon = 8, yr = 2024, smon = 3, syr = 2024, strict = 0; 
  char cwd[200];
  char *tail;

  // clean up on exit
  if (ul_reason_for_call == DLL_PROCESS_DETACH)
    return TRUE;
  if (ul_reason_for_call != DLL_PROCESS_ATTACH)
    return TRUE;

  // see if within valid time interval
  if (!jms_expired(mon, yr, smon, syr))    
    return TRUE;
  Complain("alia_act v%4.2f\nExpired as of %d/%d\njconnell@alum.mit.edu",
           sc.Version(), mon, yr);           

  // provide "backdoor" - override if directly in "jhc" directory
  _getcwd(cwd, 200);
  if ((tail = strrchr(cwd, '\\')) != NULL)
    if (strcmp(tail, "\\jhc") == 0)
      return TRUE;
  
  // potentially prohibit use
  if (strict <= 0)
    return TRUE;
  return FALSE;
}

#endif

///////////////////////////////////////////////////////////////////////////
//                      Visible External Functions                       //
///////////////////////////////////////////////////////////////////////////

//= Specify which hardware susbsystems are present and working.
// set to 1 or 0: nok = neck, aok = arm, fok = fork lift, bok = base
// call before alia_reset, can also be called if something breaks

extern "C" DEXP void alia_body (int nok, int aok, int fok, int bok)
{
  sc.neck0.nok = nok;
  sc.arm0.aok  = aok;
  sc.lift0.lok = fok;
  sc.base0.bok = bok;
}


//= Configure reasoning system and load knowledge base.
// dir: base directory for config, language, log, and KB subdirectories
// rname: robot name (like "Herbie Ganbei") where last name encodes body type
// prog: name of test program to print on console at beginning
// should configure hardware present flags before calling this function
// returns 1 if okay, 0 or negative for problem
// NOTE: robot hardware should be reset before this is called

extern "C" DEXP int alia_reset (const char *dir, const char *rname, const char *prog)    
{
  int ok;

  // clear battery voltage (for final printout)
  alia_batt = 100.0;

  // clear all speech information
  *sp_in  = '\0';
  *sp_out = '\0';
  alia_hear = 0;
  alia_talk = 0;
  alia_attn = 0;

  // initialize reasoner state (log file, no console output)
  active = 0;
  ok = sc.Reset(dir, rname, 1);

  // announce entry on console output
  printf("\n--------------------------------------------------------\n");
  printf("%s - ALIA reasoner %4.2f - hit ESC x2 to quit\n\n", 
         ((prog != NULL) ? prog : "alia_act"), sc.Version());
  io.Start();
  return ok;
}


//= Exchange command and sensor data then start reasoning a bit.
// returns 2 if okay, 1 if not ready, 0 for quit, negative for problem
// NOTE: can take up to 100ms to finish on Raspberry Pi 4

extern "C" DEXP int alia_think ()   
{
  jhcSwapNeck *n = &(sc.neck0);
  jhcSwapArm  *a = &(sc.arm0);
  jhcSwapLift *f = &(sc.lift0);
  jhcSwapBase *b = &(sc.base0);

  // make sure system is initialized and previous cycle is finished
  if (active < 0)
    return -1;
  if (active > 0)
    if (pthread_busy(mull))
      return 1;                                  // sc frobbing vars


  // refresh body command variables 
  n->Command(alia_npt, alia_ntt, alia_npv, alia_ntv, alia_npi, alia_nti);
  a->PosCmd( alia_axt, alia_ayt, alia_azt, alia_apv, alia_apm, alia_api);
  a->DirCmd( alia_apt, alia_att, alia_art, alia_adv, alia_adm, alia_adi);
  a->AuxCmd( alia_awt, alia_awv, alia_ajv, alia_awi, alia_aji);
  f->Command(alia_fht, alia_fhv, alia_fhi);
  b->Command(alia_bmt, alia_brt, alia_bsk, alia_bmv, alia_brv, alia_bmi, alia_bri);
  alia_mood = (sc.mood).Quantized();

  // refresh body sensor variables
  n->Status(alia_np, alia_nt, alia_nx, alia_ny, alia_nz);
  a->Status(alia_ax, alia_ay, alia_az, alia_ap, alia_at, alia_ar, 
            alia_aw, alia_af, alia_aj);
  f->Status(alia_fh);
  b->Status(alia_bx, alia_by, alia_bh);
  (sc.mood).Battery(alia_batt);

  // post any newly accepted input or generated output
  io.Post(sc.LastIn(), 1);
  io.Post(sc.LastOut(), 0);

  // refresh TTS and speaker attention then ingest input text 
  strcpy_s(sp_out, sc.LastTTS());
  if (sc.SelectSrc(io.Get(), sp_in) == 1)
    *sp_in = '\0';
  alia_attn = sc.UpdateAttn(alia_hear, alia_talk, 0);

  // start several cycles of reasoning in background
  if (io.Done() > 0)
    return 0;                                    // exit requested
  pthread_create(&mull, NULL, churn, NULL);
  active = 1;
  return 2;
}


//= Cleanly stop reasoning system and possibly save knowledge base.
// returns 1 if okay, 0 or negative for problem

extern "C" DEXP int alia_done (int save) 
{
  int rc;

  // let thread complete then shutdown reasoner
  if (active > 0)
    pthread_join(mull, 0);
  rc = sc.Done(save);

  // announce finish on console output
  io.Stop();
  printf("\nClean ALIA exit\n");
  printf("--------------------------------------------------------\n");
  printf("battery = %d%%\n\n", ROUND(alia_batt));
  return rc;
}


//= Text output from reasoner for TTS (enforces const).

extern "C" DEXP const char *alia_spout () 
{
  return sp_out;
}


//= Text input to reasoner from speech recognition (prevents overrun).

extern "C" DEXP void alia_spin (const char *reco) 
{
  strcpy_s(sp_in, reco);
}

