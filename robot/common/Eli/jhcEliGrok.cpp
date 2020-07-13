// jhcEliGrok.cpp : post-processed sensors and innate behaviors for ELI robot
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
  s3.SetMap(192.0, 96.0, 96.0, 0.0, 20.0, 84.0, 0.3, 0.0);   // 16' wide x 8' front
  s3.ch = 34.0;                                              // allow seated
  s3.h0 = 40.0;
  s3.ring = 120.0;                                           // long range okay
  s3.edn = 5.0;

  // change some face finder/gaze defaults
  fn.xsh = 0.4;                                              // big search box
  fn.ysh = 0.4;
  fn.dadj = 2.0;                                             // head is shell

  // information about watching behavior
  src.SetSize(4);
  wtarg[8][0] = '\0';
  strcpy_s(wtarg[7], "<frozen>");
  strcpy_s(wtarg[6], "speaker");
  strcpy_s(wtarg[5], "SOUND");
  strcpy_s(wtarg[4], "closest person");
  strcpy_s(wtarg[3], "eye contact");
  strcpy_s(wtarg[2], "recent face");
  strcpy_s(wtarg[1], "twitch");
  strcpy_s(wtarg[0], "rise");
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


//= Generate a string telling what the robot is paying attention to.

const char *jhcEliGrok::Watching () const
{
  int bid[8] = {rise, twitch, face, stare, close, sound, speak, freeze};
  int i, win;

  // see which bid value won last command arbitration  
  if (phy <= 0)
    return wtarg[8];
  win = neck->GazeWin();
  if (win <= 0)
    return wtarg[8];
  
  // determine behavior by checking against standard bids (if non-zero)
  for (i = 0; i < 8; i++)
    if (win == bid[i])
      return wtarg[i];
  return wtarg[8];
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters controlling what sort of activities to watch.

int jhcEliGrok::watch_params (const char *fname)
{
  jhcParam *ps = &wps;
  int ok;

  ps->SetTag("grok_watch", 0);
  ps->NextSpec4( &freeze, 27, "Freeze bid (neg = disable all)");   
  ps->NextSpec4( &speak,  26, "Current speaker bid");
  ps->NextSpec4( &sound,  25, "Most recent sound bid");
  ps->NextSpec4( &close,  24, "Closest head bid");
  ps->NextSpec4( &stare,  23, "Most recent stare bid");
  ps->NextSpec4( &face,   22, "Most recent face bid");

  ps->NextSpec4( &twitch, 21, "Random gaze bid");
  ps->NextSpec4( &rise,   20, "Head rise bid");   
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for picking targets to watch.

int jhcEliGrok::orient_params (const char *fname)
{
  jhcParam *ps = &ops;
  int ok;

  ps->SetTag("grok_orient", 0);
  ps->NextSpecF( &edge,   25.0, "Sound trigger offset (deg)");  // was 30
  ps->NextSpecF( &hnear,  72.0, "Head distance thresh (in)");
  ps->NextSpec4( &fmin,    3,   "Min face detections");
  ps->NextSpecF( &center,  1.0, "Twitch start stable (deg)");
  ps->NextSpecF( &pdev,   45.0, "Max twitch pan (deg)");
  ps->NextSpecF( &aimed,   2.0, "Gaze done error (degs)");

  ps->NextSpecF( &pdist,  36.0, "Default person dist (in)");
  ps->NextSpecF( &hdec,   10.0, "Head rise decrement (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters controlling behavior timing.

int jhcEliGrok::time_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("grok_time", 0);
  ps->NextSpecF( &bored,  10.0, "Post-cmd freeze (sec)");       
  ps->NextSpecF( &relax,   7.0, "Twitch interval (sec)");  
  ps->NextSpecF( &rdev,    3.0, "Twitch deviation (sec)");
  ps->NextSpecF( &gtime,   0.3, "Gaze response (sec)");  
  ps->NextSpecF( &ttime,   0.7, "Twitch response (sec)");  
  ps->NextSpecF( &rtime,   1.5, "Rise response (sec)");  

  ps->NextSpecF( &side,  -50.0, "Body rotate thresh (deg)");    // 0 = don't
  ps->NextSpecF( &btime,   1.5, "Rotate response (sec)");      
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters controlling where heads are expected to be found.

int jhcEliGrok::vis_params (const char *fname)
{
  jhcParam *ps = &vps;
  int ok;

  ps->SetTag("grok_vis", 0);
  ps->NextSpecF( &lvis, 20.0, "Max left head offset (deg)");
  ps->NextSpecF( &rvis, 20.0, "Max right head offset (deg)");
  ps->NextSpecF( &tvis, 10.0, "Max top head offset (deg)");
  ps->NextSpecF( &bvis, 10.0, "Max bottom head offset (deg)");
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

  ok &= watch_params(fname);
  ok &= orient_params(fname);
  ok &= time_params(fname);
  ok &= vis_params(fname);
  ok &= fn.Defaults(fname);      // does s3 also
  ok &= nav.Defaults(fname);
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

  ok &= wps.SaveVals(fname);
  ok &= ops.SaveVals(fname);
  ok &= tps.SaveVals(fname);
  ok &= vps.SaveVals(fname);
  ok &= fn.SaveVals(fname);      // does s3 also
  ok &= nav.SaveVals(fname);
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
// NOTE: body should be reset outside of this

void jhcEliGrok::Reset (int rob)
{
  jhcVideoSrc *v;
  const jhcImg *src;

  // reset vision components
  s3.Reset();
  fn.Reset();
  nav.Reset();

  // configure body 
  phy = 0;
  seen = 0;
  if ((rob > 0) && (body != NULL))
  {
    phy = 1; 
    base->Zero();
    body->InitPose(-1.0);
    body->Update(-1, 1);       // sensor info will be waiting and need to be read
  }

  // configure vision elements
  if (body != NULL) 
    if ((v = body->vid) != NULL)
    {
      // setup navigation
      nav.SrcSize(v->XDim(), v->YDim(), v->Focal(1), v->Scaling(1));

      // make status images
      body->BigSize(mark);
      mark.FillArr(0);
      mark2.InitSize(nav.map);

      // configure visual analysis for camera images
      src = body->View();
      s3.SetSize(*src);
    }

  // high-level commands
  wlock = 0;
  wwin = 0;
  slock = 0;
  vlock = 0;

  // clear state for sound and twitch behaviors
  seek = 0;
  slew = 0;  
  idle = 0;

  // restart background loop, which first generates a body Issue call
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
  head_img();
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

//= Get sensor inputs and fully process images.

void jhcEliGrok::body_update ()
{
  jhcMatrix pos(4), dir(4);

  // wait (if needed) for body sensor data to be received (no mic)
  if (phy > 0)
{
jtimer(2, "body update");
    body->Update(-1, 0);
jtimer_x(2);
}

  // do slow head finding and tracking (needs both pose and image)
  if (seen > 0)
  {  
    // find new person location based on current camera pose
    adjust_heads();
jtimer(3, "face track");
    neck->HeadPose(pos, dir, lift->Height());
    fn.SetCam(pos, dir);
    fn.Analyze(body->Color(), body->Range());
jtimer_x(3);

jtimer(4, "navigation");
    // update navigation map
    nav.AdjustMaps(base->StepFwd(), base->StepLeft(), base->StepTurn());
    nav.RefineMaps(body->Range(), pos, dir);
jtimer_x(4);
  }
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
    // check for valid person who was not sensed on last cycle
    p = &(s3.dude[i]);
    if ((p->TrackID() <= 0) || (p->Last() <= 1))
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


//= Run local behaviors then send arbitrated commands to body.

void jhcEliGrok::body_issue ()
{
jtimer(12, "body_issue");
  // record current time
  tnow = jms_now();
  if (body == NULL)
    return;

  // run some reactive behaviors (tk is up-to-date) 
  if (freeze >= 0)
  {
    cmd_freeze();
    watch_talker();
    gaze_sound();
    watch_closest();
    gaze_stare();
    gaze_face();
    gaze_random();
    head_rise();
  }

  // interpret high-level commands
  assert_watch();
  assert_seek();
  assert_servo();

  // start actuator commands and get new raw images
  if (phy > 0)
{
jtimer(13, "Issue");
    body->Issue();
jtimer_x(13);
}
jtimer(11, "UpdateImgs");
  seen = body->UpdateImgs();    
jtimer_x(11);

jtimer_x(12);
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
    orient_toward(targ, wlock);
  else
  {
    // most recently selected person has vanished
    wlock = 0;
    wwin = 0;
  }
}


//= Aim camera at target location, rotating body if needed.
// set "turn" to zero or negative to prevent body rotation

void jhcEliGrok::orient_toward (const jhcMatrix *targ, int bid)
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
double t0 = -60.0;
  double trav, head;
  int bid = slock;

  // check if some command, then reset arbitration for next round
  if (slock <= 0)
    return;
  slock = 0;
 
  // make sure map in front of robot exists
  if (nav.Blind())
  {  
    neck->GazeTarget(0.0, t0, 1.0, 0.0, bid + 1);
    base->DriveTarget(0.0, 0.0, 1.0, bid);
    return;
  }

  // pick a steering angle and travel speed
  nav.Avoid(trav, head, sx, sy);
  base->MoveTarget(trav, ((trav < 0.0) ? 0.7 : ssp), bid);
  base->TurnFix(head, 1.0, 1.0, bid);             
}


//= Try to keep robot at td = 0 from target at azimuth ta (0 is forward).
// tries to aim toward target at all times, moving backward if too close
// generally speed to follow (1.5) is higher than speed to approach (1.0)
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliGrok::ServoPolar (double td, double ta, double sp, int bid)
{
  if (bid <= vlock)
    return 0;
  vlock = bid;
  vd = td;
  va = ta;
  vsp = sp;
  return 1;
}


//= Take necessary (pre-emptive) body actions to maintain distance from target.

void jhcEliGrok::assert_servo ()
{
double t0 = -60.0, ttime = 1.0, atime = 2.0, orient = 60.0;
  double trav, head;
  int bid = vlock;

  // check if some command, then reset arbitration for next round
  if (vlock <= 0)
    return;
  vlock = 0;
 
  // make sure map in front of robot exists
  if (nav.Blind())
  {  
    neck->GazeTarget(0.0, t0, 1.0, 0.0, bid + 1);
    base->DriveTarget(0.0, 0.0, 1.0, bid);
    return;
  }

  // pick a steering angle and travel speed
  nav.Swerve(trav, head, vd, va);
  base->TurnFix(head, ttime, 1.0, bid);
  if (fabs(head) < orient)
    base->MoveFix(trav, atime, vsp, bid);   
}


///////////////////////////////////////////////////////////////////////////
//                            Innate Behaviors                           //
///////////////////////////////////////////////////////////////////////////

//= Freeze head and feet if recent conscious command issued.

void jhcEliGrok::cmd_freeze ()
{
  if (freeze <= 0)
    return;
  if (body->NeckIdle(tnow) <= bored)                 
    neck->ShiftTarget(0.0, 0.0, 0.0, freeze);        // lock in place
  if (body->BaseIdle(tnow) <= bored)                 
    base->DriveTarget(0.0, 0.0, 1.0, freeze);        // active limp
}


//= Continuously look at whomever is currently talking (if anyone).

void jhcEliGrok::watch_talker ()
{
  if (speak <= 0)
    return;
  WatchPerson(tk.Speaking(), speak);
}


//= Look at non-central sound source (if any) for a while.
// state machine controlled by "seek" and timer "swait"

void jhcEliGrok::gaze_sound ()
{
  double ang, rads;
  int old = 1;

  // see if behavior desired 
  if (sound <= 0)
    return;

  // trigger when sound is far to either side wrt gaze direction
  if (mic->VoiceStale() <= 0)
  {
    ang = mic->VoiceDir();
    if (fabs(ang - neck->Pan()) >= edge)
    {
      // remember location since sound is often short duration
      rads = D2R * (ang + 90.0);
      src.SetVec3(pdist * cos(rads), pdist * sin(rads), s3.h0);
      old = 0;
      seek = 1;
    }
  }
  if (seek <= 0)
    return;

  // adjust for any base motion then aim at remembered location
  if (old > 0)
    base->AdjustTarget(src);   
  if (neck->GazeErr(src, lift->Height()) > aimed)
    orient_toward(&src, sound);
  else
    seek = 0;
}


//= Track the closest head (even if face not visible).

void jhcEliGrok::watch_closest ()
{
  const jhcMatrix *hd;
  int sel;

  // see if behvaior desired then find closest head
  if (close <= 0)
    return;
  if ((sel = s3.Closest()) < 0)
    return;

  // follow if planar distance close enough
  hd = s3.GetPerson(sel);
  if (hd->PlaneVec3() <= hnear)
    WatchPerson(s3.PersonID(sel), close);
}


//= Look at most recent person staring at robot (if any).

void jhcEliGrok::gaze_stare ()
{
  if (stare <= 0)
    return;
  WatchPerson(fn.GazeNewID(), stare);
}


//= Look a while at most recently found face (if any),

void jhcEliGrok::gaze_face ()
{
  if (face <= 0)
    return;
  WatchPerson(fn.FrontNewID(0, fmin), face);
}


//= Flick gaze toward a random position after a while.
// state machine controlled by "delay" and timers "idle" and "rwait"
// <pre>
//   rwait    idle      state
//     0        0       get new inter-twitch
//     0        x       wait until twitch
//     x        0       twitch for a while
//     x        x       --never occurs--
// </pre>

void jhcEliGrok::gaze_random ()
{
  jhcMatrix hd(4);

  // see if behavior desired
  if (twitch <= 0)
    return;

  // if not currently twitching
  if (slew == 0)
  {
    // possibly pick a new time for next twitch (once)           
    if (idle == 0) 
    {
      delay = ROUND(1000.0 * jrand_cent(relax, rdev));
      delay = __max(1, delay);
      neck->Gaze(prand, trand);
      idle = tnow;                         // advance to next state
      return;
    }

    // reset inter-twitch timer if head moved too much
    if (jms_diff(tnow, idle) < delay)
    {
      if (neck->GazeErr(prand, trand) > center)
        idle = tnow;
      neck->Gaze(prand, trand);
      return;
    }

    // get tilt to likely forward head and pick a random pan
    hd.SetVec3(0.0, pdist, s3.h0);
    neck->AimFor(prand, trand, hd, lift->Height());
    prand = jrand_cent(0.0, pdev);
    slew = 1;                              // advance to next state
    idle = 0;
  }

  // if time not expired then pursue target, otherwise reset state machine 
  if (neck->GazeErr(prand, trand) > aimed)
    neck->GazeFix(prand, trand, ttime, twitch);
  else
    slew = 0;                              // advance to next state
}


//= Slowly raise gaze to highest reasonable person head.

void jhcEliGrok::head_rise ()
{
  jhcMatrix hd(4);
  double pan, tilt;

  if (rise <= 0)
    return;
  hd.SetVec3(0.0, pdist, s3.h1 - hdec);
  neck->AimFor(pan, tilt, hd, lift->Height());
  if (neck->TiltErr(tilt) > aimed)                 // should add hysteresis
    neck->TiltFix(tilt, rtime, rise);
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Make a pretty version of color image showing relevant items.

void jhcEliGrok::head_img ()  
{
  if (!(body->NewFrame()))
    return;
  body->ImgBig(mark);
  s3.HeadsCam(mark);                               // magenta - heads
  s3.ShowIDCam(mark, tk.Speaking(), 0, 1, 0, 2);   // green   - speaking
  fn.FacesCam(mark);                               // cyan    - faces
  fn.GazeCam(mark, fn.GazeNew());                  // yellow  - gaze
}


//= Make pretty version of overhead map and robot sensors.

void jhcEliGrok::nav_img ()
{
  if (!(body->NewFrame()))
    return;
  nav.LocalMap(mark2);
  nav.Paths(mark2);
  nav.RobotBody(mark2);
  nav.Tail(mark2);
}
