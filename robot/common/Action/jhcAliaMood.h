// jhcAliaMood.h : maintains slow changing state variables for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021-2022 Etaoin Systems
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

#include "Reasoning/jhcAliaNote.h"     // common audio


//= Maintains slow changing state variables for ALIA system.
// collects data about many activities (but some ignored)

class jhcAliaMood
{
// PRIVATE MEMBER VARIABLES
private:
  // raw data collection
  UL32 now;
  int win, lose;

  // rule changes
  double surp;

  // activity level
  double busy;
  UL32 kvetch;
  int yikes, blah;

  // interaction level
  double input;
  UL32 call;
  int lament;

  // battery level
  int power;
  UL32 moan;
  int delay;
 

// PUBLIC MEMBER VARIABLES
public:
  // activity parameters
  jhcParam bps;
  int very;
  double frantic, engaged, idle, bored, nag, tc;

  // social parameters
  jhcParam sps;
  int bereft;
  double attn, sat, prod, ramp, needy, fade;

  // power parameters
  jhcParam tps;
  int fresh, tired, slug, psamp;
  double repeat, urgent, calm;

// should move to jhcStats
jhcArr bhist;
int sz, fill;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaMood ();
  jhcAliaMood ();
  double Busy () const     {return busy;}
  double Interact () const {return input;}
  double Energy () const   {return power;}
  double Surprise () const {return surp;}
//  int NumVars () const {return 0;}
//  void Levels (double now[]) const;

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  void Reset ();
  void Update (jhcAliaNote& rpt, int nag =1);
//  double Preference (const double chg[]) const;
//  void Adjust (double chg[], const double start[]);

  // data collection
  void Launch ();
  void Win ();
  void Lose ();
  void Speak (int len =1);
  void Hear (int len =1);
  void Infer (int cnt =1);
  void React (int cnt =1);
  void Believe (double miss =0.5);
  void Prefer (double adj =0.1);
//  void Praise (double deg =0.5);
//  void Rebuke (double deg =0.5);
  void Energy (int pct =100);
  void Walk (double sp =1.0);
  void Wave (double sp =0.5);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int busy_params (const char *fname);
  int lonely_params (const char *fname);
  int tired_params (const char *fname);

  // main functions
  void clr_data ();
  int chk_busy (jhcAliaNote& rpt);
  int chk_lonely (jhcAliaNote& rpt);
  int chk_tired (jhcAliaNote& rpt);

};


#endif  // once




