// jhcLocalNav.cpp : collection of local robot navigation routines
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2016 IBM Corporation
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

#include "Eli/jhcLocalNav.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcLocalNav::jhcLocalNav ()
{
  // no robot to control yet
  Bind(NULL);
 
  // shared depth analyzer
  fol.Bind(&sf);
  obs.Bind(&sf);

  // load processing parameters
  Defaults();
  Reset(0);
}


//= Default destructor does necessary cleanup.

jhcLocalNav::~jhcLocalNav ()
{
}


//= Connect system to set of sensors and actuators.

void jhcLocalNav::Bind (jhcEliBody *body)
{
  eb = body;
}


//= Reset state for the beginning of a sequence.

int jhcLocalNav::Reset (int body)
{
  int noisy = 1;

  // determine current image size
  if (eb != NULL)
    sf.SetSize(eb->XDim(), eb->YDim());

  // reset components 
//  sf.Reset();
  fol.Reset();
  obs.Reset();

  // no targets yet
  HipReset();
  SndReset();
  DirReset(0.0);

  // BODY = 1 - see if should talk to arm (or if even connected)
  comm = 0;
  if ((body <= 0) || (eb == NULL))
    return 1;
  comm = 1;

  // retract arm and close hand, set lift to desired height, look straight ahead 
  (eb->arm).Stow();
  (eb->lift).SetLift((eb->lift).ht0);
  (eb->neck).SetNeck(0.0, -40.0);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcLocalNav::Defaults (const char *fname)
{
  int ok = 1;

  ok &= follow_params(fname);
  ok &= pf.Defaults(fname);
  ok &= fol.Defaults(fname);
  ok &= obs.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcLocalNav::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= fps.SaveVals(fname);
  ok &= pf.SaveVals(fname);
  ok &= fol.SaveVals(fname);
  ok &= obs.SaveVals(fname);
  return ok;
}


//= Parameters used for adjusting following behavior.
// nothing geometric that differs between bodies

int jhcLocalNav::follow_params (const char *fname)
{
  jhcParam *ps = &fps;
  int ok;

  ps->SetTag("follow", 0);
  ps->NextSpecF( &fdown, -25.0, "Preferred tilt (deg)");  
  ps->NextSpecF( &finit,  48.0, "Max initial distance (in)");
  ps->NextSpecF( &foff,   28.0, "Offset distance (in)");      // from Kinect
  ps->NextSpecF( &gtime,   0.5, "Gaze response (secs)");      
  ps->NextSpecF( &rtime,   1.0, "Turn response (secs)");      
  ps->NextSpecF( &mtime,   0.5, "Move response (secs)");      

  ps->NextSpecF( &align,  60.0, "Alignment for move (degs)");
  ps->NextSpecF( &skew,    5.0, "Ballistic turn tolerance");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                            Person Following                           //
///////////////////////////////////////////////////////////////////////////

//= Clear any old target and get ready to follow new person.

void jhcLocalNav::HipReset ()
{
  fol.ClrLeader();
  fazm0  = 0.0;
  fdist0 = 0.0;
  fazm   = 0.0;
  fdist  = 0.0;
  fmode = 0;
}


//= Move robot to follow person using depth image.
// can optionally rotate and drive base as well as neck
// assumes Update methods called before, and Step methods called after
// "fazm" and "fdist" are persistent member variables for target
// returns 0 if no person seen, 1 if flywheeling, 2 if seen now

int jhcLocalNav::HipFollow (const jhcImg& d16, int base)
{
  double hh = 27.0;      // height of head above lift shelf
  int fmode0 = fmode;

  if (comm <= 0)
    return -1;
  if (!d16.SameFormat(sf.XDim(), sf.YDim(), 2))
    return Fatal("Bad images to jhcLocalNav::HipFollow");

  // do floor projection and find person 
  sf.SetCamera(0.0, 0.0, (eb->lift).Height() + hh);
  sf.CacheXYZ(d16, (eb->neck).Pan(), (eb->neck).Tilt(), 0.0);
  fol.AdjLeader((eb->base).StepSide(), (eb->base).StepFwd(), (eb->base).StepTurn());
  fmode = fol.FindLeader((eb->neck).Pan(), finit);

  // determine changes to direction and distance goals
  fazm0 = fazm;
  fdist0 = fdist;
  if (fmode > 0)
  {
    fazm = fol.LeaderAzm();
    fdist = fol.LeaderDist() - foff;
    if (fabs(fazm) > align)
      fdist = 0.0;
  }

  // announce if person newly acquired
  if ((fmode >= 2) && (fmode0 < 2))
    jprintf("    hips @ %+d degs x %d in\n", ROUND(fazm), ROUND(fdist + foff));

  // move head and base toward target (or expected position)
  (eb->neck).PanFix(fazm, gtime);   
  (eb->neck).TiltTarget(fdown);
  if (base > 0)
  {
    (eb->base).TurnFix(fazm, rtime);
    (eb->base).MoveFix(fdist, mtime);
  }
  return fmode;
}


///////////////////////////////////////////////////////////////////////////
//                           Sound Orientation                           //
///////////////////////////////////////////////////////////////////////////

//= Clear any old target and get ready to turn toward new sound.

void jhcLocalNav::SndReset ()
{
  nazm0 = 0.0;
  nazm  = 0.0;
  nmode = 0;
}


//= Move head to follow sound source using microphone only.
// can optionally rotate base as well as neck
// assumes Update methods called before, and Step methods called after
// "nazm" is a persistent member variable for target
// returns 1 for normal functioning

int jhcLocalNav::SndTrack (int talk, int base)
{
  int nmode0 = nmode;

  if (comm <= 0)
    return -1;
  if ((eb->mic).CommOK() <= 0)
    return Fatal("Bad audio in jhcLocalNav::SndTrack");

  // determine change to goal direction
  nazm0 = nazm;
  if (talk <= 0)
    nazm = (eb->mic).SmoothDir();
  else if ((eb->mic).VoiceStale() < 5)
    nazm = (eb->mic).VoiceDir();
  else
    nazm -= (eb->base).StepTurn();   // adjust saved heading for turn so far

  // announce voice if newl acquired
  nmode = 0;
  if ((eb->mic).VoiceStale() <= 1)
    nmode = 1;
  if ((nmode > 0) && (nmode0 <= 0))
    jprintf("    voice @ %+d degs\n", ROUND(nazm));

  // set neck (pan only) and base 
  (eb->neck).PanFix(nazm, gtime); 
  if (base > 0)
  {
//    (eb->base).TurnTarget(nazm, 180.0);
//    (eb->base).MoveTarget(0.0, 18.0);
    (eb->base).TurnFix(nazm, rtime);
    (eb->base).MoveFix(0.0, mtime);
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Ballistic Turn                              //
///////////////////////////////////////////////////////////////////////////

//= Remember desired orientation realtive to start.

void jhcLocalNav::DirReset (double desire)
{
  dazm = desire;
}


//= Keep rotating until desired azimuth reached.
// returns 1 when close to goal, 0 if still trying

int jhcLocalNav::DirTurn ()
{
  // adjust goal for robot motion
  dazm -= (eb->base).StepTurn();
  while (dazm > 180.0)
    dazm -= 360.0;
  while (dazm <= -180.0)
    dazm += 360.0; 

  // turn toward target
  if (fabs(dazm) < skew)
    dazm = 0.0;
  (eb->base).TurnTarget(dazm);
  if (dazm == 0.0)
    return 1;
  return 0;
}

