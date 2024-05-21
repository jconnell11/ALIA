// jhcBallistic.h : interface to ELI motion kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2021-2024 Etaoin Systems
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

#include "jhcGlobal.h"

#include "API/jhcAliaDesc.h"           // common audio
#include "API/jhcAliaNote.h"     

#include "Data/jhcParam.h"             // common video

#include "Geometry/jhcMatrix.h"        // common robot
#include "RWI/jhcGenRWI.h"

#include "Kernel/jhcStdKern.h"         


//= Interface to ELI motion kernel for ALIA system.
//   DO: allows user to directly move parts of the robot using verbal commands
//       including neck, lift stage, arm, gripper, and base
// NOTE: also volunteers "I am tired" when the battery gets low
//       and "I lost grip" when it accidentally drops something

class jhcBallistic : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  // instance control variables
  jhcMatrix *cpos, *cdir; 

  // link to hardware
  jhcGenRWI *rwi;

  // reported events
  jhcAliaNote *rpt;


// PRIVATE MEMBER PARAMETERS
private:
  // parameters for translation
  double stf, qtf, step, move, drive, ftime;

  // parameters for rotation
  double srf, qrf, turn, rot, spin;

  // motion progress
  double mprog, tprog;
  int mstart, mmid, tstart, tmid;

  // parameters for lift stage
  double slf, qlf, lift, lprog;
  int lstart, lmid;

  // parameters for grabbing
  double wtol, gprog, fhold;
  int gstart, gmid, fask;

  // parameters for arm extension
  double extx, exty, extz, edir, etip;

  // parameters for hand shift
  double dxy, dz, hdone, zdone, hprog;
  int hstart, hmid;

  // parameters for wrist reorientation
  double wpan, wtilt, wroll, wdone, wprog;
  int wstart, wmid;

  // parameters for neck reorientation
  double npan, ntilt, sgz, qgz, ndone, nprog;
  int nstart, nmid;


// PUBLIC MEMBER VARIABLES
public:
  jhcParam tps, rps, pps, lps, gps, aps, hps, wps, nps;
  int gok;                   // whether succeeds without body
  int dbg;                   // control of diagnostic messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcBallistic (); 
  jhcBallistic ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int trans_params (const char *fname);
  int rot_params (const char *fname);
  int prog_params (const char *fname);
  int lift_params (const char *fname);
  int grab_params (const char *fname);
  int arm_params (const char *fname);
  int hand_params (const char *fname);
  int wrist_params (const char *fname);
  int neck_params (const char *fname);

  // overridden virtuals
  void local_platform (void *soma);
  void local_reset (jhcAliaNote& top);
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

  // overall poses
  JCMD_DEF(ball_stop);

  // translation
  JCMD_DEF(ball_drive);
  int get_dist (double& dist, const jhcAliaDesc *act) const;
  int get_vel (double& speed, const jhcAliaDesc *act) const;

  // rotation
  JCMD_DEF(ball_turn);
  int get_ang (double& ang, const jhcAliaDesc *act) const;
  int get_spin (double& speed, const jhcAliaDesc *act) const;

  // lift
  JCMD_DEF(ball_lift);
  int get_up (double& dist, const jhcAliaDesc *act) const;
  int get_vsp (double& speed, const jhcAliaDesc *act) const;

  // grip 
  JCMD_DEF(ball_grip);
  int get_hand (double& width, const jhcAliaDesc *act) const;

  // arm 
  JCMD_DEF(ball_arm);
  int get_pos (int i, const jhcAliaDesc *act);

  // wrist 
  JCMD_DEF(ball_wrist);
  int get_dir (int i, const jhcAliaDesc *act);

  // neck 
  JCMD_DEF(ball_neck);
  int get_gaze (int i, const jhcAliaDesc *act);
  int get_gsp (double& speed, const jhcAliaDesc *act) const;

  // utilities
  int set_degs (double& ang, const jhcAliaDesc *amt) const;
  int set_inches (double& dist, const jhcAliaDesc *amt, double clip) const;
  bool stuck (int i, double err, double prog, int start, int mid);
  int err_hw (const char *sys);

};




