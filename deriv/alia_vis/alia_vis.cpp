// alia_vis.cpp : interface to ALIA language, perception, and reasoning 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2025 Etaoin Systems
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
  #include "resource.h"
#endif

#include "jhc_pthread.h"

#include "RWI/jhcVisCoord.h"           // common robot

#include "Interface/jhcUserIO.h"       // common video
#include "Interface/jtimer.h"

#include "API/alia_vis.h"              // common audio


///////////////////////////////////////////////////////////////////////////
//                   Hidden Variables and Functions                      //
///////////////////////////////////////////////////////////////////////////

//= Main reasoning engine and console interaction.

static jhcVisCoord vc;
static jhcUserIO io;


//= Background reasoning thread control.

static pthread_t mull;  
static int active = -1;                // not initialized yet


//= Hidden global speech strings.

static char sp_in[500], sp_out[500];
static double sp_delay = 0.0;          // recognition result lag
static int emit = 0;                   // TTS not sent yet


//= Background thread function runs several reasoning cycles.

static pthread_ret churn (void *dummy)      
{
  double ms;
  UL64 start = jtimer_now();

jtimer(25, "churn (ALIA reason)");
  // respond to sensors and think a bit more (flushes pending jprintf's)
  vc.Respond();
  vc.DayDream();

  // complain if it was very slow
  ms = 1000.0 * jtimer_secs(start);
  if (ms >= 100.0)
    jprintf("------------------- %5.2f ms think -------------------\n", ms);

jtimer_x(25);
  // value can be cast to void * or int
  return 0;                  
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
  int mon = 6, yr = 2025, smon = 1, syr = 2025, strict = 0; 
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
  Complain("alia_vis v%4.2f\nExpired as of %d/%d\njconnell@alum.mit.edu",
           vc.Version(), mon, yr);           

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

//= Configure reasoning system and load knowledge base.
// dir: base directory for config, language, log, and KB subdirectories
// rname: robot name (like "Herbie Ganbei") where last name encodes body type
// prog: name of test program to print on console at beginning
// dbg: which debugging image to produce (0 = none, 1 = overhead map, 2-17 = various) 
// returns 1 if okay, 0 or negative for problem
// NOTE: robot hardware should be reset before this is called

extern "C" DEXP_V int alia_reset (const char *dir, const char *rname, const char *prog, int dbg)
{
  char cfile[80];
  char *end;
  int ok;

  // clear battery voltage (for final printout)
  alia_batt = 100.0;

  // clear all speech information
  *sp_in  = '\0';
  *sp_out = '\0';
  emit = 0;
  alia_hear = 0;
  alia_talk = 0;
  alia_attn = 0;
  alia_snd  = 0;                       // in case mic missing

  // clear all input image linkages and ready flags (in case cameras missing)
  alia_rng  = NULL;
  alia_col  = NULL;
  alia_aux  = NULL;
  alia_rfmt = 0;
  alia_cfmt = 0;  
  alia_afmt = 0;

  // clear all output image linkages and desired flags
  alia_view = NULL;
  alia_map  = NULL;
  alia_vfmt = 0;
  alia_mfmt = 0;

  // load camera adjustments
  sprintf_s(cfile, "%s/config/%s", dir, rname);
  if ((end = strchr(cfile, ' ')) != NULL)
    strcpy_s(end, (cfile + 80) - end, "_cam.cal");
  (vc.rwi).LoadCfg(cfile);

  // initialize reasoner state (log file, no console output)
  active = 0;
  (vc.rwi).probe = dbg;
  ok = vc.Reset(dir, rname, 1);

  // announce entry on console output
  printf("\x1b[1;32m");
  printf("\n--------------------------------------------------------\n");
  printf("%s - ALIA reasoner %4.2f - hit ESC to quit\n\n", 
         ((prog != NULL) ? prog : "alia_vis"), vc.Version());
  printf("\x1b[0m");
  io.Start();
  return ok;
}


//= Exchange command and sensor data then start reasoning a bit.
// commands are from LAST thought cycle (so hardware does not get blocked)
// returns 2 if okay, 1 if not ready, 0 for quit, negative for problem
// NOTE: can take up to 100ms to finish on Raspberry Pi 4 (typical = 4ms)

extern "C" DEXP_V int alia_think ()   
{
  jhcSwapBody *r = &(vc.body0);
  jhcSwapNeck *n = &(r->neck0);
  jhcSwapArm  *a = &(r->arm0);
  jhcSwapLift *f = &(r->lift0);
  jhcSwapBase *b = &(r->base0);

jtimer(24, "alia_think (quick xchg)");
  // make sure system is initialized and previous cycle is finished
  if (active < 0)
    return -1;
  if (active > 0)
    if (pthread_busy(mull))
{
jtimer_x(24);
      return 1;                                  // sc frobbing vars
}

  // OUT - refresh body command variables (neck = range-finder)
  r->Lock();
  n->PosCmd( alia_rxt, alia_ryt, alia_rzt, alia_rgv, alia_rgi);
  n->DirCmd( alia_rpt, alia_rtt, alia_rpv, alia_rtv, alia_rpi, alia_rti);
  a->PosCmd( alia_axt, alia_ayt, alia_azt, alia_apv, alia_apm, alia_api);
  a->DirCmd( alia_apt, alia_att, alia_art, alia_adv, alia_adm, alia_adi);
  a->AuxCmd( alia_awt, alia_awv, alia_ajv, alia_awi, alia_aji);
  f->Command(alia_fht, alia_fhv, alia_fhi);
  b->Command(alia_bmt, alia_brt, alia_bsk, alia_bmv, alia_brv, alia_bmi, alia_bri);
  alia_mood = (vc.mood).Quantized();

  // OUT - refresh status images
  vc.GetView(alia_view, alia_vfmt);
  vc.GetMap(alia_map, alia_mfmt);

  // IN - alter recognition status if unused speech text in cache
  if (*sp_in != '\0')
    alia_hear = 2; 

  // IN - refresh body sensor variables (neck = range-finder)
  n->Status(alia_rp, alia_rt, alia_rx, alia_ry, alia_rz);
  a->Status(alia_ax, alia_ay, alia_az, alia_ap, alia_at, alia_ar, 
            alia_aw, alia_af, alia_aj);
  f->Status(alia_fh);
  b->Status(alia_bt, alia_bw, alia_bx, alia_by);
  (vc.mood).Battery(alia_batt);
  (vc.mic0).Sensor(alia_snd, alia_rp, alia_hear);         

  // IN - refresh body pitch and roll
  r->Status(alia_tilt, alia_roll);  

  // IN - refresh camera positions and orientations (pan = 0 is to right)
  r->RangePose(alia_rx, alia_ry, alia_rz, alia_rp + 90.0, alia_rt, alia_rr);
  r->ColorPose(alia_cx, alia_cy, alia_cz, alia_cp + 90.0, alia_ct, alia_cr);
  r->AuxCamPose(alia_nx, alia_ny, alia_nz, alia_np + 90.0, alia_nt, alia_nr);

  // IN - ingest new depth sensor images (flags auto-reset)
  r->SetRange(alia_rng, alia_rfmt);
  r->SetColor(alia_col, alia_cfmt);
  r->SetAuxCam(alia_aux, alia_afmt);

  // post any newly accepted input or generated output
  io.Post(vc.LastIn(), 1);
  io.Post(vc.LastOut(), 0);

  // refresh TTS and speaker attention then ingest input text 
  strcpy_s(sp_out, vc.LastTTS());
  emit = 0;
  if (vc.SelectSrc(io.Get(), sp_in) == 1)
    *sp_in = '\0';                               // clear text cache
  alia_attn = vc.UpdateAttn(alia_hear, alia_talk, vc.Stare(), sp_delay);
  r->Unlock();

  // start several cycles of reasoning in background
  if (io.Done() > 0)
{
jtimer_x(24);
    return 0;                                    // exit requested
}
  pthread_create(&mull, NULL, churn, NULL);
  active = 1;
jtimer_x(24);
  return 2;
}


//= Cleanly stop reasoning system and possibly save knowledge base.
// returns 1 if okay, 0 or negative for problem

extern "C" DEXP_V int alia_done (int save) 
{
  abstime_t one_sec;
  int rc, batt = ROUND(alia_batt);

  // let thread complete then shutdown reasoner
  if (active > 0)
    pthread_timedjoin_np(mull, 0, abstime_wait(&one_sec, 1000));
  rc = vc.Done(save, batt);

  // announce finish on console output
  io.Stop();
  printf("\x1b[1;32m");
  printf("\nClean ALIA exit\n");
  printf("--------------------------------------------------------\n");
  printf("battery = %d%%", batt);
  if ((batt > 0) && (batt <= 20))
    printf(" - CONSIDER RECHARGING");
  printf("\n\x1b[0m\n");
  return rc;
}


//= Text input to reasoner from speech recognition (prevents overrun).

extern "C" DEXP_V const char *alia_spout () 
{
  if (emit++ > 0)            // prevent double issue if ALIA slow
    *sp_out = '\0';
  return sp_out;             // does not like NULL
}


//= Text input to reasoner from speech recognition (copies argument).
// optionally records delay (ms) from start of speech to recognition 

extern "C" DEXP_V void alia_spin (const char *reco, int ms) 
{
  strcpy_s(sp_in, reco);
  sp_delay = 0.001 * ms;
}


//= Get width of alternate debugging "map" image (only valid after reset).

extern "C" DEXP_V int alia_wmap ()
{
  return (vc.rwi).XDim();
}                  


//= Get height of alternate debugging "map" image (only valid after reset).

extern "C" DEXP_V int alia_hmap ()                    
{
  return (vc.rwi).YDim();
}


//= Get title of alternate debugging "map" image (only valid after reset).

extern "C" DEXP_V const char *alia_tmap () 
{
  return (vc.rwi).Title();          
}
