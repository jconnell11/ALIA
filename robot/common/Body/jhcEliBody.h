// jhcEliBody.h : all mechanical aspects of Eli Robot (arm, neck, base, lift)
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
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

#ifndef _JHCELIBODY_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCELIBODY_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"                   // common video
#include "Data/jhcParam.h"
#include "Interface/jms_x.h"
#include "Processing/jhcLUT.h"
#include "Processing/jhcResize.h"
#include "Video/jhcVideoSrc.h"

#include "Body/jhcEliArm.h"                // common robot
#include "Body/jhcEliBase.h"
#include "Body/jhcEliLift.h"
#include "Body/jhcEliNeck.h"
#include "Peripheral/jhcAccelXY.h"
#include "Peripheral/jhcDirMic.h"
#include "Peripheral/jhcDynamixel.h"


//= Controls all mechanical aspects of Eli Robot (arm, neck, base, lift).
// also interfaces to Kinect depth camera and array microphone

class jhcEliBody : private jhcLUT, private jhcResize
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg col, rng, col2;                   // images from Kinect sensor
  DWORD tcmd[10];
  UL32 ntime, ltime, atime, gtime, ttime, mtime;
  int bnum, cw, ch, iw, ih, kin, mok, tstep, tfill;


// PUBLIC MEMBER VARIABLES
public:
  // default robot name and TTS voice
  char rname[80], vname[80], errors[200];
  int loud;

  // AX-12 servo actuators
  jhcDynamixel dyn;
  jhcEliArm arm;
  jhcEliNeck neck;

  // zenither, wheels, and crash sensing
  jhcEliLift lift;
  jhcEliBase base;
  jhcAccelXY acc;

  // sound direction - Kinect (or video) plus audio is external
  jhcVideoSrc *vid;
  jhcDirMic mic;

  // AX-12 communication parameters
  jhcParam bps;
  int dport, dbaud, mega, id0, idn;

  // idle count thresholds
  jhcParam ips;
  int nbid, lbid, abid, gbid, tbid, mbid;

  // static pose parameters
  jhcParam sps;
  double pdef, tdef, hdef;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcEliBody ();
  jhcEliBody ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;

  // configuration
  void BindVideo (jhcVideoSrc *v);
  int SetKinect (int rpt =1);
  int Reset (int rpt =0, int full =0);
  int CfgFile (char *fname, int chk, int ssz); 
  int CommOK (int rpt =1, int bad =0) const;
  int VideoOK () const {if ((vid == NULL) || !vid->Valid()) return 0; return 1;}
  int BodyNum () const {return __max(0, bnum);}
  const char *Problems ();
  double MegaReport ();
  void StaticPose () 
    {neck.Inject(pdef, tdef); lift.Inject(hdef);}

  // configuration - convenience
  template <size_t ssz>
    int CfgFile (char (&fname)[ssz], int chk =0) 
      {return CfgFile(fname, chk, ssz);}

  // resource idle time
  double NeckIdle (UL32 now) const  
    {return jms_secs(now, ntime);}
  double LiftIdle (UL32 now) const  
    {return jms_secs(now, ltime);}
  double ArmIdle (UL32 now) const   
    {return jms_secs(now, atime);}
  double GripIdle (UL32 now) const  
    {return jms_secs(now, gtime);}
  double ManipIdle (UL32 now) const 
    {return __min(jms_secs(now, atime), jms_secs(now, gtime));}
  double TurnIdle (UL32 now) const  
    {return jms_secs(now, ttime);}
  double MoveIdle (UL32 now) const  
    {return jms_secs(now, mtime);}
  double BaseIdle (UL32 now) const  
    {return __min(jms_secs(now, ttime), jms_secs(now, mtime));}
  double BodyIdle (UL32 now) const;

  // read-only access to camera parameters
  int ColW () const {return cw;}
  int ColH () const {return ch;}
  int XDim () const {return iw;}
  int YDim () const {return ih;}
  double MidX () const {return(0.5 * (iw - 1));}
  double MidY () const {return(0.5 * (ih - 1));}
  double ColMidX () const {return(0.5 * (cw - 1));}
  double ColMidY () const {return(0.5 * (ch - 1));}
  double ColScale () const {return(cw / (double) iw);}
  void BigSize (jhcImg& dest) const {dest.SetSize(cw, ch, 3);}
  void SmallSize (jhcImg& dest) const {dest.SetSize(iw, ih, 3);}
  void DepthSize (jhcImg& dest) const {dest.SetSize(iw, ih, 1);}
  int FrameMS () const {return tstep;}
  double FrameTime () const {return(0.001 * tstep);}

  // access to Kinect images
  const jhcImg& Color () const {return col;}  /** Returns native resolution RGB image. */
  const jhcImg& Range () const {return rng;}  /** Returns native (8 or 16) depth map.  */
  int ImgSmall (jhcImg& dest);
  int ImgBig (jhcImg& dest); 
  int Depth8 (jhcImg& dest) const; 
  int Depth16 (jhcImg& dest) const; 

  // image acquisition 
  bool NewFrame () const {return(vid != NULL);}
  const jhcImg *View () const {return &col;}
  
  // basic actions
  int Freeze ();
  int Limp ();

  // main functions
  int UpdateImgs ();
  int Update (int voice =0, int imgs =1, int bad =0);
  void CamPose (double& pan, double& tilt, double& ht);
  int Issue (double lead =3.0);

  // ballistic functions
  void Beep ();
  int InitPose (double ht =0.0); 


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int body_params (const char *fname);
  int idle_params (const char *fname);
  int static_params (const char *fname);

  // configuration
  void chk_vid (int start);

};


#endif  // once




