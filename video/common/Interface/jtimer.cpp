// jtimer.cpp : poor man's profiler for single-threaded non-recursive code
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2019 IBM Corporation
// Copyright 2023-2024 Etaoin Systems
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

#ifndef __linux__

  #include <windows.h>
  #include <direct.h>
  #define val64(v) (v.QuadPart)
  #define set64(v, n) (v.QuadPart = (LONGLONG)(n))
  #define QPF(vptr) QueryPerformanceFrequency(vptr);
  #define QPC(vptr) QueryPerformanceCounter(vptr)

#else

  #include <time.h>
  #define LONGLONG unsigned long long
  #define LARGE_INTEGER long long
  #define val64(v) (v)
  #define set64(v, n) (v = (LARGE_INTEGER)(n))
  #define QPF(vptr) (*(vptr) = 1000000000)
  #define QPC(vptr) {timespec ts;  \
                     clock_gettime(CLOCK_BOOTTIME, &ts);  \
                     *(vptr) = (long long)(ts.tv_sec * 1000000000 + ts.tv_nsec);}

#endif

#include <stdio.h>

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


//= Persistent global array of maximum time in each function.

static int maxtime[TIMERS];


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
    under[i]   = -2;
    count[i]   = 0;
    maxtime[i] = 0;
    set64(total[i], 0);
    set64(start[i], 0);
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
  QPC(start + n);

  // infer parent if not yet assigned
  if (under[n] == -2)
  {
    win = -1;
    diff = 0;
    for (i = 0; i < TIMERS; i++)
      if ((i != n) && (val64(start[i]) != 0))    // look for active calls
      {
        diff = (int)(val64(start[n]) - val64(start[i]));
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
  int i, len;

  // possibly close all timers still open
  if (n < 0)
  {
    QPC(&now);
    for (i = 0; i < TIMERS; i++)
      if (val64(start[i]) != 0)
      {
        // increment final statistics
        len = (int)(val64(now) - val64(start[i]));
        maxtime[i] = __max(maxtime[i], len);
        set64(total[i], val64(total[i]) + val64(now) - val64(start[i]));
        set64(start[i], 0);
        count[i] += 1;
      }
    return;
  }

  // check for valid id and suitable entrance
  if ((n < TIMERS) && (val64(start[n]) != 0))
  {
    QPC(&now);
    len = (int)(val64(now) - val64(start[n]));
    maxtime[n] = __max(maxtime[n], len);
    set64(total[n], val64(total[n]) + val64(now) - val64(start[n]));
    set64(start[n], 0);
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
  fprintf(out, "   max ms  timer name\n");
  fprintf(out, " -------------  -------  ---------  -------");
  for (i = 0; i < deep; i++)
    fprintf(out, "--");
  fprintf(out, "  -------  ------------------------\n");

  // find max time in any function and measure timer frequency
  set64(big, 0);
  for (i = 0; i < TIMERS; i++)
    if (val64(total[i]) > val64(big))
      set64(big, val64(total[i]));
  f = jtimer_freq();

  // generate report lines then cleanup
  for (i = 0; i < TIMERS; i++)
    done[i] = 0;
  jtimer_lines(out, val64(big), f, -1, 0, deep);
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
    if (_chdir("timing") != 0)
      _mkdir("timing");
    else if (_chdir("..") != 0)        // should never fail
      return NULL;

    // make up name based on time
    sprintf_s(fn, "timing/%s_%s.txt", ((fname != NULL) ? fname : "timing"), jms_date(tmp));
  }
  if (fopen_s(&out, fn, "w") != 0)
    out = NULL;
  return out;
}


//= Determine actual timer count speed.

static double jtimer_freq ()
{
  LARGE_INTEGER freq, t0, t1;
  double f, fmax;
  int i, ms = 100;

  // validate rough timer frequency
  QPC(&t0);
  jms_sleep(ms);
  QPC(&t1);
  fmax = (1000.0 * (double)(val64(t1) - val64(t0))) / ms;

  // get a reasonable value for actual frequency 
  QPF(&freq);
  f = (double) val64(freq);
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
    set64(best, 0);
    for (i = 0; i < TIMERS; i++)
      if (done[i] <= 0)
        if (under[i] == focus)
          if (val64(total[i]) > val64(best))
          {
            set64(best, val64(total[i]));
            win = i;
          }

    // see if all functions have been reported
    if (val64(best) <= 0)
      return;
    done[win] = 1;

    // generate report line for this function
    if (out != NULL)
    {
      fprintf(out, " %13.2f  %6.2f   %9d  ", 
              (1000.0 * (double) val64(total[win])) / f, 
              (100.0  * (double) val64(total[win])) / (double) all,
              count[win]);
      for (i = 0; i < lvl; i++)              // indent
        fprintf(out, "  ");
      fprintf(out, "%7.2f", (1000.0 * (double) val64(total[win])) / (count[win] * f));
      for (i = depth - lvl; i > 0; i--)      // tab
        fprintf(out, "  ");
      fprintf(out, "  %7.2f  %s\n", (1000.0 * maxtime[win]) / f, name[win]);
    }

    // list any subfunctions directly below this one
    jtimer_lines(out, all, f, win, lvl + 1, depth);
  }
}


///////////////////////////////////////////////////////////////////////////

//= Utility that gives current timestamp for use with jtimer_secs.
// granularity is sub-microsecond referenced to some random start 

UL64 jtimer_now ()
{
  LARGE_INTEGER t0;

  QPC(&t0);
  return val64(t0);
}


//= Utility computes difference in seconds from random timestamp t0 to now.

double jtimer_secs (UL64 t0)
{
  LARGE_INTEGER freq, t1;

  QPC(&t1);
  QPF(&freq);
  return((double)(val64(t1) - t0) / (double) val64(freq));
}
