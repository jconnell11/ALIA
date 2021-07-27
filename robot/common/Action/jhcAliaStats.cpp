// jhcAliaStats.cpp : monitors internal processes of ALIA reasoner
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

#include "Interface/jhcDisplay.h"      // common video - since only spec'd as class in header

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
  ht = 100;
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
  fill = 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Clear all accumulated data.

void jhcAliaStats::Reset ()
{
  // clear core data
  goal.Fill(0);
  wmem.Fill(0);
  hmem.Fill(0);

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

  // reset state
  w0 = sz;
  h0 = ht;
  fill = 0;
}


//= Add new data points to graphs based on current operation of ALIA core.
// need to call Shift once all data from this cycle are entered
// display with Memory 

void jhcAliaStats::Thought (const jhcActionTree& atree)
{
  int w = atree.WmemSize();

  goal.Scroll(fill, 100 * atree.NumGoals());
  wmem.Scroll(fill, 100 * w);
  hmem.Scroll(fill, 100 * (w + atree.HaloSize()));
}


//= Add new data points for base commands and actual speeds.
// need to call Shift once all data from this cycle are entered
// display with Wheels

void jhcAliaStats::Drive (double m, double mest, double r, double rest)
{
  mcmd.Scroll(fill, ROUND(100.0 * m));
  mips.Scroll(fill, ROUND(100.0 * mest));
  rcmd.Scroll(fill, ROUND(100.0 * r));
  rdps.Scroll(fill, ROUND(100.0 * rest));
}


//= Add new data points for neck commands and actual positions.
// need to call Shift once all data from this cycle are entered
// display with Neck

void jhcAliaStats::Gaze (double p, double pest, double t, double test)
{
  pcmd.Scroll(fill, ROUND(100.0 * p));
  pdeg.Scroll(fill, ROUND(100.0 * pest));
  tcmd.Scroll(fill, ROUND(100.0 * t));
  tdeg.Scroll(fill, ROUND(100.0 * test));
}


///////////////////////////////////////////////////////////////////////////
//                           Graphical Display                           //
///////////////////////////////////////////////////////////////////////////

//= Display memory-related statistics on the screen at some grid position.
// needs data from Thought function

void jhcAliaStats::Memory (jhcDisplay *d, int i, int j, double hz) 
{
  double pk = 1.1;
  int x, y, g = goal.MaxVal(), m = hmem.MaxVal(), top = __max(1, ROUND(pk * m));

  if (corner(x, y, i, j, d) <= 0)
    return;
  d->Graph(hmem, x, y, top, 2, "Goals (%d) and memory (%d) over %3.1f secs", g / 100, m / 100, sz / hz);
  d->GraphOver(wmem, top, 4);
  d->GraphOver(goal, __max(1, ROUND(pk * g)), 8);
  restore(d);
}


//= Display base motion servo performance on the screen at some grid position.
// needs data from Drive function

void jhcAliaStats::Wheels (jhcDisplay *d, int i, int j) 
{
  int mc = mcmd.MaxAbs(), mv = mips.MaxAbs(), rc = rcmd.MaxAbs(), rv = rdps.MaxAbs();
  int x, y, mpk = __max(mc, mv), rpk = __max(rc, rv);

  // driving speed
  if (corner(x, y, i, j, d) <= 0)
    return;
  d->Graph0(x, y, "Move speed (%3.1f ips)", 0.01 * mpk);
  d->GraphVal(0, -mpk, 2);
  d->GraphOver(mcmd, -mpk, 8);
  d->GraphOver(mips, -mpk, 4);

  // turning speed
  d->Graph0(d->BelowX(), d->BelowY(), "Turn speed (%3.1f dps)", 0.01 * rpk);
  d->GraphVal(0, -rpk, 2);
  d->GraphOver(rcmd, -rpk, 8); 
  d->GraphOver(rdps, -rpk, 1); 
  restore(d);
}


//= Display neck motion servo tracking on the screen at some grid position.
// needs data from Gazee function

void jhcAliaStats::Neck (jhcDisplay *d, int i, int j) 
{
  int pc = pcmd.MaxAbs(), pv = pdeg.MaxAbs(), tc = tcmd.MaxAbs(), tv = tdeg.MaxAbs();
  int x, y, ppk = __max(pc, pv), tpk = __max(tc, tv);

  // panning speed
  if (corner(x, y, i, j, d) <= 0)
    return;
  d->Graph0(x, y, "Pan (+/- %1.0f deg)", 0.01 * ppk);
  d->GraphVal(0, -ppk, 2);
  d->GraphOver(pcmd, -ppk, 8);
  d->GraphOver(pdeg, -ppk, 4);

  // tilting speed
  d->Graph0(d->BelowX(), d->BelowY(), "Tilt (+/- %1.0f deg)", 0.01 * tpk);
  d->GraphVal(0, -tpk, 2);
  d->GraphOver(tcmd, -tpk, 8);
  d->GraphOver(tdeg, -tpk, 1); 
  restore(d);
}


//= Get a screen position based on grid or just use below if bad grid.
// use negative i for adjacent, negative j for below
// returns 1 if okay, 0 for problem

int jhcAliaStats::corner (int& x, int& y, int i, int j, jhcDisplay *d) 
{
  if (d == NULL)
    return 0;
  w0 = d->gwid;
  h0 = d->ght;
  d->gwid = sz;
  d->ght  = ht;
  d->ScreenPos(x, y, i, j);
  return 1;
}


//= Change display back to normal graph size.

void jhcAliaStats::restore (jhcDisplay *d) const
{
  d->gwid = w0;
  d->ght  = h0;
}

