// jhcAliaDLL.cpp : add procedural grounding functions from a DLL to ALIA
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#include "Interface/jhcString.h"       // common video

#include "Action/jhcAliaDLL.h"


//= The DLL bind function is equivalent to this type.

typedef void (*bfcn)(void *body);


//= The DLL reset function is equivalent to this type.

typedef void (*rfcn)(jhcAliaNote *attn);


//= The DLL volunteer function is equivalent to this type.

typedef void (*vfcn)();


//= The DLL start, status, and stop functions are all equivalent to this type.

typedef int (*sfcn)(const jhcAliaDesc *desc, int i);


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// NOTE: items in list must be deleted elsewhere

jhcAliaDLL::~jhcAliaDLL ()
{
  close();
}


//= Get rid of all bound functions and close DLL handle.
// always returns 0 for convenience

int jhcAliaDLL::close ()
{
  HMODULE dll = (HMODULE) lib;

  if (dll != NULL)
    FreeLibrary(dll);
  lib = NULL;
  local_bind      = NULL;
  local_reset     = NULL;
  local_volunteer = NULL;
  local_start     = NULL;
  local_status    = NULL;
  local_stop      = NULL;
  return 0;
}


//= Default constructor initializes certain values.
// takes the name of the DLL to open as a required argument

jhcAliaDLL::jhcAliaDLL (const char *file)
{
  next = NULL;
  lib = NULL;
  Load(file);
}


//= Try to bind necessary dispatch functions from given DLL name.
// returns 1 if successful, 0 if missing functions, -1 if bad file

int jhcAliaDLL::Load (const char *file)
{
  jhcString fn;
  HMODULE dll;
  int all = 0;

  // get rid of any old functions then try to open new DLL file
  close();
  fn.Set(file);
  if ((dll = LoadLibrary(fn.Txt())) == NULL)
    return -1;
  lib = (void *) dll;

  // bind required functions
  while (1)
  {
    if ((local_bind      = (bfcn) GetProcAddress(dll, "pool_bind")) == NULL)
      break;
    if ((local_reset     = (rfcn) GetProcAddress(dll, "pool_reset")) == NULL)
      break;
    if ((local_volunteer = (vfcn) GetProcAddress(dll, "pool_volunteer")) == NULL)
      break;
    if ((local_start     = (sfcn) GetProcAddress(dll, "pool_start")) == NULL)
      break;
    if ((local_status    = (sfcn) GetProcAddress(dll, "pool_status")) == NULL)
      break;
    if ((local_stop      = (sfcn) GetProcAddress(dll, "pool_stop")) == NULL)
      break;
    all = 1;
    break;
  }

  // unload everything if failure else connect to body
  if (all <= 0)
    return close();
  return 1;
}


//= Tack another pool of functions onto tail of list.

void jhcAliaDLL::AddFcns (jhcAliaKernel *pool)
{
  if (pool == NULL)
    return;
  if (next != NULL)
    next->AddFcns(pool);
  else
    next = pool;
}


//= Generally need to connect routines to some physical "body".

void jhcAliaDLL::Bind (void *body)
{
  if (local_bind != NULL)
    (*local_bind)(body);
}


//= Kill all instance of all functions.
// automatically chains to "next" pool

void jhcAliaDLL::Reset (jhcAliaNote *attn)  
{
  if (local_reset != NULL)
    (*local_reset)(attn);
  if (next != NULL) 
    next->Reset(attn);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Post any spontaneous observations to attention queue.
// automatically chains to "next" pool

void jhcAliaDLL::Volunteer ()
{
  if (local_volunteer != NULL)
    (*local_volunteer)();
  if (next != NULL) 
    next->Volunteer();
}


//= Start a function using given importance bid.
// returns new instance number (>= 0) if success, -1 for problem, -2 for unknown

int jhcAliaDLL::Start (const jhcAliaDesc *desc, int bid)
{
  int rc = -2;

  if (local_start != NULL)
    rc = (*local_start)(desc, bid);
  if ((rc <= -2) && (next != NULL))
    rc = next->Start(desc, bid);
  return rc;
}


//= Check whether a function instance has completed yet.
// returns positive for done, 0 for still running, -1 for failure, -2 if unknown

int jhcAliaDLL::Status (const jhcAliaDesc *desc, int inst)
{
  int rc = -2;

  if (local_status != NULL)
    rc = (*local_status)(desc, inst);
  if ((rc <= -2) && (next != NULL))
    rc = next->Status(desc, inst);
  return rc;
}


//= Stop a particular function instance (or all if negative).
// calling with NULL description causes all instances of everything to stop
// returns positive for convenience, -2 if unknown (courtesy call - should never wait)

int jhcAliaDLL::Stop (const jhcAliaDesc *desc, int inst)
{
  int rc = -2;

  if (local_stop != NULL)
    rc = (*local_stop)(desc, inst);
  if (((rc <= -2) || (desc == NULL)) && (next != NULL))
    rc = next->Status(desc, inst);
  return rc;
}

