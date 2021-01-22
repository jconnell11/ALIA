// jhcEliGrok.h : post-processed sensors and innate behaviors for ELI robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#ifndef _JHCELIGROK_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCELIGROK_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Body/jhcEliBody.h"           // common robot
#include "Environ/jhcLocalOcc.h"
#include "Objects/jhcPatchProps.h"
#include "Objects/jhcSurfObjs.h"
#include "People/jhcFaceName.h"
#include "People/jhcSpeaker.h"
#include "People/jhcStare3D.h"

#include "Body/jhcBackgRWI.h"


//= Post-processed sensors and innate behaviors for ELI robot.
// holds basic body control and sensors as well as follow-on processing modules
// generally processing belongs here while links to FCNs are in a kernel class
// allows attachment of different versions of body but assumes not shared

class jhcEliGrok : public jhcBackgRWI
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg mark, mark2;
  UL32 tnow;
  int phy, seen, reflex;

  // target watching control
  char wtarg[9][20];
  jhcMatrix src;
  double prand, trand;
  int seek, slew, delay;
  UL32 idle;

  // high-level commands
  double sx, sy, ssp, vd, va, vsp;
  int wlock, wwin, slock, vlock;


// PUBLIC MEMBER VARIABLES
public:
  // robot and subcomponents
  jhcEliBody *body;                    // allow physical or simulator
  jhcEliBase *base;
  jhcEliNeck *neck;
  jhcEliLift *lift;
  jhcEliArm  *arm;
  jhcDirMic  *mic;

  // processing elements
  jhcStare3D s3;                       // head finder using depth
  jhcFaceName fn;                      // face ID and gaze for heads
  jhcSpeaker tk;                       // sound location vs head
  jhcLocalOcc nav;                     // navigation obstacles
  jhcSurfObjs tab;                     // depth-based object detection
  jhcPatchProps ext;                   // object color analysis

  // watching behaviors bids
  jhcParam wps;
  int freeze, speak, close, sound, stare, face, twitch, rise;

  // self-orientation parameters
  jhcParam ops;
  double edge, hnear, center, pdev, aimed, pdist, hdec;
  int fmin;

  // behavior timing parameters
  jhcParam tps;
  double bored, relax, rdev, gtime, ttime, rtime, side, btime;
  
  // head visibiliy parameters
  jhcParam vps;
  double lvis, rvis, tvis, bvis;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcEliGrok ();
  jhcEliGrok ();
  void BindBody (jhcEliBody *b);
  const jhcImg *HeadView () const {return &mark;}
  const jhcImg *MapView () const  {return &mark2;}
  const char *Watching () const;
  bool Ghost () const {return(phy <= 0);}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname);
  int SaveCfg (const char *fname) const;

  // main functions
  void Reset (int rob =1, int behaviors =1);
  int Update (int voice =0, UL32 resume =0);
  void Stop ();

  // high-level people commands
  int WatchPerson (int id, int bid =10);
  double PersonErr (int id) const;

  // high-level navigation commands
  int SeekLoc (double tx, double ty, double sp =1.0, int bid =10);
  int SeekLoc (const jhcMatrix& targ, double sp =1.0, int bid =10) 
    {return SeekLoc(targ.X(), targ.Y(), sp, bid);}
  int ServoPolar (double td, double ta, double sp =1.0, int bid =10);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr_ptrs ();
  
  // processing parameters
  int watch_params (const char *fname);
  int orient_params (const char *fname);
  int time_params (const char *fname);
  int vis_params (const char *fname);

  // high-level people commands
  void assert_watch ();
  void orient_toward (const jhcMatrix *targ, int bid);

  // high-level navigation commands
  void assert_seek ();
  void assert_servo ();

  // interaction overrrides and helpers
  void body_update ();
  void interpret ();
  void interpret2 ();
  void body_issue ();
  void adjust_heads ();

  // innate behaviors
  void cmd_freeze ();
  void watch_talker ();
  void watch_closest ();
  void gaze_sound ();
  void gaze_stare ();
  void gaze_face ();
  void gaze_random ();
  void head_rise ();

  // debugging graphics
  void head_img ();
  void nav_img ();


};


#endif  // once




