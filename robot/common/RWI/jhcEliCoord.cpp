// jhcEliCoord.cpp : top level parsing, learning, and control for ELI robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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
  // for debugging - only happens when program closes
  DumpAll();
}


//= Default constructor initializes certain values.

jhcEliCoord::jhcEliCoord ()
{
  // connect display utilities to data
  disp.Bind(stat);
  
  // connect processing to basic robot I/O
  rwi.BindBody(body);

  // attach grounding kernels
  kern.AddFcns(ball);
  kern.AddFcns(soc);
  kern.AddFcns(svis);
  kern.AddFcns(man);
  kern.AddFcns(sup);
  kern.Platform(&rwi);

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

//= Add the names of important people for face recognition and grammar.
// can append to any that have previously been specified
// should be called after Reset (else speech grammar might be cleared)
// better to build word list at this point (wds > 0) rather than in Reset
// returns number just added

int jhcEliCoord::SetPeople (const char *fname, int append, int wds)
{
  int i, n, n0 = ((append > 0) ? vip.Names() : 0);

  ((rwi.fn).fr).LoadDB(fname, append);
  n = vip.Load(fname, append);
  for (i = 0; i < n; i++)
    AddName(vip.Full(i + n0));
  jprintf("Added %d known users from %s\n\n", n, fname);
  if (wds > 0)
    vc.GetWords(gr.Expansions());
  return n;
}


//= Connect a possibly canned video input to robot (or disconnect all). 

int jhcEliCoord::BindVideo (jhcVideoSrc *v, int vnum)
{
  body.BindVideo(v);
  return 1;
}


//= Reset state for the beginning of a sequence.
// bmode: 0 for no body, 1 or more for init body (2 used for autorun in CBanzaiDoc)
// if wds > 0 then assumes vocabulary is complete and build word list
// returns 2 if robot ready, 1 if ready but no robot, 0 or negative for error

int jhcEliCoord::Reset (int bmode)
{
  int rc = 0;

  // set graph scaling
  disp.hz = shz;

  // start up body (and get robot name)
  mech = bmode;
  if (mech > 0)
    if ((rc = body.Reset(1, mech - 1)) <= 0)
      return -1;

  // start background processing of video
  rwi.Reset(mech);
  alert = 0;

  // initialize timing and speech components
  if (jhcAliaSpeech::Reset(body.rname, body.vname, 1) <= 0)
    return 0;
  if (mech > 0)
    body.UpdateBat();        // possibly reset battery gauge
  return((rc <= 0) ? 1 : 2);
}


//= Generate actions in response to update sensory information.
// returns 1 if happy, 0 to end interaction 

int jhcEliCoord::Respond ()
{
  const jhcEliBase *b = &(body.base);
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
    (rwi.base)->AttnLED(alert);                  // could use eye instead

  // pass dynamic status of body to mood monitor and statistics collector
  if (!rwi.Ghost())
  {
    mood.Body(body.BodyIPS(), a->FingerIPS(), body.Percent());
    stat.Drive(b->MoveCmdV(), b->MoveIPS(0), b->TurnCmdV(), b->TurnDPS(0));
    stat.Gaze(n->PanCtrlGoal(), n->Pan(), n->TiltCtrlGoal(), n->Tilt());
  }

  // figure out what to do then issue action commands
  if (jhcAliaSpeech::Respond(eye) <= 0)
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
  // stop real time interaction
  if (mech > 0)
    body.Freeze();
  rwi.Stop();
  if ((body.vid) != NULL)
    (body.vid)->Prefetch(0);

  // save learned items
  DumpSession();                       // brand new rules and ops
  jhcAliaSpeech::Done(1);              // incl. accumulated knowledge
  if (face > 0)
    ((rwi.fn).fr).SaveDB("all_people.txt");

  // possibly report robot power level
  if (!rwi.Ghost())
    body.ReportCharge();
  return 1;
}


