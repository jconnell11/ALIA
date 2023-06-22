// jhcAliaStats.h : monitors internal processes of ALIA reasoner
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

#ifndef _JHCALIASTATS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIASTATS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"               // common video


//= Monitors internal processes of ALIA reasoner.

class jhcAliaStats
{
// PRIVATE MEMBER VARIABLES
private:
  int sz;                              // graph length
  

// PUBLIC MEMBER VARIABLES
public:
  jhcArr goal, wmem, hmem;             // core operations
  jhcArr spch, talk, attn;             // speech state
  jhcArr busy, walk, wave, emit;       // activity monitor
  jhcArr mcmd, mips, rcmd, rdps;       // wheel servos
  jhcArr pcmd, pdeg, tcmd, tdeg;       // neck servos
  double cred, opt;                    // belief and preference
  double active, bored, fidget;        // cached mood


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaStats ();
  jhcAliaStats (int n =600);
  void SetSize (int n =600);
  int Len () const {return sz;}
  double Time (double hz) const {return(sz / hz);}
 
  // data collection
  void Reset ();
  void Thought (class jhcAliaCore *core);
  void Speech (int sprc, int tts, int gate);
  void Motion (const class jhcAliaMood& mood);
  void Drive (double m, double mest, double r, double rest);
  void Gaze (double p, double pest, double t, double test);


// PRIVATE MEMBER FUNCTIONS
private:


};


#endif  // once




