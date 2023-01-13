// jhcProcMem.cpp : procedural memory for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#include <stdio.h>

#include "Interface/jhcMessage.h"   // common video

#include "Reasoning/jhcProcMem.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcProcMem::~jhcProcMem ()
{
  clear();
}


//= Get rid of all loaded operators.
// always returns 0 for convenience

int jhcProcMem::clear ()
{
  jhcAliaOp *p0, *p = ops;

  while (p != NULL)
  {
    p0 = p;
    p = p->next;
    delete p0;
  }
  ops = NULL;
  np = 0;
  return 0;
}


//= Default constructor initializes certain values.

jhcProcMem::jhcProcMem ()
{
  ops = NULL;
  np = 0;
  noisy = 2;                 // defaulted from jhcAliaCore
  detail = 0;
//detail = 44;               // show detailed matching for some operator 
}


///////////////////////////////////////////////////////////////////////////
//                             List Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Add item onto tail of list.
// returns 1 if successful, 0 or negative for some problem (consider deleting)

int jhcProcMem::AddOperator (jhcAliaOp *p, int ann)
{
  jhcAliaOp *p0 = ops;

  // add to end of appropriate list
  if (p == NULL)
    return 0;
  if (p0 == NULL)
    ops = p;
  else
  {
    while (p0->next != NULL)
      p0 = p0->next;
    p0->next = p;
  }

  // assign operator ID number
  p->next = NULL;
  p->id = ++np;

  // possibly announce formation
  if ((ann > 0) && (noisy >= 1))
  {
    jprintf("\n.................................\n");
    p->Print();
    jprintf(".................................\n\n");
  }
  return 1;
}


//= Remove an operator from the list and permanently delete it.
// must make sure original rem pointer is set to NULL in caller
// used by jhcAliaDir to clean up incomplete ADD operator

void jhcProcMem::Remove (const jhcAliaOp *rem)
{
  jhcAliaOp *prev = NULL, *p = ops;

  // look for operator in list
  if (rem == NULL)
   return;
  while (p != NULL)
  {
    // possibly splice out of list
    if (p == rem)
    {
      if (prev != NULL)
        prev->next = p->next;
      else
        ops = p->next;
      break;
    }

    // move on to next list entry
    prev = p;
    p = p->next;
  }

  // delete given item even if it was not in list
  delete rem;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Find applicable operators that match trigger directive.
// operators and bindings are stored inside directive itself
// returns total number of bindings found

int jhcProcMem::FindOps (jhcAliaDir *dir, jhcWorkMem& wmem, double pth, double mth) const
{
  jhcAliaOp *p = ops;
  int i, k, mc0, mmax;

  // get operator type by examining directive kind
  if (dir == NULL)
    return -2;
  k = dir->kind;
  if ((k < 0) || (k >= JDIR_MAX))
    return -1;
  if ((k == JDIR_BIND) || (k == JDIR_EACH) || (k == JDIR_ANY))
    k = JDIR_FIND;

  // set up to get up to bmax bindings using halo as needed
  mmax = dir->MaxOps();
  dir->mc = mmax;
  wmem.MaxBand(3);

  // try matching all operators above the preference threshold
  while (p != NULL)
  {
    if ((p->kind == k) && (p->pref >= pth))
    {
      // save operator associated with each group of bindings
      mc0 = dir->mc;
      p->dbg = ((p->id == detail) ? 3 : 0);
      if (p->FindMatches(*dir, wmem, mth) < 0)
        break;
      for (i = mc0 - 1; i >= dir->mc; i--)
        dir->op[i] = p;                         
    }
    p = p->next;
  }

  // possibly report summary of what was found
  if (noisy >= 2)
  {
    int n = mmax - dir->mc;
    jprintf("%d matches", n);
    if (n > 0)
      jprintf(": OPS = ");
    for (i = mmax - 1; i >= dir->mc; i--)
      jprintf("%d ", (dir->op[i])->id);
    jprintf("\n");
  }
  return(mmax - dir->mc);
}


///////////////////////////////////////////////////////////////////////////
//                            File Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Read a list of procedures from a file.
// appends to existing advice unless add <= 0
// level: 0 = kernel, 1 = extras, 2 = previous accumulation
// typically give base file name like "KB/kb_072721_1038", fcn adds ".ops"
// returns number of operators read, 0 or negative for problem

int jhcProcMem::Load (const char *base, int add, int rpt, int level)
{
  jhcTxtLine in;
  char full[200], src[80] = "";
  const char *fname = base;
  jhcAliaOp *p;
  char *end;
  int ans = 1, n = 0;

  // possibly clear old stuff then try to open file
  if (add <= 0)
    clear();
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.ops", base);
    fname = full;
  }
  if (!in.Open(fname))
  {
    jprintf("  >>> Could not read operator file: %s !\n", fname);
    return -1;
  }

  // determine provenance string to use
  if (level <= 1)
  {
    strcpy_s(src, fname);
    if ((end = strrchr(src, '.')) != NULL)
      *end = '\0';
  } 

  // try reading operators from file  
  while (ans >= 0)
  {
    // make and load a new operator
    p = new jhcAliaOp;                    
    if ((ans = p->Load(in)) <= 0)
    {
      // delete and purge input if parse error 
      if (!in.End())
        jprintf(">>> Bad syntax at line %d in: %s\n", in.Last(), fname);
      delete p;
      if (in.NextBlank() == NULL)
        break;
    }
    else
    {
      // successful addition
      p->lvl = level;
      strcpy_s(p->prov, src);
      if (AddOperator(p, 0) <= 0)
        delete p;
      n++;
    }
  }

  // possibly announce result
  if (n > 0)
    jprintf(2, rpt, "  %3d action operators from: %s\n", n, fname);
  else
    jprintf(2, rpt, "   -- action operators from: %s\n", fname);
  return n;
}


//= Save all current operators at or above some level to a file.
// level: 0 = kernel, 1 = extras, 2 = previous accumulation, 3 = newly added
// typically give base file name like "KB/kb_072721_1038", fcn adds ".ops"
// returns number of operators saved, negative for problem

int jhcProcMem::Save (const char *base, int level) const
{
  char full[200];
  const char *fname = base;
  FILE *out;
  int cnt;

  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.ops", base);
    fname = full;
  }
  if (fopen_s(&out, fname, "w") != 0)
  {
    jprintf("  >>> Could not write operator file: %s !\n", fname);
    return -1;
  }
  if (level >= 2)
  {
    fprintf(out, "// newly learned operators not in KB0 or KB2\n");
    fprintf(out, "// ==========================================\n\n");
  }
  cnt = save_ops(out, level);
  fclose(out);
  return cnt;
}


//= Save all operators in order irrespective of category.
// returns number saved

int jhcProcMem::save_ops (FILE *out, int level) const
{
  jhcAliaOp *p = ops;
  int cnt = 0;

  while (p != NULL)
  {
    if (p->lvl >= level)
      if (p->Save(out) > 0)
      {
        fprintf(out, "\n\n");
        cnt++;
      }
    p = p->next;
  }
  return cnt;
}


//= Store alterations of preference values relative to KB0 and KB2 operators.
// typically give base file name like "KB/kb_072721_1038", fcn adds ".pref"
// returns number of exceptions stored (writes file)

int jhcProcMem::Alterations (const char *base) const
{
  char full[200];
  const char *fname = base;
  FILE *out;
  const jhcAliaOp *p = ops;
  int na = 0;

  // try opening file and writing header
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.pref", base);
    fname = full;
  }
  if (fopen_s(&out, fname, "w") != 0)
  {
    jprintf("  >>> Could not write preference file: %s !\n", fname);
    return -1;
  }
  fprintf(out, "// learned changes to default operator preferences and durations\n\n");

  // scan through operators for altered values
  while (p != NULL)
  {
    if ((*(p->prov) != '\0') && ((p->pref != p->pref0) || (p->Budget() != p->time0)))
    {
      fprintf(out, "%s %d = %4.2f", p->prov, p->pnum, p->pref);
      if (p->Budget() != p->time0)
        fprintf(out, " : %3.1f + %3.1f", p->tavg, p->tstd);
      fprintf(out, "\n");
      na++; 
    }  
    p = p->next;
  }

  // clean up
  fclose(out);
  return na;
}


//= Change default preference values of KB0 and KB2 operators based on learning.
// typically give base file name like "KB/kb_072721_1038", fcn adds ".pref"
// returns number of operators altered (reads file)

int jhcProcMem::Overrides (const char *base)
{
  jhcTxtLine in;
  char full[200], src[40];
  const char *item, *fname = base;
  jhcAliaOp *p;
  double pf, ta, ts;
  int n, dur, na = 0;

  // try opening file
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.pref", base);
    fname = full;
  }
  if (!in.Open(fname))
  {
    jprintf("  >>> Could not read preference file: %s !\n", fname);
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

    // extract updated confidence value (required)
    if ((item = in.Token()) == NULL)
      break;
    if (strcmp(item, "=") != 0)
      break;
    if ((item = in.Token()) == NULL)
      break;
    if (sscanf_s(item, "%lf", &pf) != 1)
      break;

    // see if any timing information (optional)
    dur = 0;
    if ((item = in.Token()) != NULL)
    {
      if (strcmp(item, ":") != 0)
        break;
      if ((item = in.Token()) == NULL)
        break;
      if (sscanf_s(item, "%lf", &ta) != 1)
        break;
      if ((item = in.Token()) == NULL)
        break;
      if (strcmp(item, "+") != 0)
        break;
      if ((item = in.Token()) == NULL)
        break;
      if (sscanf_s(item, "%lf", &ts) != 1)
        break;
      dur = 1;
    }

    // find matching operator (if any)
    p = ops;
    while (p != NULL)
    {
      if ((*(p->prov) != '\0') && (p->pnum == n) && (strcmp(p->prov, src) == 0))
      {
        // update preference value and possibly duration
        p->pref = pf;
        if (dur > 0)
        {
          p->tavg = ta;
          p->tstd = ts;
        }
        na++;
        break;
      }
      p = p->next;
    }
  }
  return na;
}
