// jhcManipulate.cpp : interface to ELI object manipulation kernel for ALIA system 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021-2023 Etaoin Systems
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

#include "Eli/jhcEliGrok.h"            // common robot - since only spec'd as class in header

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
}


//= Default constructor initializes certain values.

jhcManipulate::jhcManipulate ()
{
  // static configuration
  ver = 2.10;
  strcpy_s(tag, "Manipulate");

  // vector sizes
  end.SetSize(4);
  aim.SetSize(4);
  perr.SetSize(4);
  derr.SetSize(4);

  // overall interaction parameters
  Platform(NULL);
  rpt = NULL;

  // no object in hand currently
  clear_grip();

  // dynamic values and parameters
  Defaults();
  dbg = 1;
dbg = 3;
}


//= Attach physical enhanced body and make pointers to some pieces.

void jhcManipulate::Platform (jhcEliGrok *robot) 
{
  rwi = robot;
  if (rwi == NULL)
    return;
  sobj = &(rwi->sobj);
  lift = rwi->lift;
  arm  = rwi->arm;
  pos  = arm->Position();
  dir  = arm->Direction();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters controlling gripping of objects.

int jhcManipulate::grab_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("man_grab", 0);
  ps->NextSpecF( &knob,   0.8, "Slice off for grab (in)");  
  ps->NextSpecF( &mesa,   0.3, "Slice off for deposit (in)");  
  ps->NextSpecF( &choke,  3.0, "Max object size (in)");  
  ps->NextSpecF( &ecc0,   1.3, "Round eccentriciy");           // was 1.2
  ps->NextSpecF( &drop,   0.0, "Grab down from top (in)");     // was 1, 0.5 then 0.3
  ps->NextSpecF( &gulp,   0.6, "Center into gripper (in)");    // was 0
  
  ps->NextSpecF( &loose,  0.5, "Extra open each side (in)");
  ps->NextSpecF( &tip,   30.0, "Standard grip tilt (deg)");    // was 15 then 45
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
  ps->NextSpec4( &park,   5,   "Base static for update (cyc)");
  ps->NextSpecF( &ttol,   0.2, "Error for straight up (in)");
  ps->NextSpecF( &hold,  12.0, "Holding force (oz)");
  ps->NextSpecF( &wmin,   0.3, "Empty hand width (in)");
  ps->NextSpecF( &wtim,   2.0, "Open/close timeout (sec)");
  ps->NextSpecF( &edge,  20.0, "Tilt to surface edge (deg)");

  ps->NextSpecF( &over,   1.8, "Tip travel height (in)");      // was 1.3 then 1.6
  ps->NextSpecF( &graze,  0.9, "Min grip point height (in)");  // was 1.2 then 1.3 
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
  ps->NextSpecF( &atol,    7.0,  "Direction tol (deg)"); 
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

  ok &= grab_params(fname);
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

//= Set up for new run of system.
// NOTE: sobj->map is wrong size at this point!

void jhcManipulate::local_reset (jhcAliaNote *top)
{
  rpt = top;
  clear_grip();              // nothing in hand
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

int jhcManipulate::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(man_grab);
  JCMD_SET(man_lift);
  JCMD_SET(man_take);
  JCMD_SET(man_move);
  JCMD_SET(man_tuck);
  JCMD_SET(man_point);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcManipulate::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(man_grab);
  JCMD_CHK(man_lift);
  JCMD_CHK(man_take);
  JCMD_CHK(man_move);
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
// assumes "gulp" and "hang" variables were set by close_fingers()
// Note: object might have useful non-observable semantic features like "mine"

int jhcManipulate::update_held ()
{
  double ang, rads, c, s, wx, wy, wz, ht = lift->Height(), sqz0 = 5.0;

  // wait for next sensor cycle 
  if ((rwi == NULL) || rwi->Ghost() || !rwi->Accepting())
    return 0; 
  if (held == NULL)
    return 0;

  // check that object is still being held (allow short bobble)
  if ((arm->Width() < wmin) || (arm->Squeeze() < sqz0))
    if (drop++ > 0)
    {
      err_lack(held);                
      return clear_grip();
    }

  // update object pose based on robot arm configuration
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
//                           Motion Sequences                            //
///////////////////////////////////////////////////////////////////////////

//= Start trying to hold object with hand.
// instance number and bid already recorded by base class
// assumes not holding anything currently (will likely drop)
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_grab0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  // argument parsing
  if ((cobj[i] = desc->Val("arg")) == NULL)
    return -1;

  // intialize state
  cmode[i] = -1;             // no explicit destination
  citem[i] = -1;             // await access to vision
  ccnt[i]  = 0;              // non-detect count
  ccnt2[i] = 0;
  cflag[i] = 0;              // no workspace violations
  return 1;
}


//= Continue trying to hold object with hand.
// sets up continuing request to body if not finished
//   cst[i]: 0 = check if graspable 
//           1 = travel to via point
//           2 = travel to grasp point
//           3 = close fingers on object
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_grab (const jhcAliaDesc *desc, int i)
{
  int rc = 0;

  // lock to sensor cycle and check update standard cached info
  if (!rwi->Accepting())
    return 0;
  init_vals(i);

  // make sure target object still is known 
  if ((citem[i] = sobj->ObjTrack(rpt->VisID(cobj[i]))) < 0)
    return err_gone(cobj[i]);
  if (rwi->Ghost() || (arm->CommOK() <= 0))
    return err_arm();

  // check for serendipitous grasp or interference
  if (cst[i] <= 3) 
  {
    if (held == cobj[i])
      return 1;                        // lucky
    else if (chk_hand(NULL) <= 0)
      fail_clean();
  }

  // get desired position and orientation based on state
  if (cst[i] <= 0)
    rc = assess_obj();
  else if (cst[i] == 1)
    rc = goto_via();
  else if (cst[i] == 2)
    rc = goto_grasp();
  else if (cst[i] == 3)                // last grasp
    rc = close_fingers();
  else if (cst[i] == 4)                // success
    return 1;

  // cleanup from failure
  if (cst[i] == 20)             
    rc = stow_arm();           
  else if (cst[i] == 21)
    return tuck_elbow(-1);             // joint mode
  return command_bot(rc);
}


//= Start trying to lift held object above surface.
// instance number and bid already recorded by base class
// assumes already holding proper thing to be lifted (i.e. incremental)
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_lift0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  // argument parsing
  if ((cobj[i] = desc->Val("arg")) == NULL)
    return -1;

  // initialize state
  cmode[i] = -1;             // no explicit destination
  citem[i] = -1;             // await access to vision
  ccnt[i]  = 0;              // non-detect count
  ccnt2[i] = 0;
  cflag[i] = 0;              // no workspace violations
  return 1;
}


//= Continue trying to lift held object above surface.
// sets up continuing request to body if not finished
//   cst[i]: 0 = check if graspable 
//           1 = travel to via point
//           2 = travel to grasp point
//           3 = close fingers on object
//         * 4 = lift object off surface
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_lift (const jhcAliaDesc *desc, int i)
{
  int rc = 0;

  // lock to sensor cycle and update standard cached info
  if (!rwi->Accepting())
    return 0;
  init_vals(i);

  // make sure target object is still known 
  if ((citem[i] = sobj->ObjTrack(rpt->VisID(cobj[i]))) < 0)
    return err_gone(cobj[i]);
  if (rwi->Ghost() || (arm->CommOK() <= 0))
    return err_arm();

  // check for serendipitous grasp or interference
  if (cst[i] <= 3)                   
  {
    if (held == cobj[i])    
    {         
      cst[i]  = 4;                     // skip ahead
      cst2[i] = 0;
    }
    else if (chk_hand(NULL) <= 0)
      fail_clean();
  }
  else if (cst[i] <= 5)                // lifting
    if (chk_hand(cobj[i]) <= 0)
      fail_clean();

  // get desired position and orientation based on state
  if (cst[i] <= 0)
    rc = assess_obj();
  else if (cst[i] == 1)
    rc = goto_via();
  else if (cst[i] == 2)
    rc = goto_grasp();
  else if (cst[i] == 3)                // last grasp
    rc = close_fingers();
  else if (cst[i] == 4)                
    rc = lift_off();
  else if (cst[i] == 5)                // success
    return 1;

  // cleanup from failure
  if (cst[i] == 20)             
    rc = stow_arm();           
  else if (cst[i] == 21)
    return tuck_elbow(-1);             // joint mode
  return command_bot(rc);
}


//= Start trying to stow held object in travel position.
// instance number and bid already recorded by base class
// assumes already holding proper thing to be stowed (i.e. incremental)
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_take0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  // argument parsing
  if ((cobj[i] = desc->Val("arg")) == NULL)
    return -1;

  // initialize state
  cmode[i] = -1;             // no explicit destination
  citem[i] = -1;             // await access to vision
  ccnt[i]  = 0;              // non-detect count
  ccnt2[i] = 0;
  cflag[i] = 0;              // no workspace violations
  return 1;
}


//= Continue trying to stow held object in travel position.
// sets up continuing request to body if not finished
//   cst[i]: 0 = check if graspable 
//           1 = travel to via point
//           2 = travel to grasp point
//           3 = close fingers on object
//         * 4 = retract to stowed location
//           5 = avoid protruding joints
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_take (const jhcAliaDesc *desc, int i)
{
  int rc = 0;

  // lock to sensor cycle and update standard cached info
  if (!rwi->Accepting())
    return 0;
  init_vals(i);

  // make sure target object is still known 
  if ((citem[i] = sobj->ObjTrack(rpt->VisID(cobj[i]))) < 0)
    return err_gone(cobj[i]);
  if (rwi->Ghost() || (arm->CommOK() <= 0))
    return err_arm();

  // check for serendipitous grasp or interference
  if (cst[i] <= 3)                     
  {
    if (held == cobj[i])
    {
      cst[i]  = 4;                     // skip ahead
      cst2[i] = 0;
    }
    else if (chk_hand(NULL) <= 0)
      fail_clean();
  }
  else if (cst[i] <= 5)                // stowing
    if (chk_hand(cobj[i]) <= 0)
      fail_clean();

  // get desired position and orientation based on state
  if (cst[i] <= 0)
    rc = assess_obj();
  else if (cst[i] == 1)
    rc = goto_via();
  else if (cst[i] == 2)
    rc = goto_grasp();
  else if (cst[i] == 3)                // last grasp
    rc = close_fingers();
  else if (cst[i] == 4)                
    rc = stow_arm();
  else if (cst[i] == 5)                
    return tuck_elbow(1);              // joint mode

  // cleanup from failure
  if (cst[i] == 20)             
    rc = stow_arm();           
  else if (cst[i] == 21)
    return tuck_elbow(-1);             // joint mode
  return command_bot(rc);
}


//= Start trying to move held object to some location.
// only functions on semnet allowed, cannot interrogate vision system
// binds moving object to cobj[i] and destination place to cspot[i]
// sets cmode[i] to be relation number (fails if no "arg2")
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_move0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  // argument parsing (cannot put something on itself)
  if (((cobj[i] = desc->Val("arg")) == NULL) || 
      ((cspot[i] = desc->Val("arg2")) == NULL))
    return -1;
  if ((cmode[i] = txt2rnum(cspot[i]->Lex())) < 0)
    return -1;
  if ((cmode[i] == ON) && (cspot[i]->Val("ref") == cobj[i]))
    return -1;

  // initialize state
  citem[i] = -1;             // await access to vision
  cref[i]  = -1;
  cref2[i] = -1;
  ccnt[i]  = 0;              // non-detect count
  ccnt2[i] = 0;
  cflag[i] = 0;              // no workspace violations
  return 1;
}


//= Continue trying to lift held object above surface.
// assumes cmode[i] has relation and cspot[i] has destination description
// sets cref[i] and cref2[i] to be tracks for reference objects (if any)
//   cst[i]: 0 = check for reasonable deposit spot
//           1 = check if graspable 
//         + 2 = travel to via point
//           3 = travel to grasp point
//           4 = close fingers on object
//         * 5 = transfer to over destination
//           6 = lower to destination height
//           7 = release object
//           8 = retract to stowed location
//           9 = avoid protruding joints
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_move (const jhcAliaDesc *desc, int i)
{
  int rc = 0;

  // lock to sensor cycle and update standard cached info
  if (!rwi->Accepting())
    return 0;
  init_vals(i);

  // make sure target and reference object(s) are still known 
  if ((citem[i] = sobj->ObjTrack(rpt->VisID(cobj[i]))) < 0)
    return err_gone(cobj[i]);
  if (ref_tracks(cref[i], cref2[i], cmode[i], cspot[i]) < 0)   
    return -1;                         // generates err_gone also
  if (rwi->Ghost() || (arm->CommOK() <= 0))
    return err_arm();

  // check for serendipitous grasp or interference
  if (cst[i] >= 1)
  {
    if (cst[i] <= 4)                   // grasping
    {
      if (held == cobj[i])
      {            
        cst[i]  = 5;                   // skip ahead
        cst2[i] = 0;
      }
      else if (chk_hand(NULL) <= 0)
        fail_clean();
    }
    else if (cst[i] == 5)              // transferring
      if (chk_hand(cobj[i]) <= 0)
        fail_clean();
  }

  // get desired position and orientation based on state
  if (cst[i] <= 0)
    rc = assess_spot();                // sets cend[i] and caux[i]
  else if (cst[i] == 1)                // * first grasp                     
    rc = assess_obj();                 // sets cpos[i], cdir[i], and camt[i]
  else if (cst[i] == 2)
    rc = goto_via();
  else if (cst[i] == 3)
    rc = goto_grasp();
  else if (cst[i] == 4)                // last grasp
    rc = close_fingers();
  else if (cst[i] == 5)                
    rc = xfer_over();
  else if (cst[i] == 6)
    rc = place_on();
  else if (cst[i] == 7)
    rc = release_obj();                // held -> NULL
  else if (cst[i] == 8)
    rc = stow_arm();
  else if (cst[i] == 9)
    return tuck_elbow(1);              // joint mode

  // cleanup from failure
  if (cst[i] == 20)             
    rc = stow_arm();           
  else if (cst[i] == 21)
    return tuck_elbow(-1);             // joint mode
  return command_bot(rc);
}


//= Start trying to retract the arm to travel position.
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_tuck0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  // initialize state
  cst[i] = 20;               // start cleanup immediately
  cflag[i] = 0;              // no workspace violations
  return 1;
}


//= Continue trying to retract arm to travel position.
//   cst[i]: 0 = retract to stowed location
//           1 = avoid protruding joints
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_tuck (const jhcAliaDesc *desc, int i)
{
  int rc = 0;

  // lock to sensor cycle and update standard cached info
  if (!rwi->Accepting())
    return 0;
  if (rwi->Ghost() || (arm->CommOK() <= 0))
    return err_arm();
  init_vals(i);

  // get desired position and orientation based on state
  if (cst[i] == 20)
    rc = stow_arm();
  else if (cst[i] == 21)
    return tuck_elbow(1);              // joint mode
  else 
    return -1;                         // no failure handler
  return command_bot(rc);
}


//= Start trying to indicate an object with the hand.
// only functions on semnet allowed, cannot interrogate vision system
// returns 1 if okay, -1 for interpretation error

int jhcManipulate::man_point0 (const jhcAliaDesc *desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;

  // argument parsing
  if ((cobj[i] = desc->Val("arg")) == NULL)
    return -1;

  // intialize state
  citem[i] = -1;             // await access to vision
  cflag[i] = 0;              // no workspace violations
  return 1;
}


//= Continue trying to indicate an object with the hand.
// sets up continuing request to body if not finished
// only has one step so no finite state machine (or failure cleanup)
// returns 1 if done, 0 if still working, -1 for failure

int jhcManipulate::man_point (const jhcAliaDesc *desc, int i)
{
  double ht, dp;

  // lock to sensor cycle and check update standard cached info
  if (!rwi->Accepting())
    return 0;
  if (rwi->Ghost() || (arm->CommOK() <= 0))
    return err_arm();
  init_vals(i);

  // make sure target object still is known then check for interference
  if ((citem[i] = sobj->ObjTrack(rpt->VisID(cobj[i]))) < 0)
    return err_gone(cobj[i]);
  if (chk_hand(NULL) <= 0)
    return -1;

  // possibly print entry message and choose zero gaze offset
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: point %s\n", cbid[inst], cobj[inst]->Nick());
    cpos[inst].Zero();
    cst2[inst] = 1;
  }
  
  // track object to get desired arm pose (just over centroid)
  sobj->World(end, citem[inst]);
  end.SetZ(sobj->MaxZ(citem[inst]) + graze + over);
  aim.SetVec3(corner_ang(end.X(), end.Y()), -tip, 0.0);
  wid = 0.0;
  target = 1;

  // see if close enough to desired pose
  ht = lift->Height();
  dp = arm->PosErr3D(perr, end, ht, 0);
  if (dp > ptol)
  {
    // fail if not making progress unless in right ballpark (hope for best)
    if (chk_stuck(dp) <= 0)
      return command_bot(0);
    jprintf(2, dbg, "    stuck: dp = %s\n", dp);
    if (arm->PosOffset3D(end, ht) > cont)
      return err_reach(cobj[inst]);
  }
  return final_pose(1);                // success
}


///////////////////////////////////////////////////////////////////////////
//                              Take Phases                              //
///////////////////////////////////////////////////////////////////////////

//= Look at target object and determine a good grasp point on its top.
// assumes cobj[i] bound to target node and citem[i] is vision track
// sets cpos[i] to object-relative grasp point, cdir[i] to absolute angles
// sets camt[i] to width (negative if top is roundish)
// returns 1 if done, 0 if still working, -1 for timeout
// NOTE: always first phase of man_grab, man_lift, man_take (second of man_move)

int jhcManipulate::assess_obj ()
{
  jhcMatrix obj(4);
  double da, ht = lift->Height();
  int t = citem[inst];

  // temporarily assign grasp point as object center (zero offset)
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: pick grasp %s\n", cbid[inst], cobj[inst]->Nick());
    cpos[inst].Zero();
    cst2[inst] = 1;
  } 

  // leave arm in current location but look toward object
  end.RelVec3(*pos, 0.0, 0.0, ht);
  aim.Copy(*dir);
  wid = arm->Width();
  worksp = 0;                          // no body motion
  target = 1;                          // look at source object

  // see if close enough to proper gaze direction
  sobj->World(obj, t);
  da = (rwi->neck)->GazeErr(obj, ht);
  if (da > atol)
  {
    if (chk_stuck(0.1 * da) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: gaze = %3.1f\n", da);
  }

  // possibly wait several cycles for target object to be actively detected
  if (!update_src(ccnt[inst]))
    return 0;
  return compute_src(camt[inst], cpos[inst], cdir[inst]);
}


//= Move hand to via point appropriate for object.
// assumes cpos[i] is object-relative grasp point, cdir[i] is absolute angles
// updates target grasp parameters if object currently detected
// returns 1 if done, 0 if still working, -1 for timeout
// NOTE: always comes from deploy_arm

int jhcManipulate::goto_via ()
{ 
  double dp, da, dw, ht = lift->Height();

  // possibly print entry message then refind grasp (if needed) and maybe destination
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: approach %s\n", cbid[inst], cobj[inst]->Nick());
    cst2[inst] = 1;
  }
  if (update_src(ccnt[inst]))
    compute_src(camt[inst], cpos[inst], cdir[inst]);
  if (cmode[inst] >= 0)
    if (update_dest(ccnt2[inst]))
      compute_dest(cend[inst], caux[inst], cflag[inst], 0);   

  // set for hand destination but high enough to clear object top
  src_full(end, 1);
  aim.Copy(cdir[inst]);
  wid = arm->MaxWidth();               // fully open
  dmode = 0x4;                         // exact R orientation (0100)
  target = 1;                          // look at pickup location

  // see if close enough to desired pose
  dp = arm->PosErr3D(perr, end, ht, 0);
  da = arm->DirErr(derr, aim, 0);
  dw = arm->WidthErr(wid);
  if ((dp > ptol) || (da > atol) || (dw > wtol))
  {
    // fail if not making progress unless in right ballpark (hope for best)
    if (chk_stuck(perr.SumAbs3() + 0.1 * derr.SumAbs3() + dw) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: perr = %s, derr = %s, werr = %3.1f\n", perr.ListVec3(txt), derr.ListVec3(txt2), dw);
    if (arm->PosOffset3D(end, ht) > cont)
      return err_reach(cobj[inst]);
  }

  // wait for oscillation to subside then possibly tell final accuracy
  if (arm->Static() < detwang)
  {
    if (arm->Static() == (detwang - 1))
      jprintf(2, dbg, "    detwang ...\n");
    return 0;
  }
  return final_pose(1);
}


//= Move hand to grasp point appropriate for object.
// assumes cpos[i] is object-relative grasp point, cdir[i] is absolute angles
// returns 1 if done, 0 if still working, -1 for timeout
// NOTE: always comes from goto_via

int jhcManipulate::goto_grasp ()
{ 
  double dp, da, ht = lift->Height();

  // possibly print entry message 
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: engulf %s\n", cbid[inst], cobj[inst]->Nick());
    cst2[inst] = 1;
  }

  // select desired final arm pose 
  src_full(end, 0);
  aim.Copy(cdir[inst]);
  wid = arm->MaxWidth();               // fully open
  pmode = 0x3;                         // exact YX position (011)
  dmode = 0x4;                         // exact R orientation (0100)
  target = 1;                          // look at pickup location

  // see if close enough to desired pose
  dp = arm->PosErr3D(perr, end, ht, 0);
  da = arm->DirErr(derr, aim, 0);
  if ((dp > ptol) || (da > atol))
  {
    // fail if not making progress unless in right ballpark (hope for best)
    if (chk_stuck(perr.SumAbs3() + 0.1 * derr.SumAbs3()) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: perr = %s, derr = %s\n", perr.ListVec3(txt), derr.ListVec3(txt2));
    if (arm->PosOffset3D(end, ht) > cont)
      return err_reach(cobj[inst]);
  }

  // possibly tell final accuracy
  return final_pose(1);
}


//= Close fingers around object until standard force achieved.
// assumes cpos[i] is object-relative grasp point, cdir[i] is absolute angles
// sets "held", "htrk", "nose", and "hang" once hold is established
// returns 1 if done, 0 if still working, -1 if fingers empty or timeout
// NOTE: always comes from goto_grasp

int jhcManipulate::close_fingers ()
{
  // possibly print entry message and remember start time (wtim not chk_stuck) 
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: wrap %s\n", cbid[inst], cobj[inst]->Nick());
    fcnt = 0;
    ct0[inst] = jms_now();   
    cst2[inst] = 1;         
  }

  // stay at grasping point but squeeze hand closed 
  src_full(end, 0);
  aim.Copy(cdir[inst]);
  wid = 0.0;                           // ballistic move (not force)
  pmode = 0x4;                         // exact Z position (100)
  dmode = 0x4;                         // exact R orientation (0100)
  target = 1;                          // look at pickup location

  // fail if closed on air, succeed if moderate force
  if (arm->Width() < wmin)             
    return err_grasp();
  fcnt++;
  if ((hold - arm->Squeeze()) > ftol) 
    fcnt = 0;
  if (fcnt < 5)
  {
    // quit if motion takes too long
    if (jms_elapsed(ct0[inst]) < wtim)
      return 0;
    jprintf(2, dbg, "    stuck: timeout\n");
    return err_grasp();
  }

  // remember engagement details and generate "holding" event
  record_grip(cobj[inst], citem[inst]);
  msg_hold();                           
  wt = -1.0;                           // not measured yet
  return final_pose(0);
}


//= Raise grasped object slightly off table to allow moving.
// sets cpos[i] and cdir[i] to absolute arm pose to achieve after lifting object
// records estimate of weight of held object in "wt"
// returns 1 if done, 0 if still working, -1 if timeout (non-movable?)
// NOTE: earlier grasp steps might have been skipped 

int jhcManipulate::lift_off ()
{
  double under, z3d = pos->Z() + lift->Height();   

  // remember starting absolute pose 
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: lift %s\n", cbid[inst], cobj[inst]->Nick());
    cpos[inst].SetVec3(pos->X(), pos->Y(), over + z3d);
    cdir[inst].SetVec3(dir->P(), -tip, 0.0);
    cst2[inst] = 1;
  }

  // maintain planar pose but raise hand while still squeezing (no gaze command)
  end.Copy(cpos[inst]);
  aim.Copy(cdir[inst]);
  wid = -hold;                         // maintain force
  dmode = 0xE;                         // any pan, exact RT orientation (1110)

  // see if high enough yet (ignore any other errors)
  under = end.Z() - z3d;
  if (under > ztol)
  {
    if (chk_stuck(under) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: zerr = %3.1f\n", -under);
    return final_pose(0);
  }

  // ideal time to estimate weight
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

//= Look at target object and determine a good grasp point on its top.
// assumes cmode[i] is spatial relation, cref[i] is reference object track
// temporarily sets cend[i] to approximate location of destination description
// sets anchor-relative hand destination cend[i] and absolute orientation caux[i] 
// returns 1 if done, 0 if still working, -1 for timeout
// NOTE: always first phase of man_move

int jhcManipulate::assess_spot ()
{
  jhcMatrix anchor(4), full(4);
  double da, ht = lift->Height();

  // possibly announce entry then set initial spot to look at
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: pick dest %s\n", cbid[inst], cobj[inst]->Nick());
    if (cref[inst] < 0)
      sobj->NearTable(cend[inst], citem[inst]);  // closest table point
    else
      cend[inst].Zero();                         // anchor location
    cst2[inst] = 1;
  }

  // leave arm in current location but look toward rough destination
  end.RelVec3(*pos, 0.0, 0.0, ht);
  aim.Copy(*dir);
  wid = ((held != NULL) ? -hold : wmin);
  worksp = 0;                          // no body motion
  target = 2;                          // look at destination

  // see if close enough to proper gaze direction
  dest_full(anchor, 0);
  da = (rwi->neck)->GazeErr(anchor, ht);
  if (da > atol)
  {
    if (chk_stuck(0.1 * da) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: gaze = %3.1f\n", da);
  }

  // possibly wait several cycles for location anchors then find rough destination
  if (!update_dest(ccnt2[inst]))
    return 0;
  return compute_dest(cend[inst], caux[inst], cflag[inst], 0);   
}


//= Move object at travel height over to destination location.
// assumes cend[i] is anchor-relative destination, caux[i] is absolute orientation
// returns 1 if done, 0 if still working, -1 if timeout 
// NOTE: earlier grasp steps might have been skipped

int jhcManipulate::xfer_over ()
{
  jhcMatrix full(4), anchor(4);
  double ht = lift->Height();

  // possibly announce entry then adjust rough destination for actual grip parameters
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: transfer %s\n", cbid[inst], cobj[inst]->Nick());
    dest_full(full, 0);
    adjust_dest(full, caux[inst], cflag[inst] & 0x10);
    dest_rel(cend[inst], full);
    cst2[inst] = 1;
  }
  if (update_dest(ccnt2[inst]))
    compute_dest(cend[inst], caux[inst], cflag[inst], 1);

  // set for hand destination but up a bit (keep squeezing)
  dest_full(end, 1);
  aim.SetVec3(caux[inst], -tip, 0.0);
  wid = -hold;
  pmode = 0x4;                         // exact Z position (0100)
  dmode = 0x6;                         // exact RT orientation (0110)
  target = 2;                          // look at destination
  tim = 1.0;                           // long timeout

  // see if at destination position yet (ignore any orientation error)
  if (arm->PosOffset3D(end, ht) > cont)
  {
    // if not making progress then fail 
    arm->PosErr3D(perr, end, ht, 0);
    if (chk_stuck(perr.SumAbs3()) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: perr = %s\n", perr.ListVec3(txt));
    return err_reach(cspot[inst]);
  }

  // wait for oscillation to subside then possibly tell final accuracy
  if (arm->Static() < detwang)
  {
    if (arm->Static() == (detwang - 1))
      jprintf(2, dbg, "    detwang ...\n");
    return 0;
  }
  return final_pose(1);
}


//= Descend toward destination height until upwards force felt.
// assumes cpos[i] is anchor-relative hand destination, caux[i] is absolute orientation
// returns 1 if done, 0 if still working (never fails)
// NOTE: always comes from xfer_over

int jhcManipulate::place_on ()
{
  double dx, dy, dz, ht = lift->Height();

  // possibly announce entry
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: deposit %s\n", cbid[inst], cobj[inst]->Nick());
    cst2[inst] = 1;
  }

  // aim for destination pose while still squeezing 
  dest_full(end, 0);
  aim.SetVec3(caux[inst], -tip, 0.0);
  wid = -hold;
  pmode = 0x3;               // exact YX position (0011)
  dmode = 0x7;               // exact RTP orientation (0111)
  target = 2;                // look at destination

  // see if approximately in contact with surface
  dx = pos->X() - end.X();
  dy = pos->Y() - end.Y();
  dz = (pos->Z() + ht) - end.Z();
  if ((fabs(dx) > ptol) || (fabs(dy) > ptol) || (dz > dtol))
  {
    // quit if no longer making progress
    if (chk_stuck(dz + fabs(dx) + fabs(dy)) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: perr = [%3.1f %3.1f %3.1f]\n", dx, dy, dz);
  }

  // possibly tell final accuracy
  return final_pose(1);
}


//= Open fingers wide to release object.
// assumes cend[i] is anchor-relative hand destination, caux[i] is absolute orientation
// sets "held" to NULL at end
// returns 1 if done, 0 if still working
// NOTE: always comes from place_on

int jhcManipulate::release_obj ()
{
  // remember desired width and start time (wtim not chk_stuck)
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: release %s\n", cbid[inst], cobj[inst]->Nick());
    camt[inst] = arm->Width() + 2.0 * loose;
    ct0[inst] = jms_now();         
    cst2[inst] = 1;
  }

  // use object itself as point to back away from
  dest_full(end, 0);
  aim.SetVec3(caux[inst], -tip, 0.0);
  wid = camt[inst];
  pmode = 0x7;                         // exact ZYX position (0111)
  dmode = 0x7;                         // exact RTP orientation (0111)
  target = 2;                          // look at destination

  // see if hand is open wide enough yet
  if (fabs(arm->Width() - wid) > wtol)
  {
    // quit if motion takes too long but never fail
    if (jms_elapsed(ct0[inst]) < wtim)
      return 0;
    jprintf(2, dbg, "    stuck: timeout\n");
  }

  // update_helds generates "not holding" event and marks hand as empty
  return final_pose(0);
}

 
//= Retract arm (possibly with object) in preparation for environmental navigation.
// returns 1 if done, 0 if still working (never fails)

int jhcManipulate::stow_arm ()
{
  double pk, wz, under, dp, da, ht = lift->Height();

  // set up absolute retracted pose (only lift if some obstruction)
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: stow %s\n", cbid[inst], 
            ((cobj[inst] != NULL) ? cobj[inst]->Nick() : "arm"));

    pk = obj_peaks(arm->retx, arm->rety, arm->Width(), 1);
    if (pk <= sobj->ztab)
      wz = pos->Z() + ht;              // current height okay
    else
      wz = pk + over + hang;
    cpos[inst].SetVec3(arm->retx, arm->rety, wz);
    cdir[inst].SetVec3(arm->rdir, -tip, 0.0);
    cst2[inst] = 1;
  }

  // go to stowed pose while still squeezing any held object (no gaze command)
  end.Copy(cpos[inst]);
  aim.Copy(cdir[inst]);
  under = end.Z() - (pos->Z() + ht);
  if (held != NULL)
    wid = -hold;
  else if (under > ztol)               // don't close until hi enough
    wid = arm->Width();
  else
    wid = wmin;
  if (under > ztol)                    // slow until clear
    sp = __max(0.5, sp);
  worksp = 1;                          // only up/dn adjustments

  // see if close enough to desired pose
  dp = arm->PosErr3D(perr, end, ht, 0);
  da = arm->DirErr(derr, aim, 0);
  if ((dp > ptol) || (da > atol))
  {
    // quit is stop making progress
    if (chk_stuck(perr.SumAbs3() + 0.1 * derr.SumAbs3()) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: perr = %s, derr = %s\n", perr.ListVec3(txt), derr.ListVec3(txt2));
  }
  return final_pose(0);                // always advance to tuck          
}


//= Make sure elbow is tight to robot for body navigation.
// uses arm in joint mode so should not call command_bot afterward
// sets caux[i] to elbow, camt[i] to mid-arm lift, cdir[i] to wrist angles
// returns rc if done (possibly a fail), 0 if still working (never fails)
// NOTE: always comes from stow_arm, no states ever follow this

int jhcManipulate::tuck_elbow (int rc)
{
  const jhcMatrix *cfg = arm->ArmConfig();
  jhcMatrix tuck(6);
  double ds, de, etol = 10.0;

  // remember outer arm configuration (but change elbow if stow failed)
  if (cst2[inst] == 0)
  {
    jprintf(1, dbg, "|- Manipulate %d: tuck %s\n", cbid[inst], 
            ((cobj[inst] != NULL) ? cobj[inst]->Nick() : "arm"));
    if (fabs(arm->JtAng(1) - arm->rete) > etol)
      caux[inst] = arm->rete;                    
    else
      caux[inst] = cfg->VRef(1);
    camt[inst] = cfg->VRef(2);
    cdir[inst].SetVec3(cfg->VRef(3), cfg->VRef(4), cfg->VRef(5)); 
    ct0[inst] = jms_now();       
    cst2[inst] = 1;  
  }

  // keep original joint angles except shoulder 
  tuck.VSet(0, arm->rets);
  tuck.VSet(1, caux[inst]);
  tuck.VSet(2, camt[inst]);
  tuck.VSet(3, cdir[inst].VRef(0));
  tuck.VSet(4, cdir[inst].VRef(1));
  tuck.VSet(5, cdir[inst].VRef(2));

  // check if shoulder sufficiently close 
  ds = fabs(cfg->VRef(0) - tuck.VRef(0));
  de = fabs(cfg->VRef(1) - tuck.VRef(1));
  if ((ds > arm->align) || (de > arm->align))
  {
    // quit if not making progress
    if (chk_stuck(0.1 * (ds + de)) <= 0)
    {
      // send joint angles to arm and close hand (no gaze command)
      arm->CfgTarget(tuck, 1.0, cbid[inst]);
      arm->HandTarget(((held != NULL) ? -hold : wmin), 1.0, cbid[inst]);                              
      return 0;
    }
    jprintf(2, dbg, "    stuck: ds = %3.1f, de = %3.1f\n", ds, de);
  }

  // return value given, possibly advancing state if normal sequence
  if (rc <= 0)
    return rc;
  cst[inst] += 1;            // largely for GUI
  cst2[inst] = 0;            
  ct0[inst]  = 0;           
  return 1;                         
}


///////////////////////////////////////////////////////////////////////////
//                           Sequence Helpers                            //
///////////////////////////////////////////////////////////////////////////

//= Set up some standard control values.

void jhcManipulate::init_vals (int i)
{
  // current instance information
  inst = i;                    
  msg = 0;                     

  // default command details
  sp = csp[i];              
  pmode = 0;                
  dmode = 0;   
  worksp = 2;
  target = 0;
  tim = 0.5;               

  // never erase occluded objects, target will not change shape
  sobj->RetainAll();           
  sobj->KeepShape(citem[i]);   

  // roughly update absolute deposit location on surface using odometry
  if (cref[i] < 0)
    (rwi->base)->AdjustTarget(cend[i]);
}


//= Make sure hand is holding the expected object (or nothing).
// automatically generates error messages for drop or interference
// <pre>
// width   held    expect   
// -----  ------  --------  
//   -     NULL     NULL    -> okay
//   -     NULL     obj-1   -> error: not holding obj-1
//   -     obj-2    NULL    -> error: holding obj-2
//   -     obj-2    obj-1   -> error: holding obj-2
//   0     obj-1    obj-1   -> error: dropped obj-1
//   w     obj-1    obj-1   -> okay
// </pre>
// returns 1 if okay, 0 or negative for problem

int jhcManipulate::chk_hand (jhcAliaDesc *expect)
{
  double sqz0 = 5.0;

  // holding nothing
  if (held == NULL) 
  {
    if (expect != NULL)
      return err_lack(expect);
    return 1;
  }

  // holding something
  if (held != expect)
    return err_lack(expect);
  if (arm->SqueezeGoal() > 0.0)
    if ((arm->Width() < wmin) || (arm->Squeeze() < sqz0))
    {
      err_lack(held);
      return clear_grip();
    }
  return 1;
}


//= Detect lack of substantial error reduction over given time.
// detects lack of improvement over "prog" for "tim" (about 15 cycles)
// hardcoded for 0.1" position progress, otherwise scale error first 
// assumes member variable "inst" bound with proper command index
// if body adjustment active then evaluates progress of that not total goal
// assumes "end" is arm target, cflag[i] bit 3 set if change from last mode
// returns 1 if at asymptote, 0 if still moving toward goal

int jhcManipulate::chk_stuck (double diff)
{
  double prog = 0.1;     
  double chg, err = diff;
  int fix = cflag[inst] & 0x07;

  // if body motion needed then only monitor relevant coordinate
  if (fix != 0)
  {
    err = diff_workspace(fix);
    if (fix <= 2)
      tim = 2.0;                       // very slow translation
    else if (fix <= 4)
      tim = 1.0;                       // slow base rotation
  }
  if ((cflag[inst] & 0x08) != 0)  
    ct0[inst] = 0;                     // restart timeout if new violation     
  chg = cerr[inst] - err;

  // reset timer if minimal progress being made
  if ((ct0[inst] == 0) || (chg >= prog))
  {
    ct0[inst] = jms_now();
    cerr[inst] = err;
  }
  else if (jms_elapsed(ct0[inst]) > tim)
    return 1;

double secs = jms_elapsed(ct0[inst]);
if (secs > 0.5)
jprintf("chk_stuck: secs = %4.2f\n", secs);

  return 0;
}


//= Tell arm command versus actual pose at the end of some step.
// can also give details of position error in 3D
// always returns 1 for convenience

int jhcManipulate::final_pose (int xyz) 
{  
  double ht = lift->Height();

  jprintf(3, dbg, "      command: %s %s\n", end.ListVec3(txt), aim.ListVec3(txt2));
  jprintf(3, dbg, "      -> pose: [%3.1f %3.1f %3.1f] %s\n", pos->X(), pos->Y(), pos->Z() + ht, dir->ListVec3(txt));
  if ((xyz > 0) && (dbg >= 2))
  {
    arm->PosErr3D(perr, end, ht, 0);
    jprintf("    final offset = %s\n", perr.ListVec3(txt)); 
  }
  return 1;
}


//= Fail sequence by transitioning to the cleanup phase.
// always returns 0 for convenience

int jhcManipulate::fail_clean ()
{
  cst[inst]  = 20;           // special constant
  cst2[inst] = 0;            // mark as newly started
  ct0[inst] = 0;       
  cflag[inst] &= 0xF0;       // no body fixes required      
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                          Coordinated Motion                           //
///////////////////////////////////////////////////////////////////////////

//= Act on commands generated if successful and possibly advance state.
// assumes member variable "inst" bound with proper command index
// assumes "end", "aim", "wid", and "sp" encode desired absolute action
// pmode bits:              2 = exact Z,    1 = exact Y,    0 = exact X
// dmode bits: 3 = any pan, 2 = exact roll, 1 = exact tilt, 0 = exact pan
// target: 0 = no gaze, 1 = pickup location, 2 = deposit location
// stores workspace limit violations in cflag[i] 

int jhcManipulate::command_bot (int rc)
{
  jhcMatrix view(4);
  double pan, tilt, ht = lift->Height(), ex = end.X(), ey = end.Y(), ez = end.Z() - ht;
  double ztop = ((pos->Y() < (arm->rety + fwd)) ? arm->retz : wz1), gtim = 1.0;                        
  int fix = 0;
  
  // go to cleanup phase if error (stow and tuck never fail)
  if (rc < 0)
    return fail_clean();     

  // try to fix any important arm problems then fill in rest of parameters
  fix = chk_outside(cflag[inst], ex, ey, ez);
  if ((ez - pos->Z()) > ttol)
  {
    if (fix == 6)                                          // up but no base motion 
      lift->LiftShift(ez - (ztop - zup) + ztra, 0.5, cbid[inst]);     
    else                                                   // up but no lateral motion
      arm->PosTarget(pos->X(), pos->Y(), ez, sp, cbid[inst]);
  }
  else if (fix != 0)
    adj_workspace(fix, ex, ey, ez);                        // attempt body jockeying
  else
  {
    arm->PosTarget3D(end, ht, sp, cbid[inst], pmode);      // normal trajectory          
    arm->DirTarget(aim, sp, cbid[inst], dmode);
  }
  arm->ArmTarget(*pos, *dir, 1.0, 1.0, cbid[inst]);        // only as default
  arm->HandTarget(wid, sp, cbid[inst]);                              

  // decide where to look (shift closer if fixing +y violation)
  if (target > 0)
  {
    if (target == 1)
      src_full(view, 0);                                   // pickup location
    else 
      dest_full(view, 0);                                  // deposit location
    (rwi->neck)->AimFor(pan, tilt, view, ht);
    if (ey > wy1)                                          // approach surface edge
      tilt -= edge;
    (rwi->neck)->GazeFix(pan, tilt, gtim, cbid[inst]);
  }

  // possibly shift to next sequence state on following cycle 
  if (rc >= 1)
  {
    cst[inst] += 1;
    cst2[inst] = 0;          // mark as newly started
    ct0[inst]  = 0;    
    cflag[inst] &= 0xF0;     // no body fixes required      
  }
  return 0;
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
  int fix = old & 0x07, bad = 0;

  // if no adjustment allowed then clear violation mode
  if (worksp <= 0)
  {
    old &= 0xF0;                                 
    if (fix != 0)
      old |= 0x08;
    return 0;
  }

  // find current violations in priority order
  if (gz > ztop)
    bad = 6;                 
  else if (gz < wz0)
    bad = 5;                 
  else if (worksp >= 2)
  {
    if (gx > wx1)
      bad = 4;                 
    else if (gx < wx0)
      bad = 3;                 
    else if (gy < wy0)                           // if true then gy > wy1 false
      bad = 1; 
    else if (gy > wy1) 
      if ((gap = surf_gap()) > 0.0)              // only set if movement possible
        bad = 2;                 
  }

  // see if old violation resolved (or remove if being ignored)
  if ((worksp <= 1) && (fix < 5))      
    fix = 0;                                     
  if (((fix == 6) && (gz < (ztop - zup))) ||
      ((fix == 5) && (gz > (wz0 + zdn))) ||
      ((fix == 4) && (gx < (wx1 - xbd))) ||
      ((fix == 3) && (gx > (wx0 + xbd))) ||
      ((fix == 2) && (gy < (wy1 - ybd))) || 
      ((fix == 1) && (gy > (wy0 + ybd))))
    fix = 0;

  // stop advancing if movement no longer possible
  if ((fix == 2) && (bad < 2))
    if ((gap = surf_gap()) <= 0.0)
      fix = 0;

  // continue to fix old error or switch to new one
  if (bad > fix)
    fix = bad;
  if (fix == (old & 0x07))
    old &= 0xF7;                                 // clear change flag
  else
  {
    old &= 0xF0;                                 // clear all workspace data
    old |= (0x08 | fix); 
    jprintf(2, dbg, "      workspace: fix %d - %s\n", fix, prob[fix]);
  }
  return fix;                
}


//= Move lift stage or base to fix most important workspace violation.
// fixes one body error at a time: Z then X then finally Y
// assumes "gap" variable already has allowable travel distance if fix = 2
// returns 1 or 0 if special motion undertaken, -1 if arm should be moveable

int jhcManipulate::adj_workspace (int fix, double gx, double gy, double gz)
{
  jhcEliBase *base = rwi->base;
  double trav, nd, azm, ang, ztop = ((pos->Y() < (arm->rety + fwd)) ? arm->retz : wz1);                        

  // sanity check
  if ((fix <= 0) || (fix > 6))
    return -1;

  // z and y errors fixed by moving some number of inches
  if (fix == 6)
    return lift->LiftShift(gz - (ztop - zup) + ztra, 0.5, cbid[inst]);    // too low -> up 
  if (fix == 5) 
    return lift->LiftShift(gz - (wz0 + zdn) - ztra, 0.5, cbid[inst]);     // too high -> down  
  if (fix == 2)
  {
    trav = gy - (wy1 - ybd) + ytra;
    return base->MoveTarget(__min(trav, gap), 1.0, cbid[inst]);           // too far -> fwd 
  }
  if (fix == 1)
    return base->MoveTarget(gy - (wy0 + ybd) - ytra, 1.0, cbid[inst]);    // too close -> rev

  // x errors fixed by rotating some number of degrees
  nd = -sqrt(gx * gx + gy * gy);
  azm = R2D * asin(gx / nd);                
  if (fix == 4)
    ang = azm - R2D * asin((wx1 - xbd) / nd) - xtra;       // too far right -> neg turn (CW)
  else
    ang = azm - R2D * asin((wx0 + xbd) / nd) + xtra;       // too far left -> pos turn (CCW)
  return base->TurnTarget(ang, 1.0, cbid[inst]);                
}


//= Find out how far robot can travel before bumping into edge of surface.

double jhcManipulate::surf_gap () const
{
  double x[4], y[4];
  double hw = (rwi->nav).rside, hpel = (rwi->s3).I2P(hw), mx = (rwi->tab).MidX();
  double m, clear, lx = mx - hpel, rx = mx + hpel;
  int ly, ry;

  // impossible to estimate if no current surface
  if (!(rwi->tab).SurfOK())
    return 0.0;

  // find intersections of travel corridor sides with bottom of sensor beam
  if ((rwi->s3).BeamCorners(x, y, (rwi->tab).SurfHt()) <= 0)
    return 0.0;
  if ((lx < x[0]) || (rx > x[1]))      // outside swx to sex span
    return 0.0;
  m = (y[1] - y[0]) / (x[1] - x[0]);
  ly = ROUND(m * (lx - x[0]) + y[0]);
  ry = ROUND(m * (rx - x[0]) + y[0]);

  // see how far robot can travel before bumping into surface edge
  clear = (rwi->tab).SurfMove(hw, __max(ly, ry)) - prow;
  return __max(0.0, clear);
}


//= Tell how close some workspace violation is to being fixed.
// fix: 6 = +z, 5 = -z, 4 = +x, 3 = -x, 2 = +y, 1 = -y, 0 = none
// assumes "end" holds desired grip position in robot-centric coords

double jhcManipulate::diff_workspace (int fix) const
{
  double x = end.X(), y = end.Y(), z = end.Z() - lift->Height();
  double ztop = ((pos->Y() < (arm->rety + fwd)) ? arm->retz : wz1);

  if (fix == 6)
    return(z - (ztop - zup));
  if (fix == 5)
    return((wz0 + zdn) - z);
  if (fix == 4)
    return(x - (wx1 - xbd));
  if (fix == 3)
    return((wx0 + xbd) - x);
  if (fix == 2)
    return(y - (wy1 - ybd));
  if (fix == 1)
    return((wy0 + ybd) - y);
  return 0.0;
}


///////////////////////////////////////////////////////////////////////////
//                           Object Acquisition                          //
///////////////////////////////////////////////////////////////////////////

//= Find full hand grasp location based on offset from tracked object position.
// can optionally raise the position to be the skyhook approach height 
// returns absolute orientation of target object, negative if no longer tracked

double jhcManipulate::src_full (jhcMatrix& full, int up) 
{
  double peak, ang = sobj->World(full, citem[inst]);

  if (ang >= 0.0)
  {
    full.IncVec3(cpos[inst]);
    if (up > 0)
    {
      peak = obj_peaks(full.X(), full.Y(), arm->MaxWidth(), 0);
      full.SetZ(__max(full.Z(), peak) + over);
    }
  }
  return ang;
}


//= Convert a full source position into an anchor-relative offset vector.

void jhcManipulate::src_rel (jhcMatrix& rel, const jhcMatrix& full) const
{
  sobj->World(rel, citem[inst]);
  rel.DiffVec3(full, rel);
}


//= Determine whether source should be recomputed on this cycle.
// wants base to be stopped and target object actually detected
// generally needed to handle changed orientation of surface, hence hand offset
// set cnt = 0 externally to get first estimate

bool jhcManipulate::update_src (int& fail) const
{
  int stopped = (rwi->base)->Static(), tries = 5;

  // if base is moving then reset count and do not update
  if (stopped < -3)
    fail = 0;                
  if (stopped < park)
    return false;

  // if waiting to estimate then check if target is currently detected 
  if (fail >= tries)
    return false;
  if (sobj->Component(citem[inst]) <= 0)
    fail++;
  else
    fail = tries;                      // estimate once now
  return(fail == tries);               // too many failures
}


//= Find grip position relative to object centroid and absolute orientation.
// also determines expected gripper width (negative if roundish)
// returns 1 if okay, -1 if object height or width bad

int jhcManipulate::compute_src (double& grip, jhcMatrix& rel, jhcMatrix& dir)
{
  jhcMatrix full(4), obj(4);
  double pan, ang;
  int rc, t = citem[inst];

  // get hand location and angle for grabbing but barf if object top is too big
  if ((rc = pick_grasp(grip, pan, full, t)) < 0)
    return err_size(rc);
  if (rc == 1)
    grip = -grip;                      // mark as roundish

  // record object-relative grasp point and absolute orientation
  src_rel(rel, full);
  dir.SetVec3(pan, -tip, 0.0);
  if (dbg >= 3)
  {
    ang = sobj->World(obj, t);
    jprintf("    hand %s @ %3.1f <- object %s @ %3.1f\n", full.ListVec3(txt), pan, obj.ListVec3(txt2), ang);
  }
  return 1;
}


//= Find grasp position, orientation, and gripper width for object with given track index.
// prefers estimate based on top slice but will default to using full object footprint
// returns 2 if elongated, 1 if roundish, -1 = too big, -2 = too flat

int jhcManipulate::pick_grasp (double& open, double& ang, jhcMatrix& grab, int t) const
{
  double flat = 0.5;
  double wx, wy, wz, wid, len, rads, ht = sobj->SizeZ(t);

  // find location, orientation, and size of graspable top
  if (ht < flat)
    return -2;
  if ((ang = sobj->FullTop(wx, wy, wid, len, t, knob)) < 0.0)     
  {
    ang = sobj->World(wx, wy, t);      
    wid = sobj->Minor(t);
    len = sobj->Major(t);
  }
  if (wid > choke)                                                 
    return -1;                                  
  
  // if top elongated then align with it, else orient gripper for convenience
  wz = sobj->MinZ(t) + __max(graze, ht - drop);
  open = arm->MaxWidth();
  ang = easy_grip(ang, len / wid, corner_ang(wx, wy));

  // make sure gripper enagages small objects
  rads = D2R * ang;
  wx += gulp * cos(rads);
  wy += gulp * sin(rads); 
  grab.SetVec3(wx, wy, wz);
  return(((len / wid) > ecc0) ? 2 : 1);
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
// awlays returns -1 for convenience

int jhcManipulate::clear_grip ()
{
  held = NULL;
  htrk = -1;
  nose = 0.0;
  left = 0.0;
  hang = graze;
  skew = 0.0;
  drop = 0;
  return -1;
}


//= Compute relative geometry of hand versus object.
// sets "held", "htrk", "nose", "left", "hang", and "skew" member variables
// assumes cdir[i] holds intended grip angle, camt[i] tells whether roundish

void jhcManipulate::record_grip (jhcAliaDesc *obj, int t)
{
  double ang, wx, wy, pan0, dx, dy, rads, c, s, z3d;

  // remember semantic node and visual track
  held = obj;
  htrk = t;

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
  jprintf(3, dbg, "    nose = %3.1f, left = %3.1f, hang = %3.1f, skew = %3.1f\n", nose, left, hang, skew);
}


///////////////////////////////////////////////////////////////////////////
//                         Trajectory Utilities                          //
///////////////////////////////////////////////////////////////////////////

//= Find maximum height of any object crossed by trajectory path.
// can optionally exclude held object if carry > 0 (cf. free_space)
// returns height above supporting surface in inches

double jhcManipulate::obj_peaks (double wx, double wy, double fsep, int carry) 
{
  double wid, len;
  int i, h20, n = sobj->ObjLimit(); 

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
//                      Destination Dtermination                         //
///////////////////////////////////////////////////////////////////////////

//= Return the absolute anchor position based on current position of references.

void jhcManipulate::anchor_loc (jhcMatrix& loc) const
{
  jhcMatrix obj2(4);

  // "down" destination is absolute and has no anchor
  if (cref[inst] < 0)
  {    
    loc.Zero();
    return;
  }

  // destination anchor generally based on some tracked object(s)
  sobj->World(loc, cref[inst]);
  if (cref2[inst] < 0)
    return;
  sobj->World(obj2, cref2[inst]);
  loc.MixVec3(obj2, 0.5);               
}


//= Fill vector with full absolute position for hand at deposit spot.
// can optionally boost z coordinate to clear any intervening obstacles

void jhcManipulate::dest_full (jhcMatrix& full, int up)
{
  double mtn;

  anchor_loc(full); 
  full.IncVec3(cend[inst]);
  if (up > 0)
  {
    mtn = obj_peaks(full.X(), full.Y(), arm->Width(), 1) + hang;
    full.SetZ(__max(full.Z(), mtn) + over);
  }
}


//= Convert a full destination position into an anchor-relative offset vector.

void jhcManipulate::dest_rel (jhcMatrix& rel, const jhcMatrix& full) const
{
  anchor_loc(rel);
  rel.DiffVec3(full, rel);
}


//= Determine whether destination should be recomputed on this cycle.
// wants base to be stopped and reference object(s) actually detected
// generally needed to handle changed orientation of surface, hence hand offset
// set cnt = 0 externally to get first estimate

bool jhcManipulate::update_dest (int& fail) const
{
  int stopped = (rwi->base)->Static(), tries = 5;

  // if base is moving then reset count and do not update
  if (stopped < -3)
    fail = 0;        
  if (stopped < park)
    return false;

  // if waiting to estimate then check if target is currently detected 
  if (fail >= tries)
    return false;
  if ((cref[inst] >= 0) && (sobj->Component(cref[inst]) <= 0))
    fail++;
  else if ((cref2[inst] >= 0) && (sobj->Component(cref2[inst]) <= 0))
    fail++;
  else
    fail = tries;                      // estimate once now
  return(fail == tries);               // too many failures
}


//= Find optimal hand deposit position relative to location anchor and absolute grasp angle.
// assumes cmode[i] is spatial relation, cref[i] is reference object track
// assumes hand-to-object offsets "nose", "left", "hang" and "skew" have been set
// sets bit 4 of eflag if underlying location is not oriented (for adjust_dest)
// returns 1 if successful, -1 if no such location possible
// Note: before grip "rel" essentially gets set to zero 

int jhcManipulate::compute_dest (jhcMatrix& rel, double& pan, int& eflag, int adj) 
{
  jhcMatrix loc(4), hand(4);
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
  dest_rel(rel, hand);
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


//= Find desired robot relative coordinates for bottom of held object.
// also tells final gripper orientation needed to fit in spot
// prefers top slice for "on" but defaults to full object footprint
// deposit location estimate is tighter if all objects are curently detected
// sets variables "xdest" and "ydest" as selected map point (for debugging)
// returns 1 if successful, -1 if no space (values unchanged)

int jhcManipulate::dest_bottom (jhcMatrix& loc, double& pan)
{
  jhcMatrix dir(4);
  double ang, wid, len, ix, iy,  wx, wy;
  int rn = cmode[inst], a = cref[inst], a2 = cref2[inst], t = htrk;

  // simple handler for "on" some object
  if (rn == ON)
  {
    if ((ang = sobj->FullTop(wx, wy, wid, len, a, mesa)) < 0.0)     
    {
      ang = sobj->World(wx, wy, a);      
      wid = sobj->Minor(t);
      len = sobj->Major(t);
    }
    loc.SetVec3(wx, wy, sobj->MaxZ(a));
    pan = easy_grip(ang, len / wid, corner_ang(wx, wy));
    return 1;
  }

  // attempt to find satisfactory map deposit location and orientation 
  free_space(((t == a) || (t == a2)) ? -1 : t);
  dest_ref(ix, iy, t, rn, a, a2);
  ang = dest_ang(ix, iy, t, rn, a, a2);
  if (pick_spot(xdest, ydest, ix, iy, ang, t, rn, a, a2) <= 0)
    return -1;

  // convert to map pose to full world coordinates
  sobj->PelsXY(wx, wy, xdest, ydest);
  loc.SetVec3(wx, wy, sobj->ztab);
  pan = sobj->FullAngle(ang);
  return 1;
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
  double dx, dy, f;

  // special case of setting down (examine around closest point of table)
  if (rn == DOWN)
  {
    sobj->NearTable(loc, citem[inst]);
    sobj->ViewPels(ix, iy, loc.X(), loc.Y());
    return;
  }

  // assume reference point at anchor object
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
    if ((a = sobj->ObjTrack(rpt->VisID(ref))) < 0)
      return err_gone(ref);
  }

  // dual references ("between")
  if (rn == TWIXT)
  {
    if ((ref = place->Val("ref2")) == NULL)
      return -2;
    if ((a2 = sobj->ObjTrack(rpt->VisID(ref))) < 0)
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
  part = rpt->NewNode("obj");
  own = rpt->NewProp(part, "ako", "arm");
  rpt->AddArg(own, "wrt", rpt->Self());
  arm = rpt->Resolve(part);                      // find or make part
  fail = rpt->NewNode("act", "work", 1);
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
  fail = rpt->NewNode("act", "fit", 1, 1.0, 1);
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
  fail = rpt->NewNode("act", "see", 1);
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
  fail = rpt->NewNode("act", "reach", 1, 1.0, 1);
  rpt->AddArg(fail, "agt", rpt->Self());
  if (obj->ObjNode())
    rpt->AddArg(fail, "obj", obj);               // target
  else if (cmode[inst] == ON)
    rpt->AddArg(fail, "obj", obj->Val("ref"));   // "on" something
  else
  {
    obj2 = rpt->NewNode("place");
    rpt->NewProp(obj2, "ako", "destination");    // other locations
    rpt->AddArg(fail, "obj", obj2);               
  }
  rpt->FinishNote(fail);
  return -1;
}


//= Say that the robot did not grasp the object.
// <pre>
//   NOTE[ act-1 -lex-  acquire
//               -ach-  0
//               -obj-> obj-1
//               -agt-> self-1 ]
// </pre>
// always returns -1 for convenience

int jhcManipulate::err_grasp ()
{
  jhcAliaDesc *fail;

  // possibly tell arm position
  final_pose(0);                      
  if (cobj[inst] == NULL)
    return -1;

  // generate failure event
  rpt->StartNote();
  fail = rpt->NewNode("act", "acquire", 1, 1.0, 1);
  rpt->AddArg(fail, "obj", cobj[inst]);
  rpt->AddArg(fail, "agt", rpt->Self());
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

  // sanity check
  if (obj == NULL)
    return -1;

  // event generation (adds as failure reason even if whole motion succeeds)
  rpt->StartNote();
  fail = rpt->NewNode("act", "hold", 1);
  rpt->AddArg(fail, "obj", obj);
  rpt->AddArg(fail, "agt", rpt->Self());
  rpt->FinishNote(fail);
  return -1;
}


//= Say that the robot is holding the remembered object.
// <pre>
//   NOTE[ act-1 -lex-  hold
//               -obj-> obj-1
//               -agt-> self-1 ]
// </pre>

void jhcManipulate::msg_hold ()
{
  jhcAliaDesc *act;

  // sanity check
  if (held == NULL)
    return;

  // event generation
  rpt->StartNote();
  act = rpt->NewNode("act", "hold");
  rpt->AddArg(act, "obj", held);
  rpt->AddArg(act, "agt", rpt->Self());
  rpt->FinishNote(NULL);
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
  (rwi->sobj).ViewPels(gx[0], gy[0], wx0, wy0);
  (rwi->sobj).ViewPels(gx[1], gy[1], wx1, wy0);
  (rwi->sobj).ViewPels(gx[2], gy[2], wx1, wy1);
  (rwi->sobj).ViewPels(gx[3], gy[3], wx0, wy1);
  DrawPoly(dest, gx, gy, 4, 1, r, g, b);
}


//= Force action to be applied to some particluar object track.

void jhcManipulate::ForceItem (int t)
{
  jhcAliaDesc *n = rpt->NewNode("obj");

  // bind object node to track
  rpt->VisAssoc(sobj->ObjID(t), n);

  // object specification
  cobj[0]  = n;
  citem[0] = t;

  // destination specification
  cspot[0] = n;
  cmode[0] = -1;             // never recompute destination
  cref[0]  = -1;
  cref2[0] = -1;

  // motion parameters 
  csp[0]  = 1.0;
  cbid[0] = 10;

  // initial state
  cst[0] = 1;                // no need to find deposit spot
  ct0[0] = 0;

  // motion state
  ccnt[0]  = 0;              // detection trial
  ccnt2[0] = 0;
  cflag[0] = 0;              // no workspace errors
}


//= Force a particular deposit position and default orientation.

void jhcManipulate::ForceDest (double wx, double wy, double wz) 
{
  cend[0].SetVec3(wx, wy, wz);    
  caux[0] = -90.0;                     // any convenient
}

