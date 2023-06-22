// jhcJoint.cpp : one rotational DOF for a Dynamixel servo chain
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#include <stdio.h>
#include <math.h>

#include "Interface/jhcMessage.h"

#include "Geometry/jhcJoint.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcJoint::jhcJoint ()
{
  // not part of a group yet, no communication channel
  *name  = '\0';
  *group = '\0';
  jnum = 0;
  dyn = NULL;
  prev = 0.0;
  th   = 0.0;
  sv   = 0.0;
  f    = 0.0;
  th2  = 0.0;
  sv2  = 0.0;
  f2   = 0.0;

  // set forward kinematics matrix sizes and initial values
  dhm.SetSize(4, 4);
  fwd.SetSize(4, 4);
  fwd.Zero(1.0);

  // set geometry vector sizes
  orig.SetSize(4);
  xdir.SetSize(4);
  ydir.SetSize(4);
  zdir.SetSize(4);

  // fill in basic values and clear state
  vstd = 200.0;
  SetServo( 1,   0,   10.0, 0.031, 180.0, 180.0,  180.0,  -2.0 );
  SetGeom(  0.0, 7.0,  0.0, 0.0,     0.0,   0.0, -150.0, 150.0 );
  a0 = amin;
  a1 = amax;
  LoadCfg();
  Defaults(); 
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Initialize geometric transforms (and compute angular range of motion).

void jhcJoint::InitGeom ()
{
  rng = amax - amin;
  while (rng >= 360.0)
    rng -= 360.0;
  while (rng < 0.0)
    rng += 360.0;
  dh_matrix(0.0, 1);
}


//= Clear any errors that cause servos to shut down.

int jhcJoint::Boot (int chk)
{
  int ok = 1;

  if (dyn == NULL) 
    return -1;
  if (id != 0)
    if (dyn->Init(abs(id), chk) <= 0)
      ok = 0;
  if (id2 != 0)
    if (dyn->Init(abs(id2), chk) <= 0)
      ok = 0; 
  return ok;
}


//= Reset state for the beginning of a sequence.

int jhcJoint::Reset ()
{
  double lo, hi, lo2, hi2;

  // initialize kinematics matrix
  InitGeom();

  // get current servo values 
  if (dyn == NULL) 
    return -1;
  if (GetState() <= 0)
    return 0;
  prev = th;

  // find offset between servos (if two)
  off = 0.0;
  if (id2 != 0)
    off = th2 - th;

  // set up joint limits (must compute "off" first)
  ang_limits(lo, hi, lo2, hi2);
  if (id != 0)
    dyn->SetLims(abs(id), lo, hi);
  if (id2 != 0)
    dyn->SetLims(abs(id2), lo2, hi2);

  // set up joint springiness
  if (id != 0)
  {
    dyn->SetMargin(abs(id));
    dyn->SetSlope(abs(id), stiff, stiff);
    dyn->SetPunch(abs(id), step);
  }
  if (id2 != 0)
  {
    dyn->SetMargin(abs(id2));
    dyn->SetSlope(abs(id2), stiff, stiff);
    dyn->SetPunch(abs(id2), step);
  }

  // no trapezoidal profile in progress
  return 1;
}


//= Compute joint limits taking into account offset, calibration, and hard stops.
// also determines and saves command limits a0 and a1 (usually overkill)

void jhcJoint::ang_limits (double& bot, double& top, double& bot2, double& top2)
{
  double swap, inc = zero + cal, inc2 = inc + off;
  double lo = amin + inc, hi = amax + inc, lo2 = amin + inc2, hi2 = amax + inc2;

  // handle direction reversal
  if (id < 0)
  {
    swap = lo;
    lo = -hi;
    hi = -swap;
  }
  if (id2 < 0)
  {
    swap = lo2;
    lo2 = -hi2;
    hi2 = -swap;
  }

  // apply hard limits (potentiometer only valid in this range)
  lo  = __max(-150.0, lo);
  hi  = __min(hi, 150.0);
  lo2 = __max(-150.0, lo2);
  hi2 = __min(hi2, 150.0);

  // copy to output variables (finished)
  bot  = lo;
  top  = hi;
  bot2 = lo2;
  top2 = hi2;

  // convert back to command direction
  if (id < 0)
  {
    swap = lo;
    lo = -hi;
    hi = -swap;
  }
  if (id2 < 0)
  {
    swap = lo2;
    lo2 = -hi2;
    hi2 = -swap;
  }

  // convert back to command values
  lo  -= inc;
  hi  -= inc;
  lo2 -= inc2;
  hi2 -= inc2;

  // save tightest limits in each direction
  a0 = __max(lo, lo2);
  a1 = __min(hi, hi2);
}


//= Make sure that associated servos are connected to serial network.

int jhcJoint::Check (int noisy)
{
  int ok = 1;

  // check for serial network
  if (dyn == NULL)
  {
    if (noisy > 0)
      Complain("No serial network specified in jhcJoint::Check");
    return 0;
  }

  // check for main servo
  if (id != 0)
    if (dyn->Ping(abs(id)) <= 0)
    {
      if (noisy > 0)
        Complain("Could not communicate with servo %d in jhcJoint::Check", abs(id));
      ok = 0;
      return ok;
    }

  // check for secondary servo
  if (id2 != 0)
    if (dyn->Ping(abs(id2)) <= 0)
    {
      if (noisy > 0)
        Complain("Could not communicate with servo %d in jhcJoint::Check", abs(id2));
      ok = 0;
    }
  return ok;
}


//= Check supply voltage to joint servos (to nearest 100mv).
// returns the lowest value if several servos

double jhcJoint::Battery ()
{
  double v2, v = 0.0;

  if (dyn == NULL)
    return v;

  // read value at first servo
  if (id != 0)
    v = dyn->Voltage(abs(id));

  // read value at second servo (if any)
  if (id2 != 0)
  {
    v2 = dyn->Voltage(abs(id2));
    if (id != 0)
      v = __min(v, v2);
    else
      v = v2;
  }
  return v;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcJoint::Defaults (const char *fname)
{
  return servo_params(fname);
}


//= Read just body specific values from a file.

int jhcJoint::LoadCfg (const char *fname)
{
  return geom_params(fname);
}


//= Write current processing variable values to a file.

int jhcJoint::SaveVals (const char *fname) const
{
  return sps.SaveVals(fname);
}


//= Write current body specific values to a file.

int jhcJoint::SaveCfg (const char *fname) const
{
  return gps.SaveVals(fname);
}


//= Parameters used for basic servo control of joint.
// negative id or id2 means servo angle should be inverted
// if no secondary servo use id2 = 0

int jhcJoint::servo_params (const char *fname)
{
  jhcParam *ps = &sps;
  char tag[20];
  int ok;

  ps->SetTitle("%s servo", name);
  sprintf_s(tag, "%s_%csvo", group, tolower(name[0]));
  ps->SetTag(tag, 0);
  ps->NextSpec4( &id,    "Main servo ID (neg if rev)");
  ps->NextSpec4( &id2,   "Aux servo ID (neg if rev)");
  ps->NextSpecF( &stiff, "Compliance band (degs)");
  ps->NextSpecF( &step,  "Min error force (frac)");
  ps->NextSpecF( &vstd,  "Std rotation speed (dps)");      
  ps->NextSpecF( &astd,  "Std acceleration (dps^2)"); 
     
  ps->NextSpecF( &dstd,  "Std deceleration (dps^2)");
  ps->NextSpecF( &done,  "Min progress move (neg deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set servo parameters in same order as configuration file.

void jhcJoint::SetServo (int n, int n2, double st, double p, 
                         double v, double a, double d, double frac)
{
  // AX-12 servo parameters
  id    = n;
  id2   = n2;
  stiff = st;
  step  = p;

  // trapezoidal profiling and integral feedback
  vstd = v;
  astd = a;
  dstd = d;
  done = frac;
}


//= Parameters used for basic geometric interpretation of joint.
// see Wikipedia for an illustration of DH parameters
// describes a link with the articulated angle at its base
//  dhd = offset ALONG this axis (z) to origin of next axis
//  dhr = closest approach of next axis line to this axis line
//  dht = angle of closest approach line AROUND this axis (new x)
//  dha = rotation around closest approach line to align axes

int jhcJoint::geom_params (const char *fname)
{
  jhcParam *ps = &gps;
  char tag[20];
  int ok;

  ps->SetTitle("%s geometry", name);
  sprintf_s(tag, "%s_%ccal", group, tolower(name[0]));
  ps->SetTag(tag, 0);
  ps->NextSpecF( &dhd,  "D-H offset along axis (in)");
  ps->NextSpecF( &dhr,  "D-H ortho offset (in)");
  ps->NextSpecF( &dht,  "D-H axis zero angle (degs)");
  ps->NextSpecF( &dha,  "D-H axis tilt (degs)");
  ps->NextSpecF( &cal,  "Calibration (degs)");
  ps->NextSpecF( &zero, "World zero wrt servo (degs)"); 

  ps->NextSpecF( &amin, "Min world angle (degs)");      
  ps->NextSpecF( &amax, "Max world angle (degs)");      
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set geometry parameters in same order as configuration file.

void jhcJoint::SetGeom (double d, double r, double t, double a, double c, double z, double a0, double a1)
{
  // link and axis description
  dhd  = d;
  dhr  = r;
  dht  = t;
  dha  = a;

  // servo angle interpretation
  cal  = c;
  zero = z;
  amin = a0;
  amax = a1;
}


//= Set the underlying position feedback gain of the servos.

void jhcJoint::SetStiff (double st)
{
  if (id != 0)
    dyn->SetSlope(abs(id), st, st);
  if (id2 != 0)
    dyn->SetSlope(abs(id2), st, st);
}


///////////////////////////////////////////////////////////////////////////
//                            Command Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Normalizes angle to be in range amin to amax.
// useful for input to trapezoidal profiling

double jhcJoint::CycNorm (double degs, int clamp) const
{
  double norm, d1, d0 = degs - amin;

  // convert to a positive angle wrt amin
  while (d0 < 0.0)
    d0 += 360.0;
  while (d0 >= 360.0)
    d0 -= 360.0;
  norm = amin + d0;
  if (clamp <= 0)
    return norm;

  // clamp to closest range endpoint
  d1 = norm - amax;
  if (d1 <= 0.0)
    return norm;
  d0 = 360.0 - d0;
  if (d0 < d1)
    return amin;
  return amax;
}


//= Constrain angle command to valid range for servo.

double jhcJoint::Clamp (double degs) const 
{
  double a = degs;

  if (a > 180.0)
    a -= 360.0 * ROUND(a / 360.0);
  else if (a <= -180.0)
    a += 360.0 * ROUND(a / 360.0);
  return __max(a0, __min(a, a1));
}


//= Set just this one joint as passive (no torque).
// if doit <= 0 then freezes joint at current position
// waits on acknowledgement, use ServoLimp if multiple joints

int jhcJoint::Limp (int doit)
{
  int svo[2];
  int n = 0;

  if (dyn == NULL)
    return 0;

  // assemble list of servos
  if (id != 0)
    svo[n++] = abs(id);
  if (id2 != 0)
    svo[n++] = abs(id2);

  // send single or multiple command
  if (n == 1)
    dyn->Limp(svo[0], doit);
  else if (n == 2)
    dyn->MultiLimp(svo, 2, doit);
  return 1;
}


//= Command joint to go to a certain angle at some maximum speed.

int jhcJoint::SetAngle (double degs, double dps)
{
  double a = degs + zero + cal, a2 = a + off;

  if (dyn == NULL)
    return 0;

  if (id < 0)
    a = -a;
  if (id2 < 0)
    a2 = -a2;
  if (id != 0)
    dyn->SetPosVel(abs(id), a, dps);
  if (id2 != 0)
    dyn->SetPosVel(abs(id2), a2, dps);
  return 1;
}


//= Adds appropriate servo ids, positions, and velocites to arrays starting at n.
// returns number of entries made, useful for combining all joints into one command

int jhcJoint::ServoCmd (int *sv, double *pos, double *vel, int n, double degs, double dps) 
{
  double a = degs + zero + cal, a2 = a + off;
  int i = n;

  if ((sv == NULL) || (pos == NULL) || (vel == NULL))
    return 0;

  if (id != 0)
  {
    sv[i]  = abs(id);
    pos[i] = ((id > 0) ? a : -a);
    vel[i] = dps;
    i++;
  }
  if (id2 != 0)
  {
    sv[i]  = abs(id2);
    pos[i] = ((id2 > 0) ? a2 : -a2);
    vel[i] = dps;
    i++;
  }
  return(i - n);
}


//= Adds appropriate servo ids to array starting at n (for jhcDynamixel::MultiLimp).
// returns number of entries made, useful for combining all joints into one command

int jhcJoint::ServoNums (int *sv, int n)
{
  int i = n;

  if (sv == NULL) 
    return 0;

  if (id != 0)
    sv[i++] = abs(id);
  if (id2 != 0)
    sv[i++] = abs(id2);
  return(i - n);
}


///////////////////////////////////////////////////////////////////////////
//                           Status Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Read current state parameters from primary & secondary servos.

int jhcJoint::GetState ()
{
  int ok = 1;

  // make sure connected to serial port
  if (dyn == NULL)
    return Fatal("No port bound in jhcJoint::GetState");
  prev = th;

  // primary servo
  if (id != 0)
  {
    if (dyn->GetState(th, sv, f, abs(id)) <= 0)
      ok = 0;
    else
    {
      err = dyn->Flags();
      if (id < 0)
      {
        th = -th;
        sv = -sv;
        f  = -f;
      }
      th -= (zero + cal);
    }
  }

  // secondary coupled servo (if any)
  if (id2 != 0)
  {
    if (dyn->GetState(th2, sv2, f2, abs(id2)) <= 0)
      ok = 0;
    else
    {
      err2 = dyn->Flags();
      if (id2 < 0)
      {
        th2 = -th2;
        sv2 = -sv2;
        f2  = -f2;
      }
    }
  }
  return ok;
}


//= Report torque exerted by joint (1.0 = about 220 oz-in).
// can automatically multiply by max torque to yield oz-in
// with tmax = 1 returns fraction of maximum
// if two servos, then gives sum of torques

double jhcJoint::Torque (double tmax) const    
{
  double sum = 0.0;

  if (id != 0)
    sum += f;
  if (id2 != 0)
    sum += f2;
  return(tmax * sum);
}


//= Check misbalance between servo torques and adjust offset to lessen.
// only meaningful if there are two servo (e.g. ELI lift joint)
// target misbalance can be near zero, or in a band (reduces jittering)
// returns current signed unbalance = f - f2

double jhcJoint::AdjBal (double inc, double lo, double hi)
{
  double diff = f - f2; 

  // check if applicable
  if ((id == 0) || (id2 == 0))
    return 0.0;
  if (inc == 0.0)
    return diff;

  // see which servo is pushing harder
  if (diff >= 0.0)
  {
    // jockey into positive pass band
    if (diff > hi)
      off += inc;
    else if (diff < lo)
      off -= inc;
  }
  else
  {
    // jockey into negative pass band
    if (diff < -hi)
      off -= inc;
    else if (diff > -lo)
      off += inc;
  }
  return diff;
}


//= Returns change in joint angle needed to approximate traversal from a0 to a1.
// clips a0 and a1 to ends and only suggests traversal in valid control range

double jhcJoint::CtrlDiff (double a1, double a0) const
{
  double start, end;

  canonical(&start, a0);
  canonical(&end, a1);
  return(end - start);
}


//= Tells how far angle is outside of control range.
// positive numbers are beyond amax, negative are below amin

double jhcJoint::CtrlErr (double a) const
{
  return canonical(NULL, a);
}


//= Computes angle in span amin to amin + rng of (and clips if needed).
// returns angular error if input needed clipping

double jhcJoint::canonical (double *norm, double a) const
{
  double da, e1, e0, err = 0.0;

  // get angle beyond amin into 0-360 deg range
  da = a - amin;
  while (da < 0.0)
    da += 360.0;
  while (da >= 360.0)
    da -= 360.0;

  // check distances to ends if out of range
  e1 = da - rng;
  if (e1 > 0.0)
  {
    e0 = 360.0 - da;
    if (e0 < e1)
    {
      // clamp to lower angle
      da = 0.0;
      err = -e0;
    }
    else
    {
      // clamp to higher angle
      da = rng;
      err = e1;
    }
  }

  // possibly bind corrected control angle
  if (norm != NULL)
    *norm = amin + da;
  return err;
}


///////////////////////////////////////////////////////////////////////////
//                           Geometry Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Compute single joint and global coordinate transform matrices.
// takes joint angle and global mapping of previous joint (or x0, y0, z0)
// dices up transform from previous joint into several vectors

void jhcJoint::SetMapping (double degs, const jhcJoint *prev, 
                           double x0, double y0, double z0)
{
  const jhcMatrix *p;
  double v;
  int i, j, k;

  // get local transform (= global if no previous joint)
  dh_matrix(degs, 0);
  if (prev == NULL)
  {
    // set up default position & directions of coordinate system
    fwd.Copy(dhm);
    fwd.Translate(x0, y0, z0);
    orig.SetVec3(x0, y0, z0, 1.0);
    xdir.SetVec3(1.0, 0.0, 0.0, 0.0);
    ydir.SetVec3(0.0, 1.0, 0.0, 0.0);
    zdir.SetVec3(0.0, 0.0, 1.0, 0.0);
    return;
  }

  // cascade with global transform (bottom row unchanged)
  p = &(prev->fwd);
  for (i = 0; i < 4; i++)
    for (j = 0; j < 3; j++)
    {
      v = 0.0;
      for (k = 0; k < 4; k++)
        v += p->MRef(k, j) * dhm.MRef(i, k);
      fwd.MSet(i, j, v);
    }

  // save global position of origin and global direction of axes 
  prev->End0(orig);
  prev->EndX(xdir);
  prev->EndY(ydir);
  prev->EndZ(zdir);
}


//= Fill homogeneous Denavit-Hartenberg forward transform matrix.
// only recomputes theta dependent entries unless full is non-zero

void jhcJoint::dh_matrix (double degs, int full)
{
  double tr = D2R * (degs + dht), ct = cos(tr), st = sin(tr);
  double ar = D2R * dha, ca = cos(ar), sa = sin(ar);

  // theta dependent portion 
  dhm.MSet(0, 0,  ct);                               // top row
  dhm.MSet(1, 0,      -st * ca);
  dhm.MSet(2, 0,                st * sa);
  dhm.MSet(3, 0,                         ct * dhr);
  dhm.MSet(0, 1,  st);                               // second row
  dhm.MSet(1, 1,       ct * ca);
  dhm.MSet(2, 1,               -ct * sa);
  dhm.MSet(3, 1,                         st * dhr);

  // bottom two rows of matrix (constant)
  if (full <= 0)
    return;
  dhm.MSet(0, 2, 0.0);                               // third row
  dhm.MSet(1, 2,         sa);
  dhm.MSet(2, 2,                  ca);
  dhm.MSet(3, 2,                           dhd);
  dhm.MSet(0, 3, 0.0);                               // bottom row
  dhm.MSet(1, 3,         0.0);
  dhm.MSet(2, 3,                  0.0);
  dhm.MSet(3, 3,                           1.0);
}


//= Return the global coordinates for the given local point.
// can transform directions only if dir > 0

int jhcJoint::GlobalMap (jhcMatrix& gbl, const jhcMatrix& tool, int dir) const
{
  double x, y, z;

  if (!gbl.Vector(4) || !tool.Vector(4))
    return Fatal("Bad input to jhcJoint::GlobalMap");

  // save parts of input vector
  x = tool.X();
  y = tool.Y();
  z = tool.Z();

  // do rotation part (arranged in easily optimized order)
  gbl.SetX(x * fwd.MRef(0, 0));
  gbl.SetY(x * fwd.MRef(0, 1));
  gbl.SetZ(x * fwd.MRef(0, 2)); 
  gbl.IncX(y * fwd.MRef(1, 0));
  gbl.IncY(y * fwd.MRef(1, 1));
  gbl.IncZ(y * fwd.MRef(1, 2));
  gbl.IncX(z * fwd.MRef(2, 0));
  gbl.IncY(z * fwd.MRef(2, 1));
  gbl.IncZ(z * fwd.MRef(2, 2));

  // possibly add in translation
  if (dir > 0)
  {
    gbl.SetH(0.0);
    return 1;
  }
  gbl.IncX(fwd.MRef(3, 0));
  gbl.IncY(fwd.MRef(3, 1));
  gbl.IncZ(fwd.MRef(3, 2));
  gbl.SetH(1.0);
  return 1;
}


//= Return origin of coordinate system after mapping.
// use Axis0 to get position before mapping

void jhcJoint::End0 (jhcMatrix& loc) const
{
  if (!loc.Vector(4))
    Fatal("Bad input to jhcJoint::End0");

  loc.SetVec3(fwd.MRef(3, 0), fwd.MRef(3, 1), fwd.MRef(3, 2), 1.0);
}


//= Return x axis direction coordinate system after mapping.
// use AxisX to get direction before mapping

void jhcJoint::EndX (jhcMatrix& dir) const
{
  if (!dir.Vector(4))
    Fatal("Bad input to jhcJoint::EndX");

  dir.SetVec3(fwd.MRef(0, 0), fwd.MRef(0, 1), fwd.MRef(0, 2), 0.0);
}


//= Return y axis direction coordinate system after mapping.
// use AxisY to get direction before mapping

void jhcJoint::EndY (jhcMatrix& dir) const
{
  if (!dir.Vector(4))
    Fatal("Bad input to jhcJoint::EndY");

  dir.SetVec3(fwd.MRef(1, 0), fwd.MRef(1, 1), fwd.MRef(1, 2), 0.0);
}


//= Return z axis direction coordinate system after mapping.
// use AxisZ to get direction before mapping

void jhcJoint::EndZ (jhcMatrix& dir) const
{
  if (!dir.Vector(4))
    Fatal("Bad input to jhcJoint::EndZ");

  dir.SetVec3(fwd.MRef(2, 0), fwd.MRef(2, 1), fwd.MRef(2, 2), 0.0);
}




