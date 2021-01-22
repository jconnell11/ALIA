// jhcSocial.cpp : interface to ELI people tracking kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#include <math.h>
#include <string.h>

#include "Interface/jms_x.h"           // common video

#include "Grounding/jhcSocial.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSocial::~jhcSocial ()
{
}


//= Default constructor initializes certain values.

jhcSocial::jhcSocial ()
{
  ver = 1.10;
  strcpy_s(tag, "Social");
  Platform(NULL);
  rpt = NULL;
  dbg = 0;
  Defaults();
}


//= Attach physical enhanced body and make pointers to some pieces.

void jhcSocial::Platform (jhcEliGrok *robot) 
{
  rwi = robot;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for detecting and selecting people.

int jhcSocial::attn_params (const char *fname)
{
  jhcParam *ps = &aps;
  int ok;

  ps->SetTag("soc_attn", 0);
  ps->NextSpecF( &pnear, 40.0, "Person near alert (in)");
  ps->Skip();
  ps->NextSpecF( &ltol,   2.0, "Look achieved (deg)");
  ps->NextSpecF( &lquit,  2.0, "Look timeout (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for motion relative to people.

int jhcSocial::move_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("soc_move", 0);
  ps->NextSpecF( &cozy,   36.0, "Approach distance (in)");
  ps->NextSpecF( &aquit,  10.0, "Timeout for approach (sec)");
  ps->NextSpecF( &ideal,  36.0, "Following distance (in)");
  ps->NextSpecF( &worry,  60.0, "Too far distance (in)");
  ps->NextSpecF( &ttime,   1.0, "Turn response (sec)");
  ps->NextSpecF( &orient, 60.0, "Alignment for move (deg)");

  ps->NextSpecF( &atime,   2.0, "Approach response (sec)");
  ps->NextSpecF( &ftime,   1.0, "Follow response (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcSocial::Defaults (const char *fname)
{
  int ok = 1;

  ok &= attn_params(fname);
  ok &= move_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcSocial::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= aps.SaveVals(fname);
  ok &= mps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Set up for new run of system.

void jhcSocial::local_reset (jhcAliaNote *top)
{
  rpt = top;
  hwin = -1;
  told = 0;
  uid = 0;
}


//= Post any spontaneous observations to attention queue.

void jhcSocial::local_volunteer ()
{
  vip_seen();   
  vip_close(); 
  user_gone();
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcSocial::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(soc_look);
  JCMD_SET(soc_approach);
  JCMD_SET(soc_follow);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcSocial::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(soc_look);
  JCMD_CHK(soc_approach);
  JCMD_CHK(soc_follow);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                           Reported Events                             //
///////////////////////////////////////////////////////////////////////////

//= Inject NOTE saying a particular person's face has just been recognized.
// states: "X hq visible"

void jhcSocial::vip_seen ()
{
  int i;

  // lock to sensor cycle 
  if ((rpt == NULL) || (rwi == NULL) || (rwi->body == NULL))
    return;
  if (!rwi->Accepting())
    return;

  // generate message if new person just recognized 
  if ((i = (rwi->fn).JustNamed()) < 0)
    return;
  person_prop(i, "visible", 0);
}


//= Generate an event whenever a person gets inside the robot's personal space.
// states: "X is close" where X is a person and perhaps has a name

void jhcSocial::vip_close ()
{
  jhcBodyData *p;
  int old = hwin;

  // lock to sensor cycle 
  if ((rpt == NULL) || (rwi == NULL) || (rwi->body == NULL))
    return;
  if (!rwi->Accepting())
    return;

  // see if a new person is the closest
  if ((hwin = (rwi->s3).Closest()) < 0)
    return;
  if ((hwin == old) && (told > 0))
    return;
  told = 0;

  // if they are in personal space then generate message
  p = (rwi->s3).RefPerson(hwin);
  if (p->PlaneVec3() > pnear)
    return;
  person_prop(hwin, "close", 0);
  told = 1;                            // only once per person
}


//= Generate an event whenever user had been seen but now seems to have vanished.
// states: "X is not visible" where X is the current user 

void jhcSocial::user_gone ()
{
  jhcAliaDesc *user;
  int uid0 = uid;

  // lock to sensor cycle 
  if ((rpt == NULL) || (rwi == NULL) || (rwi->body == NULL))
    return;
  if (!rwi->Accepting())
    return;

  // see if user has just vanished
  user = rpt->User();
  uid = (rwi->s3).NodeID((void *) user);
  if ((uid0 <= 0) || (uid > 0))
    return;

  // generate event
  rpt->StartNote();
  rpt->NewProp(user, "hq", "visible", 1);
//  rpt->NewProp(user, "hq", "gone");
  rpt->FinishNote();
}


//= Add some property to the semantic node for a person.

void jhcSocial::person_prop (int i, const char *prop, int neg) const
{
  jhcBodyData *p = (rwi->s3).RefPerson(i);
  jhcAliaDesc *n = (jhcAliaDesc *) p->node;
  const char *name = (rwi->fn).FaceName(i);

  // if needed, find or make semantic node for person
  rpt->StartNote();
  if (n == NULL)
  {
    if ((n = rpt->Person(name)) == NULL)
      n = rpt->NewNode("dude", NULL, 0, -1.0);
    p->node = (void *) n;
  }

  // generate main message to core reasoner
  rpt->NewProp(n, "hq", prop, neg);
  add_name(n, name);
  rpt->FinishNote();
}


//= Add both parts of face recognition name to some user node.
// takes index of person (not ID)

void jhcSocial::add_name (jhcAliaDesc *n, const char *name) const
{
  char first[80];
  const jhcAliaDesc *kind;
  char *end;
  int i = 0;

  // add personhood if missing
  if (n == NULL) 
    return;
  while ((kind = n->Fact("ako", i++)) != NULL)
    if (kind->HasWord("person"))
      break;
  if (kind == NULL)
    rpt->NewProp(n, "ako", "person");

  // possibly add full name ("Jon Connell")
  if ((name == NULL) || (*name == '\0'))
    return;
  if (!n->HasWord(name))
    rpt->NewLex(n, name);

  // possibly add first name ("Jon")
  strcpy_s(first, name);
  if ((end = strchr(first, ' ')) != NULL)
    *end = '\0';
  if (!n->HasWord(first))
    rpt->NewLex(n, first);
}


///////////////////////////////////////////////////////////////////////////
//                       Looking At/For People                           //
///////////////////////////////////////////////////////////////////////////

//= Start aiming camera toward a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_look_set (const jhcAliaDesc *desc, int i)
{
  if ((cst[i] = get_dude(desc->Val("arg"))) <= 0)
    return -1;
  ct0[i] += ROUND(1000.0 * lquit);
  return 1;
}


//= Continue aiming camera toward a person until close enough.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_look_chk (const jhcAliaDesc *desc, int i)
{
  // lock to sensor cycle 
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;

  // see if timeout then check if person is still there or gaze achieved
  if (jms_diff(jms_now(), ct0[i]) > 0)
    return -1;
  if ((rwi->s3).GetID(cst[i]) == NULL)
    return -1;
  if (rwi->PersonErr(cst[i]) <= ltol)
    return 1;

  // re-issue basic command (drive forward if orientation okay)
  jprintf(1, dbg, ">> REQUEST %d: look at person %d\n", cbid[i], cst[i]);
  rwi->WatchPerson(cst[i], cbid[i]);
  return 0;
}


//= Find ID of person to track based on semantic node.
// returns 0 if no good track found, -1 for rwi unbound

int jhcSocial::get_dude (const jhcAliaDesc *agt) const
{
  if (agt == NULL) 
    return 0;
  if (rwi == NULL)
    return -1;
  return (rwi->s3).NodeID(agt);
}


///////////////////////////////////////////////////////////////////////////
//                        Moving Toward People                           //
///////////////////////////////////////////////////////////////////////////

//= Start going toward a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_approach_set (const jhcAliaDesc *desc, int i)
{
  if ((cst[i] = get_dude(desc->Val("arg"))) <= 0)
    return -1;
//  ct0[i] += ROUND(1000.0 * aquit);
ct0[i] += ROUND(10000.0 * aquit);
  return 1;
}


//= Continue approaching a person until close enough.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_approach_chk (const jhcAliaDesc *desc, int i)
{
  jhcEliBase *b;
  jhcStare3D *s3;
  const jhcMatrix *targ;
  double dist, ang, goal = cozy - 2.0;
  
  // lock to sensor cycle 
  if ((rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;
  b = rwi->base;
  s3 = &(rwi->s3);

  // see if timeout then check if person is still there 
  if (jms_diff(jms_now(), ct0[i]) > 0)
    return -1;
  if ((targ = s3->GetID(cst[i])) == NULL)
    return -1;

  // get heading and distance to target, check if done
  ang = targ->PanVec3() - 90.0;
  if ((dist = targ->PlaneVec3()) <= cozy)
    return 1;

  // re-issue basic command (drive forward if orientation okay)
  jprintf(1, dbg, ">> REQUEST %d: approach person %d\n", cbid[i], cst[i]);
  rwi->WatchPerson(cst[i], cbid[i]);
  rwi->ServoPolar(dist - goal, ang, 1.0, cbid[i]);

/*
double trav, head;
jprintf("TARGET: %3.1f in offset [%3.1f], %3.1f degs\n", dist, goal, ang);
(rwi->nav).Swerve(trav, head, dist - goal, ang);
b->TurnFix(head, ttime, 1.0, cbid[i] + 1);
if (fabs(head) < orient)
  b->MoveFix(trav, atime, 1.0, cbid[i]);   // slower than follow
*/

//  b->TurnFix(ang, ttime, 1.0, cbid[i]);
//  if (fabs(ang) < orient)
//    b->MoveFix(dist - goal, atime, 1.0, cbid[i]);   // slower than follow
  return 0;
}


//= Start following a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_follow_set (const jhcAliaDesc *desc, int i)
{
  if ((cst[i] = get_dude(desc->Val("arg"))) <= 0)
    return -1;
  return 1;
}


//= Continue following a person and complain if too far.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_follow_chk (const jhcAliaDesc *desc, int i)
{
  jhcEliBase *b;
  jhcStare3D *s3;
  const jhcMatrix *targ;
  jhcAliaDesc *n;
  double dist, ang;
  
  // lock to sensor cycle 
  if ((rpt == NULL) || (rwi == NULL) || (rwi->body == NULL))
    return -1;
  if (!rwi->Accepting())
    return 0;
  b = rwi->base;
  s3 = &(rwi->s3);

  // check if person is still there 
  if ((targ = s3->GetID(cst[i])) == NULL)
    return -1;

  // get heading and distance to target, complain if too far
  ang = targ->PanVec3() - 90.0;
  if ((dist = targ->PlaneVec3()) > worry)
    if ((n = (jhcAliaDesc *) s3->GetNode(cst[i])) != NULL)
    {
      rpt->StartNote();
      rpt->NewProp(n, "hq", "far away");
      rpt->FinishNote();
    }

  // re-issue basic command (drive forward if orientation okay)
  jprintf(1, dbg, ">> REQUEST %d: follow person %d\n", cbid[i], cst[i]);
  rwi->WatchPerson(cst[i], cbid[i]);
  rwi->ServoPolar(dist - ideal, ang, 1.5, cbid[i]);

//  b->TurnFix(ang, ttime, 1.5, cbid[i]);
//  if (fabs(ang) < orient)
//    b->MoveFix(dist - ideal, ftime, 1.5, cbid[i]);   // quicker than approach
  return 0;
}


