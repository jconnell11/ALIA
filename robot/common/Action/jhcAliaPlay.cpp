// jhcAliaPlay.cpp : play contains group of coordinated FSMs in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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

#include <string.h>

#include "Reasoning/jhcAliaOp.h"       // common audio - since only spec'd as class in header

#include "Action/jhcAliaCore.h"        // common robot - since only spec'd as class in header

#include "Action/jhcAliaPlay.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaPlay::~jhcAliaPlay ()
{
  clr();
}


//= Stop all activities and get rid of descriptions.

void jhcAliaPlay::clr ()
{
  int i;

  Stop();
  for (i = ng - 1; i >= 0; i--)
  {
    delete guard[i];
    guard[i] = NULL;
  }
  for (i = na - 1; i >= 0; i--)
  {
    delete main[i];
    main[i] = NULL;
  }
  na = 0;
  ng = 0;
}


//= Default constructor initializes certain values.

jhcAliaPlay::jhcAliaPlay ()
{
  int i;

  // no activities
  for (i = 0; i < amax; i++)
  {
    main[i] = NULL;
    guard[i] = NULL;
  }
  na = 0;
  ng = 0;

  // initial state (stopped)
  verdict = -1;
}


//= Add an activity to set that must be accomplished.
// returns 1 if successful, 0 for error (act not deleted)

int jhcAliaPlay::AddReq (jhcAliaChain *act)
{
  if ((act == NULL) || (na >= amax))
    return 0;
  main[na] = act;
  status[na] = -1;     // stopped
  na++;
  return 1;
}


//= Add an activity to set that runs in parallel with main goals.
// returns 1 if successful, 0 for error (act not deleted)

int jhcAliaPlay::AddSimul (jhcAliaChain *act)
{
  if ((act == NULL) || (ng >= amax))
    return 0;
  guard[ng] = act;
  gstat[ng] = -1;      // stopped
  ng++;
  return 1;
}


//= Set mark to one for all nodes belonging to instantiated directives.
// beware that network nodes may be shared between activities
// useful for mark/sweep garbage collection (jhcActionTree::clean_mem)

void jhcAliaPlay::MarkSeeds ()
{
  int i;

  // mark nodes in all parallel activities
  for (i = 0; i < ng; i++)
    guard[i]->MarkSeeds(1);

  // mark nodes in all required activities
  for (i = 0; i < na; i++)
    main[i]->MarkSeeds(1);
}


//= Determine the maximum subgoal depth for this part of the tree.
// cyc is unique request number

int jhcAliaPlay::MaxDepth (int cyc) 
{
  int i, deep, win = 0;

  // examine all parallel activities
  for (i = 0; i < ng; i++)
    if ((deep = guard[i]->MaxDepth(cyc)) > win)
      win = deep;

  // examine all required activities
  for (i = 0; i < na; i++)
    if ((deep = main[i]->MaxDepth(cyc)) > win)
      win = deep;
  return win;
}


//= Determine number of simultaneous activities (possibly subgoaled).
// if leaf > 0 then only considers currently active goals not pass-thrus
// cyc is unique request number

int jhcAliaPlay::NumGoals (int leaf, int cyc)
{
  int i, cnt = 0;

  // examine all parallel activities
  for (i = 0; i < ng; i++)
    cnt += guard[i]->NumGoals(leaf, cyc);

  // examine all required activities
  for (i = 0; i < na; i++)
    cnt += main[i]->NumGoals(leaf, cyc);
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Start processing this parallel set.
// negative level used to partially restart any initial play
// returns: 0 = working, -2 = fail 

int jhcAliaPlay::Start (jhcAliaCore *all, int lvl)
{
  int i, level = abs(lvl);

  // start any concurrent (looping) activities 
  for (i = 0; i < ng; i++)
    if ((gstat[i] = guard[i]->Start(all, level)) < 0)
      return fail();

  // start or re-start most main activities (if any)
  for (i = 0; i < na; i++)
    if ((status[i] <= 0) || (lvl >= 0))
      if ((status[i] = main[i]->Start(all, level)) < 0)
        return fail();

  // report current status
  verdict = 0;
  return verdict;
}


//= Continue running this parallel set (focus > 0 if top-level attention item).
// return: 1 (or 2) = done, 0 = working, -2 = fail

int jhcAliaPlay::Status ()
{
  int i;

  // run any concurrent activities (barf when finished)
  for (i = 0; i < ng; i++)
    if ((gstat[i] = guard[i]->Status()) != 0)
      return fail();

  // run all main activities that have not finished
  for (i = 0; i < na; i++)
    if (status[i] == 0) 
      if ((status[i] = main[i]->Status()) < 0)
        return fail();

  // see if done (all main activities done)
  for (i = 0; i < na; i++)
    if (status[i] == 0)
      return 0;

  // record completion (never alternate)
  Stop(1);
  return 1;
}


//= Some activity failed so whole play fails.
// always returns -2 for convenience

int jhcAliaPlay::fail ()
{
  Stop();
  verdict = -2;
  return verdict;
}


//= Courtesy signal to play that its activities are no longer needed. 
// does not remove any activities from definition
// returns current non-zero state (fail, stop, cont, or alt)

int jhcAliaPlay::Stop (int ans)
{
  int i;

  // see if everything already stopped for some reason
  if (verdict != 0)
    return verdict;

  // notify all parallel activities 
  for (i = 0; i < ng; i++)
    if (gstat[i] == 0)
      guard[i]->Stop();

  // notify all required activities
  for (i = 0; i < na; i++)
    if (status[i] == 0)
      main[i]->Stop();

  // record forced termination
  verdict = ans;
  return verdict;
}


///////////////////////////////////////////////////////////////////////////
//                           Execution Tracing                           //
///////////////////////////////////////////////////////////////////////////

//= Look for all in-progress activities matching graph and possibly stop them.
// returns 1 if found (and stopped) all, 0 if nothing matching found

int jhcAliaPlay::HaltActive (const jhcGraphlet& desc, const class jhcAliaDir *skip, int halt)
{
  int i, ans = 0;

  // shortcut if nothing still active
  if (verdict != 0)
    return 0;

  // search all required activities that are still running
  for (i = 0; i < na; i++)
    if (status[i] == 0)
      if (main[i]->HaltActive(desc, skip, halt) > 0)
        ans = 1;

  // search all parallel activities (any stop causes barf)
  for (i = 0; i < ng; i++)
    if (gstat[i] == 0)
      if (guard[i]->HaltActive(desc, skip, halt) > 0)
         ans = 1;
  return ans;
}


//= Find the call pattern for the most recently started activity compatible with the description.
// cyc is unique request number for tracing graphs with loops, prev is most recent caller
// binds best match directive and its caller, done: 0 = ongoing only, 1 = any non-failure

void jhcAliaPlay::FindCall (const jhcAliaDir **act, const jhcAliaDir **src, jhcBindings *d2a, 
                            const jhcGraphlet& desc, UL32& start, int done, const jhcAliaDir *prev, int cyc)
{
  int i;

  for (i = 0; i < na; i++)
    main[i]->FindCall(act, src, d2a, desc, start, done, prev, cyc);
  for (i = 0; i < ng; i++)
    guard[i]->FindCall(act, src, d2a, desc, start, done, prev, cyc);
}


//= Find transition pointer to step containing a DO directive with the given main action.
// cyc is unique request number for tracing graphs with loops

jhcAliaChain **jhcAliaPlay::StepEntry (const jhcNetNode *act, jhcAliaChain **from, int cyc)
{
  jhcAliaChain **entry;
  int i;

  for (i = 0; i < na; i++)
    if ((entry = main[i]->StepEntry(act, &(main[i]), cyc)) != NULL)
      return entry;
  for (i = 0; i < ng; i++)
    if ((entry = guard[i]->StepEntry(act, &(guard[i]), cyc)) != NULL)
      return entry;
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                             File Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Read at current location in a file to fill in details of self.
// jhcAliaChain::LoadChain -> 5 = play end, 4 = new parallel, 3 = new required, 
//                            2 = ok + blank, 1 = successful, 0 = syntax error, -1 = end of file
// returns: 2 = ok + all done, 1 = successful, 0 = syntax error, -1 = end of file, -2 = file error 

int jhcAliaPlay::Load (jhcNodePool& pool, jhcTxtLine& in)
{
  jhcAliaChain *s;
  int add, chain = 3, kind = 2;

  // get activities of various types (initial >>> read by jhcAliaChain)
  while ((chain != 5) && (chain != 2) && (chain != 1))
  {
    // try to read a chain from the file
    s = new jhcAliaChain;
    if ((chain = s->Load(pool, in, 1)) <= 0)
    {  
      delete s;
      return chain;
    }

    // see if only an activity type marker was read
    if (s->Empty())
    {
      delete s;
      kind = chain;
      continue;
    }

    // determine if required or parallel 
    if (kind == 4)
      add = AddSimul(s);
    else
      add = AddReq(s);
    if (add <= 0)
    {
      // could not add to some list
      delete s;
      return 0;
    }
    kind = chain;
  }

  // end of play marker found (or pass blank line all the way up)
  return((chain == 2) ? 2 : 1);                 
}


//= Save self out in machine readable form to current position in a file.
// "step" is the next step number expected, negative if needs end delimiter
// plays should not be exposed directly, use jhcAliaChain::PrintStep to show
// returns 1 if successful, 0 for bad format, -1 for file error

int jhcAliaPlay::Save (FILE *out, int lvl, int *step) const
{
  int *st = step;
  int ans, i;

  // write start marker
  jfprintf(out, "%*s >>>\n", lvl, "");

  // list all main activities separated by +++
  for (i = 0; i < na; i++)
  {
    if (i != 0)
      jfprintf(out, "%*s +++\n", lvl, "");
    if (st != NULL)
      *st = abs(*st);                          // no other terminator
    if ((ans = main[i]->Save(out, lvl + 2, st)) <= 0)
      return ans;
  }

  // list all guard activities prefaced and separated by ===
  for (i = 0; i < ng; i++)
  {
    jfprintf(out, "%*s ===\n", lvl, ""); 
    if (st != NULL)
      *st = abs(*st);                          // no other terminator
    if ((ans = guard[i]->Save(out, lvl + 2, st)) <= 0)
      return ans;
  }

  // write stop marker then check for problems
  jfprintf(out, "%*s <<<\n", lvl, "");
  if (st != NULL)
    *st = abs(*st);
  return((ferror(out) != 0) ? -1 : 1);     
}
