// jhcEliGrok.h : basic runtime loop with post-processed sensors and high level commands
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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
#include "Eli/jhcEliWatch.h"
#include "Environ/jhcLocalOcc.h"
#include "Objects/jhcSurfObjs.h"
#include "People/jhcFaceName.h"
#include "People/jhcSpeaker.h"
#include "People/jhcStare3D.h"

#include "Body/jhcBackgRWI.h"


//= Basic runtime loop with post-processed sensors and high level commands.
// holds basic body control and sensors as well as follow-on processing modules
// generally processing belongs here while links to FCNs are in a kernel class
// allows attachment of different versions of body but assumes not shared
// <pre>
// class tree overview (+ = member, > = pointer):
//
//   EliGrok              
//     BackgRWI
//     +Stare3D              depth-based person finder
//       Track3D
//         Parse3D
//       Overhead3D
//         Surface3D
//           PlaneEst
//     +FaceName             face recognition
//       HeadGaze
//         Frontal
//           +FFindOCV
//       +FRecoDLL
//       >Stare3D
//     +Speaker              determine talking head
//       >Stare3D
//       >DirMic
//     +LocalOcc             navigation
//       Overhead3D
//         Surface3D
//           PlaneEst
//     +SurfObjs             object detection
//       Bumps
//         Overhead3D
//           Surface3D
//             PlaneEst
//         +SmTrack
//       +PatchProps
//     +EliWatch             gaze behaviors
// 
// </pre>

class jhcEliGrok : public jhcBackgRWI
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg mark, mark2;
  UL32 tnow;
  int phy, seen, reflex, batt;

  // high-level commands
  double sx, sy, ssp, vd, va, vsp, voff;
  int wlock, wwin, slock, vlock;

  // navigation goal 
  int approach, follow, feet;
  double sweep;
  char nmode[20];


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

  // innate behaviors
  jhcEliWatch watch;
  
  // head visibility parameters
  jhcParam vps;
  double lvis, rvis, tvis, bvis, gtime, side, btime;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcEliGrok ();
  jhcEliGrok ();
  void BindBody (jhcEliBody *b);
  const jhcImg *HeadView () const {return &mark;}
  const jhcImg *MapView () const  {return &mark2;}
  const char *Watching (int dash =1) const   
    {return watch.Behavior((phy <= 0) ? -1 : neck->GazeWin(), dash);}
  const char *NavGoal ();
  bool Ghost () const   {return(phy <= 0);}
  UL32 CmdTime () const {return tnow;}

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
  void OrientToward (const jhcMatrix *targ, int bid);
  double PersonErr (int id) const;

  // high-level navigation commands
  int SeekLoc (double tx, double ty, double sp =1.0, int bid =10);
  int SeekLoc (const jhcMatrix& targ, double sp =1.0, int bid =10) 
    {return SeekLoc(targ.X(), targ.Y(), sp, bid);}
  int ServoPolar (double td, double ta, double sp =1.0, int bid =10, double off =0.0);
  double FrontDist (double td, double ta) const;
  double FrontDist (const jhcMatrix *targ) const
    {return((targ == NULL) ? -1.0 : FrontDist(targ->PlaneVec3(), targ->PanVec3() - 90.0));} 


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr_ptrs ();
  
  // processing parameters
  int vis_params (const char *fname);

  // high-level people commands
  void assert_watch ();

  // high-level navigation commands
  void assert_seek ();
  void assert_servo ();
  int quick_survey ();

  // interaction overrrides and helpers
  void body_update ();
  void interpret ();
  void interpret2 ();
  void body_issue ();
  void adjust_heads ();

  // debugging graphics
  void cam_img ();
  void nav_img ();


};


#endif  // once




