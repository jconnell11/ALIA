// jhcBallistic.cpp : interface to ELI motion kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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
#include <string.h>

#include "Interface/jms_x.h"           // common video

#include "Body/jhcEliBody.h"           // common robot (only spec'd as class in hdr)

#include "Grounding/jhcBallistic.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcBallistic::~jhcBallistic ()
{
}


//= Default constructor initializes certain values.

jhcBallistic::jhcBallistic ()
{
  ver = 1.35;
  strcpy_s(tag, "Ballistic");
  Platform(NULL);
  rpt = NULL;
  pcnt = 0;
  power = 0;
  hold = 0;
  dbg = 0;
  Defaults();
}


//= Attach physical enhanced body and make pointers to some pieces.

void jhcBallistic::Platform (jhcEliGrok *robot) 
{
  rwi = robot;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for battery and freezing.

int jhcBallistic::evt_params (const char *fname)
{
  jhcParam *ps = &eps;
  int ok;

  ps->SetTag("ball_evt", 0);
  ps->NextSpec4( &tired,  30,   "Low battery (pct)");
  ps->NextSpec4( &fresh,  35,   "Okay battery (pct)");
  ps->NextSpec4( &psamp, 150,   "Test interval cycles");      // once every 5 sec
  ps->NextSpec4( &ptest,   6,   "Battery test cycles");       // 30 secs
  ps->NextSpecF( &nag,   180.0, "Complaint repeat (secs)");  
  ps->Skip();

  ps->NextSpecF( &hmin,    0.1, "Min hold width (in)");
  ps->NextSpec4( &hwait,  10,   "Firm hold cycles");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for translational motion.

int jhcBallistic::trans_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("ball_trans", 0);
  ps->NextSpecF( &stf,    0.5, "Slow multiplier");  
  ps->NextSpecF( &qtf,    1.5, "Fast multiplier");  
  ps->Skip();
  ps->NextSpecF( &step,   6.0, "Step distance (in)");  
  ps->NextSpecF( &move,  12.0, "Move distance (in)");  
  ps->NextSpecF( &drive, 24.0, "Drive distance (in)");  

  ps->NextSpecF( &ftime,  2.0, "Freeze time (sec)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for rotation interpretation.

int jhcBallistic::rot_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("ball_rot", 0);
  ps->NextSpecF( &srf,    0.5, "Slow multiplier");  
  ps->NextSpecF( &qrf,    1.5, "Fast multiplier");  
  ps->Skip();
  ps->NextSpecF( &turn,  90.0, "Turn angle (deg)");  
  ps->NextSpecF( &rot,  180.0, "Rotate angle (deg)");  
  ps->NextSpecF( &spin, 360.0, "Spin angle (deg)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for motion progress monitoring.

int jhcBallistic::prog_params (const char *fname)
{
  jhcParam *ps = &pps;
  int ok;

  ps->SetTag("ball_prog", 0);
  ps->NextSpecF( &mprog,   0.2, "Move progress (in)");  
  ps->NextSpec4( &mstart, 30,   "Move start cycles");
  ps->NextSpec4( &mmid,   10,   "Move stall cycles");
  ps->NextSpecF( &tprog,   0.5, "Turn progress (deg)");  
  ps->NextSpec4( &tstart, 30,   "Turn start cycles");
  ps->NextSpec4( &tmid,   10,   "Turn stall cycles");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for lifting motions.

int jhcBallistic::lift_params (const char *fname)
{
  jhcParam *ps = &lps;
  int ok;

  ps->SetTag("ball_lift", 0);
  ps->NextSpecF( &slf,     0.25, "Slow multiplier");  
  ps->NextSpecF( &qlf,     2.0,  "Fast multiplier");  
  ps->NextSpecF( &lift,    6.0,  "Lift distance (in)");
  ps->Skip(2);  
  ps->NextSpecF( &lprog,   0.2,  "Lift progress (in)");  

  ps->NextSpec4( &lstart, 20,    "Lift start cycles");
  ps->NextSpec4( &lmid,   10,    "Lift stall cycles");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for grabbing and releasing.

int jhcBallistic::grab_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("ball_grab", 0);
  ps->NextSpecF( &fhold,  12.0, "Holding force (oz)");  
  ps->NextSpec4( &fask,    5,   "Force repeat cycles");
  ps->Skip(2);
  ps->NextSpecF( &wtol,    0.1, "Width tolerance (in)");
  ps->NextSpecF( &gprog,   0.1, "Width progress (in)");  

  ps->NextSpec4( &gstart, 10,   "Width start cycles");
  ps->NextSpec4( &gmid,    5,   "Width stall cycles");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for arm extension.

int jhcBallistic::arm_params (const char *fname)
{
  jhcParam *ps = &aps;
  int ok;

  ps->SetTag("ball_arm", 0);
  ps->NextSpecF( &extx,    0.0, "Extended x postion (in)");   
  ps->NextSpecF( &exty,   21.5, "Extended y position (in)");  
  ps->NextSpecF( &extz,   -1.0, "Extended z position (in)");
  ps->Skip();
  ps->NextSpecF( &edir,   90.0, "Extended hand pan (deg)");
  ps->NextSpecF( &etip,  -15.0, "Extended hand tilt (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for incremental hand motion.

int jhcBallistic::hand_params (const char *fname)
{
  jhcParam *ps = &hps;
  int ok;

  ps->SetTag("ball_hand", 0);
  ps->NextSpecF( &dxy,     1.5, "Horizontal shift (in)");
  ps->NextSpecF( &dz,      1.0, "Vertical shift (in)");
  ps->Skip();
  ps->NextSpecF( &hdone,   0.5, "End position achieved (in)");
  ps->NextSpecF( &zdone,   0.1, "End height achieved (in)");
  ps->NextSpecF( &hprog,   0.1, "Hand progress (in)");

  ps->NextSpec4( &hstart, 10,   "Hand start cycles");
  ps->NextSpec4( &hmid,    5,   "Hand stall cycles");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for incremental hand rotation.

int jhcBallistic::wrist_params (const char *fname)
{
  jhcParam *ps = &wps;
  int ok;

  ps->SetTag("ball_wrist", 0);
  ps->NextSpecF( &wpan,   30.0, "Pan amount (deg)");
  ps->NextSpecF( &wtilt,  30.0, "Tilt amount (deg)");
  ps->NextSpecF( &wroll,  45.0, "Roll amount (deg)");
  ps->Skip();
  ps->NextSpecF( &wdone,   2.0, "Orientation achieved (deg)");
  ps->NextSpecF( &wprog,   1.0, "Rotation progress (deg)");

  ps->NextSpec4( &wstart, 10,   "Wrist start cycles");
  ps->NextSpec4( &wmid,    5,   "Wrist stall cycles");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for head reorientation.

int jhcBallistic::neck_params (const char *fname)
{
  jhcParam *ps = &nps;
  int ok;

  ps->SetTag("ball_neck", 0);
  ps->NextSpecF( &npan,   45.0, "Pan amount (deg)");
  ps->NextSpecF( &ntilt,  45.0, "Tilt amount (deg)");      // wrt -15 neutral
  ps->NextSpecF( &sgz,     0.5, "Slow multiplier");
  ps->NextSpecF( &qgz,     2.0, "Fast multiplier");
  ps->NextSpecF( &ndone,   2.0, "Orientation achieved (deg)");
  ps->NextSpecF( &nprog,   1.0, "Rotation progress (deg)");

  ps->NextSpec4( &nstart, 10,   "Neck start cycles");
  ps->NextSpec4( &nmid,    5,   "Neck stall cycles");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcBallistic::Defaults (const char *fname)
{
  int ok = 1;

  ok &= evt_params(fname);
  ok &= trans_params(fname);
  ok &= rot_params(fname);
  ok &= prog_params(fname);
  ok &= lift_params(fname);
  ok &= grab_params(fname);
  ok &= arm_params(fname);
  ok &= hand_params(fname);
  ok &= wrist_params(fname);
  ok &= neck_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcBallistic::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= eps.SaveVals(fname);
  ok &= tps.SaveVals(fname);
  ok &= rps.SaveVals(fname);
  ok &= pps.SaveVals(fname);
  ok &= lps.SaveVals(fname);
  ok &= gps.SaveVals(fname);
  ok &= aps.SaveVals(fname);
  ok &= hps.SaveVals(fname);
  ok &= wps.SaveVals(fname);
  ok &= nps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Set up for new run of system.

void jhcBallistic::local_reset (jhcAliaNote *top)
{
  rpt = top;
  power = 0;
  kvetch = 0;
  hold = 0;
}


//= Post any spontaneous observations to attention queue.

void jhcBallistic::local_volunteer ()
{
  power_state();   
  hand_drop(); 
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcBallistic::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(ball_stop);
  JCMD_SET(ball_drive);
  JCMD_SET(ball_turn);
  JCMD_SET(ball_lift);
  JCMD_SET(ball_grip);
  JCMD_SET(ball_arm);
  JCMD_SET(ball_wrist);
  JCMD_SET(ball_neck);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcBallistic::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(ball_stop);
  JCMD_CHK(ball_drive);
  JCMD_CHK(ball_turn);
  JCMD_CHK(ball_lift);
  JCMD_CHK(ball_grip);
  JCMD_CHK(ball_arm);
  JCMD_CHK(ball_wrist);
  JCMD_CHK(ball_neck);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                           Reported Events                             //
///////////////////////////////////////////////////////////////////////////

//= Inject NOTE when battery voltage low for a while.
// states: "I am tired"

void jhcBallistic::power_state ()
{
  UL32 now;
  int pct, repeat;

  // lock to sensor cycle
  if ((rpt == NULL) || (rwi == NULL) || rwi->Ghost())
    return;
  if (!rwi->Accepting())
    return;

  // only test infrequently since requires extra serial transaction
  if (pcnt++ < psamp)
    return; 
  pcnt = 0;
  pct = (rwi->arm)->Power();

  // see if battery low for a while 
  if (pct >= fresh)
    power = 0;
  else if (pct <= tired)
    power++;
  if (power < ptest)
  {
    kvetch = 0;
    return;
  }

  // determine how often to complain (more frequent when lower)
  now = jms_now();
  repeat = ROUND((1000.0 * nag * pct) / tired);
  if ((kvetch != 0) && (jms_diff(now, kvetch) < repeat))
    return;
  kvetch = now;

  // generate conscious event
  rpt->StartNote();
  rpt->NewProp(rpt->Self(), "hq", "tired");
  rpt->FinishNote();
}


//= Inject NOTE when hand had been holding something but now seems empty.
// states: "I lost my grip"

void jhcBallistic::hand_drop ()
{
  const jhcEliArm *a;
  jhcAliaDesc *evt, *n;
  int h0 = hold;

  // lock to sensor cycle
  if ((rpt == NULL) || (rwi == NULL) || rwi->Ghost())
    return;
  if (!rwi->Accepting())
    return;
  a = rwi->arm;

  // see if force and non-zero width for a while
  hold++;
  if (!a->HoldMode() || (a->WidthErr(hmin) <= wtol))
    hold = 0;

  // signal once on fingers separation going to zero
  if ((hold > 0) || (h0 < hwait)) 
    return;
  rpt->StartNote();
  evt = rpt->NewNode("evt", "lose");
  rpt->AddArg(evt, "agt", rpt->Self());
  n = rpt->NewNode("obj");
  rpt->NewProp(n, "ako", "grip");
  rpt->AddArg(evt, "obj", n);
  rpt->FinishNote();
}


///////////////////////////////////////////////////////////////////////////
//                             Overall Poses                             //
///////////////////////////////////////////////////////////////////////////

//= Start freeze of translation and rotation.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBallistic::ball_stop_set (const jhcAliaDesc *desc, int i)
{
  ct0[i] += ROUND(1000.0 * ftime);
  return 1;
}


//= Continue freeze of translation and rotation until timeout.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBallistic::ball_stop_chk (const jhcAliaDesc *desc, int i)
{
  jhcMatrix pos(4), dir(4);
  jhcEliBase *b;
  jhcEliArm *a;

  // lock to sensor cycle 
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;
  b = rwi->base;
  a = rwi->arm;

  // check for timeout
  if (jms_diff(jms_now(), ct0[i]) > 0)
    return 1;
  a->ArmPose(pos, dir);
 
  // re-issue basic command (coast to stop, no bouncing)
  jprintf(1, dbg, ">> REQUEST %d: stop motion\n", cbid[i]);
  a->ArmTarget(pos, dir, 1.0, 1.0, cbid[i]);
  b->DriveTarget(0.0, 0.0, 1.0, cbid[i]);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Translation                              //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced move command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBallistic::ball_drive_set (const jhcAliaDesc *desc, int i)
{
  if (get_vel(csp[i], desc->Val("arg")) <= 0)
    return -1;
  if (get_dist(camt[i], desc->Val("arg")) <= 0)
    return -1;
  ct0[i] = 0;
  return 1;
} 


//= Check whether move command is done yet.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBallistic::ball_drive_chk (const jhcAliaDesc *desc, int i)
{
  jhcEliBase *b;
  double err;

  // lock to sensor cycle
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;
  b = rwi->base;

  if (cst[i] <= 0)
  {
    // set up absolute target distance
    camt[i] = b->MoveGoal(camt[i]);
    cerr[i] = b->MoveErr(camt[i]);
    cst[i]  = 1;
  }
  else 
  {
    // check if finished or stuck
    err = b->MoveErr(camt[i]);
    jprintf(2, dbg, "move: %3.1f, err = %3.1f, stuck = %d\n", b->Travel(), err, ct0[i]);
    if (err < (1.5 * b->mdead))
      return 1;
    if (Stuck(i, err, mprog, mstart, mmid))
      return -1;
  }

  // re-issue basic command (move and turn are separate resources)
  jprintf(1, dbg, ">> REQUEST %d: move @ %3.1f in\n", cbid[i], camt[i]);
  b->MoveAbsolute(camt[i], csp[i], cbid[i]);
  return 0;
}


//= Read semantic network parts to determine amount of travel.
// step = 4", move = 8", drive = 16"
// returns 1 if proper values, -1 for problem

int jhcBallistic::get_dist (double& dist, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *dir;

  // sanity check
  if (act == NULL)
    return -1;

  // set distance based on main verb
  if (act->WordIn("step"))
    dist = step;
  else if (act->WordIn("move"))
    dist = move;
  else if (act->WordIn("drive"))
    dist = drive;
  else
    return -1;

  // get directional modifier of main verb 
  if ((dir = act->Fact("dir")) != NULL)
  {
    // see if some standard direction term (checks halo also)
    if (dir->WordIn("backward", "backwards"))
      dist = -dist;
    else if (!dir->WordIn("forward", "forwards"))
      return -1;
  }
  return 1;
}


//= Read semantic network parts to determine speed of travel.
// <pre>
//
// "drive backward"
//   act-1 -lex-  drive
//   dir-2 -lex-  backward
//         -dir-> act-1
//   mod-3 -lex-  slowly
//         -mod-> act-1
//
// </pre>
// returns 1 if proper values, -1 for problem

int jhcBallistic::get_vel (double& speed, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *rate;
  int w = 0;

  // sanity check
  if (act == NULL)
    return -1;

  // look for speed modifier(s)
  speed = 1.0;
  while ((rate = act->Fact("mod", w++)) != NULL)
  {
    if (rate->WordIn("slowly"))
      speed *= stf;
    else if (rate->WordIn("quickly"))
      speed *= qtf;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                                Rotation                               //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced turn command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBallistic::ball_turn_set (const jhcAliaDesc *desc, int i)
{
  if (get_spin(csp[i], desc->Val("arg")) <= 0)
    return -1;
  if (get_ang(camt[i], desc->Val("arg")) <= 0)
    return -1;
  ct0[i] = 0;
  return 1;
} 


//= Check whether turn command is done yet.
// returns 1 if done, 0 if still working, -1 for failure

int jhcBallistic::ball_turn_chk (const jhcAliaDesc *desc, int i)
{
  jhcEliBase *b;
  double err;

  // lock to sensor cycle
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;
  b = rwi->base;

  if (cst[i] <= 0)
  {
    // set up absolute target angle
    camt[i] = b->TurnGoal(camt[i]);
    cerr[i] = b->TurnErr(camt[i]);
    cst[i]  = 1;
  }
  else 
  {
    // check if finished or stuck
    err = b->TurnErr(camt[i]);
    jprintf(2, dbg, "turn: %3.1f, err = %4.2f, stuck = %d\n", b->WindUp(), err, ct0[i]);
    if (err < (1.5 * b->tdead))
      return 1;
    if (Stuck(i, err, tprog, tstart, tmid))
      return -1;
  }

  // re-issue basic command (move and turn are separate resources)
  jprintf(1, dbg, ">> REQUEST %d: turn @ %3.1f deg\n\n", cbid[i], camt[i]);
  b->TurnAbsolute(camt[i], csp[i], cbid[i]);
  return 0;
}


//= Read semantic network to get amount to rotate.
// turn = 90 degs, rotate = 180 degs, spin = 360 degs
// returns 1 if proper values, -1 for problem

int jhcBallistic::get_ang (double& ang, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *dir;

  // sanity check
  if (act == NULL)
    return -1;
  ang = turn;

  // get angle based on main verb
  if (act->WordIn("spin"))
    ang = spin;
  else if (act->WordIn("rotate"))
    ang = rot;
  else if (!act->WordIn("turn"))
    return -1;

  // get directional modifier of main verb 
  if ((dir = act->Fact("dir")) != NULL)
  {
    // see if some standard direction term (checks halo also)
    if (dir->WordIn("clockwise", "right"))
      ang = -ang;
    else if (!dir->WordIn("counterclockwise", "left"))
      return -1;
  }
  return 1;
}


//= Read semantic network parts to determine direction to turn.
// <pre>
//
// "turn clockwise"
//   act-1 -lex-  turn
//   dir-2 -lex-  clockwise
//         -dir-> act-1         
//   mod-3 -lex-  quickly
//         -mod-> act-1
//
// </pre>
// returns 1 if proper values, -1 for problem

int jhcBallistic::get_spin (double& speed, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *rate;
  int w = 0;

  // sanity check
  if (act == NULL)
    return -1;

  // look for speed modifier(s)
  speed = 1.0;
  while ((rate = act->Fact("mod", w++)) != NULL)
  {
    if (rate->WordIn("slowly"))
      speed *= srf;                         
    else if (rate->WordIn("quickly"))
      speed *= qrf;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                                  Lift                                 //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced lift command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBallistic::ball_lift_set (const jhcAliaDesc *desc, int i)
{
  if (get_up(camt[i], desc->Val("arg")) <= 0)
    return -1;
  if (get_vsp(csp[i], desc->Val("arg")) <= 0)
    return -1;
  ct0[i] = 0;  
  return 1;
} 


//= Check whether lift command is done yet.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBallistic::ball_lift_chk (const jhcAliaDesc *desc, int i)
{
  jhcEliLift *f;
  double err;

  // lock to sensor cycle
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;
  f = rwi->lift;

  if (cst[i] <= 0)
  {
    // set up absolute target angle
    camt[i] = f->LiftGoal(camt[i]);
    cerr[i] = f->LiftErr(camt[i]);
    cst[i]  = 1;
  }
  else 
  {
    // check if finished or stuck
    err = f->LiftErr(camt[i]);
    jprintf(2, dbg, "lift: %3.1f, err = %3.1f, stuck = %d\n", f->Height(), err, ct0[i]);
    if (err < f->ldone)
      return 1;
    if (Stuck(i, err, lprog, lstart, lmid))
      return -1;
  }

  // re-issue basic command
  jprintf(1, dbg, ">> REQUEST %d: lift @ %3.1f in\n\n", cbid[i], camt[i]);
  f->LiftTarget(camt[i], csp[i], cbid[i]);
  return 0;
}


//= Read semantic network parts to determine direction to move lift stage
// returns 1 if proper values, -1 for problem

int jhcBallistic::get_up (double& dist, const jhcAliaDesc *act) const
{
  jhcAliaDesc *amt;

  // sanity check
  if (act == NULL)
    return -1;
  dist = lift;

  // possibly go to some extreme
  if ((amt = act->Fact("amt")) != NULL)
    if (amt->WordIn("all the way"))
       dist = 50.0;

  // get direction based on verb
  if (act->WordIn("lower"))
    dist = -dist;
  else if (!act->WordIn("raise"))
    return -1;
  return 1;
}


//= Read semantic network parts to determine speed for lift.
// <pre>
//
// "raise the arm"
//   act-1 -lex-  raise
//         -obj-> obj-3
//   ako-4 -lex-  arm
//         -ako-> obj-3
//   mod-3 -lex-  slowly
//         -mod-> act-1
//
// </pre>
// returns 1 if proper values, -1 for problem

int jhcBallistic::get_vsp (double& speed, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *rate;
  int w = 0;

  // sanity check
  if (act == NULL)
    return -1;

  // look for speed modifier(s)
  speed = 1.0;
  while ((rate = act->Fact("mod", w++)) != NULL)
  {
    if (rate->WordIn("slowly"))
      speed *= slf;            
    else if (rate->WordIn("quickly"))
      speed *= qlf;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                                Gripper                                //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced grip command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBallistic::ball_grip_set (const jhcAliaDesc *desc, int i)
{
  if (get_hand(camt[i], desc->Val("arg")) <= 0)
    return -1;
  ct0[i] = 0;  
  return 1;
}


//= Check whether grip command is done yet.
//    hold state (csp > 0): 0 save pose, 1 width mode start, 2 width mode mid, 3 force mode
// release state (csp < 0): 0 save pose, 1 width mode start, 2 width mode mid
// returns 1 if done, 0 if still working, -1 for failure

int jhcBallistic::ball_grip_chk (const jhcAliaDesc *desc, int i)
{
  jhcEliArm *a;
  const char *act = ((camt[i] < 0.0) ? "hold" : ((camt[i] > 2.0) ? "open" : "close"));
  double err, stop = __max(0.0, camt[i]);

  // lock to sensor cycle
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;
  a = rwi->arm;

  if (cst[i] <= 0)
  {
    // remember initial finger center position
    a->ArmPose(cpos[i], cdir[i]);
    cerr[i] = a->WidthErr(camt[i]);
    cst[i]  = 1;
  }
  else if (cst[i] <= 2)
  {
    // check if target width achieved or stuck
    err = a->WidthErr(stop);
    jprintf(2, dbg, "%s[%d]: width = %3.1f in, force = %3.1f, stuck = %d\n", 
            act, cst[i], a->Width(), a->Squeeze(), ct0[i]);
    if (err < wtol)
      return((camt[i] < 0.0) ? -1 : 1);                // full close = hold fail
    if ((camt[i] < 0.0) && a->SqueezeSome(fhold))
    {
      // if holding, switch to force mode after initial contact 
      ct0[i] = 0;  
      cst[i] = 3;                                  
    }
    if (Stuck(i, err, gprog, gstart, gmid))
      return -1;
  }
  else if (cst[i] >= 3)
  {
    // request force application for a while (always succeeds)
    err = a->SqueezeErr(fhold, 0);
    jprintf(2, dbg, "hold[3]: width = %3.1f, force = %3.1f, good = %d, try = %d\n", 
            a->Width(), a->Squeeze(), ROUND(csp[i]), ct0[i]);
    if (ct0[i]++ >= (UL32) fask)  
      return 1;
  }
          
  // re-issue basic width or force command (keep finger center in same place)
  a->ArmTarget(cpos[i], cdir[i], 1.0, 1.0, cbid[i]);
  if (cst[i] <= 2)
  {
    jprintf(1, dbg, ">> REQUEST %d: %s @ %3.1f in\n\n", cbid[i], act, camt[i]);
    a->WidthTarget(camt[i], 1.0, cbid[i]);
  }
  else if (cst[i] >= 3)
  {
    jprintf(1, dbg, ">> REQUEST %d: hold @ %3.1f oz force\n\n", cbid[i], fhold);
    a->SqueezeTarget(fhold, cbid[i]);
  }
  return 0;
}


//= Read semantic network parts to determine whether to open or close.
// <pre>
//
// "hold this"
//   act-1 -lex-  hold
//         -obj-> obj-3
//
// </pre>
// width = -0.5 for "hold" (force), 0.1 for "close" (width), 3.3 for "open" (width) 
// returns 1 if proper values, -1 for problem

int jhcBallistic::get_hand (double &width, const jhcAliaDesc *act) const
{
  // sanity check
  if ((act == NULL) || (rwi == NULL) || (rwi->body == NULL))
    return -1;
  width = 0.1;

  // get hold status based on main verb
  if (act->WordIn("open", "release"))
    width = (rwi->arm)->MaxWidth();
  else if (act->WordIn("hold"))
    width = -0.5;
  else if (!act->WordIn("close"))
    return -1;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                                Arm                                    //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced arm command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBallistic::ball_arm_set (const jhcAliaDesc *desc, int i)
{
  if ((cst[i] = get_pos(i, desc->Val("arg"))) < 0)
    return -1;
  cerr[i] = cpos[i].LenVec3();       // not accurate for absolute position
  ct0[i] = 0;  
  return 1;
} 


//= Check whether arm command is done yet.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBallistic::ball_arm_chk (const jhcAliaDesc *desc, int i)
{
  jhcMatrix now(4);
  char txt[40];
  jhcEliArm *a;
  double err, zerr;

  // lock to sensor cycle
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;
  a = rwi->arm;

  if (cst[i] <= 0)
  {
    // set up absolute position based on hand direction (pan tilt roll)
    a->ArmPose(now, cdir[i]);
    cpos[i].RotPan3(cdir[i].P());
    cpos[i].IncVec3(now);
    cst[i] = 1;
  }
  else
  {
    // check if finished or stuck
    a->Position(now);
    err = now.PosDiff3(cpos[i]);
    zerr = a->ErrZ(cpos[i]);
    if (cdir[i].W() < 0.0)
      err = __max(err, a->Width());
    jprintf(2, dbg, "hand: %s, err = %3.1f in (%3.1f), stuck = %d\n", now.ListVec3(txt), err, zerr, ct0[i]);
    if ((err < hdone) && (zerr < zdone))
      return 1;
    if (Stuck(i, err, hprog, hstart, hmid))
      return -1;
  }

  // re-issue basic command (arm and wrist are combined, hand separate)
  jprintf(1, dbg, ">> REQUEST %d: hand @ %s\n\n", cbid[i], cpos[i].ListVec3(txt));
  a->ArmTarget(cpos[i], cdir[i], 1.0, 1.0, cbid[i]);
  if (cdir[i].W() < 0.0)
    a->WidthTarget(0.0);
  return 0;
}


//= Read semantic network parts to determine desired new hand postition.
// returns 1 if absolute, 0 if relative to current, -1 for problem

int jhcBallistic::get_pos (int i, const jhcAliaDesc *act) 
{
  jhcEliArm *a;
  jhcAliaDesc *dir;
  int w = 0;

  // sanity check
  if ((act == NULL) || (rwi == NULL) || (rwi->body == NULL))
    return -1;
  a = rwi->arm;

  // absolute position based on main verb
  if (act->WordIn("retract"))
  {
    cpos[i].SetVec3(a->retx, a->rety, a->retz);        
    cdir[i].SetVec3(a->rdir, a->rtip, 0.0, -1.0);    // forced closed
    return 1;
  }
  if (act->WordIn("extend"))
  {
    cpos[i].SetVec3(extx, exty, extz);  
    cdir[i].SetVec3(edir, etip, 0.0, 0.0);           // width unspecified
    return 1;
  }

  // find direction based on modifier(s)
  cpos[i].Zero(1.0);
  while ((dir = act->Fact("dir", w++)) != NULL)
  {
    // get pointing offset (assume hand is along x axis)
    if (dir->WordIn("forward", "forwards"))
      cpos[i].SetX(dxy);
    else if (dir->WordIn("backward", "backwards"))
      cpos[i].SetX(-dxy);

    // get lateral offset
    if (dir->WordIn("left"))
      cpos[i].SetY(dxy);
    else if (dir->WordIn("right"))
      cpos[i].SetY(-dxy);

    // get vertical offset
    if (dir->WordIn("up"))
      cpos[i].SetZ(dz);
    else if (dir->WordIn("down"))
      cpos[i].SetZ(-dz);
  }

  // make sure some valid direction was specified (e.g. not CCW)
  if (cpos[i].LenVec3() == 0.0)
    return -1;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                               Wrist                                   //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced wrist command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBallistic::ball_wrist_set (const jhcAliaDesc *desc, int i)
{
  if ((cst[i] = get_dir(i,  desc->Val("arg"))) < 0)
    return -1;
  cerr[i] = cdir[i].MaxAbs3();
  ct0[i] = 0;
  return 1;
} 


//= Check whether wrist command is done yet.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBallistic::ball_wrist_chk (const jhcAliaDesc *desc, int i)
{
  jhcEliArm *a;
  jhcMatrix now(4);
  char txt[40];
  double err;

  // lock to sensor cycle
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;
  a = rwi->arm;

  if (cst[i] <= 0)
  {
    // set up absolute orientation based on current hand direction (pan tilt roll)
    a->ArmPose(cpos[i], now);
    cdir[i].IncVec3(now);
    cdir[i].CycNorm3();
    cst[i] = 2;
  }
  else if (cst[i] == 1)
  {
    // change zero components to current angles
    a->ArmPose(cpos[i], now);
    cdir[i].SubZero3(now);
    cst[i] = 2;
  }
  else
  {
    // check if finished or stuck
    a->Direction(now);
    err = now.RotDiff3(cdir[i]);
    jprintf(2, dbg, "wrist: %s, err = %3.1f deg, stuck = %d\n", now.ListVec3(txt), err, ct0[i]);
    if (err < wdone)
      return 1;
    if (Stuck(i, err, wprog, wstart, wmid))
      return -1;
  }

  // re-issue basic command (arm and wrist are combined, hand separate)
  jprintf(1, dbg, ">> REQUEST %d: wrist @ %s\n\n", cbid[i], cdir[i].ListVec3(txt));
  a->ArmTarget(cpos[i], cdir[i], 1.0, 1.0, cbid[i]);
  return 0;
}


//= Read semantic network parts to determine desired new hand orientation.
// cdir[i] hold pan and tilt changes, assumes hand is pointing along x
// returns 1 if partial absolute, 0 if relative to current, -1 for problem

int jhcBallistic::get_dir (int i, const jhcAliaDesc *act) 
{
  jhcAliaDesc *dir;
  int w = 0;

  // sanity check
  if (act == NULL)
    return -1;
  cdir[i].Zero();

  // absolute position based on main verb 
  if (act->WordIn("reset"))
  {
    cdir[i].SetT(etip);         
    return 1;                // partial absolute
  }

  // possibly roll some specified direction ("twist")
  if (act->WordIn("twist"))
  {
    if ((dir = act->Fact("dir")) == NULL)
      return -1;
    if (dir->WordIn("counterclockwise", "leftt"))
      cdir[i].SetR(-wroll);
    else if (dir->WordIn("clockwise", "right"))
      cdir[i].SetR(wroll);
    else
      return -1;
    return 0;                // relative
  }

  // possibly get absolute pose for "point"
  if ((dir = act->Fact("dir")) == NULL)
    return -1;
  if (dir->WordIn("vertical"))
  {
    cdir[i].SetT(-90.0);
    return 1;
  }
  if (dir->WordIn("horizontal"))
  {
    // can combine with in-plane angle
    cdir[i].SetT(-0.1);
    if (dir->WordIn("forward", "forwards"))
      cdir[i].SetP(90.0);
    else if (dir->WordIn("sideways"))
      cdir[i].SetP(180.0);
    return 1;                // partial absolute (hence -0.1)
  }

  // find direction based on modifier(s)
  while ((dir = act->Fact("dir", w++)) != NULL)
  {
    // get incremental pan offset
    if (dir->WordIn("left"))
      cdir[i].SetP(wpan);
    else if (dir->WordIn("right"))
      cdir[i].SetP(-wpan);

    // get incremental tilt offset
    if (dir->WordIn("up"))
      cdir[i].SetT(wtilt);
    else if (dir->WordIn("down"))
      cdir[i].SetT(-wtilt);
  }

  // make sure some valid rotation was specified (e.g. not CW)
  if (cdir[i].LenVec3() == 0.0)
    return -1;
  return 0;                  // relative
}


///////////////////////////////////////////////////////////////////////////
//                               Neck                                    //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced neck command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBallistic::ball_neck_set (const jhcAliaDesc *desc, int i)
{
  if (get_gaze(i, desc->Val("arg")) < 0)
    return -1;
  if (get_gsp(csp[i], desc->Val("arg")) < 0)
    return -1;
  ct0[i] = 0;
  return 1;
} 


//= Check whether neck command is done yet.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBallistic::ball_neck_chk (const jhcAliaDesc *desc, int i)
{
  jhcEliNeck *n;
  double err = 0.0;

  // sanity check
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  n = rwi->neck;

  // determine current error (lock to sensor cycle)
  if (!rwi->Accepting())
    return 0;
  if (cdir[i].P() != 0.0)
    err = fabs(cdir[i].P() - n->Pan());
  if (cdir[i].T() != 0.0)
    err = __max(err, fabs(cdir[i].T() - n->Tilt()));

  if (cst[i] <= 0)
  {
    // record initial error
    cerr[i] = err;
    cst[i] = 1;
  }
  else
  {
    // check if finished or stuck
    jprintf(2, dbg, "neck: (%3.1f %3.1f), err = %3.1f deg, stuck = %d\n", n->Pan(), n->Tilt(), err, ct0[i]);
    if (err < ndone)
      return 1;
    if (Stuck(i, err, nprog, nstart, nmid))
      return -1;
  }

  // re-issue basic command (pan and tilt are separate resources)
  jprintf(1, dbg, ">> REQUEST %d: neck @ (%3.1f %3.1f)\n\n", cbid[i], cdir[i].P(), cdir[i].T());
  if (cdir[i].P() != 0.0)
    n->PanTarget(cdir[i].P(), csp[i], cbid[i]);
  if (cdir[i].T() != 0.0)
    n->TiltTarget(cdir[i].T(), csp[i], cbid[i]);
  return 0;
}


//= Read semantic network parts to determine desired new neck orientation.
// cdir[i] holds new (non-zero) pan and tilt values
// returns 1 if okay, -1 for problem

int jhcBallistic::get_gaze (int i, const jhcAliaDesc *act) 
{
  jhcAliaDesc *dir;
  double ntdef = -15.0;
  int w = 0;

  // sanity check
  if (act == NULL)
    return -1;
  cdir[i].Zero();

  // absolute position based on main verb else find direction
  if (act->WordIn("reset"))
  {
    cdir[i].SetVec3(0.1, -15.0, 0.0);       
    return 1;      
  }

  // find direction based on modifier(s)
  while ((dir = act->Fact("dir", w++)) != NULL)
  {
    // get incremental pan offset
    if (dir->WordIn("left"))
      cdir[i].SetP(npan);
    else if (dir->WordIn("right"))
      cdir[i].SetP(-npan);
    else if (dir->WordIn("straight"))
      cdir[i].SetP(0.1);

    // get incremental tilt offset
    if (dir->WordIn("up"))
      cdir[i].SetT(ntilt + ntdef);
    else if (dir->WordIn("down"))
      cdir[i].SetT(-ntilt + ntdef);
    else if (dir->WordIn("level"))
      cdir[i].SetT(-0.1);
  }

  // make sure some valid rotation was specified (e.g. not forward)
  if (cdir[i].LenVec3() == 0.0)
    return -1;
  return 0;        
}


//= Determine speed for gaze shift based on adverbs.
// returns 1 if proper values, -1 for problem

int jhcBallistic::get_gsp (double& speed, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *rate;
  int w = 0;

  // sanity check
  if (act == NULL)
    return -1;

  // look for speed modifier(s)
  speed = 1.0;
  while ((rate = act->Fact("mod", w++)) != NULL)
  {
    if (rate->WordIn("slowly"))
      speed *= sgz;            
    else if (rate->WordIn("quickly"))
      speed *= qgz;
  }
  return 1;
}
