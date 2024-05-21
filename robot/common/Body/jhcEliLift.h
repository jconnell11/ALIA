// jhcEliLift.h : control of Eli robot's motorized forklift stage
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "Data/jhcParam.h"         // common video
#include "Interface/jhcSerial.h"

#include "Geometry/jhcMotRamp.h"   // common robot

#include "Body/jhcGenLift.h"


//= Control of Eli robot's motorized forklift stage.
// built from dual-rack actuator driven by Polulu feedback controller
// commands are speed and goal guarded moves with expected durations
// action defaults to Stop(0) at each cycle, else highest bid wins
// all persistent goals should be maintained OUTSIDE this class

class jhcEliLift : public jhcMotRamp, public jhcGenLift
{
// PRIVATE MEMBER VARIABLES
private:
  jhcSerial lcom;               /** Serial port connection to controller. */
  int lok;                      /** Communications status.                */

  // sensor data
  int raw;                      /** Scaled feedback from controller. */
  double ht;                    /** Current height of fork stage.    */

  // speed estimate
  UL32 now;                     /** Time of last height reading.   */
  double ips;                   /** Estimated instantaneous speed. */

  // actuator command
  int llock0, llock;            /** Winning bid for fork height command.   */
  int stiff;                    /** Whether lift is under active control.  */


// PRIVATE MEMBER PARAMETERS
private:
  // controller parameters
  double ht0, ldone, quit;
  int lport, lbaud, ms;

  // geometric calibration
  double top, bot;
  int pmax, pmin;


// PUBLIC MEMBER VARIABLES
public:
  jhcParam fps, gps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization 
  ~jhcEliLift ();
  jhcEliLift ();
  int RawFB () const  {return raw;}
  int RawMax () const {return pmax;}
  int RawMin () const {return pmin;}
  double Default () const {return ht0;}
  double LiftTol () const {return ldone;}

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;

  // configuration
  int Reset (int rpt =0, int chk =1);
  int Check (int rpt =0, int tries =2);
  int CommOK () const {return lok;}
  void Inject (double ht0) {ht = ht0;}
  void AdjustRaw (double ht0, int v0, double ht1, int v1);
  void ResetRaw (int p1, int p0) {pmax = p1; pmin = p0;}

  // low level commands
  int Freeze (int doit =1, double tupd =0.033);
  int Limp ();

  // core interaction
  int Update ();
  int UpdateStart ();
  int UpdateFinish ();
  int Issue (double tupd =0.033, double lead =3.0);

  // --------------------- LIFT MAIN ----------------------------

  // current lift information
  double Height () const {return ht;}
  const double *LiftHt () const {return &ht;}
  double LiftIPS () const {return ips;}
  bool Moving (double sp =0.5) const {return(ips > sp);}

  // lift goal specification commands
  void LiftClear () {RampReset();}
  int LiftTarget (double high, double rate =1.0, int bid =10);
//  int LiftShift (double dz, double rate =1.0, int bid =10) 
//    {return LiftTarget(Height() + dz, rate, bid);}
  int LiftStop (double rate =1.5, int bid =1)
    {return LiftTarget(SoftStop(ht, ldone, rate), rate, bid);}

  // profiled motion progress
  double LiftErr (double high, int abs =1) const; 
  bool LiftClose (double tol =0.2) const {return(RampDist(ht) <= tol);} 

  // -------------------- LIFT EXTRAS ---------------------------

  // convert relative goal to absolute
  double LiftGoal (double dist) const 
    {return __max(bot, __min(ht + dist, top));}
  double RateIPS (double rate =1.0) const 
    {return(rate * vstd);}

  // lift goal characteristics
  double LiftTime (double high, double h0, double rate =1.0) const
    {return RampTime(high, h0, rate);}
  double LiftTime0 (double high, double rate =1.0) const
    {return LiftTime(high, Height(), rate);}

  // lift read only access
  double LiftCtrlVel () const  {return RampVel();}
  double LiftCtrlGoal () const {return RampCmd();}
  int LiftWin () const {return llock0;}

  // ------------------------ UTILS -----------------------------

  // atomic actions
  int SetLift (double ins);
  int IncLift (double ins) {return SetLift(Height() + ins);}


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int lift_params (const char *fname);
  int geom_params (const char *fname);

  // configuration
  int fail (int rpt);

  // basic interaction
  void clr_lock (int hist);

};

