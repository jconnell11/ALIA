// jhcManipFSM.cpp : state sequencer for complex arm motions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2016 IBM Corporation
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
#include "Interface/jms_x.h"

#include "Eli/jhcManipFSM.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcManipFSM::jhcManipFSM ()
{
  tpos0.SetSize(4);
  tdir0.SetSize(4);
  tpos.SetSize(4);
  tdir.SetSize(4);

  arm = NULL;
  zgrab = 0.0;
  noisy = 2;  // 1 for steps, 2 for target and finish, 3 for progress

  Defaults();
  SetRate(30.0);
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for specifying trajectory properties.

int jhcManipFSM::traj_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("man_traj", 0);
  ps->NextSpecF( &tz,     0.0, "Height above table"); // up-2
  ps->NextSpecF( &tilt, -15.0, "Tilt angle");  
  ps->NextSpecF( &roll,   0.0, "Roll angle");  
  ps->NextSpecF( &up,     1.0, "Lift amount");        // was 2
  ps->NextSpecF( &wid,    3.3, "Grip opening");  
  ps->NextSpecF( &wtol,   0.1, "Grip size tolerance");  

  ps->NextSpecF( &fhi,   16.0, "Grip start force");   // was 12
  ps->NextSpecF( &flo,    8.0, "Grip hold force");    // was 11
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used to specify home position.

int jhcManipFSM::home_params (const char *fname)
{
  jhcParam *ps = &hps;
  int ok;

  ps->SetTag("man_home", 0);
  ps->NextSpecF( &hx,  -3.3, "X position");      // added ax0 = -3.3
  ps->NextSpecF( &hy,  11.9, "Y position");      // added ay0 = +6.9
  ps->NextSpecF( &hz,  -0.8, "Z position");      // sub block = +1.8 up-2
  ps->NextSpecF( &hp, 180.0, "Pan angle");  
  ps->NextSpecF( &ht, -15.0, "Tilt angle");  
  ps->NextSpecF( &hr,   0.0, "Roll angle");  

  ps->NextSpecF( &hw,   0.0, "Gripper opening");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used to specify middle deployed position.

int jhcManipFSM::mid_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("man_mid", 0);
  ps->NextSpecF( &mx,  -3.3, "X position");     // added ax0 = -3.3
  ps->NextSpecF( &my,  16.9, "Y position");     // added ay0 = +6.9
  ps->NextSpecF( &mz,  -0.8, "Z position");     // sub block = +1.8 up-2
  ps->NextSpecF( &mp, 180.0, "Pan angle");  
  ps->NextSpecF( &mt, -15.0, "Tilt angle");  
  ps->NextSpecF( &mr,   0.0, "Roll angle");  

  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for passing objects.

int jhcManipFSM::pass_params (const char *fname)
{
  jhcParam *ps = &pps;
  int ok;

  ps->SetTag("man_pass", 0);
  ps->NextSpecF( &px,    -11.3, "X position");   // added ax0 = -3.3
  ps->NextSpecF( &py,     10.9, "Y position");   // added ay0 = +6.9
  ps->NextSpecF( &pz,     -0.3, "Z position");   // sub block = +1.8 up-2
  ps->NextSpecF( &pp,   -135.0, "Pan angle");  
  ps->NextSpecF( &pt,    -15.0, "Tilt angle");  
  ps->NextSpecF( &pr,      0.0, "Roll angle");  

  ps->NextSpec4( &pbox,   80,   "Handoff region (pels)");
  ps->NextSpecF( &pwait,   1.0, "Mouse wait (secs)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Read all relevant defaults variable values from a file.

int jhcManipFSM::Defaults (const char *fname)
{
  int ok = 1;

  ok &= traj_params(fname);
  ok &= home_params(fname);
  ok &= mid_params(fname);
  ok &= pass_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcManipFSM::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= tps.SaveVals(fname);
  ok &= hps.SaveVals(fname);
  ok &= mps.SaveVals(fname);
  ok &= pps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Associate controller with some arm to be commanded.

void jhcManipFSM::Bind (jhcEliArm *a)
{
  arm = a;
}


//= Reset state for the beginning of a sequence.
// clears state variable controlled motion phases

void jhcManipFSM::Reset ()
{
  retry = 0;
  phase = 1;
  mark = 0;
  grip = 0.0;
}


//= Remember target location on table along with via location and approach direction.
// assumes image coordinates already converted to real-world corrdinates for points

void jhcManipFSM::SetTarget (double x2, double y2, double x, double y, double dir)
{
  tx = x2;
  ty = y2;
  vx = x;
  vy = y;
  vdir = dir;
  Reset();
}


//= Test if some procedure has reached completion.
// returns 1 if trying, 2 finished, negative if failed

int jhcManipFSM::Complete (int total)
{
  if (phase < 0)
    return phase;
  if ((total == 0) || (phase > total))
    return 2;
  return 1;
}


//= Tells if the arm is close to the home position.

int jhcManipFSM::NearHome (double pmax, double dmax)
{
  double x, y, z, p, t, r, w, da;

  // check current position
  get_current(x, y, z, p, t, r, w);
  if ((fabs(x - hx) > pmax) || 
      (fabs(y - hy) > pmax) || 
      (fabs(z - hz) > pmax))
    return 0;
  
  // check current pan
  da = fabs(p - hp);
  if (da > 180.0)
    da = 360.0 - da;
  if (da > dmax)
    return 0;

  // check current tilt
  da = fabs(t - ht);
  if (da > 180.0)
    da = 360.0 - da;
  if (da > dmax)
    return 0;

  // check current roll
  da = fabs(r - hr);
  if (da > 180.0)
    da = 360.0 - da;
  if (da > dmax)
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                             Arm Interface                             //
///////////////////////////////////////////////////////////////////////////

//= Get current finger width.

double jhcManipFSM::get_width (int update)
{
  jhcMatrix pos(4), dir(4);

  if (arm == NULL)
    return 0.0;
  if (update > 0)
    arm->Update();
  arm->ArmPose(pos, dir);
  return dir.W();
}


//= Get grasp height and width.

void jhcManipFSM::get_grasp (double& z, double& w, int update)
{
  jhcMatrix pos(4), dir(4);

  // set up defaults
  z = 0.0;
  w = 0.0;
  if (arm == NULL)
    return;

  // read values
  if (update > 0)
    arm->Update();
  arm->ArmPose(pos, dir);
  z = pos.Z();
  w = dir.W();   
}


//= Find current discrete pose values by reading arm servo state.

void jhcManipFSM::get_current (double& x, double& y, double& z, 
                               double& p, double& t, double& r, double& w,
                               int update)
{
  jhcMatrix pos(4), dir(4);

  // fill in vectors
  pos.Zero(1.0);
  dir.Zero(0.0);
  if (arm != NULL)
  {
    if (update > 0)
      arm->Update();
    arm->ArmPose(pos, dir);
  }

  // extract pieces
  x = pos.X(); 
  y = pos.Y(); 
  z = pos.Z();
  p = dir.P(); 
  t = dir.T(); 
  r = dir.R(); 
  w = dir.W(); 
}


//= Use discrete variables to set up target position and orientation vectors.
// uses old arm-based coordinates, not wheel-centered coordinates
// saves expected number of calls in variable "tdone"

void jhcManipFSM::seek_target (double x, double y, double z, 
                               double p, double t, double r, double w,
                               double sp)
{
//  int ms = ROUND(1000.0 * tupd);

  // set up target pose
  tpos.SetVec3(x, y, z);
  tpos0.Copy(tpos);
  tdir.SetVec3(p, t, r, w);
  tdir0.Copy(tdir);
  pick_speeds(sp);
  retry = 0;

  // possible report
  if (noisy > 1)
  {
    char v1[40], v2[40];
    jprintf("\ntarget: %s x %s @ %3.1f\n", tpos.ListVec3(v1, "%4.2f", 0, 40), tdir.ListVec3(v2, "%4.2f", 1, 40), grip);
(arm->Position())->PrintVec3("now: loc", "%3.1f", 0, 0);
(arm->Direction())->PrintVec3(", dir");
  }
}


//= Pick speeds to coordinate motion.

void jhcManipFSM::pick_speeds (double sp)
{
//  double secs, at, wt;
  double secs, pt, dt, wt;

//  at = arm->ArmTime0(tpos, tdir, sp);
  pt = arm->PosTime0(tpos, sp);
  dt = arm->DirTime0(tdir, sp);
  wt = arm->WidthTime0(tdir.VRef(3), sp);
//  secs = __max(at, wt);
  secs = __max(__max(pt, dt), wt);
  slow = sp;
  psp = arm->PosRate0(tpos, secs);
  dsp = arm->DirRate0(tdir, secs);
  wsp = arm->WidthRate0(tdir.VRef(3), secs);
//jprintf("[%3.2f] arm = %3.1f secs, hand = %3.2f secs -> (%3.2f %3.2f) : %3.2f\n", sp, at, wt, psp, dsp, wsp);
jprintf("[%4.2f] arm = (%4.2f %4.2f) secs, hand = %4.2f secs -> (%4.2f %4.2f) : %4.2f\n", sp, pt, dt, wt, psp, dsp, wsp);
psp = sp;
dsp = sp;
wsp = sp;
}


//= Start or continue a motion toward some target.
// assumes target location and orientation are stored in "targ" and "tdir"
// can change closeness tolerance for x and y, z height (sags), and orientation
// takes "mv_code" as standard return value (useful for finite state machines)
// returns: mv_code = moving, mv_code + 1 = arrived, -mv_code = timeout

int jhcManipFSM::await_target (int mv_code, double inxy, double inz, double degs, int nofail)
{
  jhcMatrix perr(4), derr(4);
  char v1[40], v2[40];
  double dm, dr, dw, df, w0, wait = 0.5;

  if (arm == NULL)
    return -mv_code;

  // try to move closer to goal (must reiterate goal pose)
  arm->Update();
  arm->ArmTarget(tpos, tdir, psp, dsp);
  if (grip > 0.0)
    arm->SqueezeTarget(fhi);
  else
    arm->WidthTarget(tdir.W(), wsp);
  arm->Issue(tupd);   
//  if ((noisy > 1) && (grip > 0.0))
//    jprintf("  f = %4.1f", arm->Squeeze());

  // get residual distances from original goal
  arm->ArmErr(perr, derr, tpos0, tdir0, 1);
  dw = arm->WidthErr(tdir.W(), 1);
  dm = __max(perr.X(), perr.Y());
  dr = derr.MaxVec3();
  df = grip - arm->Squeeze();

  // possibly report progress
  if (noisy > 2)
  {
/*
jhcMatrix loc(4), dir(4);
arm->ArmPose(loc, dir);
loc.PrintVec3("now", "%4.2f", 0, 0);
tpos0.PrintVec3(" vs goal", "%4.2f");
*/
    jprintf("  xy = %5.2f, z = %5.2f, rot = %5.2f, grip = %4.2f, df = %6.2f [%4.1f]\n", 
            dm, perr.Z(), dr, dw, df, grip);
  }

  // see if close enough yet
  if ((dm <= inxy) && (perr.Z() <= inz) && (dr <= degs) && (((grip <= 0.0) && (dw < 0.5)) || ((grip > 0.0) && (df < 1.0)))) 
  {
    // reset progress indicators
    arm->ArmClear();    
    arm->HandClear();
    if (noisy > 1)
    {
      // possibly report success
      arm->ArmPose(perr, derr);
      jprintf(">> DONE: %s x %s @ %3.1f\n", perr.ListVec3(v1, "%4.2f", 0, 40), derr.ListVec3(v2, "%4.2f", 0, 40), arm->Squeeze());
    }
    return(mv_code + 1);
  }

  // see if waited too long (or just assume done)
  if (arm->ArmFail(wait)) 
    if (((grip <= 0.0) && arm->WidthFail(wait)) || 
        ((grip > 0.0) && arm->SqueezeFail(wait)))
  {
    // reset progress indicators
    arm->ArmClear();    
    arm->HandClear();
    if (retry > 0)
    {
      // set new target which balances out residual error
      w0 = tdir.W();
      perr.ClampVec3(1.0);
      derr.ClampVec3(10.0);
      tpos.DiffVec3(tpos0, perr);
      tdir.DiffVec3(tdir0, derr);
      tdir.CycNorm3();
      tdir.SetW(w0);
      pick_speeds(slow);
      retry = 0;

      // announce change
      if (noisy > 0)
      {
        jprintf("\n>> retrying\n");
        tpos.PrintVec3i("altered: pos", "%4.2f");
        tdir.PrintVec3(", dir", "%4.2f");
      }
    }
    else if (nofail > 0)
    {
      if (noisy > 1)
      {
        // possibly report failure
        arm->ArmPose(perr, derr);
        jprintf(">> timeout: %s x %s @ %3.1f\n", perr.ListVec3(v1, "%4.2f", 0, 40), derr.ListVec3(v2, "%4.2f", 0, 40), arm->Squeeze());
      }
      return(mv_code + 1); 
    }
    else 
      return -mv_code;
  }

  // continue waiting
  return mv_code;
}


///////////////////////////////////////////////////////////////////////////
//                            Basic Segments                             //
///////////////////////////////////////////////////////////////////////////

//= Close the hand until reasonable force is felt then back off.
// returns total number of steps

int jhcManipFSM::CloseHand (int step0)
{
  double x, y, z, p, t, r, w, ftol = 6.0;  // was 2.0
  int n = 0;

  // start closing gripper (width based)
  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  closing ...");
    get_current(x, y, z, p, t, r, w);
    seek_target(x, y, z, p, t, r, -0.5);
    fcnt = 0;
    phase++;
  }

  // stop when hi grip force level 
  if ((phase - step0) == n++)
  {
    if (await_target(0) != 0)    // does update
    {
      // fail if timeout 
      if (noisy > 0)
        jprintf("\n    FAIL");
      grip = 0.0;
      phase = -phase;
    }
    else
    {
      // make sure grabbing for a while
      if (arm->SqueezeSome())
        fcnt++;
      else
        fcnt = 0;

      // some force, no motion, and non-zero width
      if ((fcnt >= 3) && arm->WidthStop() && (arm->Width() > 0.5))  // fcnt was 5
      {
        if (noisy > 1)
          jprintf("\n  gripping ... ");
        gcnt = 0;
        phase++;
      }
    }
  }

  // let grip stabilize
  if ((phase - step0) == n++)
  {
    grip = fhi;       
    await_target();
    if (fabs(arm->Squeeze() - fhi) > ftol)
      gcnt = 0;
    else if (++gcnt >= 5)
    {
      if (noisy > 1)
        jprintf("done: f = %3.1f\n\n", arm->Squeeze());
      phase++;
    }
  }

  // total number of steps
  return n;
}


//= Open the hand fully without changing location or pose.
// returns total number of steps

int jhcManipFSM::OpenHand (int step0)
{
  double x, y, z, p, t, r, w;
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  release ... ");
    get_current(x, y, z, p, t, r, w);
    grip = 0.0;
    seek_target(x, y, z, p, t, r, wid);
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


//= Move hand at same height forward along point direction.
// typically moves 2.5 inches times "amt" modifier (can be negative)
// returns total number of steps

int jhcManipFSM::ExtendHand (double amt, int step0)
{
  double x, y, z, p, t, r, w, rads, c, s, mv = 2.5 * amt;
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  extend ...");
    get_current(x, y, z, p, t, r, w);
    rads = D2R * p;
    c = cos(rads);
    s = sin(rads);
    seek_target(x + c * mv, y + s * mv, z, p, t, r, w);
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


//= Move hand at same height laterally to the pointing direction.
// typically moves 2 inches times "amt" modifier (can be negative)
// returns total number of steps

int jhcManipFSM::SlideHand (double amt, int step0)
{
  double x, y, z, p, t, r, w, rads, c, s, mv = 2.0 * amt;
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  slide ...");
    get_current(x, y, z, p, t, r, w);
    rads = D2R * p;
    c = cos(rads);
    s = sin(rads);
    seek_target(x + s * mv, y + c * mv, z, p, t, r, w);
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


//= Lift hand slightly at current location and orientation.
// typically moves 2 inches times "amt" modifier (can be negative)
// returns total number of steps

int jhcManipFSM::RaiseHand (double amt, int step0)
{
  double x, y, z, p, t, r, w, mv = 2.0 * amt;
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  raise ...");
    get_current(x, y, z, p, t, r, w);
    seek_target(x, y, z + mv, p, t, r, w);
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


//= Pan the fingers slightly while maintaining the current location.
// typically moves 30 degree times "amt" modifier (can be negative)
// returns total number of steps

int jhcManipFSM::PanHand (double amt, int step0)
{
  double x, y, z, p, t, r, w, mv = 30.0 * amt;
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  pan ...");
    get_current(x, y, z, p, t, r, w);
    seek_target(x, y, z, p + mv, t, r, w);
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


//= Tilt the fingers slightly while maintaining the current location.
// typically moves 30 degree times "amt" modifier (can be negative)
// returns total number of steps

int jhcManipFSM::TiltHand (double amt, int step0)
{
  double x, y, z, p, t, r, w, mv = 30.0 * amt;
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  tilt ...");
    get_current(x, y, z, p, t, r, w);
    seek_target(x, y, z, p, t + mv, r, w);
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


//= Roll the fingers slightly while maintaining the current location.
// typically moves 30 degree times "amt" modifier (can be negative)
// returns total number of steps

int jhcManipFSM::RollHand (double amt, int step0)
{
  double x, y, z, p, t, r, w, mv = 30.0 * amt;
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  roll ...");
    get_current(x, y, z, p, t, r, w);
    seek_target(x, y, z, p, t, r + mv, w);
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                               Locations                               //
///////////////////////////////////////////////////////////////////////////

//= Move hand to via point and orientation with some hand opening.
// assumes image coordinates already converted to real-world corrdinates for via point
// returns total number of steps

int jhcManipFSM::GotoVia (double zdes, double wdes, int step0)
{  
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  via ... ");
    seek_target(vx, vy, zdes, vdir, tilt, roll, wdes);  
    retry = 1;
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


//= Return to standard home position.
// returns total number of steps

int jhcManipFSM::GotoHome (int step0)
{
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  home ... ");
    seek_target(hx, hy, hz, hp, ht, hr, hw);
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}



//= Extend the hand to a central deployed position.
// returns total number of steps

int jhcManipFSM::GotoMiddle (int step0)
{
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  deploy ... ");
    seek_target(mx, my, mz, mp, mt, mr, get_width());
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


//= Goto to a standard location for transferring an object to the user.
// returns total number of steps

int jhcManipFSM::GotoXfer (int step0)
{
  int n = 0;

  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  rendezvous ... ");
    seek_target(px, py, pz, pp, pt, pr, get_width());
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                              Full Motions                             //
///////////////////////////////////////////////////////////////////////////

//= Point at some object on the table with the gripper open or closed.
// assumes via point already set up
// returns total number of steps

int jhcManipFSM::TablePoint (double wdes, int step0)
{
  double x, y, z, p, t, r, w;
  int n = 0;

  // set up to lift arm slightly above table and set orientation
  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  prepping ...");
    get_current(x, y, z, p, t, r, w);
    seek_target(x, y, tz, p, tilt, roll, w);
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);

  // set up long distance motion
  n += GotoVia(tz, wdes, step0 + n);
  return n;
}


//= Grab and lift some object on the table.
// assumes target and via point already set up
// returns total number of steps

int jhcManipFSM::TableLift (int step0)
{
  double x, y, z, p, t, r, w;
  int n = 0;

  // get to via point with open gripper
  n += TablePoint(wid, step0);

  // set up to enclose goal point
  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  enfold ... ");
    seek_target(tx, ty, tz, vdir, tilt, roll, wid, 0.5);  // slow
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);

  // close gripper on target
  n += CloseHand(step0 + n);

  // lift up slightly while preserving orientation
  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  lift ... ");
    get_current(x, y, z, p, t, r, w);
    zgrab = z;
    seek_target(x, y, z + up, p, t, r, w, 0.5);  // half speed
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);

  // total number of steps
  return n;
}


//= Set down grasped object and return to neutral position.
// assumes via point already set up
// returns total number of steps

int jhcManipFSM::TableDeposit (int step0)
{
  double x, y, z, p, t, r, w;
  int n = 0;

  // lower object at current orientation to original grab height
  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  deposit ... ");
    get_current(x, y, z, p, t, r, w);
    seek_target(x, y, zgrab, p, t, r, w, 0.1);  // very slow
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase, 0.5, 0.5);

  // open gripper in current location then withdraw to via point
  n += OpenHand(step0 + n);
  n += GotoVia(tz, wid, step0 + n);
  return n;
}


//= Move to handoff location and wait for visual motion.
// assumes via point already set up
// returns total number of steps

int jhcManipFSM::Handoff (int click, int step0)
{
  double zdes, wdes;
  int n = 0;

  // return to via location after grasp then move to handoff location
  get_grasp(zdes, wdes);
  n += GotoVia(zdes, wdes, step0);
  n += GotoXfer(step0 + n);

  // note entry to local sequence (blocks)
  if ((phase - step0) == n++)
  {
    jms_sleep(500);                        // let arm stop shaking
    mark++;                            // signal change (1)
    phase++;
  }

  // await user acceptance of object
  if ((phase - step0) == n++)
    if (click > 0)
    {
      mark++;                          // signal change (2)
      phase++;
    }

  // open gripper in current location
  n += OpenHand(step0 + n);

  // let user's hand clear area (blocks)
  if ((phase - step0) == n++)
  {
    jms_sleep(ROUND(1000.0 * pwait));
    mark++;                            // signal change (3)
    phase++;
  }
  return n;
}


//= Accept object from user and return to original location (14 internal steps).
// assumes target and via point already set up
// returns total number of steps

int jhcManipFSM::Replace (int click, int step0)
{
  double zdes, wdes;
  int n = 0;

  // await user presentation
  if ((phase - step0) == n++)
    if (click > 0)
      phase++;
 
  // fully grasp object 
  n += CloseHand(step0 + n);
  if ((phase - step0) == n++)
  {
    mark++;                            // signal change (4)
    phase++;
  }
  
  // set up approach to original location
  get_grasp(zdes, wdes);
  n += GotoVia(zdes, wdes, step0 + n);

  // arrive at original object location
  if ((phase - step0) == n++)
  {
    if (noisy > 0)
      jprintf("\n  return ... ");
    seek_target(tx, ty, tz + up, vdir, tilt, roll, get_width());
    phase++;
  }
  if ((phase - step0) == n++)
    phase = await_target(phase);

  // put object down then go back to home position
  n += TableDeposit(step0 + n);
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                              Motion Cycles                            //
///////////////////////////////////////////////////////////////////////////

//= Grab and lift an object, then put it down again and reset arm (22 internal states).
// assumes target and via point already set up
// returns total number of steps

int jhcManipFSM::GrabCycle (double dwell, int step0)                  
{
  int n = 0;

  // grab and lift object
  n += TableLift(step0);

  // wait for user to allow retraction 
  if ((phase - step0) == n++)
  {
    if (dwell < 0.0)
      Tell("Acquired object");
    else
    {
      if (noisy > 0)
        jprintf("\n  dwell ... ");
      dcnt = ROUND(dwell / tupd);
    }
    phase++;
  }

  // wait a while to announce failure (blocks)
  if ((phase - step0) == n++)
  {
    if (dcnt-- <= 0)
      phase++;
  }

  // put object on table and go back to home position through via
  n += TableDeposit(step0 + n);
  n += GotoHome(step0 + n);
  return n;
}


//= Caerfully retract and go to home position.
// useful for failed grasps

int jhcManipFSM::GrabReset (double dwell, int step0)
{
  int n = 0;

  // wait a while to announce failure (blocks)
  if ((phase - step0) == n++)
  {
    if (dwell > 0.0)
      jms_sleep(ROUND(1000.0 * dwell));
    phase++;
  }

  // go back to home position through via
  n += GotoVia(tz, wid, step0 + n);
  n += GotoHome(step0 + n);
  return n;
}


//= Point at object then retract arm (7 internal states). 
// assumes via point already set up
// returns total number of steps

int jhcManipFSM::PointCycle (int hold, int step0)                          
{
  int hit = 0, n = 0;

  // set to grab and lift object
  n += TablePoint(0.0, step0);

  // mark arrival
  if ((phase - step0) == n++)
  {
    hit = 1;
    mark++;
    phase++;
  }

  // wait for signal to allow retraction 
  if ((phase - step0) == n++)
    if ((hit <= 0) && (hold <= 0))
      phase++;

  // withdraw to home position
  n += GotoHome(step0 + n);
  return n;
}


//= Give object to user then replace it on table.
// assumes target and via point already set up
// returns total number of steps

int jhcManipFSM::GiveCycle (int click, int step0)
{
  int n = 0;

  n += TableLift(step0);
  n += Handoff(click, step0 + n);
  n += Replace(click, step0 + n);
  n += GotoHome(step0 + n);
  return n;
}


