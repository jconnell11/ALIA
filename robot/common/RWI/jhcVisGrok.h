// jhcVisGrok.h : post-processed sensors and high-level behaviors 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2026 Etaoin Systems
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

#include "Data/jhcImg.h"               // common video
#include "Data/jhcParam.h"
#include "Processing/jhcColor.h"       
#include "Processing/jhcDraw.h"            
#include "Processing/jhcHist.h"            
#include "Processing/jhcLabel.h"        
#include "Processing/jhcLUT.h"        
#include "Processing/jhcThresh.h"        
#include "Processing/jhcWarp.h"        

#include "Environ/jhcLocalOcc.h"       // common robot
#include "Environ/jhcTable.h"
#include "Environ/jhcVisMotion.h"
#include "Geometry/jhcMatrix.h"
#include "Objects/jhcSurfObjs.h"
#include "People/jhcFaceName.h"
#include "People/jhcSpeaker.h"
#include "People/jhcStare3D.h"

#include "RWI/jhcBgndGrok.h"


//= Post-processed sensors and high-level behaviors.
// holds basic body control and sensors as well as follow-on processing modules
// generally processing belongs here while links to FCNs are in a kernel class
// allows attachment of different versions of body but assumes not shared

class jhcVisGrok : public jhcBgndGrok, private jhcColor, private jhcDraw, private jhcHist, 
                   private jhcLabel, private jhcLUT, private jhcThresh
{
// PRIVATE MEMBER VARIABLES
private:
  jhcWarp cw;
  jhcImg flat, limb, tmp, tmp2, mark, mark2;
  char tmap[80];
  double tcyc;
  UL32 tnow;
  int phy, reflex, wmap, hmap, sprc, first;

  // sensor height adjustment
  double zadj;

  // high-level commands
  double wlim, sx, sy, ssp, vd, va, vsp, voff, xsp;
  int wlock, wwin, slock, vlock, xlock, flock;

  // navigation goal 
  UL32 ahead;
  int feet, act;
  char nmode[4][20];

  // search for free path
  double view;

  // finger separation estimate
  double sep;


// PRIVATE MEMBER PARAMETERS
private:
  // rangefinder parameters
  double rflen, rsc, ign, west;

  // color camera parameters
  double cmx, cmy, cf0, cr2, cr4, cmag;
  int cenh;

  // sensor calibration parameters
  double rp0, rt0, rr0, cp0, ct0, cr0;

  // arm padding parameters
  double lpad, hpad, fpad;

  // head visibility parameters
  double lvis, rvis, tvis, bvis, lost, side, btime;

  // saccade control parameters
  double hem, umat, sacp, sacp2, sact, sact2, road, cruise;

  // balance restoration
  UL32 esc0;
  double esc_t, esc_m;

  // subconscious gaze parameters
  double tip, back;
  int pbid, obid, mbid, sbid, rbid;


// PUBLIC MEMBER VARIABLES
public:
  // processing elements
  jhcStare3D s3;                       // head finder using depth
  jhcFaceName fn;                      // face ID and gaze for heads
  jhcSpeaker tk;                       // sound location vs people
  jhcLocalOcc nav;                     // navigation obstacles
  jhcSurfObjs sobj;                    // depth-based object detection
  jhcTable tab;                        // supporting surfaces
  jhcVisMotion mot;                    // motion areas in image

  // input images (possibly shared)
  jhcImg *raw, *rng, *col, *aux;

  // parameter sets
  jhcParam rps, cps, eps, pps, vps, sps, aps;

  // debugging image to show
  const jhcImg *space2;
  int probe;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcVisGrok ();
  jhcVisGrok ();
  int XDim () const {return wmap;}
  int YDim () const {return hmap;}
  const char *Title () const {return tmap;}
  const char *NavGoal () const    
    {return(((act < 0) || (act > 3)) ? NULL : nmode[act]);}
  bool Ghost () const   {return(phy <= 0);}
  UL32 CmdTime () const {return tnow;}
  int SpeechRC () const {return sprc;}
  const char *FaceSpeak () const
    {return fn.FaceName(s3.TrackIndex(tk.Speaking()));}
  int VisTips () const {return((west > 0.0) ? 1 : 0);}
  double VisWid () const {return sep;}
  double Cycle () const  {return tcyc;}

  // access to actual input images
  const jhcImg& Color () const  {return *col;} 
  const jhcImg& Range () const  {return *rng;}
  const jhcImg& AuxCam () const {return *aux;}

  // processing parameter bundles 
  int LoadCfg (const char *fname =NULL);
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname);

  // main functions
  void Reset (int rob =1, int behaviors =1, int choke =1760);
  int Update (int voice =0, UL32 resume =0);
  void Stop ();

  // combination sensing
  int ClosestFace (double front =0.0, int cnt =1) const;
  int HeadAlong (jhcMatrix& head, double aim =0.0, double dev =5.0) const;

  // high-level people commands
  int WatchPerson (int id, double aim =0.0, int bid =10);
  void OrientToward (const jhcMatrix *targ, double aim =0.0, int bid =10);
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

  // user images
  const jhcImg *HeadView (const char *msg =NULL);
  const jhcImg *MapView (const char *msg =NULL);

  // log images
  void DumpImages (const char *wdir);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr_ptrs ();
  
  // processing parameters
  int rng_params (const char *fname);
  int cam_params (const char *fname);
  int err_params (const char *fname);
  int pad_params (const char *fname);
  int vis_params (const char *fname);
  int sacc_params (const char *fname);
  int auto_params (const char *fname);

  // high-level people commands
  void assert_watch ();

  // high-level navigation commands
  void assert_seek ();
  void assert_servo ();
  void assert_explore ();
  void assert_scan ();
  int quick_survey (int bid);

  // subconscious behaviors
  void assert_gaze ();
  void assert_tip ();

  // core processing overrrides 
  void body_update ();
  void umwelt ();
  void umwelt2 ();
  void umwelt3 ();
  void umwelt4 ();
  void body_issue ();

  // override helpers
  double finger_sep ();
  void adjust_heads ();
  int base_mode ();

  // user images
  void cam_img ();
  const char *neck_ctrl (char *txt, int ssz) const;
  void nav_img ();
  const jhcImg *integrated_map ();
  const jhcImg *front_depth ();

  const jhcImg *surface_bumps ();
  const jhcImg *object_heights ();
  const jhcImg *object_clip ();
  const jhcImg *color_mask ();
  const jhcImg *table_deposit ();  

  const jhcImg *floor_heights ();
  const jhcImg *plane_devs ();
  const jhcImg *obstacle_scan ();
  const jhcImg *free_space ();

  const jhcImg *person_heights ();
  const jhcImg *person_blobs ();
  const jhcImg *head_shoulder ();
  const jhcImg *head_gaze ();

  const jhcImg *head_sound ();
  const jhcImg *vis_motion ();

};




