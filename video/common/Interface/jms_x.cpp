// jms_x.cpp : common millisecond time functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

// Build.Settings.Link.General must have library winmm.lib
#pragma comment(lib, "winmm.lib")   

#include <windows.h>
#include <stdio.h>

#include "Interface/jms_x.h"


//= Persistent synchronization object for more precise sleep.

//static HANDLE wait = CreateWaitableTimer(NULL, TRUE, NULL);


///////////////////////////////////////////////////////////////////////////
//                             Elapsed Time                              //
///////////////////////////////////////////////////////////////////////////

//= Sleep for a certain number of milliseconds (BLOCKS).

void jms_sleep (int ms)
{
  if (ms > 0)
    Sleep(ms);
/*
  LARGE_INTEGER due;

  // figure out time to sleep in 100ns ticks
  if (ms <= 0)
    return;
  due.QuadPart = -10LL * ms;

  // wait at least the amount requested
  timeBeginPeriod(1);
  if (SetWaitableTimer(wait, &due, 0, NULL, NULL, 0))
    WaitForSingleObject(wait, INFINITE);
  timeEndPeriod(1);
*/
}


//= Tell number of milliseconds elapsed since power-on.
// never returns special value of 0 (usually means uninitialized)
// Note: wraps around roughly every 50 days

UL32 jms_now ()
{
  UL32 now = timeGetTime();

  return __max(1, now);
}


//= Returns elapsed milliseconds from "before" to "now".
// handles 50 day wraparound, so max of 24.8 days (0x7FFFFFF = 2,147,483.647 secs)
// NOTE: "now" must actually represent a later time than "before"

int jms_diff (UL32 now, UL32 before)
{
  if (now >= before)
    return((int)(now - before));
  return((int)(~(before - now) + 1));
}


//= Returns elapsed seconds from "before" to "now".

double jms_secs (UL32 now, UL32 before)
{
  return(0.001 * jms_diff(now, before));
}


//= Block until "delay" milliseconds after "tref" time.
// returns time when function is exited

UL32 jms_wait (UL32 tref, int delay)
{
  UL32 now;
  int passed = 0, dslop = delay - 1;     // tends to oversleep   

  // see if time already expired
  if (tref != 0)
  {
    now = jms_now();
    passed = jms_diff(now, tref);
    if (passed >= dslop)
      return now;
  }

  // wait remainder of time
  jms_sleep(dslop - passed);
  return jms_now();
}


//= Sleep until a specific time has come.
// good for long term pacing by constantly incrementing "cont"
// always returns 0 for convenience

int jms_resume (UL32 cont)
{
  if (cont != 0)
    jms_sleep(jms_diff(cont, jms_now()) - 1);
  return 0;
}


//= Gives string with elapsed time in hrs:min:sec.ms from base time.

char *jms_elapsed (char *dest, UL32 tref, int ssz)
{
  int h, m, s, ms = jms_diff(jms_now(), tref);

  h = ms / 3600000;
  ms -= h * 3600000;
  m = ms / 60000;
  ms -= m * 60000;
  s = ms / 1000;
  ms -= s * 1000;
  sprintf_s(dest, ssz, "%d:%02d:%02d.%03d", h, m, s, ms);
  return dest;
}


///////////////////////////////////////////////////////////////////////////
//                             Absolute Time                             //
///////////////////////////////////////////////////////////////////////////

//= Generate a date string with optional time (down to seconds).
// res: -1 gives 110717             just date
//       0 gives 110717_1027        with minutes (24 hr format)
//       1 gives 110717_102736      with minutes and seconds
// useful for labelling log files

char *jms_date (char *dest, int res, int ssz) 
{
  SYSTEMTIME t;

  GetLocalTime(&t);
  if (res < 0)
    sprintf_s(dest, ssz, "%02d%02d%02d", 
                         t.wMonth, t.wDay, t.wYear % 100);
  else if (res == 0)
    sprintf_s(dest, ssz, "%02d%02d%02d_%02d%02d", 
                         t.wMonth, t.wDay, t.wYear % 100, t.wHour, t.wMinute);
  else 
    sprintf_s(dest, ssz, "%02d%02d%02d_%02d%02d%02d", 
                         t.wMonth, t.wDay, t.wYear % 100, t.wHour, t.wMinute, t.wSecond);
  return dest;
} 


//= Generate a time string (with optional milliseconds).
// res: 0 gives 10:27:36           time with seconds
//      1 gives 10:27:36.145       time with milliseconds
// useful for tagging events

char *jms_time (char *dest, int res, int ssz) 
{
  SYSTEMTIME t;

  GetLocalTime(&t);
  if (res <= 0)
    sprintf_s(dest, ssz, "%02d:%02d:%02d", 
                         t.wHour, t.wMinute, t.wSecond);
  else 
    sprintf_s(dest, ssz, "%02d:%02d:%02d.%03d",
                         t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
  return dest;
} 


//= Tells whether current local date is outside specified window.
// years must be 4 digit, ignores start month and year if zero

bool jms_expired (int mon, int yr, int smon, int syr)
{
  SYSTEMTIME t;
  int cmon, cyr;

  GetLocalTime(&t);
  cmon = t.wMonth;
  cyr = t.wYear;
  return( (cyr > yr) || 
         ((cyr == yr) && (cmon > mon)) ||
         ((syr > 0) && (cyr < syr)) || 
         ((syr > 0) && (cyr == syr) && (smon > 0) && (cmon < smon)));
}

