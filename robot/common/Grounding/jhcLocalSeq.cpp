// jhcLocalSeq.cpp : interface to Manus sensor sequence kernel for ALIA system
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

#include "RWI/jhcManusRWI.h"

#include "Grounding/jhcLocalSeq.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcLocalSeq::~jhcLocalSeq ()
{
}


//= Default constructor initializes certain values.

jhcLocalSeq::jhcLocalSeq ()
{
  strcpy_s(tag, "LocalSeq");
  rwi = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Attach physical enhanced body and make pointers to some pieces.

void jhcLocalSeq::local_platform (void *soma)
{
  rwi = (jhcManusRWI *) soma;
}


//= Set up for new run of system.

void jhcLocalSeq::local_reset (jhcAliaNote& top)
{
  dbg = 1;
}


//= Post any spontaneous observations to attention queue.

void jhcLocalSeq::local_volunteer ()
{
}


//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcLocalSeq::local_start (const jhcAliaDesc& desc, int i)
{
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcLocalSeq::local_status (const jhcAliaDesc& desc, int i)
{
  return -2;
}


