// jhcSocial.cpp : interface to ELI people tracking kernel for ALIA system
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

#include <math.h>
#include <string.h>

#include "Reasoning/jhcActionTree.h"   // common audio
#include "Semantic/jhcNetNode.h"

#include "Interface/jms_x.h"           // common video

#include "Grounding/jhcSocial.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSocial::~jhcSocial ()
{
  delete [] cpos;
}


//= Default constructor initializes certain values.

jhcSocial::jhcSocial ()
{
  int i, n = MaxInst();

  // pool identification
  strcpy_s(tag, "Social");

  // create instance control variables
  cpos = new jhcMatrix [n];
  for (i = 0; i < n; i++)
    cpos[i].SetSize(4);    

  // body and connection 
  rwi = NULL;
  rpt = NULL;

  // processing parameters
  Defaults();
  dbg = 2;
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


//= Parameters for orienting on talking heads.

int jhcSocial::snd_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("soc_snd", 0);
  ps->NextSpec4( &recent, 60,   "Max speech lag (cyc)");
  ps->NextSpecF( &pdist,  36.0, "Close person offset (in)");
  ps->NextSpecF( &rtime,   1.5, "Rise time for tall (sec)");
  ps->NextSpecF( &sdev,   15.0, "Face sound offset (deg)");
  ps->NextSpecF( &aimed,   2.0, "Gaze final offset (deg)");
  ps->NextSpecF( &gtime,   0.3, "Gaze response (sec)"); 

  ps->NextSpecF( &side,   30.0, "Body rotate thresh (deg)");    // 0 = don't 
  ps->NextSpecF( &btime,   1.5, "Rotate response (sec)");      
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
  ok &= snd_params(fname);
  ok &= move_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcSocial::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= aps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= mps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Attach physical enhanced body and make pointers to some pieces.

void jhcSocial::local_platform (void *soma) 
{
  rwi = (jhcEliGrok *) soma;
  neck = rwi->neck;
}


//= Set up for new run of system.

void jhcSocial::local_reset (jhcAliaNote& top)
{
  rpt = &top;
  folks = 0;
  seen = 0;
  pal = 0;
  prox = 0;
  reco = 0;
  uid = 0;
}


//= Post any spontaneous observations to attention queue.
// expected position of heads already adjusted by jhcEliGrok::adjust_heads()
// "visible" = currently tracked, not necessarily in field of view

void jhcSocial::local_volunteer ()
{
  if ((rpt == NULL) || (rwi == NULL) || !rwi->Accepting())
    return;
  vip_seen();   
  head_talk();
  dude_seen();   
  dude_close(); 
  lost_dudes();
  wmem_heads();
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcSocial::local_start (const jhcAliaDesc& desc, int i)
{
  JCMD_SET(soc_talk);
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

int jhcSocial::local_status (const jhcAliaDesc& desc, int i)
{
  JCMD_CHK(soc_talk);
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

//= Inject NOTE saying a particular person's face has just been recognized.
// states: "X is Y"

void jhcSocial::vip_seen ()
{
  jhcAliaDesc *agt;
  int t, prev = reco;

  // see if newly recognized person is different from last one
  if ((t = (rwi->fn).JustNamed()) >= 0)
    if ((reco = (rwi->s3).PersonID(t)) != prev)
    {
      // find wmem node compatible with name or make new one
      if ((agt = rpt->Person((rwi->fn).FaceName(t))) == NULL)
        if ((agt = rpt->NodeFor((rwi->s3).PersonID(t), 1)) == NULL)
          agt = rpt->NewObj("dude");
      link_track(agt, t);
 
      // generate note about name
      rpt->StartNote();
      std_props(agt, t);
      rpt->FinishNote();
    }
}


//= See if valid sentence just came from some tracked head.

void jhcSocial::head_talk ()
{
  jhcActionTree *atree = dynamic_cast<jhcActionTree *>(rpt);
  jhcNetNode *dude, *user = atree->Human();
  const char *name;
  int sp, t, id;

  // if recent speaker find related head ID and semantic network node
  if (rwi->SpeechRC() != 2)
    return;
  if ((sp = (rwi->tk).Speaking()) <= 0)    
    return;
  dude = atree->ExtRef(sp, 1);
jprintf("head_talk: id = %d -> %s\n", sp, ((dude == NULL) ? "NULL" : dude->Nick()));
  if (dude == user)                              // already correct
    return;

  // see if user needs to be changed
  t = (rwi->s3).TrackIndex(sp);
  name = (rwi->fn).FaceName(t);
  id = atree->ExtRef(user, 1);
  if (((id > 0) && (id != sp)) || 
      ((id <= 0) && atree->NameClash(user, name)))
    atree->SetUser(dude);                        // already has track and name
  else
  {
    // keep user but steal track ID (unlinks dude node if any)
    link_track(user, t);
    rpt->StartNote();
    atree->AddProp(dude, "hq", "visible", 1);    // ignored if dude = NULL
    rpt->FinishNote();

    // copy over name from speaker and add some basic fatures
    rpt->StartNote();
    atree->AddName(user, name);
    atree->AddProp(user, "ako", "person", 0, 1.0, 1);
    atree->AddProp(user, "hq", "visible", 0, 1.0, 1);
    rpt->FinishNote();
  }
}


//= Note that at least one person is present.
// does not trigger for all new arrivals, just first one after absence
// states: "X is visible"

void jhcSocial::dude_seen ()
{
  jhcAliaDesc *agt;
  UL32 prev = seen;
  int t;

  // see if now people when there were not before
  t = rwi->ClosestFace((rwi->nav).Nose());
  folks = 0;                                     // assume alone currently
  if (t < 0) 
    return;
  folks = 1;
  seen = jms_now();

  // only announce occasionally
  if ((prev == 0) || (jms_secs(seen, prev) > alone))
  {
    // generally make new node for person        
    if ((agt = rpt->NodeFor((rwi->s3).PersonID(t), 1)) == NULL)
      agt = rpt->NewObj("dude");
    link_track(agt, t);

    // generate event telling of presence
    rpt->StartNote();
    std_props(agt, t);
    rpt->FinishNote();
  }
}


//= Generate an event whenever any person gets inside the robot's personal space.
// pal is positive if "close" already signalled, prox is positive if "very close" signalled
// states: "X is close" where X is a person and perhaps has a name

void jhcSocial::dude_close ()
{
  jhcAliaDesc *agt;
  double dist, front = (rwi->nav).Nose(), gone = 1.5 * pnear;
  int t, id, close, very;

  // see if there is some person in view
  if ((t = rwi->ClosestFace(front)) < 0)
  { 
    prox = 0;
    pal = 0;
    return;
  }

  // find distance of closest person to front of robot
  id = (rwi->s3).PersonID(t);
  dist = rwi->FrontDist((rwi->s3).RefPerson(t));  
  close = (((dist < pnear) && (id != pal)) ? 1 : 0);
  very  = (((dist < scare) && ((prox <= 0) || (id != pal))) ? 1 : 0);

  // if something newly close or very close
  if ((close > 0) || (very > 0))
  {
    // generally make new node for person        
    if ((agt = rpt->NodeFor((rwi->s3).PersonID(t), 1)) == NULL)
      agt = rpt->NewObj("dude");
    link_track(agt, t);

    // generate event telling newly achieved degree of proximity
    rpt->StartNote();
    if (very > 0) 
      rpt->NewDeg(agt, "hq", "close", "very");
    else 
      rpt->NewProp(agt, "hq", "close");
    std_props(agt, t);
    rpt->FinishNote();
  }

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


//= Monitor nodified people to check if they have vanished.
// states: "X is not visible" where X is the person
// actual tracking loss determined by jhcGrok::adjust_heads

void jhcSocial::lost_dudes ()
{
  int id = 0;

  while ((id = rpt->VisEnum(id, 1)) > 0)
    if ((rwi->s3).TrackIndex(id) < 0)   
    {
      rpt->StartNote();
      rpt->NewProp(rpt->NodeFor(id, 1), "hq", "visible", 1);
      rpt->FinishNote();
      rpt->VisAssoc(id, NULL, 1);                
    }
}


//= Make sure each visible head has a tag consistent with any associated node.
// overwrite blank or node-based label, does not change full name from face reco 

void jhcSocial::wmem_heads ()
{
  jhcBodyData *p;
  jhcAliaDesc *agt;
  const char *name;
  int i, nlim = (rwi->s3).PersonLim();

  for (i = 0; i < nlim; i++)
    if ((rwi->s3).PersonOK(i))
    {
      p = (rwi->s3).RefPerson(i);
      if ((agt = rpt->NodeFor(p->id, 1)) == NULL)
      {
        p->state = 0;                            // do not draw
        if (strchr(p->tag, '-') != NULL)         // node nickname
          p->tag[0] = '\0';
      }                                   
      else if ((name = rpt->Name(agt)) != NULL)
        if ((p->tag[0] == '\0') || (strcmp(p->tag, agt->Nick()) == 0))
          if (strcmp(p->tag, name) != 0)
            strcpy_s(p->tag, name);              // net-derived name
    }
}


///////////////////////////////////////////////////////////////////////////
//                        Looking For Speaker                            //
///////////////////////////////////////////////////////////////////////////

//= Start aiming camera toward most recent sound source.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_talk0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  return 1;
}


//= Continue aiming camera toward most recent speaker.
// sets up continuing request to body if not finished
// sets caux[] to sound direction, cpos[] to gaze target
// cst[i]: 0 = initialize direction
//         1 = look at head associated with sound
//         2 = look toward low head guess spot
//         3 = raise gaze toward high head guess
//         4 = return to level forward gaze
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_talk (const jhcAliaDesc& desc, int i)
{
  double ht, rads, pan, tilt, gerr, berr;
  int t;

  // lock to sensor cycle 
  if (rwi->Ghost())
    return 1;
  if (!rwi->Accepting())
    return 0;
  if ((rwi->body)->CommOK() <= 0)
    return err_body();
  ht = (rwi->lift)->Height();

  // possibly announce entry and set likely lowest head position
  if (cst[i] <= 0)
  {
    if ((rwi->mic)->VoiceStale() > recent)
      return -1;
    caux[i] = (rwi->mic)->VoiceDir();
    rads = D2R * (caux[i] + 90.0);
    cpos[i].SetVec3(pdist * cos(rads), pdist * sin(rads), (rwi->s3).h0);
    cst[i] = 2;
  }
  else 
    (rwi->base)->AdjustAng(caux[i]);   
    
  // always check for head aligned with sound direction (speaker = user)
  if ((t = rwi->HeadAlong(cpos[i], caux[i], sdev)) >= 0)
  {
    rpt->VisAssoc((rwi->s3).PersonID(t), rpt->User(), 1);  
    if (cst[i] > 1)
      ct0[i] = 0;
    cst[i] = 1;
  }
  else
    (rwi->base)->AdjustTarget(cpos[i]);   

  // look directly at chosen spot (head or guess)
  if (cst[i] <= 2)
  {
    neck->AimFor(pan, tilt, cpos[i], ht);
    if (ct0[i] == 0)
      jprintf(2, dbg, "|- Social %d: slew to %s at %d degs\n", 
              cbid[i], ((cst[i] == 1) ? "head" : "sound"), ROUND(pan));
    gerr = neck->GazeErr(pan, tilt);
    berr = ((side <= 0.0) ? 0.0 : fabs(pan) - side);
    if ((gerr > aimed) || (berr > 0.0))
    {
      if (chk_neck(i, gerr + berr) > 0)
        return -1;
      neck->GazeFix(pan, tilt, gtime, cbid[i]);
      if (berr > 0.0)
        (rwi->base)->TurnFix(((pan >= 0.0) ? side : -side), btime, 1.5, cbid[i]);  
      return 0;
    }
    if (cst[i] == 1)
      return 1;
    cpos[i].SetZ((rwi->s3).h1);
    ct0[i] = 0;
    cst[i] = 3;
    jprintf(2, dbg, "|- Social %d: rise for head\n", cbid[i]);
  }

  // raise gaze slowly toward highest head position
  if (cst[i] == 3)
  {
    if ((gerr = neck->GazeErr(cpos[i], ht)) > aimed)
    {
      if (chk_neck(i, gerr) > 0)
        return -1;
      neck->GazeFix(cpos[i], ht, rtime, cbid[i]);
      return 0;
    }
    ct0[i] = 0;
    cst[i] = 4;
    jprintf(2, dbg, "|- Social %d: neutral gaze\n", cbid[i]);
  }

  // give up on person and set default gaze
  if ((gerr = neck->GazeErr(0.0, 0.0)) > aimed)
  {
    if (chk_neck(i, gerr) > 0)
      return -1;
    neck->GazeFix(0.0, 0.0, rtime, cbid[i]);
    return 0;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                     Orienting Toward People                           //
///////////////////////////////////////////////////////////////////////////

//= Start aiming camera toward a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_look0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc.Val("arg")) == NULL)
    return -1;
  ct0[i] = jms_now() + ROUND(1000.0 * lquit);
  return 1;
}


//= Continue aiming camera toward a person until close enough.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_look (const jhcAliaDesc& desc, int i)
{
  int id;

  // lock to sensor cycle 
  if ((id = rpt->VisID(cobj[i], 1)) <= 0)
    return err_person(cobj[i]);
  if (rwi->Ghost())
    return 1;
  if (!rwi->Accepting())
    return 0;
  if ((rwi->body)->CommOK() <= 0)
    return err_body();

  // see if timeout then check if person is still there or gaze achieved
  if (jms_diff(jms_now(), ct0[i]) > 0)
    return -1;
  if ((rwi->s3).GetID(id) == NULL)
    return -1;
  if (rwi->PersonErr(id) <= ltol)
    return 1;

  // re-issue basic command (drive forward if orientation okay)
  if (cst[i] <= 0)
  {
    jprintf(2, dbg, "|- Social %d: look at person %s\n", cbid[i], cobj[i]->Nick());
    cst[i] = 1;
  }
  rwi->WatchPerson(id, cbid[i]);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                      Moving Relative to People                        //
///////////////////////////////////////////////////////////////////////////

//= Start going toward a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_approach0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc.Val("arg")) == NULL)
    return -1;
  ct0[i] = jms_now() + ROUND(10000.0 * aquit);
  return 1;
}


//= Continue approaching a person until close enough.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_approach (const jhcAliaDesc& desc, int i)
{
  const jhcMatrix *targ;
  double xs, td, ta, off, dtol = 2.0;
  int id;

  // lock to sensor cycle 
  if ((id = rpt->VisID(cobj[i], 1)) <= 0)
    return err_person(cobj[i]);
  if (rwi->Ghost())
    return 1;
  if (!rwi->Accepting())
    return 0;
  if ((rwi->body)->CommOK() <= 0)
    return err_body();

  // see if timeout then check if person is still there 
  if ((xs = jms_elapsed(ct0[i])) > 0.0)
  {
    jprintf(1, dbg, " { soc_approach: timeout %3.1f secs [%3.1f] }\n", aquit + xs, aquit);
    return -1;
  }
  if ((targ = (rwi->s3).GetID(id)) == NULL)
  {
    jprintf(1, dbg, " { soc_approach: lost person %s }\n", cobj[i]->Nick()); 
    return -1;
  }

  // get heading and distance to target
  td = targ->PlaneVec3();
  ta = targ->PanVec3() - 90.0;

  // re-issue basic command (drive forward if orientation okay)
  if (cst[i] <= 0)
  {
    jprintf(2, dbg, "|- Social %d: approach person %s\n", cbid[i], cobj[i]->Nick()); 
    cst[i] = 1;
  }
  rwi->WatchPerson(id, cbid[i]);
  rwi->MapPath(cbid[i]);
  rwi->ServoPolar(td, ta, cozy, 1.0, cbid[i]);

  // see if close enough yet
  off = rwi->FrontDist(td, ta);
  jprintf(3, dbg, "  off = %3.1f\n", off);
  if (fabs(off - cozy) > dtol)
  {
    // check if not making progress 
    if (chk_base(i, off) <= 0)
      return 0;
    jprintf(2, dbg, "    stuck: off = %3.1f\n", off);
    return -1;
  }
  return 1;                                      // success
}


//= Start going away from a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_retreat0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc.Val("arg")) == NULL)
    return -1;
  ct0[i] = jms_now() + ROUND(10000.0 * aquit);
  return 1;
}


//= Continue backing away from a person until far enough.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_retreat (const jhcAliaDesc& desc, int i)
{
  const jhcMatrix *targ;
  double xs, td, ta, off, safe = 1.2 * cozy, dtol = 2.0;
  int id;

  // lock to sensor cycle 
  if ((id = rpt->VisID(cobj[i], 1)) <= 0)
    return err_person(cobj[i]);
  if (rwi->Ghost())
    return 1;
  if (!rwi->Accepting())
    return 0;
  if ((rwi->body)->CommOK() <= 0)
    return err_body();

  // see if timeout then check if person is still there 
  if ((xs = jms_elapsed(ct0[i])) > 0.0)
  {
    jprintf(1, dbg, " { soc_retreat: timeout %3.1f secs [%3.1f] }\n", aquit + xs, aquit);
    return -1;
  }
  if ((targ = (rwi->s3).GetID(id)) == NULL)
  {
    jprintf(1, dbg, " { soc_retreat: lost person %s }\n", cobj[i]->Nick()); 
    return -1;
  }

  // get heading and distance to target, check if done
  td = targ->PlaneVec3();
  ta = targ->PanVec3() - 90.0;
  off = rwi->FrontDist(td, ta);
  if ((fabs(off - safe) <= dtol) && (fabs(ta) <= direct))
    return 1;

  // re-issue basic command (drive forward if orientation okay)
  if (cst[i] <= 0)
  {
    jprintf(2, dbg, "|- Social %d: retreat from person %s\n", cbid[i], cobj[i]->Nick());
    cst[i] = 1;
  }
  rwi->WatchPerson(id, cbid[i]);
  rwi->MapPath(cbid[i]);
  rwi->ServoPolar(td, ta, safe, 1.0, cbid[i]);
  return 0;
}


//= Start following a person.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcSocial::soc_follow0 (const jhcAliaDesc& desc, int i)
{
  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  if ((cobj[i] = desc.Val("arg")) == NULL)
    return -1;
  return 1;
}


//= Continue following a person and complain if too far.
// sets up continuing request to body if not finished
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_follow (const jhcAliaDesc& desc, int i)
{
  jhcAliaDesc *agt = desc.Val("arg");
  const jhcMatrix *targ;
  double td, ta;
  int id;
  
  // lock to sensor cycle 
  if ((id = rpt->VisID(cobj[i], 1)) <= 0)
    return err_person(cobj[i]);
  if (rwi->Ghost())
    return 1;
  if (!rwi->Accepting())
    return 0;
  if ((rwi->body)->CommOK() <= 0)
    return err_body();

  // check if person is still there 
  if ((targ = (rwi->s3).GetID(id)) == NULL)
  {
    jprintf(1, dbg, " { soc_follow: lost person %s }\n", cobj[i]->Nick()); 
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
  if (cst[i] <= 0)
  {
    jprintf(2, dbg, "|- Social %d: follow person %s\n", cbid[i], cobj[i]->Nick());
    cst[i] = 1;
  }
  rwi->WatchPerson(id, cbid[i]);
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

int jhcSocial::soc_explore0 (const jhcAliaDesc& desc, int i)
{
  double wtime = 60.0;                 // timeout in secs

  if ((rwi == NULL) || (rpt == NULL))
    return -1;
  ct0[i] = jms_now() + ROUND(1000.0 * wtime);
  return 1;
}


//= Continue wandering aimlessly for a while. 
// returns 1 if done, 0 if still working, -1 for failure

int jhcSocial::soc_explore (const jhcAliaDesc& desc, int i)
{
  double xs, wtime = 120.0;

  // check for timeout then lock to sensor cycle 
  if ((xs = jms_elapsed(ct0[i])) > 0.0)
  {
    jprintf(1, dbg, " { soc_wander: timeout %3.1f secs [%3.1f] }\n", wtime + xs, wtime);
    return 1;
  }
  if (rwi->Ghost())
    return 1;
  if (!rwi->Accepting())
    return 0;
  if ((rwi->body)->CommOK() <= 0)
    return err_body();

  // go forward as long as obstacles fairly far away
  if (cst[i] <= 0)
  {
    jprintf(2, dbg, "|- Social %d: wander\n", cbid[i]);
    cst[i] = 1;
  }
  rwi->MapPath(cbid[i]);
  rwi->Explore(0.5, cbid[i]);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                             Utilities                                 //
///////////////////////////////////////////////////////////////////////////

//= Check for lack of substantial neck angle change over given time.
// ct0[i] holds timestamp of last cycle when progress was made
// returns 1 if at asymptote, 0 if still moving toward goal

int jhcSocial::chk_neck (int i, double err)
{
  double prog = 1.0, tim = 0.5;        // 0.1 deg about 15 cycles

  if ((ct0[i] == 0) || ((cerr[i] - err) >= prog))
  {
    ct0[i] = jms_now();
    cerr[i] = err;
  }
  else if (jms_elapsed(ct0[i]) > tim)
    return 1;
  return 0;
}


//= Check for lack of substantial base motion over given time.
// ct0[i] holds timestamp of previous cycle whether in saccade or not
// ccnt[i] holds total milliseconds when in control but no progress
// returns 1 if at asymptote, 0 if still moving toward goal

int jhcSocial::chk_base (int i, double err)
{
  double prog = 0.5;                   // 0.5" over about 30 cycles
  int tim = 1000;                      
  UL32 prev = ct0[i];
  
  // record cycle timestamp but ignore err if in saccade
  ct0[i] = jms_now();
  if (rwi->Survey())
    return 0;

  // possibly reset last error if enough progress made
  if ((prev == 0) || ((cerr[i] - err) >= prog))
  {
    cerr[i] = err;
    ccnt[i] = 0;
    return 0;
  }

  // increment amount of time since noticeable progress
  if (prev != 0) 
    ccnt[i] += (int)(ct0[i] - prev);
  if (ccnt[i] > tim) 
    return 1;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                          Semantic Messages                            //
///////////////////////////////////////////////////////////////////////////

//= Complain about the body not working.
// <pre>
//   NOTE[ act-1 -lex-  work
//               -neg-  1
//               -agt-> obj-1
//         ako-1 -lex-  body
//               -ako-> obj-1
//               -wrt-> self-1 ]
// </pre>
// always returns -1 for convenience

int jhcSocial::err_body ()
{
  jhcAliaDesc *part, *own, *arm, *fail;

  rpt->StartNote();
  part = rpt->NewObj("sys");
  own = rpt->NewProp(part, "ako", "body");
  rpt->AddArg(own, "wrt", rpt->Self());
  arm = rpt->Resolve(part);                      // find or make part
  fail = rpt->NewAct("work", 1);
  rpt->AddArg(fail, "agt", arm);                 // mark as not working
  rpt->FinishNote(fail);
  return -1;
}


//= Complain that person is not visible.
// <pre>
//   NOTE[ act-1 -lex-  see
//               -neg-  1
//               -agt-> self-1
//               -obj-> dude-1 ]
// </pre>
// always returns -1 for convenience

int jhcSocial::err_person (jhcAliaDesc *dude)
{
  jhcAliaDesc *fail;

  if (dude == NULL)
    return -1;
  rpt->StartNote();
  fail = rpt->NewAct("see", 1);
  rpt->AddArg(fail, "agt", rpt->Self());
  rpt->AddArg(fail, "obj", dude);
  rpt->FinishNote(fail);
  return -1;
}


//= Associate some agent node with a particular track index (not ID).

void jhcSocial::link_track (jhcAliaDesc *agt, int t)
{
  jhcBodyData *p = (rwi->s3).RefPerson(t);
  jhcAliaDesc *old = rpt->NodeFor(p->id, 1);
  
  // associate id for track with given node
  if (agt != old)
    rpt->NewProp(old, "hq", "visible", 1);
  rpt->VisAssoc(p->id, agt, 1);

  // mark as drawable with some label
  if (p->tag[0] == '\0')
    strcpy_s(p->tag, agt->Nick());
  p->state = 1;                       
  rpt->NewFound(agt);                 // eligible for FIND
}


//= Add names to node and, if newly created, personhood and visibility facts.
// NOTE: this should be called after StartNote

void jhcSocial::std_props (jhcAliaDesc *agt, int t)
{
  char first[80];
  const char *name = (rwi->fn).FaceName(t);
  char *end;

  // add full name and first name (if needed)
  if ((name != NULL) && (*name != '\0'))
  {
    rpt->NewProp(agt, "name", name, 0, 1.0, 1);
    strcpy_s(first, name);
    if ((end = strchr(first, ' ')) != NULL)
      *end = '\0';
    rpt->NewProp(agt, "name", first, 0, 1.0, 1);
  }

  // basic item category and state (for new nodes)
  rpt->NewProp(agt, "ako", "person", 0, 1.0, 1);
  rpt->NewProp(agt, "hq", "visible", 0, 1.0, 1);
}



