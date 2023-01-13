// jhcMemStore.cpp : interface for explicit LTM formation in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2022 Etaoin Systems
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

#include "Semantic/jhcNetNode.h"       // common audio

#include "Action/jhcAliaDir.h"         // common robot

#include "Grounding/jhcMemStore.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcMemStore::~jhcMemStore ()
{
}


//= Default constructor initializes certain values.

jhcMemStore::jhcMemStore ()
{
  // static configuration
  ver = 1.15;
  strcpy_s(tag, "MemStore");

  // overall interaction parameters
  dmem = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcMemStore::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(mem_form);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcMemStore::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(mem_form);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Start trying to remember a particular fact.
// instance number and bid already recorded by base class
// returns 1 if okay, -1 for interpretation error

int jhcMemStore::mem_form0 (const jhcAliaDesc *desc, int i)
{
  if (dmem == NULL)
    return -1;
  if (desc->Val("arg") == NULL)
    return -1;
  return 1;
}


//= Continue trying to remember a particular fact.
// returns 1 if done, 0 if still working, -1 for failure

int jhcMemStore::mem_form (const jhcAliaDesc *desc, int i)
{
  const jhcNetNode *root = dynamic_cast<const jhcNetNode *>(desc);
  jhcNetNode *fact = root->Val("arg");

  note_that(fact, root);               // bring into wmem
  if (dmem->Remember(fact) < 0)
    return -1;
  return 1;
}


//= Actualize all hypothetical nodes in embedded clause (thing to be remembered is true).

void jhcMemStore::note_that (jhcNetNode *focus, const jhcNetNode *root) const
{
  int i, na, np;

  // see if node needs to be converted
  if ((focus == root) || !focus->Hyp() || !atree->InList(focus))
    return;
  focus->Actualize(0);

  // add all arguments
  na = focus->NumArgs();
  for (i = 0; i < na; i++)
    note_that(focus->ArgSurf(i), root);
    
  // add all properties
  np = focus->NumProps();
  for (i = 0; i < np; i++)
    note_that(focus->PropSurf(i), root);
}


