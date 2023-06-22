// jhcAliaMood.h : maintains slow changing state variables for ALIA system
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

#ifndef _JHCALIAMOOD_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIAMOOD_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"               // common video
#include "Data/jhcParam.h"

#include "API/jhcAliaNote.h"           // common audio


//= Maintains slow changing state variables for ALIA system.
// collects data about many activities (but some ignored)
// drives affect externally through events linked to operators
// unlike jhcActionTree which servos MinBlf and MinPref internally

class jhcAliaMood
{
// PRIVATE MEMBER VARIABLES
private:
  // for new NOTEs and MinBlf and MinPref
  class jhcActionTree *rpt; 

  // raw data collection
  UL32 now;
  int win, lose;

  // rule changes
  double surp;

  // collected body data 
  double bspeed, fspeed, mspeed, pct;

  // mental level
  double busy;
  int yikes;

  // action level
  double fidget;
  UL32 kvetch;
  int blah;

  // speech level
  double input;
  UL32 call;
  int lament;

  // battery level
  int power;
  UL32 moan;
  int delay;


// PRIVATE MEMBER PARAMETERS
private:
  // belief adjustment parameters
  double right, wrong, fyes, fno, bfade;

  // preference adjustment parameters
  double miss, dud, fgood, fbad, pfade;

  // activity parameters
  double frantic, engaged, idle, active, bored, nag, btime, ftime;

  // social parameters
  double attn, sat, prod, ramp, needy, fade;
  int bereft;

  // power parameters
  double repeat, urgent, calm;
  int fresh, tired, slug, psamp;
 

// PUBLIC MEMBER VARIABLES
public:
  int noisy;                 // debugging messages
  jhcParam bps, pps, aps, sps, tps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaMood ();
  jhcAliaMood ();
  void Bind (class jhcActionTree& t) {rpt = &t;}
  double ActiveLvl () const {return active;}
  double BoredLvl () const  {return bored;}

  // processing parameter bundles 
  int LoadCfg (const char *fname =NULL);
  int SaveCfg (const char *fname) const;

  // basic mood variables
  double Busy () const     {return busy;}
  double Active () const   {return fidget;}
  double Interact () const {return input;}
  double Energy () const   {return power;}
  double Surprise () const {return surp;}
  double BodyData (double& bsp, double& fsp, double& msp) const
    {bsp = bspeed; fsp = fspeed; msp = mspeed; return pct;}

  // main functions
  void Reset ();
  void Update (int nag =1);

  // global threshold adjustment
  void UserMinBlf (int correct =1); 
  void BumpMinBlf (int hit, int miss);
  void UserMinPref (int good =1);   
  void BumpMinPref (int inc); 

  // operator invocation 
  void Launch ();
  void Win ();
  void Lose ();

  // internal threshold servoing
  void Believe (double adj =0.1);
/*
  void Prefer (double adj =0.1);
  void Predict (double chg =0.1);      
  void Behave (double chg =0.1);       
  void Confirm (double chg =0.0);   
  void Endorse (double chg =0.0);   
*/
  // user communication recording
  void Speak (int len =1);
  void Hear (int len =1);
  void Infer (int cnt =1);
  void React (int cnt =1);

  // body activity recording
  void Body (double bips, double fips, int pct);
  void Emit (int out);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int blf_params (const char *fname);
  int pref_params (const char *fname);
  int bored_params (const char *fname);
  int lonely_params (const char *fname);
  int tired_params (const char *fname);

  // main functions
  void clr_evts ();
  int chk_busy ();
  int chk_antsy ();
  int chk_lonely ();
  int chk_tired ();

  // global threshold adjustment
  void adj_blf (double s);
  void adj_pref (double p);


};


#endif  // once




