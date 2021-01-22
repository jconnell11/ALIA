// jhcProcMem.cpp : procedural memory for ALIA system
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
  jhcAliaOp *p0, *p;
  int i;

  // go through all directive types
  for (i = 0; i < JDIR_MAX; i++)
  {
    // delete all operators in linked list
    p = resp[i];
    while (p != NULL)
    {
      p0 = p;
      p = p->next;
      delete p0;
    }
    resp[i] = NULL;
  }
  np = 0;
  return 0;
}


//= Default constructor initializes certain values.

jhcProcMem::jhcProcMem ()
{
  int i;

  for (i = 0; i < JDIR_MAX; i++)
    resp[i] = NULL;
  np = 0;
  noisy = 2;
}


///////////////////////////////////////////////////////////////////////////
//                             List Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Add item onto tail of list.
// returns id number of item added

int jhcProcMem::AddOperator (jhcAliaOp *p, int ann)
{
  jhcAliaOp *p0;
  JDIR_KIND k;

  // check presence and directive type
  if (p == NULL)
    return 0;
  k = p->kind;
  if ((k < 0) || (k >= JDIR_MAX))
    return 0;

  // add to end of appropriate list
  if ((p0 = resp[k]) == NULL)
    resp[k] = p;
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
    jprintf("\n---------------------------------\n");
    p->Print();
    jprintf("---------------------------------\n\n");
  }
  return np;
}


//= Remove an operator from the list and permanently delete it.
// must make sure original rem pointer is set to NULL in caller
// used by jhcAliaDir to clean up incomplete ADD operator

void jhcProcMem::Remove (const jhcAliaOp *rem)
{
  jhcAliaOp *op, *prev = NULL;
  int k;

  // get appropriate list for operator kind
  if (rem == NULL)
    return;
  k = rem->Kind();
  if ((k < 0) || (k >= JDIR_MAX))
    return;
  op = resp[k];

  // look for operator in list
  while (op != NULL)
  {
     // possibly splice out of list and delete
    if (op == rem)
    {
      if (prev != NULL)
        prev->next = op->next;
      else
        resp[k] = op->next;
      delete op;
      return;
    }

    // move on to next list entry
    prev = op;
    op = op->next;
  }
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Find applicable operators that match trigger directive.
// operators and bindings are stored inside directive itself
// returns total number of bindings found

int jhcProcMem::FindOps (jhcAliaDir *dir, jhcWorkMem& wmem, double pth, double mth) const
{
  jhcAliaOp *p;
  int i, k, mc0, mmax;

  // get operator type by examining directive kind
  if (dir == NULL)
    return -2;
  k = dir->kind;
  if ((k < 0) || (k >= JDIR_MAX))
    return -1;
  p = resp[k];

  // set up to get up to bmax bindings using halo as needed
  mmax = dir->MaxOps();
  dir->mc = mmax;
  wmem.SetMode(2);

  // try matching all operators above the preference threshold
  while (p != NULL)
  {
    if (p->pref >= pth)
    {
      // save operator associated with each group of bindings
      mc0 = dir->mc;
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
// returns number of operators read, 0 or negative for problem

int jhcProcMem::Load (const char *fname, int add, int rpt, int level)
{
  jhcTxtLine in;
  jhcAliaOp *p;
  int ans = 1, n = 0;

  // possibly clear old stuff then try to open file
  if (add <= 0)
    clear();
  if (!in.Open(fname))
  {
    jprintf(">>> Could not open operator file: %s !\n", fname);
    return -1;
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
        jprintf("Bad syntax at line %d in: %s\n", in.Last(), fname);
      delete p;
      if (in.NextBlank() == NULL)
        break;
    }
    else
    {
      // successful addition
      p->lvl = level;
      AddOperator(p, 0);
      n++;
    }
  }

  // possibly announce result
  if (n > 0)
    jprintf(2, rpt, "  %2d action operators from: %s\n", n, fname);
  else
    jprintf(2, rpt, "  -- no action operators from: %s\n", fname);
  return n;
}


//= Save all current operators at or above some level to a file.
// level: 0 = kernel, 1 = extras, 2 = previous accumulation, 3 = newly added
// returns number of operators saved, negative for problem

int jhcProcMem::Save (const char *fname, int level, int cats) const
{
  FILE *out;
  int cnt;

  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  if (cats > 0)
    cnt = save_cats(out, level);
  else
    cnt = save_ops(out, level);
  fclose(out);
  return cnt;
}


//= Save all operators in order irrespective of category.
// returns number saved

int jhcProcMem::save_ops (FILE *out, int level) const
{
  jhcAliaOp *p[JDIR_MAX];
  int i, low, win, cnt = 0;

  // initialize pointers for all categories
  for (i = 0; i < JDIR_MAX; i++)
    p[i] = resp[i];

  while (1)
  {
    // find lowest numbered operator across categories
    low = 0;
    for (i = 0; i < JDIR_MAX; i++)
    {
      // find next suitable operator in this category
      while (p[i] != NULL)
      {
        if (p[i]->lvl >= level)
          break;
        p[i] = p[i]->next;
      }
      if (p[i] == NULL)
        continue;

      // check its id number against current winner
      if ((low <= 0) || (p[i]->id < low))
      {
        low = p[i]->id;
        win = i;
      } 
    }

    // write selected operator (if any) to file
    if (low <= 0)            
      break;
    if (p[win]->Save(out) > 0)
    {
      jfprintf(out, "\n");
      cnt++;
    }
    p[win] = p[win]->next;
  }
  return cnt;
}


//= Save all operators in order by category.
// returns number saved

int jhcProcMem::save_cats (FILE *out, int level) const
{
  jhcAliaOp *p;
  int i, cnt = 0, last = 0, any = 0;

  // go through all categories of triggering directives
  for (i = 0; i < JDIR_MAX; i++)
  {
    // go through all operators in list
    p = resp[i];
    while (p != NULL)
    {
      if (p->lvl >= level)
      {
        // possibly print category separator then always print operator itself
        if ((last > 0) && (any <= 0))
          jfprintf(out, "// ============================================================\n\n");
        if (p->Save(out) > 0)
        {
          jfprintf(out, "\n");
          cnt++;
          any = 1;
        }
      }
      p = p->next;
    }

    // separator control variables
    if (any > 0)
    {
      last = any;
      any = 0;
    }
  }
  return cnt;
}


