// jhcEchoFcn.cpp : default handler for non-grounded ALIA functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
// Copyright 2021-2023 Etaoin Systems
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

#include <conio.h>
#include <string.h>

#include "Semantic/jhcNetNode.h"

#include "Action/jhcEchoFcn.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Initialization                     //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// NOTE: items in list must be deleted elsewhere

jhcEchoFcn::~jhcEchoFcn ()
{
}


//= Default constructor initializes certain values.

jhcEchoFcn::jhcEchoFcn ()
{
  noisy = 2;
}


//= Tack another pool of functions onto tail of list.

void jhcEchoFcn::AddFcns (jhcAliaKernel& pool)
{
  if (next != NULL)
    next->AddFcns(pool);
  else
    next = &pool;
}


//= Bind all function pools to a real-world interface for a body.
// automatically chains to "next" pool

void jhcEchoFcn::Platform (void *soma)  
{
  if (next != NULL) 
    next->Platform(soma);
}


//= Reset all function pools for start of new run.
// automatically chains to "next" pool

void jhcEchoFcn::Reset (jhcAliaNote& attn)  
{
  if (next != NULL) 
    next->Reset(attn);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Post any spontaneous observations to attention queue.
// automatically chains to "next" pool

void jhcEchoFcn::Volunteer ()
{
  if (next != NULL) 
    next->Volunteer();
}


//= Start a function using given importance bid.
// returns new instance number (>= 0) if success, -1 for problem, -2 for unknown

int jhcEchoFcn::Start (const jhcAliaDesc& desc, int bid)
{
  int rc = -2;

  // try to pass on to some real handler first
  if (next != NULL)
    rc = next->Start(desc, bid);
  if (rc > -2)
    return rc;

  // mark start of non-grounded function
  if (noisy >= 1)
  {
    jprintf(">>> GND: ");
    fcn_args(desc);
    jprintf(" start ignored\n");
  }
  return 1;
}


//= Check whether a function instance has completed yet.
// returns positive for done, 0 for still running, -1 for failure, -2 if unknown

int jhcEchoFcn::Status (const jhcAliaDesc& desc, int inst)
{
  int rc = -2;

  // try to pass on to some real handler first
  if (next != NULL)
    rc = next->Status(desc, inst);
  if (rc > -2)
    return rc;
   
  // mark status check of non-grounded function
  if (noisy >= 1)
  {
    jprintf(">>> GND: ");
    fcn_args(desc);
    jprintf(" status ignored\n");
  }
  return -1;
}


//= Stop a particular function instance (or all if negative).
// calling with NULL description causes all instances of everything to stop
// returns positive for convenience, -2 if unknown (courtesy call - should never wait)

int jhcEchoFcn::Stop (const jhcAliaDesc& desc, int inst)
{
  int rc = -2;

  // try to pass on to some real handler first
  if (next != NULL)
    rc = next->Stop(desc, inst);
  if (rc > -2)
    return rc;
/*
  // mark stop of non-grounded function
  if (noisy >= 1)
  {
    jprintf(">>> GND: ");
    fcn_args(desc);
    jprintf(" stop ignored\n\n");
  }
*/
  return 1;
}


//= Write out description of function and its arguments.

void jhcEchoFcn::fcn_args (const jhcAliaDesc& desc)
{
  const jhcNetNode *n;
  int i, na;

  if ((n = dynamic_cast<const jhcNetNode *>(&desc)) == NULL)
    return;
  jprintf("\"%s(", n->Lex());
  na = n->NumArgs();
  for (i = 0; i < na; i++)
  {
    if (i > 0)
      jprintf(", ");
    jprintf("%s", (n->Arg(i))->Tag());
  }
  jprintf(")\"");
}


