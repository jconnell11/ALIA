// jhcEliGrok.cpp : post-processed sensors and innate behaviors for ELI robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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
  tab.SetMap(108.0, 63.0, 24.0, -6.0, -2.0, 18.0, 0.15, 28.5);   // 720 x 421 map
  tab.hmix = 0.0;
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


//= Generate a string telling what is controlling the robot's motion.

const char *jhcEliGrok::NavGoal ()
{
  *nmode = '\0';
  if ((approach > 0) && (approach > follow))
    strcpy_s(nmode, "APPROACH");
  else if ((follow > 0) && (follow >= approach))
    strcpy_s(nmode, "FOLLOW");
  return nmode;
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


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcEliGrok::Defaults (const char *fname)
{
  int ok = 1;

  ok &= watch.Defaults(fname);
  ok &= vis_params(fname);
  ok &= fn.Defaults(fname);      // does s3 also
  ok &= nav.Defaults(fname);
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

  ok &= watch.SaveVals(fname);
  ok &= vps.SaveVals(fname);
  ok &= fn.SaveVals(fname);      // does s3 also
  ok &= nav.SaveVals(fname);
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
      body->InitPose(-1.0);
      body->Update(-1, 1);       // sensor info will be waiting and need to be read
    }
    else
      body->StaticPose();        // set neck angles and head height for static image

    // configure vision elements
    if ((v = body->vid) != NULL)
    {
      // setup navigation
      nav.SrcSize(v->XDim(), v->YDim(), v->Focal(1), v->Scaling(1));
      tab.SrcSize(v->XDim(), v->YDim(), v->Focal(1), v->Scaling(1));

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

  // navigation goal
  approach = 0;
  follow = 0;
  feet = 0;

  // restart background loop, which first generates a body Issue call
  reflex = behaviors;
  watch.Reset();
  jhcBackgRWI::Reset();
}


//= Read and process all sensory information from robot.
// this all happens when background thread in rwi update is quiescent
// returns 1 if okay, 0 or negative for error

int jhcEliGrok::Update (int voice, UL32 resume)
{
jtimer(1, "jhcEliGrok::Update");
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
jtimer_x(1);
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

  // use old person map to guess table height for this cycle
  tab.ztab = s3.PickPlane(lift->Height() + tab.lrel, tab.ppel, 4, tab.flip);
jtimer_x(2);
}


//= Process images for navigation and person finding in primary background thread (override).

void jhcEliGrok::interpret ()
{
  jhcMatrix pos(4), dir(4);

  // needs depth data
  if (seen <= 0)
    return;
jtimer(9, "interpret (bg1)");

  // find new person location based on current camera pose
jtimer(3, "face track (bg1)");
  adjust_heads();
  neck->HeadPose(pos, dir, lift->Height());
  fn.SetCam(pos, dir);
  if (!neck->Saccade())
    fn.Analyze(body->Color(), body->Range());
jtimer_x(3);

jtimer(4, "navigation (bg1)");
  // update navigation map
  nav.AdjustMaps(base->StepFwd(), base->StepLeft(), base->StepTurn());
  if (!neck->Saccade())
    nav.RefineMaps(body->Range(), pos, dir);
  nav.ComputePaths();
jtimer_x(4);
jtimer_x(9);
}


//= Alter expected position and visibilty of heads based on robot state.

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

  // detect objects (tab.ztab already set by body_update)
jtimer(5, "objects (bg2)");
  neck->HeadPose(pos, dir, lift->Height());
  tab.AdjTracks(pos, dir);
  if (!neck->Saccade())
    tab.FindObjects(body->Color(), body->Range());
jtimer_x(5);
}


//= Run local behaviors then send arbitrated commands to body (override).

void jhcEliGrok::body_issue ()
{
  const jhcVideoSrc *v;

jtimer(6, "body_issue");
  // record current time
  tnow = jms_now();
  if (body == NULL)
    return;

  // run some reactive behaviors (tk is up-to-date) 
  if (reflex > 0)
    watch.React(this);

  // interpret high-level commands
  assert_watch();
  assert_seek();
  assert_servo();
  if ((approach == 0) && (follow == 0))
    feet = 0;

  // start actuator commands and get new raw images
  if (phy > 0)
{
jtimer(7, "Issue");
    body->Issue();
jtimer_x(7);
}
jtimer(8, "UpdateImgs");
  seen = body->UpdateImgs(); 
  if ((v = body->vid) != NULL)
    if (v->IsClass("jhcListVSrc") > 0)
      seen = 1;                                  // for static images
jtimer_x(8);
jtimer_x(6);
}


///////////////////////////////////////////////////////////////////////////
//                     High-Level People Commands                        //
///////////////////////////////////////////////////////////////////////////

//= Connect some tracked person to motion controller.
// "wiring" persists even without command until overridden (e.g. id = 0)
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliGrok::WatchPerson (int id, int bid)
{
  if ((bid <= wlock) || (id <= 0))
    return 0;
  wlock = bid;
  wwin = id;
  return 1;
}


//= Turn selected person into tracking motion.
// needs to be called before body->Issue() due to target persistence

void jhcEliGrok::assert_watch ()
{
  const jhcMatrix *targ;

  if ((wlock <= 0) || (wwin <= 0))
    return;
  if ((targ = s3.GetID(wwin)) != NULL)
    OrientToward(targ, wlock);
  else
  {
    // most recently selected person has vanished
    wlock = 0;
    wwin = 0;
  }
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
  approach = slock;
  if (slock <= 0)
    return;
  slock = 0;   
 
  // pick a steering angle and travel speed
  if (quick_survey() <= 0)
  {
    nav.Avoid(trav, head, sx, sy);
    base->MoveTarget(trav, ((trav < 0.0) ? 0.7 : ssp), bid);
    base->TurnFix(head, 1.0, 1.0, bid);             
  }
}


//= Try to keep robot at td = off from target at azimuth ta (0 is forward).
// tries to aim toward target at all times, moving backward if too close
// generally speed to follow (1.5) is higher than speed to approach (1.0)
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliGrok::ServoPolar (double td, double ta, double sp, int bid, double off)
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
  follow = vlock;
  if (vlock <= 0)
    return;
  vlock = 0;          

  // pick a steering angle and travel speed (or update map)
  head = 0.0;
  trav = 0.0;
  if (quick_survey() <= 0)
    nav.Swerve(trav, head, vd, va, voff);

  // send selected direction to base 
  base->TurnTarget(head, 1.0, bid);
  base->MoveTarget(trav, 1.0, bid);
}


//= Rebuild map directly in front of robot using 4 discrete gaze fixations.
// returns 1 if forced head move, 0 if no floor scan step in progress

int jhcEliGrok::quick_survey ()
{
  double pan, tilt, p0 = 25.0, t0 = -25, t1 = -65.0;       // depends on Kinect FOV

  // if unsure about foot area or contrained movement then start sequence
  if (feet <= 0)
  {
    if ((nav.Blind() <= 0) && (nav.Tight() <= 0))
      return 0;
    feet = 1;
  }

  // only allow sequence to be reactivated when trigger goes away
  if (feet >= 5) 
  { 
    if ((nav.Blind() <= 0) && (nav.Tight() <= 0))
      feet = 0;
    return 0;
  }

  // force rapid look at a sequence of 4 fixations (no gaps)
  while (feet < 5)
  {
    pan  = (((feet == 1) || (feet == 2)) ? -p0 : p0);
    tilt = (((feet == 1) || (feet == 4)) ?  t0 : t1);
    if (neck->GazeDone(pan, tilt))
      feet++;
    else
    {
      neck->GazeTarget(pan, tilt, -1.5, -1.5, 2000);      
      break;
    }
  }
  return 1;
}


//= Tell the current distance (in) from front of robot to target location.

double jhcEliGrok::FrontDist (double td, double ta) const
{
  double rads = D2R * ta, dx = -td * sin(rads), dy = td * cos(rads) - nav.rfwd;

  return sqrt(dx * dx + dy * dy); 
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Make a pretty version of color image showing relevant items.

void jhcEliGrok::cam_img ()  
{
  int t, gz, sp, nt, col;

  // get current color camera view
  if (!(body->NewFrame()))
    return;
  body->ImgBig(mark);

  // people (green = talking, yellow = gaze, magenta = nodified)
  gz = fn.GazeNew();
  sp = tk.Speaking();
  nt = s3.PersonLim();
  for (t = 0; t < nt; t++)
//    if (s3.PersonState(t) > 0)
    {
      col = ((s3.PersonID(t) == sp) ? 2 : ((t == gz) ? 3 : 5));  
      s3.PersonCam(mark, t, 0, 1, 0, col);
    }

  // objects (green = focal, magenta = nodified)
  tab.AttnCam(mark, 5, 2);                      
}


//= Make pretty version of overhead map and robot sensors.

void jhcEliGrok::nav_img ()
{
  // basic environment with obstacles
  if (!(body->NewFrame()))
    return;
  nav.LocalMap(mark2);

  // directions of motion possible and robot footprint
  nav.Paths(mark2, 1);
  nav.RobotCmd(mark2, base->TurnIncGoal(), base->MoveIncGoal());
  nav.RobotBody(mark2);

  // path recently traveled and target (if any)
  nav.Tail(mark2);
  if ((approach > 0) && (approach > follow))
    nav.Target(mark2, sx, sy);
  else if ((follow > 0) && (follow >= approach))
    nav.Target(mark2, vd, va, 1);
}
