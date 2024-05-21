// jhcAliaStats.cpp : monitors internal processes of ALIA reasoner
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

  // reconfigure threshold data
  blf.SetSize(n);
  pref.SetSize(n);
  wild.SetSize(n);

  // reconfigure speech data
  spch.SetSize(n);
  talk.SetSize(n);
  attn.SetSize(n);

  // reconfigure motion data
  walk.SetSize(n);
  wave.SetSize(n);
  emit.SetSize(n);
  mdrv.SetSize(n);

  // reconfigure social data
  hear.SetSize(n);
  face.SetSize(n);
  sdrv.SetSize(n);

  // reconfigure satisfaction data
  val.SetSize(n);
  sad.SetSize(n);
  surp.SetSize(n);

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

  // clear threshold data
  blf.Fill(0);
  pref.Fill(0);
  wild.Fill(0);

  // clear speech data
  spch.Fill(0);
  talk.Fill(0);
  attn.Fill(0);

  // clear motion data
  walk.Fill(0);
  wave.Fill(0);
  emit.Fill(0);
  mdrv.Fill(0);

  // clear social data
  hear.Fill(0);
  face.Fill(0);
  sdrv.Fill(0);

  // clear satisfaction data
  val.Fill(0);
  sad.Fill(0);
  surp.Fill(0);

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
// display with jhcAliaChart functions Memory and Mood

void jhcAliaStats::Thought (jhcAliaCore *core)
{
  jhcActionTree *atree = &(core->atree);
  int w = atree->WmemSize();
  
  // action tree data (number of goals lingers)
  goal.Scroll(ROUND(100.0 * (core->mood).Busy()));
  wmem.Scroll(100 * w);
  hmem.Scroll(100 * (w + atree->HaloSize()));

  // core thresholds
  bth = atree->MinBlf();
  pth = atree->MinPref();
  wex = atree->Wild();

  // threshold history
  blf.Scroll(ROUND(1000.0 * bth));
  pref.Scroll(ROUND(1000.0 * pth));
  wild.Scroll(ROUND(1000.0 * wex));
}


//= Add new data point for speech state.
// sprc: 0 = silent, 1 = speech heard, 2 = full recognition
// display with jhcAliaChart function Audio

void jhcAliaStats::Speech (int sprc, int tts, int gate)
{
  spch.Scroll(10 * sprc);
  talk.Scroll((tts > 0) ? 10 : 0);
  attn.Scroll(10 * gate);
}


//= Add new data for computation of emotional state.
// display with jhcAliaChart functions Physical and Valence

void jhcAliaStats::Affect (const jhcAliaMood& mood)
{
  double bsp, fsp, mt, it;
  int np;

  // cache thresholds
  mok = mood.Active();

  // get raw input values 
  mood.BodyData(bsp, fsp, mt);
  np = mood.SocialData(it);

  // add motion graph data (rate 1 -> 1000)
  walk.Scroll(ROUND(1000.0 * bsp));
  wave.Scroll(ROUND(1000.0 * fsp));
  emit.Scroll((mt > 0.0) ? 1000 : 0);
  mdrv.Scroll(ROUND(1000.0 * mood.Motion()));

  // add social graph data 
  hear.Scroll((it > 0.0) ? 1000 : 0);
  face.Scroll(ROUND(1000.0 * np));
  sdrv.Scroll(ROUND(1000.0 * mood.Social()));

  // add satisfaction and surprise data
  val.Scroll(ROUND(1000.0 * mood.Valence()));
  sad.Scroll(ROUND(1000.0 * mood.Unhappy()));
  surp.Scroll(ROUND(1000.0 * mood.Surprise()));
}


//= Add new data points for base commands and actual speeds.
// depends on RWI platform - display with jhcAliaChart function Wheels

void jhcAliaStats::Drive (double m, double mest, double r, double rest)
{
  mcmd.Scroll(ROUND(100.0 * m));
  mips.Scroll(ROUND(100.0 * mest));
  rcmd.Scroll(ROUND(100.0 * r));
  rdps.Scroll(ROUND(100.0 * rest));
}


//= Add new data points for neck commands and actual positions.
// depends on RWI platform - display with jhcAliaChart function Neck 

void jhcAliaStats::Gaze (double p, double pest, double t, double test)
{
  pcmd.Scroll(ROUND(100.0 * p));
  pdeg.Scroll(ROUND(100.0 * pest));
  tcmd.Scroll(ROUND(100.0 * t));
  tdeg.Scroll(ROUND(100.0 * test));
}

