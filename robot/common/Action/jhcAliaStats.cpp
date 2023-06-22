// jhcAliaStats.cpp : monitors internal processes of ALIA reasoner
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

#include "Action/jhcAliaCore.h"        // common robot - since only spec'd as class in header

#include "Action/jhcAliaStats.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaStats::~jhcAliaStats ()
{
}


//= Default constructor initializes certain values.

jhcAliaStats::jhcAliaStats (int n)
{
  SetSize(n);
  Reset();
}


//= Set the length of the history (typically 30 samples per second).

void jhcAliaStats::SetSize (int n)
{
  // reconfigure core data
  goal.SetSize(n);
  wmem.SetSize(n);
  hmem.SetSize(n);

  // reconfigure speech data
  spch.SetSize(n);
  talk.SetSize(n);
  attn.SetSize(n);

  // reconfigure motion data
  busy.SetSize(n);
  walk.SetSize(n);
  wave.SetSize(n);
  emit.SetSize(n);
  
  // reconfigure base data
  mcmd.SetSize(n);
  mips.SetSize(n);
  rcmd.SetSize(n);
  rdps.SetSize(n);

  // reconfigure neck data
  pcmd.SetSize(n);
  pdeg.SetSize(n);
  tcmd.SetSize(n);
  tdeg.SetSize(n);

  // record new size
  sz = n;
}


///////////////////////////////////////////////////////////////////////////
//                             Data Collection                           //
///////////////////////////////////////////////////////////////////////////

//= Clear all accumulated data.

void jhcAliaStats::Reset ()
{
  // clear core data
  goal.Fill(0);
  wmem.Fill(0);
  hmem.Fill(0);

  // clear speech data
  spch.Fill(0);
  talk.Fill(0);
  attn.Fill(0);

  // clear motion data
  busy.Fill(0);
  walk.Fill(0);
  wave.Fill(0);
  emit.Fill(0);

  // clear base data
  mcmd.Fill(0);
  mips.Fill(0);
  rcmd.Fill(0);
  rdps.Fill(0);

  // clear neck data
  pcmd.Fill(0);
  pdeg.Fill(0);
  tcmd.Fill(0);
  tdeg.Fill(0);
}


//= Add new data points to graphs based on current operation of ALIA core.
// display with jhcAliaChart::Memory() and jhcAliaChart::Mood()

void jhcAliaStats::Thought (jhcAliaCore *core)
{
  jhcActionTree *atree = &(core->atree);
  int w = atree->WmemSize();
  
  // action tree data
  goal.Scroll(100 * atree->NumGoals());
  wmem.Scroll(100 * w);
  hmem.Scroll(100 * (w + atree->HaloSize()));

  // core thresholds
  cred = 1.0 - atree->MinBlf();
  opt  = 1.0 - atree->MinPref();
}


//= Add new data point for speech state.
// sprc: 0 = silent, 1 = speech heard, 2 = full recognition
// display with jhcAliaChart::Audio()

void jhcAliaStats::Speech (int sprc, int tts, int gate)
{
  spch.Scroll(10 * sprc);
  talk.Scroll((tts > 0) ? 10 : 0);
  attn.Scroll(10 * gate);
}


//= Add new data for overall body motion (overall, base, finger, mouth).
// display with jhcAliaChart::Physical() and jhcAliaChart::Mood()

void jhcAliaStats::Motion (const jhcAliaMood& mood)
{
  double bsp, fsp, msp;

  // cache values and thresholds
  mood.BodyData(bsp, fsp, msp);
  fidget = mood.Active();
  active = mood.ActiveLvl();
  bored = mood.BoredLvl();

  // add graph data
  busy.Scroll(ROUND(100.0 * fidget));
  walk.Scroll(ROUND(10.0 * bsp));
  wave.Scroll(ROUND(10.0 * fsp));
  emit.Scroll(ROUND(10.0 * msp));
}


//= Add new data points for base commands and actual speeds.
// depends on RWI platform - display with jhcAliaChart::Wheels() 

void jhcAliaStats::Drive (double m, double mest, double r, double rest)
{
  mcmd.Scroll(ROUND(100.0 * m));
  mips.Scroll(ROUND(100.0 * mest));
  rcmd.Scroll(ROUND(100.0 * r));
  rdps.Scroll(ROUND(100.0 * rest));
}


//= Add new data points for neck commands and actual positions.
// depends on RWI platform - display with jhcAliaChart::Neck() 

void jhcAliaStats::Gaze (double p, double pest, double t, double test)
{
  pcmd.Scroll(ROUND(100.0 * p));
  pdeg.Scroll(ROUND(100.0 * pest));
  tcmd.Scroll(ROUND(100.0 * t));
  tdeg.Scroll(ROUND(100.0 * test));
}

