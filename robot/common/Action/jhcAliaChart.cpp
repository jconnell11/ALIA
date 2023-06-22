// jhcAliaChart.cpp : display of statistics and mood for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023 Etaoin Systems
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

#include "Action/jhcAliaChart.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaChart::~jhcAliaChart ()
{
}


//= Default constructor initializes certain values.

jhcAliaChart::jhcAliaChart ()
{
  stat = NULL;
  hz = 30.0;
  ht = 100;
}


///////////////////////////////////////////////////////////////////////////
//                           Graphical Display                           //
///////////////////////////////////////////////////////////////////////////

//= Display memory-related statistics on the screen below last element.
// needs data from jhcAliaStats::Thought() function

void jhcAliaChart::Memory (jhcDisplay& d)
{
  double pk = 1.1;
  int g, m, top;

  // extract relevant limits
  if (stat == NULL)
    return;
  g = (stat->goal).MaxVal();
  m = (stat->hmem).MaxVal();
  top = __max(1, ROUND(pk * m));

  // show graphs
  resize(d);
  d.Graph0(d.BelowX(), d.BelowY(), "Total goals (%d max) and total memory (%d max) over %3.1f secs", g / 100, m / 100, stat->Time(hz));
  d.GraphOver(stat->hmem, top, 2); 
  d.GraphOver(stat->wmem, top, 4);
  d.GraphOver(stat->goal, __max(1, ROUND(pk * g)), 8);
  restore(d);
}


//= Display voice input and output traces on the screen below last element.
// needs data from jhcAliaStats::Speech() function

void jhcAliaChart::Audio (jhcDisplay& d)
{
  if (stat == NULL)
    return;
  resize(d);
  d.Graph0(d.BelowX(), d.BelowY(), "Speech input and output over %3.1f secs", stat->Time(hz));
  d.GraphOver(stat->attn, 20, 2);
  d.GraphOver(stat->talk, 30, 1);
  d.GraphOver(stat->spch, 15, 8); 
  restore(d);
}


//= Display robot physical activity and overall boredom level.
// needs data from jhcAliaStats::Thought() and jhcAliaStats::Speech() functions

void jhcAliaChart::Physical (jhcDisplay& d)
{
  int b, a, pk;

  // extract relevant limits
  if (stat == NULL)
    return;
  b = ROUND(100.0 * stat->bored);
  a = ROUND(100.0 * stat->active);
  pk = a + b;

  // show graphs
  resize(d);
  d.Graph0(d.BelowX(), d.BelowY(), "Activity and robot motion sources over %3.1f secs", stat->Time(hz));
  d.GraphVal(a, pk, 6, 1);
  d.GraphVal(b, pk, 6, 1);
  d.GraphOver(stat->busy, pk, 8); 
  d.GraphOver(stat->emit, 200, 1);
  d.GraphOver(stat->wave, 200, 4); 
  d.GraphOver(stat->walk, 200, 2);
  restore(d);
}


//= Display base motion servo performance on the screen below last element.
// needs data from jhcAliaStats::Drive() function

void jhcAliaChart::Wheels (jhcDisplay& d)
{
  int dc, dv, dpk, tc, tv, tpk;

  // extract relevant limits
  if (stat == NULL)
    return;
  dc = (stat->mcmd).MaxAbs();
  dv = (stat->mips).MaxAbs();
  dpk = __max(dc, dv);
  tc = (stat->rcmd).MaxAbs(); 
  tv = (stat->rdps).MaxAbs();
  tpk = __max(tc, tv);

  // driving speed
  resize(d);
  d.Graph0(d.BelowX(), d.BelowY(), "Move speed (%3.1f ips max) over %3.1f secs", 0.01 * dpk, stat->Time(hz));
  d.GraphVal(0, -dpk, 2);
  d.GraphOver(stat->mcmd, -dpk, 8);
  d.GraphOver(stat->mips, -dpk, 4);

  // turning speed
  d.Graph0(d.BelowX(), d.BelowY(), "Turn speed (%3.1f dps max) over %3.1f secs", 0.01 * tpk, stat->Time(hz));
  d.GraphVal(0, -tpk, 2);
  d.GraphOver(stat->rcmd, -tpk, 8); 
  d.GraphOver(stat->rdps, -tpk, 1); 
  restore(d);
}


//= Display neck motion servo tracking on the screen below last element.
// needs data from jhcAliaStats::Gaze() function

void jhcAliaChart::Neck (jhcDisplay& d)
{
  int pc, pv, ppk, tc, tv, tpk;

  // extract relevant limits
  if (stat == NULL)
    return;
  pc = (stat->pcmd).MaxAbs();
  pv = (stat->pdeg).MaxAbs();
  ppk = __max(1000, __max(pc, pv));
  tc = (stat->tcmd).MaxAbs();
  tv = (stat->tdeg).MaxAbs();
  tpk = __max(1000, __max(tc, tv));

  // panning speed
  resize(d);
  d.Graph0(d.BelowX(), d.BelowY(), "Pan (+/- %1.0f deg max) over %3.1f secs", 0.01 * ppk, stat->Time(hz));
  d.GraphVal(0, -ppk, 2);
  d.GraphOver(stat->pcmd, -ppk, 8);
  d.GraphOver(stat->pdeg, -ppk, 4);

  // tilting speed
  d.Graph0(d.BelowX(), d.BelowY(), "Tilt (+/- %1.0f deg max) over %3.1f secs", 0.01 * tpk, stat->Time(hz));
  d.GraphVal(0, -tpk, 2);
  d.GraphOver(stat->tcmd, -tpk, 8);
  d.GraphOver(stat->tdeg, -tpk, 1); 
  restore(d);
}


//= Temporarily alter size of graphs generated by display.

void jhcAliaChart::resize (jhcDisplay& d) 
{
  w0 = d.gwid;
  h0 = d.ght;
  d.gwid = stat->Len();
  d.ght  = ht;
}


//= Change display back to normal graph size.

void jhcAliaChart::restore (jhcDisplay& d) const
{
  d.gwid = w0;
  d.ght  = h0;
}


///////////////////////////////////////////////////////////////////////////
//                             Text Display                              //
///////////////////////////////////////////////////////////////////////////

//= List internal status variables controlling behavior.

void jhcAliaChart::Mood (jhcDisplay& d) const
{
  int b;

  if (stat == NULL)
    return;
  b = ROUND(100.0 * stat->fidget / (stat->active + stat->bored));
  d.StringBelow("%3d \tCuriosity",  ROUND(100.0 * stat->opt));
  d.StringBelow("%3d \tConfidence", ROUND(100.0 * stat->cred));
  d.StringBelow("%3d \tActive",  __min(b, 100));
//  d.StringBelow("%3d \tLonely", ROUND(50.0 * (2.0 - stat->urel)));
}


//= Generate a string of behavior variables to be printed out elsewhere.

const char *jhcAliaChart::MoodTxt ()
{
  int b;

  *msg = '\0';
  if (stat == NULL)
    return msg;
  b = ROUND(100.0 * stat->fidget / (stat->active + stat->bored));
  sprintf_s(msg, "Curiosity %d, Confidence %d, Active %d",
            ROUND(100.0 * stat->opt), ROUND(100.0 * stat->cred), __min(b, 100));
  return msg;
}




