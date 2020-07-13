// jhcManusBody.h : basic control of Manus small forklift robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#ifndef _JHCMANUSBODY_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMANUSBODY_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"             // common video
#include "Interface/jhcSerial.h"
#include "Processing/jhcWarp.h"
#include "Video/jhcVideoSrc.h"

#include "Body/jhcManusX.h"


//= Basic control of Manus small forklift robot.
// Configure: connect USB to Pololu board and start Maestro Control Center
//   Serial Settings tab:
//     UART, fixed baud rate 230400
//   Channel Setting tab:
//     ch 0  left wh:  servo  limit 1000-2000 
//     ch 1 forklift:  servo  limit  700-2000  
//     ch 2 distance:  input
//     ch 3 right wh:  servo  limit 1000-2000   
//     ch 4  gripper:  servo  limit  500-2500    
//     ch 5  squeeze:  input
// then click Apply Settings

class jhcManusBody : public jhcManusX
{
// PRIVATE MEMBER VARIABLES
private:
  // servo channels
  //     0     1     2    3     4    5
  enum {LFW, LIFT, DIST, RTW, HAND, WID}; 

  // image acquisition
  jhcWarp wp;
  jhcImg now;
  jhcVideoSrc *vid;
  int wifi;

  // communications and state
  jhcSerial ser;
  UC8 ask[32], pod[8];
  UL32 tlast;

  // state
  double pos[6];
  double lvel, rvel;

  // individual calibration
  char cfile[200];
  int id;

  // last gripper width request
  double wcmd;
  int pgrip;

  // sensor conversion factors
  double voff, rsc, roff, wsc;

  // command conversion factors
  double msc, tsc, lsc, lsf, gsc;
  

// PUBLIC MEMBER VARIABLES
public:
  // serial port search
  int port0;

  // camera parameters
  jhcParam cps;
  double w2, w4, mag, roll;

  // depth parameters
  jhcParam rps;
  double r0, r4, r12;
  int v0, v4, v12;

  // width parameters
  jhcParam wps;
  double vmax, vfat, vmed, vmin, wfat, wmed;

  // drive parameters
  jhcParam dps;
  double vcal, bal, sep;
  int lf0, rt0, ccal, dacc;

  // lift parameters
  jhcParam lps;
  double hdef, hout, arm;
  int ldef, lout, lsp, lacc;

  // grip parameters
  jhcParam gps;
  int gmax, gmin, gsp;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcManusBody ();
  jhcManusBody ();
  void SetID (int n);
  int RobotID () const {return id;}
  void SetSize (int x, int y);

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int LoadCfg (const char *dir, int robot =0, int noisy =1);
  int SaveCfg (const char *dir);
  const char *CfgName (const char *dir =NULL);

  // camera connection
  void BindVideo (jhcVideoSrc *v);
  int SetWifiCam (int rpt =1);
  const jhcImg *Raw () const {return &now;}

  // main functions
  int Reset (int noisy =0, const char *dir =NULL, int prefer =1);
  void Stop ();

  // rough odometry
  void Zero ();

  // core interaction
  int Update (int img =1);
  int UpdateImg (int rect =1);
  void Rectify ();
  int Issue ();


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int cam_params (const char *fname);
  int range_params (const char *fname);
  int width_params (const char *fname);
  int drive_params (const char *fname);
  int lift_params (const char *fname);
  int grip_params (const char *fname);

  // main functions
  int find_robot (int prefer, int noisy);
  int test_port (int n, int noisy);
  bool test_id (int i);
  void chan_coefs ();
  void v2d_eqn ();
  void servo_defs ();

  // core interaction
  double get_lf (double us) const;
  double get_rt (double us) const;
  double get_lift (double us) const;
  double get_grip (double ad) const;
  double get_dist (double ad) const;
  void inc_odom (UL32 tnow);
  void send_wheels (double ips, double dps);
  void send_lift (double ips);
  void send_grip (int dir);

  // low level serial
  int req_all ();
  int rcv_all ();
  void set_target (int ch, double us);
  void set_speed (int ch, int inc_t, int imm);
  void set_accel (int ch, int inc_v);


};


#endif  // once




