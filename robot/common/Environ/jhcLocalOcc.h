// jhcLocalOcc.h : builds and maintains local occupancy map around robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#ifndef _JHCLOCALOCC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCLOCALOCC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"               // common video
#include "Data/jhcParam.h"
#include "Processing/jhcGroup.h"
#include "Processing/jhcLUT.h"

#include "Depth/jhcOverhead3D.h"


//= Builds and maintains local occupancy map around robot.
// once a pixel has been determined to be floor it remains so until forgotten
// obstacles perceived where floor has been determined only exist when viewed
// uses obstacles to limit motion, floor and unknown pixels are assumed okay
// obstacle = 255 (white), temporary = 200 (red), floor = 128 (green)

class jhcLocalOcc : public jhcOverhead3D, private jhcGroup, private jhcLUT
{
friend class CBanzaiDoc;  // for debugging

// PRIVATE MEMBER VARIABLES
private:
  // basic map formation
  jhcImg dev, bad, obst, conf;

  // robot position 
  double rx, ry, raim;

  // map fading
  double rate;
  int cwait, ccnt;
  UC8 cmax, ctmp;

  // travel clearance
  jhcImg spin[18];
  double dist[36];
  double fresh;
  int ndir, rt0, lf1;

  // navigation
  int trip, stuck;

  // debugging graphics
  double xhist[300], yhist[300];
  int nh, fill;


// PUBLIC MEMBER VARIABLES
public:
  // controls diagnostic messages
  int dbg;

  // ground map parameters
  jhcParam eps;
  double dej, hat, fbump;
  int drop, hole;

  // robot size parameters
  jhcParam gps;
  double rside, rfwd, rback;

  // parameters for known areas
  jhcParam kps;
  double wmat, hmat, tmat, umat, kmat, temp, fade;

  // parameters for motion control
  jhcParam nps;
  double pad, lead, veer, detour, block, stime, btime;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcLocalOcc ();
  jhcLocalOcc ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  void Reset ();
  int AdjustMaps (double fwd, double lf, double dr);
  int RefineMaps (const jhcImg& d16, const jhcMatrix& pos, const jhcMatrix& dir);

  // synthetic sensors
  int NumDir () const  {return ndir;}
  double Step () const {return(180.0 / ndir);}
  int MaxRt () const   {return rt0;}
  int MaxLf () const   {return lf1;}
  double Angle (int dev) const;
  double Path (int dev, int pos =1) const;
  double Ahead (double end =0.0) const;
  double Behind (double end =0.0) const;

  // navigation
  double TurnLimit (double desired, double margin =10.0) const;
  double MoveLimit (double desired, double stop =0.0) const;
  bool Blind ();
  double Avoid (double& trav, double& head, double tx, double ty);
  bool Stymied (double delay =0.5) const
    {return(stuck > ROUND((stime + btime + delay) * rate));}

  int Swerve (double& trav, double& head, double td, double ta) const;

  // debugging graphics
  int LocalMap (jhcImg &dest, int rot =1) const;
  int Confidence (jhcImg& dest, int rot =1) const;
  int Doormat (jhcImg& dest, int rot =1) const;
  int RobotMark (jhcImg& dest, int rot =1) const;
  int RobotBody (jhcImg& dest, int rot =1) const;
  int RobotDir (jhcImg& dest, int rot =1) const;
  int Dists (jhcImg& dest, int rot =1) const;
  int Paths (jhcImg& dest, int rot =1) const;
  int Tail (jhcImg& dest, double secs =10.0) const;
  int ScanBeam (jhcImg& dest) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int env_params (const char *fname);
  int geom_params (const char *fname);
  int conf_params (const char *fname);
  int nav_params (const char *fname);

  // main functions
  void adj_hist (double fwd, double lf, double dr);
//  void expect_floor (jhcImg& dest) const;
  void mixin_scan (jhcImg& obs, jhcImg& cf, const jhcImg& junk, const jhcImg& flat) const;
  void block_bot (jhcImg& obs, jhcImg& cf) const;
  void erase_blips (jhcImg& obs, const jhcImg& junk) const;

  // synthetic sensors
  void set_spin (double da);
  void build_spin (const jhcImg& env);
  void rigid_samp (jhcImg& dest, const jhcImg& src, double degs) const;
  int clr_paths (double& fwd, double& rwv, jhcImg& view) const;
  double known_ahead (const jhcImg& cf) const;

  // navigation
  double pick_dir (double td, double ta) const;

  // debugging graphics
  double robot_pose (double& rx0, double& ry0, const jhcImg *ref) const;


};


#endif  // once




