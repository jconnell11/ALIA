// basic_act.h : grounding kernel jhcBasicAct as DLL for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include <windows.h>
#include <direct.h>                    // for _getcwd in Windows

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"

#include "Grounding/jhcBasicAct.h"     // common robot    
#include "RWI/jhcManusRWI.h"         

#include "basic_act.h"
#include "resource.h"


///////////////////////////////////////////////////////////////////////////
//                          Global Variables                             //
///////////////////////////////////////////////////////////////////////////

//= Version number of this DLL (change in Resource also).

static double ver = 5.10;


//= An instance of the main computational class.

static jhcBasicAct pool;


///////////////////////////////////////////////////////////////////////////
//                      Initialization and Locking                       //
///////////////////////////////////////////////////////////////////////////

//= Do all system initializations like unpacking auxiliary files.
// make a new compressed folder in main then add all files needed
// then do Add Resource / Import ... / as RCDATA and assign IDR_AUX_FILES

BOOL init (HANDLE hModule)
{
  HMODULE mod = (HMODULE) hModule;
  HRSRC rsrc;
  HGLOBAL hres = NULL;
  char *data;
  FILE *out;
  DWORD sz, n;

  // try to locate embedded resource 
  rsrc = FindResource(mod, MAKEINTRESOURCE(IDR_AUX_FILES), RT_RCDATA);
  if (rsrc == NULL)
    return TRUE;
  if ((hres = LoadResource(mod, rsrc)) == NULL)
    return TRUE;

  // copy all data out to temporary file
  if (fopen_s(&out, "jhc_temp.zip", "wb") != 0)
    return TRUE;
  data = (char *) LockResource(hres);
  sz = SizeofResource(mod, rsrc);
  n = (int) fwrite(data, 1, sz, out);
  fclose(out);

  // attempt to extarct all files then clean up
  if (n == sz)
    system("tar -xkf jhc_temp.zip");
  remove("jhc_temp.zip");
  return TRUE;
}


//= Only allow DLL to be used for a while.
// more of an annoyance than any real security

BOOL APIENTRY DllMain (HANDLE hModule,
                       DWORD ul_reason_for_call, 
                       LPVOID lpReserved)
{
  int mon = 11, yr = 2023, smon = 6, syr = 2023, strict = 0; 
  char cwd[200];
  char *tail;

  // clean up on exit
  if (ul_reason_for_call == DLL_PROCESS_DETACH)
    return TRUE;
  if (ul_reason_for_call != DLL_PROCESS_ATTACH)
    return TRUE;

  // see if within valid time interval
  if (!jms_expired(mon, yr, smon, syr))    
    return init(hModule);
  Complain("basic_act v%4.2f\nExpired as of %d/%d\njconnell@alum.mit.edu", ver, mon, yr);    

  // provide "backdoor" - override if directly in "jhc" directory
  _getcwd(cwd, 200);
  if ((tail = strrchr(cwd, '\\')) != NULL)
    if (strcmp(tail, "\\jhc") == 0)
      return init(hModule);

  // potentially prohibit use
  if (strict <= 0)
    return init(hModule);
  return FALSE;
}


///////////////////////////////////////////////////////////////////////////
//                       Functions for jhcAliaDLL                        //
///////////////////////////////////////////////////////////////////////////

//= Connect functions to some kind of body.

extern "C" DEXP void gnd_platform (void *soma)
{
  pool.Platform(soma);
}


//= Reset internal state for new run.

extern "C" DEXP void gnd_reset (class jhcAliaNote& attn)
{
  pool.Reset(attn);
}


//= Post any spontaneous observations to attention queue.

extern "C" DEXP void gnd_volunteer ()
{
  pool.Volunteer();
}


//= Start a function using given importance bid.
// returns new instance number (>= 0) if success, -1 for problem, -2 for unknown

extern "C" DEXP int gnd_start (const class jhcAliaDesc& desc, int bid)
{
  return pool.Start(desc, bid);
}


//= Check whether a function instance has completed yet.
// returns positive for done, 0 for still running, -1 for failure, -2 if unknown

extern "C" DEXP int gnd_status (const class jhcAliaDesc& desc, int bid)
{
  return pool.Status(desc, bid);
}


//= Stop a particular function instance (or all if negative).
// calling with NULL description causes all instances of everything to stop
// returns positive for convenience, -2 if unknown (courtesy call - should never wait)

extern "C" DEXP int gnd_stop (const class jhcAliaDesc& desc, int bid)
{
  return pool.Stop(desc, bid);
}


