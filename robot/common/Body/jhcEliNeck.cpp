// jhcEliNeck.cpp : control of Eli robot's head pan and tilt actuators
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

#include "Interface/jhcMessage.h"
#include "Interface/jms_x.h"

#include "Body/jhcEliNeck.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcEliNeck::~jhcEliNeck ()
{
  Freeze();           // use sp_grab to initialize Klepto
}


//= Default constructor initializes certain values.

jhcEliNeck::jhcEliNeck ()
{
  // cached pose
  pos0.SetSize(4);
  dir.SetSize(4);

  // set up description of joints
  strcpy_s(jt[0].group, "neck");
  strcpy_s(jt[0].name,  "Pan");
  jt[0].jnum = 0;
  strcpy_s(jt[1].group, "neck");
  strcpy_s(jt[1].name,  "Tilt");
  jt[1].jnum = 1;

  // no serial port for servos yet
  nok = -1;
  dyn = NULL;

  // profile generators
  strcpy_s(jt[0].rname, "pan_ramp");
  strcpy_s(jt[1].rname, "tilt_ramp");

  // motion control
  stiff = 0;
  clr_locks(1);

  // load specialized arm geometry (in case no config file)
  std_geom();

  // get standard processing values
  LoadCfg();
  Defaults();
  current_pose(pos0, dir);
}


//= Set up standard values describing the neck and camera geometry.
// pan top to tilt axis = 0.3" up, 1.8" forward along bracket
// camera 0.5" to right of tilt center, 1.9" up along bracket

void jhcEliNeck::std_geom ()
{
  // pan-to-tilt link: pan angle at base of link (x to right, y is forward)
  jt[0].SetServo(  10,   0,   10.0,  0.031, 90.0, 360.0, 360.0, -2.0 );
  jt[0].SetGeom(    0.3, 1.8, 90.0, 90.0,    0.0,   0.0, -70.0, 70.0 );  
 
  // tilt-to-cam link: tilt angle at base of link (x toward camera, y backward)
  jt[1].SetServo(  11,   0,   10.0,  0.031, 90.0, 360.0,  360.0, -2.0 );
  jt[1].SetGeom(    0.5, 1.9, 90.0,  0.0,    0.0,   0.0, -100.0, 35.0 );  
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for ramping jpoint positions.
// nothing geometric that differs between bodies

int jhcEliNeck::ramp_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("neck_ramp", 0);
  ps->NextSpecF( &(jt[0].vstd), "Pan std speed (deg/sec)");
  ps->NextSpecF( &(jt[0].astd), "Pan accel (deg^2/sec)");
  ps->NextSpecF( &(jt[0].dstd), "Pan decel (deg^2/sec)");
  ps->NextSpecF( &(jt[1].vstd), "Tilt std speed (deg/sec)");
  ps->NextSpecF( &(jt[1].astd), "Tilt accel (deg^2/sec)");
  ps->NextSpecF( &(jt[1].dstd), "Tilt decel (deg^2/sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for controlling neck movement.
// nothing geometric that differs between bodies

int jhcEliNeck::neck_params (const char *fname)
{
  jhcParam *ps = &nps;
  int ok;

  ps->SetTag("neck_move", 0);
  ps->NextSpecF( &gaze0, -5.0, "Initial head tilt (deg)");    // was -40
  ps->Skip();
  ps->NextSpecF( &ndone,  1.0, "Blocking gaze done test (deg)");
  ps->NextSpecF( &quit,   0.5, "Blocking move timeout (sec)");
  ps->NextSpec4( &ms,    33,   "Default condition check (ms)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters describing residual neck and camera geometry.
// pan axis = 0.25" to right of center, 5.6" in front of wheels 
// pan axis top = 25.4" over shelf bottom
// camera = 0.5" forward of tilt bracket

int jhcEliNeck::geom_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("neck_origin", 0);
  ps->NextSpecF( &nx0,   0.25, "Pan axis to right of wheels (in)");
  ps->NextSpecF( &ny0,   5.6,  "Pan axis in front of wheels (in)");
  ps->NextSpecF( &nz0,  25.4,  "Pan top above arm shelf (in)");
  ps->NextSpecF( &cfwd,  0.5,  "Camera in front of tilt (in)");  
  ps->Skip();
  ps->NextSpecF( &roll,  0.0,  "Camera roll (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Read all relevant defaults variable values from a file.

int jhcEliNeck::Defaults (const char *fname)
{
  int ok = 1;

  ok &= ramp_params(fname);
  ok &= neck_params(fname);
  ok &= jt[0].Defaults(fname);
  ok &= jt[1].Defaults(fname);
  return ok;
}


//= Read just body specific values from a file.

int jhcEliNeck::LoadCfg (const char *fname)
{
  int ok = 1;

  ok &= geom_params(fname);
  ok &= jt[0].LoadCfg(fname);
  ok &= jt[1].LoadCfg(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcEliNeck::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= rps.SaveVals(fname);
  ok &= nps.SaveVals(fname);
  ok &= jt[0].SaveVals(fname);
  ok &= jt[1].SaveVals(fname);
  return ok;
}


//= Write current body specific values to a file.

int jhcEliNeck::SaveCfg (const char *fname) const
{
  int ok = 1;

  ok &= gps.SaveVals(fname);
  ok &= jt[0].SaveCfg(fname);
  ok &= jt[1].SaveCfg(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Associate arm with some (possibly shared) Dyamixel interface.

void jhcEliNeck::Bind (jhcDynamixel *ctrl)
{
  jt[0].Bind(ctrl);
  jt[1].Bind(ctrl);
  dyn = ctrl;
  nok = 1;
  if (ctrl == NULL)
    nok = -1;
}


//= Reset state for the beginning of a sequence.
// if rpt > 0 then prints to log file
// generally nok: -1 = no port, 0 = comm error, 1 = fine

int jhcEliNeck::Reset (int rpt, int chk)
{
  double v;
  int pct;

  // announce entry
  if (rpt > 0)
    jprintf("\nNeck reset ...\n");
  clr_locks(1);
  GazeClear();

  // make sure hardware is working
  if (dyn == NULL)
  {
    nok = -1;
    return fail(rpt);
  }
  nok = 1;

  // clear any servo errors (e.g. overload)
  if (rpt > 0)
    jprintf("  servo clear ...\n");
  if ((jt[0].Boot() <= 0) || (jt[1].Boot() <= 0))
    return fail(rpt);

  if (chk > 0)
  {
    // possibly check supply voltage
    if (rpt > 0)
      jprintf("  battery ...\n");
    if ((v = Voltage()) <= 0.0)
      return fail(rpt);
    pct = Power(v);
    if (rpt > 0)
      jprintf("    %3.1f volts [%d pct]\n", v, pct);
    if (pct < 20)
    {
      if (rpt >= 2)
        Complain("jhcEliNeck - Low battery");
      else if (rpt > 0)
        jprintf(">>> jhcEliNeck - Low battery !\n");
    }

    // possibly look for all servos
    if (rpt > 0)
      jprintf("  servo check ...\n");
    Check(0, chk);
  }

  // initialize servos
  if (rpt > 0)
    jprintf("  servo init ...\n");
  if ((jt[0].Reset() <= 0) || (jt[1].Reset() <= 0))
    return fail(rpt);

  // stop any motion
  if (rpt > 0)
    jprintf("  freeze ...\n");
  Update();
  Freeze();

  // initialize targets and positions
  if (rpt > 0)
    jprintf("  current angles ...\n");
  Update();
  if (rpt > 0)
    jprintf("    pan %3.1f degs, tilt %3.1f degs\n", Pan(), Tilt());
  Freeze();

  // finished
  if (rpt > 0)
    jprintf("    ** good **\n");
  return nok;
}


//= Failure message for some part of initialization.

int jhcEliNeck::fail (int rpt) 
{
  if (nok > 0)
    nok = 0;
  if (rpt > 0)
    jprintf("    >> BAD <<\n");
  return nok;
}


//= Check that all servos are responding.
// if rpt > 0 then pops dialog boxes for failed servos

int jhcEliNeck::Check (int rpt, int tries)
{
  int n, yack = 0;
  
  // make sure hardware is working
  if (dyn == NULL)
  {
    nok = -1;
    return nok;
  }
  
  for (n = 1; n <= tries; n++)
  {
    // only potentially complain on last trial
    if ((rpt > 0) && (n >= tries))
      yack = 1;

    // see if any servo fails to respond
    nok = 1;
    if (jt[0].Check(yack) <= 0)
      nok = -1;
    else if (jt[1].Check(yack) <= 0)
      nok = -1;

    // everything is up and running
    if (nok > 0)
      break;
  }
  return nok;
}


//= Tells current voltage of main battery (to nearest 100mv).
// exchanges information with servo (i.e. takes time)

double jhcEliNeck::Voltage ()
{
  return jt[0].Battery();
}


//= Returns rough percentage charge of lead-acid battery.
// if called with 0 then talks to servos to find voltage first

int jhcEliNeck::Power (double vbat)
{
  double v = ((vbat > 0.0) ? vbat : Voltage());

  if ((v > 0.0) && (dyn != NULL))
    return dyn->Charge(v);
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                          Low Level Commands                           //
///////////////////////////////////////////////////////////////////////////

//= Make neck stop in place with brakes on.
// generally should call Update just before this
// if tupd > 0 then calls Issue after this
// NOTE: for continuing freeze set rate = 0 like ShiftTarget(0.0, 0.0, 0.0, 1000)

int jhcEliNeck::Freeze (int doit, double tupd)
{
  // tell ramp controllers to remember position
  if (doit <= 0)
    return nok;
  jt[0].rt = 0.0;
  jt[1].rt = 0.0;

  // possibly talk to servos
  stiff = 1;
  if (tupd > 0.0)
    Issue(tupd);
  return nok;
}


//= Make neck stop and go passive (pushable) and immediately talks to servos.
// for continuing limp set current position like ShiftTarget(0.0, 0.0, 1.0, 1000)
// NOTE: this is "freer" then recycling current position since motors are disabled

int jhcEliNeck::Limp ()
{
  int svo[2];
  int n = 0;

  // make sure hardware is working
  if ((nok < 0) || (dyn == NULL))
    return nok;
  nok = 1;

  // no motion
  stiff = 0;
  GazeClear();

  // tell servos to deactivate
  n += jt[0].ServoNums(svo, n);
  n += jt[1].ServoNums(svo, n);
  dyn->MultiLimp(svo, n);

  // make sure readings are up to date
  Update();
  jt[0].RampTarget(jt[0].Angle());
  jt[1].RampTarget(jt[1].Angle());
  return nok;
}


///////////////////////////////////////////////////////////////////////////
//                           Core Interation                             //
///////////////////////////////////////////////////////////////////////////

//= Update pan and tilt angles of the head by talking to servos.
// cached in member variables "pos0" (no lift compensation) and "dir"
// automatically resets "lock" for new bids

int jhcEliNeck::Update ()
{
  // make sure hardware is working
  if ((nok < 0) || (dyn == NULL))
    return nok;
  nok = 1;

  // do main work
  if (jt[0].GetState() <= 0)
    nok = 0;
  if (jt[1].GetState() <= 0)
    nok = 0;
  current_pose(pos0, dir);

  // set up for new target arbitration
  clr_locks(0);
  return nok;
}


//= Get relative position (XYZ) and direction (PTR) vectors based on joints.
// assumed joints are already updated

void jhcEliNeck::current_pose (jhcMatrix& xyz, jhcMatrix& ptr)
{
  jhcMatrix tool(4);

  // compute coordinate transform matrices
  jt[0].SetMapping(Pan(), NULL, nx0, ny0, nz0);
  jt[1].SetMapping(Tilt(), jt);

  // adjust for camera projection forward (y is reversed)
  tool.SetVec3(0.0, -cfwd, 0.0);
  jt[1].GlobalMap(xyz, tool);

  // make up an aiming vector
  ptr.SetVec3(Pan(), Tilt(), roll);
}


//= Clear winning command bids for all resources.
// can optionally clear previous bids also

void jhcEliNeck::clr_locks (int hist)
{
  // remember winning bid of last arbitration cycle
  plock0 = ((hist > 0) ? 0 : plock);
  tlock0 = ((hist > 0) ? 0 : tlock);

  // set up for new target arbitration
  plock = 0;
  tlock = 0;
}


//= Move head in straight line with acceleration limit.
// takes typical call-back interval, automatically handles acceleration 
// if send <= 0 then queues commands (e.g. for transmission with arm commands)
// NOTE: assumes Update already called to get current orientation

int jhcEliNeck::Issue (double tupd, double lead, int send)
{
  double p, pvel, t, tvel;

  // check for working communication and reasonable arguments
  if (nok < 0)
    return nok;
  if (tupd <= 0.0)
    Fatal("Bad input to jhcEliNeck::Issue");

  // check if neck is under active command
  if (stiff > 0)
  {
    // set default if no neck target specified
    Freeze((((plock <= 0) && (tlock <= 0)) ? 1 : 0), 0.0);

    // find next waypoint and speed along trajectory
    jt[0].RampNext(Pan(), tupd, lead);
    jt[1].RampNext(Tilt(), tupd, lead);

    // smoothest if given final stop, profiling used for accel/decel
    p = jt[0].RampCmd();
    t = jt[1].RampCmd();
    pvel = jt[0].RampVel();
    tvel = jt[1].RampVel();
    servo_set(p, pvel, t, tvel);
  }

  // send to servos
  if (send > 0)
    if (dyn->MultiSend() <= 0)
      nok = 0;
  return nok;
}


//= Set actual neck servo positions and speeds.

void jhcEliNeck::servo_set (double p, double pv, double t, double tv)
{
  int id[2];
  double pos[2], vel[2];
  int n;

  // make sure hardware is working
  if ((nok < 0) || (dyn == NULL))
    return;
  nok = 1;

  // assemble and add to big command packet
  n =  jt[0].ServoCmd(id, pos, vel, 0, p, pv);
  n += jt[1].ServoCmd(id, pos, vel, n, t, tv);
  dyn->MultiPosVel(id, pos, vel, n);
}


///////////////////////////////////////////////////////////////////////////
//                          Current Information                          //
///////////////////////////////////////////////////////////////////////////

//= Compute position and true gazing angle of camera.
// applies forward kinematics with calibration parameters
// sometimes useful for registering depth maps properly
// Y points forward, X to right, Z is upwards (origin = wheel midpoint)
// needs to be told height of bottom of lift stage separately

void jhcEliNeck::HeadPose (jhcMatrix& pos, jhcMatrix& aim, double lift) const
{
  if (!pos.Vector(4) || !aim.Vector(4))
    Fatal("Bad input to jhcEliNeck::HeadPose");
  pos.RelVec3(pos0, 0.0, 0.0, lift);
  aim.RelVec3(dir, 90.0, 0.0, 0.0);
}


//= Give full position of camera relative to midpoint of wheels on floor.
// Y points forward, X to right, Z is upwards (origin = wheel midpoint)
// needs to be told height of bottom of lift stage separately

void jhcEliNeck::HeadLoc (jhcMatrix& pos, double lift) const
{
  if (!pos.Vector(4))
    Fatal("Bad input to jhcEliNeck::HeadLoc");
  pos.RelVec3(pos0, 0.0, 0.0, lift);
}


//= Compute pan and tilt angles to center given target in camera.
// Y points forward, X to right, Z is upwards (origin = wheel midpoint)
// needs to be told height of bottom of lift stage separately

void jhcEliNeck::AimFor (double& p, double& t, const jhcMatrix& targ, double lift) const
{
  jhcMatrix cam(4);

  if (!targ.Vector(4))
    Fatal("Bad input to jhcEliNeck::AimFor");
  HeadLoc(cam, lift);
  cam.PanTilt3(p, t, targ);
  p -= 90.0;                           // forward = 90 degs
}


///////////////////////////////////////////////////////////////////////////
//                          Goal Specification                           //
///////////////////////////////////////////////////////////////////////////

//= Copy parameters for motion target pose and slew speed.
// if tilt rate = 0 then copies pan rate
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliNeck::GazeTarget (double pan, double tilt, double p_rate, double t_rate, int bid)
{
  double r = ((t_rate != 0.0) ? t_rate : p_rate);

  if ((bid <= plock) || (bid <= tlock))
    return 0;
  PanTarget(pan, p_rate, bid);
  TiltTarget(tilt, r, bid);
  return 1;
}


//= Change only the desired pan angle.
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliNeck::PanTarget (double pan, double rate, int bid)
{
  if (bid <= plock)
    return 0;
  plock = bid;
  stiff = 1;
  jt[0].SetTarget(pan, rate);
  return 1;
}


//= Change only the desired tilt angle.
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliNeck::TiltTarget (double tilt, double rate, int bid)
{
  if (bid <= tlock)
    return 0;
  tlock = bid;
  stiff = 1;
  jt[1].SetTarget(tilt, rate);
  return 1;
}


//= Set pan and tilt targets to look at given position.
// only approximate at start since head position changes
// does NOT coordinate pan and tilt speeds for straight arc

int jhcEliNeck::GazeAt (const jhcMatrix& targ, double lift, double rate, int bid)
{
  double pan, tilt;

  AimFor(pan, tilt, targ, lift);
  return GazeTarget(pan, tilt, rate, rate, bid);
}


///////////////////////////////////////////////////////////////////////////
//                           Motion Progress                             //
///////////////////////////////////////////////////////////////////////////

//= Return error (in degs) between current pan and goal angle.
// can optionally give absolute value and constrain pan to valid range

double jhcEliNeck::PanErr (double pan, int abs, int lim) const   
{
  double p = ((lim > 0) ? jt[0].Clamp(pan) : pan);
  double err = norm_ang(Pan() - p); 

  return((abs > 0) ? fabs(err) : err);
}


//= Return error (in degs) between current tilt and goal angle.
// can optionally give absolute value and constrain tilt to valid range

double jhcEliNeck::TiltErr (double tilt, int abs, int lim) const 
{
  double t = ((lim > 0) ? jt[1].Clamp(tilt) : tilt);
  double err = norm_ang(Tilt() - t); 
  
  return((abs > 0) ? fabs(err) : err);
}


//= Keep an angle in the range -180 to +180 degrees.

double jhcEliNeck::norm_ang (double degs) const
{
  double a = degs;

  if (a > 180.0)
    a -= 360.0 * ROUND(a / 360.0);
  else if (a <= -180.0)
    a += 360.0 * ROUND(a / 360.0);
  return a;
}


//= Gives the max absolute pan or tilt error between current gaze and target position.

double jhcEliNeck::GazeErr (const jhcMatrix& targ, double lift) const
{
  double pan, tilt;

  AimFor(pan, tilt, targ, lift);
  return GazeErr(pan, tilt);
}


///////////////////////////////////////////////////////////////////////////
//                           Neck Extras                                 //
///////////////////////////////////////////////////////////////////////////

//= Estimate time (in seconds) to reach target from given start position.
// if tilt rate = 0 then copies pan rate
// assumes neck is currently at zero velocity (i.e. move start)
// negative rate does not scale acceleration (for snappier response)
// timeout should be about 1.5x this

double jhcEliNeck::GazeTime (double p2, double t2, double p1, double t1, double p_rate, double t_rate) const
{
  double pt, tt, r = ((t_rate != 0.0) ? t_rate : p_rate);

  pt = PanTime(p2, p1, p_rate);
  tt = TiltTime(t2, t1, r);
  return __max(pt, tt);
}


//= Pick single rate to achieve reorientation within given time.
// if secs < 0 then does not scale acceleration (for snappier response)
// based on trapezoidal profile from stopped to stopped
// Note: use PanRate and TiltRate to get simultaneous completion

double jhcEliNeck::GazeRate (double p2, double t2, double p1, double t1, double secs, double rmax) const
{
  double pr, tr;
  
  pr = PanRate(p2, p1, secs, rmax);
  tr = TiltRate(t2, t1, secs, rmax);
  return __max(pr, tr);
}


///////////////////////////////////////////////////////////////////////////
//                           Blocking Routines                           //
///////////////////////////////////////////////////////////////////////////

//= Sets goal position of neck and start move there.
// Note: does NOT block during movement

int jhcEliNeck::SlewNeck (double pan, double tilt, double dps)
{
  double sp = __min(fabs(dps), __min(jt[0].vstd, jt[1].vstd));
  double p = jt[0].Clamp(pan), t = jt[1].Clamp(tilt);

  if (nok <= 0)
    return -1;
  servo_set(p, sp, t, sp);
  if (dyn->MultiSend() <= 0)
    nok = 0;
  return nok;
}


//= Send angular command to neck servos (blocks).

int jhcEliNeck::SetNeck (double pan, double tilt)
{
  // check hardware and get current pose 
  if (Update() <= 0)
    return -1;
  
  // drive neck until timeout
  while (1)
  {
    // reiterate command
    GazeTarget(pan, tilt);

    // change wheel speeds if needed then wait
    Issue(0.001 * ms);
    jms_sleep(ms);
    Update();

    // see if close enough yet
    if (GazeClose() || GazeFail())
      break;
  }
    
  // stop base and report if timeout occurred
  GazeClear();
  Freeze();
  return((GazeClose()) ? 1 : 0);
}


//= Move head up and down (blocks).

int jhcEliNeck::Nod (double tilt)
{
  SetTilt(-tilt);
  SetTilt( tilt);
  SetTilt(-tilt);
  SetTilt(0.0);
  return nok;
}


//= Swivel head left and right (blocks).

int jhcEliNeck::Shake (double pan)
{
  SetPan( pan);
  SetPan(-pan);
  SetPan( pan);
  SetPan( 0.0);
  return nok;
}

