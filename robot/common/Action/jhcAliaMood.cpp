// jhcAliaMood.cpp : maintains slow changing state variables for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021-2024 Etaoin Systems
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
  LoadCfg();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters to evaluate reasoning state and battery level.

int jhcAliaMood::core_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("mood_core", 0);
  ps->NextSpecF( &btime,    0.3, "Busy decay (sec)");
  ps->NextSpecF( &engaged, 15.0, "Tolerable goals");      
  ps->NextSpecF( &frantic, 25.0, "Overwhelmed goals");    
  ps->NextSpecF( &wtime,    3.0, "Surprise decay (sec)");
  ps->NextSpecF( &surp,     0.7, "Surprised level");
  ps->NextSpecF( &vsurp,    1.3, "Very surprised level");

  ps->NextSpecF( &low,     30.0, "Tired battery (pct)");  
  ps->NextSpecF( &vlow,    20.0, "Very tired battery (pct)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for combining inputs to assess robot motion level.

int jhcAliaMood::motion_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("mood_mot", 0);
  ps->NextSpecF( &fhand,  10.0,  "Hand idle wrt motion");
  ps->NextSpecF( &fbase,  10.0,  "Base idle wrt motion");
  ps->NextSpecF( &ftalk,   0.0,  "Talk idle wrt motion");
  ps->NextSpecF( &noise,   0.15, "Low motion squelch");
  ps->NextSpecF( &mtime,  30.0,  "Motion smoothing (sec)");  
  ps->NextSpecF( &mok,     0.5,  "Adequate motion level");  

  ps->NextSpecF( &bore,   40.0,  "Bored time (sec)");
  ps->NextSpecF( &vbore,  90.0,  "Very bored time (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for combining inputs to assess robot sociality level.

int jhcAliaMood::social_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("mood_soc", 0);
  ps->NextSpecF( &fhear,  15.0, "Speech idle wrt hearing");
  ps->NextSpecF( &fdude,   0.3, "Boost per face");
  ps->NextSpecF( &lps,    12.0, "Letters per second");
  ps->NextSpecF( &stime,  60.0, "Social smoothing (sec)");  
  ps->NextSpecF( &sok,     0.5, "Adequate social level");  
  ps->Skip();

  ps->NextSpecF( &lone,   30.0, "Lonely time (sec)");
  ps->NextSpecF( &vlone,  60.0, "Very lonely time (sec)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for combining inputs to assess happiness/unhappiness.

int jhcAliaMood::valence_params (const char *fname)
{
  jhcParam *ps = &vps;
  int ok;

  ps->SetTag("mood_val", 0);
  ps->NextSpecF( &mmix, 0.5,  "Motion importance");
  ps->NextSpecF( &smix, 0.5,  "Social importance");
  ps->NextSpecF( &hhys, 0.2,  "Happy hysteresis");
  ps->NextSpecF( &hap,  0.75, "Happy level");  
  ps->NextSpecF( &vhap, 1.5,  "Very happy level");  
  ps->NextSpecF( &lmix, 1.0,  "Lonely wrt bored wt");  

  ps->NextSpecF( &sad,  1.0,  "Unhappy level");  
  ps->NextSpecF( &vsad, 2.0,  "Very unhappy level");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for assessing performance of operators.

int jhcAliaMood::op_params (const char *fname)
{
  jhcParam *ps = &ops;
  int ok;

  ps->SetTag("mood_op", 0);
  ps->NextSpecF( &fgood,  7.0,  "User approval wt");
  ps->NextSpecF( &fbad,   2.0,  "User critique wt");
  ps->NextSpecF( &osamp, 50.0,  "Sample normalization");  
  ps->NextSpecF( &otime, 30.0,  "Reversion time (sec)");
  ps->NextSpecF( &cdes,   0.9,  "Target success rate");
  ps->NextSpecF( &chys,   0.05, "Angry hysteresis");

  ps->NextSpecF( &mad,    0.7,  "Angry rate");
  ps->NextSpecF( &vmad,   0.6,  "Very angry rate");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for assessing performance of rules.

int jhcAliaMood::rule_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("mood_rule", 0);
  ps->NextSpecF( &fconf,   7.0,  "User confirmation wt");  
  ps->NextSpecF( &fref,    2.0,  "User refutation wt");   
  ps->NextSpecF( &rsamp,  50.0,  "Sample normalization");
  ps->NextSpecF( &rtime,  45.0,  "Reversion time (sec)");
  ps->NextSpecF( &sdes,    0.9,  "Target correct rate");
  ps->NextSpecF( &shys,    0.03, "Scared hysteresis");

  ps->NextSpecF( &scare,   0.85, "Scared rate");
  ps->NextSpecF( &vscare,  0.8,  "Very scared rate");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for adjusting wildness and belief threshold.

int jhcAliaMood::adj_params (const char *fname)
{
  jhcParam *ps = &aps;
  int ok;

  ps->SetTag("mood_adj", 0);
  ps->NextSpecF( &whi,  5.0, "Op success over optimal");
  ps->NextSpecF( &wlo, -3.0, "Op success under optimal");
  ps->Skip();
  ps->NextSpecF( &bhi, -5.0, "Rule correct over optimal");
  ps->NextSpecF( &blo,  3.0, "Rule correct under optimal");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for computing overall desire for action.

int jhcAliaMood::pref_params (const char *fname)
{
  jhcParam *ps = &pps;
  int ok;

  ps->SetTag("mood_pref", 0);
  ps->NextSpecF( &mhi,  0.2, "Motion over optimal");  
  ps->NextSpecF( &mlo, -0.2, "Motion under optimal");   
  ps->NextSpecF( &shi,  0.2, "Social over optimal");
  ps->NextSpecF( &slo, -0.2, "Social under optimal");
  ps->NextSpecF( &ohi, -5.0, "Control over optimal");
  ps->NextSpecF( &olo,  2.0, "Control under optimal");

  ps->NextSpecF( &rhi, -5.0, "Sureness over optimal");
  ps->NextSpecF( &rlo,  2.0, "Sureness under optimal");
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

  ok &= core_params(fname);
  ok &= motion_params(fname);
  ok &= social_params(fname);
  ok &= valence_params(fname);
  ok &= op_params(fname);
  ok &= rule_params(fname);
  ok &= adj_params(fname);
  ok &= pref_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcAliaMood::SaveCfg (const char *fname) const
{
  int ok = 1;

  ok &= cps.SaveVals(fname);
  ok &= mps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= vps.SaveVals(fname);
  ok &= ops.SaveVals(fname);
  ok &= rps.SaveVals(fname);
  ok &= aps.SaveVals(fname);
  ok &= pps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcAliaMood::Reset ()
{
  // event data and timers
  clr_accum();
  now  = 0;
  mtim = 0.0;
  itim = 0.0;

  // drive values
  energy = 100.0;
  motion = mok;
  social = sok;

  // valence metrics
  satis = mok + sok;
  antsy = 0.0;
  isol  = 0.0;
  lack  = 0.0;

  // mental level
  busy = 0.0;
  wow  = 0.0;
  ctrl = cdes;
  sure = sdes;

  // cached emotion bits 
  melt = 0;
  vect = 0;
}


//= Clear reasoning event data which has accumulated during cycle.

void jhcAliaMood::clr_accum ()
{
  // operator data
  win  = 0;
  lose = 0;
  good = 0;
  bad  = 0;

  // rule data
  right   = 0;
  wrong   = 0;
  confirm = 0; 
  refute  = 0;
  jump    = 0.0;
}


//= Adjust emotions then set up for next round of data collection.

void jhcAliaMood::Update ()
{
  double dt;
  UL32 last = now;

  // get elapsed time since last call
  now = jms_now();  
  if (last == 0)
    return;
  dt = jms_secs(now, last);

  // evaluate basic drives 
  sm_motion(dt);
  sm_social(dt);
  valence(dt);

  // evaluate reasoning
  sm_busy(dt);
  sm_ctrl(dt);
  sm_sure(dt);

  // tune reasoning system
  adj_wild();
  adj_belief();
  adj_pref();

  // quantize emotions then clear events
  vect = bit_vector();
  clr_accum();
}


//= Generates a bit vector of thresholded mood states for external use.
// <pre>
// very:   80:00   40:00   20:00   10:00      08:00   04:00   02:00   01:00
//      surprised  angry  scared   happy  -  unhappy  bored   lonely  tired
// base:   00:80   00:40   00:20   00:10      00:08   00:04   00:02   00:01
// </pre>
// robot can be feeling multiple emotions at the same time
// "very scared and tired" = 04:00 | 00:40 | 00:01 = 04:41 hex

int jhcAliaMood::bit_vector () const
{
  int feel = 0;

  // upper byte (specific conditions)
  if (wow >= surp)                                   // surprised
    feel |= ((wow >= vsurp) ? 0x8080 : 0x0080); 
  feel |= dual_under(0x4000, ctrl,   vmad, chys);    // angry
  feel |= dual_under(0x0040, ctrl,    mad, chys);   
  feel |= dual_under(0x2000, sure, vscare, shys);    // scared
  feel |= dual_under(0x0020, sure,  scare, shys);   
  feel |= dual_over( 0x1000, satis,  vhap, hhys);    // happy
  feel |= dual_over( 0x0010, satis,   hap, hhys);   

  // lower byte (general dissatisfaction)
  if (lack >= sad)                                   // unhappy
    feel |= ((lack >= vsad)   ? 0x0808 : 0x0008);
  if (antsy >= bore)                                 // bored
    feel |= ((antsy >= vbore) ? 0x0404 : 0x0004);
  if (isol >= lone)                                  // lonely
    feel |= ((isol >= vlone)  ? 0x0202 : 0x0002);    
  if (energy <= low)                                 // tired
    feel |= ((energy <= vlow) ? 0x0101 : 0x0001);

  // special case for overwhelmed
  if (melt > 0)
    feel |= 0x8080;                                  // very surprised
  return feel;
}


//= Do hysteretic thresholding on some indicator under a threshold to set a bit.

int jhcAliaMood::dual_under (int mask, double val, double on, double hys) const
{
  int set = vect & mask;

  if (val <= on)
    return mask;
  if ((set != 0) && (val >= (on + hys)))
    return 0;
  return set;
}


//= Do hysteretic thresholding on some indicator over a threshold to set a bit.

int jhcAliaMood::dual_over (int mask, double val, double on, double hys) const
{
  int set = vect & mask;

  if (val >= on)
    return mask;
  if ((set != 0) && (val <= (on - hys)))
    return 0;
  return set;
}


///////////////////////////////////////////////////////////////////////////
//                       Drives and Evaluations                          //
///////////////////////////////////////////////////////////////////////////

//= Generate smoothed version of overall body motion.
// motion = 1.0 is ideal (servo setpoint)

void jhcAliaMood::sm_motion (double dt)
{
  double m;

  m = fhand * fspeed + fbase * bspeed;           // physical action
  if (m < noise)                                 // squelch jitter
    m = 0.0;
  if (mtim > 0.0)                                // stretched output
  {
    m += ftalk;                                  
    mtim = __max(0.0, mtim - dt);
  }
  motion += (m - motion) * dt / mtime;           // IIR filter
}


//= Generate smoothed version of social interaction.
// social = 1.0 is ideal (servo setpoint)

void jhcAliaMood::sm_social (double dt)
{
  double s;

  s = fdude * __min(people, 3);                  // human company
  if (itim > 0.0)                                // stretched input
  {
    s += fhear;
    itim = __max(0.0, itim - dt);
  }
  social += (s - social) * dt / stime;           // IIR filter
}


//= Evaluate fulfillment of basic drives.

void jhcAliaMood::valence (double dt)
{
  double bmix = 1.0;

  // get overall drive satisfaction level
  satis = mmix * motion + smix * social;

  // see if motion or social below desired level
  if (motion > mok)
    antsy = 0.0;
  else
    antsy += dt;
  if (social > sok)
    isol = 0.0;
  else
    isol += dt;

  // get overall dissatisfaction level 
  lack = bmix * (antsy / bore) + lmix * (isol / lone);  
}


//= Assess general state of reasoning system.
// short term number of goals (0-n) and rule surprise (0-1)

void jhcAliaMood::sm_busy (double dt)
{
  // mental activity
  busy -= busy * dt / btime;     
  busy = __max(busy, atree->NumGoals());      

  // check for excessive thinking
  if (busy >= frantic)
    melt = 1;
  else if (busy <= engaged)
    melt = 0;

  // surprise
  wow -= wow * dt / wtime;
  wow = __max(wow, jump); 
}


//= Determine robot's control over its environment.
// long term average of success in executing operators (0-1)
// ideal value of ctrl = cdes which yields nominal wildness

void jhcAliaMood::sm_ctrl (double dt)
{
  double pos, neg, sum, c, f;

  // slow reversion
  ctrl += (cdes - ctrl) * dt / otime;            

  // calculate positive and negative weights
  pos = win + fgood * good;
  neg = lose + fbad * bad;
  if ((sum = pos + neg) <= 0.0)
    return;

  // mix new estimate with running average
  c = pos / sum;
  f = sum / osamp;
  ctrl += __min(f, 1.0) * (c - ctrl);  
}


//= Determine robot's sureness about the analysis of its environment.
// long term average of success of inference rules (0-1)
// ideal value of sure = sdes which yields nominal confidence threshold

void jhcAliaMood::sm_sure (double dt)
{
  double pos, neg, sum, s, f;

  // slow reversion
  sure += (sdes - sure) * dt / rtime;            

  // calculate positive and negative weights
  pos = right + fconf * confirm;
  neg = wrong + fref * refute;
  if ((sum = pos + neg) <= 0.0)
    return;

  // mix new estimate with running average
  s = pos / sum;
  f = sum / rsamp;
  sure += __min(f, 1.0) * (s - sure);  
}


///////////////////////////////////////////////////////////////////////////
//                        Reasoning Adjustment                           //
///////////////////////////////////////////////////////////////////////////

//= Determine how strictly operator preference ordering should be followed.
// the higher the operator success rate, the more experimentation is allowed

void jhcAliaMood::adj_wild () const
{
  double cerr = ctrl - cdes, cf = ((cerr < 0.0) ? -wlo : whi); 
  double w, w0 = atree->RestWild();

  w = w0 + cf * cerr;
  atree->SetWild(w);
}


//= Determine minimum belief threshold for facts derived from rules.
// the higher the rule correctness rate, the weaker the conclusions accepted

void jhcAliaMood::adj_belief () const
{
  double serr = sure - sdes, sf = ((serr < 0.0) ? -blo : bhi); 
  double b, b0 = atree->RestBlf();

  b = b0 + sf * serr;
  atree->SetMinBlf(b);
}


//= Determine how much activity is currently desired.
// weighted sum of deviations from ideal values across four drives
// separate over and under factors in case setpoint toward end of range

void jhcAliaMood::adj_pref () const
{
  double merr = motion - 1.0, serr = social - 1.0;
  double oerr = ctrl - cdes, rerr = sure - sdes;
  double mf = ((merr < 0.0) ? -mlo : mhi), sf = ((serr < 0.0) ? -slo : shi);
  double of = ((oerr < 0.0) ? -olo : ohi), rf = ((rerr < 0.0) ? -rlo : rhi);
  double p, p0 = atree->RestPref(); 

  p = p0 + mf * merr + sf * serr + of * oerr + rf * rerr;
  atree->SetMinPref(p);
}


///////////////////////////////////////////////////////////////////////////
//                      User Communication Data                          //
///////////////////////////////////////////////////////////////////////////

//= Note the robot generated a linguistic output of some length.
// adds samples for total emit time 

void jhcAliaMood::Speak (int len, double hz)
{
  mtim += len / lps;
}


//= Note the user provided linguistic input of some length.
// adds samples for total input time

void jhcAliaMood::Hear (int len, double hz)
{
  itim += len / lps;
}


//= Note that a new operator has been added (by user).

void jhcAliaMood::React (int cnt)
{
  // ignored for now
}


//= Note that new rules have been added by user or through consolidation.

void jhcAliaMood::Infer (int cnt)
{
  // ignored for now
}


///////////////////////////////////////////////////////////////////////////
//                       Body and Environment Data                       //
///////////////////////////////////////////////////////////////////////////

//= Indicate that the base in translating (not rotating) at some rate wrt nominal.

void jhcAliaMood::Travel (double rate)
{
  bspeed = rate;
}


//= Indicate that the hand is traveling (not reorienting) at some rate wrt nominal.

void jhcAliaMood::Reach (double rate)
{
  fspeed = rate;
}


//= Cache battery capacity as a percentage of full.

void jhcAliaMood::Battery (double pct)
{
  energy = pct;
}


//= Record the number of faces currently visible.

void jhcAliaMood::Faces (int cnt)
{
  people = cnt;
}


///////////////////////////////////////////////////////////////////////////
//                        Operator Monitoring                            //
///////////////////////////////////////////////////////////////////////////

//= Note that a new directive has been started.

void jhcAliaMood::OpLaunch ()
{
// ignored for now
}


//= Note that some directive has completed successfully.

void jhcAliaMood::OpWin ()
{
  win++;
}


//= Note that some directive has failed.

void jhcAliaMood::OpLose ()
{
  lose++;
}


//= An operator was available but not tried due to preference threshold.

void jhcAliaMood::OpBelow ()
{
// ignored for now
}


//= Record user praise or disapproval of operator-derived action.

void jhcAliaMood::UserPref (int fb)    
{
  if (fb > 0)
    good += fb;
  else
    bad -= fb;
}


///////////////////////////////////////////////////////////////////////////
//                          Rule Monitoring                              //
///////////////////////////////////////////////////////////////////////////

//= Reccord number of correct and incorrect rule predictions.

void jhcAliaMood::RuleEval (int hit, int miss, double chg) 
{
  right += hit;
  wrong += miss;
  jump  = __max(chg, jump);
}   


//= Note that some rule strength was off by a certain amount.

void jhcAliaMood::RuleAdj (double adj)
{
// ignored for now
}


//= Record user agreement or refutation with a rule-derived fact.

void jhcAliaMood::UserConf (int correct) 
{
  if (correct > 0)
    confirm += correct;
  else
    refute -= correct;
}

