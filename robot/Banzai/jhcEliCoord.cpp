// jhcEliCoord.cpp : top level parsing, learning, and control for ELI robot
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

#include "Interface/jhcMessage.h"

#include "jhcEliCoord.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcEliCoord::jhcEliCoord ()
{
  // current software version (sync with MensEt)
  ver = 4.50;
  
  // connect processing to basic robot I/O
  rwi.BindBody(&body);

  // bridge from operators to robot motion
  ball.Platform(&rwi);
  kern.AddFcns(&ball);
  soc.Platform(&rwi);
  kern.AddFcns(&soc);
  svis.Platform(&rwi);
  kern.AddFcns(&svis);

  // default processing parameters and state
  noisy = 1;
  mech = 0;
  Defaults();
}


//= Default destructor does necessary cleanup.

jhcEliCoord::~jhcEliCoord ()
{
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
  ok &= jhcAliaCore::Defaults(fname);

  // kernel parameters
  ok &= ball.Defaults(fname);
  ok &= soc.Defaults(fname);
  ok &= svis.Defaults(fname);

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
  ok &= jhcAliaCore::SaveVals(fname);

  // kernel parameters
  ok &= ball.SaveVals(fname);
  ok &= soc.SaveVals(fname);
  ok &= svis.SaveVals(fname);

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
// returns number just added

int jhcEliCoord::SetPeople (const char *fname, int append)
{
  int i, n, n0 = ((append > 0) ? vip.Names() : 0);

  // enable spontaneous face recognition (and updating)
  ((rwi.fn).fr).LoadDB(fname, append);

  // make sure grammar knows of names (first and full)
  n = vip.Load(fname, append);
  for (i = n0; i < n; i++)
  {
    gr.ExtendRule("NAME", vip.Full(i));
    gr.ExtendRule("NAME", vip.First(i));
  }
  if (SpeechIn() <= 0)
    return n;

  // possibly update speech front-end also 
  if (spin == 1)
  {
    sp.Listen(0);
    for (i = n0; i < n; i++)
    {
      sp.ExtendRule("NAME", vip.Full(i), 0);
      sp.ExtendRule("NAME", vip.First(i), 0);
    }
    sp.Listen(1);
  }
  return n;
}


//= Reset state for the beginning of a sequence.
// bmode: 0 for no body, 1 init body, 2 for forced boot of body ONLY
// returns 2 if robot ready, 1 if ready but no robot, 0 or negative for error

int jhcEliCoord::Reset (int bmode)
{
  int rc = 0;

  // start up body (and get robot name)
  mech = bmode;
  if (mech > 0)
    if ((rc = body.Reset(1, mech - 1)) <= 0)
      return -1;

  // possibly start background processing of video
  if (mech >= 2)
    return 2;
  rwi.Reset(mech);
  alert = 0;

  // initialize timing and speech components
  if (jhcAliaSpeech::Reset(body.rname, body.vname) <= 0)
    return 0;
  if (mech > 0)                      
    body.Charge(1);                    // possibly reset battery gauge
  return((rc <= 0) ? 1 : 2);
}


//= Generate actions in response to update sensory information.
// returns 1 if happy, 0 to end interaction 

int jhcEliCoord::Respond ()
{
  const jhcEliBase *b = &(body.base);
  const jhcEliNeck *n = &(body.neck);
  int eye;

  // get new speech input then await post-processed robot sensors
  if (UpdateSpeech() <= 0)
    return 0;
  if (rwi.Update(SpeechRC(), NextSense()) <= 0)
    return 0;

  // coordinate semantic net with visual info (incl. retaining nodes)
  if (SpeechRC() == 2)
    check_user((rwi.tk).Speaking());
  wmem_heads();

  // indicate listening by LED if current eye contact (or attention word)
  if ((eye = (rwi.fn).AnyGaze()) > 0)
    alert = 1;
  else if ((amode > 0) && (Attending() > 0))     
    alert = 1;
  else if (Attending() <= 0)
    alert = 0;
  if (rwi.base != NULL)
    (rwi.base)->AttnLED(alert);

  // pass dynamic status of body to mood monitor and statistics collector
  if (!rwi.Ghost())
  {
    mood.Walk(b->MoveIPS());
    mood.Wave((body.arm).FingerIPS());
    mood.Energy(body.Percent());
    stat.Drive(b->MoveCmdV(), b->MoveIPS(), b->TurnCmdV(), b->TurnDPS());
    stat.Gaze(n->PanCtrlGoal(), n->Pan(), n->TiltCtrlGoal(), n->Tilt());
  }

  // figure out what to do then issue action commands
  if (jhcAliaSpeech::Respond(eye) <= 0)
    return 0;
  if (rwi.Issue() <= 0)
    return 0;

  // think a bit more but no GC (any new body commands must wait to run)
  DayDream(); 
  return 1;
}


//= Call at end of run to put robot in stable state and possibly save knowledge.

void jhcEliCoord::Done (int face, int status)
{
  // stop real time interaction
  if (mech > 0)
    body.Freeze();
  rwi.Stop();
  if ((body.vid) != NULL)
    (body.vid)->Prefetch(0);

  // save learned items
  DumpSession();                       // brand new rules and ops
  jhcAliaSpeech::Done();               // incl. accumulated knowledge
  if (face > 0)
    ((rwi.fn).fr).SaveDB("all_people.txt");

  // possibly report session status
  if (status > 0)
  {
    jprintf("\n==========================================================\n");
    ShowMem();
    if (!rwi.Ghost())
      body.ReportCharge();
    jprintf("DONE - Think %3.1f Hz, Sense %3.1f Hz\n", Thinking(), Sensing()); 
  }
}


///////////////////////////////////////////////////////////////////////////
//                      Visual Semantic Linkage                          //
///////////////////////////////////////////////////////////////////////////

//= Associate current speaker (if any) with current semantic net user.

void jhcEliCoord::check_user (int id)
{
  jhcBodyData *p = (rwi.s3).RefID(id);
  jhcNetNode *agt, *user = atree.Human();
  const char *name, *who;
  int cnt, i = (rwi.s3).TrackIndex(id), w = 0, best = 0;

  // make talking head and user semantic net node be the same
  if (p == NULL)
    return;
  agt = atree.ExtRef(id, 1);
  if (agt == NULL)
  {
    jprintf(1, noisy, "\n  ... linking user %s to head track %d ...\n", user->Nick(), id);
    atree.ExtLink(id, user, 1);
  }
  else if (agt != user)
  {
    jprintf(1, noisy, "\n  ... changing user %s to speaker %s ...\n", user->Nick(), agt->Nick());
    atree.SetUser(agt);
    user = agt;
  }

  // possibly set speech model based on face recognition
  if ((name = (rwi.fn).FaceName(i)) != NULL)
    if (*name != '\0')
    {
      if (strcmp(name, sp.UserName()) == 0)            // full string matches
        return;
      if (sp.SetUser(name, 0, 1) > 0)
      {
        jprintf(1, noisy, "\n  ... request acoustic model = %s ...\n", name);
        return;
      }
    }

  // try setting speech model to longest lexical tag
  while ((who = user->Name(w++)) != NULL)
  {
    cnt = (int) strlen(who);
    if (cnt > best)
    {
      name = who;
      best = cnt;
    }
  }
  if (best > 0)
    if (strncmp(name, sp.UserName(), best) != 0)     // beginning matches ("Jon")
      if (sp.SetUser(name, 0, 1) > 0)
        jprintf(1, noisy, "\n  ... request acoustic model = %s ...\n", name);
}


//= Make sure each visible head has a tag consistent with any associated node.
// overwrite blank or node-based label, does not change full name from face reco 

void jhcEliCoord::wmem_heads ()
{
  jhcBodyData *p;
  jhcNetNode *agt;
  const char *name;
  int i, nlim = (rwi.s3).PersonLim();

  for (i = 0; i < nlim; i++)
    if ((rwi.s3).PersonOK(i))
    {
      p = (rwi.s3).RefPerson(i);
      if ((agt = atree.ExtRef(p->id, 1)) == NULL)
      {
        p->state = 0;                            // do not draw
        if (strchr(p->tag, '-') != NULL)         // node nickname
          p->tag[0] = '\0';
      }                                   
      else if ((name = agt->Name(0, atree.MinBlf())) != NULL)
        if ((p->tag[0] == '\0') || (strcmp(p->tag, agt->Nick()) == 0))
          if (strcmp(p->tag, name) != 0)
            strcpy_s(p->tag, name);              // net-derived name
    }
}

