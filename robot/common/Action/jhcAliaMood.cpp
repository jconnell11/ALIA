// jhcAliaMood.cpp : maintains slow changing state variables for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021-2023 Etaoin Systems
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

#include "Reasoning/jhcActionTree.h"   // common audio - since only spec'd as class in header

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
  noisy = 1;                 // defaulted from jhcAliaCore
  LoadCfg();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for adjusting minimum belief threshold.
// NOTE: these parameters affect personality

int jhcAliaMood::blf_params (const char *fname)
{
  jhcParam *ps = &bps;
  int ok;

  ps->SetTag("mood_blf", 0);
  ps->NextSpecF( &right,  0.1, "Threshold down on correct");
  ps->NextSpecF( &wrong,  0.1, "Threshold up on wrong");
  ps->NextSpecF( &fyes,   0.1, "User \"yes\" threshold lower");
  ps->NextSpecF( &fno,    0.1, "User \"no\" threshold raise");
  ps->Skip();
  ps->NextSpecF( &bfade, 60.0, "Revert time constant (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for adjusting minimum preference threshold.
// NOTE: these parameters affect personality

int jhcAliaMood::pref_params (const char *fname)
{
  jhcParam *ps = &pps;
  int ok;

  ps->SetTag("mood_pref", 0);
  ps->NextSpecF( &miss,   0.05, "Threshold down on missed");
  ps->NextSpecF( &dud,    0.01, "Threshold up on failed");
  ps->NextSpecF( &fgood,  0.1,  "User \"good\" threshold lower");
  ps->NextSpecF( &fbad,   0.1,  "User \"bad\" threshold raise");
  ps->Skip();
  ps->NextSpecF( &pfade, 60.0,  "Revert time constant (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for assessing boredom and overwork.
// NOTE: these parameters can affect personality

int jhcAliaMood::bored_params (const char *fname)
{
  jhcParam *ps = &aps;
  int ok;

  ps->SetTag("mood_bored", 0);
  ps->NextSpecF( &frantic,  25.0, "Overwhelmed threshold");
  ps->NextSpecF( &engaged,  15.0, "Optimum thought level");
  ps->NextSpecF( &idle,     10.0, "Minimal thought level");
  ps->NextSpecF( &active,    2.0, "Good motion level");
  ps->NextSpecF( &bored,     0.5, "Bored motion threshold");
  ps->NextSpecF( &nag,      30.0, "Whine interval (sec)");

  ps->NextSpecF( &btime,     3.5, "Thought decay (sec)");  
  ps->NextSpecF( &ftime,    20.0, "Motion decay (sec)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for assessing social interaction.
// NOTE: these parameters can affect personality

int jhcAliaMood::lonely_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("mood_lonely", 0);
  ps->NextSpecF( &attn,    1.5, "Attention threshold");  
  ps->NextSpecF( &sat,     5.0, "Attention saturation");  
  ps->NextSpecF( &prod,   90.0, "Prod interval (sec)");
  ps->NextSpecF( &ramp,    5.0, "Shorten interval (sec)"); 
  ps->NextSpecF( &needy,  40.0, "Minimum interval (sec)");
  ps->NextSpec4( &bereft,  4,   "Very lonely on repeat");

  ps->NextSpecF( &fade,   10.0, "User input decay (sec)"); 
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for assessing remaining battery energy.
// NOTE: these parameters can affect personality

int jhcAliaMood::tired_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("mood_tired", 0);
  ps->NextSpec4( &fresh,   35,   "Okay battery (pct)");
  ps->NextSpec4( &tired,   30,   "Low battery (pct)");
  ps->NextSpec4( &slug,    20,   "Very low battery (pct)");
  ps->NextSpec4( &psamp,  900,   "Test interval cycles");      // 30 secs
  ps->NextSpecF( &repeat, 180.0, "Complaint repeat (sec)");  
  ps->NextSpecF( &urgent,  30.0, "Fastest repeat (sec)");  

  ps->NextSpecF( &calm,    5.0,  "Surprise decay (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcAliaMood::LoadCfg (const char *fname)
{
  int ok = 1;

  ok &= blf_params(fname);
  ok &= pref_params(fname);
  ok &= bored_params(fname);
  ok &= lonely_params(fname);
  ok &= tired_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcAliaMood::SaveCfg (const char *fname) const
{
  int ok = 1;

  ok &= bps.SaveVals(fname);
  ok &= pps.SaveVals(fname);
  ok &= aps.SaveVals(fname);
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
  // clear time stamp and raw data
  now = 0;
  clr_evts();

  // sampled body state
  bspeed = 0.0;
  fspeed = 0.0;
  mspeed = 0.0;
  pct = 100;

  // mental level
  busy = 0.0;
  yikes = 0;

  // physical level
  fidget = active;
  blah = 0;

  // speech level
  input = attn;
  lament = 0;

  // battery charge
  power = 100;
  delay = 0;

  // rule changes
  surp = 0.0;
}


//= Clear event data which has accumulated during cycle.

void jhcAliaMood::clr_evts ()
{
  win = 0;
  lose = 0;
}


//= Set up for next round of data collection.

void jhcAliaMood::Update (int nag)
{
  double dt, s0, p0, ips, noise = 2.0;           // was 1
  UL32 last = now;

  // get elapsed time since last call
  now = jms_now();  
  if (last != 0)
  {
    // figure out overall body motion (squelched)
    dt = jms_secs(now, last);
    ips = bspeed + fspeed + mspeed;
    if (ips < noise)
      ips = 0.0;

    // update various mood variables
    fidget += (ips - fidget) * dt / ftime;
    busy  -= busy * dt / btime;           
    input -= input * dt / fade;
    surp  -= dt / calm;                   

    // gradually revert belief and preference thresholds
    if (rpt != NULL)
    {
      s0 = rpt->MinBlf(); 
      p0 = rpt->MinPref();
      rpt->SetMinBlf( s0 - (s0 - rpt->RestBlf())  * dt / bfade);
      rpt->SetMinPref(p0 - (p0 - rpt->RestPref()) * dt / pfade);
    }
  }

  // clear old accumulated data 
  clr_evts();

  // possibly generate a status NOTE
  if (rpt == NULL)
    return;
  rpt->StartNote();
  if (last == 0)                                 // start up
    rpt->NewProp(rpt->Self(), "hq", "awake");
  else if (nag > 0)                              // see if nags enabled
    if (chk_busy() <= 0)                         // might be overwhelmed
      if (chk_antsy() <= 0)
        if (chk_lonely() <= 0)
          chk_tired();
  rpt->FinishNote();
}


//= See if mental activity is above the overwhelmed threshold.
// returns 1 if some NOTE composed (assumes NOTE already started)

int jhcAliaMood::chk_busy ()
{
  // clear hysteretic state (explicitly retract assertions)
  if (busy < engaged)
    if (yikes > 0)
    {
      rpt->NewProp(rpt->Self(), "hq", "overwhelmed", 1);
      yikes = 0;
      return 1;
    }

  // check for overstimulation (complains just once at beginning)
  if ((busy >= frantic) && (yikes <= 0))
  {
    jprintf(1, noisy, " { chk_busy: subgoaling at %3.1f }\n", busy);
    rpt->NewProp(rpt->Self(), "hq", "overwhelmed");
    yikes = 1;
    return 1;
  }
  return 0;                  // no NOTEs generated
}


//= See if physical activity is below the boredom threshold.
// only initiates complaint during low thought periods (clears always)
// returns 1 if some NOTE composed (assumes NOTE already started)

int jhcAliaMood::chk_antsy ()
{
  int b0 = blah, very = 3;

  // clear hysteretic state (explicitly retract assertions)
  if (fidget >= active)
  {
    blah = 0;
    if (b0 > 1)
    {
      rpt->NewProp(rpt->Self(), "hq", "bored", 1);
      return 1;
    }
  }

  // check for boredom (waits a while then complains regularly)
  if ((fidget < bored) && (blah <= 0))
  {
    kvetch = now;
    blah = 1;
  }
  else if ((blah > 0) && (jms_secs(now, kvetch) >= nag))
    if (busy <= idle)
    {
      jprintf(1, noisy, " { chk_antsy: activity at %3.1f [x%d] }\n", fidget, blah);
      rpt->NewDeg(rpt->Self(), "hq", "bored", ((blah >= very) ? "very" : NULL));
      kvetch = now;
      blah++;
      return 1;
    }
  return 0;                  // no NOTEs generated
}


//= See if user input level is below the loneliness threshold.
// only initiates complaint during low thought periods (clears always)
// returns 1 if some NOTE composed (assumes NOTE already started)

int jhcAliaMood::chk_lonely ()
{
  double pause;
  int lam0 = lament;

  // possibly erase lonely state (and explicitly retract assertion)
  if (input >= attn)
  {
    lament = 0;
    if (lam0 > 1)
    {
      rpt->NewProp(rpt->Self(), "hq", "lonely", 1);
      return 1;
    }
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
        if (busy <= idle)
        {
          jprintf(1, noisy, " { chk_lonely: interaction at %3.1f [x%d] }\n", input, lament);
          rpt->NewDeg(rpt->Self(), "hq", "lonely", ((lament >= bereft) ? "very" : NULL));
          call = now;
          lament++;
          return 1;
        }
    }
  }
  return 0;                  // no NOTEs generated
}


//= See if remaining battery charge has dropped too far.
// only initiates complaint during low thought periods (clears always)
// returns 1 if some NOTE composed (assumes NOTE already started)

int jhcAliaMood::chk_tired ()
{
  double wait;
  UL32 m0 = moan;

  // see if battery low for a while 
  if (power >= fresh)
  {
    delay = 0;
    moan = 0;
    if (m0 != 0)
    {
      rpt->NewProp(rpt->Self(), "hq", "tired", 1);
      return 1;
    }
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
      if (busy <= idle)
      {
        jprintf(1, noisy, " { chk_tired: battery at %d percent }\n", power);
        rpt->NewDeg(rpt->Self(), "hq", "tired", ((power <= slug) ? "very" : NULL));
        moan = now;
        return 1;
      }
  }
  return 0;                  // no NOTEs generated
}


///////////////////////////////////////////////////////////////////////////
//                    Global Threshold Adjustment                        //
///////////////////////////////////////////////////////////////////////////

//= Adjust minimum fact belief level based on user agreement or refutation.
// returns change to threshold value 

void jhcAliaMood::UserMinBlf (int correct) 
{
  if (rpt == NULL)
    return;
  if (correct > 0)
    adj_blf(rpt->MinBlf() - correct * fyes);  // lowers
  else if (correct < 0)
    adj_blf(rpt->MinBlf() - correct * fno);   // raises
}


//= Adjust the global belief threshold based on a number of correct and incorrect predictions.
// returns change to threshold value 

void jhcAliaMood::BumpMinBlf (int hit, int miss) 
{
  if (rpt != NULL)
    adj_blf(rpt->MinBlf() + hit * right - miss * wrong);
}   


//= Set minimum belief threshold value to new value.
// MinBlf() = "skep" hard limited to range 0.1 - 1.0
// returns change to threshold value 

void jhcAliaMood::adj_blf (double s) 
{
  double chg, s0 = rpt->MinBlf();

  rpt->SetMinBlf(s);
  chg = rpt->MinBlf() - s0;
  if ((noisy >= 1) && (fabs(chg) > 0.001))       // fades to 0.5 over cycle
    jprintf("  MOOD: belief thresh --> %4.2f (more %s)\n", rpt->MinBlf(), ((chg < 0.0) ? "confident" : "unsure"));
}


//= Adjust minimum operator preference level based on user praise or disapproval.
// threshold alteration happens immediately

void jhcAliaMood::UserMinPref (int good)    
{
  if (rpt == NULL)
    return;
  if (good > 0)
    adj_pref(rpt->MinPref() - good * fgood);     // lowers
  else if (good < 0)
    adj_pref(rpt->MinPref() - good * fbad);      // raises
}


//= Change operator preference threshold either up or down.
// inc = +1: some operator failed (perhaps should not have been tried)
// inc = -1: no applicable operators over current threshold, but something below
// threshold alteration happens immediately

void jhcAliaMood::BumpMinPref (int inc)
{
  if (rpt == NULL)
    return;
  if (inc > 0)
    adj_pref(rpt->MinPref() + inc * dud);
  else if (inc < 0)
    adj_pref(rpt->MinPref() - inc * miss);
}


//= Set minimum preference threshold value to new value.

void jhcAliaMood::adj_pref (double p) 
{
  double chg, p0 = rpt->MinPref();

  rpt->SetMinPref(p);
  chg = rpt->MinPref() - p0;
  if ((noisy >= 1) && (fabs(chg) > 0.001))       // fades to 0.5 over cycle
    jprintf("  MOOD: operator thresh --> %4.2f (more %s)\n", rpt->MinPref(), ((chg < 0.0) ? "curious" : "prudent"));
}


///////////////////////////////////////////////////////////////////////////
//                        Operator Invocation                            //
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


///////////////////////////////////////////////////////////////////////////
//                          Internal Changes                             //
///////////////////////////////////////////////////////////////////////////

//= Note that some rule strength was off by a certain amount.

void jhcAliaMood::Believe (double adj)
{
  if (adj == 0.0)
    return;
  surp = __max(surp, fabs(adj));
}


///////////////////////////////////////////////////////////////////////////
//                         User Communication                            //
///////////////////////////////////////////////////////////////////////////

//= Note that the robot generated an output (possibly with some output length).

void jhcAliaMood::Speak (int len)
{
  if (len <= 0)
    return;
  // ignored for now
}


//= Note that a user is provided input to the robot (possibly with some input length).

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


///////////////////////////////////////////////////////////////////////////
//                            Body Activity                              //
///////////////////////////////////////////////////////////////////////////

//= Cache movement speed, manipulator speed, and battery charge.
// speeds should be positive only, pct from 0 to 100, emit is 0 or 1
// typically called from main loop in overall coordination class

void jhcAliaMood::Body (double bips, double fips, int pct)
{
  bspeed = bips;
  fspeed = fips;
  power = pct;
}


//= Cache mouth speed based on whether robot is currently speaking.
// assumes "out" is positive when TTS is occurring (not utterance length)

void jhcAliaMood::Emit (int out)
{
  mspeed = ((out > 0) ? 6.0 : 0.0);
}
