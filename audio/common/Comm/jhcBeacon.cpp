// jhcBeacon.cpp : generates entry for Jeff's process management table
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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
#include <sys/types.h>                         // needed for time
#include <sys/timeb.h>                         // needed for time
#include <direct.h>                            // for _getcwd in Windows
#include <string.h>
#include <stdio.h>

#include "Interface/jhcString.h"               // common video

#include "Comm/jhcBeacon.h"


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcBeacon::jhcBeacon ()
{
  // never sent yet
  last = 0;
  slast = 0;

  // user adjustable properties
  strcpy_s(hdr, "BEACON");
  strcpy_s(dns, "watson.ibm.com");   // cheat
  gap = 3.0;

  // speech indication
  strcpy_s(snd, "ATTILA_WORDS");
  sgap = 0.5;
}


//= Load most of the basic information about the program.

void jhcBeacon::Init (const char *name)
{
  char txt[200];
  jhcString tmp;
  char *end;
  DWORD len = 500;

  // service name (supplied as argument)
  pod.SetKey("name", name);

  // machine where program is currently running
  GetComputerNameEx(ComputerNameDnsHostname, tmp.wch, &len);
  tmp.W2C();
  sprintf_s(txt, "%s.%s", tmp.ch, dns);
  pod.SetKey("host", txt);

  // process ID for the running program
  pod.SetKey("pid", (int) GetCurrentProcessId());

  // working directory (no backslashes)
  _getcwd(txt, 200);
  end = txt;
  while (*end != '\0')
  {
    if (*end == '\\')
      *end = '/';
    end++;
  }
  pod.SetKey("path", txt);

  // executable name (strip directory and extension)
  tmp.Set(GetCommandLine());
  if ((end = strrchr(tmp.ch, '\\')) != NULL)
    strcpy_s(txt, end + 1);
  else
    strcpy_s(txt, tmp.ch);
  if ((end = strchr(txt, '.')) != NULL)
    *end = '\0';
  pod.SetKey("launch", txt);
}


//= Add the current time to the JSON structure.
// returns NULL pointer if no broadcast needed yet

const jhcJSON *jhcBeacon::Update ()
{
  struct _timeb t;
  __int64 t2;
  double diff;

  // get ms since Jan 1, 1970
  _ftime_s(&t);
  t2 = 1000 * t.time + t.millitm;

  // see if enough time has elapsed
  diff = (double)(t2 - last) / 1000.0;
  if (diff < gap)
    return NULL;

  // save update time and copy to JSON
  last = t2;
  pod.SetKey("time", (double) t2);
  return &pod;
}


//= Send a message indicating speech is being heard.
// returns 1 if message should be sent, 0 if not needed

int jhcBeacon::Sound (int any)
{
  struct _timeb t;
  __int64 t2;
  double diff;

  // make sure sound is present
  if (any <= 0)
    return 0;

  // get ms since Jan 1, 1970
  _ftime_s(&t);
  t2 = 1000 * t.time + t.millitm;

  // see if enough time has elapsed
  diff = (double)(t2 - slast) / 1000.0;
  if (diff < sgap)
    return 0;

  // record time message was sent
  slast = t2;
  return 1;
}

