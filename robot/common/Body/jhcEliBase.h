// jhcEliBase.h : control of Eli mobile robot base
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
// Copyright 2021-2024 Etaoin Systems
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

#include "Data/jhcParam.h"               // common video

#include "Geometry/jhcMotRamp.h"         // common robot
#include "Peripheral/jhcSerialFTDI.h" 

#include "Body/jhcGenBase.h"


/** If condition occurs increment error count and return code, otherwise clear error count. **/
#define BBARF(val, cond) if (cond) {berr++; return(val);} else berr = 0;


//= Control of Eli mobile robot base.
// built with Parallax Motor Mount kit driven by Roboclaw board (3.5 ft/sec)
// commands are speed and goal guarded moves with expected durations
// action defaults to stopped at each cycle, else highest bid wins
// all persistent goals should be maintained OUTSIDE this class
// basic commands set rotation and translation goal positions
// progress can be monitored by TurnErr and MoveErr functions (possibly absolute)
// automatically reads wheel encoders and decodes into a variety of forms
//   integral motions (head, trav) and since-last-step change (dr, dm)
//   computed Cartesian position (xpos, ypos) and since-last-step (dx, dy)
//   AdjustXY function moves some point (target) to compensate for base motion
// errors: generally try all sends but increment berr on failed receives

class jhcEliBase : public jhcGenBase
{
// PRIVATE MEMBER VARIABLES
private:
  jhcSerialFTDI bcom;    /** Serial port connection to controller. */
  char ver[40];          /** Description of controller board.      */
  int grn;               /** Whether controller is old green PCB.  */
  int c16;               /** Whether 16 bit CRC is returned.       */

  // packets
  UC8 pod[50];           /** Array for commands and values. */
  int berr;              /** Communications status.         */
  int crc;               /** Last communications checksum.  */
  int pend;              /** Strip last N acknowledgments.  */

  // raw state
  UL32 lf;               /** Current left encoder raw value.   */
  UL32 rt;               /** Current right encoder raw value.  */
  UL32 lf0;              /** Previous left encoder raw value.  */
  UL32 rt0;              /** Previous right encoder raw value. */

  // sensor data
  double trav;           /** Current integrated movement distance.      */
  double head;           /** Current integrated turn angle.             */
  double xpos;           /** Current integrated Cartesian X position.   */
  double ypos;           /** Current integrated Cartesian Y position.   */
  double dm;             /** Change of movement distance this cycle.    */
  double dr;             /** Change of turn angle this cycle.           */
  double dx0;            /** Displacement sideways to travel (avg).     */
  double dy0;            /** Displacement in direction of travel (avg). */
  double dx;             /** Change of Cartesian position X this cycle. */
  double dy;             /** Change of Cartesian position Y this cycle. */

  // actuator command
  int mlock0, mlock;     /** Winning bid for movement command.     */
  int tlock0, tlock;     /** Winning bid for turn command.         */
  int stiff;             /** Whether base is under active control. */
  int ice;               /** Whether moving is already frozen.     */
  int ice2;              /** Whether turning is already frozen.    */
  int ms;                /** Blocking update rate (ms).            */

  // led command
  int llock0, llock;     /** Winning bid for attention light.  */
  int led0, led;         /** Attention LED state and previous. */
  
  // instantaneous speed estimates
  UL32 now;
  double mvel, tvel, imv, itv;
  int parked;


// PRIVATE MEMBER PARAMETERS
private:
  // communication parameters
  double ploop, iloop, dloop, rpm;
  int bport, bbaud, pwm, ppr;

  // profiled motion parameters
  double mdead, tdead;

  // geometric calibration
  double wd, ws, vmax;


// PUBLIC MEMBER VARIABLES
public:
  int noisy;                 // debugging messages
  jhcParam cps, mps, gps;

  // trapezoidal profile generators
  jhcMotRamp mctrl, tctrl;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcEliBase ();
  jhcEliBase ();
  double MoveTol () const {return mdead;}
  double TurnTol () const {return tdead;}
  double MaxVolt () const {return vmax;}
  void ResetBat (double v =0.0) {vmax = v;}

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;

  // configuration
  int CommOK () const {return((berr > 0) ? 0 : 1);}
  int Reset (int rpt =0, int chk =1);
  int Check (int rpt =0, int tries =2);
  double Battery ();
  const char *Version ();
  void ForceLED (int on) {bcom.SetRTS(on);}

  // low level commands
  int Freeze (int doit =1, double tupd =0.033);
  int FreezeMove (int doit =1, double tupd =0.033);
  int FreezeTurn (int doit =1, double tupd =0.033);
  int Limp ();

  // core interaction
  int Update ();
  int UpdateStart ();
  int UpdateContinue ();
  int UpdateFinish ();
  int Issue (double tupd =0.033, double lead =3.0);

  // --------------------- BASE MAIN ----------------------------

  // current base information
  int Zero ();
  double Travel () const    {return trav;}  
  double WindUp () const    {return head;}
  double Heading () const   {return norm_ang(head);}
  double X () const         {return xpos;}  
  double Y () const         {return ypos;}  
  double StepFwd () const   {return dy0;}
  double StepSide () const  {return dx0;}
//  double StepLeft () const  {return -dx0;}
  double StepTurn () const  {return dr;}
  double StepX () const     {return dx;}
  double StepY () const     {return dy;}
  double StepMove () const  {return dm;}
  double MoveCmdV () const  {return mvel;}
  double TurnCmdV () const  {return tvel;}
  double MoveIPS (int abs =1) const {return((abs > 0) ? fabs(imv) : imv);}
  double TurnDPS (int abs =1) const {return((abs > 0) ? fabs(itv) : itv);}
  double TravelRate () const {return fabs(mctrl.rt);}
  int Static () const {return parked;}
  void AdjustXY (double& tx, double& ty, double tx0 =0.0, double ty0 =0.0) const;
  void AdjustTarget (jhcMatrix& pos) const;
  double AdjustAng (double& ang) const;

  // base goal specification
  void DriveClear () {mctrl.RampReset(); tctrl.RampReset();} 
  int DriveAbsolute (double tr, double hd, double m_rate =1.0, double t_rate =0.0, int bid =10);
  int MoveAbsolute (double tr, double rate =1.0, int bid =10, double skew =0.0); 
  int TurnAbsolute (double hd, double rate =1.0, int bid =10);
  int DriveTarget (double dist, double ang, double rate =1.0, int bid =10)
    {return DriveAbsolute(trav + dist, head + ang, rate, rate, bid);}
//  int MoveTarget (double dist, double rate =1.0, int bid =10)
//    {return MoveAbsolute(trav + dist, rate, bid);}
//  int TurnTarget (double ang, double rate =1.0, int bid =10)
//    {return TurnAbsolute(head + ang, rate, bid);}
  int DriveStop (double rate =1.5, int bid =1)
    {return DriveAbsolute(mctrl.SoftStop(trav, mdead, rate), tctrl.SoftStop(head, tdead, rate), rate, rate, bid);}
  int MoveStop (double rate =1.5, int bid =1)
    {return MoveAbsolute(mctrl.SoftStop(trav, mdead, rate), rate, bid);}
  int TurnStop (double rate =1.5, int bid =1)
    {return TurnAbsolute(tctrl.SoftStop(head, tdead, rate), rate, bid);}
  int SetMoveVel (double ips, int bid =10);
  int SetTurnVel (double dps, int bid =10);

  // profiled motion progress
  double MoveErr (double mgoal) const {return fabs(mgoal - trav);}
  double TurnErr (double tgoal) const {return fabs(tgoal - head);}
  bool DriveClose (double dist =0.5, double ang =2.0) const {return(MoveClose(dist) && TurnClose(ang));}
  bool MoveClose (double tol =0.5) const {return(mctrl.RampDist(trav) <= tol);}
  bool TurnClose (double tol =2.0) const {return(tctrl.RampDist(head) <= tol);}

  // -------------------- BASE EXTRAS ---------------------------

  // convert relative goal to absolute
//  double MoveGoal (double dist) const {return(trav + dist);}
//  double TurnGoal (double ang) const  {return(head + ang);}
  double RateIPS (double rate =1.0) const {return(rate * mctrl.vstd);}
  double RateDPS (double rate =1.0) const {return(rate * tctrl.vstd);}

  // base goal characteristics
  double DriveAbsTime (double tr2, double hd2, double tr1, double hd1, 
                       double m_rate =1.0, double t_rate =0.0) const; 
  double MoveAbsTime (double tr2, double tr1, double rate =1.0) const
    {return mctrl.RampTime(tr2, tr1, rate);}
  double TurnAbsTime (double hd2, double hd1, double rate =1.0) const
    {return tctrl.RampTime(hd2, hd1, rate);}

  // base timing from current state
  double DriveTime (double dist, double ang, double m_rate =1.0, double t_rate =0.0) const
    {return DriveAbsTime(trav + dist, head + ang, trav, head, m_rate, t_rate);} 
  double MoveTime (double dist, double rate =1.0) const
    {return MoveAbsTime(trav + dist, trav, rate);}
  double TurnTime (double ang, double rate =1.0) const
    {return TurnAbsTime(head + ang, head, rate);}
  int MoveMS (double dist, double rate =1.0) const
    {return ROUND(1000.0 * MoveTime(dist, rate));}
  int TurnMS (double ang, double rate =1.0) const
    {return ROUND(1000.0 * TurnTime(ang, rate));}

  // base motion coordination
  double DriveAbsRate (double tr2, double hd2, double tr1, double hd1, double secs =0.5, double rmax =1.5) const;
  double MoveAbsRate (double tr2, double tr1, double secs =0.5, double rmax =1.5) const
    {return mctrl.RampRate(tr2, tr1, secs, rmax);}
  double TurnAbsRate (double hd2, double hd1, double secs =0.5, double rmax =1.5) const
    {return tctrl.RampRate(hd2, hd1, secs, rmax);}

  // base rates from current state
  double DriveRate (double dist, double ang, double secs =0.5, double rmax =1.5) const
    {return DriveAbsRate(trav + dist, head + ang, trav, head, secs, rmax);}
  double MoveRate (double dist, double secs =0.5, double rmax =1.5) const
    {return MoveAbsRate(trav + dist, trav, secs, rmax);}
  double TurnRate (double ang, double secs =0.5, double rmax =1.5) const
    {return TurnAbsRate(head + ang, head, secs, rmax);}

  // eliminate residual error
  int MoveFix (double dist, double secs =0.5, double rmax =1.5, int bid =10)
    {return MoveTarget(dist, MoveRate(dist, secs, rmax), bid);}
  int TurnFix (double ang, double secs =0.5, double rmax =1.5, int bid =10)
    {return TurnTarget(ang, TurnRate(ang, secs, rmax), bid);}

  // base read only access  
  double MoveCtrlVel () const {return mctrl.RampVel(mdead);}
  double TurnCtrlVel () const {return tctrl.RampVel(tdead);}
  double MoveAbsGoal () const {return mctrl.RampCmd();}
  double TurnAbsGoal () const {return tctrl.RampCmd();}
  double MoveIncGoal () const {return(mctrl.RampCmd() - trav);}
  double TurnIncGoal () const {return norm_ang(tctrl.RampCmd() - head);}
  int DriveWin () const {return __max(mlock0, tlock0);}
  int MoveWin () const  {return mlock0;}
  int TurnWin () const  {return tlock0;}

  // nose light
  int AttnLED (int on, int bid =10);

  // ------------------------ UTILS -----------------------------

  // blocking functions
  int Drive (double dist, double degs);
  int Move (double dist) {return Drive(dist, 0.0);}
  int Turn (double degs) {return Drive(0.0, degs);}


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int ctrl_params (const char *fname);
  int move_params (const char *fname);
  int geom_params (const char *fname);

  // configuration
  int fail (int ans, int rpt);
  int loop_vals ();

  // packet validation
  void start_crc (const UC8 *data, int n) 
    {crc = 0; add_crc(data, n);}
  void add_crc (const UC8 *data, int n);
  int set_crc (UC8 *data, int n) const;
  bool fail_crc (const UC8 *data) const;
  bool fail_ack (int n);
  bool fail_pend ()
    {int n = pend; pend = 0; return fail_ack(n);}

  // core interaction
  void clr_locks (int hist);
  void cvt_cnts ();
  double norm_ang (double degs) const;
  int wheel_pwm (double ips, double dps);
  int wheel_vels (double ips, double dps);

};

