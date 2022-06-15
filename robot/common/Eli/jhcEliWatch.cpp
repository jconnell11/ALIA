// jhcEliWatch.cpp : post-processed sensors and innate behaviors for ELI robot
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

#include "Eli/jhcEliGrok.h"            // common robot - since only spec'd as class in header

#include "Eli/jhcEliWatch.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcEliWatch::~jhcEliWatch ()
{
}


//= Default constructor initializes certain values.
// creates member instances here so header file has fewer dependencies

jhcEliWatch::jhcEliWatch ()
{
  // information about watching behavior
  src.SetSize(4);
  wtarg[8][0] = '\0';
  strcpy_s(wtarg[7], "--  command hold");
  strcpy_s(wtarg[6], "--  speaker");
  strcpy_s(wtarg[5], "--  SOUND");
  strcpy_s(wtarg[4], "--  closest person");
  strcpy_s(wtarg[3], "--  eye contact");
  strcpy_s(wtarg[2], "--  recent face");
  strcpy_s(wtarg[1], "--  rise");
  strcpy_s(wtarg[0], "--  recenter");

  // processing parameters and state
  Defaults();
  Reset();
}


//= Generate a string telling what the robot is paying attention to.
// needs bid of winning command to robot neck

const char *jhcEliWatch::Behavior (int gwin, int dash) const
{
  int bid[8] = {align, rise, face, stare, close, sound, speak, freeze};
  int i, off = ((dash > 0) ? 0 : 4);

  if (gwin > 0)
    for (i = 0; i < 8; i++)
      if (gwin == bid[i])
        return(wtarg[i] + off);
  return wtarg[8];
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters controlling what sort of activities to watch.

int jhcEliWatch::watch_params (const char *fname)
{
  jhcParam *ps = &wps;
  int ok;

  ps->SetTag("watch_bid", 0);
  ps->NextSpec4( &freeze,   -1, "Freeze bid (neg = disable all)");   
  ps->NextSpec4( &sound,  2000, "Most recent sound bid");      // highest
  ps->NextSpec4( &speak,    25, "Current speaker bid");
  ps->NextSpec4( &close,    24, "Closest head bid");
  ps->NextSpec4( &stare,    23, "Most recent stare bid");
  ps->NextSpec4( &face,     22, "Most recent face bid");

  ps->NextSpec4( &rise,     21, "Head rise bid");   
  ps->NextSpec4( &align,    20, "Head center bid");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for picking targets to watch.

int jhcEliWatch::orient_params (const char *fname)
{
  jhcParam *ps = &ops;
  int ok;

  ps->SetTag("watch_vals", 0);
  ps->NextSpecF( &bored, 10.0, "Post-cmd freeze (sec)");       
  ps->NextSpecF( &edge,  10.0, "Sound trigger offset (deg)");  // was 30 then 25
  ps->NextSpecF( &hnear, 72.0, "Head distance thresh (in)");
  ps->NextSpec4( &fmin,   3,   "Min face detections");
  ps->NextSpecF( &pdist, 36.0, "Default person dist (in)");
  ps->NextSpecF( &hdec,  10.0, "Head rise decrement (in)");

  ps->NextSpecF( &aimed,  2.0, "Gaze done error (deg)");
  ps->NextSpecF( &rtime,  1.5, "Reorient response (sec)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcEliWatch::Defaults (const char *fname)
{
  int ok = 1;

  ok &= watch_params(fname);
  ok &= orient_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcEliWatch::SaveVals (const char *fname) 
{
  int ok = 1;

  ok &= wps.SaveVals(fname);
  ok &= ops.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Clear state for all innate behaviors.

void jhcEliWatch::Reset ()
{
  seek = 0;
  up = 0;
  mid = 0;
}


//= Run all applicable behaviors on most recent data and issue commands.

void jhcEliWatch::React (jhcEliGrok *g)
{
  if ((freeze < 0) || (g == NULL))
    return;
  cmd_freeze(g);
  watch_talker(g);
  gaze_sound(g);
  watch_closest(g);
  gaze_stare(g);
  gaze_face(g);
  head_rise(g);
  head_center(g);
}


///////////////////////////////////////////////////////////////////////////
//                            Innate Behaviors                           //
///////////////////////////////////////////////////////////////////////////

//= Freeze head and feet if recent conscious command issued.

void jhcEliWatch::cmd_freeze (jhcEliGrok *g)
{
  const jhcEliBody *bd = g->body;
  UL32 tnow = g->CmdTime();

  if (freeze <= 0)
    return;
  if (bd->NeckIdle(tnow) <= bored)      
    (g->neck)->ShiftTarget(0.0, 0.0, 0.0, freeze);   // lock in place
  if (bd->BaseIdle(tnow) <= bored)                 
    (g->base)->DriveTarget(0.0, 0.0, 1.0, freeze);   // active limp
}


//= Look at non-central sound source (if any) for a while.
// state machine controlled by "seek" and timer "swait"
// often given high bid priority to override everything else

void jhcEliWatch::gaze_sound (jhcEliGrok *g)
{
  const jhcDirMic *m = g->mic;
  const jhcEliNeck *n = g->neck;
  double ang, rads, ht = (g->lift)->Height();
  int old = 1;

  // see if behavior desired 
  if (sound <= 0) 
    return;

  // trigger when sound is far to either side wrt gaze direction
  if (m->VoiceStale() <= 0)
  {
    ang = m->VoiceDir();
    if (fabs(ang - n->Pan()) >= edge)
    {
      // remember location since sound is often short duration
      rads = D2R * (ang + 90.0);
      src.SetVec3(pdist * cos(rads), pdist * sin(rads), (g->s3).h0);
      old = 0;
      seek = 1;
    }
  }
  if (seek <= 0)
    return;

  // adjust for any base motion then aim at remembered location
  if (old > 0)
    (g->base)->AdjustTarget(src);   
  if (n->GazeErr(src, ht) > aimed)
    g->OrientToward(&src, sound);
  else
    seek = 0;
}


//= Continuously look at whomever is currently talking (if anyone).

void jhcEliWatch::watch_talker (jhcEliGrok *g)
{
  int sel;

  if (speak <= 0) 
    return;
  if ((sel = (g->tk).Speaking()) > 0)
    g->WatchPerson(sel, speak);
}


//= Track the most prominent head with a face. 

void jhcEliWatch::watch_closest (jhcEliGrok *g)
{
  const jhcStare3D *s3 = &(g->s3);
  const jhcMatrix *hd;
  double pan, tilt;
  int sel;

  // see if behavior desired then find head closet to middle of image
  if (close <= 0) 
    return;
  if ((sel = g->ClosestFace((g->nav).rfwd)) < 0)
    return;

  // follow if planar distance close enough
  hd = s3->GetPerson(sel);
  (g->neck)->AimFor(pan, tilt, *hd, (g->lift)->Height());
  if (hd->PlaneVec3() <= hnear) 
    g->WatchPerson(s3->PersonID(sel), close);
}


//= Look at most recent person staring at robot (if any).

void jhcEliWatch::gaze_stare (jhcEliGrok *g)
{
  int sel;

  if (stare <= 0) 
    return;
  if ((sel = (g->fn).GazeNewID()) > 0)
    g->WatchPerson(sel, stare);
}


//= Look a while at most recently found face (if any),

void jhcEliWatch::gaze_face (jhcEliGrok *g)
{
  int sel;

  if (face <= 0) 
    return;
  if ((sel = (g->fn).FrontNewID(0, fmin)) > 0)
    g->WatchPerson(sel, face);
}


//= Slowly raise gaze to highest reasonable person head.

void jhcEliWatch::head_rise (jhcEliGrok *g)
{
  jhcMatrix hd(4);
  jhcEliNeck *n = g->neck;
  double pan, tilt, err, tol = 2.0;

  if (rise <= 0) 
    return;

  // find standard tilt for close heads
  hd.SetVec3(0.0, pdist, (g->s3).h1 - hdec);
  n->AimFor(pan, tilt, hd, (g->lift)->Height());

  // see whether adjustment needed
  err = n->TiltErr(tilt);
  if (err < aimed)
    up = 0;
  else if (err > (aimed + tol))
    up = 1;

  // slowly change tilt but keep azimuth (blocks head_center)
  if (up > 0)
  {
    n->TiltFix(tilt, rtime, rise);  
    n->PanTarget(n->Pan(), 1.0, rise);  
  }
}


//= Slowly move head back into alignment with body.

void jhcEliWatch::head_center (jhcEliGrok *g)
{
  jhcEliNeck *n = g->neck;
  double err, tol = 1.0; 

  if (align <= 0) 
    return;

  // see whether adjustment needed
  err = n->PanErr(0.0);
  if (err < aimed)
    mid = 0;
  else if (err > (aimed + tol))
    mid = 1;

  // just correct azimuth 
  if (mid > 0)
    n->PanFix(0.0, rtime, align);
}
