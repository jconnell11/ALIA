// jhcSocial.cpp : interface to ELI people tracking kernel for ALIA system
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
  ver = 1.55;
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
  ps->NextSpecF( &pnear,  26.0, "Person near alert (in)");
  ps->NextSpecF( &alone,   2.0, "Person alert reset (sec)");
  ps->NextSpecF( &scare,  18.0, "Person very near (in)");
  ps->Skip();
  ps->NextSpecF( &ltol,    2.0, "Look achieved (deg)");
  ps->NextSpecF( &lquit,   2.0, "Look timeout (sec)");
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
  ps->NextSpecF( &cozy,   28.0, "Approach front gap (in)");
  ps->NextSpecF( &direct, 20.0, "Approach until within (deg)");
  ps->NextSpecF( &aquit,  10.0, "Timeout for approach (sec)");
  ps->NextSpecF( &ideal,  28.0, "Following distance (in)");
  ps->NextSpecF( &worry,  48.0, "Too far distance (in)");
  ps->NextSpecF( &orient, 60.0, "Rotate until aligned (deg)");

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
  folks = 0;
  seen = 0;
  pal = 0;
  prox = 0;
  reco = 0;
  uid = 0;
}


//= Post any spontaneous observations to attention queue.

void jhcSocial::local_volunteer ()
{
  dude_seen();   
  dude_close(); 
  vip_seen();   
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
  JCMD_SET(soc_retreat);
  JCMD_SET(soc_follow);
  JCMD_SET(soc_explore);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcSocial::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(soc_look);
  JCMD_CHK(soc_approach);
  JCMD_CHK(soc_retreat);
  JCMD_CHK(soc_follow);
  JCMD_CHK(soc_explore);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                           Reported Events                             //
///////////////////////////////////////////////////////////////////////////

//= Note that at least one person is present.
// does not trigger for all new arrivals, just first one after absence
// states: "X is visible"

void jhcSocial::dude_seen ()
{
  UL32 prev = seen;
  int t;

  // see if now people when there were not before
  if ((rpt == NULL) || (rwi == NULL) || !rwi->Accepting())
    return;
//  t = (rwi->s3).Closest((rwi->nav).rfwd);
  t = rwi->ClosestFace((rwi->nav).rfwd);

  // generate event telling of presence
  folks = 0;                                     // assume alone currently
  if (t < 0) 
    return;
  folks = 1;
  seen = jms_now();
  if ((prev == 0) || (jms_secs(seen, prev) > alone))
  {
    rpt->StartNote();
    agt_node(t);
    rpt->FinishNote();
  }
}


//= Generate an event whenever any person gets inside the robot's personal space.
// pal is positive if "close" already signalled, prox is positive if "very close" signalled
// states: "X is close" where X is a person and perhaps has a name

void jhcSocial::dude_close ()
{
  double front = (rwi->nav).rfwd, gone = 1.5 * pnear, dist = gone;
  int t, id = 0;

  // find distance of closest person to front of robot (if any)
  if ((rpt == NULL) || (rwi == NULL) || !rwi->Accepting())
    return;
//  if ((t = (rwi->s3).Closest(front)) >= 0)
  if ((t = rwi->ClosestFace(front)) >= 0)
  {
    id = (rwi->s3).PersonID(t);
    dist = rwi->FrontDist((rwi->s3).RefPerson(t));   // defaults to gone
  } 

  // possibly generate event telling newly achieved degree of proximity
  rpt->StartNote();
  if ((dist < scare) && ((prox <= 0) || (id != pal)))
    rpt->NewDeg(agt_node(t), "hq", "close", "very");
  else if ((dist < pnear) && (id != pal))
    rpt->NewProp(agt_node(t), "hq", "close");
  rpt->FinishNote();

  // adjust hysteretic signalling states
  if (dist < scare)
    prox = 1;                // "very" reported
  if (dist >= pnear)
    prox = 0;                // allow new "very" 
  if (dist < pnear)
    pal = id;                // "close" reported
  if (dist >= gone)
    pal = 0;                 // allow new "close"
}


//= Inject NOTE saying a particular person's face has just been recognized.
// states: "X is Y"

void jhcSocial::vip_seen ()
{
  int t, prev = reco;

  if ((rpt == NULL) || (rwi == NULL) || !rwi->Accepting())
    return;
  if ((t = (rwi->fn).JustNamed()) >= 0)
    if ((reco = (rwi->s3).PersonID(t)) != prev)
    {
      rpt->StartNote();
      agt_node(t);
      rpt->FinishNote();
    }
}


//= Generate an event whenever user had been seen but now seems to have vanished.
// states: "X is not visible" where X is the current user 

void jhcSocial::user_gone ()
{
  jhcAliaDesc *user;
  int uid0 = uid;

  // find face ID associated with user (if any) 
  if (rpt == NULL) 
    return;
  user = rpt->User();
  uid = rpt->VisID(user, 1);
  if ((rwi->s3).TrackIndex(uid) < 0)   
    uid = 0;                           // no longer tracked

  // generate event if user has just vanished
  if ((uid <= 0) && (uid0 > 0))
  {
    rpt->StartNote();
    rpt->NewProp(user, "hq", "visible", 1, 1.0, 1);
    rpt->FinishNote();
  }
}


//= See if node already assigned to person, else create new one.
// should be called inside rpt->StartNote to include "ako person" fact

jhcAliaDesc *jhcSocial::agt_node (int t)
{
  char first[80];
  jhcBodyData *p = (rwi->s3).RefPerson(t);
  jhcAliaDesc *agt = rpt->NodeFor(p->id, 1);
  const char *name = (rwi->fn).FaceName(t);
  char *end;

  // make up new node with basic properties
  if (agt == NULL)
  {
    agt = rpt->NewNode("dude");
    rpt->NewProp(agt, "ako", "person");
    rpt->NewProp(agt, "hq", "visible");
    rpt->VisAssoc(p->id, agt, 1);
    if (p->tag[0] == '\0')
      strcpy_s(p->tag, agt->Nick());
  }

  // possibly add name if face is recognized
  if ((name != NULL) && (*name != '\0'))
  {
    rpt->NewProp(agt, "name", name, 0, 1.0, 1);
    strcpy_s(first, name);
    if ((end = strchr(first, ' ')) != NULL)
      *end = '\0';
    rpt->NewProp(agt, "name", first, 0, 1.0, 1);
  }

  // make eligible for FIND
  rpt->NewFound(agt);      
  p->state = 1;                        // drawable
  return agt;
}


///////////////////////////////////////////////////////////////////////////
//                       Looking At/For People                           //
///////////////////////////////////////////////////////////////////////////

//= Start aiming camera toward a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_look0 (const jhcAliaDesc *desc, int i)
{
  if ((cst[i] = rpt->VisID(desc->Val("arg"), 1)) <= 0)
    return -1;
  ct0[i] = jms_now() + ROUND(1000.0 * lquit);
  return 1;
}


//= Continue aiming camera toward a person until close enough.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_look (const jhcAliaDesc *desc, int i)
{
  // lock to sensor cycle 
  if ((rwi == NULL) || rwi->Ghost())
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
  jprintf(1, dbg, "|- Social %d: look at person %d\n", cbid[i], cst[i]);
  rwi->WatchPerson(cst[i], cbid[i]);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                      Moving Relative to People                        //
///////////////////////////////////////////////////////////////////////////

//= Start going toward a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_approach0 (const jhcAliaDesc *desc, int i)
{
  if ((cst[i] = rpt->VisID(desc->Val("arg"), 1)) <= 0)
    return -1;
  ct0[i] = jms_now() + ROUND(10000.0 * aquit);
  return 1;
}


//= Continue approaching a person until close enough.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_approach (const jhcAliaDesc *desc, int i)
{
  const jhcMatrix *targ;
  double xs, td, ta, off, dtol = 2.0;

  // lock to sensor cycle 
  if ((rwi == NULL) || rwi->Ghost())
    return -1;
  if (!rwi->Accepting())
    return 0;

  // see if timeout then check if person is still there 
  if ((xs = jms_elapsed(ct0[i])) > 0.0)
  {
    jprintf("{ soc_approach: timeout %3.1f secs [%3.1f] }\n", aquit + xs, aquit);
    return -1;
  }
  if ((targ = (rwi->s3).GetID(cst[i])) == NULL)
  {
    jprintf("{ soc_approach: lost target %s }\n", (desc->Val("arg"))->Nick()); 
    return -1;
  }

  // get heading and distance to target, check if done
  td = targ->PlaneVec3();
  ta = targ->PanVec3() - 90.0;
  off = rwi->FrontDist(td, ta);
  if (fabs(off - cozy) <= dtol)
{
jprintf("soc_approach: off = %3.1f (vs %3.1f), ang = %3.1f (vs %3.1f)\n", off, cozy, ta, direct);
    return 1;
}

  // re-issue basic command (drive forward if orientation okay)
  jprintf(1, dbg, "|- Social %d: approach person %d\n", cbid[i], cst[i]);
  rwi->WatchPerson(cst[i], cbid[i]);
  rwi->MapPath(cbid[i]);
  rwi->ServoPolar(td, ta, cozy, 1.0, cbid[i]);
  return 0;
}


//= Start going away from a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_retreat0 (const jhcAliaDesc *desc, int i)
{
  if ((cst[i] = rpt->VisID(desc->Val("arg"), 1)) <= 0)
    return -1;
  ct0[i] = jms_now() + ROUND(10000.0 * aquit);
  return 1;
}


//= Continue backing away from a person until far enough.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_retreat (const jhcAliaDesc *desc, int i)
{
  const jhcMatrix *targ;
  double xs, td, ta, off, safe = 1.2 * cozy, dtol = 2.0;

  // lock to sensor cycle 
  if ((rwi == NULL) || rwi->Ghost())
    return -1;
  if (!rwi->Accepting())
    return 0;

  // see if timeout then check if person is still there 
  if ((xs = jms_elapsed(ct0[i])) > 0.0)
  {
    jprintf("{ soc_retreat: timeout %3.1f secs [%3.1f] }\n", aquit + xs, aquit);
    return -1;
  }
  if ((targ = (rwi->s3).GetID(cst[i])) == NULL)
  {
    jprintf("{ soc_retreat: lost target %s }\n", (desc->Val("arg"))->Nick()); 
    return -1;
  }

  // get heading and distance to target, check if done
  td = targ->PlaneVec3();
  ta = targ->PanVec3() - 90.0;
  off = rwi->FrontDist(td, ta);
  if ((fabs(off - safe) <= dtol) && (fabs(ta) <= direct))
    return 1;

  // re-issue basic command (drive forward if orientation okay)
  jprintf(1, dbg, "|- Social %d: retreat from person %d\n", cbid[i], cst[i]);
  rwi->WatchPerson(cst[i], cbid[i]);
  rwi->MapPath(cbid[i]);
  rwi->ServoPolar(td, ta, safe, 1.0, cbid[i]);
  return 0;
}


//= Start following a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_follow0 (const jhcAliaDesc *desc, int i)
{
  if ((cst[i] = rpt->VisID(desc->Val("arg"), 1)) <= 0)
    return -1;
  return 1;
}


//= Continue following a person and complain if too far.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_follow (const jhcAliaDesc *desc, int i)
{
  jhcAliaDesc *agt = desc->Val("arg");
  const jhcMatrix *targ;
  double td, ta;
  
  // lock to sensor cycle 
  if ((rpt == NULL) || (rwi == NULL) || rwi->Ghost())
    return -1;
  if (!rwi->Accepting())
    return 0;

  // check if person is still there 
  if ((targ = (rwi->s3).GetID(cst[i])) == NULL)
  {
    jprintf("{ soc_follow: lost target %s }\n", (desc->Val("arg"))->Nick()); 
    return -1;
  }

  // get heading and distance to target, complain if too far
  ta = targ->PanVec3() - 90.0;
  if ((td = rwi->FrontDist(targ)) > worry)
  {
    rpt->StartNote();
    rpt->NewProp(agt, "hq", "far away");
    rpt->FinishNote();
  }

  // re-issue basic command (drive forward if orientation okay)
  jprintf(1, dbg, "|- Social %d: follow person %d\n", cbid[i], cst[i]);
  rwi->WatchPerson(cst[i], cbid[i]);
  rwi->MapPath(cbid[i]);
  rwi->ServoPolar(td, ta, ideal, 1.5, cbid[i]);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                          Explore Environment                          //
///////////////////////////////////////////////////////////////////////////

//= Start wandering aimlessly for a while.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_explore0 (const jhcAliaDesc *desc, int i)
{
  double wtime = 60.0;                 // timeout in secs

  ct0[i] = jms_now() + ROUND(1000.0 * wtime);
  return 1;
}


//= Continue wandering aimlessly for a while. 
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_explore (const jhcAliaDesc *desc, int i)
{
  double xs, wtime = 120.0;

  // check for timeout then lock to sensor cycle 
  if ((rwi == NULL) || rwi->Ghost())
    return -1;
  if ((xs = jms_elapsed(ct0[i])) > 0.0)
  {
    jprintf("{ soc_wander: timeout %3.1f secs [%3.1f] }\n", wtime + xs, wtime);
    return 1;
  }
  if (!rwi->Accepting())
    return 0;

  // go forward as long as obstacles fairly far away
  rwi->MapPath(cbid[i]);
  rwi->Explore(0.5, cbid[i]);
  return 0;
}
