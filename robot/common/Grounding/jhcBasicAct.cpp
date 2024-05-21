// jhcBasicAct.cpp : interface to Manus motion kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#include <math.h>
#include <string.h>

#include "Interface/jms_x.h"           // common video
#include "Interface/jprintf.h"         

#include "Grounding/jhcBasicAct.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcBasicAct::~jhcBasicAct ()
{
}


//= Default constructor initializes certain values.

jhcBasicAct::jhcBasicAct ()
{
  strcpy_s(tag, "BasicAct");
  rwi = NULL;
  rpt = NULL;
  warn = 0;
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Miscellaneous parameters including distance sensing.

int jhcBasicAct::misc_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("basic_misc", 0);
  ps->NextSpecF( &dtrig, 1.5, "Close trigger (in)");  
  ps->NextSpecF( &dtol,  0.5, "Close tolerance (in)");  
  ps->Skip();
  ps->NextSpecF( &ftime, 0.5, "Freeze time (sec)");  
  ps->NextSpecF( &gtime, 2.0, "Grip time (sec)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for translational motion.

int jhcBasicAct::trans_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("basic_trans", 0);
  ps->NextSpecF( &ips,    8.0,  "Translation speed (ips)");  
  ps->NextSpecF( &stf,    0.25, "Slow multiplier");  
  ps->NextSpecF( &qtf,    2.0,  "Fast multiplier");  
  ps->NextSpecF( &step,   4.0,  "Step distance (in)");  
  ps->NextSpecF( &move,   8.0,  "Move distance (in)");  
  ps->NextSpecF( &drive, 16.0,  "Drive distance (in)");  

  ps->NextSpecF( &madj,   2.0,  "Fast move/step adjust (in)");  
  ps->NextSpecF( &dadj,   4.0,  "Fast drive adjustment (in)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for rotation.

int jhcBasicAct::rot_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("basic_rot", 0);
  ps->NextSpecF( &dps,   90.0, "Rotation speed (dps)");  
  ps->NextSpecF( &srf,    0.7, "Slow multiplier");  
  ps->NextSpecF( &qrf,    2.0, "Fast multiplier");  
  ps->NextSpecF( &turn,  90.0, "Turn angle (deg)");  
  ps->NextSpecF( &rot,  180.0, "Rotate angle (deg)");  
  ps->NextSpecF( &spin, 360.0, "Spin angle (deg)");  

  ps->NextSpecF( &radj,   0.8, "Normal adjust factor");
  ps->NextSpecF( &sadj,   0.9, "Slow adjust factor");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for lifting motions.

int jhcBasicAct::lift_params (const char *fname)
{
  jhcParam *ps = &lps;
  int ok;

  ps->SetTag("basic_lift", 0);
  ps->NextSpecF( &zps,  1.5,  "Lift speed (ips)");  
  ps->NextSpecF( &slf,  0.33, "Slow multiplier");  
  ps->NextSpecF( &qlf,  3.0,  "Fast multiplier");  
  ps->NextSpecF( &lift, 1.5,  "Lift distance (in)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcBasicAct::Defaults (const char *fname)
{
  int ok = 1;

  ok &= misc_params(fname);
  ok &= trans_params(fname);
  ok &= rot_params(fname);
  ok &= lift_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcBasicAct::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= mps.SaveVals(fname);
  ok &= tps.SaveVals(fname);
  ok &= rps.SaveVals(fname);
  ok &= lps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Attach physical enhanced body and make pointers to some pieces.

void jhcBasicAct::local_platform (void *soma) 
{
  rwi = (jhcManusRWI *) soma;
}


//= Set up for new run of system.

void jhcBasicAct::local_reset (jhcAliaNote& top)
{
  rpt = &top;
  warn = 0;
  hold = 0;
  dbg = 0;
}


//= Post any spontaneous observations to attention queue.

void jhcBasicAct::local_volunteer ()
{
  dist_close();    // disabled for vision-based action
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcBasicAct::local_start (const jhcAliaDesc& desc, int i)
{
  JCMD_SET(base_stop);
  JCMD_SET(base_drive);
  JCMD_SET(base_turn);
  JCMD_SET(base_lift);
  JCMD_SET(base_grip);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcBasicAct::local_status (const jhcAliaDesc& desc, int i)
{
  JCMD_CHK(base_stop);
  JCMD_CHK(base_drive);
  JCMD_CHK(base_turn);
  JCMD_CHK(base_lift);
  JCMD_CHK(base_grip);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                           Distance Sensor                             //
///////////////////////////////////////////////////////////////////////////

//= Inject NOTE when some object is very close in front.
// only signals once per approach 

void jhcBasicAct::dist_close ()
{
  jhcAliaDesc *n, *n2;
  double dist;
  int w0 = warn;

  // get current distance (lock to sensor cycle)
  if ((rpt == NULL) || (rwi == NULL) || (rwi->body == NULL))
    return;
  if (!rwi->Accepting())
    return;
  dist = (rwi->body)->Distance();

  // apply hysteresis to see if crossed threshold
  if ((w0 <= 0) && (dist < dtrig))
    warn = 1;
  else if ((w0 > 0) && (dist >= (dtrig + dtol)))
    warn = 0;
  if ((w0 > 0) || (warn <= 0))
    return;
 
  // create a new chain containing only a NOTE directive
  if (rpt == NULL)
    return;
  rpt->StartNote();
  n = rpt->NewObj("obj");
  n2 = rpt->NewProp(n, "hq", "close");
  rpt->NewProp(n2, "deg", "very");
  rpt->FinishNote();
}


///////////////////////////////////////////////////////////////////////////
//                             Overall Poses                             //
///////////////////////////////////////////////////////////////////////////

//= Start freeze of translation and rotation.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBasicAct::base_stop0 (const jhcAliaDesc& desc, int i)
{
  ct0[i] += ROUND(1000.0 * ftime);
  return 1;
}


//= Continue freeze of translation and rotation until timeout.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBasicAct::base_stop (const jhcAliaDesc& desc, int i)
{
  // check for timeout then lock to sensor cycle 
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (jms_diff(jms_now(), ct0[i]) > 0)
    return 1;
  if (!rwi->Accepting())
    return 0;

  // re-issue command
  jprintf(1, dbg, "|- BasicAct %d: stop motion\n", cbid[i]);
  (rwi->body)->MoveVel(0.0, cbid[i]);
  (rwi->body)->TurnVel(0.0, cbid[i]);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Translation                              //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced move command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBasicAct::base_drive0 (const jhcAliaDesc& desc, int i)
{
  double off = 0.0;

  // get basic speed and distance
  if (get_vel(csp[i], desc.Val("arg")) <= 0)
    return -1;
  if (get_dist(camt[i], desc.Val("arg")) <= 0)
    return -1;

  // figure out stop time (fudge for trapezoidal)
  if (fabs(csp[i]) > (0.5 * (1.0 + qtf) * ips))  
    off = ((camt[i] > (0.5 * (move + drive))) ? dadj : madj);
  ct0[i] += ROUND(1000.0 * (camt[i] + off) / fabs(csp[i]));
  return 1;
} 


//= Check whether move command is done yet.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBasicAct::base_drive (const jhcAliaDesc& desc, int i)
{
  // check for timeout then lock to sensor cycle 
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (jms_diff(jms_now(), ct0[i]) > 0)
    return 1;
  if (!rwi->Accepting())
    return 0;

  // re-issue command
  jprintf(1, dbg, "|- BasicAct %d: move @ %3.1f ips\n", cbid[i], csp[i]);
  (rwi->body)->MoveVel(csp[i], cbid[i]);
  return 0;
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

int jhcBasicAct::get_vel (double& speed, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *dir, *rate;
  int w = 0;

  // sanity check
  if (act == NULL)
    return -1;

  // get directional modifier of main verb 
  speed = ips;
  if ((dir = act->Fact("dir")) != NULL)
  {
    // see if some standard direction term (checks halo also)
    if (dir->LexIn("backward", "backwards"))
      speed = -speed;
    else if (!dir->LexIn("forward", "forwards"))
      return -1;
  }

  // look for speed modifier(s)
  while ((rate = act->Fact("mod", w++)) != NULL)
  {
    if (rate->LexMatch("slowly"))
      speed *= stf;
    else if (rate->LexMatch("quickly"))
      speed *= qtf;
  }
  return 1;
}


//= Read semantic network parts to determine amount of travel.
// step = 4", move = 8", drive = 16"
// returns 1 if proper values, -1 for problem

int jhcBasicAct::get_dist (double& dist, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *fcn;

  // sanity check
  if (act == NULL) 
    return -1;
  if ((fcn = act->Fact("fcn")) == NULL)
    return -1;

  // set distance based on main verb
  if (fcn->LexMatch("step"))
    dist = step;
  else if (fcn->LexMatch("move"))
    dist = move;
  else if (fcn->LexMatch("drive"))
    dist = drive;
  else if (fcn->LexMatch("cruise"))
    dist = 30.0 * drive;               // nearly continuous (60 sec @ 8 ips)
  else
    return -1;
  
  // possibly change based on explicit request
  set_inches(dist, act->Fact("amt"), 36.0);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                                Rotation                               //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced turn command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBasicAct::base_turn0 (const jhcAliaDesc& desc, int i)
{
  double f = radj;

  // get basic speed and angle
  if (get_spin(csp[i], desc.Val("arg")) <= 0)
    return -1;
  if (get_ang(camt[i], desc.Val("arg")) <= 0)
    return -1;

  // figure out stop time (fudge for trapezoidal)
  if (fabs(csp[i]) < (0.5 * (1.0 + srf) * dps))      // hack for trapezoidal
    f = sadj;
  ct0[i] += ROUND(1000.0 * f * camt[i] / fabs(csp[i]));
  return 1;
} 


//= Check whether turn command is done yet.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBasicAct::base_turn (const jhcAliaDesc& desc, int i)
{
  // check for timeout then lock to sensor cycle 
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (jms_diff(jms_now(), ct0[i]) > 0)
    return 1;
  if (!rwi->Accepting())
    return 0;

  // re-issue command
  jprintf(1, dbg, "|- BasicAct %d: turn @ %3.1f dps\n\n", cbid[i], csp[i]);
  (rwi->body)->TurnVel(csp[i], cbid[i]);
  return 0;
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

int jhcBasicAct::get_spin (double& speed, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *dir, *rate;
  int w = 0;

  // sanity check
  if (act == NULL)
    return -1;

  // get directional modifier of main verb 
  speed = dps;
  if ((dir = act->Fact("dir")) != NULL)
  {
    // see if some standard direction term (checks halo also)
    if (dir->LexIn("clockwise", "right"))
      speed = -speed;
    else if (!dir->LexIn("counterclockwise", "left"))
      return -1;
  }

  // look for speed modifier(s)
  while ((rate = act->Fact("mod", w++)) != NULL)
  {
    if (rate->LexMatch("slowly"))
      speed *= srf;                          // slower than 60 dps stalls
    else if (rate->LexMatch("quickly"))
      speed *= qrf;
  }
  return 1;
}


//= Read semantic network to get amount to rotate.
// turn = 90 degs, rotate = 180 degs, spin = 360 degs
// returns 1 if proper values, -1 for problem

int jhcBasicAct::get_ang (double& ang, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *fcn;

  // sanity check
  if (act == NULL)
    return -1;
  if ((fcn = act->Fact("fcn")) == NULL)
    return -1;
  ang = turn;

  // get angle based on main verb or explicit request
  if (fcn->LexMatch("spin"))
    ang = spin;
  else if (fcn->LexIn("rotate", "turn"))
  {
    if (fcn->LexMatch("rotate"))
      ang = rot;
    set_degs(ang, act->Fact("amt"));   // no limit
  }
  else
    return -1;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                                  Lift                                 //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced lift command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBasicAct::base_lift0 (const jhcAliaDesc& desc, int i)
{
  if (get_vert(csp[i], desc.Val("arg")) <= 0)
    return -1;
  ct0[i] += ROUND(500.0 * sqrt(lift / fabs(csp[i])));
  return 1;
} 


//= Check whether lift command is done yet.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBasicAct::base_lift (const jhcAliaDesc& desc, int i)
{
  // check for timeout then lock to sensor cycle 
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (jms_diff(jms_now(), ct0[i]) > 0)
    return 1;
  if (!rwi->Accepting())
    return 0;

  // re-issue command
  jprintf(1, dbg, "|- BasicAct %d: lift @ %3.1f ips\n\n", cbid[i], csp[i]);
  (rwi->body)->LiftVel(csp[i], cbid[i]);
  return 0;
}


//= Read semantic network parts to determine speed for lift.
// <pre>
//
// "raise the gripper"
//   act-1 -lex-  raise
//         -obj-> obj-3
//   ako-4 -lex-  gripper
//         -ako-> obj-3
//
// </pre>
// returns 1 if proper values, -1 for problem

int jhcBasicAct::get_vert (double& speed, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *fcn, *rate;
  int w = 0;

  // sanity check
  if (act == NULL)
    return -1;
  if ((fcn = act->Fact("fcn")) == NULL)
    return -1;

  // get direction based on verb
  speed = zps;
  if (fcn->LexMatch("lower"))
    speed = -speed;
  else if (!fcn->LexMatch("raise"))
    return -1;

  // look for speed modifier(s)
  while ((rate = act->Fact("mod", w++)) != NULL)
  {
    if (rate->LexMatch("slowly"))
      speed *= slf;            
    else if (rate->LexMatch("quickly"))
      speed *= qlf;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                                  Grip                                 //
///////////////////////////////////////////////////////////////////////////

//= Convert semantic network into a nuanced grip command and request it.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcBasicAct::base_grip0 (const jhcAliaDesc& desc, int i)
{
  if (get_hand(csp[i], desc.Val("arg")) <= 0)
    return -1;
  ct0[i] += ROUND(1000.0 * gtime);
  return 1;
}


//= Check whether grip command is done yet.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcBasicAct::base_grip (const jhcAliaDesc& desc, int i)
{
  // check for timeout then lock to sensor cycle 
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (jms_diff(jms_now(), ct0[i]) > 0)
    return -1;
  if (!rwi->Accepting())
    return 0;

  // if stopped moving see if actually holding something
  if (cst[i] > 0)
    if ((rwi->body)->Stable())
      return(((csp[i] > 0.0) && (rwi->body)->Empty()) ? -1 : 1);

  // re-issue command
  jprintf(1, dbg, "|- BasicAct %d: grip %s\n\n", cbid[i], ((csp[i] > 0.0) ? "CLOSE" : "open"));
  (rwi->body)->Grab((int) csp[i], cbid[i]);
  cst[i] = 1;                                  // mark requested
  return 0;
}


//= Read semantic network parts to determine whether to open or close.
// <pre>
//
// "close the gripper"
//   act-1 -lex-  close
//         -obj-> obj-3
//   ako-4 -lex-  gripper
//         -ako-> obj-3
//
// </pre>
// returns 1 if proper values, -1 for problem

int jhcBasicAct::get_hand (double &grab, const jhcAliaDesc *act) const
{
  const jhcAliaDesc *fcn;

  // sanity check
  if (act == NULL)
    return -1;
  if ((fcn = act->Fact("fcn")) == NULL)
    return -1;
  grab = 1.0;

  // get hold status based on main verb
  if (fcn->LexMatch("open"))
    grab = -1.0;
  else if (!fcn->LexMatch("close"))
    return -1;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                               Utilities                               //
///////////////////////////////////////////////////////////////////////////

//= Digest an explicit positive angle into a number of degrees.
// returns 1 if set "ang", 0 if value unchanged

int jhcBasicAct::set_degs (double& ang, const jhcAliaDesc *amt) const
{
  const jhcAliaDesc *cnt;

  if (amt == NULL)
    return 0;
  if (!amt->LexMatch("degree"))
    return 0;
  if ((cnt = amt->Fact("cnt")) == NULL)
    return 0;
  ang = atoi(cnt->Lex());
  return 1;
} 


//= Digest an explicit positive distance into a number of inches.
// can also limit max value if clip > 0.0
// returns 1 if set "dist", 0 if value unchanged

int jhcBasicAct::set_inches (double& dist, const jhcAliaDesc *amt, double clip) const 
{
  const jhcAliaDesc *cnt;

  // sanity check
  if (amt == NULL)
    return 0;
  if (!amt->LexIn("inch", "foot", "centimeter", "meter"))
    return 0;
 
  // find units of distance
  if (amt->LexMatch("foot"))
    dist = 12.0;
  else if (amt->LexMatch("centimeter"))
    dist = 0.3937;
  else if (amt->LexMatch("meter"))
    dist = 39.37;
  else
    dist = 1.0;

  // multiply by count (if any) but limit value
  if ((cnt = amt->Fact("cnt")) != NULL)
    dist *= atoi(cnt->Lex());
  if (clip > 0.0)
    dist = __min(dist, clip);      
  return 1;     
}
