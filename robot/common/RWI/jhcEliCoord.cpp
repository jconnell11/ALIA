// jhcEliCoord.cpp : top level parsing, learning, and control for ELI robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2025 Etaoin Systems
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

#include "Interface/jhcMessage.h"

#include "RWI/jhcEliCoord.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcEliCoord::~jhcEliCoord ()
{
}


//= Default constructor initializes certain values.

jhcEliCoord::jhcEliCoord ()
{
  // connect display utilities to data
  disp.Bind(stat);
  
  // connect processing to basic robot I/O
  rwi.body = &body;
  rwi.neck = &(body.neck);
  rwi.arm  = &(body.arm);
  rwi.lift = &(body.lift);
  rwi.base = &(body.base);

  // connect microphone (if any) to person finder
  rwi.mic = &(body.mic);              
  (rwi.tk).RemoteMic(rwi.mic);

  // input images
  rwi.rng = &(body.Range());
  rwi.col = &(body.Color());
  rwi.raw = &(body.Input());
  
  // attach grounding kernels
  kern.AddFcns(ball);
  kern.AddFcns(soc);
  kern.AddFcns(svis);
  kern.AddFcns(man);
  kern.AddFcns(sup);
  kern.Platform(&rwi, "jhcVisGrok");

  // default processing parameters and state
  noisy = 1;
  mech = 0;
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for overall control of timing.
// this should be called in Defaults and tps used in SaveVals

int jhcEliCoord::kern_params (const char *fname)
{
  jhcParam *ps = &kps;
  int ok;

  ps->SetTag("kern_dbg", 0);
  ps->NextSpec4( &(svis.dbg),    2, "SceneVis objects (std = 2)");
  ps->NextSpec4( &(sup.dbg),     2, "Support surfaces (std = 2)");
  ps->NextSpec4( &(soc.dbg),     2, "Social agents (std = 2)");
  ps->Skip();
  ps->NextSpec4( &(ball.dbg),    1, "Ballistic body (std = 1)");
  ps->NextSpec4( &(man.dbg),     1, "Manipulation arm (std = 1)");

  ps->NextSpec4( &(dmem.enc),    0, "LTM encoding (dbg = 3)");
  ps->NextSpec4( &(dmem.detail), 0, "LTM retrieval for node");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcEliCoord::Defaults (const char *fname)
{
  int ok = 1;

  // local parameters
  ok &= time_params(fname);
  ok &= kern_params(fname);
  ok &= jhcAliaCore::Defaults(fname);

  // kernel parameters
  ok &= ball.Defaults(fname);
  ok &= soc.Defaults(fname);
  ok &= svis.Defaults(fname);
  ok &= man.Defaults(fname);
  ok &= sup.Defaults(fname);

  // component parameters
  ok &= rwi.Defaults(fname);
  ok &= body.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcEliCoord::SaveVals (const char *fname) 
{
  int ok = 1;

  // local parameters
  ok &= tps.SaveVals(fname);
  ok &= kps.SaveVals(fname);
  ok &= jhcAliaCore::SaveVals(fname);

  // kernel parameters
  ok &= ball.SaveVals(fname);
  ok &= soc.SaveVals(fname);
  ok &= svis.SaveVals(fname);
  ok &= man.SaveVals(fname);
  ok &= sup.SaveVals(fname);

  // component parameters
  ok &= rwi.SaveVals(fname);
  ok &= body.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Connect a possibly canned video input to robot (or disconnect all). 

int jhcEliCoord::BindVideo (jhcVideoSrc *v, int vnum)
{
  body.BindVideo(v);
  return 1;
}


//= Reset state for the beginning of a sequence.
// bmode: 0 for no body, 1 or more for init body (2 used for autorun in CBanzaiDoc)
// cvt: 0 no log file, 1 save input text to network conversions
// if wds > 0 then assumes vocabulary is complete and build word list
// returns 2 if robot ready, 1 if ready but no robot, 0 or negative for error

int jhcEliCoord::Reset (int bmode, int cvt)
{
  int rc = 0;

  // do not batch up printfs for speed
  jprintf_fflush = 1;                            

  // set graph scaling
  disp.hz = shz;

  // start up body (and get robot name)
  mech = bmode;
  if (mech > 0)
    if ((rc = body.Reset(1, mech - 1, mech)) <= 0)
      return -1;

  // configure actuators
  if (mech > 0)
  {
    (body.base).Zero();
    body.InitPose();         // used to leave height unaltered (-1.0)
    body.Update(-1, 1);      // sensor info will be waiting and need to be read
  }
  else
    body.StaticPose();       // set neck angles and head height for static image

  // start background processing of video
  rwi.Reset(mech, 1, body.choke);
  alert = 0;

  // initialize speech and reasoning and add user faces
  if (jhcAliaSAPI::Reset(body.rname, body.vname, cvt) <= 0)
    return 0;
  ((rwi.fn).fr).LoadDB(wrt("config/VIPs.txt"), 0);

  // possibly reset battery gauge
  if (mech > 0)
    body.UpdateBat();        
  return((rc <= 0) ? 1 : 2);
}


//= Generate actions in response to update sensory information.
// returns 1 if happy, 0 to end interaction 

int jhcEliCoord::Respond ()
{
  jhcEliBase *b = &(body.base);
  const jhcEliArm *a  = &(body.arm);
  const jhcEliNeck *n = &(body.neck);
  int eye;

  // get new speech input then await post-processed robot sensors
  if (UpdateSpeech() <= 0)
    return 0;
  if (rwi.Update(SpeechRC(), NextSense()) <= 0)
    return 0;

  // indicate listening by LED if current eye contact (or attention word)
  if ((eye = (rwi.fn).AnyGaze()) > 0)
    alert = 1;
  else if ((amode > 0) && (Attending() > 0))     
    alert = 1;
  else if (Attending() <= 0)
    alert = 0;
  if (rwi.base != NULL)
    b->AttnLED(alert);                 // could use eye instead

  // pass dynamic status of body to mood monitor and statistics collector
  if (!rwi.Ghost())
  {
    mood.Travel(b->TravelRate());
    mood.Reach(a->ReachRate());
    mood.Battery(body.Percent());
    stat.Drive(b->MoveCmdV(), b->MoveIPS(0), b->TurnCmdV(), b->TurnDPS(0));
    stat.Gaze(n->PanCtrlGoal(), n->Pan(), n->TiltCtrlGoal(), n->Tilt());
  }

  // figure out what to do then issue action commands
  if (jhcAliaSAPI::Consider(eye) <= 0)
    return 0;
  if (rwi.Issue() <= 0)
    return 0;

  // change acoustic model if face recognized or new name fact
  if (SpeechRC() == 2)
    UserVoice(rwi.FaceSpeak());

  // think a bit more but no GC (any new body commands must wait to run)
  DayDream();
  return 1;
}


//= Get some possibly annotated image to display on GUI.

const jhcImg *jhcEliCoord::View (int num) 
{
  if (!body.NewFrame())
    return NULL;
  return((num <= 0) ? rwi.HeadView() : rwi.MapView());
}


//= Call at end of run to put robot in stable state and possibly save knowledge.

int jhcEliCoord::Done (int face)
{
  int batt = -1;

  // stop real time interaction and get battery state
  if (mech > 0)
    body.Freeze();
  rwi.Stop();
  if ((body.vid) != NULL)
    (body.vid)->Prefetch(0);
  if (!rwi.Ghost())
    batt = body.Percent();

  // cleanup old files in some directories (2 weeks)
  jprintf("Purging old KB, log, and timing files ... ");
  fflush(stdout);
  jprintf_purge(wrt("KB"));
  jprintf_purge(wrt("log"));
  jprintf_purge(wrt("timing"));
  jprintf("\n");
  fflush(stdout);

  // save info from run
  if (acc < 2)                         // for debugging (same as KB)
    DumpAll();                         
  DumpSession();                       // brand new rules and ops
  jhcAliaSAPI::Done(1, batt);          // incl. accumulated knowledge
  if (face > 0)
    ((rwi.fn).fr).SaveDB("config/all_people.txt");
  rwi.DumpImages(Dir());

  // extra battery warning (if needed)
  if ((batt >= 0) && (batt <= 20))
  {
    jprintf("%3.1f volts - CONSIDER RECHARGING", body.Voltage());
    body.Beep();
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Overlay stick figure of arm onto camera image in some color.
// optionally show ray of some length (inches) from grasp point
// best if arm angles are not changing (i.e. don't call during update)
// NOTE: this is only for the color camera view

int jhcEliCoord::Skeleton (jhcImg& dest, double ray) const
{
  jhcMatrix pos(4), off(4);
  int jt[5] = {1, 2, 3, 6, 7};
  double px, py, ix, iy;
  int i; 

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcEliCoord::Skeleton");

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
  off.SetVec3((body.arm).ToolX() + ray, 0.0, 0.0);
  ((body.arm).jt[6]).GlobalMap(pos, off);
  (rwi.s3).ImgPtZ(ix, iy, pos.X(), pos.Y(), pos.Z() + (body.lift).Height());
  DrawLine(dest, px, py, ix, iy, 3, -3); 
  return 1;
}


//= Overlay stick figure of arm onto overhead object map image.
// optionally show ray of some length (inches) from grasp point
// NOTE: this is only for the overhead map view (adjusts for neck pose)

int jhcEliCoord::MapArm (jhcImg& dest, double ray) const
{
  jhcMatrix pos(4), off(4);
  int jt[5] = {1, 2, 3, 6, 7};
  double px, py, mx, my;
  int i; 

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcEliCoord::MapArm");

  // draw links from shoulder and circle grip location
  (body.arm).JtPos(pos, 0);
  (rwi.sobj).ViewPels(px, py, pos.X(), pos.Y());
  for (i = 0; i < 5; i++)
  {
    // select some joint and get map coords
    if (jt[i] == 7)
      (body.arm).Position(pos);
    else if (jt[i] == 2)
      (body.arm).LiftBase(pos);        // looks better on screen
    else
      (body.arm).JtPos(pos, jt[i]);
    (rwi.sobj).ViewPels(mx, my, pos.X(), pos.Y());

    // draw segment
    DrawLine(dest, px, py, mx, my, 3, -5);
    px = mx;
    py = my;
  }
  CircleEmpty(dest, px, py, 10, 3, -5);  

  // ray in grip direction 
  if (ray <= 0.0)
    return 1;
  off.SetVec3((body.arm).ToolX() + ray, 0.0, 0.0);
  ((body.arm).jt[6]).GlobalMap(pos, off);
  (rwi.sobj).ViewPels(mx, my, pos.X(), pos.Y());
  DrawLine(dest, px, py, mx, my, 3, -3); 
  return 1;
}


//= Find the pixel location of a particular arm joint.
// jt: 0 = shoulder,   1 = elbow,     2 = FOREARM lift, 
//     3 = wrist roll, 4 = wrist pan, 5 = wrist tilt, 
//     6 = jaw axis,   7 = mid tips
// returns non-scaled z coordinate (for use with jhcSurface3D::WorldPt)

double jhcEliCoord::ImgJt (double& ix, double& iy, int jt) const
{
  jhcMatrix pos(4);

  if ((jt < 0) || (jt > 7))
    return 0;

  if (jt == 7)
    (body.arm).Position(pos);
  else if (jt == 2)
    (body.arm).LiftBase(pos);         // looks better on screen
  else
    (body.arm).JtPos(pos, jt);
  return (rwi.s3).ImgPtZ(ix, iy, pos.X(), pos.Y(), pos.Z() + (body.lift).Height());
}


//= Get angle difference of the click location versus projected jt1 relative to projected jt0.
// primarily used by arm calibration routines in jhcBanzaiDoc

double jhcEliCoord::ImgVeer (int mx, int my, int jt1, int jt0) const
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

