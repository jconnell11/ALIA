// jhcCeliaNeck.h : control of Celia robot's head pan and tilt actuators
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 IBM Corporation
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

#ifndef _JHCCELIANECK_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCCELIANECK_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"             // common video

#include "Geometry/jhcJoint.h"         // common robot
#include "Peripheral/jhcDynamixel.h"


//= Control of Celia robot's head pan and tilt actuators.
// built using two Robotis AX-12+ Dynamixel servos on serial network
// control mode is a linear, trapezoidal profiled slew to some gaze pose
// commands are speed and goal guarded moves with expected durations
// action defaults to Stop(0) at each cycle, else highest bid wins
// all persistent goals should be maintained OUTSIDE this class
// provides basic smooth motions of neck using polling

class jhcCeliaNeck
{
// PRIVATE MEMBER VARIABLES
private:
  // state
  double pvel;                      /** Current ramped position change speed. */
  double tvel;                      /** Current ramped tilt change speed.     */
  int beam;                         /** Positive if laser is currently on.    */
  int nok;                          /** Communications status.                */

  // actuator command
  double pwin;                      /** Winning pan goal from bidding.   */
  double twin;                      /** Winning tilt goal from bidding.  */
  double psp;                       /** Winning pan speed from bidding.  */
  double tsp;                       /** Winning tilt speed from bidding. */
  int plock;                        /** Winning bid for pan command.     */
  int tlock;                        /** Winning bid for tilt command.    */


// PUBLIC MEMBER VARIABLES
public:
  // components
  jhcDynamixel dyn;                 /** Dynamixel serial controller. */
  jhcJoint jt[2];                   /** Individual neck joints. */

  // default camera pose
  jhcParam pps;
  double cx0, cy0, cz0, p0, t0, r0;

  // parameters for motion control
  jhcParam nps;
  int dport, dbaud;
  double nvlim, nacc, nlead, ndead, dps0, ndone;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcCeliaNeck ();
  ~jhcCeliaNeck ();
  int CommOK () const {return nok;}
  int NeckReset (int noisy =0, int chk =1);
  int Check (int noisy =0, int tries =2);

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int LoadCal (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCal (const char *fname) const;

  // individual joint status
  double PanVel () const    {return jt[0].Speed();}
  double PanForce () const  {return jt[0].Force();}
  int PanStatus () const    {return jt[0].Flags();}
  double TiltVel () const   {return jt[1].Speed();}
  double TiltForce () const {return jt[1].Force();}
  int TiltStatus () const   {return jt[1].Flags();}
  jhcParam *PanServo ()     {return &(jt[0].sps);}
  jhcParam *PanGeom ()      {return &(jt[0].gps);}
  jhcParam *TiltServo ()    {return &(jt[1].sps);}
  jhcParam *TiltGeom ()     {return &(jt[1].gps);}

  // angle sensing interface
  int NeckUpdate ();
  double Pan () const; 
  double Tilt () const;
  double Pan0 () const  {return jt[0].Angle();} 
  double Tilt0 () const {return jt[1].Angle();} 
  int Flipped () const  {return((jt[1].Previous() < -90.0) ? 1 : 0);}
  int Angles (double& p, double& t) const;
  void Position (double& cx, double& cy, double& cz) const;
  void Position (jhcMatrix& pos) const;
  void PoseVecs (jhcMatrix& pos, jhcMatrix& dir) const; 
  void Pose6 (double* p6) const;

  // basic commands
  int Freeze ();
  int Limp ();
  int Stop (int bid =0);

  // goal specification
  double GazeTarget (double pan, double tilt, double dps =90.0, int bid =0);
  double PanTarget (double pan, double dps =90.0, int bid =0);
  double TiltTarget (double tilt, double dps =90.0, int bid =0);
  double GazeFix (double pan, double tilt, double secs =0.5, int bid =0);
  double PanFix (double pan, double secs =0.5, int bid =0);
  double TiltFix (double tilt, double secs =0.5, int bid =0);

  // profiled motion and progress
  int NeckIssue (double tupd =0.033, int imm =1);
  double PanErr (double pan) const {return fabs(norm_ang(Pan() - pan));}
  double TiltErr (double tilt) const {return fabs(norm_ang(Tilt() - tilt));}

  // initialization of pose
  void InitPose6 (double *p6) const 
    {p6[0] = p0; p6[1] = t0; p6[2] = r0; p6[3] = cx0; p6[4] = cy0; p6[5] = cz0;}
  int InitNeck () {return SetNeck(p0, t0);}

  // atomic and blocking commands
  int SlewNeck (double pan, double tilt, double dps =60.0);
  int SetNeck (double pan, double tilt);
  int SetPan (double pan) {return SetNeck(pan, Tilt());}
  int SetTilt (double tilt) {return SetNeck(Pan(), tilt);}
  int Nod (double tilt =15.0);
  int Shake (double pan =30.0);
  int Blink (int n =10);
  int Laser (int red =1);


// PRIVATE MEMBER FUNCTIONS
private:
  // configuration
  void std_geom ();

  // processing parameters
  int neck_params (const char *fname);
  int pose_params (const char *fname);

  // profiled motion
  double norm_ang (double degs) const;
  double alter_vel (double vel, double err, double cmd, double acc, double tupd) const;
  int servo_set (double p, double pv, double t, double tv, int send);
  double gaze_time (double pan, double tilt, double dps) const;
  double act_time (double err, double vmax, double acc) const;


};


#endif  // once




