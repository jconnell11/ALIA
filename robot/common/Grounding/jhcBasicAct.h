// jhcBasicAct.h : interface to Manus motion kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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

#ifndef _JHCBASICACT_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBASICACT_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "API/jhcAliaNote.h"           // common audio
#include "API/jhcAliaDesc.h" 

#include "Data/jhcParam.h"             // common video

#include "RWI/jhcManusRWI.h"           // common robot

#include "Kernel/jhcStdKern.h"       


//= Interface to Manus motion kernel for ALIA system.

class jhcBasicAct : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  // link to hardware
  jhcManusRWI *rwi;

  // gripper goal and status
  int hold;

  // distance sensor
  jhcAliaNote *rpt;
  int warn;


// PRIVATE MEMBER PARAMETERS
private:
  // miscellaneous parameters
  double dtrig, dtol, ftime, gtime;

  // parameters for translation
  double ips, stf, qtf, step, move, drive, madj, dadj;

  // parameters for rotation
  double dps, srf, qrf, turn, rot, spin, radj, sadj;

  // parameters for lift stage
  double zps, slf, qlf, lift;


// PUBLIC MEMBER VARIABLES
public:
  int dbg;                   // controls diagnostic messages
  jhcParam mps, tps, rps, lps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcBasicAct (); 
  jhcBasicAct ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int trans_params (const char *fname);
  int rot_params (const char *fname);
  int lift_params (const char *fname);
  int misc_params (const char *fname);

  // overridden virtuals
  void local_platform (void *soma);
  void local_reset (jhcAliaNote& top);
  void local_volunteer ();
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

  // distance sensor
  void dist_close ();

  // overall poses
  JCMD_DEF(base_stop);

  // translation
  JCMD_DEF(base_drive);
  int get_vel (double& speed, const jhcAliaDesc *act) const;
  int get_dist (double& dist, const jhcAliaDesc *act) const;

  // rotation
  JCMD_DEF(base_turn);
  int get_spin (double& speed, const jhcAliaDesc *act) const;
  int get_ang (double& ang, const jhcAliaDesc *act) const;

  // lift
  JCMD_DEF(base_lift);
  int get_vert (double& speed, const jhcAliaDesc *act) const;

  // grip 
  JCMD_DEF(base_grip);
  int get_hand (double& grab, const jhcAliaDesc *act) const;

  // utlities
  int set_degs (double& ang, const jhcAliaDesc *amt) const;
  int set_inches (double& dist, const jhcAliaDesc *amt, double clip) const;


};


#endif  // once




