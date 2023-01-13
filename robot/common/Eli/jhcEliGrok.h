// jhcEliGrok.h : basic runtime loop with post-processed sensors and high level commands
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#include "Processing/jhcDraw.h"        // common video

#include "Body/jhcEliBody.h"           // common robot
#include "Environ/jhcLocalOcc.h"
#include "Environ/jhcTable.h"
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
//     +Table                supporting surfaces
//     +EliGrab              manipulation
// 
// </pre>

class jhcEliGrok : public jhcBackgRWI, private jhcDraw
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg limb, mark, mark2;
  UL32 tnow;
  int phy, seen, reflex, batt;

  // high-level commands
  double sx, sy, ssp, vd, va, vsp, voff, xsp;
  int wlock, wwin, slock, vlock, xlock, flock;

  // navigation goal 
  UL32 ahead;
  int feet, act;
  char nmode[4][20];


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
  jhcSurfObjs sobj;                    // depth-based object detection
  jhcTable tab;                        // supporting surfaces
  
  // head visibility parameters
  jhcParam vps;
  double lvis, rvis, tvis, bvis, gtime, side, btime;

  // saccade control parameters
  jhcParam sps;
  double hem, umat, sacp, sact, sact2, road, cruise;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcEliGrok ();
  jhcEliGrok ();
  void BindBody (jhcEliBody *b);
  const jhcImg *HeadView () const {return &mark;}
  const jhcImg *MapView () const  {return &mark2;}
  const char *NavGoal () const    {return(((act < 0) || (act > 3)) ? NULL : nmode[act]);}
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

  // combination sensing
  int ClosestFace (double front =0.0, int cnt =1) const;
  int HeadAlong (jhcMatrix& head, double aim =0.0, double dev =5.0) const;

  // high-level people commands
  int WatchPerson (int id, int bid =10);
  void OrientToward (const jhcMatrix *targ, int bid =10);
  double PersonErr (int id) const;

  // high-level navigation commands
  int SeekLoc (double tx, double ty, double sp =1.0, int bid =10);
  int SeekLoc (const jhcMatrix& targ, double sp =1.0, int bid =10) 
    {return SeekLoc(targ.X(), targ.Y(), sp, bid);}
  int ServoPolar (double td, double ta, double off =0.0, double sp =1.0, int bid =10);
  int ServoLoc (const jhcMatrix& targ, double off =0.0, double sp =1.0, int bid =10)
    {return ServoPolar(targ.PlaneVec3(), targ.PanVec3() - 90.0, off, sp, bid);}
  double FrontDist (double td, double ta) const;
  double FrontDist (const jhcMatrix *targ) const
    {return((targ == NULL) ? -1.0 : FrontDist(targ->PlaneVec3(), targ->PanVec3() - 90.0));} 
  int Explore (double sp =1.0, int bid =10);
  int MapPath (int bid =10);
  bool Survey () const {return((feet >= 1) && (feet <= 4));}

  // debugging graphics
  int Skeleton (jhcImg& dest, double ray =6.0) const;
  int MapArm (jhcImg& map, double ray =6.0) const;
  const jhcImg *ArmMask (jhcImg& map, int clr =1) const;
  double ImgJt (double& ix, double& iy, int jt) const;
  double ImgVeer (int mx, int my, int jt1, int jt0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr_ptrs ();
  
  // processing parameters
  int vis_params (const char *fname);
  int sacc_params (const char *fname);

  // high-level people commands
  void assert_watch ();

  // high-level navigation commands
  void assert_seek ();
  void assert_servo ();
  void assert_explore ();
  void assert_scan ();
  int quick_survey (int bid);

  // interaction overrrides and helpers
  void body_update ();
  void interpret ();
  void interpret2 ();
  void body_issue ();
  void adjust_heads ();
  int base_mode ();

  // debugging graphics
  void cam_img ();
  void nav_img ();


};


#endif  // once




