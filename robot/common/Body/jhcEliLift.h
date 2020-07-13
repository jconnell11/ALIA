// jhcEliLift.h : control of Eli robot's motorized forklift stage
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

#ifndef _JHCELILIFT_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCELILIFT_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"         // common video
#include "Interface/jhcSerial.h"

#include "Geometry/jhcMotRamp.h"   // common robot


//= Control of Eli robot's motorized forklift stage.
// built from dual-rack actuator driven by Polulu feedback controller
// commands are speed and goal guarded moves with expected durations
// action defaults to Stop(0) at each cycle, else highest bid wins
// all persistent goals should be maintained OUTSIDE this class

class jhcEliLift : public jhcMotRamp
{
// PRIVATE MEMBER VARIABLES
private:
  jhcSerial lcom;               /** Serial port connection to controller. */
  int lok;                      /** Communications status.                */

  // sensor data
  double ht;                    /** Current height of fork stage. */

  // actuator command
  int llock0, llock;            /** Winning bid for fork height command.   */
  int stiff;                    /** Whether lift is under active control.  */


// PUBLIC MEMBER VARIABLES
public:
  // controller parameters
  jhcParam fps;
  int lport, lbaud, ms;
  double ht0, ldone, quit;

  // geometric calibration
  jhcParam gps;
  double top, bot;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization 
  ~jhcEliLift ();
  jhcEliLift ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;

  // configuration
  int Reset (int rpt =0, int chk =1);
  int Check (int rpt =0, int tries =2);
  int CommOK (int bad =0) const {return lok;}

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

  // lift goal specification commands
  void LiftClear () {RampReset();}
  int LiftTarget (double high, double rate =1.0, int bid =10);
  int LiftShift (double dz, double rate =1.0, int bid =10) 
    {return LiftTarget(Height() + dz, rate, bid);}

  // profiled motion progress
  double LiftErr (double high, int abs =1, int lim =1) const; 
  bool LiftClose (double tol =0.2) const {return(RampDist(ht) <= tol);} 
  bool LiftFail (double secs =0.5) const {return(RampDone() > secs);}

  // -------------------- LIFT EXTRAS ---------------------------

  // convert relative goal to absolute
  double LiftGoal (double dist) const 
    {return __max(bot, __min(ht + dist, top));}
  double LiftIPS (double rate =1.0) const 
    {return(rate * vstd);}

  // lift goal characteristics
  double LiftTime (double high, double h0, double rate =1.0) const
    {return RampTime(high, h0, rate);}
  double LiftTime0 (double high, double rate =1.0) const
    {return LiftTime(high, Height(), rate);}

  // lift read only access
  double LiftCtrlVel () const {return RampVel();}
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


#endif  // once




