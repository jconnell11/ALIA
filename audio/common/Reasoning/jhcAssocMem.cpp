// jhcAssocMem.cpp : deductive rules for use in halo of ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#include <stdio.h>

#include "Interface/jhcMessage.h"      // common video

#include "Action/jhcAliaCore.h"        // common robot - since only spec'd as class in header

#include "Parse/jhcTxtLine.h"          // common audio

#include "Reasoning/jhcAssocMem.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAssocMem::~jhcAssocMem ()
{
  clear();
}


//= Get rid of all loaded rules.
// always returns 0 for convenience

int jhcAssocMem::clear ()
{
  jhcAliaRule *r0, *r = rules;

  while (r != NULL)
  {
    r0 = r;
    r = r->next;
    delete r0;
  }
  rules = NULL;
  nr = 0;
  return 0;
}


//= Default constructor initializes certain values.

jhcAssocMem::jhcAssocMem ()
{
  rules = NULL;
  nr = 0;
  noisy = 2;
}


///////////////////////////////////////////////////////////////////////////
//                             List Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Add item onto tail of list.
// returns id number of item added

int jhcAssocMem::AddRule (jhcAliaRule *r, int ann)
{
  jhcAliaRule *r0 = rules;

  // sanity check
  if (r == NULL)
    return 0;

  // add to end of list
  if (r0 == NULL)
    rules = r;
  else
  {
    while (r0->next != NULL)
      r0 = r0->next;
    r0->next = r;
  }

  // assign rule id number
  r->next = NULL;
  r->id = ++nr;

  // possibly announce formation
  if ((ann > 0) && (noisy >= 1))
  {
    jprintf("---------------------------------\n");
    jprintf(">>> Newly added:\n\n");
    r->Print();
    jprintf("---------------------------------\n\n");
  }
  return nr;
}


///////////////////////////////////////////////////////////////////////////
//                              Configuration                            //
///////////////////////////////////////////////////////////////////////////

//= Read a list of declarative rules from a file.
// appends to existing rules unless add <= 0
// level: 0 = kernel, 1 = extras, 2 = previous accumulation
// ignores actual rule IDs from file and assigns new ones
// returns number of rules read, 0 or negative for problem

int jhcAssocMem::Load (const char *fname, int add, int rpt, int level)
{
  jhcTxtLine in;
  jhcAliaRule *r;
  int ans = 1, n = 0;

  // possibly clear old stuff then try to open file
  if (add <= 0)
    clear();
  if (!in.Open(fname))
  {
    jprintf(">>> Could not open rule file: %s !\n", fname);
    return -1;
  }

  // try reading rules from file  
  while (ans >= 0)
  {
    // make and load a new rule, delete if parse error 
    r = new jhcAliaRule;
    if ((ans = r->Load(in)) <= 0)
    {
      // delete and purge input if parse error 
      if (!in.End())
        jprintf("Bad syntax at line %d in: %s\n", in.Last(), fname);
      delete r;
      if (in.NextBlank() == NULL)
        break;
    }
    else
    {
      // add rule to list
      r->lvl = level;
      AddRule(r, 0);
      n++;
    }
  }

  // possibly announce result
  if (n > 0)
    jprintf(2, rpt, "  %2d inference rules  from: %s\n", n, fname);
  else
    jprintf(2, rpt, "  -- no inference rules  from: %s\n", fname);
  return n;
}


//= Save all current rules at or above some level to a file.
// level: 0 = kernel, 1 = extras, 2 = previous accumulation, 3 = newly added
// returns number of rules saved, negative for problem

int jhcAssocMem::Save (const char *fname, int level) const
{
  FILE *out;
  int cnt;

  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  cnt = save_rules(out, level);
  fclose(out);
  return cnt;
}


//= Save all rules in order.
// returns number saved

int jhcAssocMem::save_rules (FILE *out, int level) const
{
  jhcAliaRule *r = rules;
  int cnt = 0;

  while (r != NULL)
  {
    if (r->lvl >= level)
      if (r->Save(out) > 0)
        cnt++;
    r = r->next;
  }
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Apply all rules to main portion of working memory, results go to halo.
// will not match conditions with blf < mth, or even try weak rules
// returns number of invocations

int jhcAssocMem::RefreshHalo (jhcWorkMem& wmem, double mth, int dbg) const
{
  jhcAliaRule *r = rules;
  int cnt = 0;

  // erase previous halo, only match to nodes in main pool
  jprintf(1, dbg, "HALO refresh ...\n");
  wmem.ClearHalo();
  wmem.SetMode(0);

  // try all rules in list
  jprintf(2, dbg, "\n\n");
  while (r != NULL)
  {
    if (r->Confidence() >= mth)
      cnt += r->AssertMatches(wmem, mth, dbg - 1);
    r = r->next;
  }

  // report result
  jprintf(1, dbg, "  %d rule invocations\n\n", cnt);
  return cnt;
}
