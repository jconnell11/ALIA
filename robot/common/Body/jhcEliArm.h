// jhcEliArm.h : kinematics and serial control for ELI robot arm
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
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

#ifndef _JHCELIARM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCELIARM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"              // common video

#include "Geometry/jhcJoint.h"          // common robot
#include "Geometry/jhcMatrix.h"
#include "Geometry/jhcMotRamp.h"
#include "Peripheral/jhcDynamixel.h"


//= Kinematics and serial control for ELI robot arm.
// Built using eight Robotis AX-12+ Dynamixel servos on serial network.
// Commands are speed and goal guarded moves with expected durations.
// Action defaults to Stop(0) at each cycle, else highest bid wins.
// All persistent goals should be maintained OUTSIDE this class.
// MUST call one of the target setting commands on each cycle.
// <pre>
//
// Gripper has 2 basic modes:
//
//   * Width Mode - Adjust the finger separation to a given value. Coupled 
//     with arm moves (if in Cartesian mode) so center of the gripper remains 
//     in the SAME LOCATION (i.e. looks like both fingers are moving) or 
//     so center of grip traverses specified path. 
//
//   * Force Mode - Adjusts finger width automatically to give some standard
//     position error in gripper servo (i.e. force). Coupled with arm moves 
//     (if in Cartesian mode) so hand remains in the same place (i.e. symmetric 
//     closing around gripped object). Selected when fwin >= 0.
//
// Arm has 2 basic modes:
//
//   * Joint Mode - Generates a trapezoidal rotational speed profile.  
//     Coordinated to achieve the final arm configuration with all joints
//     at the same time (i.e. all joint always moving, but some slowly).   
//     Selected when alock > plock and alock > dlock (both strictly greater).
//
//   * Cartesian Mode - Generates a linear path of fingertips with a 
//     trapezoidal XYZ speed profile. Coordinated to achieve the final end
//     effector position (X Y Z) and direction (aX aY AZ) at the same 
//     time instant (i.e. one is slowed down to allow the other to finish).
// 
// Orientations (degrees) have pan wrt Z axis, T wrt XY plane, R is along gripper.
// Positions (inches) have Y pointing forward, X to right, Z is upwards.
// NOTE: Coordinates relative to center of wheelbase and bottom of shelf.
//
// Moves used trapezoidal speed profiles on the controlled values.
// Takes mvstd (ips) and dvstd (dps) speeds, and macc and dacc accels.
// Speeds are scaled by "rate" and accels can optionally be scaled by "rate" squared.
// Generally rates are limited to some high (1.5x) and low (0.05x) values.
// Use a negative rate to linearly change completion time using identical trajectory.
// </pre>

class jhcEliArm 
{
// PRIVATE MEMBER VARIABLES
private:
  jhcDynamixel *dyn;                    /** Dynamixel serial controller. */
  jhcMatrix tool, tol, dtol;
  int aok;                              /** Communications status.       */

  // sensor data refreshed by Update
  jhcMatrix ang0;                       /** Extracted arm joint angles.  */ 
  jhcMatrix loc;                        /** Current gripper position.    */
  jhcMatrix aim;                        /** Current gripper orientation. */
  jhcMatrix fvec;                       /** Raw endpoint force vector.   */
  jhcMatrix fsm;                        /** Temporally smoothed force.   */
  double w00, w0;                       /** Gripper width on last cycle. */
  double sqz;                           /** Current gripper force.       */
  int first;                            /** No update since reset call.  */

  // hand current cycle actuator cmds
  double fwin;                          /** Winning hand force goal from bidding. */
  int wlock0, wlock;                    /** Winning bid for gripper command.      */

  // run-time calibration 
  double zint;                          /** Gravity correction target offset.   */
  double tmax;                          /** Estimated maximum servo torque.     */
  int gcal;                             /** Whether gripper cloed width tested. */
  int share;                            /** Whether lift load sharing tested.   */

  // arm current cycle actuator cmds
  int alock0, alock;                    /** Winning bid for configuration command. */
  int plock0, plock;                    /** Winning bid for position command.      */
  int dlock0, dlock;                    /** Winning bid for orientation command.   */

  // arm profiled move state
  int stiff;                            /** Whether the arm is under active control. */
  int ice;                              /** Whether arm is already in frozen mode.   */
  int ice2;                             /** Whether hand is already in frozen mode.  */


// PUBLIC MEMBER VARIABLES
public:
  // individual arm and hand joints
  jhcJoint jt[7];  

  // trapezoidal profile generators
  jhcMotRamp wctrl, pctrl, dctrl;

  // trajectory debugging information
  jhcMatrix miss, dmiss, stop, dstop;
  int size, iter;
  double q;

  // trajectory generation
  jhcParam tps;
  double zf, zlim;

  // inverse kinematics solver
  jhcParam ips;
  int tries, loops;
  double step, dstep, shrink, close, align;

  // arm and finger force interpretation
  jhcParam fps;
  double fmix, fmix2, fadj, fnone, fhold;

  // arm stowing position
  jhcParam sps;
  int ms, prt;
  double retx, rety, retz, rdir, rtip;

  // residual geometric calibration
  jhcParam gps;
  double ax0, ay0, az0, fc, fp, ft;
  

// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  jhcEliArm ();
  int CommOK (int bad =0) const {return aok;}

  // configuration
  void Bind (jhcDynamixel *ctrl);
  int Reset (int rpt =0, int chk =1);
  int Check (int rpt =0, int tries =2);
  double Voltage ();
  int Power (double vbat =0.0);

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;

  // kinematic controls
  void FingerTool (double dpad =1.0);
  void SetTool (double dx, double dy, double dz) 
    {tool.SetVec3(dx, dy, dz);}
  void StdTols ();
  void PosTols (double dx, double dy, double dz)         
    {tol.SetVec3(dx, dy, dz, 0.0);}
  void DirTols (double dpan, double dtilt, double droll) 
    {dtol.SetVec3(dpan, dtilt, droll, 0.0);}
  void OldCoords (jhcMatrix& table, const jhcMatrix& wheel) const
    {table.RelVec3(wheel, -ax0, -ay0, 1.8);}
  void NewCoords (jhcMatrix& wheel, const jhcMatrix& table) const
    {wheel.RelVec3(table, ax0, ay0, -1.8);}

  // basic commands
  int Freeze (double tupd =0.033);
  int FreezeArm (int doit =1, double tupd =0.0);
  int FreezeHand (int doit =1, double tupd =0.0);
  int Limp ();

  // core interaction
  int Update (int mega =1);
  int Issue (double tupd =0.033, double lead =3.0, int send =1);

  // ---------------------- HAND MAIN ----------------------------

  // current hand information
  double Width () const {return w0;}
  double Squeeze () const {return sqz;}
  bool WidthStop (double wch =0.1) const {return(fabs(w00 - w0) < wch);}
  bool SqueezeSome (double f =8.0) const {return(sqz >= f);}
  bool HoldMode () const {return(fwin > 0.0);}
    
  // hand goal specification commands
  void HandClear () {wctrl.RampReset();}
  int WidthTarget (double sep, double rate =1.0, int bid =10);
  int SqueezeTarget (double force =16.0, int bid =10);

  // hand motion progress
  double WidthErr (double sep, int abs =1) const;
  double SqueezeErr (double f, int abs =1) const;
  bool WidthClose (double wtol =0.1) const {return(wctrl.RampDist(w0) <= wtol);}
  bool SqueezeClose (double ftol =2.0) const {return(fabs(sqz - fwin) <= ftol);}
  bool HandClose (double wid, double wtol =0.1, double ftol =2.0) const
   {return(((wid >= 0.0) && !WidthClose(wtol)) || ((wid < 0.0) && !SqueezeClose(ftol)));}
  bool WidthFail (double secs =0.5) const {return(wctrl.RampDone() > secs);}
  bool SqueezeFail (double secs =0.5) const {return WidthFail(secs);}
  bool HandFail (double wid, double secs =0.5) const
    {return(((wid >= 0.0) && WidthFail(secs)) || ((wid < 0.0) && SqueezeFail(secs)));}
          

  // --------------------- HAND EXTRAS ---------------------------

  // hand goal characteristics
  double MaxWidth () const;
  int Graspable (double wid) const;
  double WidthTime (double w2, double w1, double rate =1.0) const
    {return wctrl.RampTime(w2, w1, rate);}
  double SqueezeTime (double w, double rate =1.0) const
    {return wctrl.RampTime(-0.5, w, rate);}

  // hand timing from current state
  double WidthTime0 (double w, double rate =1.0) const
    {return WidthTime(w, Width(), rate);}
  double SqueezeTime0 (double rate =1.0) const 
    {return SqueezeTime(Width(), rate);}

  // hand motion coordination 
  double WidthRate (double w2, double w1, double secs =0.5) const
    {return wctrl.RampRate(w2, w1, secs);}

  // hand rates from current state
  double WidthRate0 (double w, double secs =0.5) const
    {return WidthRate(w, Width(), secs);}

  // hand read only access
  double SqueezeGoal () const {return fwin;}
  double WidthGoal () const   {return wctrl.RampCmd();}
  double WidthSpeed () const  {return wctrl.RampVel();}
  int HandWin () const {return wlock0;}

  // ---------------------- ARM MAIN ----------------------------

  // current arm information
  void ArmConfig (jhcMatrix& ang) const;
  const jhcMatrix *ArmConfig () const {return &ang0;}
  void ArmPose (jhcMatrix& pos, jhcMatrix& dir) const; 
  void Position (jhcMatrix& pos) const; 
  void Direction (jhcMatrix& dir) const;
  const jhcMatrix *Position () const  {return &loc;} 
  const jhcMatrix *Direction () const {return &aim;} 
  int ForceVect (jhcMatrix &fdir, double z0 =0.0, int raw =0) const;
  double Force (const jhcMatrix &dir, int raw =0) const;

  // arm goal specification commands
  void CfgClear ();
  void ArmClear () {pctrl.RampReset(); dctrl.RampReset();}
  int CfgTarget (const jhcMatrix& ang, double rate =1.0, int bid =10);
  int CfgTarget (const jhcMatrix& ang, const jhcMatrix& rates, int bid =10);
  int ArmTarget (const jhcMatrix& pos, const jhcMatrix& dir, double p_rate =1.0, double d_rate =0.0, int bid =10);
  int PosTarget (const jhcMatrix& pos, double rate =1.0, int bid =10);
  int DirTarget (const jhcMatrix& dir, double rate =1.0, int bid =10);
  int ShiftTarget (const jhcMatrix& dpos, double rate =1.0, int bid =10);

  // arm motion progress
  void CfgErr (jhcMatrix& aerr, const jhcMatrix& ang, int abs =1) const;
  void ArmErr (jhcMatrix& perr, jhcMatrix& derr, 
               const jhcMatrix& pos, const jhcMatrix& dir, int abs =1) const;
  void PosErr (jhcMatrix& perr, const jhcMatrix& pos, int abs =1) const;
  void DirErr (jhcMatrix& derr, const jhcMatrix& dir, int abs =1) const;
  double CfgOffset (const jhcMatrix& ang) const;
  double PosOffset (const jhcMatrix& pos) const {return loc.PosDiff3(pos);}
  double DirOffset (const jhcMatrix& dir) const {return aim.RotDiff3(dir);}
  double ErrZ (const jhcMatrix& pos) const {return fabs(pos.Z() - loc.Z());}

  bool CfgClose (double tol =2.0) const;
  bool ArmClose (double xyz =0.25, double atol =5.0) const {return(PosClose(xyz) && DirClose(atol));}
  bool PosClose (double tol =0.25) const {return(pctrl.RampDist(loc) <= tol);}
  bool DirClose (double tol =15.0) const  {return(dctrl.RampDist(aim) <= tol);}
  bool CfgFail (double secs =0.5) const;
  bool ArmFail (double secs =0.5) const {return(PosFail(secs) && DirFail(secs));}
  bool PosFail (double secs =0.5) const {return(pctrl.RampDone() > secs);}
  bool DirFail (double secs =0.5) const {return(dctrl.RampDone() > secs);}
    
  // --------------------- ARM EXTRAS ---------------------------

  // arm goal characteristics
  double Feasible (const jhcMatrix& ang) const;
  double Reachable (const jhcMatrix& pos, const jhcMatrix& dir, double qlim =30.0, int from =1);
  double CfgTime (const jhcMatrix& ang2, const jhcMatrix& ang1, double rate =1.0) const;
  double CfgTime (const jhcMatrix& ang2, const jhcMatrix& ang1, const jhcMatrix& rates) const;
  double PosTime (const jhcMatrix& pos2, const jhcMatrix& pos1, double rate =1.0) const
    {return pctrl.RampTime(pos2, pos1, rate);}
  double DirTime (const jhcMatrix& dir2, const jhcMatrix& dir1, double rate =1.0) const
    {return dctrl.RampTime(dir2, dir1, rate);}
  double ArmTime (const jhcMatrix& pos2, const jhcMatrix& dir2, const jhcMatrix& pos1, const jhcMatrix& dir1, 
                  double p_rate =1.0, double d_rate =0.0) const;
  double ShiftTime (const jhcMatrix& dpos, const jhcMatrix& pos0, double rate =1.0) const;

  // arm timing from current state
  double CfgTime0 (const jhcMatrix& ang, double rate =1.0) const
    {return CfgTime(ang, ang0, rate);} 
  double CfgTime0 (const jhcMatrix& ang, const jhcMatrix& rates) const
    {return CfgTime(ang, ang0, rates);}
  double PosTime0 (const jhcMatrix& pos, double rate =1.0) const
    {return PosTime(pos, loc, rate);}
  double DirTime0 (const jhcMatrix& dir, double rate =1.0) const
    {return DirTime(dir, aim, rate);}
  double ArmTime0 (const jhcMatrix& pos, const jhcMatrix& dir, double p_rate =1.0, double d_rate =0.0) const
    {return ArmTime(pos, dir, loc, aim, p_rate, d_rate);}
  double ShiftTime0 (const jhcMatrix& dpos, double rate =1.0) const
    {return ShiftTime(dpos, loc, rate);}

  // arm motion coordination 
  double CfgRate (const jhcMatrix& ang2, const jhcMatrix& ang1, double secs =0.5) const;
  void CfgRate (jhcMatrix& rates, const jhcMatrix& ang2, const jhcMatrix& ang1, double secs =0.5) const;
  double ArmRate (const jhcMatrix& pos2, const jhcMatrix& dir2, 
                  const jhcMatrix& pos1, const jhcMatrix& dir1, double secs =0.5) const;
  double PosRate (const jhcMatrix& pos2, const jhcMatrix& pos1, double secs =0.5) const
    {return pctrl.RampRate(pos2, pos1, secs);}
  double DirRate (const jhcMatrix& dir2, const jhcMatrix& dir1, double secs =0.5) const
    {return dctrl.RampRate(dir2, dir1, secs);}

  // arm rates from current state
  double CfgRate0 (const jhcMatrix& ang, double secs =0.5) const
    {return CfgRate(ang, ang0, secs);}
  void CfgRate0 (jhcMatrix& rates, const jhcMatrix& ang, double secs =0.5) const
    {return CfgRate(rates, ang, ang0, secs);}
  double ArmRate0 (const jhcMatrix& pos, const jhcMatrix& dir, double secs =0.5) const
    {return ArmRate(pos, dir, loc, aim, secs);}
  double PosRate0 (const jhcMatrix& pos, double secs =0.5) const
    {return PosRate(pos, loc, secs);}
  double DirRate0 (const jhcMatrix& dir, double secs =0.5) const
    {return DirRate(dir, aim, secs);}

  // arm read only access
  void ArmGoal (jhcMatrix& tpos, jhcMatrix& tdir) const 
   {tpos.Copy(pctrl.cmd); tdir.Copy(dctrl.cmd);}
  double PosSpeed () const {return pctrl.rt;}
  double DirSpeed () const {return dctrl.rt;}
  int CfgWin () const {return alock0;}
  int ArmWin () const {return __max(plock0, dlock0);}
  int PosWin () const {return plock0;}
  int DirWin () const {return dlock0;}

  // individual joint status
  const char *JtName (int n) const {return jt[__max(0, __min(n, 6))].name;}
  double JtAng (int n) const    {return jt[__max(0, __min(n, 6))].Angle();}
  double JtSpeed (int n) const  {return jt[__max(0, __min(n, 6))].Speed();}
  double JtTorque (int n) const {return jt[__max(0, __min(n, 6))].Torque();}
  int JtErr (int n) const       {return jt[__max(0, __min(n, 6))].Flags();}
  char JtChar (int n) const  {return *(jt[__max(0, __min(n, 6))].name);}
  jhcParam *JtServo (int n)  {return &(jt[__max(0, __min(n, 6))].sps);}
  jhcParam *JtGeom (int n)   {return &(jt[__max(0, __min(n, 6))].gps);}
  int JtPos (jhcMatrix& loc, int n) const;

  // trajectory generator state
  double CtrlVel (int n) const  {return jt[__max(0, __min(n, 6))].RampVel();}
  double CtrlGoal (int n) const {return jt[__max(0, __min(n, 6))].RampCmd();}

  // ------------------------ UTILS ------------------------------

  // calibration
  int ZeroGrip (int always =0);
  int ShareLift (int always =0);

  // blocking routines
  int Grab (double fhold =16.0);
  int Drop ();
  int SetConfig (const jhcMatrix& ang, double rate =1.0);
  int Stow (int hold =0);
  int Reach (const jhcMatrix& pos, const jhcMatrix& dir, double wid =3.0,
             double qlim =10.0, double inxy =0.5, double inz =1.0, double degs =5.0);  

  // debugging tools
  void JointLoop (int n =2, int once =0);  
  void FingerLoop ();


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and configuration
  int fail (int rpt);
  void std_geom ();

  // processing parameter manipulation
  int traj_params (const char *fname);
  int force_params (const char *fname);
  int iter_params (const char *fname);
  int stow_params (const char *fname);
  int geom_params (const char *fname);

  // basic interaction
  void find_force (jhcMatrix &dir) const;
  void clr_locks (int hist);
  void config_move (double tupd, double lead);
  void simul_move (const jhcMatrix& ang1, const jhcMatrix& ang0, double secs); 

  // forward kinematics
  void get_angles (jhcMatrix& ang) const;
  double deg2w (double degs) const;
  double get_pose (jhcMatrix& pos, jhcMatrix& dir, 
                   const jhcMatrix& ang, int finger);

  // inverse kinematics
  double w2deg (double w) const;
  double v2dps (double v, double w) const;
  double pick_angles (jhcMatrix& ang, const jhcMatrix& end, const jhcMatrix& aim, 
                      double sep, const jhcMatrix *ang0, int finger);
  void j_trans (jhcMatrix& jact, jhcMatrix& djact, const jhcMatrix& pos) const;
  double pos_diff (jhcMatrix& fix, jhcMatrix& err, 
                   const jhcMatrix& pos, const jhcMatrix& end) const;
  double dir_diff (jhcMatrix& dfix, jhcMatrix& derr, 
                   const jhcMatrix& dir, const jhcMatrix& aim) const; 
  void jt3x3 (jhcMatrix& f2t) const;

  // arm goal specification
  void sync_xyz ();

};


#endif  // once




