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
  // current software version
  ver = 3.80;
  
  // connect processing to basic robot I/O
  rwi.BindBody(&body);

  // bridge from operators to robot motion
  ball.Platform(&rwi);
  kern.AddFcns(&ball);
  soc.Platform(&rwi);
  kern.AddFcns(&soc);
  vscn.Platform(&rwi);
  kern.AddFcns(&vscn);

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

  ok &= time_params(fname);
  ok &= ball.Defaults(fname);
  ok &= soc.Defaults(fname);
  ok &= vscn.Defaults(fname);
  ok &= rwi.Defaults(fname);
  ok &= body.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcEliCoord::SaveVals (const char *fname) 
{
  int ok = 1;

  ok &= tps.SaveVals(fname);
  ok &= ball.SaveVals(fname);
  ok &= soc.SaveVals(fname);
  ok &= vscn.SaveVals(fname);
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
// mech: 0 for no body, 1 init body, 2 for forced boot of body ONLY
// returns 2 if robot ready, 1 if ready but no robot, 0 or negative for error

int jhcEliCoord::Reset (int mech)
{
  int rc = 0;

  // start up body (and get robot name)
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
  return((rc <= 0) ? 1 : 2);
}


//= Generate actions in response to update sensory information.
// returns 1 if happy, 0 to end interaction 

int jhcEliCoord::Respond ()
{
  int eye;

  // get new speech input then await post-processed robot sensors
  if (UpdateSpeech() <= 0)
    return 0;
  if (rwi.Update(SpeechRC(), NextSense()) <= 0)
    return 0;

  // coordinate semantic net with visual info
  if (SpeechRC() == 2)
    check_user((rwi.tk).Speaking());
  tag_tracks();

  // always pay attention to speech if current eye contact
  if ((eye = (rwi.fn).AnyGaze()) > 0)
    alert = 1;
  else if (Attending() <= 0)
    alert = 0;
  if (rwi.base != NULL)
    (rwi.base)->AttnLED(alert);

  // figure out what to do then issue action commands
  if (jhcAliaSpeech::Respond(eye) <= 0)
    return 0;
  if (rwi.Issue() <= 0)
    return 0;

  // think a bit more (but must wait to give commands)
  DayDream(); 
  return 1;
}


//= Call at end of run to put robot in stable state and possibly save knowledge.

void jhcEliCoord::Done (int face)
{
  if (mech > 0)
    body.Freeze();
  rwi.Stop();
  if ((body.vid) != NULL)
    (body.vid)->Prefetch(0);
  jhcAliaSpeech::Done();
  if (face > 0)
    ((rwi.fn).fr).SaveDB("all_people.txt");
}


///////////////////////////////////////////////////////////////////////////
//                      Visual Semantic Linkage                          //
///////////////////////////////////////////////////////////////////////////

//= Associate current speaker (if any) with current semantic net user.

void jhcEliCoord::check_user (int id)
{
  jhcBodyData *p = (rwi.s3).RefID(id);
  jhcNetNode *n, *user = atree.Human();
  const char *name, *lex;
  int cnt, i = (rwi.s3).TrackIndex(id), w = 0, best = 0;

  // make talking head and user semantic net node be the same
  if (p == NULL)
    return;
  n = (jhcNetNode *) p->node;
  if (n == NULL)
    p->node = (void *) user;
  else if (n != user)
  {
    jprintf(1, noisy, "\n  ... changing user %s to speaker %s ...\n", user->Nick(), n->Nick());
    atree.SetUser(n);
    user = n;
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
  while ((lex = user->Word(w++)) != NULL)
    if ((strcmp(lex, "me") != 0) && (strcmp(lex, "I") != 0))
    {
      cnt = (int) strlen(lex);
      if (cnt > best)
      {
        name = lex;
        best = cnt;
      }
    }
  if (best > 0)
    if (strncmp(name, sp.UserName(), best) != 0)     // beginning matches ("Jon")
      if (sp.SetUser(name, 0, 1) > 0)
        jprintf(1, noisy, "\n  ... request acoustic model = %s ...\n", name);
}


//= Make sure each visible head has a tag consistent with any associated node.
// also sets all person-associated nodes to be retained during garbarge collection

void jhcEliCoord::tag_tracks ()
{
  char first[80];
  jhcBodyData *p;
  jhcNetNode *n;
  const char *name;
  char *end;
  int i, w, nlim = (rwi.s3).PersonLim();

  // examine all valid tracks
  for (i = 0; i < nlim; i++)
    if ((rwi.s3).PersonOK(i))
    {
      // get associated semantic node (if any)
      p = (rwi.s3).RefPerson(i);
      n = (jhcNetNode *) p->node;
      if (n == NULL) 
        continue;
      n->keep = 1;                     // do not remove reference

      // possibly copy node's preferred word to tag
      w = 0;
      while ((name = n->Word(w++)) != NULL)
        if ((strcmp(name, "me") != 0) && (strcmp(name, "I") != 0))
          break;
      if (name != NULL)
        if (strcmp(p->tag, name) != 0)
          strcpy_s(p->tag, name);

      // possibly add face recognition name as lexical form
      if ((name = (rwi.fn).FaceName(i)) != NULL)
        if (*name != '\0')
        {
          // possibly add full name ("Jon Connell")
          if (!n->HasWord(name))
          {
            atree.AddLex(n, name);
            jprintf(1, noisy, "\n  ... adding %s lex \"%s\" ...\n", n->Nick(), name);
          }

          // possibly add first name ("Jon")
          strcpy_s(first, name);
          if ((end = strchr(first, ' ')) != NULL)
            *end = '\0';
          if (!n->HasWord(first))
            atree.AddLex(n, first);
        }
  }
}
