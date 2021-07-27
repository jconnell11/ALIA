// jhcAssocMem.cpp : deductive rules for use in halo of ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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

#include "Interface/jprintf.h"         // common video

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
  detail = 0;
//detail = 14;               // show detailed matching for some rule 
}


///////////////////////////////////////////////////////////////////////////
//                             List Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Add new rule onto tail of list.
// returns id number of item added, 0 if not added (consider deleting)

int jhcAssocMem::AddRule (jhcAliaRule *r, int ann)
{
  jhcAliaRule *r0 = rules, *prev = NULL;

  // check for likely duplication
  if (r == NULL)
    return 0;
  if (r->Tautology())
    return jprintf(1, ann, "  ... new rule rejected as a tautology\n\n");
  while ((prev = NextRule(prev)) != NULL)
    if (r->Identical(*prev))
      return jprintf(1, ann, "  ... new rule rejected as identical to rule %d\n\n", prev->RuleNum());

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
    jprintf("\n---------------------------------\n");
    r->Print();
    jprintf("---------------------------------\n\n");
  }
  return nr;
}


//= Remove a rule from the list and permanently delete it.
// must make sure original rem pointer is set to NULL in caller
// used by jhcAliaDir to clean up incomplete ADD operator

void jhcAssocMem::Remove (const jhcAliaRule *rem)
{
  jhcAliaRule *prev = NULL, *r = rules;

  // look for rule in list
  if (rem == NULL)
    return;
  while (r != NULL)
  {
     // possibly splice out of list and delete
    if (r == rem)
    {
      if (prev != NULL)
        prev->next = r->next;
      else
        rules = r->next;
      delete r;
      return;
    }

    // move on to next list entry
    prev = r;
    r = r->next;
  }
}


///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Apply all rules to main portion of working memory, results go to halo.
// will not match conditions with blf < mth, or even try weak rules
// returns number of invocations

int jhcAssocMem::RefreshHalo (jhcWorkMem& wmem, int dbg) const
{
  jhcAliaRule *r = NULL;
  double mth = wmem.MinBlf();
  int cnt = 0, cnt2 = 0;

//dbg = 2;                         // quick way to show halo inferences

jtimer(13, "RefreshHalo");
  // PASS 1 - erase all previous halo deductions
  jprintf(1, dbg, "HALO refresh ...\n");
  wmem.ClearHalo();
  wmem.SetMode(0);                 // only match to nodes in main pool
  jprintf(2, dbg, "1-step:\n");
  while ((r = NextRule(r)) != NULL)
  {
    r->dbg = ((r->id == detail) ? 3 : 0);
    cnt += r->AssertMatches(wmem, mth, 0, dbg - 1);
  }

  // PASS 2 - mark start of 2-step inferences
  wmem.Horizon();
  wmem.SetMode(1);                 // match to main pool and 1-step   
  r = NULL;
  jprintf(2, dbg, "2-step:\n");
  while ((r = NextRule(r)) != NULL)
  {
    r->dbg = ((r->id == detail) ? 3 : 0);
    cnt2 += r->AssertMatches(wmem, mth, 1, dbg - 1);
  }

  // report result
  jprintf(1, dbg, "  %d + %d rule invocations\n\n", cnt, cnt2);
//wmem.PrintHalo();
jtimer_x(13);
  return cnt;
}


//= If a two rule series used to infer an essential fact then combine the two rules.
// needs raw bindings before halo migration (i.e. before modification by ReifyRules)
// returns number of new rules created

int jhcAssocMem::Consolidate (const jhcBindings& b, int dbg)
{
  jhcBindings list, list2, m2c;
  jhcAliaRule *r2, *r1, *mix = NULL;
  jhcBindings *b2, *b1;
  int j, nc, nb = b.NumPairs(), i = -1, cnt = 0;

  // search through main fact inference bindings (length may change)
  list.Copy(b);
  while ((i = next_halo(&r2, &b2, list, i + 1)) < nb)
  {
    // look for halo facts used to trigger this step-2 halo rule
    list2.Copy(b2);
    nc = r2->NumPat();
    j = -1;
    while ((j = next_halo(&r1, &b1, list2, j + 1)) < nc)
    {
      // merge step-1 halo rule into consolidated rule (possibly create)
      if (mix == NULL)
      {
        jprintf(1, dbg, "CONSOLIDATE: rule %d <== rule %d", r2->RuleNum(), r1->RuleNum());
        mix = new jhcAliaRule;
        m2c.Clear();
      }
      else
        jprintf(1, dbg, " + rule %d", r1->RuleNum());
      mix->AddCombo(m2c, *r1, *b1);
    }

    // add complete rule to procedural memory
    if (mix != NULL)
    {
      jprintf(1, dbg, "\n");
      mix->LinkCombo(m2c, *r2, *b2);
      if (AddRule(mix, 1 + dbg) <= 0)
        delete mix;                        // possibly a duplicate
      mix = NULL;                          
      cnt++;
    }
  }
  return cnt;
}


//= Look down list of bindings for next halo fact and return rule and binding used to infer it.
// also alters original list to ignore items with similar provenance (or non-halo items)

int jhcAssocMem::next_halo (jhcAliaRule **r, jhcBindings **b, jhcBindings& list, int start) const
{
  const jhcNetNode *item;
  int i, j, nb = list.NumPairs();

  // scan through remainder of list
  for (i = start; i < nb; i++)
  {
    // ignore non-halo items or already removed nodes
    if ((item = list.GetSub(i)) == NULL)
        continue;
    if (!item->Halo())
      continue;
    *r = item->hrule;
    *b = item->hbind;

    // edit tail of list to have only halo items with different provenance
    for (j = i + 1; j < nb; j++)
      if ((item = list.GetSub(j)) != NULL)
        if (!item->Halo() || ((item->hrule == *r) && (item->hbind == *b)))
          list.SetSub(j, NULL);
    break;
  }
  return i;                  // start next at index after this
}


///////////////////////////////////////////////////////////////////////////
//                            File Functions                             //
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
      // add rule to list if not a duplicate (unlikely)
      r->lvl = level;
      if (AddRule(r, 0) <= 0)
        delete r; 
      n++;
    }
  }

  // possibly announce result
  if (n > 0)
    jprintf(2, rpt, "  %3d inference rules  from: %s\n", n, fname);
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
      if (r->Save(out, 2) > 0)
        cnt++;
    r = r->next;
  }
  return cnt;
}


