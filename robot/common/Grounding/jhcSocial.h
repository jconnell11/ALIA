// jhcSocial.h : interface to ELI people tracking kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#ifndef _JHCSOCIAL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSOCIAL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Reasoning/jhcAliaNote.h"     // common audio
#include "Semantic/jhcAliaDesc.h" 

#include "Data/jhcParam.h"             // common video

#include "Action/jhcStdKern.h"         // common robot
#include "Eli/jhcEliGrok.h"       


//= Interface to ELI people tracking kernel for ALIA system.
//   DO: allows user to briefly "look" at someone, or continuously "watch" someone
//       also accepts the commands "come here", "approach X", and "follow X"
//  CHK: can determine if a particular someone is visible 
// FIND: can tell who is currently visible and who is closest
// NOTE: spontaneously volunteers "I see X" for people on the face reco VIP list
//       and produces the event "X is close" even if no name is known

class jhcSocial : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  // link to hardware
  jhcEliGrok *rwi;
  jhcEliNeck *neck;

  // reported events
  jhcAliaNote *rpt;

  // event state
  UL32 seen;
  int folks, pal, prox, reco, uid;


// PUBLIC MEMBER VARIABLES
public:
  // control of diagnostic messages
  int dbg;                   

  // attention parameters
  jhcParam aps;
  double pnear, alone, scare, ltol, lquit;

  // sound localization parameters
  jhcParam sps;
  double pdist, rtime, sdev, aimed, gtime, side, btime;
  int recent;

  // motion parameters 
  jhcParam mps;
  double cozy, direct, aquit, ideal, worry, orient, atime, ftime;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSocial (); 
  jhcSocial ();
  void Platform (jhcEliGrok *robot);

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int attn_params (const char *fname);
  int snd_params (const char *fname);
  int move_params (const char *fname);

  // overridden virtuals
  void local_reset (jhcAliaNote *top);
  void local_volunteer ();
  int local_start (const jhcAliaDesc *desc, int i);
  int local_status (const jhcAliaDesc *desc, int i);

  // reported events
  void dude_seen ();
  void dude_close ();
  void vip_seen ();
  void lost_dudes ();
  jhcAliaDesc *agt_node (int t, int& born);
  void std_props (jhcAliaDesc *agt, int t, int born);

  // looking for speaker
  JCMD_DEF(soc_talk);

  // orienting toward people
  JCMD_DEF(soc_look);

  // moving toward people
  JCMD_DEF(soc_approach);
  JCMD_DEF(soc_retreat);
  JCMD_DEF(soc_follow);

  // explore environment
  JCMD_DEF(soc_explore);

  // utilities
  int chk_neck (int i, double err);
  int chk_base (int i, double err);

  // semantic messages
  int err_body ();
  int err_person (jhcAliaDesc *dude);


};


#endif  // once




