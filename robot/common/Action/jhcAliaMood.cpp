// jhcAliaMood.cpp : maintains slow changing state variables for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021 Etaoin Systems
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

#include "Interface/jms_x.h"           // common video

#include "Action/jhcAliaMood.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaMood::~jhcAliaMood ()
{
}


//= Default constructor initializes certain values.

jhcAliaMood::jhcAliaMood ()
{
// should move to jhcStats
sz = 600;
bhist.SetSize(sz);

  Defaults();
  Reset();
}

/*
//= Give values of each individual influence on mood.

void jhcAliaMood::Levels (double now[]) const
{
}
*/

///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters for assessing boredom and overwork.

int jhcAliaMood::busy_params (const char *fname)
{
  jhcParam *ps = &bps;
  int ok;

  ps->SetTag("mood_busy", 0);
  ps->NextSpecF( &frantic,  25.0, "Overwhelmed threshold");
  ps->NextSpecF( &engaged,  12.0, "Optimum threshold");
  ps->NextSpecF( &idle,      5.0, "Low activity threshold");
  ps->NextSpecF( &bored,     1.0, "Bored threshold");
  ps->NextSpecF( &nag,      40.0, "Whine interval (sec)");
  ps->NextSpec4( &very,      3,   "Very bored on repeat");

  ps->NextSpecF( &tc,        3.5, "Activity decay (sec)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for assessing social interaction.

int jhcAliaMood::lonely_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("mood_lonely", 0);
  ps->NextSpecF( &attn,    1.5, "Attention threshold");  
  ps->NextSpecF( &sat,     5.0, "Attention saturation");  
  ps->NextSpecF( &prod,   60.0, "Prod interval (sec)");
  ps->NextSpecF( &ramp,   10.0, "Shorten interval (sec)"); 
  ps->NextSpecF( &needy,  20.0, "Minimum interval (sec)");
  ps->NextSpec4( &bereft,  4,   "Very lonely on repeat");

  ps->NextSpecF( &fade,   10.0, "User input decay (sec)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for assessing remaining battery energy.

int jhcAliaMood::tired_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("mood_tired", 0);
  ps->NextSpec4( &fresh,   55,   "Okay battery (pct)");
  ps->NextSpec4( &tired,   50,   "Low battery (pct)");
  ps->NextSpec4( &slug,    25,   "Very low battery (pct)");
  ps->NextSpec4( &psamp,  900,   "Test interval cycles");      // 30 secs
  ps->NextSpecF( &repeat, 180.0, "Complaint repeat (sec)");  
  ps->NextSpecF( &urgent,  30.0, "Fastest repeat (sec)");  

  ps->NextSpecF( &calm,    1.0, "Surprise decay (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcAliaMood::Defaults (const char *fname)
{
  int ok = 1;

  ok &= busy_params(fname);
  ok &= lonely_params(fname);
  ok &= tired_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcAliaMood::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= bps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= tps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcAliaMood::Reset ()
{
// should move to jhcStats
bhist.Fill(0);
fill = 0;

  // clear time stamp and raw data
  now = 0;
  clr_data();

  // activity level
  busy = 0.0;
  yikes = 0;
  blah = 0;

  // interaction level
  input = 0.0;
  lament = 0;

  // battery charge
  power = 100;
  delay = 0;

  // rule changes
  surp = 0.0;
}


//= Clear raw data which has accumulated during cycle.

void jhcAliaMood::clr_data ()
{
  win = 0;
  lose = 0;
}


//= Set up for next round of data collection.

void jhcAliaMood::Update (jhcAliaNote& rpt)
{
  double dt = 0.0;
  UL32 last = now;

  // get current time and decay old values
  now = jms_now();  
  if (last != 0)
  {
    dt = jms_secs(now, last);
    busy  *= exp(dt / -tc);            // IIR filter 
    input *= exp(dt / -fade);
    surp  -= surp * calm * dt;         // linear decay
  }

// should move to jhcStats
//bhist.Scroll(fill, ROUND(1000.0 * busy));
bhist.Scroll(fill, ROUND(1000.0 * input));
fill++;

  // clear old data and possibly generate a status NOTE
  clr_data();
  rpt.StartNote();
  if (last == 0)                                 // start up
    rpt.NewProp(rpt.Self(), "hq", "awake");
  else if (chk_busy(rpt) <= 0)                   // might be overwhelmed
    if (busy <= idle)                            // only complain if idle
      if (chk_lonely(rpt) <= 0)
        chk_tired(rpt);
  rpt.FinishNote();
}


//= See if busyness is below the boredom threshold.
// returns 1 if some NOTE composed (assumes NOTE already started)

int jhcAliaMood::chk_busy (jhcAliaNote& rpt)
{
  // clear hysteretic states (explicitly retract assertions)
  if (busy <= engaged)
  {
    if (yikes > 0)
      rpt.NewProp(rpt.Self(), "hq", "overwhelmed", 1);
    yikes = 0;
  }
  if (busy >= engaged)
  {
    if (blah > 1)
      rpt.NewDeg(rpt.Self(), "hq", "bored", ((blah >= very) ? "very" : NULL), 1);
    blah = 0;
  }

  // check for overstimulation (complains just once at beginning)
  if ((busy >= frantic) && (yikes <= 0))
  {
    jprintf("{ chk_busy: overwhelmed at %3.1f }\n", busy);
    rpt.NewProp(rpt.Self(), "hq", "overwhelmed");
    yikes = 1;
    return 1;
  }

  // check for boredom (waits a while then complains regularly)
  if ((busy <= bored) && (blah <= 0))
  {
    kvetch = now;
    blah = 1;
  }
  else if ((blah > 0) && (jms_secs(now, kvetch) >= nag))
  {
    jprintf("{ chk_busy: bored at %3.1f [x%d] }\n", busy, blah);
    rpt.NewDeg(rpt.Self(), "hq", "bored", ((blah >= very) ? "very" : NULL));
    kvetch = now;
    blah++;
    return 1;
  }
  return 0;                  // no NOTEs generated
}


//= See if user input level is below the loneliness threshold.
// returns 1 if some NOTE composed (assumes NOTE already started)

int jhcAliaMood::chk_lonely (jhcAliaNote& rpt)
{
  double pause;

  // possibly erase lonely state (and explicitly retract assertion)
  if (input >= attn)
  {
    if (lament > 1)
      rpt.NewDeg(rpt.Self(), "hq", "lonely", ((lament >= bereft) ? "very" : NULL), 1);
    lament = 0;
  }

  // comment at successively more frequent intervals (skip first)
  if (input < attn)
  {
    if (lament <= 0)
    {
      call = now;
      lament = 1;
    }
    else
    {
      pause = prod - ramp * lament;
      pause = __max(needy, pause);
      if (jms_secs(now, call) >= pause)
      {
        jprintf("{ chk_lonely: input at %3.1f [x%d] }\n", input, lament);
        rpt.NewDeg(rpt.Self(), "hq", "lonely", ((lament >= bereft) ? "very" : NULL));
        call = now;
        lament++;
        return 1;
      }
    }
  }
  return 0;                  // no NOTEs generated
}


//= See if remaining battery charge has dropped too far.
// returns 1 if some NOTE composed (assumes NOTE already started)

int jhcAliaMood::chk_tired (jhcAliaNote& rpt)
{
  double wait;

  // see if battery low for a while 
  if (power >= fresh)
  {
    delay = 0;
    moan = 0;
  }
  else if (power <= tired)
    delay++;

  // determine how often to complain (more frequent when lower)
  if (delay >= psamp)
  {
    delay = 0;
    wait = repeat * power / tired;
    wait = __max(urgent, wait);
    if ((moan == 0) || (jms_secs(now, moan) >= wait))
    {
      jprintf("{ power_state: battery at %d percent }\n", power);
      rpt.NewDeg(rpt.Self(), "hq", "tired", ((power <= slug) ? "very" : NULL));
      moan = now;
      return 1;
    }
  }
  return 0;                  // no NOTEs generated
}

/*
//= Determine preference for operator if it changes values as predicted.
// assumes chg array has at least NumVars entries

double jhcAliaMood::Preference (const double chg[]) const
{
  return 1.0;
}


//= Alter affordances for an operator based on starting and current state.
// assumes chg and start arrays have at least NumVars entries

void jhcAliaMood::Adjust (double chg[], const double start[])
{
}
*/

///////////////////////////////////////////////////////////////////////////
//                              Data Collection                          //
///////////////////////////////////////////////////////////////////////////

//= Note that a new directive has been started.

void jhcAliaMood::Launch ()
{
  busy += 1.0;
}


//= Note that some directive has completed successfully.

void jhcAliaMood::Win ()
{
  win++;
}


//= Note that some directive has failed.

void jhcAliaMood::Lose ()
{
  lose++;
}


//= Note that the robot is speaking (possibly with some output length).

void jhcAliaMood::Speak (int len)
{
  if (len <= 0)
    return;
  // ignored for now
}


//= Note that a user is speaking to the robot (possibly with some input length).

void jhcAliaMood::Hear (int len)
{
  if (len <= 0)
    return;
  input += 1.0;
  input = __min(input, sat);
}


//= Note that a new rule has been added (or more than one).

void jhcAliaMood::Infer (int cnt)
{
  if (cnt <= 0)
    return;
  // ignored for now
}


//= Note that a new operator has been added (or more than one).

void jhcAliaMood::React (int cnt)
{
  if (cnt <= 0)
    return;
  // ignored for now
}


//= Note that some rule strength was off by a certain amount.

void jhcAliaMood::Believe (double miss)
{
  if (miss <= 0.0)
    return;
  surp = __max(surp, miss);
}


//= Note that some operator preference was adjusted by some amount.

void jhcAliaMood::Prefer (double adj)
{
  if (adj == 0.0)
    return;
  // ignored for now
}

/*
//= Note user reinforcement of some behavior.

void jhcAliaMood::Praise (double deg)
{
  if (deg <= 0.0)
    return;
  // not connected yet
}


//= Note user disparagment of some behavior.

void jhcAliaMood::Rebuke (double deg)
{
  if (deg <= 0.0)
    return;
  // not connected yet
}
*/

//= Note the remaining energy level as a percentage.
// typically called from a class derived from jhcBackgRWI

void jhcAliaMood::Energy (int pct)
{
  power = pct;
}


//= Note that the robot is moving either forward or backward at some speed.
// typically called from a class derived from jhcBackgRWI

void jhcAliaMood::Walk (double sp)
{
  if (sp <= 0.0)
    return;
  // not connected yet
}


//= Note that the finger tips are separating or moving at some speed.
// typically called from a class derived from jhcBackgRWI

void jhcAliaMood::Wave (double sp)
{
  if (sp <= 0.0)
    return;
  // not connected yet
}


