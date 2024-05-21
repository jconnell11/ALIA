// alia_act.h : interface to ALIA reasoner as a passive resource
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

#pragma once

#ifdef __linux__
  #define DEXP               // nothing special needed for Linux shared lib
#else 

  // function declarations (possibly combined with other header files)
  #ifndef DEXP
    #ifdef ALIAACT_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #endif

  // link to library stub
  #ifndef ALIAACT_EXPORTS
    #ifdef _DEBUG
      #pragma comment(lib, "alia_act_d.lib")
    #else
      #pragma comment(lib, "alia_act.lib")
    #endif
  #endif

#endif

///////////////////////////////////////////////////////////////////////////

//= Interface to ALIA reasoner as a passive resource.
// basically dumps a big pile of variables on the floor

extern "C" 
{
  //= Specify which hardware susbsystems are present and working.
  // set to 1 or 0: nok = neck, aok = arm, fok = fork lift, bok = base
  // call before alia_reset, can also be called if something breaks
  DEXP void alia_body (int nok, int aok, int fok, int bok);

  //= Configure reasoning system and load knowledge base.
  // dir: base directory for config, language, log, and KB subdirectories
  // rname: robot name (like "Jim Jones"), can be NULL if desired
  // prog: name of program to print on console at beginning
  // makes file "config/all_names.txt" for speech recognition
  // returns 1 if okay, 0 or negative for problem
  DEXP int alia_reset (const char *dir, const char *rname, const char *prog);

  //= Exchange command and sensor data then start reasoning a bit.
  // returns 2 if okay, 1 if not ready, 0 for quit, negative for problem
  // NOTE: can take up to 100ms to finish on Raspberry Pi 4
  DEXP int alia_think ();

  //= Cleanly stop reasoning system and possibly save knowledge base.
  // returns 1 if okay, 0 or negative for problem
  DEXP int alia_done (int save); 

  //= Text output from reasoner for TTS (enforces const).
  DEXP const char *alia_spout ();

  //= Text input to reasoner from speech recognition (prevents overrun).
  DEXP void alia_spin (const char *reco);

  // -------------------------- SPEECH ----------------------------------

  DEXP int alia_attn;                        // paying attention (no wake)

  DEXP int alia_hear, alia_talk;             // hearing speech or talking now

  // --------------------------- BODY -----------------------------------

  DEXP int alia_mood;                        // mood bit vector (happy, angry)

  DEXP float alia_batt;                      // battery capacity percent
  DEXP float alia_tilt, alia_roll;           // vehicle tilt and roll now

  // --------------------------- NECK -----------------------------------

  DEXP float alia_npt, alia_ntt;             // desired camera orientation
  DEXP float alia_npv, alia_ntv;             // pan and tilt rates wrt normal
  DEXP int alia_npi, alia_nti;               // pan and tilt cmd importance

  DEXP float alia_nx, alia_ny, alia_nz;      // camera location now
  DEXP float alia_np, alia_nt, alia_nr;      // camera orientation now

  // ---------------------------- ARM -----------------------------------

  DEXP float alia_axt, alia_ayt, alia_azt;   // desired gripper position
  DEXP float alia_apt, alia_att, alia_art;   // desired gripper direction
  DEXP float alia_apv, alia_adv;             // position and direction rates
  DEXP int alia_apm, alia_adm;               // position and direction mode bits
  DEXP int alia_api, alia_adi;               // position and direction importance
  DEXP float alia_awt;                       // desired gripper width (force)
  DEXP float alia_awv;                       // width change rate wrt normal
  DEXP int alia_awi;                         // gripper width cmd importance
  DEXP float alia_ajv;                       // tuck joints rate wrt normal
  DEXP int alia_aji;                         // tuck joints cmd importance

  DEXP float alia_ax, alia_ay, alia_az;      // gripper location now
  DEXP float alia_ap, alia_at, alia_ar;      // gripper direction now
  DEXP float alia_aw, alia_af;               // gripper width and force now
  DEXP float alia_aj;                        // max tuck joints error

  // --------------------------- LIFT -----------------------------------

  DEXP float alia_fht;                       // desired fork height
  DEXP float alia_fhv;                       // height change rate wrt normal
  DEXP int alia_fhi;                         // lift command importance

  DEXP float alia_fh;                        // fork height now

  // --------------------------- BASE -----------------------------------

  DEXP float alia_bmt, alia_brt;             // incremental motion amounts
  DEXP float alia_bsk;                       // move direction wrt forward
  DEXP float alia_bmv, alia_brv;             // move and rotation rates
  DEXP int alia_bmi, alia_bri;               // move and rotation cmd importance

  DEXP float alia_bx, alia_by, alia_bh;      // map location and heading

}
