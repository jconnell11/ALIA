// jhcJoint.h : one rotational DOF for a Dynamixel servo chain
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
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

#ifndef _JHCJOINT_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCJOINT_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <math.h>

#include "Data/jhcParam.h"               // common video

#include "Geometry/jhcMatrix.h"          // common robot
#include "Geometry/jhcMotRamp.h"
#include "Peripheral/jhcDynamixel.h"     


//= One rotational DOF for a Dynamixel servo chain.
// Note: angle and rate in member variables reflects physical device
//       transforms and vector may represent imagined configuration

class jhcJoint : public jhcMotRamp
{
// PRIVATE MEMBER VARIABLES
private:
  jhcMatrix orig, xdir, ydir, zdir;
  int err, err2;
  double th, sv, f, th2, sv2, f2, prev;
  double off, a0, a1, rng;
  jhcDynamixel *dyn;
  jhcMatrix dhm, fwd;


// PUBLIC MEMBER VARIABLES
public:
  char name[80], group[20];
  int jnum;

  // servo parameters
  jhcParam sps;
  int id, id2;
  double stiff, step, zero, amin, amax;

  // geometry parameters
  jhcParam gps;
  double dhd, dhr, dht, dha, cal; 


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcJoint ();
  void Bind (jhcDynamixel *ctrl) {dyn = ctrl;}
  void InitGeom ();
  int Boot (int chk =1);
  int Reset ();
  int Check (int noisy =0);
  double Battery ();
  void Inject (double degs) {th = degs;}

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;
  void SetServo (int n, int n2, double st, double p, double v, double a, double d, double frac);
  void SetGeom (double d, double r, double t, double a, double c, double z, double a0, double a1);
  void SetStiff (double st);

  // command functions
  void SetTarget (double degs, double rate) {RampTarget(CycNorm(degs), rate);}
  double CycNorm (double degs, int clamp =1) const;
  double Clamp (double degs) const;
  int Limp (int doit =1);
  int SetAngle (double degs, double dps =30.0);
  int ServoCmd (int *sv, double *pos, double *vel, int n, double degs, double dps =30.0);
  int ServoNums (int *sv, int n);
  
  // status functions
  int GetState (); 
  double Angle () const {return th;}
  double Speed () const {return sv;}
  double Torque (double tmax =1.0) const;   
  double AdjBal (double inc =0.0, double lo =0.0, double hi =0.1); 
  double Previous () const {return prev;}
  int Flags () const       {return(err | err2);}
  double CtrlDiff (double a1, double a0) const;
  double CtrlErr (double a) const;

  // geometry functions
  void SetMapping (double degs, const jhcJoint *prev =NULL, 
                   double x0 = 0.0, double y0 =0.0, double z0 =0.0);
  int GlobalMap (jhcMatrix& gbl, const jhcMatrix& tool, int dir =0) const;
  const jhcMatrix *Axis0 () const {return &orig;}
  const jhcMatrix *AxisX () const {return &xdir;}
  const jhcMatrix *AxisY () const {return &ydir;}
  const jhcMatrix *AxisZ () const {return &zdir;}
  void End0 (jhcMatrix& loc) const;
  void EndX (jhcMatrix& dir) const;
  void EndY (jhcMatrix& dir) const;
  void EndZ (jhcMatrix& dir) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // configuration
  void ang_limits (double& bot, double& top, double& bot2, double& top2);

  // processing parameters
  int servo_params (const char *fname);
  int geom_params (const char *fname);

  // status functions
  double canonical (double *norm, double a) const;

  // geometry functions
  void dh_matrix (double degs, int full);

};


#endif  // once




