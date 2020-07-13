// jhcCeliaNeck.cpp : control of Celia robot's head pan and tilt actuators
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

#include "Interface/jhcMessage.h"

#include "Body/jhcCeliaNeck.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcCeliaNeck::jhcCeliaNeck ()
{
  // attach joints to controller
  jt[0].Bind(&dyn);
  jt[1].Bind(&dyn);
  nok = -1;

  // set up description of joints
  strcpy_s(jt[0].group, "neck");
  strcpy_s(jt[0].name, "Pan");
  jt[0].jnum = 0;
  strcpy_s(jt[1].group, "neck");
  strcpy_s(jt[1].name, "Tilt");
  jt[1].jnum = 1;

  // load specialized arm geometry (in case no config file)
  std_geom();

  // get standard processing values
  Defaults();
  beam  = 0;
  plock = 0;
  tlock = 0;
}


//= Default destructor does necessary cleanup.

jhcCeliaNeck::~jhcCeliaNeck ()
{
  Limp();
}


//= Reset state for the beginning of a sequence.
// if noisy > 0 then prints to log file
// generally nok: -1 = no port, 0 = comm error, 1 = fine

int jhcCeliaNeck::NeckReset (int noisy, int chk)
{
  double p = 0.0, t = 0.0;

  // announce entry
  if (noisy > 0)
    jprintf("\nNeck reset ...\n");
  beam  = 0;
  plock = 0;
  tlock = 0;

  // set up geometry even if servos not accessible
  jt[0].InitGeom();
  jt[1].InitGeom();

  // connect to proper serial port (if needed) 
  if (nok < 0)
  {
    if (dyn.SetSource(dport, dbaud, 256) > 0)
      nok = 1;
    else if (noisy > 0)
      Complain("Could not open Dynamixel serial port %d in jhcCeliaNeck::Reset", dport);
  }
  dyn.Reset();
  nok = 1;

  // clear any servo errors (e.g. overload)
  if (noisy > 0)
    jprintf("  servo clear ...\n");
  if ((jt[0].Boot() <= 0) || (jt[1].Boot() <= 0))
  {
    nok = 0;
    return nok;
  }

  // possibly look for all servos
  if (chk > 0)
  {
    if (noisy > 0)
      jprintf("  servo check ...\n");
    Check(0, chk);
  }

  // initialize servos
  if (noisy > 0)
    jprintf("  servo init ...\n");
  if ((jt[0].Reset() <= 0) || (jt[1].Reset() <= 0))
  {
    nok = 0;
    return nok;
  }

  // stop any motion
  if (noisy > 0)
    jprintf("  limp ...\n");
  Limp();

  // initialize targets and positions
  if (noisy > 0)
    jprintf("  current angles ...\n");
  NeckUpdate();
  if (noisy > 0)
    jprintf("    pan %3.1f, tilt %3.1f\n", Pan0(), Tilt0());
  Stop(0);

  // finished
  if (noisy > 0)
    jprintf("    ** good **\n");
  return nok;
}


//= Check that all servos are responding.
// if noisy > 0 then pops dialog boxes for failed servos

int jhcCeliaNeck::Check (int noisy, int tries)
{
  int n, yack = 0;
  
  for (n = 1; n <= tries; n++)
  {
    // only potentially complain on last trial
    if ((noisy > 0) && (n >= tries))
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


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcCeliaNeck::Defaults (const char *fname)
{
  int ok = 1;

  ok &= neck_params(fname);
  ok &= LoadCal(fname);
  return ok;
}


//= Read just geometric calibration values from a file.

int jhcCeliaNeck::LoadCal (const char *fname)
{
  int ok = 1;

  ok &= pose_params(fname);
  ok &= jt[0].Defaults(fname);
  ok &= jt[1].Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcCeliaNeck::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= nps.SaveVals(fname);
  ok &= SaveCal(fname);
  return ok;
}


//= Write current geometric calibration values to a file.

int jhcCeliaNeck::SaveCal (const char *fname) const
{
  int ok = 1;

  ok &= pps.SaveVals(fname);
  ok &= jt[0].SaveVals(fname);
  ok &= jt[1].SaveVals(fname);
  return ok;
}


//= Parameters used for default head position.

int jhcCeliaNeck::pose_params (const char *fname)
{
  jhcParam *ps = &pps;
  int ok;

  ps->SetTag("neck_pose", 0);
  ps->NextSpecF( &cx0,  19.0, "Default sensor X (in)");
  ps->NextSpecF( &cy0, 128.5, "Default sensor Y (in)");
  ps->NextSpecF( &cz0,  71.5, "Default sensor Z (in)");
  ps->NextSpecF( &p0,  -48.0, "Default sensor pan (deg)");
  ps->NextSpecF( &t0,  -15.5, "Default sensor tilt (deg)");
  ps->NextSpecF( &r0,    0.0, "Default sensor roll (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for controlling neck movement.
// nothing geometric that differs between bodies

int jhcCeliaNeck::neck_params (const char *fname)
{
  jhcParam *ps = &nps;
  int ok;

  ps->SetTag("neck_move", 0);
  ps->NextSpecF( &nvlim,  90.0, "Slew speed (dps)");               // 1 sec for 90 degs
  ps->NextSpecF( &nacc,   90.0, "Slew acceleration (dps^2)");      // 2 sec to full speed
  ps->NextSpecF( &nlead,   4.0, "Pursuit lead factor");            // was 3
  ps->NextSpecF( &ndead,   0.5, "Target deadband (deg)");          // was 1
  ps->NextSpecF( &dps0,   90.0, "Blocking speed (dps)");
  ps->NextSpecF( &ndone,   1.0, "Blocking done test (deg)");

  ps->NextSpec4( &dport,       5, "Dynamixel serial port");  
  ps->NextSpec4( &dbaud, 1000000, "Dynamixel baud rate");          
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set up standard values describing the neck and camera geometry.
// pan bottom to tilt = 1.0" up, 3.8" forward
// tilt to camera = 0.5" left, 1.89" up and 0.5" forward (1.96" diag @ 14.8 degs)
// non-existent joint 2 is at color camera with z axis along line of sight
// pan axis bottom nz0 = 97.5" over floor, ny0 = 122.0" in front of entry door

void jhcCeliaNeck::std_geom ()
{
  // pan (servo direction reversed)
  jt[0].SetServo( -10,   0,     0.0, -135.0, 135.0,  90.0, 20.0, 0.031 );  // 20 for smoothness
  jt[0].SetGeom(    1.0, 3.8,   0.0,   90.0,   0.0                     );  
 
  // tilt (mid-range is pointed straight down)
  jt[1].SetServo(  11,   0,    90.0, -175.0,   0.0,  90.0, 10.0, 0.031 );  // 10 for gravity load
  jt[1].SetGeom(    0.5, 1.96, 75.2,   90.0,   0.0                     );  
}


///////////////////////////////////////////////////////////////////////////
//                       Angle Sensing Interface                         //
///////////////////////////////////////////////////////////////////////////

//= Update pan and tilt angles of the head by talking to servos.
// automatically resets "lock" for new bids

int jhcCeliaNeck::NeckUpdate ()
{
  // do main work
  nok = 1;
  if (jt[0].GetState() <= 0)
    nok = 0;
  if (jt[1].GetState() <= 0)
    nok = 0;

  // figure out shift in camera (use delayed angles)
  jt[0].SetMapping(jt[0].Previous(), NULL, cx0, cy0, cz0);
  jt[1].SetMapping(jt[1].Previous(), jt);

  // set up for next command
  plock = 0;
  tlock = 0;
  Stop(0);
  return nok;
}


//= Tell pan angle of the head, adjusting for upside down images.
// if tilt something like -160 degs, pan of 30 degs goes to 210 degs
// gives best guess of where images were acquired using delayed angle
// use Pan0 to get neck pan angle with no flipping or delay

double jhcCeliaNeck::Pan () const
{ 
  double p = jt[0].Previous(), t = jt[1].Previous();

  if (t >= -90.0)
    return p;
  return(p + 180.0);
}


//= Tell tilt angle of the head, adjusting for upside down images.
// tilt such as -175 degs goes to tilt -5 degs to match image
// gives best guess of where images were acquired using delayed angle
// use Tilt0 to get neck tilt angle with no flipping or delay

double jhcCeliaNeck::Tilt () const
{
  double t = jt[1].Previous();

  if (t >= -90.0)
    return t;
  return(-(t + 180.0));
}


//= Retrieve cached angles but return 0 if no communication.
// always adjusts pan and tilt for upside down images
// Note: leaves input values unchanged if no communication

int jhcCeliaNeck::Angles (double& p, double& t) const
{
  if (CommOK() <= 0)
    return 0;
  p = Pan();
  t = Tilt();
  return 1;
}


//= Compute world position of camera with current neck angles.

void jhcCeliaNeck::Position (double& cx, double& cy, double& cz) const
{
  jhcMatrix pos(4);

  Position(pos);
  cx = pos.X();
  cy = pos.Y();
  cz = pos.Z();
}


//= Compute position of camera as a vector.
// always adjusts pan and tilt for upside down images

void jhcCeliaNeck::Position (jhcMatrix& pos) const
{
  if (!pos.Vector(4))
    Fatal("Bad input to jhcCeliaNeck::Position");
  jt[1].End0(pos);
}


//= Compute position and true gazing angle of camera.
// applies forward kinematics with calibration parameters
// sometimes useful for registering depth maps properly
// always adjusts pan and tilt for upside down images

void jhcCeliaNeck::PoseVecs (jhcMatrix& pos, jhcMatrix& dir) const
{
  if (!pos.Vector(4) || !dir.Vector(4))
    Fatal("Bad input to jhcCeliaNeck::PoseVecs");
  Position(pos);
  dir.SetVec3(Pan(), Tilt(), r0);
}


//= Fill 6 element vector with PTRXYZ values.
// always adjusts pan and tilt for upside down images

void jhcCeliaNeck::Pose6 (double* p6) const
{
  jhcMatrix pos(4);
  
  p6[0] = Pan();
  p6[1] = Tilt();
  p6[2] = r0;
  Position(pos);
  pos.SetPos6(p6);
}


///////////////////////////////////////////////////////////////////////////
//                              Basic Commands                           //
///////////////////////////////////////////////////////////////////////////

//= Make neck stop in place with brakes on.

int jhcCeliaNeck::Freeze ()
{
  // do main work
  nok = 1;
  if (jt[0].Freeze() <= 0)
    nok = 0;
  if (jt[1].Freeze() <= 0)
    nok = 0;

  // no motion
  pvel = 0.0;
  tvel = 0.0;
  return nok;
}


//= Make neck stop and go passive (pushable).

int jhcCeliaNeck::Limp ()
{
  // do main work
  nok = 1;
  if (jt[0].Limp() <= 0)
    nok = 0;
  if (jt[1].Limp() <= 0)
    nok = 0;
  Laser(0);

  // no motion
  pvel = 0.0;
  tvel = 0.0;
  return nok;
}


//= Stay put at current orientation.
// returns 1 if new target accepted, 0 if ignored

int jhcCeliaNeck::Stop (int bid)
{
  GazeTarget(Pan0(), Tilt0(), dps0, bid);
  return nok;
}


///////////////////////////////////////////////////////////////////////////
//                            Profiled Motion                            //
///////////////////////////////////////////////////////////////////////////

//= Copy parameters for motion target pose and slew speed.
// bid value must be greater or same as previous command to take effect
// returns expected number of seconds for completion

double jhcCeliaNeck::GazeTarget (double pan, double tilt, double dps, int bid)
{
  double sp = __min(fabs(dps), nvlim);

  // see if previous command takes precedence
  if ((bid >= plock) && (bid >= tlock))
  {
    // record priority
    plock = bid;
    tlock = bid;

    // remember overall speed and goal pose
    pwin = norm_ang(pan);
    twin = norm_ang(tilt);
    psp = sp;
    tsp = sp;
  }
  return gaze_time(pan, tilt, sp);
}


//= Change only the desired pan angle.
// returns expected number of seconds for completion

double jhcCeliaNeck::PanTarget (double pan, double dps, int bid)
{
  double sp = __min(fabs(dps), nvlim);

  // see if previous command takes precedence
  if (bid >= plock)
  {
    plock = bid;
    psp = sp;
    pwin = norm_ang(pan);
  }
  return act_time(PanErr(pan), sp, nacc);
}


//= Change only the desired tilt angle.
// returns expected number of seconds for completion

double jhcCeliaNeck::TiltTarget (double tilt, double dps, int bid)
{
  double sp = __min(fabs(dps), nvlim);

  // see if previous command takes precedence
  if (bid >= tlock)
  {
    tlock = bid;
    tsp = sp;
    twin = norm_ang(tilt);
  }
  return act_time(TiltErr(tilt), sp, nacc);
}


//= Keep an angle in the range -180 to +180 degrees.

double jhcCeliaNeck::norm_ang (double degs) const
{
  double a = degs;

  while (a > 180.0)
    a -= 360.0;
  while (a <= -180.0)
    a += 360.0;
  return a;
}


//= Try to achieve target pose in approximately give time.
// sets speed automatically, nacc governs acceleration
// returns expected number of seconds for completion

double jhcCeliaNeck::GazeFix (double pan, double tilt, double secs, int bid)
{
  double pd = PanErr(pan), td = TiltErr(tilt);

  return GazeTarget(pan, tilt, __max(pd, td) / secs, bid);
}


//= Try to achieve pan target in given time.
// returns expected number of seconds for completion

double jhcCeliaNeck::PanFix (double pan, double secs, int bid)
{
  return PanTarget(pan, PanErr(pan) / secs, bid);
}


//= Try to achieve tilt target in given time.
// returns expected number of seconds for completion

double jhcCeliaNeck::TiltFix (double tilt, double secs, int bid)
{
  return TiltTarget(tilt, TiltErr(tilt) / secs, bid);
}


//= Move head in straight line with acceleration limit.
// takes typical call-back interval, automatically handles acceleration 
// if imm <= 0 then queues commands (e.g. for transmission with arm commands)
// NOTE: assumes NeckUpdate already called to get current orientation

int jhcCeliaNeck::NeckIssue (double tupd, int imm)
{
  double perr = Pan0() - pwin, terr = Tilt0() - twin;   // had norm_ang before
  double p, t, f, pcmd = psp, tcmd = tsp, tchk = nlead * tupd;
/*
  // stop motion if reasonably close to target 
  if (fabs(perr) <= ndead)
    perr = 0.0;
  if (fabs(terr) <= ndead)
    terr = 0.0;
*/
  // convert overall desired speed into speeds for individual axes
  if (perr == 0.0)
    pcmd = 0.0;
  else
  {
    f = fabs(terr / perr);
    if (f >= 1.0)
      pcmd /= f;
    else
      tcmd *= f;
  }

  // set pan and tilt velocities (signed) within acceleration limits
  pvel = alter_vel(pvel, perr, pcmd, nacc, tupd);
  tvel = alter_vel(tvel, terr, tcmd, nacc, tupd);

  // create local pan and tilt position goals
  p = Pan0() + pvel * tchk;
  p = jt[0].Clamp(p);
  t = Tilt0() + tvel * tchk;
  t = jt[1].Clamp(t);

  // issue command
  return servo_set(p, pvel, t, tvel, imm);
}


//= Set new actuator signed velocity within acceleration limits.
// does slow ramp up and down as well as for reversal, limits speed near goal
// err = position - target, cmd = desired max speed, acc = ramp limit

double jhcCeliaNeck::alter_vel (double vel, double err, double cmd, double acc, double tupd) const
{
  double inc = tupd * acc, derr = err + vel * tupd;    // likely error on next cycle

  // goal lies ahead so faster forward (unless close)
  if (derr < 0.0)
  {
    if ((vel >= sqrt(-2.0 * acc * derr)) || (vel > cmd))
      return(vel - __min(vel, inc));                
    return(vel + __min(cmd - vel, inc));        
  }

  // goal lies behind so faster backward (unless close)
  if (derr > 0.0)
  {
    if ((-vel >= sqrt(2.0 * acc * derr)) || (-vel > cmd))
      return (vel + __min(-vel, inc));               
    return(vel - __min(cmd + vel, inc));        
  }

  // already at goal so kill motion
  if (vel >= 0.0)                                 
    return(vel - __min(vel, inc));
  return(vel + __min(-vel, inc));
}


//= Set actual neck servo positions and speeds.
// if send <= 0 then queues commands 

int jhcCeliaNeck::servo_set (double p, double pv, double t, double tv, int send)
{
  int id[2];
  double pos[2], vel[2];
  int n;

  // make sure hardware is working
  if (nok < 0)
    return nok;
  nok = 1;

  // assemble and send basic command
  n =  jt[0].ServoCmd(id, pos, vel, 0, p, pv);
  n += jt[1].ServoCmd(id, pos, vel, n, t, tv);
  if (dyn.MultiPosVel(id, pos, vel, n, send) <= 0)
    nok = 0;
  return nok;
}


//= Estimate time (in seconds) to reach target.
// assumes robot is at rest initially

double jhcCeliaNeck::gaze_time (double pan, double tilt, double sp) const
{
  double f, perr = PanErr(pan), terr = TiltErr(tilt);
  double pt = 0.0, tt = 0.0, pcmd = sp, tcmd = sp;

  // convert overall speed into speeds for individual axes.
  if (perr == 0.0)
    pcmd = 0;
  else
  {
    f = terr / perr;
    if (f >= 1.0)
      pcmd /= f;
    else
      tcmd *= f;
  }

  // figure out completion times
  if (pcmd > 0.0)
    pt = act_time(perr, pcmd, nacc);
  if (tcmd > 0.0)
    tt = act_time(terr, tcmd, nacc);  
  return __max(pt, tt);
}


//= Returns approximate time to reach target from stop with vmax and acc.
// checks if triangular or trapezoidal velocity profile

double jhcCeliaNeck::act_time (double err, double vmax, double acc) const
{
  if (err <= ((vmax * vmax) / acc))
    return(2.0 * sqrt(err / acc));
  return((vmax / acc) + (err / vmax));
}


///////////////////////////////////////////////////////////////////////////
//                           Blocking Routines                           //
///////////////////////////////////////////////////////////////////////////

//= Sets goal position of neck and start move there.
// Note: does NOT block during movement

int jhcCeliaNeck::SlewNeck (double pan, double tilt, double dps)
{
  double p = jt[0].Clamp(pan), t = jt[1].Clamp(tilt);

  if (nok <= 0)
    return -1;
  servo_set(p, dps, t, dps, 1);
  return nok;
}


//= Send serial velocity command to neck servos.
// Note: BLOCKS until position is achieved 

int jhcCeliaNeck::SetNeck (double pan, double tilt)
{
  double secs;
  int i, tcnt, ms = 33;

  // check hardware then get expected motion duration
  if (nok <= 0)
    return -1;
  NeckUpdate();
  secs = GazeTarget(pan, tilt, dps0);
  tcnt = ROUND(1.5 * 1000.0 * secs / ms);
  
  // drive neck until timeout
  for (i = 0; i <= tcnt; i++)
  {
    // see if close enough yet
    if ((PanErr(pan) <= ndone) && (TiltErr(tilt) <= ndone))
      break;

    // reiterate command
    GazeTarget(pan, tilt, dps0);

    // change wheel speeds if needed then wait
    NeckIssue(0.001 * ms);
    Sleep(ms);
    NeckUpdate();
  }
    
  // stop base and report if timeout occurred
  Freeze();
  if (i >= tcnt)
    return 0;
  return 1;
}


//= Move head up and down (blocks).

int jhcCeliaNeck::Nod (double tilt)
{
  SetTilt(-tilt);
  SetTilt( tilt);
  SetTilt(-tilt);
  SetTilt(0.0);
  return nok;
}


//= Swivel head left and right (blocks).

int jhcCeliaNeck::Shake (double pan)
{
  SetPan( pan);
  SetPan(-pan);
  SetPan( pan);
  SetPan( 0.0);
  return nok;
}


//= Turn red laser either on or off.
// minimizes RTS transmissions by keeping track of state
// Note: frequent RTS calls seem to break servo link
// returns 1 if turned on, 0 if same, -1 if turned off

int jhcCeliaNeck::Laser (int red)
{
  if (nok <= 0)
    return 0;

  // possibly turn on
  if ((red > 0) && (beam <= 0)) 
  {
    dyn.SetRTS(1);
    beam = 1;
    return 1;
  }

  // possibly turn off
  if ((red <= 0) && (beam > 0))
  {
    dyn.SetRTS(0);
    beam = 0;
    return -1;
  }

  // no change needed
  return 0;
}


//= Blink laser rapidly a number of times (blocks)

int jhcCeliaNeck::Blink (int n)
{
  int i, osc = 67;

  if (nok <= 0)
    return 0;
  for (i = 0; i < 10; i++)
  {
    dyn.SetRTS(1);
    Sleep(osc);
    dyn.SetRTS(0);
    Sleep(osc);
  }
  return 1;
}
