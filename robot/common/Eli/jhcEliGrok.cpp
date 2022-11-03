// jhcEliGrok.cpp : post-processed sensors and innate behaviors for ELI robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#include "Interface/jms_x.h"           // common video
#include "Interface/jrand.h"

#include "Eli/jhcEliGrok.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcEliGrok::~jhcEliGrok ()
{
}


//= Default constructor initializes certain values.
// creates member instances here so header file has fewer dependencies

jhcEliGrok::jhcEliGrok ()
{
  // connect head tracker to other pieces
  fn.Bind(&s3);
  tk.Bind(&s3);

  // no body yet
  clr_ptrs();

  // change some head finder/tracker defaults
  s3.SetMap(144.0, 144.0, 72.0, 0.0, -2.0, 84.0, 0.3, 0.0);      // 12' wide x 12' front
  s3.ch = 30.0;                                                  // allow seated
  s3.h0 = 40.0;
  s3.ring = 200.0;                                               // long range okay (16.7')
  s3.edn = 5.0;

  // change some face finder/gaze defaults
  fn.xsh = 0.4;                                                  // big search box
  fn.ysh = 0.4;
  fn.dadj = 2.0;                                                 // head is shell

  // configure object finding map
  sobj.SetMap(108.0, 63.0, 24.0, -6.0, -2.0, 18.0, 0.15, 28.5);   // 720 x 421 map
  sobj.hmix = 0.0;

  // set up display strings
  nmode[0][0] = '\0';
  strcpy_s(nmode[1], "--  APPROACH");
  strcpy_s(nmode[2], "--  Follow");
  strcpy_s(nmode[3], "--  wander ...");
}


//= Attach extra processing to physical or simulated body.

void jhcEliGrok::BindBody (jhcEliBody *b)
{
  // possible unbind body and pieces
  clr_ptrs();
  if (b == NULL)
    return;
  phy = 1;

  // make direct pointers to body parts (for convenience)
  // and use voice tracker mic for speaker direction
  body = b;
  base = &(b->base);
  neck = &(b->neck);
  lift = &(b->lift);
  arm  = &(b->arm);
  mic  = &(b->mic);
  tk.RemoteMic(mic);         
}


//= Null pointers to body and subcomponents.

void jhcEliGrok::clr_ptrs ()
{
  body = NULL;
  base = NULL;
  neck = NULL;
  lift = NULL;
  arm  = NULL;
  mic  = NULL;
  tk.RemoteMic(NULL);
  phy = 0;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters controlling where heads are expected to be found.

int jhcEliGrok::vis_params (const char *fname)
{
  jhcParam *ps = &vps;
  int ok;

  ps->SetTag("grok_vis", 0);
  ps->NextSpecF( &lvis,   20.0, "Max left head offset (deg)");
  ps->NextSpecF( &rvis,   20.0, "Max right head offset (deg)");
  ps->NextSpecF( &tvis,   10.0, "Max top head offset (deg)");
  ps->NextSpecF( &bvis,   10.0, "Max bottom head offset (deg)");
  ps->Skip();
  ps->NextSpecF( &gtime,   0.3, "Gaze response (sec)"); 

  ps->NextSpecF( &side,  -50.0, "Body rotate thresh (deg)");    // 0 = don't 
  ps->NextSpecF( &btime,   1.5, "Rotate response (sec)");      
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters controlling navigation related gaze activities.

int jhcEliGrok::sacc_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("grok_sacc", 0);
  ps->NextSpecF( &hem,      6.0, "Forward motion blocked (in)");
  ps->NextSpecF( &umat,     0.5, "Fraction unknown doormat");
  ps->NextSpecF( &sacp,    25.0, "Saccade lateral pan (deg)");
  ps->NextSpecF( &sact,   -25.0, "Saccade nearby tilt (deg)");
  ps->NextSpecF( &sact2,  -65.0, "Saccade floor tilt (deg)"); 
  ps->Skip();

  ps->NextSpecF( &road,   -40.0, "Path check tilt (deg)");      
  ps->NextSpecF( &cruise,   2.0, "Path check interval (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcEliGrok::Defaults (const char *fname)
{
  int ok = 1;

  ok &= vis_params(fname);
  ok &= sacc_params(fname);
  ok &= fn.Defaults(fname);      // does s3 also
  ok &= nav.Defaults(fname);
  ok &= sobj.Defaults(fname);
  ok &= tab.Defaults(fname);
  return ok;
}


//= Read just deployment specific values from a file.

int jhcEliGrok::LoadCfg (const char *fname)
{
  int ok = 1;

  if (body != NULL)
    ok &= body->Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcEliGrok::SaveVals (const char *fname) 
{
  int ok = 1;

  ok &= vps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= fn.SaveVals(fname);      // does s3 also
  ok &= nav.SaveVals(fname);
  ok &= sobj.SaveVals(fname);
  ok &= tab.SaveVals(fname);
  return ok;
}


//= Write current deployment specific values to a file.

int jhcEliGrok::SaveCfg (const char *fname) const
{
  int ok = 1;

  if (body != NULL)
    ok &= body->SaveVals(fname);
  fn.SaveCfg(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Restart background processing loop.
// if rob > 0 then runs with body active (else motion disabled)
// if behaviors > 0 then runs with autonomic behaviors (else only commands)
// NOTE: body should be reset outside of this !!!

void jhcEliGrok::Reset (int rob, int behaviors)
{
  jhcVideoSrc *v;
  const jhcImg *src;

  // disable background threads then reset vision components
  jhcBackgRWI::Stop();
  s3.Reset();
  fn.Reset();
  nav.Reset();
  sobj.Reset();
  tab.SetSize(s3.map);
  tab.Reset(); 

  // configure body 
  phy = 0;
  seen = 0;
  batt = 0;
  if (body != NULL)
  {
    // configure actuators
    if (rob > 0)
    {
      phy = 1; 
      base->Zero();
      body->InitPose();          // used to leave height unaltered (-1.0)
      body->Update(-1, 1);       // sensor info will be waiting and need to be read
    }
    else
      body->StaticPose();        // set neck angles and head height for static image

    // configure vision elements
    if ((v = body->vid) != NULL)
    {
      // setup navigation
      nav.SrcSize(v->XDim(), v->YDim(), v->Focal(1), v->Scaling(1));
      sobj.SrcSize(v->XDim(), v->YDim(), v->Focal(1), v->Scaling(1));

      // make status images
      body->BigSize(mark);
      mark.FillArr(0);
      mark2.InitSize(nav.map);

      // configure visual analysis for camera images
      src = body->View();
      s3.SetSize(*src);
    }
  }

  // high-level commands
  wlock = 0;
  wwin = 0;
  slock = 0;
  vlock = 0;
  xlock = 0;
  flock = 0;

  // navigation goal and FSM
  act = 0;
  feet = 0;
  ahead = 0;

  // restart background loop, which first generates a body Issue call
  reflex = behaviors;
  jhcBackgRWI::Reset();
}


//= Read and process all sensory information from robot.
// this all happens when background thread in rwi update is quiescent
// returns 1 if okay, 0 or negative for error

int jhcEliGrok::Update (int voice, UL32 resume)
{
  // do slow vision processing in background (already started usually)
  if (jhcBackgRWI::Update(0) <= 0)
    return 0;

  // do fast sound processing in foreground (needs voice)
  if (mic != NULL)
    mic->Update(voice);
  tk.Analyze(voice);

  // create pretty picture then enforce min wait (to simulate robot)
  cam_img();
  nav_img();
  jms_resume(resume);  
  return 1;
}


//= Call at end of main loop to stop background processing and robot motion.

void jhcEliGrok::Stop ()
{
  jhcBackgRWI::Stop();
  if (phy > 0)
    body->Limp();
}


///////////////////////////////////////////////////////////////////////////
//                          Interaction Overrides                        //
///////////////////////////////////////////////////////////////////////////

//= Get new sensor inputs from robot body (override).
// waits (if needed) for data to be received (no mic)

void jhcEliGrok::body_update ()
{
  int bsamp = 150;                     // roughly every 5 sec

jtimer(2, "body update");
  // get actuator positions, etc.
  if (phy > 0)
  {
    body->Update(-1, 0);
    if (batt++ >= bsamp)               // requires extra communication
    {
      body->Charge(1);                 // caches values
      batt = 0;
    }   
  }

  // use old person map to guess table height for this cycle (both threads need)
//  if (!neck->Saccade())
    sobj.ztab = tab.PickPlane(s3.map, s3.IPP(), s3.HMIN(), s3.HMAX());
jtimer_x(2);
}


//= Process images for navigation and person finding in primary background thread (override).

void jhcEliGrok::interpret ()
{
  jhcMatrix pos(4), dir(4);
  double ht = lift->Height();

  // needs depth data
  if (seen <= 0)
    return;

jtimer(3, "interpret (tab + face + nav)");
jtimer(4, "find table (bg1)");
  // find support surface as a target in old map (s3 in other thread)
  neck->HeadPose(pos, dir, ht);
//  if (!neck->Saccade())
    tab.FindSurf(pos, ht);
jtimer_x(4);  

  // find new person location based on current camera pose
jtimer(5, "face track (bg1)");
  adjust_heads();
  fn.SetCam(pos, dir);
  if (!neck->Saccade())
    fn.Analyze(body->Color(), body->Range());
jtimer_x(5);

jtimer(6, "navigation (bg1)");
  // update navigation map
  nav.AdjustMaps(base->StepFwd(), base->StepLeft(), base->StepTurn());
  if (!neck->Saccade())                // needed
    nav.RefineMaps(body->Range(), pos, dir);
  nav.ComputePaths();
jtimer_x(6);
jtimer_x(3);
}


//= Alter expected position and visibilty of heads based on robot state.
// NOTE: odometry only provides coarse adjustment, true tracking is more accurate

void jhcEliGrok::adjust_heads ()
{
  jhcBodyData *p;
  double p0, t0, perr, terr, ht = lift->Height();
  int i, n = s3.PersonLim(1);

  // consider each potential person track 
  for (i = 0; i < n; i++)
  {
    // check for valid person 
    p = &(s3.dude[i]);
    if (p->TrackID() <= 0)
      continue;

    // adjust head for base motion (ignores hands)
    base->AdjustTarget(*p);

    // determine if head should have been matchable (allows erasure)
    neck->AimFor(p0, t0, *p, ht);
    perr = neck->PanErr(p0, 0, 0); 
    terr = neck->TiltErr(t0, 0, 0); 
    if ((perr >= -lvis) && (perr <= rvis) && (terr >= -bvis) && (terr <= tvis))
      p->vis = 1;
    else 
      p->vis = 0;
  }
}


//= Process images for object finding in secondary background thread (overrride).

void jhcEliGrok::interpret2 ()
{
  jhcMatrix pos(4), dir(4);

  // needs depth data
  if (seen <= 0)
    return;

jtimer(7, "interpret2 (objects)");
  // detect objects (ztab already set by body_update)
  sobj.AdjBase(base->StepSide(), base->StepFwd(), base->StepTurn()); 
  neck->HeadPose(pos, dir, lift->Height());
  sobj.AdjNeck(pos, dir);
  if (!neck->Saccade())                // needed
    sobj.FindObjects(body->Color(), body->Range(), ArmMask(limb));
jtimer_x(7);
}


//= Run local behaviors then send arbitrated commands to body (override).

void jhcEliGrok::body_issue ()
{
  const jhcVideoSrc *v;

jtimer(8, "body_issue");
  // record current time
  tnow = jms_now();
  if (body == NULL)
    return;

  // interpret high-level commands (in order of priority)
  act = base_mode();
  assert_scan();
  assert_watch();
  assert_seek();
  assert_servo();
  assert_explore();

  // start actuator commands and get new raw images
  if (phy > 0)
{
jtimer(9, "Issue");
    body->Issue();
jtimer_x(9);
}
jtimer(10, "UpdateImgs");
  seen = body->UpdateImgs(); 
  if ((v = body->vid) != NULL)
    if (v->IsClass("jhcListVSrc") > 0)
      seen = 1;                                  // for static images
jtimer_x(10);
jtimer_x(8);
}


//= Set descriptive string telling what high-level command the robot is performing.
// picks dominant mode (in priority order)

int jhcEliGrok::base_mode ()
{
  int top = __max(xlock, __max(vlock, slock));

  if (top <= 0)
    return 0;
  if (slock == top)
    return 1;
  if (vlock == top)
    return 2;
  if (xlock == top)
    return 3;
  return -1;                 // should never get here
}


///////////////////////////////////////////////////////////////////////////
//                       Combination Sensing                             //
///////////////////////////////////////////////////////////////////////////

//= Find person with a face closest in 3D to camera origin in projection space.
// can optionally take a forward offset from robot origin and min face detections
// returns tracker index not person ID

int jhcEliGrok::ClosestFace (double front, int cnt) const
{
  jhcMatrix pos(4);
  double dx, dy, d2, best;
  int i, n = s3.PersonLim(), win = -1;

  for (i = 0; i < n; i++)
    if (s3.PersonOK(i) && s3.Visible(i))
      if (fn.FaceCnt(i) >= cnt)
      {
        s3.Head(pos, i);
        dx = pos.X();
        dy = pos.Y() - front;
        d2 = dx * dx + dy * dy;
        if ((win < 0) || (d2 < best))
        {
          win = i;
          best = d2;
        }
      }
  return win;
}


//= Find the head closest to view direction with the given deviation.
// assumes view = 0 means forward wrt microphone position (approx. robot center)
// if successful binds position vector to center (else unchanged)
// returns index of winner, negative if nothing suitable

int jhcEliGrok::HeadAlong (jhcMatrix& head, double aim, double dev) const
{
  jhcMatrix pos(4);
  double off, best;
  int i, n = s3.PersonLim(), win = -1;

  for (i = 0; i < n; i++)
    if (s3.PersonOK(i) && s3.Visible(i))
    {
      s3.Head(pos, i);
      off = fabs(mic->OffsetAng(pos, aim));
      if ((win < 0) || (off < best))
      {
        win = i;
        best = off;
      }
    }
  if ((win < 0) || (best > dev))
    return -1;
  head.Copy(pos);
  return win;
}


///////////////////////////////////////////////////////////////////////////
//                     High-Level People Commands                        //
///////////////////////////////////////////////////////////////////////////

//= Connect some tracked person to motion controller semi-permanently.
// "wiring" persists even without command until overridden (e.g. id = 0)
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliGrok::WatchPerson (int id, int bid)
{
  if (bid <= wlock) 
    return 0;
  wlock = bid;
  wwin = id;
  return 1;
}


//= Turn selected person into tracking motion.
// needs to be called before body->Issue() due to target persistence
// keeps trying to watch person regardless of visibility, angle, or distance

void jhcEliGrok::assert_watch ()
{
  const jhcMatrix *targ;
  double ang, horizon = 120.0, crane = 120.0;
  int bid = wlock;
 
  if ((wlock <= 0) || (wwin <= 0))
    return;

  // see if most recently selected person is still close enough
  if ((targ = s3.GetID(wwin)) != NULL)
    if (targ->PlaneVec3() <= horizon)
    {
      // make sure that the person is in the visible zone
      ang = targ->PanVec3() - 90.0;
      if (ang <= -180.0)
        ang += 360.0;
      else if (ang > 180.0)
        ang -= 360.0;
      if (fabs(ang) <= crane)
      {
        OrientToward(targ, bid);
        return;
      }
    }

  // give up on watching that particular person
  wwin = 0;
  wlock = 0;
}


//= Aim camera at target location, rotating body if needed.
// set "turn" to zero or negative to prevent body rotation

void jhcEliGrok::OrientToward (const jhcMatrix *targ, int bid)
{
  double pan, tilt;

  if (targ == NULL) 
    return;
  neck->AimFor(pan, tilt, *targ, lift->Height());
  neck->GazeFix(pan, tilt, gtime, bid);
  if ((side > 0.0) && (fabs(pan) > side))
    base->TurnFix(pan, btime, 1.5, bid);         // swivel base
}


//= Gives the max absolute pan or tilt error between current gaze and some person.
// useful for telling if move is progressing or has finished
// returns negative if person is no longer visible

double jhcEliGrok::PersonErr (int id) const
{
  const jhcMatrix *targ;

  if ((targ = s3.GetID(id)) == NULL)
    return -1.0;
  return neck->GazeErr(*targ, lift->Height());
}


///////////////////////////////////////////////////////////////////////////
//                     High-Level Navigation Commands                    //
///////////////////////////////////////////////////////////////////////////

//= Drive the robot toward the target location (y is forward, not x).
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliGrok::SeekLoc (double tx, double ty, double sp, int bid)
{
  if (bid <= slock)
    return 0;
  slock = bid;
  sx = tx;
  sy = ty;
  ssp = sp;
  return 1;
}


//= Take necessary (pre-emptive) body actions to approach winning target.

void jhcEliGrok::assert_seek ()
{
  double trav, head;
  int bid = slock;

  // check if some command, then reset arbitration for next round
  if (slock <= 0)
    return;
  slock = 0;   
/*
  // pick a steering angle and travel speed
  nav.Avoid(trav, head, sx, sy);
  base->MoveTarget(trav, ((trav < 0.0) ? 0.7 : ssp), bid);
  base->TurnFix(head, 1.0, 1.0, bid);             
*/
}


//= Try to keep robot center at td = off from target with azimuth ta (0 is forward).
// tries to aim toward target at all times, moving backward if too close
// generally speed to follow (1.5) is higher than speed to approach (1.0)
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority
// NOTE: should also call MapPath with same bid to make sure feet are free

int jhcEliGrok::ServoPolar (double td, double ta, double off, double sp, int bid)
{
  if (bid <= vlock)
    return 0;
  vlock = bid;           
  vd = td;
  va = ta;
  vsp = sp;
  voff = off;
  return 1;
}


//= Take necessary (pre-emptive) body actions to maintain distance from target.

void jhcEliGrok::assert_servo ()
{
  double trav, head;              
  int bid = vlock;

  // check if some command, then reset arbitration for next round
  if (vlock <= 0)
    return;
  vlock = 0;          

  // pick a steering angle and travel speed (or update map)
  nav.Swerve(trav, head, vd, va, voff);
  base->TurnTarget(head, 1.0, bid);
  base->MoveTarget(trav, 1.0, bid);
}


//= Tell the current distance (in) from front of robot to target location.

double jhcEliGrok::FrontDist (double td, double ta) const
{
  double rads = D2R * ta, dx = -td * sin(rads), dy = td * cos(rads) - nav.rfwd;

  return sqrt(dx * dx + dy * dy); 
}


//= Wander aimlessly without hitting things.

int jhcEliGrok::Explore (double sp, int bid)
{
  if (bid <= xlock)
    return 0;
  xlock = bid;
  xsp = sp;
  return 1;
}


//= Drive along frontmost path that is sufficiently long. 
// assumes assert_scan called early to referesh map if needed

void jhcEliGrok::assert_explore ()
{
  double trav, head;              
  int bid = xlock;

  // check if some command, then reset arbitration for next round
  if (xlock <= 0)
    return;
  xlock = 0;          

  // pick a steering angle and travel speed (gaze ahead and down)
  nav.Wander(trav, head);
  base->TurnTarget(head, 0.5, bid);
  base->MoveTarget(trav, xsp, bid);
}


//= Request mapping in front of robot, sometimes at feet if needed.
// predicate Survey() will be true when head is being moved

int jhcEliGrok::MapPath (int bid)
{
  if (bid <= flock)
    return 0;
  flock = bid + 1;           // a bit of a hack
  return 1;
}


//= Set robot gaze appropriately to build portion of map needed.
// generally most important motion so called first in sequence of "asserts"

void jhcEliGrok::assert_scan ()
{
  int bid = flock;

  // check if some command, then reset arbitration for next round
  if (flock <= 0)
  {  
    // reset state when not in use
    feet = 0;      
    return;
  }
  flock = 0;         

  // look at feet if needed, otherwise occasionally look ahead
  if ((quick_survey(bid) > 0) || (jms_elapsed(ahead) < cruise))
    return;
jprintf("* CRUISE\n");
  if (!neck->GazeDone(0.0, road))
    neck->GazeTarget(0.0, road, 1.0, 1.0, bid);      
  else
    ahead = jms_now();                 // reset cycle timer
}


//= Use a series of 4 rapid gaze fixations to map floor ahead of robot.
//   feet: 0 = check if foot saccade needed
//         1 = await mid-right saccade
//         2 = await low-right saccade
//         3 = await low-left saccade
//         4 = await mid-left saccade
//         5 = await reset
// returns 1 if moving head, 0 if no gaze command

int jhcEliGrok::quick_survey (int bid)
{
  double pan, tilt;

  // reset saccade when free to travel or unknown doormat area
  if (feet >= 5)
  {
    if (nav.Tight(hem) && !nav.Blind(umat))
      return 0;
    feet = 0;
  }

  // if contrained movement then start sequence
  if (feet <= 0)
  {
    if (!nav.Tight(hem))
      return 0;
    feet = 1;
  }

  // force rapid look at a sequence of 4 fixations (no time gaps)
  while (feet < 5)
  {
    pan  = (((feet == 1) || (feet == 2)) ? -sacp : sacp);
    tilt = (((feet == 1) || (feet == 4)) ?  sact : sact2);
    if (neck->GazeDone(pan, tilt))
      feet++;
    else
    {
      // no base motion during saccade
      neck->GazeTarget(pan, tilt, -1.5, -1.5, bid);      
      base->DriveTarget(0.0, 0.0, 1.0, bid);      
      return 1;
    }
  }
  return 0;        // feet just set to 5
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Overlay stick figure of arm onto camera image in some color.
// optionally show ray of some length (inches) from grasp point
// best if arm angles are not changing (i.e. don't call during update)
// NOTE: this is only for the color camera view

int jhcEliGrok::Skeleton (jhcImg& dest, double ray) const
{
  jhcMatrix pos(4), off(4);
  int jt[5] = {1, 2, 3, 6, 7};
  double px, py, ix, iy;
  int i; 

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcEliGrok::Skeleton");

  // draw links and grip location
  ImgJt(px, py, 0);
  for (i = 0; i < 5; i++)
  {
    ImgJt(ix, iy, jt[i]);
    DrawLine(dest, px, py, ix, iy, 3, -5);
    px = ix;
    py = iy;
  }
  CircleEmpty(dest, px, py, 10, 3, -5);  

  // ray in grip direction 
  if (ray <= 0.0)
    return 1;
  off.SetVec3(arm->ToolX() + ray, 0.0, 0.0);
  (arm->jt[6]).GlobalMap(pos, off);
  s3.ImgPtZ(ix, iy, pos.X(), pos.Y(), pos.Z() + lift->Height());
  DrawLine(dest, px, py, ix, iy, 3, -3); 
  return 1;
}


//= Overlay stick figure of arm onto overhead object map image.
// optionally show ray of some length (inches) from grasp point
// NOTE: this is only for the overhead map view (adjusts for neck pose)

int jhcEliGrok::MapArm (jhcImg& dest, double ray) const
{
  jhcMatrix pos(4), off(4);
  int jt[5] = {1, 2, 3, 6, 7};
  double px, py, mx, my;
  int i; 

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcEliGrok::MapArm");

  // draw links from shoulder and circle grip location
  arm->JtPos(pos, 0);
  sobj.ViewPels(px, py, pos.X(), pos.Y());
  for (i = 0; i < 5; i++)
  {
    // select some joint and get map coords
    if (jt[i] == 7)
      arm->Position(pos);
    else if (jt[i] == 2)
      arm->LiftBase(pos);      // looks better on screen
    else
      arm->JtPos(pos, jt[i]);
    sobj.ViewPels(mx, my, pos.X(), pos.Y());

    // draw segment
    DrawLine(dest, px, py, mx, my, 3, -5);
    px = mx;
    py = my;
  }
  CircleEmpty(dest, px, py, 10, 3, -5);  

  // ray in grip direction 
  if (ray <= 0.0)
    return 1;
  off.SetVec3(arm->ToolX() + ray, 0.0, 0.0);
  (arm->jt[6]).GlobalMap(pos, off);
  sobj.ViewPels(mx, my, pos.X(), pos.Y());
  DrawLine(dest, px, py, mx, my, 3, -3); 
  return 1;
}


//= Overlay stick figure of arm with some blocks onto overhead map image.
// useful for suppressing object detection of arm itself
// NOTE: this is only for the overhead map view (adjusts for neck pose)

const jhcImg *jhcEliGrok::ArmMask (jhcImg& dest, int clr) const
{
  jhcMatrix pos(4), off(4);
  int jt[5] = {1, 2, 3, 6, 7};
  double px, py, mx, my, side = -1.5, lift = 1.5, wrist = 2.0, hand = 3.0;
  int i; 

  // set image size and clear background
  dest.SetSize(sobj.map, __max(1, dest.Fields()));
  if ((clr > 0) || (phy <= 0))
    dest.FillArr(0);
  if (phy <= 0)
    return &dest;

  // draw links from shoulder and circle grip location
  arm->JtPos(pos, 0);
  sobj.ViewPels(px, py, pos.X(), pos.Y());
  for (i = 0; i < 5; i++)
  {
    // select some joint and get map coords
    if (jt[i] == 7)
      arm->Position(pos);
    else if (jt[i] == 2)
      arm->LiftBase(pos);      // looks better on screen
    else
      arm->JtPos(pos, jt[i]);
    sobj.ViewPels(mx, my, pos.X(), pos.Y());

    // draw segment
    DrawLine(dest, px, py, mx, my, 3, -5);
    px = mx;
    py = my;
  }

  // block out big section around lift pod
  arm->LiftBase(pos, side);
  sobj.ViewPels(mx, my, pos.X(), pos.Y());
  CircleFill(dest, mx, my, sobj.I2P(lift), -5); 

  // block out around back of gripper
  arm->JtPos(pos, 6);
  sobj.ViewPels(mx, my, pos.X(), pos.Y());
  CircleFill(dest, mx, my, sobj.I2P(wrist), -5); 

  // block out around front of gripper
  arm->Position(pos);
  sobj.ViewPels(mx, my, pos.X(), pos.Y());
  CircleFill(dest, mx, my, sobj.I2P(hand), -5); 
  return &dest;
}


//= Find the pixel location of a particular arm joint.
// jt: 0 = shoulder,   1 = elbow,     2 = FOREARM lift, 
//     3 = wrist roll, 4 = wrist pan, 5 = wrist tilt, 
//     6 = jaw axis,   7 = mid tips
// returns non-scaled z coordinate (for use with jhcSurface3D::WorldPt)

double jhcEliGrok::ImgJt (double& ix, double& iy, int jt) const
{
  jhcMatrix pos(4);

  if ((jt < 0) || (jt > 7))
    return 0;
  if (jt == 7)
    arm->Position(pos);
  else if (jt == 2)
    arm->LiftBase(pos);      // looks better on screen
  else
    arm->JtPos(pos, jt);
  return s3.ImgPtZ(ix, iy, pos.X(), pos.Y(), pos.Z() + lift->Height());
}


//= Get angle difference of the click location versus projected jt1 relative to projected jt0.
// primarily used by arm calibration routines in jhcBanzaiDoc

double jhcEliGrok::ImgVeer (int mx, int my, int jt1, int jt0) const
{
  double x0, y0, x1, y1, ang, click, diff;

  // find interjoint angle and click angle
  ImgJt(x0, y0, jt0);
  ImgJt(x1, y1, jt1);
  ang = R2D * atan2(y1 - y0, x1 - x0);
  click = R2D * atan2(my - y0, mx - x0);

  // normalize difference
  diff = click - ang;
  if (diff > 180.0)
    diff -= 360.0;
  else if (diff <= -180.0)
    diff += 360.0;
  return diff;
}


//= Make a pretty version of color image showing relevant items.

void jhcEliGrok::cam_img ()  
{
  jhcMatrix target(4);
  int t, gz, sp, nt, col;

  // get current color camera view
  if (!(body->NewFrame()))
    return;
  body->ImgBig(mark);

  // show people
  gz = fn.GazeNew();
  sp = tk.Speaking();
  nt = s3.PersonLim();
  for (t = 0; t < nt; t++)
//    if (s3.PersonState(t) > 0)
    {
      if (s3.PersonID(t) == sp) 
        col = 2;                                 // green   = speaker
      else if (t == gz)
        col = 3;                                 // yellow  = newest direct gaze
      else if (fn.FaceCnt(t) > 0)
        col = 5;                                 // magenta = only if face seen
      else 
        continue;
      s3.PersonCam(mark, t, 0, 1, 0, col);
    }
  fn.FacesCam(mark);                             // cyan    = faces

  // objects (green = focal, yellow = nodified, magenta = others, cyan = target)
  sobj.AttnCam(mark, 2, 3, 5);     
  arm->PosGoal(target, lift->Height());
  sobj.MarkCam(mark, target, 6);
}


//= Make pretty version of overhead map and robot sensors.

void jhcEliGrok::nav_img ()
{
  // basic environment with obstacles
  if (!(body->NewFrame()))
    return;
  nav.LocalMap(mark2);

  // directions of motion possible and robot footprint
  nav.Dists(mark2, 1);
  nav.RobotCmd(mark2, base->TurnIncGoal(), base->MoveIncGoal());
  nav.RobotBody(mark2);

  // path recently traveled and target (if any)
  nav.Tail(mark2);
  if (act == 1)                        // approach
    nav.Target(mark2, sx, sy);
  else if (act == 2)                   // follow
    nav.Target(mark2, vd, va, 1);
}

