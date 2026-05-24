// jhcSwapArm.cpp : control interface for external robot arm
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2025 Etaoin Systems
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

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"          

#include "Body/jhcSwapArm.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// needed as a placeholder for derived class virtual destructor

jhcSwapArm::~jhcSwapArm ()
{
}


//= Default constructor initializes certain values.

jhcSwapArm::jhcSwapArm ()
{
  // target position and direction command
  pdes.SetSize(4);
  ddes.SetSize(4);
  pdes0.SetSize(4);
  ddes0.SetSize(4);

  // passive gripper opening and status
  aok = 1;
  Defaults();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used to define stowed arm postion.
// nothing geometric that differs between bodies

int jhcSwapArm::stow_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("arm_stow", 0);
  ps->NextSpecF( &retx,  -2.0, "Tucked x postion (in)");   
  ps->NextSpecF( &rety,  10.5, "Tucked y position (in)");  
  ps->NextSpecF( &retz,  -2.0, "Tucked z position (in)");        
  ps->NextSpecF( &rdir, 180.0, "Tucked point direction (deg)");
  ps->NextSpecF( &rtip, -15.0, "Tucked tip direction (deg)");    
  ps->Skip();

  ps->NextSpecF( &wmax,   3.0, "Max grip width (in)");
  ps->NextSpecF( &wlax,   1.0, "Relaxed width (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Configuration                            //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcSwapArm::Reset () 
{
  // arm speed
  now = jms_now(); 
  iarm = 0.0; 
  parked = 0; 

  // arm and hand status 
  Status((float) retx, (float) rety, (float) retz, (float) rdir, (float) rtip, 0.0, 
         (float) wlax, 0.0, 0.0);
  Update();

  // arm and hand commands
  def_cmd();   
  Issue();
}


//= Reset locks and specify default commands.

void jhcSwapArm::def_cmd ()
{
  // position command
  prate = 0.0;
  plock = 0;

  // direction command
  drate = 0.0;
  dlock = 0;

  // hand command
  wrate = 0.0;
  wlock = 0;

  // tuck command
  trate = 0.0;
  tlock = 0;
}


///////////////////////////////////////////////////////////////////////////
//                             Data Exchange                             //
///////////////////////////////////////////////////////////////////////////

//= Cache new arm status from robot sensors (call Update to transfer).
// x, y, z is hand position while p, t, r is hand orientation
// w is gripper opening, f is applied force, e is offset from tuck pose

void jhcSwapArm::Status (float x, float y, float z, float p, float t, float r, 
                         float w, float f, float e)
{
  // save Cartesion position & orientation
  loc0.SetVec3(x, y, z);
  aim0.SetVec3(p, t, r);

  // save gripper width and force
  wid0 = w;
  sqz0 = f;

  // save configuration angular offset
  terr0 = e;
}


//= Report gripper position command for robot actuators (use Issue to refresh).
// x, y, z is hand goal position, vel is translation speed wrt nominal
// mode bits: 2 = exact Z, 1 = exact Y, 0 = exact X
// bid is the importance of the position command 

void jhcSwapArm::PosCmd (float& x, float& y, float& z, float& vel, int& mode, int& bid)
{
  // copy position goal command 
  x = (float) pdes0.X();
  y = (float) pdes0.Y();
  z = (float) pdes0.Z();

  // copy speed and coordinate priority
  vel = (float) prate0;
  mode = pmode0;

  // copy command importance
  bid = plock0;
}
                

//= Report gripper orientation command for robot actuators (use Issue to refresh).
// p, t, r is hand goal orientation, vel is rotation speed wrt nominal
// mode bits: 3 = any pan, 2 = exact roll, 1 = exact tilt, 0 = exact pan
// bid is the importance of the orientation command 

void jhcSwapArm::DirCmd (float& pan, float& tilt, float& roll, float& vel, int& mode, int& bid)
{
  // copy orientation goal command 
  pan  = (float) ddes0.P();
  tilt = (float) ddes0.T();
  roll = (float) ddes0.R();

  // copy speed and aspect priority
  vel = (float) drate0;
  mode = dmode0;

  // copy command importance
  bid = dlock0;
}
                

//= Report gripper width and arm tuck command for robot actuators (use Issue to refresh).
// wf is desired finger separation if positive, gripping force if negative
// wvel is the speed to open/close the gripper wrt nominal
// svel is the speed to assumed a tucked configuration (0 = not invoked) 
// wbid, sbid are the importance of the gripper and tuck commands 

void jhcSwapArm::AuxCmd (float& wf, float& wvel, float& svel, int& wbid, int& sbid)
{
  // copy finger separation (or force) command and speed
  wf   = (float) wstop0;
  wvel = (float) wrate0;

  // copy arm tuck command speed (0 = don't bother)
  svel = (float) trate0;

  // copy command importances
  wbid = wlock0;
  sbid = tlock0; 
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Update joint angles of the arm and hand (load cache values with Status).
// retrieves "data" from "data0", automatically resets "lock" for new bids

void jhcSwapArm::Update ()
{
  jhcMatrix orig(4);
  double a, s, mix = 1.0, twang = 1.0;     // twang was 3 for big robot
  UL32 last = now;

  // save Cartesion position & orientation
  orig.Copy(loc);
  loc.Copy(loc0);
  aim.Copy(aim0);

  // save gripper width and force
  wid = wid0;
  sqz = sqz0;

  // save configuration angular offset
  terr = terr0;

  // instantaneous speed estimates (check angular rate also?)
  now = jms_now();
  if (last != 0)
    if ((s = jms_secs(now, last)) > 0.0)
    {
      a = orig.PosDiff3(loc) / s;
      iarm  += mix * (a - iarm); 
    }
  if (iarm >= twang)
    parked = __min(0, parked - 1);
  else
    parked = __max(1, parked + 1);

  // set up for next cycle of command arbitration
  def_cmd();
}


//= Harvest final servo commands now that arbitration is done.
// caches "cmd" into "cmd0" for PosCmd, DirCmd, and AuxCmd

void jhcSwapArm::Issue ()
{
  // position command
  pdes0.Copy(pdes);
  pmode0 = pmode;
  prate0 = prate;
  plock0 = plock;

  // gripper orientation command
  ddes0.Copy(ddes);
  dmode0 = dmode;
  drate0 = drate;
  dlock0 = dlock;

  // finger command
  wstop0 = wstop;
  wrate0 = wrate;
  wlock0 = wlock;

  // angular configuration command
  trate0 = trate;
  tlock0 = tlock; 
} 


///////////////////////////////////////////////////////////////////////////
//                       Hand Goal Specification                         //
///////////////////////////////////////////////////////////////////////////

//= Request a particular separation between fingers (or force if negative).
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapArm::HandTarget (double wf, double rate, int bid)
{
  // see if previous command takes precedence
  if (bid < wlock)
    return 0;
  wlock = bid;

  // set width (positive) or force (negative) goal
  wstop = wf;
  wrate = rate;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Arm Goal Specification                         //
///////////////////////////////////////////////////////////////////////////

//= Request both a finger position and gripper orientation.
// defaults to mode 0 (nothing exact) for both position and orientation 
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapArm::ArmTarget (const jhcMatrix& pos, const jhcMatrix& dir, double p_rate, double d_rate, int bid)
{
  double dr = ((d_rate != 0.0) ? d_rate : p_rate);
  int pok, dok;

  pok = PosTarget(pos, p_rate, bid, 0x0);
  dok = DirTarget(dir, dr, bid, 0x0);
  return __min(pok, dok);
}


//= Request a particular Cartesian finger position in local arm coordinates.
// x is to right, y is forward, z is up
// rate is ramping speed relative to standard move speed
// mode bits: 2 = exact Z, 1 = exact Y, 0 = exact X
// returns 1 if newly set, 0 if pre-empted by higher priority
// NOTE: coordinates relative to center of wheelbase and bottom of shelf

int jhcSwapArm::PosTarget (const jhcMatrix& pos, double rate, int bid, int mode)
{
  // see if previous command takes precedence 
  if (bid < plock)
    return 0;
  plock = bid;

  // set up position command
  pdes.Copy(pos);
  prate = rate;
  pmode = mode;
  return 1;
}


//= Request a particular XYZ finger position using explicit local arm coordinates.
// x is to right, y is forward, z is up
// rate is ramping speed relative to standard move speed
// mode bits: 2 = exact Z, 1 = exact Y, 0 = exact X
// returns 1 if newly set, 0 if pre-empted by higher priority
// NOTE: coordinates relative to center of wheelbase and bottom of shelf

int jhcSwapArm::PosTarget (double ax, double ay, double az, double rate, int bid, int mode)
{
  jhcMatrix pos(4);

  pos.SetVec3(ax, ay, az);
  return PosTarget(pos, rate, bid, mode);
}


//= Request a particular Cartesian gripper orientation.
// rate is ramping speed relative to standard reorientation speed
// mode bits: 3 = any pan, 2 = exact roll, 1 = exact tilt, 0 = exact pan
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapArm::DirTarget (const jhcMatrix& dir, double rate, int bid, int mode)
{
  // see if previous command takes precedence 
  if (bid < dlock)
    return 0;
  dlock = bid;

  // set up position command
  ddes.Copy(dir);
  drate = rate;
  dmode = mode;
  return 1;
}


//= Request a particular pan, tilt, roll gripper orientation.
// rate is ramping speed relative to standard reorientation speed
// mode bits: 3 = any pan, 2 = exact roll, 1 = exact tilt, 0 = exact pan
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapArm::DirTarget (double hp, double ht, double hr, double rate, int bid, int mode)
{
  jhcMatrix dir(4);

  dir.SetVec3(hp, ht, hr);
  return DirTarget(dir, rate, bid, mode);
}


//= Request arm to go to its stowed/tucked configuration.
// rate is ramping speed relative to angular motion speed
// position or direction command with same bid trumps tuck command
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcSwapArm::Tuck (double rate, int bid)
{
  if (bid < tlock) 
    return 0;
  tlock = bid;
  trate = rate;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Arm Motion Progress                          //
///////////////////////////////////////////////////////////////////////////

//= Computes difference from given global coordinate goal position in x, y, and z.
// needs height of shelf to correct arm z value
// can optionally save absolute value of component errors
// returns max of absolute differences across all coordinates

double jhcSwapArm::PosErr3D (jhcMatrix& perr, const jhcMatrix& pos, double ht, int abs) const
{
  if (!perr.Vector(4) || !pos.Vector(4))
    Fatal("Bad input to jhcSwapArm::PosErr3D");

  perr.DiffVec3(loc, pos);  
  perr.IncZ(ht);
  if (abs > 0)
    perr.Abs();
  return perr.MaxAbs3();
}


//= Computes Cartesian distance from given global coordinate goal to nominal hand point.
// needs height of shelf to correct arm z value

double jhcSwapArm::PosOffset3D (const jhcMatrix& pos, double ht) const
{
  double dx, dy, dz;

  if (!pos.Vector(4))
    Fatal("Bad input to jhcSwapArm::PosOffset3D");

  dx = loc.X() - pos.X();
  dy = loc.Y() - pos.Y();
  dz = (loc.Z() + ht) - pos.Z();
  return sqrt(dx * dx + dy * dy + dz * dz);
}


//= Computes difference from given goal orientation in pan, tilt, and roll.
// can optionally save absolute value of component errors instead
// returns max of absolute differences across all pose angles

double jhcSwapArm::DirErr (jhcMatrix& derr, const jhcMatrix& dir, int abs) const
{
  if (!derr.Vector(4) || !dir.Vector(4))
    Fatal("Bad input to jhcSwapArm::DirErr");

  derr.DiffVec3(aim, dir);  
  derr.CycNorm3();
  if (abs > 0)
    derr.Abs();
  return derr.MaxAbs3();
}
