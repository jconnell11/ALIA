// jtimer.cpp : poor man's profiler for single-threaded non-recursive code
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2019 IBM Corporation
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

#include <windows.h>

#include <stdio.h>
#include <direct.h>

#include "Interface/jms_x.h"

#include "Interface/jtimer.h"


// forward declaration of report helper functions

static FILE *jtimer_file (const char *fname, int full);
static double jtimer_freq ();
static int jtimer_depth (int focus, int lvl);
static void jtimer_lines (FILE *out, LONGLONG all, double f, int focus, int lvl, int depth);


///////////////////////////////////////////////////////////////////////////

//= Maximum number of possible functions to timer.

#define TIMERS 100


//= Persistent global array of names associated with each timer.

static char name[TIMERS][80];


//= Persistent global array of parent for each timer.

static int under[TIMERS];


//= Persistent global array of most recent function start times.

static LARGE_INTEGER start[TIMERS];


//= Persistent global array of total time in each function.

static LARGE_INTEGER total[TIMERS];


//= Persistent global array of total calls to each function.

static int count[TIMERS];


//= Persistent global array telling whether function has been reported.

static int done[TIMERS];


///////////////////////////////////////////////////////////////////////////

//= Clear data associated with all timers.

void jtimer_clr ()
{
  int i;

  for (i = 0; i < TIMERS; i++)
  {
    name[i][0] = '\0';
    under[i] = -2;
    total[i].QuadPart = 0;
    start[i].QuadPart = 0;
    count[i] = 0;
  }
}


//= Call this at the beginning of a function (before any local assignments).
// if no name is given then timer number will be used

void jtimer (int n, const char *fcn)
{
  int i, diff, best, win;

  // check for valid id
  if ((n < 0) || (n >= TIMERS))
    return;

  // bind name if none so far
  if (name[n][0] == '\0')
  {
    if (fcn != NULL)
      strcpy_s(name[n], fcn);
    else
      sprintf_s(name[n], "jtimer %d", n);
  }

  // record entrance time
  QueryPerformanceCounter(start + n);

  // infer parent if not yet assigned
  if (under[n] == -2)
  {
    win = -1;
    diff = 0;
    for (i = 0; i < TIMERS; i++)
      if ((i != n) && (start[i].QuadPart != 0))  // look for active calls
      {
        diff = (int)(start[n].QuadPart - start[i].QuadPart);
        if ((win < 0) || (diff <= best))
        {
          win = i;
          best = diff;
        }
      }
    under[n] = win;  // may be -1 if nothing active
  }
}


//= Call this just before leaving a function.
// if there are multiple return points this should be called at each
// default argument of -1 closes everything that is open

void jtimer_x (int n)
{
  LARGE_INTEGER now;
  int i;

  // possibly close all timers still open
  if (n < 0)
  {
    QueryPerformanceCounter(&now);
    for (i = 0; i < TIMERS; i++)
      if (start[i].QuadPart != 0)
      {
        // increment final statistics
        total[i].QuadPart += (now.QuadPart - start[i].QuadPart);
        start[i].QuadPart = 0;
        count[i] += 1;
      }
    return;
  }

  // check for valid id and suitable entrance
  if ((n < TIMERS) && (start[n].QuadPart != 0))
  {
    QueryPerformanceCounter(&now);
    total[n].QuadPart += (now.QuadPart - start[n].QuadPart);
    start[n].QuadPart = 0;
    count[n] += 1;
  }
}


///////////////////////////////////////////////////////////////////////////

//= Generate a file containing a sorted list of function statistics.
// this is all wall-clock time, so if system is loaded these are longer
// if tree > 0 then orders by call tree else simply creates a flat list
// if no name is given then uses current directory and "timing"
// unless full > 0 then tacks on "_MMDDYY_HHMM.txt"
// Note: does not automatically close calls, use j_timer() instead

int jtimer_rpt (int tree, const char *fname, int full)
{
  FILE *out;
  LARGE_INTEGER big;
  double f;
  int i, deep = 0;

  if (tree > 0)
  {
    // fix tree structure for functions with missing parents
    for (i = 0; i < TIMERS; i++)
      if ((under[i] < 0) || (under[i] >= TIMERS) || (count[under[i]] <= 0))
        under[i] = -1;

    // find max call tree depth
    for (i = 0; i < TIMERS; i++)
      done[i] = 0;
    deep = jtimer_depth(-1, 0);
  }
  else
    for (i = 0; i < TIMERS; i++)    // flatten tree
      under[i] = -1;

  // open output file and generate report header
  if ((out = jtimer_file(fname, full)) == NULL)
    return 0;
  fprintf(out, "    total ms    pct max    calls     avg ms");
  for (i = 0; i < deep; i++)
    fprintf(out, "  ");
  fprintf(out, "  timer name\n");
  fprintf(out, " -------------  -------  ---------  -------");
  for (i = 0; i < deep; i++)
    fprintf(out, "--");
  fprintf(out, "  ------------------------\n");

  // find max time in any function and measure timer frequency
  big.QuadPart = 0;
  for (i = 0; i < TIMERS; i++)
    if (total[i].QuadPart > big.QuadPart)
      big.QuadPart = total[i].QuadPart;
  f = jtimer_freq();

  // generate report lines then cleanup
  for (i = 0; i < TIMERS; i++)
    done[i] = 0;
  jtimer_lines(out, big.QuadPart, f, -1, 0, deep);
  fclose(out);
  return 1; 
}


//= Create new report file name based on current time.

static FILE *jtimer_file (const char *fname, int full)
{
  char tmp[80], fn[500];
  FILE *out = NULL;

  if ((full > 0) && (fname != NULL))
    strcpy_s(fn, fname);
  else
  {
    // possibly create directory
    if (_chdir("timing") == 0)
      _chdir("..");
    else
      _mkdir("timing");

    // make up name based on time
    sprintf_s(fn, "timing/%s_%s.txt", ((fname != NULL) ? fname : "timing"), jms_date(tmp));
  }
  fopen_s(&out, fn, "w");
  return out;
}


//= Determine actual timer count speed.

static double jtimer_freq ()
{
  LARGE_INTEGER freq, t0, t1;
  double f, fmax;
  int i, ms = 100;

  // validate rough timer frequency
  QueryPerformanceCounter(&t0);
  Sleep(ms);
  QueryPerformanceCounter(&t1);
  fmax = (1000.0 * (t1.QuadPart - t0.QuadPart)) / ms;

  // get a reasonable value for actual frequency 
  QueryPerformanceFrequency(&freq);
  f = (double)(freq.QuadPart);
  if (f > fmax)
  {
    i = ROUND(f / fmax);
    f /= i;
  }
  return f;
}


//= Find maximum depth of call tree (for pct column width).

static int jtimer_depth (int focus, int lvl)
{
  int i, sub, depth = lvl - 1;

  while (1)
  {
    // find next call under current focus (if any)
    for (i = 0; i < TIMERS; i++)
      if ((done[i] <= 0) && (under[i] == focus))
        break;

    // see if all functions have been examined
    if (i >= TIMERS)
      return depth;
    depth = __max(depth, lvl);
    done[i] = 1;

    // look for any subfunctions directly below this one
    sub = jtimer_depth(i, lvl + 1);
    depth = __max(depth, sub);
  }
}

     
//= Find next relevant functions to generate lines in the report.

static void jtimer_lines (FILE *out, LONGLONG all, double f, int focus, int lvl, int depth)
{
  LARGE_INTEGER best;
  int i, win;

  while (1)
  {
    // find next biggest time hog under current focus (if any)
    best.QuadPart = 0;
    for (i = 0; i < TIMERS; i++)
      if (done[i] <= 0)
        if (under[i] == focus)
          if (total[i].QuadPart > best.QuadPart)
          {
            best.QuadPart = total[i].QuadPart;
            win = i;
          }

    // see if all functions have been reported
    if (best.QuadPart <= 0)
      return;
    done[win] = 1;

    // generate report line for this function
    if (out != NULL)
    {
      fprintf(out, " %13.2f  %6.2f   %9d  ", 
              (1000.0 * total[win].QuadPart) / f, 
              (100.0  * total[win].QuadPart) / all,
              count[win]);
      for (i = 0; i < lvl; i++)              // indent
        fprintf(out, "  ");
      fprintf(out, "%7.2f", (1000.0 * total[win].QuadPart) / (count[win] * f));
      for (i = depth - lvl; i > 0; i--)      // tab
        fprintf(out, "  ");
      fprintf(out, "  %s\n", name[win]);
    }

    // list any subfunctions directly below this one
    jtimer_lines(out, all, f, win, lvl + 1, depth);
  }
}


 


