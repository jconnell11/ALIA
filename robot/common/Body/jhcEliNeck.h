// jhcEliNeck.h : control of Eli robot's head pan and tilt actuators
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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

#ifndef _JHCELINECK_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCELINECK_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"             // common video

#include "Geometry/jhcJoint.h"         // common robot
#include "Peripheral/jhcDynamixel.h"


//= Control of Eli robot's head pan and tilt actuators.
// built using two Robotis AX-12+ Dynamixel servos on serial network
// control mode is a linear, trapezoidal profiled slew to some gaze pose
// commands are speed and goal guarded moves with expected durations
// action defaults to Stop(0) at each cycle, else highest bid wins
// all persistent goals should be maintained OUTSIDE this class

class jhcEliNeck
{
// PRIVATE MEMBER VARIABLES
private:
  jhcDynamixel *dyn;                /** Dynamixel serial controller. */
  int nok;                          /** Communications status.       */

  // sensor data
  jhcMatrix pos0;                   /** World position of camera. */ 
  jhcMatrix dir;                    /** Gaze angle of camera.     */

  // actuator command
  int plock0, plock;                /** Winning bid for pan command.          */
  int tlock0, tlock;                /** Winning bid for tilt command.         */
  int stiff;                        /** Whether neck is under active control. */

  // angular speed estimate
  UL32 now;
  double p0, t0, ipv, itv;
  int stable;

  // control loop performance
  double pvel, tvel;


// PRIVATE MEMBER PARAMETERS
private:
  // parameters for motion control
  double gaze0, ndone, quit;
  int ms;

  // residual geometric calibration
  double nx0, ny0, nz0, cfwd, roll;


// PUBLIC MEMBER VARIABLES
public:
  jhcParam rps, nps, gps;

  // individual neck joints
  jhcJoint jt[2];           


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcEliNeck ();
  jhcEliNeck ();
  double Default () const  {return gaze0;}
  void SetDef (double t)   {gaze0 = t;}
  void IncZ (double dz)    {nz0 += dz;}
  void IncRoll (double dr) {roll += dr;}

  // configuration
  void Bind (jhcDynamixel& ctrl);
  int CommOK (int bad =0) const {return nok;}
  int Reset (int rpt =0, int chk =1);
  int Check (int rpt =0, int tries =2);
  double Voltage ();
  void Inject (double pan, double tilt);

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;

  // low level commands
  int Freeze (int doit =1, double tupd =0.033);
  int Limp ();

  // core interaction
  int Update ();
  int Issue (double tupd =0.033, double lead =3.0, int send =1);

  // --------------------- NECK MAIN ----------------------------

  // current neck information
  void HeadPose (jhcMatrix& pos, jhcMatrix& aim, double lift) const; 
  void HeadLoc (jhcMatrix& pos, double lift) const;
  double HeadZ (double lift) const {return(pos0.Z() + lift);}
  double Pan () const  {return jt[0].Angle();}
  double Tilt () const {return jt[1].Angle();}
  void Gaze (double& p, double& t) const {p = Pan(); t = Tilt();}
  void AimFor (double& p, double& t, const jhcMatrix& targ, double lift) const;
  double PanCmdV () const  {return pvel;}
  double TiltCmdV () const {return tvel;}
  double PanDPS (int abs =0) const   {return((abs > 0) ? fabs(ipv) : ipv);}
  double TiltDPS (int abs =0) const  {return((abs > 0) ? fabs(itv) : itv);}
  double PanStep (int abs =0) const  {return((abs > 0) ? fabs(Pan() - p0)  : Pan() - p0);}
  double TiltStep (int abs =0) const {return((abs > 0) ? fabs(Tilt() - t0) : Tilt() - t0);}
  bool Saccade (double plim =3.5, double tlim =1.0) const 
    {return((PanStep(1) > plim) || (TiltStep(1) > tlim));}
  int Stare () const {return stable;}

  // goal specifying commands for view
  void GazeClear () {jt[0].RampReset(); jt[1].RampReset();}
  int GazeTarget (double pan, double tilt, double p_rate =1.0, double t_rate =0.0, int bid =10);
  int PanTarget (double pan, double rate =1.0, int bid =10);
  int TiltTarget (double tilt, double rate =1.0, int bid =10);
  int ShiftTarget (double dp, double dt, double rate =1.0, int bid =10)
    {return GazeTarget(Pan() + dp, Tilt() + dt, rate, rate, bid);}
  int GazeAt (const jhcMatrix& targ, double lift, double rate =1.0, int bid =10);
  int GazeAt (const jhcMatrix *targ, double lift, double rate =1.0, int bid =10)
    {return((targ == NULL) ? 0 : GazeAt(*targ, lift, rate, bid));}
  int GazeFix (const jhcMatrix& targ, double lift, double secs =0.5, int bid =10);
  int GazeStop (double rate =1.5, int bid =1)
    {return GazeTarget(jt[0].SoftStop(Pan(), ndone, rate), jt[1].SoftStop(Tilt(), ndone, rate), rate, bid);}

  // progress irrespective of controlling behavior
  double PanErr (double pan, int abs =1, int lim =1) const;
  double TiltErr (double tilt, int abs =1, int lim =1) const;
  double GazeErr (double pan, double tilt, int lim =1) const
    {return __max(PanErr(pan, 1, lim), TiltErr(tilt, 1, lim));}
  double GazeErr (const jhcMatrix& targ, double lift) const;
  bool PanDone (double p, double tol =3.0) const  
    {return(PanErr(p) <= tol);}
  bool TiltDone (double t, double tol =3.0) const 
    {return(TiltErr(t) <= tol);}
  bool GazeDone (double p, double t, double tol =3.0) const 
    {return(PanDone(p, tol) && TiltDone(t, tol));}
  bool GazeDone (const jhcMatrix& targ, double lift, double tol =3.0) const 
    {return(GazeErr(targ, lift) <= tol);}

  // profiled motion progress
  bool GazeClose (double tol =1.0) const {return(PanClose(tol) && TiltClose(tol));}
  bool PanClose (double tol =1.0) const  {return(jt[0].RampDist(Pan()) <= tol);}
  bool TiltClose (double tol =1.0) const {return(jt[1].RampDist(Tilt()) <= tol);}

  // -------------------- NECK EXTRAS ---------------------------

  // neck goal characteristics
  double GazeTime (double p2, double t2, double p1, double t1, 
                   double p_rate =1.0, double t_rate =0.0) const;
  double PanTime (double p2, double p1, double rate =1.0) const
    {return jt[0].RampTime(jt[0].CycNorm(p2), jt[0].CycNorm(p1), rate);}
  double TiltTime (double t2, double t1, double rate =1.0) const
    {return jt[1].RampTime(jt[1].CycNorm(t2), jt[1].CycNorm(t1), rate);}
  double ShiftTime (double dp, double dt, double p, double t, double rate =1.0) const
    {return GazeTime(p + dp, t + dt, p, t, rate, rate);}

  // neck timing from current state
  double GazeTime0 (double p, double t, double p_rate =1.0, double t_rate =0.0) const
    {return GazeTime(p, t, Pan(), Tilt(), p_rate, t_rate);}
  double PanTime0 (double p, double rate =1.0) const
    {return PanTime(p, Pan(), rate);}
  double TiltTime0 (double t, double rate =1.0) const
    {return TiltTime(t, Tilt(), rate);}
  double ShiftTime0 (double dp, double dt, double rate =1.0) const
    {return ShiftTime(dp, dt, Pan(), Tilt(), rate);}

  // neck motion coordination 
  double GazeRate (double p2, double t2, double p1, double t1, double secs =0.5, double rmax =1.5) const;
  double PanRate (double p2, double p1, double secs =0.5, double rmax =1.5) const
    {return jt[0].RampRate(jt[0].CycNorm(p2), jt[0].CycNorm(p1), secs, rmax);}
  double TiltRate (double t2, double t1, double secs =0.5, double rmax =1.5) const
    {return jt[1].RampRate(jt[1].CycNorm(t2), jt[1].CycNorm(t1), secs, rmax);}
  double ShiftRate (double dp, double dt, double p, double t, double secs =0.5, double rmax =1.5) const
    {return GazeRate(p + dp, t + dt, p, t, secs, rmax);}

  // neck rates from current state
  double GazeRate0 (double p, double t, double secs =0.5, double rmax =1.0) const
    {return GazeRate(p, t, Pan(), Tilt(), secs, rmax);}
  double PanRate0 (double p, double secs =0.5, double rmax =1.0) const
    {return PanRate(p, Pan(), secs, rmax);}
  double TiltRate0 (double t, double secs =0.5, double rmax =1.0) const
    {return TiltRate(t, Tilt(), secs, rmax);}
  double ShiftRate0 (double dp, double dt, double secs =0.5, double rmax =1.0) const
    {return ShiftRate(dp, dt, Pan(), Tilt(), secs, rmax);}

  // eliminate residual error
  int GazeFix (double pan, double tilt, double secs =0.5, int bid =10)
    {return GazeTarget(pan, tilt, PanRate0(pan, secs), TiltRate0(tilt, secs), bid);}
  int PanFix (double pan, double secs =0.5, int bid =10)
    {return PanTarget(pan, PanRate0(pan, secs), bid);}
  int TiltFix (double tilt, double secs =0.5, int bid =10)
    {return TiltTarget(tilt, TiltRate0(tilt, secs), bid);}

  // arm read only access
  double PanCtrlVel () const   {return jt[0].RampVel();}
  double TiltCtrlVel () const  {return jt[1].RampVel();}
  double PanCtrlGoal () const  {return jt[0].RampCmd();}
  double TiltCtrlGoal () const {return jt[1].RampCmd();}
  int GazeWin () const {return __max(plock0, tlock0);}
  int PanWin () const  {return plock0;}
  int TiltWin () const {return tlock0;}

  // ------------------------ UTILS -----------------------------

  // individual joint status
  double PanSpeed () const   {return jt[0].Speed();}
  double PanTorque () const  {return jt[0].Torque();}
  int PanStatus () const     {return jt[0].Flags();}
  double TiltSpeed () const  {return jt[1].Speed();}
  double TiltTorque () const {return jt[1].Torque();}
  int TiltStatus () const    {return jt[1].Flags();}
  jhcParam *PanServo ()      {return &(jt[0].sps);}
  jhcParam *PanGeom ()       {return &(jt[0].gps);}
  jhcParam *TiltServo ()     {return &(jt[1].sps);}
  jhcParam *TiltGeom ()      {return &(jt[1].gps);}

  // atomic and blocking commands
  int SlewNeck (double pan, double tilt, double dps =60.0);
  int SetNeck (double pan, double tilt);
  int SetPan (double pan)   {return SetNeck(pan, Tilt());}
  int SetTilt (double tilt) {return SetNeck(Pan(), tilt);}
  int Nod (double tilt =15.0);
  int Shake (double pan =30.0);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int ramp_params (const char *fname);
  int neck_params (const char *fname);
  int geom_params (const char *fname);

  // configuration
  int fail (int rpt);
  void std_geom ();

  // core interaction
  void current_pose (jhcMatrix& xyz, jhcMatrix& ptr);
  void clr_locks (int hist);

  // profiled motion
  void servo_set (double p, double pv, double t, double tv);

  // motion progress
  double norm_ang (double degs) const;

};


#endif  // once




