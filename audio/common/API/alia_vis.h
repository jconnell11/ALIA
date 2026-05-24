// alia_vis.h : interface to ALIA language, actuators, and perception
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

#pragma once


// define DEXP_V (uses other DLLs whose header files define DEXP)

#ifdef __linux__
  #define DEXP_V               // nothing special needed for Linux shared lib
#else 

  // function declarations
  #ifndef DEXP_V
    #ifdef ALIAVIS_EXPORTS
      #define DEXP_V __declspec(dllexport)
    #else
      #define DEXP_V __declspec(dllimport)
    #endif
  #endif

  // link to library stub
  #ifndef ALIAVIS_EXPORTS
    #pragma comment(lib, "alia_vis.lib")
  #endif

#endif


///////////////////////////////////////////////////////////////////////////
 
//= Interface to ALIA language, actuators, and perception.
// basically dumps a big pile of variables on the floor

extern "C" 
{
  //= Configure reasoning system and load knowledge base.
  // dir: base directory for config, language, log, and KB subdirectories
  // rname: robot name (like "Jim Jones"), can be NULL if desired
  // prog: name of program to print on console at beginning
  // dbg: which debugging image to produce (0 = none, 1 = overhead map, 2-16 = various) 
  // makes file "config/all_names.txt" for speech recognition
  // returns 1 if okay, 0 or negative for problem
  DEXP_V int alia_reset (const char *dir, const char *rname, const char *prog, int dbg =0);

  //= Exchange command and sensor data then start reasoning a bit.
  // alia_XXX variables only read/written while this function is blocking
  // returns 2 if okay, 1 if not ready, 0 for quit, negative for problem
  // NOTE: can take up to 100ms to finish on Raspberry Pi 4
  DEXP_V int alia_think ();

  //= Cleanly stop reasoning system and possibly save knowledge base.
  // returns 1 if okay, 0 or negative for problem
  DEXP_V int alia_done (int save); 

  // -------------------------- SPEECH ----------------------------------

  //= Text output from reasoner for TTS (beware of overrun).
  DEXP_V const char *alia_spout ();

  //= Text input to reasoner from speech recognition (copies argument).
  // optionally records delay (ms) from start of speech to recognition 
  DEXP_V void alia_spin (const char *reco, int ms =0);

  DEXP_V int alia_attn;                        // paying attention (no wake)

  DEXP_V int alia_hear, alia_talk;             // hearing speech or talking now

  // --------------------------- BODY -----------------------------------

  DEXP_V int alia_mood;                        // mood bit vector (happy, angry)

  DEXP_V float alia_batt;                      // battery capacity percent
  DEXP_V float alia_tilt, alia_roll;           // vehicle tilt and roll now
  DEXP_V int alia_snd;                         // sound arrival direction

  // --------------------------- NECK -----------------------------------

  DEXP_V float alia_rxt, alia_ryt, alia_rzt;   // desired target location to view
  DEXP_V float alia_rpt, alia_rtt;             // desired range-finder orientation
  DEXP_V float alia_rpv, alia_rtv, alia_rgv;   // range-finder pan, tilt, gaze rates 
  DEXP_V int alia_rpi, alia_rti, alia_rgi;     // range-finder command importance
  DEXP_V float alia_cpt, alia_ctt;             // desired main camera orientation
  DEXP_V float alia_cpv, alia_ctv;             // main camera pan and tilt rates
  DEXP_V int alia_cpi, alia_cti;               // main camera command importance
  DEXP_V float alia_npt, alia_ntt;             // desired aux camera orientation
  DEXP_V float alia_npv, alia_ntv;             // aux camera pan and tilt rates
  DEXP_V int alia_npi, alia_nti;               // aux camera command importance

  DEXP_V float alia_rx, alia_ry, alia_rz;      // range-finder location now
  DEXP_V float alia_rp, alia_rt, alia_rr;      // range-finder orientation now
  DEXP_V float alia_cx, alia_cy, alia_cz;      // main camera location now
  DEXP_V float alia_cp, alia_ct, alia_cr;      // main camera orientation now
  DEXP_V float alia_nx, alia_ny, alia_nz;      // aux camera location now
  DEXP_V float alia_np, alia_nt, alia_nr;      // aux camera orientation now

  // ---------------------------- ARM -----------------------------------

  DEXP_V float alia_axt, alia_ayt, alia_azt;   // desired gripper position
  DEXP_V float alia_apt, alia_att, alia_art;   // desired gripper direction
  DEXP_V float alia_apv, alia_adv;             // position and direction rates
  DEXP_V int alia_apm, alia_adm;               // position and direction mode bits
  DEXP_V int alia_api, alia_adi;               // position and direction importance
  DEXP_V float alia_awt;                       // desired gripper width (force)
  DEXP_V float alia_awv;                       // width change rate wrt normal
  DEXP_V int alia_awi;                         // gripper width cmd importance
  DEXP_V float alia_ajv;                       // tuck joints rate wrt normal
  DEXP_V int alia_aji;                         // tuck joints cmd importance

  DEXP_V float alia_ax, alia_ay, alia_az;      // gripper location now
  DEXP_V float alia_ap, alia_at, alia_ar;      // gripper direction now
  DEXP_V float alia_aw, alia_af;               // gripper width and force now
  DEXP_V float alia_aj;                        // max tuck joints error

  // --------------------------- LIFT -----------------------------------

  DEXP_V float alia_fht;                       // desired fork height
  DEXP_V float alia_fhv;                       // height change rate wrt normal
  DEXP_V int alia_fhi;                         // lift command importance

  DEXP_V float alia_fh;                        // fork height now

  // --------------------------- BASE -----------------------------------

  DEXP_V float alia_bmt, alia_brt;             // cumulative motion targets
  DEXP_V float alia_bsk;                       // move direction wrt forward
  DEXP_V float alia_bmv, alia_brv;             // move and rotation rates
  DEXP_V int alia_bmi, alia_bri;               // move and rotation cmd importance

  DEXP_V float alia_bt, alia_bw;               // cumulative displacements
  DEXP_V float alia_bx, alia_by;               // Cartesian map location

  // --------------------------- VISION ---------------------------------

  //= Get width of alternate debugging "map" image (only valid after reset).
  DEXP_V int alia_wmap ();                     

  //= Get height of alternate debugging "map" image (only valid after reset).
  DEXP_V int alia_hmap ();    

  //= Get title of alternate debugging "map" image (only valid after reset).
  DEXP_V const char *alia_tmap ();       

  DEXP_V void *alia_view, *alia_map;           // object, map destination buffers  
  DEXP_V int alia_vfmt, alia_mfmt;             // object and map write formats

  DEXP_V const void *alia_rng;                 // range-finder source buffer
  DEXP_V const void *alia_col, *alia_aux;      // main and aux source buffers
  DEXP_V int alia_rfmt;                        // range-finder read format
  DEXP_V int alia_cfmt, alia_afmt;             // main and aux read formats

}
