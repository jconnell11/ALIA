// jhcManipulate.cpp : interface to ELI object manipulation kernel for ALIA system 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021-2025 Etaoin Systems
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
#include "Interface/jprintf.h"

#include "Grounding/jhcManipulate.h"


///////////////////////////////////////////////////////////////////////////
//                             Terminology                               //
///////////////////////////////////////////////////////////////////////////

//= Deposit locations based on directions and spatial relations (follows RNUM).
//                                             0          1          2             3
const char * const jhcManipulate::rel[] = {"between", "left of", "right of", "in front of", 
                                           "behind",  "near",    "next to",  "on",  "down"};
//                                             4          5          6         7      8


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcManipulate::~jhcManipulate ()
{
  delete [] ccnt2;
  delete [] cflag;
  delete [] cref2;
  delete [] cref;
  delete [] citem;
  delete [] cvia;
  delete [] cdir;
  delete [] cpos;
}


//= Default constructor initializes certain values.

jhcManipulate::jhcManipulate ()
{
  int i, n = MaxInst();

  // pool identification
  strcpy_s(tag, "Manipulate");

  // create instance control variables
  cpos = new jhcMatrix [n];
  for (i = 0; i < n; i++)
    cpos[i].SetSize(4);             
  cdir = new jhcMatrix [n];
  for (i = 0; i < n; i++)
    cdir[i].SetSize(4);             
  cvia = new jhcMatrix [n];
  for (i = 0; i < n; i++)
    cvia[i].SetSize(4);             
  citem = new int [n];
  cref  = new int [n];
  cref2 = new int [n];
  cflag = new int [n];
  ccnt2 = new int [n];

  // misc vector sizes
  perr.SetSize(4);
  derr.SetSize(4);
  dest.SetSize(4);

  // body and mind connection
  rwi  = NULL;
  sobj = NULL;
  lift = NULL;
  arm  = NULL;
  base = NULL;
  pos  = NULL;
  dir  = NULL;
  rpt  = NULL;

  // no object in hand currently
  clear_grip(0);

  // processing parameters
  Defaults();
  gok = 1;                   // either 1 or -1
  dbg = 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters controlling gripping location.

int jhcManipulate::grip_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("man_grip", 0);
  ps->NextSpecF( &knob,   0.8, "Slice off for grab (in)");  
  ps->NextSpecF( &mesa,   0.3, "Slice off for deposit (in)");  
  ps->NextSpecF( &choke,  3.0, "Max object size (in)");  
  ps->NextSpecF( &ecc0,   1.3, "Round eccentriciy");           // was 1.2
  ps->NextSpecF( &down,   0.2, "Grab down from top (in)");     // was 1, 0.5, 0.3, then 0
  ps->NextSpecF( &gulp,   0.6, "Center into gripper (in)");    // was 0
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters controlling fingers when gripping.

int jhcManipulate::tip_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("man_tip", 0);
  ps->NextSpecF( &loose,   1.0, "Extra open for grab (in)");
  ps->NextSpecF( &stip,  -30.0, "Sideways grip tilt (deg)");    // -40 for Ganbei
  ps->NextSpecF( &otip,  -30.0, "Overhead grip tilt (deg)");    // -60 for Ganbei
  ps->NextSpecF( &oht,     1.5, "Height for overhead (in)");    
  ps->NextSpecF( &olen,    2.5, "Length for overhead (in)");    
  ps->NextSpecF( &hys,     0.2, "Flip hysteresis (in)");    
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for finding deposit spot.
// jhcSceneVis also has same sdev, tween, buddy, and hood value

int jhcManipulate::spot_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("man_spot", 0);
  ps->NextSpecF( &swell,  1.2, "Expand from ellipse size");
  ps->NextSpecF( &fuzz,   0.5, "Deposit uncertainty (in)");
  ps->NextSpecF( &iwid,   0.5, "Extra side padding (in)");
  ps->NextSpecF( &bias,   0.5, "Bias prox toward current (in)");
  ps->NextSpecF( &sdev,  30.0, "Side zone deviation (deg)");  
  ps->NextSpecF( &tween,  0.3, "Between fraction from middle");

  ps->NextSpecF( &buddy,  1.5, "Adjacent distance wrt size");  
  ps->NextSpecF( &hood,   3.0, "Near distance wrt size");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters governing control of grabbing motions.

int jhcManipulate::ctrl_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("man_ctrl", 0);
  ps->NextSpec4( &park,   10,  "Camera still for detection");    // VisGrok::ign + SmTrack::gone
  ps->NextSpecF( &ttol,   0.2, "Error for straight up (in)");
  ps->NextSpecF( &hold,  12.0, "Holding force (oz)");
  ps->NextSpecF( &wmin,   0.3, "Empty hand width (in)");
  ps->NextSpecF( &wtim,   2.0, "Open/close timeout (sec)");
  ps->NextSpecF( &edge,  20.0, "Tilt to surface edge (deg)");

  ps->NextSpecF( &over,   1.8, "Tip travel height (in)");        // was 1.3 then 1.6
  ps->NextSpecF( &graze,  0.9, "Min grip point height (in)");    // was 1.2 then 1.3 
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters specifying endpoint tolerarance for various phases.

int jhcManipulate::done_params (const char *fname)
{
  jhcParam *ps = &dps;
  int ok;

  ps->SetTag("man_done", 0);
  ps->NextSpec4( &detwang, 3,    "Oscillation wait (cyc)");
  ps->NextSpecF( &ptol,    0.25, "Position tol (in)");
  ps->NextSpecF( &atol,    7.0,  "Wrist tol or ignore (deg)");       // neg to ignore all
  ps->NextSpecF( &wtol,    0.1,  "Grip width tol (in)");
  ps->NextSpecF( &ftol,    2.0,  "Grip force tol (oz)");
  ps->NextSpecF( &cont,    1.5,  "Position continue (in)"); 

  ps->NextSpecF( &ztol,    0.5,  "Under height tol (in)");
  ps->NextSpecF( &dtol,    0.2,  "Deposit drop tol (in)"); 
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters defining optimal workspace for manipulation.

int jhcManipulate::work_params (const char *fname)
{
  jhcParam *ps = &wps;
  int ok;

  ps->SetTag("man_work", 0);
  ps->NextSpecF( &wx1,   5.0, "Right wrt robot (in)");
  ps->NextSpecF( &wx0, -13.0, "Left wrt robot (in)");
  ps->NextSpecF( &wy1,  22.0, "Far wrt robot (in)");       // was 23
  ps->NextSpecF( &wy0,  15.0, "Near wrt robot (in)");      // was 14
  ps->NextSpecF( &wz1,   1.5, "Top wrt shelf (in)");
  ps->NextSpecF( &wz0,  -4.5, "Bottom wrt shelf (in)");

  ps->NextSpecF( &fwd,   3.0, "Shoulder scrape zone (in)");
  ps->NextSpecF( &wcy,   4.0, "Easy angle corner dy (in)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for moving robot to bring target object into workspace.

int jhcManipulate::into_params (const char *fname)
{
  jhcParam *ps = &ips;
  int ok;

  ps->SetTag("man_into", 0);
  ps->NextSpecF( &zup,   0.5, "Overhead margin (in)");
  ps->NextSpecF( &zdn,   0.1, "Underneath margin (in)");  
  ps->NextSpecF( &ztra,  0.2, "Extra vertical lift (in)");
  ps->NextSpecF( &ybd,   1.0, "Extension space margin (in)");   // was 0.5
  ps->NextSpecF( &prow,  1.5, "Robot chest in front (in)");     // positive
  ps->NextSpecF( &ytra,  2.0, "Extra extension move (in)");

  ps->NextSpecF( &xbd,   1.0, "Lateral space margin (in)");
  ps->NextSpecF( &xtra,  5.0, "Extra lateral turn (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcManipulate::Defaults (const char *fname)
{
  int ok = 1;

  ok &= grip_params(fname);
  ok &= tip_params(fname);
  ok &= spot_params(fname);
  ok &= ctrl_params(fname);
  ok &= done_params(fname);
  ok &= work_params(fname);
  ok &= into_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcManipulate::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= gps.SaveVals(fname);
  ok &= tps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= cps.SaveVals(fname);
  ok &= dps.SaveVals(fname);
  ok &= wps.SaveVals(fname);
  ok &= ips.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Attach physical enhanced body and make pointers to some pieces.
// NOTE: must be careful that type cast from void * is valid!

void jhcManipulate::local_platform (void *soma, const char *kind) 
{
  if (strcmp(kind, "jhcVisGrok") != 0)           // type check
    return;
  rwi = (jhcVisGrok *) soma;
  rwi->space2 = &space;                          // for debugging
  sobj = &(rwi->sobj);
  body = rwi->body;
  neck = rwi->neck;
  arm  = rwi->arm;
  lift = rwi->lift;
  base = rwi->base;
  pos  = arm->Position();
  dir  = arm->Direction();
}


//= Set up for new run of system.
// NOTE: sobj->map is wrong size at this point!

void jhcManipulate::local_reset (jhcAliaNote& top)
{
  rpt = &top;
  clear_grip(0);             // nothing in hand
  msg = 0;                   
}


//= Post any spontaneous observations to attention queue.

void jhcManipulate::local_volunteer ()
{
  set_size(sobj->map);       // sobj->map not valid at local_reset
  rpt->Keep(held);           // make sure "held" stays valid
  update_held();
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcManipulate::local_start (const jhcAliaDesc& desc, int i)
{
  JCMD_SET(man_held);
  JCMD_SET(man_wrap);
  JCMD_SET(man_lift);
  JCMD_SET(man_trans);
  JCMD_SET(man_tuck);
  JCMD_SET(man_point);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcManipulate::local_status (const jhcAliaDesc& desc, int i)
{
  JCMD_CHK(man_held);
  JCMD_CHK(man_wrap);
  JCMD_CHK(man_lift);
  JCMD_CHK(man_trans);
  JCMD_CHK(man_tuck);
  JCMD_CHK(man_point);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                          Recurring Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Make sure local images matching overhead depth image.
// no re-allocation if images already correct size

void jhcManipulate::set_size (const jhcImg& ref)
{
  space.SetSize(ref);
  align.SetSize(ref);
  shrink.SetSize(ref);
  path.SetSize(ref);
  mtns.SetSize(ref);
}


//= Make sure object being held is not forgotten and its pose is current.
// "holding" = able to fling around, not just squeezing in hand
// assumes "gulp" and "hang" variables were set by close_fingers()
// Note: object might have useful non-observable semantic features like "mine"

int jhcManipulate::update_held ()
{
  double ang, rads, c, s, wx, wy, wz, ht = lift->Height(), sqz0 = 5.0;
  int dcnt = 5;

  // wait for next sensor cycle 
  if ((rwi == NULL) || rwi->Ghost() || !rwi->Accepting())
    return 0; 
  if (held == NULL)
    return 0;

  // check that object is still being held (allow short bobble)
  if ((arm->Squeeze() >= sqz0) && 
      ((thin > 0) || (arm->Width() >= wmin)))
    drop = 0;
  else if (++drop >= dcnt)
  {
    if (arm->SqueezeGoal() > 0.0)      // see if not intentional
      err_drop(held);                
    else
      msg_hold(held, 1);               // update WMEM with event
    return clear_grip(1);
  }

  // update grasped object pose based on robot arm configuration
  ang = dir->P() + skew;
  rads = D2R * dir->P();
  c = cos(rads);
  s = sin(rads);
  wx = pos->X() + nose * c - left * s;
  wy = pos->Y() + nose * s + left * c;
  wz = (pos->Z() + ht) - hang + 0.5 * sobj->SizeZ(htrk);
  sobj->ForcePose(htrk, wx, wy, wz, ang);

  // preserve visual track 
  sobj->Retain(htrk);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Hand Information                            //
///////////////////////////////////////////////////////////////////////////

//= First call to hand contents reporter but not allowed to fail.
// answers "Am I holding X?" or "What am I holding?"
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_held0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  return 1;
}


//= Report hadn contents in response to either a Y/N or WH- question.
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_held (const jhcAliaDesc& desc, int i)
{
  jhcAliaDesc *obj;

  if ((obj = desc.Val("arg")) != NULL)
    msg_hold(obj, ((obj == held) ? 0 : 1));      // yes/no
  else if (held != NULL)
    msg_hold(held, 0);                           // what
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Motion Sequences                            //
///////////////////////////////////////////////////////////////////////////

//= Start trying to squeeze object with hand.
// instance number and bid already recorded by base class
// assumes not holding anything currently (will likely drop)
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_wrap0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc.Val("arg")) == NULL)
    return -1;
  ccnt2[i] = 0;              // jockey attempts
  return 1;
}


//= Continue trying to squeeze object with hand.
// sets up continuing request to body if not finished
//   cst[i]: 0 = check if graspable 
//           1 = maneuver body into range
//           2 = travel to via point
//           3 = travel to grasp point
//           4 = close fingers on object
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_wrap (const jhcAliaDesc& desc, int i)
{
  int rc = 0;

  // lock to sensor cycle and check update standard cached info
  if (!rwi->Accepting())
    return 0;
  init_vals(i, 0);

  // make sure target is still present, set citem[] to current track 
  if ((citem[i] = sobj->ObjTrack(rpt->VisID(cobj[i], 0))) < 0)
    return err_gone(cobj[i]);
  if (rwi->Ghost())
    return gok;
  if (arm->CommOK() <= 0)
    return err_arm();

  // check for serendipitous grasp or interference
  if (cst[i] <= 4) 
  {
    if (held == cobj[i])
      return 1;                        // lucky
    if (held != NULL)
      fail_clean(NULL);
  }

  // get desired position and orientation based on state
  if (cst[i] <= 0)
    rc = assess_obj();
  else if (cst[i] == 1)
    rc = jockey(0);                    // action point = src
  else if (cst[i] == 2)
    rc = goto_via(0);
  else if (cst[i] == 3)
    rc = goto_grasp();
  else if (cst[i] == 4)                // last grasp
    rc = close_fingers();
  else if (cst[i] == 5)                // success
    return 1;

  // cleanup from failure
  if (cst[i] >= 20)             
    return stow_arm(-1);               // joint mode
  return command_bot(rc);
}


//= Start trying to securely raise held object above surface.
// instance number and bid already recorded by base class
// required: look at dest, look at src, others optional
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_lift0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc.Val("arg")) == NULL)
    return -1;
  ccnt2[i] = 0;              // jockey attempts
  return 1;
}


//= Continue trying to securely raise held object above surface.
// sets up continuing request to body if not finished
//   cst[i]: 0 = check if graspable 
//           1 = maneuver body into range
//           2 = travel to via point
//           3 = travel to grasp point
//           4 = close fingers on object
//         * 5 = lift object off surface
// repeats most of man_wrap but skips forward if already acquired
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_lift (const jhcAliaDesc& desc, int i)
{
  int rc = 0;

  // lock to sensor cycle and update standard cached info
  if (!rwi->Accepting())
    return 0;
  init_vals(i, 0);

  // make sure target is still present, set citem[] to current track 
  if ((citem[i] = sobj->ObjTrack(rpt->VisID(cobj[i], 0))) < 0)
    return err_gone(cobj[i]);
  if (rwi->Ghost())
    return gok;
  if (arm->CommOK() <= 0)
    return err_arm();

  // check for serendipitous grasp or interference
  if (cst[i] <= 4)                   
  {
    if (held == cobj[i])    
    {         
      cst[i]  = 5;                     // skip ahead
      cst2[i] = 0;
    }
    else if (held != NULL)
      fail_clean(NULL);
  }
  else if (cst[i] == 6)                // should have object
    if (held != cobj[i])
      fail_clean(cobj[i]);

  // get desired position and orientation based on state
  if (cst[i] <= 0)
    rc = assess_obj();
  else if (cst[i] == 1)
    rc = jockey(0);                    // src action point
  else if (cst[i] == 2)
    rc = goto_via(0);
  else if (cst[i] == 3)
    rc = goto_grasp();
  else if (cst[i] == 4)                // last grasp
    rc = close_fingers();
  else if (cst[i] == 5)                
    rc = lift_off();
  else if (cst[i] == 6)                // success
    return 1;

  // cleanup from failure
  if (cst[i] >= 20)
    return stow_arm(-1);               // joint mode
  return command_bot(rc);
}


//= Start trying to move already held object to some location.
// only functions on semnet allowed, cannot interrogate vision system
// binds moving object to cobj[i] and destination place to cspot[i]
// sets cmode[i] to be relation number (fails if no "arg2")
// returns 1 if okay, -1 for interpretation error
// NOTE: using sequence of grab then move might occlude destination

int jhcManipulate::man_trans0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  // argument parsing (cannot put something on itself)
  if (((cobj[i] = desc.Val("arg")) == NULL) || 
      ((cspot[i] = desc.Val("arg2")) == NULL))
    return -1;
  if ((cmode[i] = txt2rnum(cspot[i]->Lex())) < 0)
    return -1;
  if (cmode[i] < 0)
    return -1;

  if ((cmode[i] == ON) && (cspot[i]->Val("ref") == cobj[i]))
    return -1;

  // initialize state
  citem[i] = -1;             // await access to vision
  cref[i]  = -1;
  cref2[i] = -1;
  ccnt2[i] = 0;              // jockey attempts
  return 1;
}


//= Continue trying to move already held object to some location.
// assumes cmode[i] has relation and cspot[i] has destination description
// sets cref[i] and cref2[i] to be tracks for reference objects (if any)
//   cst[i]: 0 = check for reasonable deposit spot
//           1 = maneuver body into range
//           2 = transfer to over destination
//           3 = lower to destination height
//           4 = release object
//           5 = retract to stowed location
//           6 = avoid protruding joints
// returns 1 if done, 0 if still working, -1 for failure
// NOTE: calls assess_spot while holding object so destination may be occluded

int jhcManipulate::man_trans (const jhcAliaDesc& desc, int i)
{
  int rc = 0;

  // lock to sensor cycle and update standard cached info
  if (!rwi->Accepting())
    return 0;
  init_vals(i, 1);

  // make sure target and reference object(s) are still present
  // set citem[] to held object track, cref[] to deposit object track
  if (cst[i] <= 3)
  {
    if ((citem[i] = sobj->ObjTrack(rpt->VisID(cobj[i], 0))) < 0)
      return err_gone(cobj[i]);
    if (ref_tracks(cref[i], cref2[i], cmode[i], cspot[i]) < 0)   
      return -1;                       // invokes err_gone() also
  }
  if (rwi->Ghost())
    return gok;
  if (arm->CommOK() <= 0)
    return err_arm();

  // check for accidental drop
  if (cst[i] <= 3)
    if (held != cobj[i])
      fail_clean(cobj[i]);

  // get desired position and orientation based on state
  if (cst[i] <= 0)
    rc = assess_spot();                // sets cpos, cdir, cvia
  else if (cst[i] == 1)                
    rc = jockey(1);                    // dest action point
  else if (cst[i] == 2)                
    rc = xfer_over();
  else if (cst[i] == 3)
    rc = place_on();
  else if (cst[i] == 4)
    rc = release_obj();                // held -> NULL
  else if (cst[i] == 5)
    return stow_arm(1);                // joint mode

  // cleanup from failure
  if (cst[i] >= 20)
    return stow_arm(-1);               // joint mode
  return command_bot(rc);
}


//= Start trying to retract the arm to travel position.
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_tuck0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  // initialize state
  cobj[i] = held;
  return 1;
}


//= Continue trying to retract arm to travel position.
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_tuck (const jhcAliaDesc& desc, int i)
{
  // lock to sensor cycle and update standard cached info
  if (rwi->Ghost())
    return gok;
  if (!rwi->Accepting())
    return 0;
  if (arm->CommOK() <= 0)
    return err_arm();
  init_vals(i, 0);

  // get desired position and orientation based on state
  if (stow_arm(1) == 0)                // joint mode
    return 0;
  if (held == cobj[i])                 // check if still holding
    return 1;
  err_lack(cobj[i]);
  return -1;
}


//= Start trying to indicate an object with the hand.
// only functions on semnet allowed, cannot interrogate vision system
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_point0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  // argument parsing
  if ((cobj[i] = desc.Val("arg")) == NULL)
    return -1;

  // intialize state
  citem[i] = -1;             // await access to vision
  ccnt2[i] = 0;              // jockey attempts
  return 1;
}


//= Continue trying to indicate an object with the hand.
// sets up continuing request to body if not finished
//   cst[i]: 0 = check current position
//           1 = maneuver body into range
//           2 = travel to via point
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_point (const jhcAliaDesc& desc, int i)
{
  int rc = 0;

  // lock to sensor cycle and check update standard cached info
  if (!rwi->Accepting())
    return 0;
  init_vals(i, 0);

  // make sure target object still is present, set citem[] to current track
  if ((citem[i] = sobj->ObjTrack(rpt->VisID(cobj[i], 0))) < 0)
    return err_gone(cobj[i]);
  if (rwi->Ghost())
    return gok;
  if (arm->CommOK() <= 0)
    return err_arm();

  // move fingertips to via position over object
  if (cst[i] <= 0)
    rc = assess_obj();
  else if (cst[i] == 1)
    rc = jockey(0);                    // action point = src
  else if (cst[i] == 2)
    rc = goto_via(1);
  else if (cst[i] == 3)                // success
    return 1;
  else
    return -1;                         // failure at some step

  // drive arm and base, possibly transition to next state
  return command_bot(rc);
}


///////////////////////////////////////////////////////////////////////////
//                              Take Phases                              //
///////////////////////////////////////////////////////////////////////////

// Look for target object and set up motion vectors for grabbing. 
// want reliable estimate of object grasp point within workspace
// assumes cobj[] is target object semnet node, citem[] is current track
// Sets:  cpos[] = absolute grasp location
//        cdir[] = gripper orientation
//        cvia[] = absolute trajectory point over object
//        camt[] = object width (negative if top is roundish)
// Uses:  ccnt[] = failed blob detections
//        caux[] = initial gripper width
// returns 1 if done, 0 if still working, -1 for timeout

int jhcManipulate::assess_obj ()
{
  double da, mix = 0.1, gtol = 5.0;
  int rc, tries = 5;

  // announce entry and zero failed dectection count
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: assess grab %s\n", bid, cobj[inst]->Nick());
    cst2[inst] = 1;
    caux[inst] = arm->Width();         // remember finger separation
    ct0[inst] = 0;                     // gaze timeout (chk_stuck)
    ccnt[inst] = 0;                    // missed detections
    if (ccnt2[inst] <= 0)
      sobj->World(cpos[inst], citem[inst]);      // general vicinity   
  } 

  // extra command info
  wid = caux[inst];                    // keep gripper the same

  // look at target grasp point (or object to be picked up) 
  da = neck->GazeErr(cpos[inst], lift->Height());
  if (da > gtol)
  {
    if (chk_stuck(mix * da, 1.0) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: gaze = %3.1f\n", da);  
  }

  // make sure object is actively detected 
  ct0[inst] = 0;                                 // reset gaze timeout (chk_stuck)
  stare = 1;
  if ((rc = src_visible()) > 0)                  
  {
    // try determine exact grasp point
    if (update_src(camt[inst], cpos[inst], cdir[inst], cvia[inst]) <= 0)
      if (ccnt2[inst] <= 0)                      // must find originally
        return -1;
    return 1;
  }   
  if ((rc < 0) && (ccnt[inst]++ >= tries))       // can wait several cycles
    return -1;                                   // required
  return 0;
}


//= Drive, turn, or lift to bring hand action point into valid workspace zone.
// fixes one body error at a time: Z then X then finally Y
// assumes "gap" variable already has allowable travel distance if fix = 2
// Uses:  ccnt2[] = total number of jockey phases attempted
//         caux[] = initial grasp force or finger separation
// returns 1 if done, 0 if still working, -1 for timeout, -2 for re-assess

int jhcManipulate::jockey (int dest)
{
  double err, ex = cpos[inst].X(), ey = cpos[inst].Y(), ez = cpos[inst].Z();
  int fix, flail = 8;

  // announce entry
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: jockey %s %s\n", 
            bid, ((dest <= 0) ? "grab" : "dest"), cobj[inst]->Nick());
    cst2[inst] = 1;
    caux[inst] = ((dest > 0) ? -hold : arm->Width());
    cflag[inst] = 0;    
    ct0[inst] = 0;                     // move timeout (chk_stuck)
    ccnt2[inst] += 1;                  // started another fixing round
  } 

  // find most important violations of workspace
  fix = chk_outside(cflag[inst], ex, ey, ez);
  if (fix == 0)
    return 1;
  if (fix < 0)
  {
    if (ccnt2[inst] >= flail)
      return -1;                       // too many attempts
    return -2;                         // just resolved so re-assess
  }

  // try to get location into arm workspace by moving body
  wid = caux[inst];
  if ((err = adj_workspace(fix, ex, ey, ez)) < 0.0)
    return -1;
  if (chk_stuck(err, 2.0) > 0)
    return -1;
  return 0;
}


//= Move hand to via point appropriate for object.
// can update target grasp parameters if object currently detected
// returns 1 if done, 0 if still working, -1 for timeout

int jhcManipulate::goto_via (int pt)
{ 
  double err, dp, da, dw;

  // possibly print entry message 
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: approach %s\n", bid, cobj[inst]->Nick());
    cst2[inst] = 1;
    ct0[inst] = 0;                               // move timeout (chk_stuck)
  }

  // possibly track moving object while reaching for it
  if (src_visible() > 0)
    update_src(camt[inst], cpos[inst], cdir[inst], cvia[inst]);

  // extra command parameters
  wid = ((pt > 0) ? 0.0 : fabs(camt[inst]));     // open to object size plus some
  dmode = 0x4;                                   // exact R orientation (0100)

  // see if close enough to desired pose
  err = pose_err(dp, da, 1);
  dw = arm->WidthErr(wid);
  if (atol < 0.0)                               // possibly ignore wrist
    da = derr.Zero();
  if ((dp > ptol) || (da > fabs(atol)) || (dw > wtol))
  {
    // fail if not making progress unless in right ballpark (hope for best)
    if (chk_stuck(err + dw, 1.0) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: perr = %s, derr = %s, werr = %3.1f\n", perr.ListVec3(txt), derr.ListVec3(txt2), dw);
    if (arm->PosOffset3D(cvia[inst], 0.0) > cont)
      return err_reach(cobj[inst]);
  }

  // wait for oscillation to subside then possibly tell final accuracy
  if ((detwang >= 0) && (arm->Static() < detwang))
  {
    if (arm->Static() == (detwang - 1))
      jprintf(2, dbg, "    detwang ...\n");
    return 0;
  }
  return final_pose(1);
}


//= Move hand to grasp point appropriate for object.
// returns 1 if done, 0 if still working, -1 for timeout

int jhcManipulate::goto_grasp ()
{ 
  double err, dp, da;

  // possibly print entry message 
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: engulf %s\n", bid, cobj[inst]->Nick());
    cst2[inst] = 1;
    ct0[inst] = 0;                     // descend timeout (chk_stuck)
  }

  // extra command parameters
  wid = fabs(camt[inst]);              // open to object size plus some
  pmode = 0x3;                         // exact YX position (011)
  dmode = 0x4;                         // exact R orientation (0100)

  // see if close enough to desired pose
  err = pose_err(dp, da, 0);
  if ((dp > ptol) || (da > fabs(atol)))
  {
    // fail if not making progress unless in right ballpark (hope for best)
    if (chk_stuck(err, 0.5) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: perr = %s, derr = %s\n", perr.ListVec3(txt), derr.ListVec3(txt2));
    if (arm->PosOffset3D(cpos[inst], lift->Height()) > cont)
      return err_reach(cobj[inst]);
  }

  // possibly tell final accuracy
  return final_pose(1);
}


//= Close fingers around object until standard force achieved.
// sets "held", "htrk", "nose", "hang", and "thin" once hold is established
// returns 1 if done, 0 if still working, -1 if fingers empty or timeout

int jhcManipulate::close_fingers ()
{
  // possibly print entry message and remember start time (wtim not chk_stuck) 
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: squeeze %s\n", bid, cobj[inst]->Nick());
    cst2[inst] = 1;         
    fcnt = 0;                          // number of squeezing cycles 
    ct0[inst] = jms_now();             // closure timeout (jms_elapsed)
  }

  // extra command parameters
  wid = 0.0;                           // ballistically pinch (not force)
  pmode = 0x4;                         // exact Z position (100)
  dmode = 0x4;                         // exact R orientation (0100)

  // succeed if moderate force for several cycles
  fcnt++;
  if ((hold - arm->Squeeze()) > ftol) 
    fcnt = 0;
  if (fcnt < 5)
  {
    // quit if motion takes too long
    if (jms_elapsed(ct0[inst]) < wtim)
      return 0;
    jprintf(2, dbg, "    stuck: timeout\n");
    return -1;
  }

  // remember engagement details and generate "holding" event
  record_grip(citem[inst]);
  wt = -1.0;                           // not measured yet
  if (held == NULL)                    // might be second call
  {
    held = cobj[inst];
    msg_hold(cobj[inst], 0);           // successfully holding
  }
  return final_pose(0);
}


//= Raise grasped object slightly off table to allow moving.
// sets "wt" to estimate of weight of held object
// returns 1 if done, 0 if still working, -1 if timeout (non-movable?)

int jhcManipulate::lift_off ()
{
  double under, z3d = pos->Z() + lift->Height();   

  // remember starting absolute pose 
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: lift %s\n", bid, cobj[inst]->Nick());
    cst2[inst] = 1;
    cpos[inst].SetZ(over + z3d);         // slightly up from grasp location
    cdir[inst].Copy(*dir);               // maintain current orientation
    ct0[inst] = 0;                       // lift timeout (chk_stuck)
  }

  // extra command parameters
  wid = -hold;                           // maintain force
  dmode = 0xE;                           // any pan, exact RT orientation (1110)

  // see if high enough yet (ignore any other errors)
  under = cpos[inst].Z() - z3d;
  if (under > ztol)
  {
    if (chk_stuck(under, 1.0) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: zerr = %3.1f\n", -under);
    final_pose(0);
    return -1;
  }

  // estimate weight and record held object
  if (wt < 0.0)
  {
    wt = arm->ObjectWt();         
    jprintf(2, dbg, "    wt = %3.1f oz\n", wt);
  }
  return final_pose(0);
}


///////////////////////////////////////////////////////////////////////////
//                              Move Phases                              //
///////////////////////////////////////////////////////////////////////////

//= Determine an appropriate ending location to move the object to.
// want reliable estimate of object-relative deposit location within workspace
// assumes cmode[i] is spatial relation, cref[i] is reference object track
// Sets:  cpos[] = absolute deposit location
//        cdir[] = gripper orientation
//        cvia[] = absolute trajectory point over destination
// Uses:  ccnt[] = failed blob detections
// returns 1 if done, 0 if still working, -1 for timeout

int jhcManipulate::assess_spot ()
{
  double da, mix = 0.1, gtol = 5.0;
  int rc, tries = 5;

  // possibly announce entry and clear state
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- manipulate %d: assess dest %s\n", bid, cobj[inst]->Nick());
    cst2[inst] = 1;
    ccnt[inst] = 0;                    // missed detections
    ct0[inst] = 0;                     // gaze timeout (chk_stuck)
    if (ccnt2[inst] <= 0)
      anchor_loc(cpos[inst]);          // general vicinity of deposit
  }

  // extra command parameters
  wid = -hold;                         // always transporting something

  // try to look at deposit location (or reference location)
  da = neck->GazeErr(cpos[inst], lift->Height());
  if (da > gtol)
  {
    if (chk_stuck(mix * da, 1.0) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: gaze = %3.1f\n", da);
  }

  // make sure reference object(s) is actively detected (can wait several cycles)
  ct0[inst] = 0;                                 // gaze timeout (chk_stuck)
  stare = 1;
  if ((rc = dest_visible()) > 0)                 
  {
    // try to determine exact deposit location
    if (update_dest(cpos[inst], cdir[inst], cvia[inst], 1) <= 0)
      if (ccnt2[inst] <= 0)
        return -1;                               // must find originally
    return 1;
  }
  if ((rc < 0) && (ccnt[inst]++ >= tries))       // can wait several cycles
    return -1;                                   // required to be visible     
  return 0;
}


//= Move object at travel height over to destination location.
// can update deposit location if reference object currently detected
// returns 1 if done, 0 if still working, -1 if timeout 

int jhcManipulate::xfer_over ()
{
  jhcMatrix full(4), anchor(4);
  double cont = 0.3;

  // possibly announce entry 
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: transfer %s\n", bid, cobj[inst]->Nick());
    cst2[inst] = 1;
  }

  // possibly track reference object(s) while moving arm (drifts if dest = "down")
  if (dest_visible() > 0)
    update_dest(cpos[inst], cdir[inst], cvia[inst], 1);

  // extra command parameters
  wid = -hold;                         // keep squeezing
  pmode = 0x4;                         // exact Z position (0100)
  dmode = 0x6;                         // exact RT orientation (0110)

  // see if at destination position yet (ignore any orientation error)
  if (arm->PosOffset3D(cvia[inst], 0.0) > cont)
  {
    // if not making progress then fail 
    arm->PosErr3D(perr, cvia[inst], 0.0, 0);
    if (chk_stuck(perr.SumAbs3(), 5.0) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: perr = %s\n", perr.ListVec3(txt));
    return err_reach(cspot[inst]);
  }

  // wait for oscillation to subside then possibly tell final accuracy
  if ((detwang >= 0) && (arm->Static() < detwang))
  {
    if (arm->Static() == (detwang - 1))
      jprintf(2, dbg, "    detwang ...\n");
    return 0;
  }
  return final_pose(1);
}


//= Descend toward destination height until upwards force felt.
// returns 1 if done, 0 if still working (never fails)

int jhcManipulate::place_on ()
{
  double dx, dy, dz, ht = lift->Height();

  // possibly announce entry
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: deposit %s\n", bid, cobj[inst]->Nick());
    cst2[inst] = 1;
  }

  // extra command parameters
  wid = -hold;               // keep squeezing
  pmode = 0x3;               // exact YX position (0011)
  dmode = 0x7;               // exact RTP orientation (0111)

  // see if approximately in contact with surface
  dx = pos->X() - cpos[inst].X();
  dy = pos->Y() - cpos[inst].Y();
  dz = (pos->Z() + ht) - cpos[inst].Z();
  if ((fabs(dx) > ptol) || (fabs(dy) > ptol) || (dz > dtol))
  {
    // quit if no longer making progress
    if (chk_stuck(dz + fabs(dx) + fabs(dy), 0.5) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: perr = [%3.1f %3.1f %3.1f]\n", dx, dy, dz);
  }

  // possibly tell final accuracy
  return final_pose(1);
}


//= Open fingers wide to release object.
// sets "held" to NULL 
// returns 1 if done, 0 if still working

int jhcManipulate::release_obj ()
{
  // remember desired width and start time (wtim not chk_stuck)
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: release %s\n", bid, cobj[inst]->Nick());
    cst2[inst] = 1;
    ct0[inst] = jms_now();             // release timeout
    held = NULL;                       // prevent drop detection
  }

  // extra command parameters
  wid = sobj->Minor(citem[inst]) + loose;
  pmode = 0x7;                         // exact ZYX position (0111)
  dmode = 0x7;                         // exact RTP orientation (0111)

  // see if hand is open wide enough yet
  if (fabs(arm->Width() - wid) > wtol)
  {
    // quit if motion takes too long but never fail
    if (jms_elapsed(ct0[inst]) < wtim)
      return 0;
    jprintf(2, dbg, "    stuck: timeout\n");
  }

  // generate "not holding" event and mark hand as empty
  msg_hold(cobj[inst], 1);
  cobj[inst] = NULL;
  clear_grip(1);
  return final_pose(0);
}


//= Return arm to tucked configuration suitbale for body navigation.
// directly drives arm in joint mode so disable drive in command_bot()
// Uses:  caux[] = initial finger separation
// returns 1 if done, 0 if still working, -1 if fail marked (rc < 0)

int jhcManipulate::stow_arm (int rc)
{
  double da, mix = 0.1;

  // remember outer arm configuration (but change elbow if stow failed)
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: stow %s\n", bid, 
            ((cobj[inst] != NULL) ? cobj[inst]->Nick() : "arm"));
    cst2[inst] = 1;  
    caux[inst] = arm->Width();         // initial finger separation
    ct0[inst] = 0;                     // pose timeout (chk_stuck)
  }

  // only close hand when high enough
  if (held != NULL)
    wid = -hold;
  else if ((arm->retz - pos->Z()) > ztol)        
    wid = caux[inst];
  else
    wid = arm->wlax;

  // check if tucked yet
  da = arm->TuckErr();
  if (da > arm->AngTol())
  {
    // quit if not making progress
    if (chk_stuck(mix * da, 0.5) <= 0)
    {
      // send joint angles to arm and close hand (no gaze command)
      arm->Tuck(1.0, bid);
      arm->HandTarget(wid, 1.0, bid);                              
      return 0;
    }
    jprintf(2, dbg, "    stuck: da = %3.1f\n", da);
  }

  // return value given, possibly advancing state if normal sequence
  if (rc <= 0)
    return rc;
  cst[inst] += 1;            // largely for GUI
  return 1;                         
}


///////////////////////////////////////////////////////////////////////////
//                           Sequence Helpers                            //
///////////////////////////////////////////////////////////////////////////

//= Set up some standard control values.

void jhcManipulate::init_vals (int i, int keep)
{
  double pan = cdir[i].P();

  // current instance information
  inst = i;                    
  msg = 0;                     

  // default command details
  sp = csp[i];    
  bid = cbid[i];          
  pmode = 0;                
  dmode = 0;   
  stare = 0;

  // default update target, via, and grasp direction using odometry
  // these will be overridden if anchor objects are visible
  base->AdjustTarget(cpos[i]);
  base->AdjustTarget(cvia[i]);
  cdir[i].SetP(base->AdjustAng(pan));
}


//= Detect lack of substantial error reduction over given time.
// detects lack of improvement over "prog" for "tim" (about 15 cycles)
// hardcoded for 0.1" position progress, otherwise scale error first 
// assumes member variable "inst" bound with proper command index
// uses cerr[] and ct0[] to monitor progress across calls
// returns 1 if at asymptote, 0 if still moving toward goal

int jhcManipulate::chk_stuck (double err, double tim)
{
  double chg = cerr[inst] - err, prog = 0.1;     

  if ((ct0[inst] == 0) || (chg >= prog))
  {
    // reset timer if minimal progress has been made
    ct0[inst] = jms_now();
    cerr[inst] = err;
  }
  else if (jms_elapsed(ct0[inst]) > tim)
    return 1;
  return 0;
}


//= Tell arm command versus actual pose at the end of some step.
// can also give details of position error in 3D
// always returns 1 for convenience

int jhcManipulate::final_pose (int xyz) 
{  
  const jhcMatrix *cmd = ((cst[inst] < 3) ? &(cvia[inst]) : &(cpos[inst]));
  double ht = lift->Height();
    
  jprintf(3, dbg, "      command: %s %s\n", cmd->ListVec3(txt), cdir[inst].ListVec3(txt2));
  jprintf(3, dbg, "      -> pose: [%3.1f %3.1f %3.1f] %s\n", pos->X(), pos->Y(), pos->Z() + ht, dir->ListVec3(txt));
  if ((xyz > 0) && (dbg >= 2))
  {
    arm->PosErr3D(perr, *cmd, ht, 0);
    jprintf("    final offset = %s\n", perr.ListVec3(txt)); 
  }
  return 1;
}


//= Fail sequence by transitioning to the cleanup phase.
// opriotnally generates a failure event about holding the wrong object
// always returns 0 for convenience

int jhcManipulate::fail_clean (jhcAliaDesc *obj)
{
  if (obj != NULL)
    err_lack(obj);
  cst[inst]  = 20;           // jump to stow_arm()
  cst2[inst] = 0;            // mark as newly started
  ct0[inst] = 0;       
  cflag[inst] &= 0xF0;       // no body fixes required      
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                          Coordinated Motion                           //
///////////////////////////////////////////////////////////////////////////

//= Find current deviation from desired hand position and orientation.
// binds "perr" and "derr" vectors of component-wise signed differences
// binds max absolute value of xyz deviations (dp) and ptr deviations (da)
// returns wieghted SUM of absolute positional and angular coordinate errors

double jhcManipulate::pose_err (double& dp, double& da, int via) 
{
  double psum, dsum, dt, dr, mix = 0.1;

  // signed positional xyz error (via or action location)
  if (via > 0)
    dp = arm->PosErr3D(perr, cvia[inst], lift->Height(), 0);
  else
    dp = arm->PosErr3D(perr, cpos[inst], lift->Height(), 0);
  psum = perr.SumAbs3();

  // signed angular pan-tilt-roll error (possibly ignore pan)
  da = arm->DirErr(derr, cdir[inst], 0);
  dsum = derr.SumAbs3();
  if (atol < 0.0)                      // leave derr intact
  {
    dt = fabs(derr.T());
    dr = fabs(derr.R());
    da = __max(dt, dr);
    dsum = dt + dr;
  }

  // return Manhattan distance from goal in 6D
  return(psum + mix * dsum);
}


//= Sets up arm and gaze commands based on FSM state from cst[].
// generally moving arm toward cpos[] or cvia[] depending on state
// additional info passed through vars "wid", "pmode", and "dmode"
// handles state transitions and exits based on return code "rc"
// <pre>
//
//   state                   gaze     arm
//   -----                  ------  ------
//     0    assess_obj       obj       x     
//     1    jockey           obj       x
//     2    goto_via        (obj)     via
//     3    goto_grasp        -      grasp
//     4    close_fingers     -      grasp
//     5    lift_off          -      grasp+ 
//
//     0    assess_spot      spot      x     
//     1    jockey           spot      x
//     2    xfer_over       (spot)    via
//     3    place_on          -       dump
//     4    release_obj       -       dump
//
//    20    stow_arm          -       stow
//
// </pre>

int jhcManipulate::command_bot (int rc)
{
  jhcMatrix rise(4);
  const jhcMatrix *loc = &(cpos[inst]);
  double ht = lift->Height(); 
  int state = cst[inst];
  
  // cleanup on FSM fail (stow never fails)
  if (rc == -1)
    return fail_clean(NULL); 

  // gaze at dest for assess_X() + jockey() + goto_via()/xfer_over()
  if ((state <= 2) && (stare <= 0))     
    neck->GazeAt(cpos[inst], ht, 1.0, bid);   

  // always control fingers (need to hold during assess_spot)
  arm->HandTarget(wid, sp, bid);                              

  // directly commmand arm for assess_X() + jockey() + stow_arm() 
  if ((state >= 2) && (state <= 20))
  {
    if (state == 2)                    // goto_via() or xfer_over()                                        
      loc = &(cvia[inst]);       
    if ((loc->Z() - pos->Z()) > ttol)
    {
      // jump straight up if arm too low
      rise.SetVec3(pos->X(), pos->Y(), loc->Z());          
      loc = &rise;  
    }
    arm->PosTarget(*loc, sp, bid, pmode);
    arm->DirTarget(cdir[inst], sp, bid, dmode);
  }

  // possibly shift to different sequence state on following cycle 
  if ((rc >= 1) || (rc <= -2))
  {
    // assess location again if workspace aligned, else advance to next step
    if (rc <= -2)            
      cst[inst] = 0;         
    else   
      cst[inst] += 1;        

    // mark as newly started: no failed detections, no workspace violations
    cst2[inst] = 0;          
    ct0[inst]  = 0;    
    ccnt[inst] = 0;          
    cflag[inst] &= 0xF0;   
  }
  return 0;                                                // FSM continue
}


//= Figure which workspace violation to fix or continue working on.
// fix lower bits: 6 = +z, 5 = -z, 4 = +x, 3 = -x, 2 = +y, 1 = -y, 0 = none
// fix bit 3 set if change from last mode (only in "old" not return value)
// takes previous primary violation for hysteresis
// worksp: 0 = ignore all, 1 = only up/dn, 2 = all violations
// vertical limits shifted during worksp = 1 (stow_arm step)
// returns updated value of which violation to fix (also alters "old")
// also sets "gap" variable with allowable forward distance if fix = 2
// NOTE: cflag[i] contains other data, only lower 4 bits are workspace related

int jhcManipulate::chk_outside (int& old, double gx, double gy, double gz) 
{
  char prob[7][20] = {"okay for arm", "backoff", "advance", "swivel left", "swivel right", "lower", "raise"};
  double ztop = ((pos->Y() < (arm->rety + fwd)) ? arm->retz : wz1);                        
  int lok = lift->CommOK(), prev = old & 0x07, fix = prev, bad = 0;

  // find current violations in priority order
  if ((gz > ztop) && (lok > 0))                  // too high
    bad = 6;                 
  else if ((gz < wz0) && (lok > 0))              // too low
    bad = 5;                 
  else if (gy < wy0)                             // too close
    bad = 1; 
  else if (gx > wx1)                             // too far right
    bad = 4;                 
  else if (gx < wx0)                             // too far left
    bad = 3;                 
  else if (gy > wy1)                             // too far
//    if ((gap = surf_gap()) > 0.0)                
    if ((gap = (rwi->nav).Forward()) > 0.0)      // only set if movement possible
      bad = 2;                 

  // see if old violation resolved (or remove if being ignored)
  if (((fix == 6) && (gz < (ztop - zup))) ||     // up/down
      ((fix == 5) && (gz > (wz0 + zdn))) ||
      ((fix == 4) && (gx < (wx1 - xbd))) ||      // left/roight
      ((fix == 3) && (gx > (wx0 + xbd))) ||
      ((fix == 2) && (gy < (wy1 - ybd))) ||      // fwd/back
      ((fix == 1) && (gy > (wy0 + ybd))))
    fix = 0;

  // stop advancing if movement no longer possible
  if ((fix == 2) && (bad < 2))
//    if ((gap = surf_gap()) <= 0.0)
    if ((gap = (rwi->nav).Forward()) <= 0.0)
      fix = 0;

  // continue to fix old error or switch to new one
  if (bad > fix)
    fix = bad;
  if (fix == (old & 0x07))
    old &= 0xF7;                       // clear change flag
  else
  {
    old &= 0xF0;                       // clear all workspace data
    old |= (0x08 | fix); 
    jprintf(2, dbg, "      workspace: fix %d - %s\n", fix, prob[fix]);
    if (prev != 0)
      return -1;                       // re-assess after every fix
  }
  return fix;                
}


//= Move lift stage or base to fix most important workspace violation.
// fixes one body error at a time: Z then X then finally Y
// assumes "gap" variable already has allowable travel distance if fix = 2
// returns 1 or 0 if special motion undertaken, -1 if arm should be moveable
// returns size of current error being reduced (negative for error)

double jhcManipulate::adj_workspace (int fix, double gx, double gy, double gz)
{
  double jfsp = 0.5, jtsp = 0.3, jmsp = 0.3; 
  double err, ztop = ((pos->Y() < (arm->rety + fwd)) ? arm->retz : wz1);                        
  double ang, nd = -sqrt(gx * gx + gy * gy), azm = R2D * asin(gx / nd);                

  // sanity check
  if ((fix <= 0) || (fix > 6))
    return -1.0;

  // z and y errors fixed by moving some number of inches
  if (fix == 6)                                  // too low -> up 
  {
    err = gz - (ztop - zup);
    lift->LiftShift(err + ztra, jfsp, bid);       
  }
  else if (fix == 5)                             // too high -> down  
  {
    err = gz - (wz0 + zdn);
    lift->LiftShift(err - ztra, jfsp, bid);       
  }
  else if (fix == 4)                             // too far right -> neg turn (CW)
  {
    err = gx - (wx1 - xbd);
    ang = azm - R2D * asin((wx1 - xbd) / nd);    
    base->TurnTarget(ang - xtra, jtsp, bid);
  }
  else if (fix == 3)                             // too far left -> pos turn (CCW)
  {
    err = gx - (wx0 + xbd);
    ang = azm - R2D * asin((wx0 + xbd) / nd);    
    base->TurnTarget(ang + xtra, jtsp, bid);
  }
  else if (fix == 2)                             // too far -> fwd 
  {
    err = gy - (wy1 - ybd);
    base->MoveTarget(__min(err + ytra, gap), jmsp, bid);    
  }
  else                                           // too close -> rev
  {
    err = gy - (wy0 + ybd);
    base->MoveTarget(err - ytra, jmsp, bid);      
  }
  return fabs(err);                              // always in inches
}


///////////////////////////////////////////////////////////////////////////
//                           Object Acquisition                          //
///////////////////////////////////////////////////////////////////////////

//= Tell if object currently detected in range image.
// returns 0 if not stable, 1 if item detected, -1 if not seen

int jhcManipulate::src_visible () const
{
  if (body->RngStatic() < park) 
    return 0;
  if (sobj->Component(citem[inst]) < 0)  
    return -1;
  return 1;
}


//= Find absolute grip position, orientation, and approach point for selected object.
// also determines expected gripper width (negative if roundish)
// returns 1 if okay, -1 if object height or width bad

int jhcManipulate::update_src (double& sep, jhcMatrix& hand, jhcMatrix& orient, jhcMatrix& via) 
{
  jhcMatrix obj(4);
  double pan, tilt, tad, mtns, ang, mid = 0.5 * (stip + otip);
  int rc, t = citem[inst];

  // get hand location and angle for grabbing but barf if object top is too big
  if ((rc = pick_grasp(sep, pan, hand, t)) < 0)
    return err_size(rc);
  if (rc == 1)
    sep = -sep;                                  // mark as roundish

  // pick overhead or sideways grasp angle
  tad = ((orient.T() < mid) ? hys : -hys);       // positive if already aimed down
  if ((sobj->SizeZ(t) < (oht + tad)) ||          // very short -> overhead
      (sobj->Major(t) > (olen - tad)))           // very long  -> overhead
    tilt = otip; 
  else
    tilt = stip;
  orient.SetVec3(pan, tilt, 0.0);         

  // determine approach via point over object
  mtns = obj_peaks(hand.X(), hand.Y(), arm->wmax, 0);
  via.SetVec3(hand.X(), hand.Y(), __max(sobj->MaxZ(t), mtns) + over);

  // possibly report grasp details
  ang = sobj->World(obj, t);
  jprintf(3, dbg, "      grasp %s @ %3.1f <- object %s @ %3.1f\n", hand.ListVec3(txt), pan, obj.ListVec3(txt2), ang);
  return 1;
}


//= Find grasp position, orientation, and gripper width for object with given track index.
// prefers estimate based on top slice but will default to using full object footprint
// returns 2 if elongated, 1 if roundish, -1 = too big, -2 = too flat

int jhcManipulate::pick_grasp (double& open, double& ang, jhcMatrix& grab, int t) const
{
  double flat = 0.5;
  double wx, wy, wz, wid, len, ht = sobj->SizeZ(t);

  // find location, orientation (on table), and size of graspable top
  if (ht < flat)
    return -2;
  if ((ang = sobj->FullTop(wx, wy, wid, len, t, knob)) < 0.0)  
  {
    // if no good top knob, grasp middle of body
    ang = sobj->World(wx, wy, t);      
    wid = sobj->Minor(t);
    len = sobj->Major(t);
  }
  if (wid > choke)                                                 
    return -1;                                

  // if top elongated then align with it, else orient gripper for convenience
  wz = sobj->MinZ(t) + __max(graze, ht - down);
  grab.SetVec3(wx, wy, wz);
  open = __min(wid + loose, arm->wmax);
  ang = easy_grip(ang, len / wid, corner_ang(wx, wy));     // gripper wrt plane

  // make sure gripper enagages small objects (Eli???)
  if ((len / wid) <= ecc0)
    return 1;
//  rads = D2R * ang;
//  grab.IncVec3(gulp * cos(rads), gulp * sin(rads), 0.0);   // not really useful?
  return 2;
}


//= Get natural pan angle for gripper at some location.
// generally configures arm to maximize movement possibilities
// points gripper directly away from lower right corner of workspace
// both coordinates and returned angle are in robot-relative system (not map)

double jhcManipulate::corner_ang (double tx, double ty) const
{
  return(R2D * atan2(ty - (wy0 - wcy), tx - wx1));  
}


//= Possibly flip orientation 180 degrees to better align with preferred grip.
// defaults to grip angle if elongation is low

double jhcManipulate::easy_grip (double pan, double ecc, double grip) const
{
  double dev = pan - grip, ang = pan;

  // keep close to corner angle
  if (ecc < ecc0)                      // roundish
    return grip;
  if (dev <= -90.0)               
    ang += 180.0;
  else if (dev > 90.0)                 // unlikely
    ang -= 180.0;

  // normalize value
  if (ang > 180.0)
    ang -= 360.0;
  else if (ang <= -180.0)
    ang += 360.0;
  return ang;
}


//= Clear all member variables related to object being gripped.
// always returns -1 for convenience

int jhcManipulate::clear_grip (int dn)
{
  if (dn > 0)
    sobj->OnTable(htrk);
  held = NULL;
  htrk = -1;
  nose = 0.0;
  left = 0.0;
  hang = graze;
  skew = 0.0;
  thin = 0;
  drop = 0;
  return -1;
}


//= Compute relative geometry of hand versus object.
// sets "held", "htrk", "nose", "left", "hang", and "skew" member variables
// assumes cdir[i] holds intended grip angle, camt[i] tells whether roundish

void jhcManipulate::record_grip (int t)
{
  double ang, wx, wy, pan0, dx, dy, rads, c, s, z3d;

  // remember visual track (wait on semantic node until lifted)
  htrk = t;
  slope = dir->T();                    // hand tilt at grasp time
  thin = ((arm->Width() < wmin) ? 1 : 0); 

  // elongated tops are physically forced into alignment with fingers
  ang = sobj->World(wx, wy, t);
  pan0 = cdir[inst].P();               // forced alignment
  if (camt[inst] < 0.0)                // roundish
    pan0 = dir->P();
  skew = ang - pan0;                   // object angle wrt gripper
  if (skew > 90.0)
    skew -= 180.0;
  else if (skew <= -90.0)
    skew += 180.0;

  // consider object centroid wrt actual grip point
  dx = wx - pos->X();
  dy = wy - pos->Y();
  rads = D2R * pan0;
  c = cos(rads);
  s = sin(rads);
  nose =  dx * c + dy * s;             // centroid along grip direcion
  left = -dx * s + dy * c;             // centroid laterally from grip

  // determine how much vertical clearance the hand needs now
  z3d = pos->Z() + lift->Height();
  hang = z3d - sobj->MinZ(t);         // amount below grip point
  jprintf(3, dbg, "    nose = %3.1f, left = %3.1f, hang = %3.1f, skew = %3.1f, thin %d\n", nose, left, hang, skew, thin);
}


///////////////////////////////////////////////////////////////////////////
//                         Trajectory Utilities                          //
///////////////////////////////////////////////////////////////////////////

//= Find maximum height of any object crossed by trajectory path.
// can optionally exclude held object if carry > 0 (cf. free_space)
// returns absolute height in inches (= obstacles + supporting surface)

double jhcManipulate::obj_peaks (double wx, double wy, double fsep, int carry) 
{
  double wid, len;
  int i, h20, n = sobj->ObjLimit(); 

// ignore restriction (MasterPi always goes hi to look ...)
return 0.0;

  // clear image where footprint of each object is marked by its maximum height
  mtns.FillMax(0);

  // mark all currently occluded objects as oriented rectangles
  for (i = 0; i < n; i++)
    if (sobj->ObjOK(i) && (sobj->Component(i) < 0))
      if ((carry <= 0) || (i != htrk))
      {
        wid = sobj->I2P(swell * sobj->Minor(i));
        len = sobj->I2P(swell * sobj->Major(i));
        h20 = ROUND(20.0 * sobj->OverZ(i));
        BlockRot(mtns, sobj->MapX(i), sobj->MapY(i), len, wid, sobj->Angle(i), h20);
      }

  // mark all currently detected objects using actual pixels
  for (i = 0; i < n; i++)
    if (sobj->ObjOK(i) && (sobj->Component(i) >= 0))
      if ((carry <= 0) || (i != htrk))
        sobj->DetPels(mtns, i, ROUND(20.0 * sobj->OverZ(i)));    

  // find tallest object to avoid in trajectory path (call traj_path)
  traj_path(wx, wy, fsep, carry);
  OverGate(mtns, mtns, path);
  return(0.05 * MaxVal(mtns) + sobj->ztab);
}


//= Create a binary mask showing linear region of concern for hand or object.
// sets ROI of "path" image tight around sampling stripe

void jhcManipulate::traj_path (double wx, double wy, double fsep, int carry) 
{  
  double obj, hw, hx, hy, tx, ty, dx, dy, len, degs, lf, rt, bot, top, hand = fabs(fsep) + 0.5;

  // determine appropriate streak width (pels) 
  if (carry > 0)
  {
    obj = swell * sobj->Major(htrk) + __max(fabs(nose), fabs(left)); 
    hand = __max(obj, hand);
  }
  hw = sobj->I2P(0.5 * hand + iwid);

  // find location (pels) of hand and destination then draw as circles
  sobj->ViewPels(hx, hy, pos->X(), pos->Y());
  sobj->ViewPels(tx, ty, wx, wy);
  path.FillMax(0);
  CircleFill(path, hx, hy, hw);
  CircleFill(path, tx, ty, hw);

  // connect endpoint circles with fat bar 
  dx = tx - hx;
  dy = ty - hy;
  len = sqrt(dx * dx + dy * dy);
  degs = sobj->ViewAngle(R2D * atan2(dy, dx));
  BlockRot(path, 0.5 * (hx + tx), 0.5 * (hy + ty), len, 2.0 * hw, degs);

  // set region of interest (for speed)
  lf  = __min(hx, tx) - hw;
  rt  = __max(hx, tx) + hw;
  bot = __min(hy, ty) - hw;
  top = __max(hy, ty) + hw;
  path.SetRoiLims(ROUND(lf), ROUND(bot), ROUND(rt), ROUND(top));
}


///////////////////////////////////////////////////////////////////////////
//                      Destination Determination                        //
///////////////////////////////////////////////////////////////////////////

//= Return the absolute anchor position based on current position of references.
// Note: this is not the same as the hand action point, merely same neighborhood

void jhcManipulate::anchor_loc (jhcMatrix& loc) const
{
  jhcMatrix obj2(4);
  double wx, wy;

  // "down" has no anchor object(s) - use current hand position
  if (cref[inst] < 0)
  {    
    wx = __max(wx0 + xbd, __min(pos->X(), wx1 - xbd));
    wy = __max(wy0 + ybd, __min(pos->Y(), wy1 - ybd));
    loc.SetVec3(wx, wy, sobj->ztab);
    return;
  }

  // destination anchor generally based on some tracked object(s)
  sobj->World(loc, cref[inst]);
  if (cref2[inst] < 0)
    return;
  sobj->World(obj2, cref2[inst]);
  loc.MixVec3(obj2, 0.5);               
}


//= Tell if reference object(s) currently detected in range image.
// returns 0 if not stable, 1 if item detected, -1 if not seen

int jhcManipulate::dest_visible () const
{
  int rn = cmode[inst];

  if (body->RngStatic() < park)                  // needed for obj_peaks()
    return 0;
  if ((rn == DOWN) || (rn == TWIXT))             // no ref or big separation
    return 1;
  if (sobj->Component(cref[inst]) < 0)           
    return -1;
  return 1;
}


//= Find absolute release position, orientation, and approach point for deposit.
// returns 1 if successful, -1 if no such location possible

int jhcManipulate::update_dest (jhcMatrix& hand, jhcMatrix& orient, jhcMatrix& via, int adj) 
{
  double pan, mtn, ht = sobj->ztab;

  // get hand action point 
  if (compute_dest(hand, pan, cflag[inst], adj) < 0)
    return -1;
  orient.SetVec3(pan, slope, 0.0);  

  // get intermediate location directly above action point
  if ((cmode[inst] == ON) && (cref[inst] >= 0))
    ht = sobj->MaxZ(cref[inst]);
  mtn = obj_peaks(hand.X(), hand.Y(), arm->Width(), 1) + hang;
  via.SetVec3(hand.X(), hand.Y(), __max(ht, mtn) + over);
  return 1;
}


//= Find optimal absolute hand deposit position relative and gripper angle.
// assumes cmode[i] is spatial relation, cref[i] is reference object track
// assumes hand-to-object offsets "nose", "left", "hang" and "skew" have been set
// sets bit 4 of eflag if underlying location is not oriented (for adjust_dest)
// returns 1 if successful, -1 if no such location possible

int jhcManipulate::compute_dest (jhcMatrix& hand, double& pan, int& eflag, int adj) 
{
  jhcMatrix loc(4);
  double ang;

  // find robot centric position for bottom of held object
  if (dest_bottom(loc, ang) < 0)
  {
    if (adj <= 0)
      return err_spot();     // initial selection   
    return -1;
  }
  eflag &= 0xEF;
  if (ang < 0.0)
    eflag |= 0x10;           // mark orientation as irrelevant

  // adjust hand position for current grip on object (if any)
  hand.Copy(loc);
  pan = ang;
  if (adj > 0)
    adjust_dest(hand, pan, eflag & 0x10);

  // possibly report hand pose then save hand offset from anchor
  jprintf(3, dbg, "    hand %s @ %3.1f <- deposit %s @ %3.1f\n", hand.ListVec3(txt), pan, loc.ListVec3(txt2), ang); 
  return 1;
}


//= Find desired robot relative coordinates for bottom of held object.
// also tells final gripper orientation needed to fit in spot
// prefers top slice for "on" but defaults to full object footprint
// deposit location estimate is tighter if all objects are curently detected
// sets variables "xdest" and "ydest" as selected map point (for debugging)
// returns 1 if successful, -1 if no space (values unchanged)

int jhcManipulate::dest_bottom (jhcMatrix& loc, double& pan)
{
  jhcMatrix dir(4);
  int side[4] = {180, 0, -90, 90};
  double ang, wid, len, ix, iy,  wx, wy;
  int ok, rn = cmode[inst], a = cref[inst], a2 = cref2[inst], t = htrk;

  // simple handler for "on" some object
  if (rn == ON)
  {
    if ((ang = sobj->FullTop(wx, wy, wid, len, a, mesa)) < 0.0)     
    {
      // if no good top plateau, deposit at middle of object
      ang = sobj->World(wx, wy, a);      
      wid = sobj->Minor(t);
      len = sobj->Major(t);
    }
    loc.SetVec3(wx, wy, sobj->MaxZ(a));
    pan = easy_grip(ang, len / wid, corner_ang(wx, wy));
    space.FillArr(0);                  // debug img
    return 1;
  }

  // attempt to find satisfactory map deposit location and orientation 
  free_space(((t == a) || (t == a2)) ? -1 : t);
  dest_ref(ix, iy, t, rn, a, a2);
  ang = dest_ang(ix, iy, t, rn, a, a2);
  ok = pick_spot(xdest, ydest, ix, iy, ang, t, rn, a, a2);

  // debug img: mark search cone (if any) and reference location 
  if ((rn >= 1) && (rn <= 4))
  {
    ang = sobj->ViewAngle(side[rn - 1]);
    Ray(space, ix, iy, ang + sdev, 0.0, 1, 90);
    Ray(space, ix, iy, ang - sdev, 0.0, 1, 90);
  }
  Cross(space, ix, iy, 17, 17, 1, 200);

  // debug img: mark chosen deposit footprint for held object
  if (ok <= 0)
    return -1;
  wid = 1.2 * sobj->I2P(sobj->Minor(t));
  len = 1.2 * sobj->I2P(sobj->Major(t));
  RectCent(space, xdest, ydest, len, wid, ang, 3, -5);  

  // convert map pose to full world coordinates
  sobj->PelsXY(wx, wy, xdest, ydest);
  loc.SetVec3(wx, wy, sobj->ztab);
  pan = sobj->FullAngle(ang);
  return 1;
}


//= Fix up relative deposit location and hand angle once gripping parameters are known.
// takes FULL deposit position and orientation, any > 0 means pick convenient angle

void jhcManipulate::adjust_dest (jhcMatrix& full, double& pan, int any)
{
  double rads, c, s, dx, dy, pan0 = pan;

  // determine appropriate hand pan to achieve desired object orientation (if any)
  if (any > 0)
    pan = corner_ang(full.X(), full.Y());
  else
  {
    pan += skew;
    if (pan > 180.0)
      pan -= 360.0;
    else if (pan <= -180.0)
      pan += 360.0;
  }

  // adjust hand final position to account for object centroid offsets
  rads = D2R * pan;
  c = cos(rads);
  s = sin(rads);
  dx = -nose * c + left * s;
  dy = -nose * s - left * c;

  // possibly tell result
  jprintf(3, dbg, "    hand [%3.1f %3.1f %3.1f] @ %3.1f <- rough %s @ %3.1f\n", 
          full.X() + dx, full.Y() + dy, full.Z() + hang, pan, full.ListVec3(txt), pan0); 
  full.IncVec3(dx, dy, hang);
}


///////////////////////////////////////////////////////////////////////////
//                          Open Space Finding                           //
///////////////////////////////////////////////////////////////////////////

//= Finds areas which are part of supporting surface but free of obstacles.
// can optionally assume object "exc" does not matter (e.g. htrk)
// leaves result in member image "space", not blocked if over 150

void jhcManipulate::free_space (int exc)
{
  double wid, len, margin = 1.0;
  int i, n = sobj->ObjLimit(), ej = ROUND(sobj->I2P(3.0 * margin)) | 0x01;

  // find basic support surface and remove rim around edge
  Threshold(space, sobj->top, 50);
  BoxThresh(space, space, ej, 200);

  // exclude actual pixels for all tracked objects (if currently detected)
  for (i = 0; i < n; i++)
    if ((i != exc) && sobj->ObjOK(i))
      sobj->DetPels(space, i, 128);    

  // black outline for currently occluded objects (except the one in gripper) 
  for (i = 0; i < n; i++)
    if ((i != exc) && sobj->ObjOK(i) && (sobj->Component(i) < 0))
    {
      wid = sobj->I2P(swell * sobj->Minor(i));
      len = sobj->I2P(swell * sobj->Major(i));
      RectCent(space, sobj->MapX(i), sobj->MapY(i), len, wid, sobj->Angle(i), 3, 50);
    }
}


//= Find map reference location (pels) for specified relation relative to anchor(s).
// takes payload object track, relation number, and one or two anchor object tracks
// rn: 0 = "down", "between", "left of", "right of", "in front of", "behind", "near", "next to"

void jhcManipulate::dest_ref (double& ix, double& iy, int t, int rn, int a, int a2) 
{
  jhcMatrix loc(4);
  double wx, wy, dx, dy, f;

  // special case of setting down (nearest workspace point shifted to table)
  if (rn == DOWN)
  {
    wx = __max(wx0 + xbd, __min(pos->X(), wx1 - xbd));
    wy = __max(wy0 + ybd, __min(pos->Y(), wy1 - ybd));
//    sobj->NearTable(loc, wx, wy);
    loc.SetVec3(wx, wy, sobj->ztab);
    sobj->ViewPels(ix, iy, loc.X(), loc.Y());    // ix, iy for debugging
    return;
  }

  // assume reference point at anchor object (ix, iy for debugging)
  ix = sobj->MapX(a);
  iy = sobj->MapY(a);
  if (rn == TWIXT)
  {
    // ideal position at midpoint
    ix = 0.5 * (ix + sobj->MapX(a2));
    iy = 0.5 * (iy + sobj->MapY(a2));
  }
  else if ((rn == NEARBY) || (rn == NEXTTO))
  {
    // move ideal position slightly toward current location
    dx = sobj->MapX(t) - ix;
    dy = sobj->MapY(t) - iy;
    if ((dx != 0.0) || (dy != 0.0))
    {
      f = sobj->I2P(bias) / sqrt(dx * dx + dy * dy);
      ix += f * dx;
      iy += f * dy;
    }
  }
}


//= Choose best orientation for object deposit given image reference point (pels).
// takes payload object track, relation number, and one or two anchor object tracks
// rn: 0 = "down", "between", "left of", "right of", "in front of", "behind", "near", "next to"
// returns suggestion for final orientation relative to map image (negative for "any")

double jhcManipulate::dest_ang (double ix, double iy, int t, int rn, int a, int a2) const
{
  double wx, wy, mgrip, mdir, dx, dy;

  // special case of setting down (prefer no changes)
  if (rn == DOWN)
    return sobj->ViewAngle(dir->P());

  // default deposit angle is the one easiest for arm
  sobj->PelsXY(wx, wy, ix, iy);
  mgrip = sobj->ViewAngle(corner_ang(wx, wy));
  mdir = mgrip;                                            // roundish and "near"

  // adjust for various spatial relations                         
  if (sobj->Elongation(t) > ecc0)
  {
    if (rn == TWIXT)
    {
      dx = sobj->MapX(a2) - sobj->MapX(a);
      dy = sobj->MapY(a2) - sobj->MapX(a);
      if (dx != 0.0)
        mdir = R2D * atan(dy / dx) + 90.0;                 // thread gap
      else
        mdir = 90.0;
    }
    else if ((rn == LEFT) || (rn == RIGHT))
      mdir = 90.0;
    else if ((rn == FRONT) || (rn == BEHIND))
      mdir = 0.0;
    else if ((rn == NEXTTO) && (sobj->Elongation(a) > ecc0))
      mdir = sobj->Angle(a);                               // parallel
  }
  return easy_grip(mdir, 10.0, mgrip);                     // just adjust mdir
}


//= Find map-based (pel) center position for object deposition.
// leaves center map in member image "shrink" (also creates image "align")
// sets variables "xpick" and "ypick" to choosen location in "shrink" (for debugging)
// returns 1 if successful, 0 if nothing suitable

int jhcManipulate::pick_spot (double& cx, double& cy, double ix, double iy, double pan, int t, int rn, int a, int a2)
{
  double w = sobj->I2P(swell * sobj->Minor(t) + 2.0 * (fuzz + iwid));
  double h = sobj->I2P(swell * sobj->Major(t) + 2.0 * fuzz);
  double rot = 90.0 - pan, rads = -D2R * rot, c = cos(rads), s = sin(rads); 
  double dist, dx, dy, sep;
  int mx = align.XDim() >> 1, my = align.YDim() >> 1;

  // rotate free space map to final object orientation and shrink by object size.
  Rigid(align, space, rot, mx, my, ix, iy);
  FitsBox(shrink, align, ROUND(w), ROUND(h), 150);

  // find closest feasible point respecting geometric constraints
  if (rn == LEFT) 
    dist = NearSect(xpick, ypick, shrink, sobj->ViewAngle(rot + 180.0), sdev);
  else if (rn == RIGHT) 
    dist = NearSect(xpick, ypick, shrink, sobj->ViewAngle(rot), sdev);
  else if (rn == FRONT)
    dist = NearSect(xpick, ypick, shrink, sobj->ViewAngle(rot - 90.0), sdev);
  else if (rn == BEHIND)
    dist = NearSect(xpick, ypick, shrink, sobj->ViewAngle(rot + 90.0), sdev);
  else
    dist = NearCent(xpick, ypick, shrink);       // down, between, next, near
  if (dist < 0.0)
    return 0;

  // see if found position is close enough (all in inches)
  dist = sobj->P2I(dist);
  if (((rn == NEARBY) && (dist > (hood  * sobj->Major(a)))) ||
      ((rn == NEXTTO) && (dist > (buddy * sobj->Major(a)))))
    return 0;  
  else if (rn == TWIXT)
  {
    dx = sobj->PosX(a) - sobj->PosX(a2);
    dy = sobj->PosY(a) - sobj->PosY(a2);
    sep = sqrt(dx * dx + dy * dy);
    if (dist > (tween * sep))
      return 0;
  }

  // transform nearest center point back into original map coords
  dx = xpick - mx;
  dy = ypick - my;
  cx = (dx * c - dy * s) + ix;
  cy = (dx * s + dy * c) + iy;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Destination Parsing                            //
///////////////////////////////////////////////////////////////////////////

//= Generate unique spatial relation number based on textual name.

int jhcManipulate::txt2rnum (const char *txt) const
{
  int i;

  // match canoncial name
  if (txt == NULL)
    return -1;
  for (i = 0; i < RMAX; i++)
    if (strcmp(txt, rel[i]) == 0)
      return i;

  // handle some variants
  if (strcmp(txt, "to the left of") == 0)
    return LEFT;
  if (strcmp(txt, "to the right of") == 0)
    return RIGHT;
  if (strcmp(txt, "in back of") == 0)
    return BEHIND;
  if ((strcmp(txt, "near to") == 0) || (strcmp(txt, "close to") == 0))
    return NEARBY;
  if ((strcmp(txt, "onto") == 0) || (strcmp(txt, "on to") == 0) || (strcmp(txt, "on top of") == 0))
    return ON;
  return -1;
}


//= Retrieve text name for given spatial relation number.

const char *jhcManipulate::rnum2txt (int rn) const
{
  if ((rn < 0) || (rn >= RMAX))
    return NULL;
  return rel[rn];
}


//= Bind track numbers for references needed by spatial relation.
// returns 1 if okay, -1 if bad ref, -2 bad network

int jhcManipulate::ref_tracks (int& a, int& a2, int rn, const jhcAliaDesc *place) 
{
  jhcAliaDesc *ref;

  // set up defaults (both invalid)
  a  = -1;
  a2 = -1;

  // single reference
  if (rn != DOWN)
  {
    if ((ref = place->Val("ref")) == NULL)
      return -2;
    if ((a = sobj->ObjTrack(rpt->VisID(ref, 0))) < 0)
      return err_gone(ref);
  }

  // dual references ("between")
  if (rn == TWIXT)
  {
    if ((ref = place->Val("ref2")) == NULL)
      return -2;
    if ((a2 = sobj->ObjTrack(rpt->VisID(ref, 0))) < 0)
      return err_gone(ref);
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Semantic Messages                            //
///////////////////////////////////////////////////////////////////////////

//= Complain about the arm not working.
// <pre>
//   NOTE[ act-1 -lex-  work
//               -neg-  1
//               -agt-> obj-1
//         ako-1 -lex-  arm 
//               -ako-> obj-1
//               -wrt-> self-1 ]
// </pre>
// always returns -1 for convenience

int jhcManipulate::err_arm ()
{
  jhcAliaDesc *part, *own, *arm, *fail;

  rpt->StartNote();
  part = rpt->NewObj("sys");
  own = rpt->NewProp(part, "ako", "arm");
  rpt->AddArg(own, "wrt", rpt->Self());
  arm = rpt->Resolve(part);                      // find or make part
  fail = rpt->NewAct("work", 1);
  rpt->AddArg(fail, "agt", arm);                 // mark as not working
  rpt->FinishNote(fail);
  return -1;
}


//= Generate error event for bad height or width.
// pick_grasp rc: -1 = too big, -2 = too flat
// example "it is too big":
// <pre>
//   NOTE[  hq-1 -lex-  big
//               -hq--> obj-1
//         deg-1 -lex-  too
//               -deg-> hq-1 ]
// </pre>
// returns -1 always for convenience

int jhcManipulate::err_size (int rc)
{
  jhcAliaDesc *fail = NULL;
  
  // sanity check
  if (cobj[inst] == NULL)
    return -1;  
  msg = rc;                            // record problem for GUI

  // event generation
  rpt->StartNote();
  if (rc == -2)
    fail = rpt->NewDeg(cobj[inst], "hq", "flat", "too");
  else if (rc == -1)
    fail = rpt->NewDeg(cobj[inst], "hq", "big", "too");
  rpt->FinishNote(fail);
  return -1;    
}


//= Generate error event for no sutiable deposit spot found.
// <pre>
//   NOTE[ act-1 -lex-  fit
//               -ach-  0
//               -agt-> obj-1
//         loc-1 -lex-  behind
//               -loc-> act-1
//               -ref-> obj-2 ]
// </pre> 
// always returns -1 for convenience

int jhcManipulate::err_spot ()
{
  jhcAliaDesc *fail, *loc, *place = cspot[inst];
  int rn = cmode[inst];

  // sanity check
  if (cobj[inst] == NULL)
    return -1;
  msg = 3;                   // record for GUI

  // generate error event that object does not fit somewhere
  rpt->StartNote();
  fail = rpt->NewAct("fit", 1, 1);
  rpt->AddArg(fail, "agt", cobj[inst]);
  if (rn != DOWN)
  {
    loc = rpt->NewProp(fail, "loc", rnum2txt(rn));
    rpt->AddArg(loc, "ref", place->Val("ref"));
    if (rn == TWIXT)
      rpt->AddArg(loc, "ref", place->Val("ref2"));
  }
  rpt->FinishNote(fail);
  return -1;
}


//= Generate error event for object not being seen.
// <pre>
//   NOTE[ act-1 -lex-  see
//               -neg-  1
//               -agt-> self-1
//               -obj-> obj-1 ]
// </pre>
// returns -1 always for convenience

int jhcManipulate::err_gone (jhcAliaDesc *obj)
{
  jhcAliaDesc *fail;

  // sanity check
  if (obj == NULL)
    return -1;

  // event generation
  rpt->StartNote();
  fail = rpt->NewAct("see", 1);
  rpt->AddArg(fail, "agt", rpt->Self());
  rpt->AddArg(fail, "obj", obj);
  rpt->FinishNote(fail);
  return -1;
}


//= Generate error event for not getting to some object.
// does not generate message unless object is non-NULL
// <pre>
//   NOTE[ act-1 -lex-  reach
//               -ach-  0
//               -agt-> self-1
//               -obj-> obj-1 ]
// </pre>
// returns -1 always for convenience

int jhcManipulate::err_reach (jhcAliaDesc *obj)
{
  jhcAliaDesc *fail, *obj2;

  // possibly tell arm position
  final_pose(1);                      
  if (obj == NULL)
    return -1;
 
  // generate failure event
  rpt->StartNote();
  fail = rpt->NewAct("reach", 1, 1);
  rpt->AddArg(fail, "agt", rpt->Self());
  if (obj->ObjNode())
    rpt->AddArg(fail, "obj", obj);               // target
  else if (cmode[inst] == ON)
    rpt->AddArg(fail, "obj", obj->Val("ref"));   // "on" something
  else
  {
    obj2 = rpt->NewObj("place");
    rpt->NewProp(obj2, "ako", "destination");    // other locations
    rpt->AddArg(fail, "obj", obj2);               
  }
  rpt->FinishNote(fail);
  return -1;
}


//= Generate error message that the robot is not holding the given object.
// <pre>
//   NOTE[ act-1 -lex-  hold
//               -neg-  1
//               -obj-> obj-1
//               -agt-> self-1 ]
// </pre>
// always returns -1 for convenience
// Note: "not holding" better than "dropped" for ANTE operator

int jhcManipulate::err_lack (jhcAliaDesc *obj)
{
  jhcAliaDesc *fail;

  // event generation (adds as failure reason even if whole motion succeeds)
  rpt->StartNote();
  fail = rpt->NewAct("hold", 1);
  rpt->AddArg(fail, "agt", rpt->Self());
  rpt->AddArg(fail, "obj", obj);
  rpt->FinishNote(fail);
  return -1;
}


//= Generate error message that the robot dropped the given object.
// <pre>
//   NOTE[ act-1 -lex-  drop
//               -ach-  1
//               -obj-> obj-1
//               -agt-> self-1 
//         act-2 -lex-  hold
//               -neg-  1
//               -obj-> obj-1
//               -agt-> self-1 ]
// </pre>
// always returns -1 for convenience
// Note: incorporating "not holding" into same NOTE is necessary

int jhcManipulate::err_drop (jhcAliaDesc *obj)
{
  jhcAliaDesc *fail, *act;

  // sanity check
  if (obj == NULL)
    return -1;

  // event generation (adds as failure reason even if whole motion succeeds)
  rpt->StartNote();
  fail = rpt->NewAct("drop", 0, 1);              // reason
  rpt->AddArg(fail, "agt", rpt->Self());
  rpt->AddArg(fail, "obj", obj);
  act = rpt->NewAct("hold", 1);                  // extra fact
  rpt->AddArg(act, "agt", rpt->Self());
  rpt->AddArg(act, "obj", obj);
  rpt->FinishNote(fail);
  return -1;
}


//= Say that the robot is or is not (intentionally) holding the remembered object.
// generally used when change of status is expected
// <pre>
//   NOTE[ act-1 -lex-  hold
//               -obj-> obj-1
//               -agt-> self-1 ]
// </pre>

void jhcManipulate::msg_hold (jhcAliaDesc *obj, int neg)
{
  jhcAliaDesc *act;

  // sanity check
  if (obj == NULL)
    return;

  // event generation
  rpt->StartNote();
  act = rpt->NewAct("hold", neg);
  rpt->AddArg(act, "agt", rpt->Self());
  rpt->AddArg(act, "obj", obj);
  rpt->FinishNote(NULL);                         // not an error
}


///////////////////////////////////////////////////////////////////////////
//                          Debugging Utilities                          //
///////////////////////////////////////////////////////////////////////////

//= Show optimal workspace for manipulation on overhead map image.

void jhcManipulate::Workspace (jhcImg& dest, int r, int g, int b) const
{
  int gx[4], gy[4];

  if ((rwi == NULL) || !dest.Valid())
    return;
  sobj->ViewPels(gx[0], gy[0], wx0, wy0);
  sobj->ViewPels(gx[1], gy[1], wx1, wy0);
  sobj->ViewPels(gx[2], gy[2], wx1, wy1);
  sobj->ViewPels(gx[3], gy[3], wx0, wy1);
  DrawPoly(dest, gx, gy, 4, 1, r, g, b);
}


//= Force action to be applied to some particluar object track.

void jhcManipulate::ForceItem (int t)
{
  jhcAliaDesc *n = rpt->NewObj("obj");
  int i = 0;

  // bind object node to track
  rpt->VisAssoc(sobj->ObjID(t), n, 0);

  // object specification
  cobj[i]  = n;
  citem[i] = t;

  // destination specification
  cspot[i] = n;
  cmode[i] = -1;             // never recompute destination
  cref[i]  = -1;
  cref2[i] = -1;

  // motion parameters 
  csp[i]  = 1.0;
  cbid[i] = 10;

  // initial state
  half = 0;                  // for Move()
  cst[i] = 0;                // start with pick grasp                
  cst2[i] = 0;
  ccnt2[i] = 0;              // jockey attempts
}


//= Force a particular deposit position and default orientation.

void jhcManipulate::ForceDest (double wx, double wy, double wz) 
{
  dest.SetVec3(wx, wy, wz);    
  half = 0;                  // for Move()
}


//= Go through all steps of a "take" sequence after forcing object and destination.
// does man_wrap followed by man_trans, needs external state variable half = 0 at start
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::Move () 
{
  const jhcAliaDesc *dummy = rpt->User();
  int rc;

  // do background processing first (no ALIA)
  local_volunteer(); 
 
  // do either object acquisition or deposit
  if (half <= 0)
    rc = man_wrap(*dummy, 0);
  else
    rc = man_trans(*dummy, 0);
    
  // check for failure and success
  if (rc < 0)
    return -1;
  if (rc == 0)
    return 0;
  if (half > 0)
    return 1;

  // transition between controllers
  cpos[0].Copy(dest);
  cdir[0].Copy(*dir);
  cst[0] = 1;                // start with jockey                
  cst2[0] = 0;
  ccnt2[0] = 0;              // jockey attempts
  half = 1;
  return 0;
}
