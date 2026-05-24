// jhcSwapBody.h : body components with buffering of sensors and commands
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2026 Etaoin Systems
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

#include "jhc_pthread.h"               // for double buffering

#include "Data/jhcImg.h"               // common video

#include "Body/jhcSwapArm.h"           // common robot
#include "Body/jhcSwapBase.h"
#include "Body/jhcSwapNeck.h"
#include "Body/jhcSwapLift.h"
#include "Geometry/jhcMatrix.h"

#include "Body/jhcGenBody.h"


//= Body components with buffering of sensors and commands.
// local code mostly handles camera poses and images 
// provides support for double buffering (if desired)

class jhcSwapBody : public jhcGenBody
{
// PRIVATE MEMBER VARIABLES
private:
  // TOF cam resampling
  int sx[480], fx[480], sy[640], fy[640];

  // body attitude wrt gravity
  double pitch, roll;        

  // double buffered input
  jhcImg img_c0, img_r0, img_a0;
  jhcMatrix pos_r0, dir_r0, pos_c0, dir_c0, pos_a0, dir_a0;
  double pitch0, roll0;
  int seen0;

  // exclusive access
  pthread_mutex_t io_lock;


// PUBLIC MEMBER VARIABLES
public:
  // components
  jhcSwapNeck neck0;
  jhcSwapArm arm0;
  jhcSwapLift lift0;         // for eventual Eli port
  jhcSwapBase base0;

  // double buffered input images
  jhcImg raw, img_r, img_c, img_a;

  // current camera poses and readiness
  jhcMatrix pos_r, dir_r, pos_c, dir_c, pos_a, dir_a;
  int seen;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSwapBody ();
  jhcSwapBody ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // attitude
  double Pitch () const {return pitch;}
  double Roll () const  {return roll;}

  // sensor info
  int RngReady () const  {return seen;} 
  double RngStatic () const {return __min(base0.Oriented(), neck0.Stare());}
  double ColStatic () const {return RngStatic();}
  void RngPose (jhcMatrix& pos, jhcMatrix& dir, double zadj =0.0) const
    {pos.RelVec3(pos_r, 0.0, 0.0, zadj); dir.Copy(dir_r);}   
  void ColPose (jhcMatrix& pos, jhcMatrix& dir, double zadj =0.0) const
    {pos.RelVec3(pos_c, 0.0, 0.0, zadj); dir.Copy(dir_c);}  
  void AuxPose (jhcMatrix& pos, jhcMatrix& dir) const
    {pos.Copy(pos_a); dir.Copy(dir_a);}                      // no fudge

  // configuration
  void Reset ();

  // overall attitude
  void Status (double up, double ccw);

  // sensor poses
  void ColorPose (double x, double y, double z, double p, double t, double r);
  void RangePose (double x, double y, double z, double p, double t, double r);
  void AuxCamPose (double x, double y, double z, double p, double t, double r);

  // input images
  int SetRange (const void *pels, int& fmt);
  int SetColor (const void *pels, int& fmt);
  int SetAuxCam (const void *pels, int& fmt);

  // core interaction
  int Update (int voice =0, int imgs =1);
  int Issue (double lead =3.0);

  // exclusive sensor and command access
  void Lock ();
  void Unlock ();


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and configuration
  void tof_sampling ();

  // input images
  int set_z16_bot (jhcImg& dest, const void *pels) const;
  int set_f32_top (jhcImg& dest, const void *pels) const;
  int set_z16_tof (jhcImg& dest, const void *pels) const;
  int set_bgr_bot (jhcImg& dest, const void *pels) const;
  int set_rgb_top (jhcImg& dest, const void *pels) const;
  int set_bgr_top (jhcImg& dest, const void *pels) const;

};

