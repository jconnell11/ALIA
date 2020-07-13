// man_drv.cpp : simple semantic network interpretation for Manus robot control
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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
#include <direct.h>                    // for _getcwd in Windows

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"

#include "Body/jhcManusBody.h"         // common robot
#include "Manus/jhcBasicAct.h"       

#include "man_drv.h"


// Local function prototypes

BOOL init ();
BOOL shutdown ();


///////////////////////////////////////////////////////////////////////////
//                          Global Variables                             //
///////////////////////////////////////////////////////////////////////////

//= An instance of the main computational class.

static jhcBasicAct drv;


///////////////////////////////////////////////////////////////////////////
//                      Initialization and Locking                       //
///////////////////////////////////////////////////////////////////////////

//= Only allow DLL to be used for a while.
// more of an annoyance than any real security

BOOL APIENTRY DllMain (HANDLE hModule,
                       DWORD ul_reason_for_call, 
                       LPVOID lpReserved)
{
  int mon = 12, yr = 2019, smon = 7, syr = 2019, strict = 0; 
  char cwd[200];
  char *tail;

  // clean up on exit
  if (ul_reason_for_call == DLL_PROCESS_DETACH)
    return shutdown();
  if (ul_reason_for_call != DLL_PROCESS_ATTACH)
    return TRUE;

  // see if within valid time interval
  if (!jms_expired(mon, yr, smon, syr))
    return init();
  Complain("man_drv DLL %s\nExpired as of %d/%d\njconnell@us.ibm.com",
           drv.Version(), mon, yr);           

  // provide "backdoor" - override if directly in "jhc" directory
  _getcwd(cwd, 200);
  if ((tail = strrchr(cwd, '\\')) != NULL)
    if (strcmp(tail, "\\jhc") == 0)
      return init();

  // potentially prohibit use
  if (strict <= 0)
    return init();
  return FALSE;
}


//= Do all system initializations.

BOOL init ()
{
  return TRUE;
}


//= Do all clean up activities.

BOOL shutdown ()
{
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////
//                       Functions for jhcAliaDLL                        //
///////////////////////////////////////////////////////////////////////////

//= Connect functions to some kind of body.

extern "C" DEXP void pool_bind (void *body)
{
  drv.BindRobot((jhcManusBody *) body);
}


//= Reset internal state for new run.

extern "C" DEXP void pool_reset (class jhcAliaNote *attn)
{
  drv.Reset(attn);
}


//= Post any spontaneous observations to attention queue.

extern "C" DEXP void pool_volunteer ()
{
  drv.Volunteer();
}


//= Start a function using given importance bid.
// returns new instance number (>= 0) if success, -1 for problem, -2 for unknown

extern "C" DEXP int pool_start (const class jhcAliaDesc *desc, int bid)
{
  return drv.Start(desc, bid);
}


//= Check whether a function instance has completed yet.
// returns positive for done, 0 for still running, -1 for failure, -2 if unknown

extern "C" DEXP int pool_status (const class jhcAliaDesc *desc, int bid)
{
  return drv.Status(desc, bid);
}


//= Stop a particular function instance (or all if negative).
// calling with NULL description causes all instances of everything to stop
// returns positive for convenience, -2 if unknown (courtesy call - should never wait)

extern "C" DEXP int pool_stop (const class jhcAliaDesc *desc, int bid)
{
  return drv.Stop(desc, bid);
}


