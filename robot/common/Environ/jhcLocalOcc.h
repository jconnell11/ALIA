// jhcLocalOcc.h : builds and maintains local occupancy map around robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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
#include "Processing/jhcGroup.h"
#include "Processing/jhcLUT.h"
#include "Processing/jhcStats.h"

#include "Depth/jhcOverhead3D.h"


//= Builds and maintains local occupancy map around robot.
// analyzes height in narrow range around presumed floor (typ. -4" to +4")
// once a pixel has been determined to be floor it remains so until forgotten
// obstacles perceived where floor was previoulsy only exist when viewed (temporary)
// uses obstacles to limit motion, only known floor pixels are assumed okay
// obstacle = 255 (white), temporary = 200 (red), drop = 128 (green), floor = 50 (blue)

class jhcLocalOcc : public jhcOverhead3D, private jhcGroup, private jhcLUT, private jhcStats
{
friend class CBanzaiDoc;  // for debugging

// PRIVATE MEMBER VARIABLES
private:
  static const int tail = 500;         /** Position history length. */

  // basic map formation
  jhcImg dev, bad, scan, sea, obst, conf;

  // robot position 
  double rx, ry, raim;

  // map fading
  double rate;
  int cwait, ccnt;
  UC8 cmax, ctmp;

  // travel clearance
  jhcImg spin[18];
  double dist[36];
  int ndir, rt0, lf1;

  // navigation indicators
  double known;
  int side, stuck;

  // debugging graphics
  double xhist[tail], yhist[tail];
  int nh, fill, minit;


// PRIVATE MEMBER PARAMETERS
private:
  // ground mapping parameters
  double dej, hat, fbump;
  int thin, drop;

  // robot size and map fading parameters 
  double rside, rfwd, rback, pad, chute, fade, temp;
  int hole;

  // sensors and avoidance parameters
  double veer, lead, wmat, hmat, tmat, glide, orient;
  int free;


// PUBLIC MEMBER VARIABLES
public:
  int dbg;                   // controls diagnostic messages
  jhcParam eps, gps, nps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcLocalOcc ();
  jhcLocalOcc ();
  double Nose () const {return rfwd;}
  double Hip () const  {return rside;}
  const jhcImg& Deviations () const  {return dev;}
  const jhcImg& Terrain () const     {return scan;}
  const jhcImg& OpenAreas () const   {return bad;}
  const jhcImg& Traversable () const {return sea;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // synthetic sensors
  double Step () const {return(180.0 / ndir);}
  bool Blind (double umat =0.5) const {return(known < umat);}
  bool Tight (double hem =6.0) const;
  double Forward () const {return dist[ndir];}

  // main functions
  void Reset ();
  void AdjustMaps (double fwd, double lf, double dr);

  // map building and sensors
  void RefineMaps (const jhcImg& d16, const jhcMatrix& pos, const jhcMatrix& dir);
  void RefreshBody (const jhcRoi *map =NULL);
  void ComputePaths ();

  // fancier map building and sensors
  void RefineMaps2 (const jhcImg& d16, const jhcMatrix& pos, const jhcMatrix& dir);
  void ComputePaths2 ();

  // navigation
  void Swerve (double& trav, double& head, double td, double ta, double stop =0.0);
  void Wander (double& trav, double& head);

  // debugging graphics
  int LocalMap (jhcImg &dest, int rot =1) const;
  int Confidence (jhcImg& dest, int rot =1) const;
  int Doormat (jhcImg& dest, int rot =1) const;
  int RobotMark (jhcImg& dest, int rot =1) const;
  int RobotBody (jhcImg& dest, int rot =1) const;
  int RobotDir (jhcImg& dest, int rot =1) const;
  int Dists (jhcImg& dest, int rot =1) const;
  int Paths (jhcImg& dest, int half =0, int rot =1) const;
  int RobotCmd (jhcImg& dest, double head, double trav =1.0) const;
  int Tail (jhcImg& dest, double secs =10.0) const;
  int ScanBeam (jhcImg& dest) const;
  int Target (jhcImg& dest, double tx, double ty, int polar =0) const; 


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int env_params (const char *fname);
  int geom_params (const char *fname);
  int nav_params (const char *fname);

  // main functions
  void set_spin (double da);
  void adj_hist (double fwd, double lf, double dr);
  void known_ahead (double& any, const jhcImg& cf) const;

  // map building and sensors
  double traverse (const jhcImg& basin, double ang) const;

  // fancier map building and sensors
  void mixin_scan (jhcImg& obs, jhcImg& cf, const jhcImg& junk, const jhcImg& flat) const;
  void block_bot (jhcImg& obs, jhcImg& cf) const;
  void erase_blips (jhcImg& obs, const jhcImg& junk) const;
  void build_spin (const jhcImg& env);
  void rigid_samp (jhcImg& dest, const jhcImg& src, double degs) const;
  int clr_paths (double& fwd, double& rwv, jhcImg& view) const;

  // debugging graphics
  double robot_pose (double& rx0, double& ry0, const jhcImg *ref) const;


};

