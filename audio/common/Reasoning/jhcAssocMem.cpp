// jhcAssocMem.cpp : deductive rules for use in halo of ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "Parse/jhcTxtLine.h"          // common audio

#include "Reasoning/jhcAssocMem.h"

#include "Interface/jtimer.h"
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
  noisy = 1;                 // defaulted from jhcAliaCore
  detail = 0;
//detail = 186;              // show detailed matching for some rule 
}


///////////////////////////////////////////////////////////////////////////
//                             List Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Add new rule onto tail of list.
// ann: 0 = no msgs, 1 = fail only, 2 = fail with rule, 3 = new rule 
// returns 1 if successful, 0 or negative for some problem (consider deleting)

int jhcAssocMem::AddRule (jhcAliaRule *r, int ann, int usr)
{
  jhcAliaRule *r0 = rules, *prev = NULL;

  // check for likely duplication or other format problems
  if (r == NULL)
    return 0;
  if ((r->result).Empty())
  {
    jprintf(1, ann, "  ... REJECT: new rule result is empty\n");
    return -1;
  }
  if (r->Tautology())
  {
    jprintf(1, ann, "  ... REJECT: new rule is a tautology\n");
    return -2;
  }
  if (r->Bipartite())
  {
    jprintf(1, ann, "  ... REJECT: new rule is disconnected\n");
    return -3;
  }
  while ((prev = NextRule(prev)) != NULL)
    if (r->Identical(*prev))
    {
      if (usr > 0)
      {
        // possibly revise old rule instead of adding
        jprintf(1, ann, "  ... KNOWN: set old rule %d confidence = %4.2f\n", 
                prev->RuleNum(), r->conf);
        prev->conf = r->conf;
        if ((ann >= 2) && (noisy >= 1))
        {
          jprintf("\n.................................\n");
          prev->Print();
          jprintf(".................................\n\n");
        }
        delete r;                      // clean up since not saved
        return 1;
      }
      jprintf(1, ann, "  ... DUPLICATE: identical to old rule %d\n", prev->RuleNum());
      return -4;
    }

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
  if ((ann >= 2) && (noisy >= 1))
  {
    jprintf("\n.................................\n");
    r->Print();
    jprintf(".................................\n\n");
  }
  return 1;
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
     // possibly splice out of list
    if (r == rem)
    {
      if (prev != NULL)
        prev->next = r->next;
      else
        rules = r->next;
      break;
    }

    // move on to next list entry
    prev = r;
    r = r->next;
  }

  // delete given item even if it was not in list
  delete rem;
}


///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Apply all rules to main portion of working memory, results go to halo.
// will not match conditions with blf < mth, or even try weak rules
// returns number of invocations
// NOTE:assumes all halo inferences have been erased already (e.g. ClearHalo called)

int jhcAssocMem::RefreshHalo (jhcWorkMem& wmem, int dbg) const
{
  jhcAliaRule *r = NULL;
  double mth = wmem.MinBlf();
  int cnt = 0, cnt2 = 0;

//dbg = 2;                               // quick way to show halo inferences

jtimer(14, "RefreshHalo");
  // possibly announce entry
  jprintf(1, dbg, "HALO refresh ...\n");

  // PASS 1 - run 1-step inference on working memory and long term props  
  // only match to nodes in main pool and LTM props
  wmem.MaxBand(1);                     
  jprintf(2, dbg, "1-step:\n");
  while ((r = NextRule(r)) != NULL)
  {
    r->dbg = ((r->id == detail) ? 3 : 0);
    cnt += r->AssertMatches(wmem, mth, 0, dbg - 1);
  }
  wmem.Horizon();                      // sets "nimbus" to single vs double rule boundary

  // PASS 2 - run 2-step inference using first set of halo assertions
  // match to main pool, LTM props, and 1-step    
  wmem.MaxBand(2);                     
  jprintf(2, dbg, "2-step:\n");
  r = NULL;
  while ((r = NextRule(r)) != NULL)
  {
    r->dbg = ((r->id == detail) ? 3 : 0);
    cnt2 += r->AssertMatches(wmem, mth, 1, dbg - 1);
  }

  // report result
  jprintf(1, dbg, "  %d + %d rule invocations\n", cnt, cnt2);
jtimer_x(14);
//wmem.PrintHalo();
  return cnt;
}


//= If a two rule series used to infer an essential fact then combine the two rules.
// needs raw bindings before halo migration (i.e. before modification by ReifyRules)
// returns number of new rules created

int jhcAssocMem::Consolidate (const jhcBindings& b, int dbg)
{
  jhcBindings list, list2, m2c;
  jhcAliaRule *r2 = NULL, *r1 = NULL, *mix = NULL;
  jhcBindings *b2 = NULL, *b1 = NULL;
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
        jprintf(1, dbg, "\nCONSOLIDATE: rule %d <== rule %d", r2->RuleNum(), r1->RuleNum());
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
      if (AddRule(mix, 1 + dbg, 0) <= 0)
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
    if (!item->Halo() || (item->hrule == NULL))
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
// typically give base file name like "KB/kb_072721_1038", fcn adds ".rules"
// returns number of rules read, 0 or negative for problem

int jhcAssocMem::Load (const char *base, int add, int rpt, int level)
{
  jhcTxtLine in;
  char full[200], src[80] = "";
  const char *fname = base;
  jhcAliaRule *r;
  char *end;
  int ans = 1, n = 0;

  // possibly clear old stuff then try to open file
  if (add <= 0)
    clear();
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.rules", base);
    fname = full;
  }
  if (!in.Open(fname))
  {
//    jprintf("  >>> Could not read rule file: %s !\n", fname);
    return -1;
  }

  // determine provenance string to use
  if (level <= 1)
  {
    strcpy_s(src, fname);
    if ((end = strrchr(src, '.')) != NULL)
      *end = '\0';
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
        jprintf(">>> Bad syntax at line %d in: %s\n", in.Last(), fname);
      delete r;
      if (in.NextBlank() == NULL)
        break;
    }
    else
    {
      // add rule to list if not a duplicate (unlikely)
      r->lvl = level;
      strcpy_s(r->prov, src);
      if (AddRule(r, 1, 0) > 0)
        n++;
      else
      {
        jprintf(">>> Invalid rule at line %d in: %s\n", in.Last(), fname);
        delete r; 
      }
    }
  }

  // possibly announce result
  if (n > 0)
    jprintf(2, rpt, "  %3d inference rules  from: %s\n", n, fname);
  else
    jprintf(2, rpt, "   -- inference rules  from: %s\n", fname);
  return n;
}


//= Save all current rules at or above some level to a file.
// typically give base file name like "KB/kb_072721_1038", fcn adds ".rules"
// level: 0 = kernel, 1 = extras, 2 = previous accumulation, 3 = newly added
// returns number of rules saved, negative for problem

int jhcAssocMem::Save (const char *base, int level) const
{
  char full[200];
  const char *fname = base;
  FILE *out;
  int cnt;

  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.rules", base);
    fname = full;
  }
  if (fopen_s(&out, fname, "w") != 0)
  {
    jprintf("  >>> Could not write rule file: %s !\n", fname);
    return -1;
  }
  if (level >= 2)
  {
    fprintf(out, "// newly learned rules not in KB0 or KB2\n");
    fprintf(out, "// ======================================\n\n");
  }
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
      {
        fprintf(out, "\n\n");
        cnt++;
      }
    r = r->next;
  }
  return cnt;
}


//= Store alterations of confidence values relative to KB0 and KB2 rules.
// typically give base file name like "KB/kb_072721_1038", fcn adds ".conf"
// returns number of exceptions stored (writes file)

int jhcAssocMem::Alterations (const char *base) const
{
  char full[200];
  const char *fname = base;
  FILE *out;
  const jhcAliaRule *r = rules;
  int na = 0;

  // try opening file and writing header
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.conf", base);
    fname = full;
  }
  if (fopen_s(&out, fname, "w") != 0)
  {
    jprintf("  >>> Could not write confidence file: %s !\n", fname);
    return -1;
  }
  fprintf(out, "// learned changes to default rule confidences\n\n");

  // scan through rules for altered values
  while (r != NULL)
  {
    if ((*(r->prov) != '\0') && (r->conf != r->conf0))
    {
      fprintf(out, "%s %d = %4.2f\n", r->prov, r->pnum, r->conf);
      na++; 
    }  
    r = r->next;
  }

  // clean up
  fclose(out);
  return na;
}


//= Change default confidence values of KB0 and KB2 rules based on learning.
// typically give base file name like "KB/kb_072721_1038", fcn adds ".conf"
// returns number of rules altered (reads file)

int jhcAssocMem::Overrides (const char *base)
{
  jhcTxtLine in;
  char full[200], src[40];
  const char *item, *fname = base;
  jhcAliaRule *r;
  double cf;
  int n, na = 0;

  // try opening file
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.conf", base);
    fname = full;
  }
  if (!in.Open(fname))
  {
//    jprintf("  >>> Could not read confidence file: %s !\n", fname);
    return -1;
  }

  // read and parse each line
  while (in.NextContent() != NULL)
  {
    // extract provenance file and original number
    if ((item = in.Token()) == NULL)
      break;
    strcpy_s(src, item);
    if ((item = in.Token()) == NULL)
      break;
    if (sscanf_s(item, "%d", &n) != 1)
      break;

    // extract updated confidence value
    if ((item = in.Token()) == NULL)
      break;
    if (strcmp(item, "=") != 0)
      break;
    if ((item = in.Token()) == NULL)
      break;
    if (sscanf_s(item, "%lf", &cf) != 1)
      break;

    // find matching rule (if any)
    r = rules;
    while (r != NULL)
    {
      if ((*(r->prov) != '\0') && (r->pnum == n) && (strcmp(r->prov, src) == 0))
      {
        // update confidence value
        r->conf = cf;
        na++;
        break;
      }
      r = r->next;
    }
  }
  return na;
}
