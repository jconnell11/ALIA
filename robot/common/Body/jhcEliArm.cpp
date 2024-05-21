// jhcEliArm.cpp : kinematics and serial control for ELI robot arm
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

#include <math.h>
#include <conio.h>
#include <stdio.h>

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"
#include "Interface/jprintf.h"        

#include "Body/jhcEliArm.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcEliArm::jhcEliArm ()
{
  char tag[20] = "selwvug";
  char part[7][20] = {"Shoulder", "Elbow", "Lift", "Wrist", "Veer", "Up/down", "Gripper"}; 
  int i;

  // set size for local vectors
  stop.SetSize(4);
  dstop.SetSize(4);
  tool.SetSize(4);
  tol.SetSize(4);
  dtol.SetSize(4);

  // pose sensor vectors
  ang0.SetSize(7);
//  loc.SetSize(4);
//  aim.SetSize(4);
  fvec.SetSize(4);
  fsm.SetSize(4);

  // set up description of joints
  for (i = 0; i < 7; i++)
  {
    sprintf_s(jt[i].rname, "%c_ramp", tag[i]);
    strcpy_s(jt[i].group, "arm");
    strcpy_s(jt[i].name, part[i]);
    jt[i].jnum = i;
  }

  // no serial port for servos yet
  dyn = NULL;
  aok = -1;

  // set up trapezoidal parameters
  strcpy_s(wctrl.rname, "grip_ramp");
  strcpy_s(pctrl.rname, "hand_ramp");
  strcpy_s(dctrl.rname, "dir_ramp");
  wctrl.done =  0.2;
  pctrl.done =  0.5;
  dctrl.done = -2.0;        // cyclic

  // no run-time calibration
  gcal = 0;
  share = 0;  
  tmax = 220.0;

  // no motion in progress
  stiff = 0;
  ice = 0;
  ice2 = 0;
  clr_locks(1);

  // load specialized arm geometry (in case no config file)
  std_geom();

  // get standard processing values
  LoadCfg();
  Defaults();
}


//= Set up standard values describing the arm geometry.
// really needs stiff = 20 for smoothness and step = 0.2 for small moves
// step = 0.1 with stiff = 10 seems a reasonable compromise
// vmax = 100 degs/sec is fine for most things
// coordinate center = table height (down 4.1" from upper arm link center)
//                     9.9" in front of shoulder axis (four holes)

void jhcEliArm::std_geom ()
{
  // shoulder (9.625 + 2.1 = 9.85"/12.31deg, zero at 135 degs) 
  jt[0].SetServo(  2,    0,     20.0,   0.12,   90.0,  360.0,  360.0,  -2.0 );  // 20 for shake, 0.12 for move
  jt[0].SetGeom(   0.0,  9.85,  12.31,  0.0 ,    0.0, -135.0,  -12.0, 180.0 );    
 
  // elbow joint - reversed (origin in middle)
  jt[1].SetServo( -3,    0,     20.0,   0.1,    90.0,  360.0,  360.0,  -2.0 );  // 20 for shake, 0.1 for move
  jt[1].SetGeom(   0.0,  2.8,   76.8,  90.0,     0.0,    0.0,  -75.0,  80.0 );  // 90 - 12.31

  // lift joint (origin in middle)
  jt[2].SetServo(  5,   -4,     20.0,   0.1,    90.0,  360.0,  360.0,  -2.0 );  // 20 for smooth, 0.1 for move
  jt[2].SetGeom(   1.2,  0.6, -135.0, -90.0,     0.0,    0.0,  -60.0,  90.0 );  // 1.2 was 1.4, -90.0

  // wrist (origin under right lift) = roll 
  jt[3].SetServo(  6,    0,     10.0,   0.031,  90.0,  360.0,  360.0,  -2.0 );  // jitters when 0.1 at end?
  jt[3].SetGeom(   4.7,  0.8,   90.0,  90.0,     0.0,    0.0, -120.0, 120.0 );  // 0.8 was 0.6

  // finger veer (origin in yaw servo) = yaw (pan)
  jt[4].SetServo(  7,    0,     10.0,   0.031,  90.0,  360.0,  360.0,  -2.0 );
  jt[4].SetGeom(   2.5,  0.0,  -90.0,  90.0,     0.0,    0.0, -120.0, 120.0 );  

  // finger up/dn (origin at grip servo) = pitch (tilt)
  jt[5].SetServo( -8,    0,     10.0,   0.031,  90.0,  360.0,  360.0,  -2.0 );
  jt[5].SetGeom(  -1.7,  1.5,  -45.0,  90.0,     0.0,    0.0,  -60.0, 150.0 );  // 1.7 was 1.6

  // gripper (x axis points backward)
  jt[6].SetServo( -9,    0,     20.0,   0.031, 180.0,  180.0,  360.0,  -2.0 );  // 20 for sensing (was 10)
  jt[6].SetGeom(  0.0,   0.0,  180.0,   0.0,     0.0,  -56.0,   -5.0,  55.0 ); 
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for trajectory generation.
// the speeds are standard (rate = 1) values, not top limits
// nothing geometric that differs between bodies

int jhcEliArm::traj_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("arm_traj", 0);
  ps->NextSpecF( &(dctrl.vstd), 180.0,  "Rotation speed (dps)");        // 0.5 sec for 90 degs
  ps->NextSpecF( &(dctrl.astd), 180.0,  "Rotation accel (dps^2)");      // 1 sec to full speed
  ps->NextSpecF( &(dctrl.dstd), 180.0,  "Rotation decel (dps^2)");      // 1 sec from full speed
  ps->NextSpecF( &(pctrl.vstd),  24.0,  "Translation speed (ips)");     // 0.5 sec for 12" travel
  ps->NextSpecF( &(pctrl.astd),  96.0,  "Translation accel (ips^2)");   // 0.25 sec to full speed (was 48)
  ps->NextSpecF( &(pctrl.dstd),  24.0,  "Translation decel (ips^2)");   // 1.0 sec from full speed (was 48)

  ps->NextSpecF( &zf,             0.07, "Z error integral gain");       // was 0.2 then 0.05
  ps->NextSpecF( &zlim,           1.0,  "Max gravity offset (in)");   
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for force control of fingers and arm.
// nothing geometric that differs between bodies

int jhcEliArm::force_params (const char *fname)
{
  jhcParam *ps = &fps;
  int ok;

  ps->SetTag("arm_force", 0);
  ps->NextSpecF( &(wctrl.vstd),   6.0,   "Grip speed (ips)");            // 0.5 sec full close
  ps->NextSpecF( &(wctrl.astd),  24.0,   "Grip accel (ips^2)");          // 0.25 sec to full speed (was 6)
  ps->NextSpecF( &(wctrl.dstd),  24.0,   "Grip decel (ips^2)");          // 0.25 sec to full speed (was 6)
  ps->NextSpecF( &fadj,           0.005, "Grip adjust factor (in/oz)");     
  ps->NextSpecF( &fhold,         16.0,   "Default holding force");       // was 11
  ps->NextSpecF( &fnone,          8.0,   "Default open compliance");   

  ps->NextSpecF( &fmix,           0.2,   "End XY force update");           
  ps->NextSpecF( &fmix2,          0.2,   "End Z force update");          // was 0.1
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for inverse kinematics iteration.
// nothing geometric that differs between bodies
// should call StdTols after this if stop values change 

int jhcEliArm::iter_params (const char *fname)
{
  jhcParam *ps = &ips;
  int ok;

  ps->SetTag("arm_ikin", 0);
  ps->NextSpec4( &tries,   4,    "Max step sizes");        
  ps->NextSpec4( &loops,  30,    "Max refinements");       // was 150 then 30
  ps->NextSpecF( &step,    0.60, "Position step");   
  ps->NextSpecF( &dstep,   0.40, "Direction step");    
  ps->NextSpecF( &shrink,  0.5,  "Step shrinkage");
  ps->NextSpecF( &osc,     1.0 , "Max Q wrt previous");    // was 1.2 then 1.1

  ps->NextSpecF( &close,   0.1,  "Default stop inches");  
  ps->NextSpecF( &align,   2.0,  "Default stop degrees");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used to define stowed arm position.
// nothing geometric that differs between bodies

int jhcEliArm::stow_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("arm_stow", 0);
  ps->NextSpecF( &retx,  -2.0, "Tucked x postion (in)");   
  ps->NextSpecF( &rety,  10.5, "Tucked y position (in)");  
  ps->NextSpecF( &retz,  -2.0, "Tucked z position (in)");        // was -1 then -3
  ps->NextSpecF( &rdir, 180.0, "Tucked point direction (deg)");
  ps->NextSpecF( &rtip, -15.0, "Tucked tip direction (deg)");    
  ps->NextSpecF( &rgap,   0.5, "Initial grip width (in)");

  ps->NextSpecF( &rets, -12.0, "Tight shoulder angle (deg)");    // SetGeom must allow
  ps->NextSpecF( &rete,  80.0, "Tight elbow angle (in)");        // SetGeom must allow
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for residual finger and coordinate geometry.
// shoulder = 3.3" left of center, 6.9" in front of wheels, 2.4" over shelf bottom

int jhcEliArm::geom_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("arm_origin", 0);
  ps->NextSpecF( &ax0, -3.3, "Shoulder right of wheels (in)");  
  ps->NextSpecF( &ay0,  6.9, "Shoulder fwd of wheels (in)");  
  ps->NextSpecF( &az0,  2.4, "Shoulder up from shelf (in)");  
  ps->NextSpecF( &fc,   3.6, "Crease distance from axis (in)"); 
  ps->NextSpecF( &fp,   4.0, "Pad distance from axis (in)");   
  ps->NextSpecF( &ft,   4.4, "Tip distance from axis (in)");  

  ps->NextSpecF( &dpad, 1.0, "Grip point in from pad (in)");   // was 0.6 then 0.4
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Read all relevant defaults variable values from a file.

int jhcEliArm::Defaults (const char *fname)
{
  int i, ok = 1;

  // load any overall parameters
  ok &= traj_params(fname);
  ok &= force_params(fname);
  ok &= iter_params(fname);
  ok &= stow_params(fname);

  // try loading specialized joint values
  for (i = 0; i < 7; i++)
    ok &= jt[i].Defaults(fname);
  return ok;
}


//= Read just body specific values from a file.
// load fps finger lengths also?

int jhcEliArm::LoadCfg (const char *fname)
{
  int i, ok = 1;

  ok &= geom_params(fname);
  for (i = 0; i < 7; i++)
    ok &= jt[i].LoadCfg(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcEliArm::SaveVals (const char *fname) const
{
  int i, ok = 1;

  // save overall parameters
  ok &= tps.SaveVals(fname);
  ok &= fps.SaveVals(fname);
  ok &= ips.SaveVals(fname);
  ok &= sps.SaveVals(fname);

  // save specialized joint values
  for (i = 0; i < 7; i++)
    ok &= jt[i].SaveVals(fname);
  return ok;
}


//= Write current body specific values to a file.
// save fps finger lengths also?

int jhcEliArm::SaveCfg (const char *fname) const
{
  int i, ok = 1;

  ok &= gps.SaveVals(fname);
  for (i = 0; i < 7; i++)
    ok &= jt[i].SaveCfg(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                            Configuration                              //
///////////////////////////////////////////////////////////////////////////

//= Associate arm with some (possibly shared) Dyamixel interface.

void jhcEliArm::Bind (jhcDynamixel& ctrl)
{
  int i;

  for (i = 0; i < 7; i++)
    jt[i].Bind(ctrl);
  dyn = &ctrl;
  aok = 1;
}


//= Reset state for the beginning of a sequence.
// if rpt > 0 then prints to log file
// generally aok: -1 = no port, 0 = comm error, 1 = fine

int jhcEliArm::Reset (int rpt, int chk)
{
  double v;
  int i;

  // announce entry
  if (rpt > 0)
    jprintf("\nArm reset ...\n");
  clr_locks(1);
  CfgClear();
  ArmClear();
  HandClear();

  // set up kinematic parameters 
  FingerTool(dpad);
  StdTols();
  zint = 0.0;
  fwin = -1.0;
  
  // make sure hardware is working
  if (dyn == NULL)
  {
    aok = -1;
    return fail(rpt);
  }
  aok = 1;

  // clear any servo errors (e.g. gripper overload)
  if (rpt > 0)
  {
    jprintf("  servo clear ...\n");
    jprintf("    joint");
  }
  for (i = 0; i < 7; i++)
  {
    if (rpt > 0)
      jprintf(" %d", i);
    if (jt[i].Boot() <= 0)
      return fail(rpt);
  }
  if (rpt > 0)
    jprintf("\n");

  if (chk > 0)
  {
    // possibly check supply voltage
    if (rpt > 0)
      jprintf("  battery ...\n");
    if ((v = Voltage()) <= 0.0)
      return fail(rpt);
    if (rpt > 0)
      jprintf("    %3.1f volts nominal\n", v);

    // possibly look for all servos
    if (rpt > 0)
      jprintf("  servo check ...\n");
    Check(0, chk);
  }

  // initialize servo limits and speeds (also syncs controllers)
  if (rpt > 0)
    jprintf("  servo init ...\n");
  for (i = 0; i < 7; i++)
    if (jt[i].Reset() <= 0)
      return fail(rpt);

  // possibly normalize joint angles
  if (rpt > 0)
    jprintf("  untwist ...\n");
  Untwist();

  // possibly find elbow servo balance
  if (rpt > 0)
    jprintf("  lift balance ...\n");
  ShareLift();

  // possibly find gripper close point (x2)
  if (rpt > 0)
    jprintf("  grip zero ...\n");
  ZeroGrip();

  // freeze arm (and sync profile generators)
  if (rpt > 0)
    jprintf("  freeze ...\n");
  Update();
  Freeze();
  first = 1;

  // initialize targets and positions
  if (rpt > 0)
    jprintf("  current pose ...\n");
  Update();
  if (rpt > 0)
  {
    loc.PrintVec3("    loc", "%3.1f");
    aim.PrintVec3("    aim", "%3.1f");
  }
  ice = 0;
  ice2 = 0;
  Freeze();

  // instantaneous speed estimates
  now = 0;
  iarm = 0.0;
  igrip = 0.0;
  parked = 0;

  // finished
  if (rpt > 0)
    jprintf("    ** good **\n");
  return aok;
}


//= Failure message for some part of initialization.

int jhcEliArm::fail (int rpt) 
{
  if (aok > 0)
    aok = 0;
  if (rpt > 0)
    jprintf("    >> BAD <<\n");
  return aok;
}


//= Check that all servos are responding.
// if rpt > 0 then pops dialog boxes for failed servos

int jhcEliArm::Check (int rpt, int tries)
{
  int i, n, yack = 0;
  
  // make sure hardware is working
  if (dyn == NULL)
  {
    aok = -1;
    return aok;
  }

  for (n = 1; n <= tries; n++)
  {
    // only potentially complain on last trial
    if ((rpt > 0) && (n >= tries))
      yack = 1;

    // see if any servo fails to respond
    aok = 1;
    for (i = 0; i < 7; i++)
      if (jt[i].Check(yack) <= 0)
      {
        aok = -1;
        break;
      }

    // everything is up and running
    if (aok > 0)
      break;
  }
  return aok;
}


//= Tells current voltage of main battery (to nearest 100mv).
// also updates expected running torque of servos
// exchanges information with servo (i.e. takes time)

double jhcEliArm::Voltage ()
{
  double v = jt[6].Battery();

  if ((v > 0.0) && (dyn != NULL))
    tmax = dyn->HoldAX12(v);
  return v;
}


///////////////////////////////////////////////////////////////////////////
//                           Kinematic Controls                          //
///////////////////////////////////////////////////////////////////////////

//= Set working point to be some distance in from the pad location.

void jhcEliArm::FingerTool (double deep)
{
  SetTool(fp - deep, 0.0, 0.0);
}


//= Set up standard tolerances for solving inverse kinematics.
// use PosTols and DirTols to mark certain elements as less important

void jhcEliArm::StdTols ()
{
  PosTols(close, close, close);
  DirTols(align, align, align);
}


///////////////////////////////////////////////////////////////////////////
//                          Low Level Commands                           //
///////////////////////////////////////////////////////////////////////////

//= Set desired angles for all servos to be current angle.
// generally should call Update just before this
// if tupd > 0 then calls Issue after this

int jhcEliArm::Freeze (double tupd)
{
  FreezeArm(1, 0.0);
  FreezeHand(1, 0.0);
  if (tupd > 0.0)
    Issue(tupd);
  return aok;
}


//= Keep the arm in the current configuration.
// generally should call Update just before this
// if tupd > 0 then calls Issue after this

int jhcEliArm::FreezeArm (int doit, double tupd)
{
  // reset edge trigger 
  if (doit <= 0)
  {
    ice = 0;
    return aok;
  }

  // remember angles at first call (prevents drift)
  if (ice <= 0)                                      // needed!
  {
//    ArmStop(1.5, 2001);
    pctrl.RampTarget(loc);
    dctrl.RampTarget(aim);
    ice = 1;
  }

  // possibly talk to servos
  stiff = 1;
  if (tupd > 0.0)
    Issue(tupd);
  return aok;
}


//= Keep the hand at the current width or gripping force.
// generally should call Update just before this
// if tupd > 0 then calls Issue after this

int jhcEliArm::FreezeHand (int doit, double tupd)
{
  // reset edge trigger 
  if (doit <= 0)
  {
    ice2 = 0;
    return aok;
  }

  // possibly keep hand in gripping mode (with same force)
  // remember width at first call (prevents drift)
  if ((Squeeze() > fnone) && (WidthErr(MaxWidth()) > 0.1))
    SqueezeTarget((fwin > 0.0) ? fwin : fhold);
  else if (ice2 <= 0)
  {
    wctrl.RampTarget(w0);
    ice2 = 1;
  }

  // possibly talk to servos
  if (tupd > 0.0)
    Issue(tupd);
  return aok;
}


//= Turn off torque on all arm and hand servos (only).
// immediately talks to servos

int jhcEliArm::Limp ()
{
  int svo[8];
  int i, n = 0;
  
  // make sure hardware is working
  if ((aok < 0) || (dyn == NULL))
    return aok;
  aok = 1;

  // no joint based arm motion underway
  stiff = 0;
  CfgClear();
  ArmClear();
  HandClear();
  zint = 0.0;

  // collect servo id's and send disable commands
  for (i = 0; i < 7; i++)
    n += jt[i].ServoNums(svo, n);
  dyn->MultiLimp(svo, n);

  // make sure readings are up to date
  Update();
  wctrl.RampTarget(w0);
  pctrl.RampTarget(loc);
  dctrl.RampTarget(aim);
  return aok;
}


///////////////////////////////////////////////////////////////////////////
//                           Basic Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Find current pose of arm by talking to servos.
// automatically resets "lock" for new bids
// saves configuration in "ang0", "loc", and "aim" vectors 
// also computes endpoint force "fvec" and smoothed version "fsm"

int jhcEliArm::Update (int mega)
{
  jhcMatrix orig(4), diff(4);
  UL32 last = now;
  double s, a, g, wprev = w0, mix = 0.2, twang = 3.0;      // 3 ips = 0.1" / 33 ms
  int i;

  // make sure hardware is working
  if (aok < 0)
    return aok;
  orig.Copy(loc);

  // works from end to base so more sensitive angles are "fresher"
  if (mega > 0)
    dyn->MegaUpdate(2, 9);       // change if servos renumbered 
  for (i = 6; i >= 0; i--)
    if (jt[i].GetState() <= 0)
      aok = 0;

  // consolidate angles to generate current pose
  get_angles(ang0);                          
  get_pose(loc, aim, ang0, 0);     

  // decode and cache gripper state
  w00 = w0;
  w0 = deg2w(jt[6].Angle());
  sqz = jt[6].Torque(tmax) / -fc;   

  // get force and simple IIR filtered version
  find_force(fvec);
  if (first > 0)
  {
    // just copy raw values (and set old width)
    fsm.Copy(fvec);
    w00 = w0;
    first = 0;
  }
  else
  {
    // gradually approach sensed values (Z slower)
    diff.DiffVec3(fvec, fsm);
    fsm.IncVec3(fmix * diff.X(), fmix * diff.Y(), fmix2 * diff.Z());
  }

  // instantaneous speed estimates
  now = jms_now();
  if (last != 0)
    if ((s = jms_secs(now, last)) > 0.0)
    {
      a = orig.PosDiff3(loc) / s;
      g = fabs(w0 - wprev) / s;
      iarm  += mix * (a - iarm); 
      igrip += mix * (g - igrip); 
    }

  // do qualitative evaluation of motion
  if (iarm >= twang)
    parked = __min(0, parked - 1);
  else
    parked = __max(1, parked + 1);

  // set up for new target arbitration
  clr_locks(0);
  return aok;
}


//= Resolve servo torques into endpoint forces.

void jhcEliArm::find_force (jhcMatrix &dir) const
{
  jhcMatrix f2t(3, 3), t2f(3, 3);
  jhcMatrix t(3), f(3);

  // build vector of torques 
  t.VSet(0, -jt[0].Torque(tmax));
  t.VSet(1, -jt[1].Torque(tmax));
  t.VSet(2, -jt[2].Torque(tmax));

  // multiply by inverse 3x3 Jacobian
  jt3x3(f2t);
  if (t2f.Invert(f2t) <= 0)
    return;
  f.MatVec(t2f, t);

  // apply fudge factor to match experiments (2.5x?)
  dir.SetVec3(f.X(), f.Y(), f.Z(), 0.0);
  dir.Scale(2.0);
}


//= Clear winning command bids for all resources.
// can optionally clear previous bids also

void jhcEliArm::clr_locks (int hist)
{
  // remember winning bid of last arbitration cycle
  wlock0 = ((hist > 0) ?  0 : wlock);
  alock0 = ((hist > 0) ? -1 : alock);
  plock0 = ((hist > 0) ?  0 : plock);
  dlock0 = ((hist > 0) ?  0 : dlock);

  // set up for new target arbitration (prefer Cartesian if none)
  wlock =  0;
  alock = -1;
  plock =  0;
  dlock =  0;
}


//= Move arm along a set of acceleration-limited linear segments.
// takes typical call-back interval and move slow-down factor
// based on target pose and 3 motion speeds: move, turn, grab
// automatically handles acceleration and deceleration
// does not worry about joint inertia or possible ringing
// plots a pursuit point between current pose and target
// typically want all joint changes < 10 degs for linearity
// takes goal from members "wwin", "fwin", "awin", "pwin", and "dwin"
// sets ramped speeds in members "wvel", "pvel", "dvel", and "avel" 
// if "fwin" is positive then tries to maintain given finger force
// if "alock" > "plock" and "dlock" then sets arm joints directly
// assumes Update already called to get "ang0", "loc", and "aim"

int jhcEliArm::Issue (double tupd, double lead, int send)
{
  jhcMatrix ang(7), err(4); 
  double w, zerr, ztol = 0.1, crazy = 25.0;
  int doit;

  // check for working communication and reasonable arguments
  if (aok < 0) 
    return aok;
  if (tupd <= 0.0)
    Fatal("Bad input to jhcEliArm::Issue");

  // set default if no hand target specified (keep gripping if some force)
  doit = (((wlock <= 0) && (fwin < 0.0)) ? 1 : 0);
  FreezeHand(doit, 0.0);

  // slightly open or close gripper to change force
  if (fwin >= 0.0)
    wctrl.RampInc((Squeeze() - fwin) * fadj);

  // check if arm is under active command
  if (stiff > 0)
  {
    // set default if no arm target specified
    doit = (((alock <= 0) && (plock <= 0) && (dlock <= 0)) ? 1 : 0);
    FreezeArm(doit, 0.0);

    // check if mode is joint angles or Cartesian 
    if ((alock > plock) && (alock > dlock))
    {
      config_move(tupd, lead);
      zint = 0.0;
    }
    else
    {
      // compare current height to original height of PREVIOUS trajectory stop point
      zerr = loc.Z() - (stop.Z() - zint);

      // choose gripper trajectory point position, orientation, and width
      pctrl.RampNext(stop, loc, tupd, lead);
      pctrl.ExactNext(stop, pmode);
      dctrl.RampNext(dstop, aim, tupd, lead);
      dctrl.ExactNext(dstop, dmode);
      w = wctrl.RampNext(Width(), tupd, lead);

      // apply gravity compensation with deadband
      if (fabs(zerr) > ztol)
        zint -= zf * zerr;
      zint = __max(-zlim, __min(zint, zlim));
      stop.IncZ(zint);

//char txt[80], txt2[80];
//jprintf("ISSUE:  goal pos = %s, gdir = %s, zint = %3.1f\n", (pctrl.cmd).ListVec3(txt, "%5.1f"), (dctrl.cmd).ListVec3(txt2, "%5.1f"), zint);
//jprintf("  pmode = %02X, dmode = %02X\n", pmode & 0x7, dmode & 0x7);

      // convert to joint space and send servo commands 
      if (pick_angles(ang, stop, dstop, w, &ang0, 0) >= crazy)
        ang.Copy(ang0);
      simul_move(ang, ang0, lead * tupd);
    }
  }

  // send to servos
  if (send > 0)
    if (dyn->MultiSend() <= 0)
      aok = 0;
  return aok;
}


//= Assemble positions and velocities for all servos.

void jhcEliArm::config_move (double tupd, double lead)
{
  int id[8];
  double pos[8], vel[8];
  double stop, slew;
  int i, n = 0;

  // load arm joint guard positions and speeds
  for (i = 0; i < 6; i++)
  { 
    // update ideal position and velocity first
    stop = jt[i].RampNext(jt[i].Angle(), tupd, lead);
    slew = jt[i].RampVel();
    n += jt[i].ServoCmd(id, pos, vel, n, stop, slew);
  }

  // add in proper gripper angle and speed 
  stop = wctrl.RampNext(Width(), tupd, lead);
  slew = wctrl.RampVel();
  n += jt[6].ServoCmd(id, pos, vel, n, w2deg(stop), v2dps(slew, Width()));

  // add to big servo packet 
  dyn->MultiPosVel(id, pos, vel, n);
}


//= Move from angular configuration 0 to 1 so all motion completes at once.
// specify desired time for transition, if send <= 0 then queues commands

void jhcEliArm::simul_move (const jhcMatrix& ang1, const jhcMatrix& ang0, double secs)
{
  int id[8];
  double pos[8], vel[8];
  double dps, f, sc = 1.0;
  int i, n = 0;

  // make sure hardware is working
  if ((aok < 0) || (dyn == NULL))
    return;
  aok = 1;

  // see what factor needed to constrain joint velocities inside limits
  // all transitions slowed down (sc) to respect joint with lowest limit
  for (i = 0; i < 7; i++)
  {
    dps = fabs(ang1.VRef(i) - ang0.VRef(i)) / secs;
    if (dps > 0.0)
    {
      f = jt[i].vstd / dps;
      sc = __min(sc, f);
    }
  }

  // assemble basic command and add to big packet
  for (i = 0; i < 7; i++)
  {
    dps = sc * fabs(ang1.VRef(i) - ang0.VRef(i)) / secs;
    n += jt[i].ServoCmd(id, pos, vel, n, ang1.VRef(i), dps);
  }
  dyn->MultiPosVel(id, pos, vel, n);
} 


///////////////////////////////////////////////////////////////////////////
//                           Forward Kinematics                          //
///////////////////////////////////////////////////////////////////////////

//= Get current joint angles (in degrees).as an array.
// always gets 7 values (6 for arm + 1 for gripper)
// does NOT update joint transforms with current angles (call get_pose)

void jhcEliArm::get_angles (jhcMatrix& ang) const
{
  int i;

  for (i = 0; i < 7; i++)
    ang.VSet(i, jt[i].Angle());
}


//= Convert a gripper joint angle to an opening width.

double jhcEliArm::deg2w (double degs) const
{
  return(2.0 * fc * sin(0.5 * D2R * degs));
}


//= For given joint angles get position of end tool and orientation of gripper.
// dir vector is gripper pan, tilt, and roll angles plus opening width
// always updates joints' transform matrices
// returns gripper opening

double jhcEliArm::get_pose (jhcMatrix& end, jhcMatrix& dir, const jhcMatrix& ang, int finger)
{
  jhcMatrix xdir(4), ydir(4);
  double sep;
  int i;

  // update matrics for main joints of arm
  jt[0].SetMapping(ang.VRef(0), NULL, ax0, ay0, az0);
  for (i = 1; i <= 5; i++)
    jt[i].SetMapping(ang.VRef(i), jt + i - 1);

  // handle gripper specially
  sep = ang.VRef(6);
  if (finger == 0)
    sep *= 0.5;
  else if (finger > 0)
    sep = 0.0;
  jt[6].SetMapping(sep, jt + 5);

  // find tool mapping and local axis unit vectors at gripper
  jt[6].GlobalMap(end, tool);
  jt[6].EndX(xdir);
  jt[6].EndY(ydir);

  // resolve into angles
  dir.SetP(xdir.YawVec3());      // pan
  dir.SetT(xdir.PitchVec3());    // tilt
  dir.SetR(ydir.PitchVec3());    // roll
  dir.SetH(0.0);

  // add opening width at end
  return deg2w(ang.VRef(6));  
}


///////////////////////////////////////////////////////////////////////////
//                            Inverse Kinematics                         //
///////////////////////////////////////////////////////////////////////////

//= Convert an opening width to a gripper joint angle.

double jhcEliArm::w2deg (double w) const
{
  return(2.0 * R2D * asin(0.5 * w / fc));
}


//= Convert gripper width speed in inches/sec to servo degrees/sec.

double jhcEliArm::v2dps (double v, double w) const
{
  return(2.0 * R2D * v / sqrt(4.0 * fc * fc - w * w));
} 


//= Takes an endpoint offset and figures joint angles to move it to the specified pose.
// aim is a vector of gripper yaw, pitch, and roll angles plus opening width
// assumes cfg array is set to starting configuration (not strictly necessary)
// if oscillation then reduces step size, stops after full loop count at some scale
// returns max ratio of error (pos or dir) to tolerance (i.e. solved means <= 1.0)
// about 16us per loop with transpose (so typical 40 loops = 0.6ms) on a 3.2GHz Xeon

double jhcEliArm::pick_angles (jhcMatrix& ang, const jhcMatrix& end, const jhcMatrix& aim, 
                               double sep, const jhcMatrix *cfg, int finger)
{
  jhcMatrix jinv(3, 6), djinv(3, 6);
  jhcMatrix ang0(7), win(7), adj(6);
  jhcMatrix pos(4), dir(4), pfix(4), dfix(4);
  double pq, dq, qcfg, pq0, dq0, best = -1.0, f = step, df = dstep;
  int scale, n, i;

  if (!ang.Vector(7) || !end.Vector(3) || !aim.Vector(3) || 
      ((cfg != NULL) && !cfg->Vector(7)))
    Fatal("Bad input to jhcEliArm::pick_angles");

//end.PrintVec3("PICK_ANGLES: via", "%5.1f", 0, 0);
//aim.PrintVec3(", vdir", "%5.1f");

  // copy starting configuration (if any) and directly solve for gripper opening 
  ang0.Zero();
  if (cfg != NULL)
    ang0.Copy(*cfg);
  ang0.VSet(6, w2deg(sep));

//get_pose(pos, dir, ang0, finger);
//pos.PrintVec3("        now: pos", "%5.1f", 0, 0);
//dir.PrintVec3(",  dir", "%5.1f");
//char txt4[80];
//jprintf("  now: %s\n", ang0.ListVec(txt4));

  // keep using smaller step size until solution found
  for (scale = 0; scale < tries; scale++)
  {
    // take multiple partial steps from current configuration 
    ang.Copy(ang0);
    for (n = 0; n <= loops; n++)
    {
      if (n > 0)
      {
        // compute transpose of current Jacobian to get adjustment hints
        // Note: true inverse prefers changing X with wrist instead of shoulder!
        j_trans(jinv, djinv, pos);

        // adjust joint angles for either better position or better direction
        if (pq >= dq)
        {
          pfix.ScaleVec3(f);           // scales position shift
          adj.MatVec0(jinv, pfix);
          ang.IncVec(adj);
        }
        else
        {
          dfix.ScaleVec3(df);          // scales rotation angle
          adj.MatVec0(djinv, dfix);
          ang.IncVec(adj);
        }

        // make sure joint angles respect movement limits
        for (i = 0; i < 7; i++)
          ang.VSet(i, jt[i].Clamp(ang.VRef(i)));
        pq0 = pq;
        dq0 = dq;
      }

      // find new pose and calculate errors relative to tolerances
      get_pose(pos, dir, ang, finger);
      pq = pos_diff(pfix, end, pos);
      dq = dir_diff(dfix, aim, dir);
      qcfg = __max(pq, dq);

//char txt[80], txt2[80];
//jhcMatrix derr(4);
//derr.DiffVec3(aim, dir);  
//derr.CycNorm3();
//jprintf("    frac %4.2f [%2d]: perr %s, derr %s -> %5.2f %5.2f : %5.2f %c\n", 
//        f, n, pfix.ListVec3(txt, "%5.2f"), derr.ListVec3(txt2, "%5.2f"), pq, dq, qcfg,
//        ((qcfg < best) ? '<' : ' '));

      // save if best so far then check if goal achieved  
      if ((best < 0.0) || (qcfg < best))
      { 
        win.Copy(ang);
        best = qcfg;
        if (best <= 1.0)
          break;
      }
      else if (n > 0) 
      {               
        // quit this scale if q is oscillating (i.e. increases)                   
        if (((pq0 >= dq0) && (pq >= (osc * pq0))) ||    
            ((dq0 >  pq0) && (dq >= (osc * dq0))))
        break; 
      }
    }

    // unless done or scale fully scanned shrink scale and try again
    if ((best <= 1.0) || (n >= loops))
      break;
    f  *= shrink;
    df *= shrink;
  }
  
  // make sure best configuration selected and save statistics of run
  ang.Copy(win);

//win.DiffVec(ang, ang0);
//char txt3[80];
//jprintf("  Q = %4.2f, change %s\n", best, win.ListVec(txt3, "%4.2f"));
//jprintf("  cmd: %s\n", ang.ListVec(txt3));

  return best;
}


//= Find transpose of arm's Jacobian and split into position and direction parts.
// assumes end vector (pos) and joint DH matrices up to date (i.e. call get_pose)

void jhcEliArm::j_trans (jhcMatrix& jact, jhcMatrix& djact, const jhcMatrix& pos) const
{
  jhcMatrix mv(4);
  const jhcMatrix *axis, *orig;
  int i; 

  for (i = 0; i < 6; i++)
  {
    // find joint's location and axis of rotation 
    axis = jt[i].AxisZ();
    orig = jt[i].Axis0();

    // determine rotation sensitivity 
    mv.DiffVec3(pos, *orig);   
    mv.CrossVec3(*axis, mv);

    // save coefficients in arrays
    jact.MSet(0, i, mv.X());
    jact.MSet(1, i, mv.Y());
    jact.MSet(2, i, mv.Z());
    djact.MSet(0, i, axis->X());       
    djact.MSet(1, i, axis->Y());       
    djact.MSet(2, i, axis->Z());       
  }
}


//= Determines the position error of end point relative to goal.
// returns max coordinate difference wrt tolerance (shows progress better than avg)

double jhcEliArm::pos_diff (jhcMatrix& fix, const jhcMatrix& end, const jhcMatrix& pos) const
{ 
  double diff, scd, worst = 0.0;
  int i;

  // find which direction to move the end point
  fix.DiffVec3(end, pos);
  for (i = 0; i < 3; i++)
  {
    // scale absolute difference by associated tolerance to find worst fit
    diff = fabs(fix.VRef(i));
    scd = diff / tol.VRef(i);
    worst = __max(worst, scd);
  }
  return worst; 
}


//= Determines the orientation error of end point relative to goal direction.
// both aim and dir are vectors of pan, tilt, and roll angles
// if d0mode < 0 then ignores all pan errors
// computes XYZ axis of desired composite rotation scaled by amount of rotation
// returns max error relative to tolerances for command PTR angles (shows progress better than avg)

double jhcEliArm::dir_diff (jhcMatrix& dfix, const jhcMatrix& aim, const jhcMatrix& dir) const
{
  jhcMatrix now(4), goal(4), q1(4), q2(4), q3(4), slew(4);
  double dot, degs, diff, scd, worst = 0.0;
  double pan   = (((dmode & 0x8) != 0) ? dir.P() : aim.P());         
  int i, start = (((dmode & 0x8) != 0) ? 1 : 0);

  // convert angle specs into pointing vectors based on pan and tilt (only)
  now.EulerVec3(dir.P(), dir.T());
  goal.EulerVec3(pan, aim.T());

  // form quaternion to rotate around current gripper pointing vector
  q1.Quaternion(now, aim.R() - dir.R());

  // figure out how much to slew the pointing vector itself
  dot = goal.DotVec3(now); 
  dot = __max(-1.0, __min(dot, 1.0));
  degs = R2D * acos(dot);

  // find axis around which pointing vector slews then form quaternion
  slew.CrossVec3(now, goal);                           
  slew.UnitVec3();
  q2.Quaternion(slew, degs);

  // compose rotations and convert back to scaled rotation axis and total angle 
  q3.CascadeQ(q1, q2);
  dfix.RotatorQ(q3);

  for (i = start; i < 3; i++)
  {
    // find component-wise absolute differences in PTR angles (not XYZ angles)
    diff = fabs(aim.VRef(i) - dir.VRef(i));
    if (diff > 180.0)
      diff = 360.0 - diff;

    // scale by associated tolerance to find worst fit
    scd = diff / dtol.VRef(i);
    worst = __max(worst, scd);             
  }
  return worst; 
}


//= Construct the current 3x3 Jacobian transpose for the first 3 joints.
// inverse useful for converting torques into endpoint forces

void jhcEliArm::jt3x3 (jhcMatrix& f2t) const
{
  jhcMatrix mv(4);
  const jhcMatrix *axis, *orig;
  int i;

  for (i = 0; i < 3; i++)
  {
    // find joint's axis of rotation and location
    axis = jt[i].AxisZ();
    orig = jt[i].Axis0();
 
    // get rotation sensitivity 
    mv.DiffVec3(loc, *orig);   
    mv.CrossVec3(*axis, mv);

    // save in array (transposed)
    f2t.MSet(0, i, mv.X());
    f2t.MSet(1, i, mv.Y());
    f2t.MSet(2, i, mv.Z());
  }
}


///////////////////////////////////////////////////////////////////////////
//                      HAND - Goal Specification                        //
///////////////////////////////////////////////////////////////////////////

//= Request a particular separation between fingers.
// negative rate does not scale acceleration (for snappier response)
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::WidthTarget (double sep, double rate, int bid)
{
  // see if previous command takes precedence
  if (bid <= wlock)
    return 0;
  wlock = bid;

  // set width goal and disable force target
  wctrl.RampTarget(__max(0.0, sep), rate);
  fwin = -1.0;
  return 1;
}


//= Request a gripping force to maintain with fingers.
// uses fwin >= 0 to override width-based control
// negative rate does not scale acceleration (for snappier response)
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::SqueezeTarget (double force, int bid)
{
  // see if previous command takes precedence
  if (bid <= wlock)
    return 0;
  wlock = bid;

  // set force level
  fwin = __max(0.0, force);
  return 1;
}


//= Set finger physical separation or grasp force (if sep < 0).
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::HandTarget (double sep, double rate, int bid)
{
  if (sep < 0.0)
    return SqueezeTarget(-sep, bid);
  return WidthTarget(sep, rate, bid); 
}


///////////////////////////////////////////////////////////////////////////
//                        HAND - Motion Progress                         //
///////////////////////////////////////////////////////////////////////////

//= Returns difference from given gripper opening width.
// always returns absolute value 

double jhcEliArm::WidthErr (double sep) const
{
  return fabs(Width() - sep);
}


//= Returns signed difference from given gripper closing force.

double jhcEliArm::SqueezeErr (double f) const
{
  return(Squeeze() - f);
}


///////////////////////////////////////////////////////////////////////////
//                      HAND - Goal Characteristics                      //
///////////////////////////////////////////////////////////////////////////

//= Tells the maximum width that the gripper can be set for.
// uses distance to outer crease in fingers (not tips)
// answer is in inches

double jhcEliArm::MaxWidth () const
{
  return deg2w(jt[6].MaxAng());
}


//= Tell if object will fit inside gripper.

int jhcEliArm::Graspable (double wid) const
{
  if ((wid >= 0.0) && (wid <= MaxWidth()))
    return 1;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                      ARM - Current Information                        //
///////////////////////////////////////////////////////////////////////////

//= Get current 6 joint angles for the arm.

void jhcEliArm::ArmConfig (jhcMatrix& ang) const
{
  if (!ang.Vector(6))
    Fatal("Bad input to jhcEliArm::ArmConfig");
  ang.Copy(ang0);
}


//= Get current pose of finger crease using cached values.
// NOTE: coordinates relative to center of wheelbase and bottom of shelf

void jhcEliArm::ArmPose (jhcMatrix& pos, jhcMatrix& dir) const
{
  if (!pos.Vector(4) || !dir.Vector(4))
    Fatal("Bad input to jhcEliArm::ArmPose");
  pos.Copy(loc); 
  dir.Copy(aim);
}


//= Get current position of finger crease using cached values.
// x is to right, y is forward, z is up in global system
// NOTE: coordinates relative to center of wheelbase and bottom of shelf
/*
void jhcEliArm::Position (jhcMatrix& pos) const
{
  if (!pos.Vector(4))
    Fatal("Bad input to jhcEliArm::Position");
  pos.Copy(loc); 
}
*/

//= Get current direction of fingers using cached values.
// "dir" gets a vector of gripper yaw, pitch, and roll ANGLES plus opening width
// Note: pan axis bisects finger creases, but hand shape not quite symmetric around this
/*
void jhcEliArm::Direction (jhcMatrix& dir) const
{
  if (!dir.Vector(4))
    Fatal("Bad input to jhcEliArm::Direction");
  dir.Copy(aim);
}
*/

//= Interpret wrist errors as a force through grip point.
// computes direction of force (unit vec) scaled by magnitude (oz)
// this is the force applied to (not generated by) the fingers
// can subtract off a presumed gravity loading (e.g. z0 = -10)
// NOTE: static force not very reliable when arm is moving 

int jhcEliArm::ForceVect (jhcMatrix &dir, double z0, int raw) const
{
  if (!dir.Vector(4))
    Fatal("Bad input to jhcEliArm::ForceVect");
  dir.Copy((raw > 0) ? fvec : fsm);
  dir.IncVec3(0.0, 0.0, -z0);
  return 1;
}


//= Resolve wrist errors to find finger force (oz) along a given axis.
// does not compensate for gravity loading in Z direction
// NOTE: static force not very reliable when arm is moving 

double jhcEliArm::ForceAlong (const jhcMatrix &dir, int raw) const
{
  if (!dir.Vector(4))
    Fatal("Bad input to jhcEliArm::ForceAlong");
  return dir.DotVec3(((raw > 0) ? fvec : fsm));
}


//= Get estimate of vertical force acting on gripper (oz).
// can subtract off an initial value (e.g. gravity loading z0 = -10)
// negative is downward (weight), positive is upward (surface contact)
// hand load approximately = -0.4 * arm->ForceZ(-18.5) 
// NOTE: static force not very reliable when arm is moving 

double jhcEliArm::ForceZ (double z0, int raw) const
{
  if (raw > 0)
   return(fvec.Z() - z0);
  return(fsm.Z() - z0);
}


//= Estimates weight (oz) of held object using only main lift joint.
// fsc = 0.57 suggests tmax = 120 rather than 214 calculated from voltage
// ignores actual mass distibution of forearm and lift angle for gravity load
// ignores changes due to wrist pose, but object moment arm depends on wrist pan 
// NOTE: most accurate after a short vertical lift

double jhcEliArm::ObjectWt (double grav, double fsc) const
{
  jhcMatrix base(4);
  double dx, dy, dot, oz, tq = jt[2].Torque(tmax), rads = D2R * Forearm();

  LiftBase(base);
  dx = loc.X() - base.X();
  dy = loc.Y() - base.Y();
  dot = dx * cos(rads) + dy * sin(rads);   // moment arm
  oz = fsc * (tq / dot) - grav;            // linear correction
  return __max(0.0, oz);
}


///////////////////////////////////////////////////////////////////////////
//                      ARM - Goal Specification                         //
///////////////////////////////////////////////////////////////////////////

//= Clear all joint progress indicators.

void jhcEliArm::CfgClear ()
{
  int i;

  for (i = 0; i < 6; i++)
    jt[i].RampReset();
}


//= Request the arm joints to assume the given angles at a single rate.
// rate is ramping speed relative to standard reorientation speed
// negative rate does not scale acceleration (for snappier response)
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::CfgTarget (const jhcMatrix& ang, double rate, int bid)
{
  jhcMatrix rates(6);

  rates.FillVec(rate);
  return CfgTarget(ang, rates, bid);
}


//= Request the arm joints to assume the given angles at the given rates.
// rate is ramping speed relative to standard reorientation speed
// negative rate does not scale acceleration (for snappier response)
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::CfgTarget (const jhcMatrix& ang, const jhcMatrix& rates, int bid)
{
  int i; 

  if (!ang.Vector(6) || !rates.Vector(6))
    Fatal("Bad input to jhcEliArm::CfgTarget");

  // see if previous command takes precedence (defers to equal xyz)
  if ((bid <= alock) || (bid <= plock) || (bid <= dlock))
    return 0;

  // set goal angle and rate for each joint
  alock = bid;
  stiff = 1;
  for (i = 0; i < 6; i++)
    jt[i].SetTarget(ang.VRef(i), rates.VRef(i));
  return 1;
}


//= Make sure arm is close to body by setting innermost joint angles.
// rate is ramping speed relative to standard reorientation speed
// negative rate does not scale acceleration (for snappier response)
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::Tuck (double rate, int bid)
{
  jhcMatrix ang(6);

  ang.Copy(ang0);
  ang.VSet(0, rets);
  ang.VSet(1, rete);
  return CfgTarget(ang, rate, bid);
}


//= Request both a finger position and gripper orientation (but not force).
// x is to right, y is forward, z is up
// rate is ramping speed relative to standard move speed
// negative rate does not scale acceleration (for snappier response)
// NOTE: coordinates relative to center of wheelbase and bottom of shelf
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::ArmTarget (const jhcMatrix& pos, const jhcMatrix& dir, double p_rate, double d_rate, int bid)
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
// negative rate does not scale acceleration (for snappier response)
// mode bits: 2 = exact Z, 1 = exact Y, 0 = exact X
// NOTE: coordinates relative to center of wheelbase and bottom of shelf
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::PosTarget (const jhcMatrix& pos, double rate, int bid, int mode)
{
  if (!pos.Vector(4))
    Fatal("Bad input to jhcEliArm::PosTarget");
  return PosTarget(pos.X(), pos.Y(), pos.Z(), rate, bid, mode);
}


//= Request a Cartesion finger grab point in discrete local arm coordinates.
// NOTE: coordinates relative to center of wheelbase and bottom of shelf
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::PosTarget (double ax, double ay, double az, double rate, int bid, int mode)
{
  // see if previous command takes precedence (trumps equal cfg)
  if ((bid <= plock) || (bid < alock))
    return 0;
  plock = bid;

  // set up command
  stiff = 1;
  pctrl.RampTarget(ax, ay, az, rate);
  pmode = mode;
  return 1;
}


//= Request a particular Cartesian finger position in global coordinates.
// converts to local arm coordinates by subtracting off height of shelf
// mode bits: 2 = exact Z, 1 = exact Y, 0 = exact X
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::PosTarget3D (const jhcMatrix& pos, double ht, double rate, int bid, int mode)
{
  if (!pos.Vector(4))
    Fatal("Bad input to jhcEliArm::PosTarget3D");

  // see if previous command takes precedence (trumps equal cfg)
  if ((bid <= plock) || (bid < alock))
    return 0;
  plock = bid;

  // set up command
  stiff = 1;
  pctrl.RampTarget(pos.X(), pos.Y(), pos.Z() - ht, rate);
  pmode = mode;
  return 1;
}


//= Request a particular Cartesian gripper orientation.
// x is to right, y is forward, z is up
// rate is ramping speed relative to standard reorientation speed
// negative rate does not scale acceleration (for snappier response)
// mode bits: 3 = any pan, 2 = exact roll, 1 = exact tilt, 0 = exact pan
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::DirTarget (const jhcMatrix& dir, double rate, int bid, int mode)
{
  if (!dir.Vector(4))
    Fatal("Bad input to jhcEliArm::DirTarget");

  // see if previous command takes precedence (trumps equal cfg)
  if ((bid <= dlock) || (bid < alock))
    return 0;
  dlock = bid;

  // set up command
  stiff = 1;
  dctrl.RampTarget(dir, rate);
  dmode = mode;
  return 1;
}


//= Request a finger position which is an offset from the current position.
// x is to right, y is forward, z is up
// rate is ramping speed relative to standard move speed
// negative rate does not scale acceleration (for snappier response)
// mode bits: 2 = exact Z, 1 = exact Y, 0 = exact X
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliArm::ShiftTarget (const jhcMatrix& dpos, double rate, int bid, int mode)
{
  jhcMatrix p2(4);

  if (!dpos.Vector(4))
    Fatal("Bad input to jhcEliArm::ShiftTarget");
  p2.AddVec(loc, dpos);
  return PosTarget(p2, rate, bid, mode);
}


//= Bring hand to rest using limited deceleration (no sudden jerk).

int jhcEliArm::ArmStop (double rate, int bid)
{
  jhcMatrix pos(4), dir(4);

  pctrl.SoftStop(pos, loc, 0.25, rate);
  dctrl.SoftStop(dir, aim, 5.0, rate);
  return ArmTarget(pos, dir, rate, rate, bid);
}


///////////////////////////////////////////////////////////////////////////
//                        ARM - Motion Progress                          //
///////////////////////////////////////////////////////////////////////////

//= Computes difference of current configuration from given joint angles.
// can optionally record absolute value of component errors instead

void jhcEliArm::CfgErr (jhcMatrix& aerr, const jhcMatrix& ang, int abs) const
{
  double diff;
  int i;

  if (!aerr.Vector(6) || !ang.Vector(6))
    Fatal("Bad input to jhcEliArm::CfgErr");

  for (i = 0; i < 6; i++)
  {
    diff = jt[i].CycNorm(ang.VRef(i)) - ang0.VRef(i);
    while (diff > 180.0)
      diff -= 360.0;
    while (diff <= -180.)
      diff += 360.0;   
    aerr.VSet(i, ((abs > 0) ? fabs(diff) : diff));
  } 
}


//= Determine max absolute angular error from shoulder-elbow tucked pose.

double jhcEliArm::TuckErr () const
{
  double ds = fabs(ang0.VRef(0) - rets), de = fabs(ang0.VRef(1) - rete);
 
  return __max(ds, de);
}


//= Computes difference from given goal in all components of pose.
// first is XYZ position of end point, second is pan-tilt-roll 
// can optionally save absolute value of component errors instead

void jhcEliArm::ArmErr (jhcMatrix& perr, jhcMatrix& derr, 
                        const jhcMatrix& pos, const jhcMatrix& dir, int abs) const
{
  PosErr(perr, pos, abs);
  DirErr(derr, dir, abs);
}


//= Computes difference from given local coordinate goal position in x, y, and z.
// can optionally save absolute value of component errors
// returns max of absolute difference across all coordinates

double jhcEliArm::PosErr (jhcMatrix& perr, const jhcMatrix& pos, int abs) const
{
  if (!perr.Vector(4) || !pos.Vector(4))
    Fatal("Bad input to jhcEliArm::PosErr");

  perr.DiffVec3(loc, pos);  
  if (abs > 0)
    perr.Abs();
  return perr.MaxAbs3();
}


//= Computes difference from given global coordinate goal position in x, y, and z.
// needs height of shelf to correct arm z value
// can optionally save absolute value of component errors
// returns max of absolute differences across all coordinates

double jhcEliArm::PosErr3D (jhcMatrix& perr, const jhcMatrix& pos, double ht, int abs) const
{
  if (!perr.Vector(4) || !pos.Vector(4))
    Fatal("Bad input to jhcEliArm::PosErr3D");

  perr.DiffVec3(loc, pos);  
  perr.IncZ(ht);
  if (abs > 0)
    perr.Abs();
  return perr.MaxAbs3();
}


//= Computes Cartesian distance from given global coordinate goal to nominal hand point.
// needs height of shelf to correct arm z value

double jhcEliArm::PosOffset3D (const jhcMatrix& pos, double ht) const
{
  double dx, dy, dz;

  if (!pos.Vector(4))
    Fatal("Bad input to jhcEliArm::PosOffset3D");

  dx = loc.X() - pos.X();
  dy = loc.Y() - pos.Y();
  dz = (loc.Z() + ht) - pos.Z();
  return sqrt(dx * dx + dy * dy + dz * dz);
}


//= Computes difference from given goal orientation in pan, tilt, and roll.
// can optionally save absolute value of component errors instead
// returns max of absolute differences across all pose angles

double jhcEliArm::DirErr (jhcMatrix& derr, const jhcMatrix& dir, int abs) const
{
  if (!derr.Vector(4) || !dir.Vector(4))
    Fatal("Bad input to jhcEliArm::DirErr");

  derr.DiffVec3(aim, dir);  
  derr.CycNorm3();
  if (abs > 0)
    derr.Abs();
  return derr.MaxAbs3();
}


//= Computes the deviation of gripper pan angle from desired value.
// can optionally return absolute value

double jhcEliArm::PanErr (double pan, int abs) const
{
  double diff = pan - aim.P();

  if (diff > 180.0)
    diff -= 360.0;
  while (diff <= -180.0)
    diff += 360.0;
  return((abs > 0) ? fabs(diff) : diff);
}


//= Returns maximum absolute angular error over all joints wrt given angles.
// answer is in degrees

double jhcEliArm::CfgOffset (const jhcMatrix& ang) const
{
  jhcMatrix aerr(6);

  if (!ang.Vector(6))
    Fatal("Bad input to jhcEliArm::CfgOffset");

  CfgErr(aerr, ang, 1);
  return aerr.MaxVec();
}


///////////////////////////////////////////////////////////////////////////
//                      ARM - Goal Characteristics                       //
///////////////////////////////////////////////////////////////////////////

//= Tell if arm can achieve given configuration of joint angles.
// returns 1 if ok, negative value gives max angular error

double jhcEliArm::Feasible (const jhcMatrix& ang) const
{
  double a, err, worst = 0.0;
  int i;

  if (!ang.Vector(6))
    Fatal("Bad input to jhcEliArm::Feasible");

  for (i = 0; i < 6; i++)
  {
    a = ang.VRef(i);
    err = fabs(jt[i].CtrlErr(a));
    worst = __min(-err, worst);
  }
  return worst;
}


//= Tell if arm can achieve this configuration (at least approximately).
// if "from" > 0 then starts with current pose (should call Update first)
// returns negative if bad, zero or positive if good

double jhcEliArm::Reachable (const jhcMatrix& pos, const jhcMatrix& dir, double qlim, int from)
{ 
  jhcMatrix ang0(7), ang(7);
  double q;

  if (!pos.Vector(4) || !dir.Vector(4))
    Fatal("Bad input to jhcEliArm::Reachable");

  // get starting configuration
  ang0.Zero();
  if (from > 0)
    get_angles(ang0);

  // compute inverse kinematic solution and check altered target
  q = pick_angles(ang, pos, dir, Width(), &ang0, 0);
  if (q > qlim)
    return -q;
  return q;
}


//= Estimate time (in secs) to reach goal configurtion with common rate.
// assumes arm is currently at zero velocity (i.e. move start)
// negative rate does not scale acceleration (for snappier response)
// timeout should be about 1.5x this

double jhcEliArm::CfgTime (const jhcMatrix& ang2, const jhcMatrix& ang1, double rate) const
{
  jhcMatrix rates(6);

  rates.FillVec(rate);
  return CfgTime(ang2, ang1, rates);
}


//= Estimate time (in secs) to reach goal configurtion with individual rates.
// assumes arm is currently at zero velocity (i.e. move start)
// negative rate does not scale acceleration (for snappier response)
// timeout should be about 1.5x this

double jhcEliArm::CfgTime (const jhcMatrix& ang2, const jhcMatrix& ang1, const jhcMatrix& rates) const
{
  double a2, a1, t, move = 0.0;
  int i;

  if (!ang2.Vector(6) || !ang1.Vector(6) || !rates.Vector(6))
    Fatal("Bad input to jhcEliArm::CfgTime");

  // find longest time required by any joint     
  for (i = 0; i < 6; i++)
  {
    a2 = jt[i].CycNorm(ang2.VRef(i));
    a1 = jt[i].CycNorm(ang1.VRef(i));
    t = jt[i].RampTime(a2, a1, rates.VRef(i));
    move = __max(t, move);
  }
  return move;
}


//= Estimate time (in seconds) to reach full pose target.
// x is to right, y is forward, z is up
// assumes arm is currently at zero velocity (i.e. move start)
// negative rate does not scale acceleration (for snappier response)
// timeout should be about 1.5x this
// NOTE: coordinates relative to center of wheelbase and bottom of shelf

double jhcEliArm::ArmTime (const jhcMatrix& pos2, const jhcMatrix& dir2, const jhcMatrix& pos1, const jhcMatrix& dir1, 
                           double p_rate, double d_rate) const
{
  double mt, rt, r = ((d_rate != 0.0) ? d_rate : p_rate);

  mt = PosTime(pos2, pos1, p_rate);
  rt = DirTime(dir2, dir1, r);
  return __max(mt, rt);
}


//= Estimate time (in seconds) to shift finger position by given vector.
// x is to right, y is forward, z is up
// assumes arm is currently at zero velocity (i.e. move start)
// negative rate does not scale acceleration (for snappier response)
// timeout should be about 1.5x this

double jhcEliArm::ShiftTime (const jhcMatrix& dpos, const jhcMatrix& pos0, double rate) const
{
  jhcMatrix pos2(4);

  if (!dpos.Vector(4))
    Fatal("Bad input to jhcEliArm::ShiftTime");
  pos2.AddVec3(pos0, dpos);
  return pctrl.RampTime(pos2, pos0, rate);
}


///////////////////////////////////////////////////////////////////////////
//                        ARM - Motion Coordination                      //
///////////////////////////////////////////////////////////////////////////

//= Gives overall rate to ensure change from angles ang1 to ang2 in given time.
// if secs < 0 then does not scale acceleration (for snappier response)
// based on trapezoidal profile from stopped to stopped

double jhcEliArm::CfgRate (const jhcMatrix& ang2, const jhcMatrix& ang1, double secs) const
{
  double a2, a1, r, rate = 0.0;
  int i;

  if (!ang2.Vector(6) || !ang1.Vector(6))
    Fatal("Bad input to jhcEliArm::CfgRate");
  
  for (i = 0; i < 6; i++)
  {
    a2 = jt[i].CycNorm(ang2.VRef(i));
    a1 = jt[i].CycNorm(ang1.VRef(i));
    r = jt[i].RampRate(a2, a1, secs);
    rate = __max(r, rate);
  }
  return rate;
}


//= Gives individual rates to achieve change from angles ang1 to ang2 in given time.
// if secs < 0 then does not scale acceleration (for snappier response)
// based on trapezoidal profile from stopped to stopped

void jhcEliArm::CfgRate (jhcMatrix& rates, const jhcMatrix& ang2, const jhcMatrix& ang1, double secs) const
{
  double a2, a1;
  int i;

  if (!rates.Vector(6) || !ang2.Vector(6) || !ang1.Vector(6))
    Fatal("Bad input to jhcEliArm::CfgRate");
  
  for (i = 0; i < 6; i++)
  {
    a2 = jt[i].CycNorm(ang2.VRef(i));
    a1 = jt[i].CycNorm(ang1.VRef(i));
    rates.VSet(i, jt[i].RampRate(a2, a1, secs));
  }
}


//= Gives rate to achieve change from arm pose from pos1/dir1 to pos2/dir2 in given time.
// if secs < 0 then does not scale acceleration (for snappier response)
// based on trapezoidal profile from stopped to stopped

double jhcEliArm::ArmRate (const jhcMatrix& pos2, const jhcMatrix& dir2, 
                           const jhcMatrix& pos1, const jhcMatrix& dir1, double secs) const
{
  double prate, drate;

  if (!pos2.Vector(4) || !dir2.Vector(4) || !pos1.Vector(4) || !dir1.Vector(4))
    Fatal("Bad input to jhcEliArm::ArmRate");

  prate = PosRate(pos2, pos1, secs);
  drate = DirRate(dir2, dir1, secs);
  if (fabs(drate) > fabs(prate))
    return drate;
  return prate;
}


///////////////////////////////////////////////////////////////////////////
//                           Joint Status                                //
///////////////////////////////////////////////////////////////////////////

//= Get rough position of joint's servo (largely for graphics).
// assumes joint matrices have already been updated with get_pose

int jhcEliArm::JtPos (jhcMatrix& pos, int n) const
{
  const jhcMatrix *zdir, *orig;

  if (!pos.Vector(4) || (n < 0) || (n > 6))
    Fatal("Bad input to jhcEliArm::JtPos");

  zdir = jt[n].AxisZ();
  orig = jt[n].Axis0();
  pos.ScaleVec3(*zdir, jt[n].NextZ(), 1.0);
  pos.IncVec3(*orig);
  return 1;
}


//= Find global position of lift axis shifted laterally to center of forearm link.
// side is offset wrt original joint point (neg = lf, pos = rt)
// looks better for arm skeleton and to get forearm tilt

void jhcEliArm::LiftBase (jhcMatrix& pos, double side) const
{
  jhcMatrix off(4);

  if (!pos.Vector(4))
    Fatal("Bad input to jhcEliArm::LiftBase");
  off.SetVec3(0.0, -side, 0.0);
  jt[2].GlobalMap(pos, off);
}


//= Give the planar xy direction of the forearm link.

double jhcEliArm::Forearm () const
{
  jhcMatrix lift(4), wrist(4);

  LiftBase(lift);
  JtPos(wrist, 3);
  return wrist.PanRel3(lift);
}


///////////////////////////////////////////////////////////////////////////
//                              Calibration                              //
///////////////////////////////////////////////////////////////////////////

//= Recalibrates gripper by setting zero degrees as full closed.
// Note: BLOCKS until a good amount of force is exerted

int jhcEliArm::ZeroGrip (int always)
{
  jhcJoint *g = jt + 6;
  double z, z0 = g->Ang0(), a0 = g->MinAng(), a1 = g->MaxAng();
  double lo = 0.3, hi = 0.4, dps = 30.0, adj = 10.0; 
  int k, i = 0, j = 0, fwd = 100, back = 20, time = 33;

  // skip if done recently else check hardware
  if ((always <= 0) && (gcal > 0))
    return 1;
  if (aok <= 0)
    return -1;

  // do coarse then fine adjustment
  for (k = 0; k < 2; k++)
  {
    // erase zero and widen motion limits
    g->SetRange(150.0, 0.0, -150.0);
    g->Reset();

    // start closing movement then wait for high force
    for (i = 0; i < fwd; i++)
    {
      // continue closing
      g->SetAngle(-150.0, dps);
      jms_sleep(time);

      // check current force
      g->GetState();
      if ((i > 2) && (g->Torque(-1.0) >= hi))
        break;
    }

    // start backing off slowly until low force
    for (j = 0; j < back; j++)
    {
      // continue opening
      g->SetAngle(150.0, dps);
      jms_sleep(time);

      // check current force
      g->GetState();
      if (g->Torque(-1.0) < lo)
        break;
    }
  }

  // stop all motion and then get current position 
  g->Limp();
  jms_sleep(time);
  g->GetState();
  z = g->Angle();

  // mark if successful, else revert to previous zero
  if ((i < fwd) && (j < back) && (fabs(z - z0) <= adj))
    gcal = 1;                                        
  else
    z = z0;                                          

  // set joint zero and restore old angle limits
  g->SetRange(a1, z, a0);
  g->Reset();
  return 1;
}


//= Adjusts the offset between the two lift servos so the load is evenly shared.
// Note: BLOCKS until balance achieved

int jhcEliArm::ShareLift (int always)
{
  jhcJoint *elb = jt + 1, *lf = jt + 2;
  double flex = 45.0, out = 60.0, dps = 60.0, tol = 2.0;
  double bal, f, lo = 0.1, hi = 0.2, inc = 0.2; 
  int i, ok = 0, wait = 100, done = 8, time = 33;

  // skip if done recently else check hardware
  if ((always <= 0) && (share > 0))
    return 1;
  if (aok <= 0)
    return -1;

  // extend elbow to avoid hitting base
  Limp();
  elb->SetAngle(flex, dps);

  // make lift joint stick straight out
  lf->SetAngle(out, dps);
  for (i = 0; i < wait; i++)
  {
    jms_sleep(time);
    lf->GetState();
    if (fabs(out - lf->Angle()) <= tol)
      break;
  }

  // check servo forces
  for (i = 0; i < wait; i++)
  {
    // reiterate command (possibly changed offset)
    lf->SetAngle(out, dps);
    jms_sleep(time);
    lf->GetState();

    // see if forces still unbalanced
    bal = lf->AdjBal(inc, lo, hi);
    f = fabs(bal);
    if ((f > lo) && (f < hi))
      ok++;
    else
      ok = 0;

    // stable adjustment found
    if (ok >= done)
      return 1;
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                           Blocking Routines                           //
///////////////////////////////////////////////////////////////////////////

//= Close gripper on object while remaining in the same position and orientation.
// returns -2 if broken, -1 if fully closed, 0 if timeout, 1 if successful
// Note: BLOCKS until a good amount of force is exerted

int jhcEliArm::Grab (double fhold)
{
double fwait = 2.0;
int stab = 5, ms = 33;
  jhcMatrix pos(4), dir(4);
  int n = ROUND(fwait / (0.001 * ms)), i = 0, gcnt = 0, ok = 1;

  // check hardware 
  if (Update() <= 0)
    return -2;
  ArmPose(pos, dir);

  // wait until high enough force or fingers all the way closed
  WidthTarget(-0.5);
  while ((Squeeze() < 8.0) || !WidthStop())
  {
    // reiterate command (only one finger moves so arm must compensate)
    ArmTarget(pos, dir);
    WidthTarget(-0.5);

    // move arm a little more
    Issue(0.001 * ms);
    jms_sleep(ms);
    Update();
  }

  // wait for force to stablilize
  SqueezeTarget(fhold);
  if (ok > 0)
    while (gcnt < stab)
    {
      // count consecutive intervals of proper force
      if (!SqueezeClose())
        gcnt = 0;
      else 
        gcnt++;

      // check for timeout
      if (i++ >= n)
      {
        ok = 0;
        break;
      }

      // reiterate command (only one finger moves so arm must compensate)
      ArmTarget(pos, dir);
      SqueezeTarget(fhold);

      // move arm a little more
      Issue(0.001 * ms);
      jms_sleep(ms);
      Update();
    }

  // command done
  Freeze();
  return ok;
}


//= Opens hand fully while maintaining position and orientation.
// returns -2 if broken, 0 if timed out, 1 if successful

int jhcEliArm::Drop ()
{
  jhcMatrix pos(4), dir(4);
  double open = MaxWidth();
  int ok = 1, ms = 33;

  // check hardware 
  if (Update() <= 0)
    return -2;
  ArmPose(pos, dir);   

  // wait until fingers all the way open
  WidthTarget(open);
  while (!WidthClose())
  {
    // reiterate command (only one finger moves so arm must compensate)
    ArmTarget(pos, dir);
    WidthTarget(open);

    // move arm a little more
    Issue(0.001 * ms);
    jms_sleep(ms);
    Update();
  }

  // command done
  Freeze();
  return ok;
} 


//= Set arm to some particular angular configuration.
// can also ask for some gripper width if 7th element of "ang"
// Note: BLOCKS until goal achieved or times out

int jhcEliArm::SetConfig (const jhcMatrix& ang, double rate)
{
  double quit = 2.0;                   // was 5 secs
  UL32 start = jms_now();
  int n = ang.Rows(), ok = 1, ms = 33;

  // check arguments then hardware 
  if (n < 6)
    return Fatal("Bad input to jhcEliArm::SetConfig");
  if (Update() <= 0)
    return -1;

  // wait until all the angles are close
  CfgTarget(ang);
  if (n >= 7)
    WidthTarget(ang.VRef(6));
  while ((CfgOffset(ang) > align) || ((n >= 7) && !WidthClose()))
  {
    // check for timeout
    if (jms_elapsed(start) > quit)
    {
      ok = 0;
      break;
    }

    // reissue command
    CfgTarget(ang);
    if (n >= 7)
      WidthTarget(ang.VRef(6));

    // move arm a little more
    Issue(0.001 * ms);
    jms_sleep(ms);
    Update();
  }

  // command done
  CfgClear();
  if (n >= 7)
    HandClear();
  Freeze();
  return ok;
}


//= Tucks the arm in suitably for travel.
// Note: BLOCKS until goal achieved or times out

int jhcEliArm::Stow (int fix)
{
  jhcMatrix end(4), dir(4), cfg(7);
  int ans = 1;

  // get arm into roughly normal state
  if (fix > 0)
    Untwist();

  // move hand to retracted position
  end.SetVec3(retx, rety, retz);        
  dir.SetVec3(rdir, rtip, 0.0, 0.0);
  if (Reach(end, dir, rgap) <= 0)
    ans = 0;

  // make sure elbow is tight to chest
  ArmConfig(cfg);
  cfg.VSet(0, rets);
  cfg.VSet(1, rete);
  if (SetConfig(cfg) <= 0)
    ans = 0;
  return ans;
}


//= Get the arm back into a semi-standard set of joint angles.
// alters angles to canonical values one at a time in some order

void jhcEliArm::Untwist ()
{
  // joint num:       S0    E1    L2    R3    P4    T5    W6
  double canon[7] = {10.0, 70.0, 45.0,  0.0,  0.0, 45.0,  0.5};  
  int seq[7] = {2, 5, 6, 4, 3, 1, 0};
  jhcMatrix cfg(7);
  int i, j;

  Update();
  ArmConfig(cfg);
  cfg.VSet(6, Width());
  for (i = 0; i < 7; i++)
  {
    j = seq[i];
    cfg.VSet(j, canon[j]);
    SetConfig(cfg);
  }
}


//= Go to specified pose using profiled motion (freezes at end).
// if width < 0 then holds object with standard force
// returns -2 if broken, -1 if not possible, 0 if timed out, 1 if successful
// Note: BLOCKS until goal achieved or times out

int jhcEliArm::Reach (const jhcMatrix& pos, const jhcMatrix& dir, double wid,  
                      double qlim, double inxy, double inz, double degs)
{
  double quit = 5.0;
  UL32 start = jms_now();
  int ok = 1, ms = 33;

  // check arguments then hardware and feasibility
  if (!pos.Vector(4) || !dir.Vector(4))
    return Fatal("Bad input to jhcEliArm::Reach");
  if (Update() <= 0)
    return -2;
  if (Reachable(pos, dir, qlim) < 0.0)
    return -1;

  // set up target
  ArmTarget(pos, dir);
  if (wid < 0.0)
    SqueezeTarget(fhold);
  else
    WidthTarget(wid);

  // keep reasserting command until success or failure
  while (!ArmClose() || !HandClose(wid))
  {
    // check for timeout
    if (jms_elapsed(start) > quit)
    {
      jprintf(">>> More than %3.1f secs in jhcEliArm::Reach !\n", quit);
      ok = 0;
      break;
    }

    // reiterate command to override default
    ArmTarget(pos, dir);
    if (wid < 0.0)
      SqueezeTarget(fhold);
    else
      WidthTarget(wid);

    // move arm a little more
    Issue(0.001 * ms);
    jms_sleep(ms);
    Update();
  }

  // command done
  ArmClear();
  HandClear();
  Freeze();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                             Debugging Tools                           //
///////////////////////////////////////////////////////////////////////////

//= Continuously reports position of gripper or some other joint.
// useful for calibrating angle offsets of first 3 servos

void jhcEliArm::JointLoop (int n, int once)
{
  jhcMatrix ang(7);
  const jhcMatrix *pos, *prev;
  char v1[40], v2[40];
  int prt = 100;

  if ((n < 0) || (n > 6))
    return;

  while (_kbhit())
    _getch();
  if (once <= 0)
    jprintf("Hit any key to exit ...\n\n");

  while (1)
  {
    // get joint data
    Update();
    get_angles(ang);
    pos = jt[n].Axis0();

    // display position
    if (n == 0)
      jprintf("%s = %s [%4.0f] \r", jt[n].name, pos->ListVec3(v1, "%5.1f", 0, 40), ang.VRef(n));
    else
    {
      prev = jt[n - 1].Axis0();
      jprintf("%s\t = %s [%4.0f]", jt[n].name, pos->ListVec3(v1, "%5.1f", 0, 40), ang.VRef(n));
      if (once <= 0)
        jprintf(" from %c = %s [%4.0f]\r", JtChar(n - 1), prev->ListVec3(v2, "%5.1f", 0, 40), ang.VRef(n - 1));           
    }

    // wait for next loop
    if ((once > 0) || _kbhit())
      break;
    jms_sleep(prt);
  }
  jprintf("\n");
}


// Continously reports position, orientation and width of gripper.
// useful for calibrating angle offsets of last 4 servos

void jhcEliArm::FingerLoop ()
{
  jhcMatrix pos(4), dir(4), ang(7);
  char v1[40], v2[40];
  int prt = 100;

  jprintf("Finger crease (hit any key to exit) ...\n\n");
  FingerTool(dpad);
  while (_kbhit())
    _getch();

  while (!_kbhit())
  {
    Update();
    ArmPose(pos, dir);
    jprintf("  %s with dir %s [%4.1f] \r", 
            pos.ListVec3(v1, "%4.2f", 0, 40), dir.ListVec3(v2, "%4.2f", 0, 40), 0.0); //ang.VRef(6));
    jms_sleep(prt);
  }
  jprintf("\n\n");
}


