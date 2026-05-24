// jhcVisGrok.cpp : post-processed sensors and high-level behaviors 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2026 Etaoin Systems
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
#include "Interface/jprintf.h"
#include "Interface/jtimer.h"          // for profiling

#include "RWI/jhcVisGrok.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcVisGrok::~jhcVisGrok ()
{
}


//= Default constructor initializes certain values.
// creates member instances here so header file has fewer dependencies

jhcVisGrok::jhcVisGrok ()
{
  // default debugging image
  wmap = 640;
  hmap = 480;
  strcpy_s(tmap, "Debug");

  // no input images (yet)
  raw = NULL;
  rng = NULL;
  col = NULL;
  aux = NULL;

  // Kinect optical parameters
  rflen = 525.0;
  rsc = 0.9659; 

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
  s3.dbg = 0;

  // change some face finder/gaze defaults
  fn.xsh = 0.4;                                                  // big search box
  fn.ysh = 0.4;
  fn.dadj = 2.0;                                                 // head is shell

  // configure object finding map
  sobj.SetMap(108.0, 63.0, 24.0, -6.0, -2.0, 18.0, 0.15, 28.5);  // 720 x 421 map
  sobj.hmix = 0.0;

  // set up display strings
  nmode[0][0] = '\0';
  strcpy_s(nmode[1], "--  APPROACH");
  strcpy_s(nmode[2], "--  Follow");
  strcpy_s(nmode[3], "--  wander ...");

  // processing parameters
  LoadCfg();
  Defaults();
  space2 = NULL;
  probe = 0;
}


//= Null pointers to body and subcomponents.

void jhcVisGrok::clr_ptrs ()
{
  neck = NULL;
  arm  = NULL;
  lift = NULL;
  base = NULL;
  phy = 0;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Angular corrections for sensor mounting errors.
// Note: these are body-specific calibration values

int jhcVisGrok::err_params (const char *fname)
{
  jhcParam *ps = &eps;
  int ok;

  ps->SetTag("grok_cal", 0);
  ps->NextSpecF( &rp0, 0.0, "Range pan err (deg)");
  ps->NextSpecF( &rt0, 0.0, "Range tilt err (deg)");
  ps->NextSpecF( &rr0, 0.0, "Range roll err (deg)");
  ps->NextSpecF( &cp0, 0.0, "Color pan err (deg)");
  ps->NextSpecF( &ct0, 0.0, "Color tilt err (deg)");
  ps->NextSpecF( &cr0, 0.0, "Color roll err (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters describing camera optics and images.

int jhcVisGrok::rng_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("grok_rng", 0);
  ps->NextSpecF( &rflen, 525.0,    "Range-finder focal length");
  ps->NextSpecF( &rsc,     0.9659, "Range-finder depth scaling");
  ps->Skip();
  ps->NextSpecF( &ign,     0.15,   "Min stable for objects (sec)");
  ps->NextSpecF( &west,    0.0,    "Nearness for held width (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters describing camera optics and images.

int jhcVisGrok::cam_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("grok_cam", 0);
  ps->NextSpecF( &cmx,   319.5, "Raw X center (pels)");
  ps->NextSpecF( &cmy,   239.5, "Raw Y center (pels)");
  ps->NextSpecF( &cf0,   525.0, "Raw focal length (pels)");
  ps->NextSpecF( &cr2,     0.0, "R^2 warping coef");
  ps->NextSpecF( &cr4,     0.0, "R^4 warping coef");
  ps->NextSpecF( &cmag,    1.0, "Final magnification");

  ps->NextSpec4( &cenh,    2,   "Enhance (none, cont, hue)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters describing padding for arm mask in image.

int jhcVisGrok::pad_params (const char *fname)
{
  jhcParam *ps = &pps;
  int ok;

  ps->SetTag("grok_pad", 0);
  ps->NextSpecF( &lpad, 1.5, "Block near lift joint (in)");
  ps->NextSpecF( &hpad, 2.0, "Block near hand base (in)");
  ps->NextSpecF( &fpad, 3.0, "Block near fingertips (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters controlling where heads are expected to be found.

int jhcVisGrok::vis_params (const char *fname)
{
  jhcParam *ps = &vps;
  int ok;

  ps->SetTag("grok_vis", 0);
  ps->NextSpecF( &lvis,   20.0, "Max left head offset (deg)");
  ps->NextSpecF( &rvis,   20.0, "Max right head offset (deg)");
  ps->NextSpecF( &tvis,   10.0, "Max top head offset (deg)");
  ps->NextSpecF( &bvis,   10.0, "Max bottom head offset (deg)");
  ps->NextSpecF( &lost,  120.0, "Max head memory dist (in)");      // 10 ft
  ps->Skip();

  ps->NextSpecF( &side,   50.0, "Body rotate thresh (deg)");       // 0 = don't 
  ps->NextSpecF( &btime,   1.5, "Rotate response (sec)");      
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters controlling navigation related gaze activities.

int jhcVisGrok::sacc_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("grok_sacc", 0);
  ps->NextSpecF( &hem,      6.0, "Forward motion blocked (in)");
  ps->NextSpecF( &umat,     0.5, "Fraction unknown doormat");
  ps->NextSpecF( &sacp,    25.0, "Saccade nearby pan (deg)");
  ps->NextSpecF( &sacp2,   25.0, "Saccade wide pan (deg)");
  ps->NextSpecF( &sact,   -25.0, "Saccade nearby tilt (deg)");
  ps->NextSpecF( &sact2,  -65.0, "Saccade floor tilt (deg)"); 

  ps->NextSpecF( &road,   -40.0, "Path check tilt (deg)");      
  ps->NextSpecF( &cruise,   2.0, "Path check interval (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters controlling subconcious orienting and flinch behaviors.

int jhcVisGrok::auto_params (const char *fname)
{
  jhcParam *ps = &aps;
  int ok;

  ps->SetTag("grok_auto", 0);
  ps->NextSpec4( &pbid, 60,   "Person tracking bid");
  ps->NextSpec4( &obid, 55,   "Object tracking bid");
  ps->NextSpec4( &mbid, 50,   "Motion seeking bid");
  ps->NextSpec4( &sbid, 45,   "Sound seeking bid");        // 0 = disabled
  ps->NextSpec4( &rbid, 40,   "Center restore bid");
  ps->Skip();

  ps->NextSpecF( &tip,  10.0, "Off-balance tip (deg)");    // 0 = disabled
  ps->NextSpecF( &back,  2.0, "Balance restore (sec)");    
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read just body-specific values from a file.

int jhcVisGrok::LoadCfg (const char *fname)
{
  return err_params(fname);
}


//= Read all relevant defaults variable values from a file.

int jhcVisGrok::Defaults (const char *fname)
{
  int ok = 1;

  ok &= rng_params(fname);
  ok &= cam_params(fname);
  ok &= pad_params(fname);
  ok &= vis_params(fname);
  ok &= sacc_params(fname);
  ok &= auto_params(fname);
  ok &= fn.Defaults(fname);      // does s3 also
  ok &= nav.Defaults(fname);
  ok &= sobj.Defaults(fname);
  ok &= tab.Defaults(fname);
  ok &= mot.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcVisGrok::SaveVals (const char *fname) 
{
  int ok = 1;

  ok &= rps.SaveVals(fname);
  ok &= cps.SaveVals(fname);
  ok &= pps.SaveVals(fname);
  ok &= vps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= aps.SaveVals(fname);
  ok &= fn.SaveVals(fname);      // does s3 also
  ok &= nav.SaveVals(fname);
  ok &= sobj.SaveVals(fname);
  ok &= tab.SaveVals(fname);
  ok &= mot.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Restart background processing loop.
// if rob > 0 then runs with body active (else motion disabled)
// if behaviors > 0 then runs with autonomic behaviors (else only commands)
// NOTE: body should be reset outside of this !!!

void jhcVisGrok::Reset (int rob, int behaviors, int choke)
{
  double cflen;

  // disable background threads 
  jhcBgndGrok::Stop();

  // configure body 
  phy = rob;
  sprc = 0;

  // setup navigation, object, and person finding
  nav.SrcSize(640, 480, rflen, rsc);
  nav.SetCamFix(0, rp0, rt0, rr0);
  sobj.SrcSize(640, 480, rflen, rsc);
  sobj.SetCamFix(0, rp0, rt0, rr0);
  s3.SetSize(640, 480, 0, rflen, rsc);
  s3.SetCamFix(0, rp0, rt0, rr0);

  // configure color camera undistortion
  flat.SetSize(640, 480, 3);
  cw.InitSize(640, 480, 3);
  cflen = cw.Flatten5(cmx, cmy, cf0, cr2, cr4, cmag);

  // set color camera warping and focal length 
  sobj.AltFlen(0, cflen, 1.0);
  sobj.SetAltFix(0, cp0, ct0, cr0);
  fn.AltFlen(0, cflen, 1.0);
  fn.SetAltFix(0, cp0, ct0, cr0);  
  mot.SetSize(640, 480, cflen);

  // reset vision components
  s3.Reset();
  fn.Reset(0);
  nav.Reset();
  sobj.Reset();
  tab.SetSize(s3.map);
  tab.Reset();

  // make default status images
  mark.SetSize(640, 480, 3);
  mark.FillArr(0);
  mark2.InitSize(nav.map);
  nav_img();

  // what to do about short depths
  s3.choke   = choke;
  nav.choke  = choke;
  sobj.choke = choke;

  // sensor height adjustment
  zadj = 0.0;

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
  ahead = jms_now();

  // default finger separation
  sep = 0.0;

  // balance restoration state
  esc0 = 0;

  // restart background loop, which first generates a body Issue call
  reflex = behaviors;
  first = 1;
  tcyc = 0.033;
  jhcBgndGrok::Reset();
}


//= Read and process all sensory information from robot.
// this all happens when background thread in rwi update is quiescent
// returns 1 if okay, 0 or negative for error

int jhcVisGrok::Update (int voice, UL32 resume)
{
  jhcImg *src = &flat;

  // make various corrections to input color image
  // since *raw changes slowly, *col will too (even if mix of several frames)
  if (body->RngReady() >= 2)
  {
    // undo lens distortion
    if ((cr2 != 0.0) || (cr4 != 0.0) || (cmag != 1.0))
      cw.Warp(flat, *raw);      
    else           
      src = raw;

    // improve contrast or remove hue cast
    if (cenh == 1)
      Enhance(*col, *src, 2.0);         
    else if (cenh >= 2)
      Enhance3(*col, *src, 2.0);        
    else
      col->CopyArr(*src);              // no enhancement
  }

  // do slow vision processing in background (already started usually)
  // does: body_update + [umwelt | umwelt2 | umwelt3 | umwelt4] + body_issue
  if (jhcBgndGrok::Update(0) <= 0)
    return 0;

  // do fast sound processing in foreground (needs voice)
  sprc = voice;
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

void jhcVisGrok::Stop ()
{
  jhcBgndGrok::Stop();
  if ((phy > 0) && (body != NULL))
    body->Limp();
}


///////////////////////////////////////////////////////////////////////////
//                       Core Processing Overrides                       //
///////////////////////////////////////////////////////////////////////////

//= Acquires and pre-processes new sensor inputs from robot body (override).
// waits (if needed) for data to be received (no mic)

void jhcVisGrok::body_update ()
{
jtimer(3, "body_update");
  // get actuator positions, battery, etc. (possibly time-intensive)
  if (phy > 0) 
    body->Update(-1, 0);                  
  zadj = nav.HtDev();        // camera correction based on floor

  // possibly override gripper width based on visual indicators
  if (west > 0.0)
    arm->JawOpen(finger_sep());        

  // do not erase any objects if arm is extended (can occlude)
  if (arm->TuckErr() > 5.0)
    sobj.RetainAll();

  // use old person map to guess table height for this cycle (many threads need)
  sobj.ztab = tab.PickPlane(s3.map, s3.IPP(), s3.HMIN(), s3.HMAX());
jtimer_x(3);
}


//= Process images for SURFACES in primary background thread (override).

void jhcVisGrok::umwelt ()
{
  jhcMatrix pos(4), dir(4);

  // needs depth data
  if (body->RngReady() <= 0)
    return;
  body->RngPose(pos, dir, zadj);          

  // find support SURFACE as a target in old map (s3 in other thread)
jtimer(4, "find table (bg1)");
  if (first > 0)
    first--;
  else
    tab.FindSurf(pos, lift->Height());   // needs wmap set by PickPlane
jtimer_x(4);

jtimer(5, "visual motion (bg1)");
  // look for waving hands
  mot.Analyze(*col, dir.P() - 90.0, dir.T(), body->RngStatic());
jtimer_x(5);
}


//= Process images for OBJECT finding in secondary background thread (overrride).

void jhcVisGrok::umwelt2 ()
{
  jhcMatrix cp(4), cd(4), vp(4), vd(4);

  // needs depth data
  if (body->RngReady() <= 0)
    return;

jtimer(6, "find objects (bg2)");
  // color camera choices for Spectralize + debugging
  body->RngPose(cp, cd, zadj);
  body->ColPose(vp, vd, zadj);
  sobj.SetFront(0, vp, vd, cp, cd);

  // detect bump-like objects (ztab already set by body_update)
  sobj.AdjBase(base->StepSide(), base->StepFwd(), base->StepTurn()); 
  sobj.AdjNeck(cp, cd);
  if ((ign <= 0.0) || (body->RngStatic() >= ign))  // was !neck->Saccade()
    sobj.FindObjects(*col, *rng);
jtimer_x(6);
}


//= Process images for PEOPLE in tertiary background thread (override).

void jhcVisGrok::umwelt3 ()
{
  jhcMatrix cp(4), cd(4), vp(4), vd(4);

  // needs depth data
  if (body->RngReady() <= 0)
    return;

jtimer(7, "find people (bg3)");
  // color camera choices for face analysis + debugging
  body->RngPose(cp, cd, zadj);
  body->ColPose(vp, vd, zadj);
  fn.SetFront(0, vp, vd, cp, cd);

  // find new person locations based on current camera pose
  adjust_heads();
  fn.SetCam(cp, cd);
  if ((ign <= 0) || (body->RngStatic() >= ign))  // was !neck->Saccade()
    fn.Analyze(*col, *rng);
jtimer_x(7);
}


//= Process images for NAVIGATION in quaternary background thread (override).

void jhcVisGrok::umwelt4 ()
{
  jhcMatrix pos(4), dir(4);

  // needs depth data
  if (body->RngReady() <= 0)
    return;

jtimer(8, "navigation (bg4)");
  // update NAVIGATION map
  body->RngPose(pos, dir);             // no height adjustment
  nav.AdjustMaps(base->StepFwd(), base->StepLeft(), base->StepTurn());
  if (body->RngStatic() > 0.015)       // roughly 3 frames
    nav.RefineMaps(*rng, pos, dir);
  else
    nav.RefreshBody();              
  nav.ComputePaths();                  // might be affected by decay
jtimer_x(8);
}


//= Run local behaviors then send arbitrated commands to body (override).

void jhcVisGrok::body_issue ()
{
  double dt, mix = 0.1;
  UL32 tprev = tnow;

jtimer(9, "body_issue");
  // record current time and estimate cycle length
  tnow = jms_now();
  if ((dt = jms_secs(tnow, tprev)) < 0.5)
    tcyc += mix * (dt - tcyc);

  // umwelt high-level commands (in order of priority)
  act = base_mode();
  assert_scan();
  assert_watch();
  assert_seek();
  assert_servo();
  assert_explore();

  // subconcious behaviors
  assert_gaze();
  assert_tip();

  // start actuator cmds and get new images (possibly time-intensive)
  if (body != NULL)
  {
    if (phy > 0) 
{
jtimer(10, "Issue");
      body->Issue();                   // for jhcEliBody
jtimer_x(10);
}
jtimer(11, "UpdateImgs");
    body->UpdateImgs(); 
jtimer_x(11);
  }
jtimer_x(9);
}


///////////////////////////////////////////////////////////////////////////
//                           Override Helpers                            //
///////////////////////////////////////////////////////////////////////////

//= Estimate finger separation from very close range readings.

double jhcVisGrok::finger_sep ()
{
  double gap, dist = west, mix = 0.3;
  int w = rng->XDim(), mid = w >> 1, th = ROUND(west * 101.6);
  int x, span = -1, sum = 0, cnt = 0;
  US16 *s = (US16 *) rng->PxlSrc();

  // find longest ultra-close stretch on bottom line = gripped object
  for (x = 0; x < w; x++, s++)
    if ((x > mid) && (span <= 0))
      return -1.0;   
    else if ((*s >= th) && (*s != 0xFFFF))
    {
      // definite non-run pixel found
      if ((span > 0) && (x > mid))
        break;                                   
      span = 0;      
      sum = 0;
      cnt = 0;
    }
    else if (span >= 0)                         
    {
      // pixel probably in run (ultra-close can drop out)
      span++;        
      if (*s != 0xFFFF)
      {
        sum += *s;
        cnt++;
      }
    }

  // estimate object width using ranger focal length
  if (cnt > 0)
    dist = sum / (101.6 * cnt);
  gap = dist * span / rflen;
  sep += mix * (gap - sep);            // smooth over time
  return sep;
}


//= Alter expected position and visibilty of heads based on robot state.
// NOTE: odometry only provides coarse adjustment, true tracking is more accurate

void jhcVisGrok::adjust_heads ()
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
    if (p->PlaneVec3() > lost)
    {
      p->vis = 1;                      // allow erasure of far heads
      return;
    }

    // determine if head should have been matchable (allows erasure)
    neck->AimFor(p0, t0, *p, ht);
    perr = neck->PanErr(p0, 0); 
    terr = neck->TiltErr(t0, 0); 
    if ((perr >= -lvis) && (perr <= rvis) && (terr >= -bvis) && (terr <= tvis))
      p->vis = 1;
    else 
      p->vis = 0;
  }
}


//= Set descriptive string telling what high-level command the robot is performing.
// picks dominant mode (in priority order)

int jhcVisGrok::base_mode ()
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

int jhcVisGrok::ClosestFace (double front, int cnt) const
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
// assumes dev = 0 means forward wrt microphone position (approx. robot center)
// if successful binds position vector to center (else unchanged)
// returns index of winner, negative if nothing suitable

int jhcVisGrok::HeadAlong (jhcMatrix& head, double aim, double dev) const
{
  jhcMatrix pos(4);
  double off, best;
  int i, n = s3.PersonLim(), win = -1;

  if (mic == NULL)
    return -1;
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
// blim is maximum body offset angle to tolerate (0 = any)
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcVisGrok::WatchPerson (int id, double blim, int bid)
{
  if (bid < wlock) 
    return 0;
  wlock = bid;
  wwin = id;
  wlim = blim;
  return 1;
}


//= Turn selected person into tracking motion.
// needs to be called before body->Issue() due to target persistence
// keeps trying to watch person regardless of visibility, angle, or distance

void jhcVisGrok::assert_watch ()
{
  const jhcMatrix *targ;
  double ang, horizon = 120.0, crane = 120.0;
  int bid = __max(10, wlock);
 
  if (wwin <= 0)                       // not wlock, for persistence
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
        OrientToward(targ, wlim, bid);
        wlock = 0;                               // allow override
        wlim = 0.0;
        return;                                  // preserve default wwin 
      }
    }

  // give up on watching that particular person
  wwin = 0;
  wlock = 0;
}


//= Aim camera at target location, rotating body if needed.
// generally only rotates body slightly unless blim > 0
// set "turn" to zero or negative to prevent body rotation

void jhcVisGrok::OrientToward (const jhcMatrix *targ, double blim, int bid)
{
  if (targ == NULL) 
    return;
  neck->GazeAt(*targ, lift->Height(), 0.5, bid);
/*
  if (((side > 0.0) && (fabs(pan) > side)) ||
      ((blim > 0.0) && (fabs(targ->PanVec3() - 90.0) > blim)))
    base->TurnFix(pan, btime, 1.5, bid);         // swivel base
*/
}


//= Gives the max absolute pan or tilt error between current gaze and some person.
// useful for telling if move is progressing or has finished
// returns negative if person is no longer visible

double jhcVisGrok::PersonErr (int id) const
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

int jhcVisGrok::SeekLoc (double tx, double ty, double sp, int bid)
{
  if (bid < slock)
    return 0;
  slock = bid;
  sx = tx;
  sy = ty;
  ssp = sp;
  return 1;
}


//= Take necessary (pre-emptive) body actions to approach winning target.

void jhcVisGrok::assert_seek ()
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

int jhcVisGrok::ServoPolar (double td, double ta, double off, double sp, int bid)
{
  if (bid < vlock)
    return 0;
  vlock = bid;           
  vd = td;
  va = ta;
  vsp = sp;
  voff = off;
  return 1;
}


//= Take necessary (pre-emptive) body actions to maintain distance from target.

void jhcVisGrok::assert_servo ()
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

double jhcVisGrok::FrontDist (double td, double ta) const
{
  double rads = D2R * ta, dx = -td * sin(rads), dy = td * cos(rads) - nav.Nose();

  return sqrt(dx * dx + dy * dy); 
}


//= Wander aimlessly without hitting things.

int jhcVisGrok::Explore (double sp, int bid)
{
  if (bid < xlock)
    return 0;
  xlock = bid;
  xsp = sp;
  return 1;
}


//= Drive along frontmost path that is sufficiently long. 
// assumes assert_scan called early to referesh map if needed

void jhcVisGrok::assert_explore ()
{
  double trav, head, tsp;             
  int bid = xlock;

  // check if some command, then reset arbitration for next round
  if (xlock <= 0)
    return;
  xlock = 0;          

  // pick a steering angle and travel speed (gaze ahead and down)
  nav.Wander(trav, head);
  tsp = ((fabs(head) < 2.0) ? 0.0 : xsp);        // speed 0 for no oscillation
  base->TurnTarget(head, tsp, bid);
  base->MoveTarget(trav, xsp, bid);
}


//= Request mapping in front of robot, sometimes at feet if needed.
// predicate Survey() will be true when head is being moved

int jhcVisGrok::MapPath (int bid)
{
  if (bid < flock)
    return 0;
  flock = bid + 1;                     // +1 hack
  return 1;
}


//= Set robot gaze appropriately to build portion of map needed.
// generally most important motion so called first in sequence of "asserts"

void jhcVisGrok::assert_scan ()
{
  double odet2 = 2.0 * (ign + tcyc * sobj.Born()), gtol = 1.0;  // was 3
  int bid = flock;

  // check if some command, then reset arbitration for next round
  if (flock <= 0)
  {  
    feet = 0;                                    // cancel any saccade
    return;
  }
  flock = 0;         

  // look at feet whenever needed, otherwise check path occasionally
  if (quick_survey(bid) > 0)
    return;
  if (jms_elapsed(ahead) < cruise)               // not time to check yet
    return;

  // force robot to look straight ahead
  if ((neck->GazeErr(0.0, road) > gtol) || 
      (body->RngStatic() <= odet2))              // allow obj detect
  {
    neck->GazeTarget(0.0, road, 1.0, bid);       // look down at path (sp was 0.5)    
    base->TurnTarget(0.0, 0.0, bid);             // in case circling 
    return;
  }
  ahead = jms_now();                             // reset cycle timer
}


//= Use a series of 4 rapid gaze fixations to map floor ahead of robot.
//   feet: 0 = check if foot saccade needed
//         1 = await mid-right saccade
//         2 = await low-right saccade
//         3 = await low-left saccade
//         4 = await mid-left saccade
//         5 = assess whether still blocked
//         6 = turn left 90 degrees
// returns 1 if moving robot, 0 if no motion command
// Note: takes about 4 secs on Ganbei robot

int jhcVisGrok::quick_survey (int bid)
{
  double p[4] = {-sacp, -sacp2, sacp2, sacp};
  double t[4] = { sact,  sact2, sact2, sact}; 
  double pan, tilt, gtol = 1.0, ttol = 5.0;        // gtol was 3
  double odet2 = 2.0 * (ign + tcyc * sobj.Born());            

  // check if constrained movement or unknown doormat area
  if (feet <= 0)
  {
    if (!nav.Tight(hem) && !nav.Blind(umat))
      return 0;
    feet = 1;                                      // start scan
  }

  // force rapid sequence of 4 fixations on nearby floor
  while (feet <= 4)
  {
    pan  = p[feet - 1];
    tilt = t[feet - 1];
    if ((neck->GazeErr(pan, tilt) > gtol) || 
        (body->RngStatic() <= odet2))              // allow obj detect
    {
      base->Park(bid);                             // no base motion
      neck->GazeTarget(pan, tilt, 1.0, bid);       // was sp = -1.5 then 0.5
      return 1;
    }
    feet++;                                        // next scan position
  }

  // check if still blocked right after scan
  if (feet <= 5)
  {
    if (!nav.Tight(hem))                           // path found
    {
      feet = 0;
      return 0;                                    
    }
    view = base->TurnGoal(90.0);                   
    feet++;                                        // start turn
  }    

  // reorient base to find better path
  if (base->TurnErr(view) > ttol) 
  {
    base->TurnAbsolute(view, 1.0, bid);            // can oscillate!
    return 1;
  }
  if (body->RngStatic() <= odet2)
  {
    base->Park(bid);
    return 1;
  }

  // check forward path at this new orientation
  feet = 0;          
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                       Subconscious Behaviors                          //
///////////////////////////////////////////////////////////////////////////

//= Set default gaze (bid 0 = disabled, sp 0 = parked).
// uses GazeSoft() since tracking with no final deadband

void jhcVisGrok::assert_gaze ()
{
  jhcMatrix xyz(4);
  double p, t, ht = lift->Height(), gsp = 0.5, soft = 15.0;
  int iw = col->XDim(), ih = col->YDim();

  if ((mbid > 0) && mot.Interest(p, t))                    // twitchy fingers
    neck->GazeSoft(p, t, gsp, mbid, soft);
  if ((pbid > 0) && fn.Interest(xyz, iw, ih))              // track face
    neck->GazeSoft(xyz, ht, gsp, pbid, soft);
  if ((obid > 0) && sobj.Interest(xyz, iw, ih))            // track object
    neck->GazeSoft(xyz, ht, gsp, obid, soft);
  if ((sbid > 0) && (mic != NULL) && mic->Interest(p, t))  // look for talking
    neck->GazeSoft(p, t, gsp, sbid, soft);
  if ((rbid > 0) && mot.Askew(p, t))                       // back to center
    neck->GazeSoft(p, t, gsp, rbid, soft);
}


//= Back and turn if front wheels bump up or dip down.

void jhcVisGrok::assert_tip ()
{
  double prads, cp, sp, rrads, cr, sr, dev, azm;
  int bid = 2000; 

  // if behavior enabled 
  if (tip <= 0.0)
    return;

  // get current body attitude
  prads = D2R * body->Pitch();
  cp = cos(prads);
  sp = sin(prads);
  rrads = D2R * body->Roll(); 
  cr = cos(rrads);
  sr = sin(rrads);

  // compute deviation from gravity vector and planar direction of "up" hill
  dev = R2D *  acos(cp * cr);
  azm = R2D * atan2(cp * sr, -sp);
//printf("pitch %3.1f, roll %3.1f -> dev %3.1f @ azm %3.1f\n", body->Pitch(), body->Roll(), dev, azm);

  // if non-level for significant time initiate back-and-turn maneuver
  if (dev >= tip)
  {
printf("  off-balance !\n");
    esc0  = jms_now();
    esc_t = base->TurnGoal((azm > 0.0) ? 90.0 : -90.0);
    esc_m = base->MoveGoal(-12.0);
  }

  // continue commanding back-and-turn if aleady started
  if ((esc0 == 0) || (jms_elapsed(esc0) > back))
    return;
  if ((base->MoveErr(esc_m) < 1.0) && (base->TurnErr(esc_t) < 5.0))
    esc0 = 0;
  else
  {
printf("  -> back and turn %3.1f (move err %3.1f)\n", jms_elapsed(esc0), base->Travel() - esc_m);
    base->TurnTarget(esc_t, 0.7, bid);
    base->MoveTarget(esc_m, 1.0, bid);
  }
}


///////////////////////////////////////////////////////////////////////////
//                              User Images                              //
///////////////////////////////////////////////////////////////////////////

//= Get forward color camera image, possibly post-labelling lower left corner.

const jhcImg *jhcVisGrok::HeadView (const char *msg) 
{
  if (msg != NULL) 
    LabelSolid(mark, 10, 30, msg, 1, -5); 
  return &mark;
}


//= Make a pretty version of color image showing relevant items.

void jhcVisGrok::cam_img ()  
{
  jhcMatrix target(4);
  char txt[40];
  int t, gz, sp, nt, hc;

  // get current color camera view
  mark.CopyArr(*col);
  LabelSolid(mark, 560, 20, neck_ctrl(txt, 40), 1, -4);

  // show people
  gz = fn.GazeNew();
  sp = tk.Speaking();
  nt = s3.PersonLim();
  for (t = 0; t < nt; t++)
//    if (s3.PersonState(t) > 0)
    {
      if (s3.PersonID(t) == sp) 
        hc = 2;                                  // green   = speaker
      else if (t == gz)
        hc = 3;                                  // yellow  = newest direct gaze
      else if (fn.FaceCnt(t) > 0)
        hc = 5;                                  // magenta = only if face seen
      else 
        continue;
      s3.PersonCam(mark, t, 0, 1, 0, hc);
    }
  fn.FacesCam(mark);                             // cyan    = faces

  // objects (green = focal, yellow = nodified, magenta = others, cyan = target)
  sobj.AttnCam(mark, 2, 3, 5);     
//  arm->PosGoal(target, lift->Height());          // where the hand will end up
//  sobj.MarkCam(mark, target, 6);                
}


//= Generate string telling which behavior is controlling the neck.

const char *jhcVisGrok::neck_ctrl (char *txt, int ssz) const
{
  int bid = neck->MaxBid();

  *txt = '\0';
  if (bid >= 100)
    strcpy_s(txt, ssz, "conscious");
  else if (bid > 0)                    // not disabled
  {
    if (bid == pbid)
      strcpy_s(txt, ssz, "person");
    else if (bid == obid)
      strcpy_s(txt, ssz, "object");
    else if (bid == mbid)
      strcpy_s(txt, ssz, "motion");
    else if (bid == sbid)
      strcpy_s(txt, ssz, "sound");
    else if (bid == rbid)
      strcpy_s(txt, ssz, "restore");
  }
  return txt;
}


//= Get overhead map (or debugging image), possibly post-labelling lower left corner.

const jhcImg *jhcVisGrok::MapView (const char *msg) 
{
  if (msg != NULL) 
    LabelRight(mark2, 35, 40, msg, 16, -3); 
  return &mark2;
}


//= Make pretty version of overhead map and robot sensors.
// always returns a color image and saves dims in wmap, hmap
// probe:  1 = integrated navigation overhead map
//         2 = front-facing false color depth
//
//         3 = object deviations from flat
//         4 = object overhead height map
//         5 = forward range with object boxes
//         6 = object mask, dims, and colors
//         7 = table location for object deposit
//
//         8 = current near floor heights 
//         9 = deviations from planar floor
//        10 = current obstacle classification
//        11 = traversable regions + sensors
//
//        12 = person overhead height map 
//        13 = potential people over min height 
//        14 = person head and shoulders
//        15 = person gaze direction
//
//        16 = sound direction wrt heads
//        17 = visual motion regions

void jhcVisGrok::nav_img ()
{
  const jhcImg *src;

//  if (!(body->NewFrame()))
//    return;

  // choose intermediate image
  if (probe == 2)                      // forward depth
    src = front_depth();       
  else if (probe == 3)                 // object finding        
    src = surface_bumps();              
  else if (probe == 4)     
    src = object_heights();   
  else if (probe == 5) 
    src = object_clip();                
  else if (probe == 6)
    src = color_mask();
  else if (probe == 7)
    src = table_deposit();
  else if (probe == 8)                 // navigation
    src = floor_heights();
  else if (probe == 9)
    src = plane_devs();
  else if (probe == 10)
    src = obstacle_scan();
  else if (probe == 11)
    src = free_space();
  else if (probe == 12)                // person finding
    src = person_heights();
  else if (probe == 13)
    src = person_blobs();
  else if (probe == 14)
    src = head_shoulder();
  else if (probe == 15)
    src = head_gaze();
  else if (probe == 16)                // attentional cues
    src = head_sound();
  else if (probe == 17)
    src = vis_motion();
  else            
    src = integrated_map();            // default overhead map
  
  // convert to color (if needed) 
  if (src != &mark2)
  {
    mark2.SetSize(*src, 3);
    if (src->Valid(1))
      FalseColor(mark2, *src);
    else
      mark2.CopyArr(*src);
  }

  // cache dimensions
  wmap = mark2.XDim();
  hmap = mark2.YDim();
}


//= Integrated overhead navigation map (normal display).
// shows robot in center with distance beams amid classes of obstacles 
// demo 1

const jhcImg *jhcVisGrok::integrated_map ()
{
  strcpy_s(tmap, "Overhead Map");

  // obstacles, sensors, and footprint
  tmp.SetSize(nav.map);
  nav.LocalMap(tmp);               
  nav.Dists(tmp, 1);               
//  nav.RobotCmd(tmp, base->TurnIncGoal(), base->MoveIncGoal());       // robot destination
  nav.RobotBody(tmp);
  
  // recent robot path and target location
  nav.Tail(tmp);
  if (act == 1)                        // approach
    nav.Target(tmp, sx, sy);
  else if (act == 2)                   // follow
    nav.Target(tmp, vd, va, 1);
  return &tmp;
}


//= Input depth image as false color (for debugging).
// demo 2

const jhcImg *jhcVisGrok::front_depth ()             
{
  strcpy_s(tmap, "Depth Image");

  tmp.SetSize(*rng, 1);
  NightSD(tmp, *rng, 2.0, sobj.choke);
  return &tmp;
}


//= Object deviations from flat (for debugging).
// shows zoomed depth around surface and corrections from plane fitting
// demo 3

const jhcImg *jhcVisGrok::surface_bumps ()              
{
  char msg[80];

  strcpy_s(tmap, "Surface Bumps");

  sobj.Detection(tmp);
  sprintf_s(msg, "dt %+5.2f\ndr %+5.2f\ndh %+5.2f", 
            sobj.TiltDev(), sobj.RollDev(), sobj.HtDev());
  LabelSolid(tmp, 10, tmp.YDim() - 40, msg);
  return &tmp;
}


//= Object overhead heights (for debugging).
// shows full height overhead projection with object bounding boxes
// demo 4

const jhcImg *jhcVisGrok::object_heights ()                 
{
  jhcMatrix loc(4);
  char msg[80];
  int item = sobj.Closest(1);

  strcpy_s(tmap, "Object Heights");

  // show full height overhead table view
  tmp.Clone(sobj.map);
  (sobj.blob).DrawOutline(tmp);
 
  // list estimated table height
  sprintf_s(msg, "table %3.1f", sobj.SurfHt());
  LabelRight(tmp, 10, tmp.YDim() - 20, msg, 16, -3);

  // list coordinates for grabbing
  if (item >= 0)
  {
    sobj.World(loc, item);
    sprintf_s(msg, "closest (%3.1f %3.1f %3.1f)", loc.X(), loc.Y(), loc.Z());
    LabelRight(tmp, 10, tmp.YDim() - 40, msg);
  }
  return &tmp;
}


//= Input depth image as grayscale with object detections (for debugging).
// demo 5

const jhcImg *jhcVisGrok::object_clip ()             
{
  strcpy_s(tmap, "Object Clipping");

  tmp.SetSize(*rng, 1);
  NightSD(tmp, *rng, 2.0, sobj.choke);
  mark2.SetSize(tmp, 3);
  CopyMono(mark2, tmp);                // not false color
  sobj.RngClip(mark2);
  return &mark2;
}


//= Bitmask for biggest object along with dimensions and colors (for debugging).
// demo 6

const jhcImg *jhcVisGrok::color_mask ()
{
  char msg[80];
  const jhcImg *shrink = sobj.Swatch();
  int item = sobj.Closest(1);

  strcpy_s(tmap, "Object Properties");

  // clear bitmask
  tmp.SetSize(*col);
  tmp.FillRGB(0, 0, 255);

  // make sure color patch analyzer has been started
  if (shrink != NULL)
    if (sobj.Spectralize(*col, *rng, item, 1) > 0)
    {
      // gate color image where object detected (ROI = just object)
      OverGateRGB(tmp, *col, *shrink, 128, 0, 0, 255);  
      tmp.MaxRoi();                    

      // write dimensions and colors at top
      sprintf_s(msg, "len %3.1f, wid %3.1f, ht %3.1f", 
                sobj.Major(item), sobj.Minor(item), sobj.SizeZ(item));
      sobj.Colors(msg, 80);
      LabelRight(tmp, 10, tmp.YDim() - 40, msg);
    }
  return &tmp;
}


//= Object deposit location wrt table free-space (for debugging).
// demo 7

const jhcImg *jhcVisGrok::table_deposit ()
{
  strcpy_s(tmap, "Deposit Location");

  if (!space2->Valid())
    return &(sobj.map);
  return space2;             // borrowed from jhcManipulate
}


//= Near-floor overhead navigation heights (for debugging).
// demo 8

const jhcImg *jhcVisGrok::floor_heights ()
{
  char msg[80];

  strcpy_s(tmap, "Floor Heights");

  tmp.Clone(nav.map);
  nav.ScanBeam(tmp);
  nav.RobotMark(tmp, 0);
  sprintf_s(msg, "floor %3.1f", nav.SurfHt());
  LabelRight(tmp, 10, tmp.YDim() - 20, msg, 16, -3);
  return &tmp;
}


//= Deviations from planar floor fit (for debugging).
// demo 9

const jhcImg *jhcVisGrok::plane_devs ()
{
  char msg[80];

  strcpy_s(tmap, "Planar Deviations");

  tmp.Clone(nav.Deviations());
  sprintf_s(msg, "dt %+3.1f\ndr %+3.1f\ndh %+3.1f", nav.TiltDev(), nav.RollDev(), nav.HtDev());
  LabelRight(tmp, 10, tmp.YDim() - 40, msg, 16, -5);
  if (nav.NoPlane())
    LabelRight(tmp, 10, tmp.YDim() - 80, "bad fit", 16, -7);
  return &tmp;
}


//= Single scan overhead obstacle classification (for debugging).
// demo 10

const jhcImg *jhcVisGrok::obstacle_scan ()
{
  strcpy_s(tmap, "Obstacle Scan");

  tmp.Clone(nav.Terrain());
  if (nav.NoPlane())
    LabelRight(tmp, 10, tmp.YDim() - 20, "bad fit", 16, -7);
  return &tmp;
}


//= Traversable regions and sensors rays (for debugging).
// demo 11

const jhcImg *jhcVisGrok::free_space ()
{
  strcpy_s(tmap, "Free Space");

  tmp.SetSize(nav.map);
  Threshold(tmp, nav.OpenAreas(), 128, 70);          // full = light blue
  UnderGate(tmp, tmp, nav.Traversable(), 128, 50);   // center = dark blue
  nav.Dists(tmp, 0);               
  return &tmp;
}


//= Person-finding overhead heights (for debugging).
// demo 12

const jhcImg *jhcVisGrok::person_heights ()
{
  char msg[80];

  strcpy_s(tmap, "Person Heights");

  tmp.Clone(s3.map);
  sprintf_s(msg, "ztab %+3.1f", sobj.ztab);
  LabelRight(tmp, 10, tmp.YDim() - 20, msg, 16, -5);
  return &tmp;
}


//= Blobs tall enough to be people (for debugging).
// demo 13

const jhcImg *jhcVisGrok::person_blobs ()
{
  strcpy_s(tmap,"Potential People");

  return s3.ChestShrink();
}


//= Detected person heads and shoulders (for debugging).
// demo 14

const jhcImg *jhcVisGrok::head_shoulder ()
{
  strcpy_s(tmap, "Heads and Shoulders");

  s3.dbg = 1;
  tmp.SetSize(s3.map);
  s3.HeadLevels(tmp);
  s3.ShowHeads(tmp, s3.dude, s3.NumPotential(), 0, 8.0, -7);
  return &tmp;
}


//= Overhead heads and gaze direction from face (for debugging).
// demo 15

const jhcImg *jhcVisGrok::head_gaze ()
{
  strcpy_s(tmap, "Gaze Direction");

  tmp.Clone(s3.map);
  s3.CamLoc(tmp, 0);
  s3.AllHeads(tmp);
  fn.AllGaze(tmp);
  return &tmp;
}


//= Overhead heads and sound direction beam (for debugging).
// demo 16

const jhcImg *jhcVisGrok::head_sound ()
{
  int spk = tk.Speaking(), persist = -5;

  strcpy_s(tmap, "Sound Direction");

  tmp.Clone(s3.map);
  s3.AllHeads(tmp);                    // magenta
  if (spk >= 0)                        // cyan
    s3.ShowID(tmp, spk);
  tk.SoundMap(tmp, 0, 2);              // talk
  if (mic->Sound() > persist)
    tk.SoundMap(tmp);                  // beam
  return &tmp;
}


//= Areas with high visual motion (for debugging).
// demo 17

const jhcImg *jhcVisGrok::vis_motion ()
{
  double mx, my;

  strcpy_s(tmap, "Motion Regions");

  tmp.SetSize(mot.sal);
  Squelch(tmp, mot.sal, 128);
  if (mot.Center(mx, my))
    Cross(tmp, mx, my, 17, 17, 3, 50); 
  return &tmp;
}


///////////////////////////////////////////////////////////////////////////
//                             Log Images                                //
///////////////////////////////////////////////////////////////////////////

//= Save input and output images from end of run.
// needs base working directory for file names (mostly Linux)

void jhcVisGrok::DumpImages (const char *wdir)
{
  jhcImgIO jio;
  char aux[80], fname[80];

  // needed for creating full file names (Linux mostly)
  if (wdir == NULL)
    return;

  // get rid of previous input pair (delete uses wildcard for previous tilt)
#ifdef __linux__
  sprintf_s(aux, "rm -f %sdump/last*.*", wdir);
#else
  sprintf_s(aux, "del \"%sdump\\last*.*\" > nul 2>&1", wdir);
#endif
  if (system(aux) != 0)
    jprintf(">>> Could not purge old image files in dump directory!\n");

  // save final input image pair 
  sprintf_s(fname, "%sdump/last_t%d.bmp", wdir, ROUND(-10.0 * neck->Tilt()));
  jio.SaveDual(fname, *raw, *rng); 

  // save final basic console images
  sprintf_s(fname, "%sdump/view.bmp", wdir);
  jio.Save(fname, *(HeadView()), 1);
  sprintf_s(fname, "%sdump/map.bmp", wdir);
  jio.Save(fname,  *(MapView()), 1);
}

