// jhcWorkMem.cpp : main temporary semantic network for ALIA system
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

#include "Action/jhcAliaDir.h"         // common robot

#include "Reasoning/jhcWorkMem.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcWorkMem::~jhcWorkMem ()
{
}


//= Default constructor initializes certain values.

jhcWorkMem::jhcWorkMem ()
{
  halo.NegID();
  src = 0;
  mode = 0;
}


///////////////////////////////////////////////////////////////////////////
//                              List Access                              //
///////////////////////////////////////////////////////////////////////////

//= Returns next node in list, transitioning from halo to main if needed.
// call with prev = NULL to get first node, use SetMode(1) to include halo
// assumes all items in halo node pool have "id" < 0, tries non-halo first

jhcNetNode *jhcWorkMem::NextNode (const jhcNetNode *prev) const
{
  jhcNetNode *n;

  // get start of list if needed, else next node in current list
  if (prev == NULL)
    n = Pool();
  else
    n = Next(prev);                            // pool does not matter

  // possibly switch from main to halo list
  if ((n == NULL) && (mode > 0)) 
    if ((prev == NULL) || (prev->Inst() >= 0))
      n = halo.Pool();
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                            Halo Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Make up network in halo for some pattern (typically result of rule).
// uses main nodes found in bindings, all new nodes asserted with blf = conf
// carefully re-uses main nodes when identical instead of making new halo nodes
// conceptually, facts should not be added to halo if they are already in main 
// returns source label if something made, 0 if nothing made, negative for error

int jhcWorkMem::AssertHalo (const jhcGraphlet& pat, jhcBindings& b, double conf, int tval)
{
  jhcNetNode *n;
  const jhcNetNode *pn;
  int i, ni = pat.NumItems();

  // mark all unbound parts of pattern as eligible for selection
  for (i = 0; i < ni; i++)
  {
    n = pat.Item(i);
    n->mark = ((b.InKeys(n)) ? 0 : 1);
  }

  // select some pattern node that currently has all its args in main
  // bind to similar main node if main node is at least as sure
  while ((pn = args_bound(pat, b)) != NULL)
    if ((n = main_equiv(pn, b, conf)) != NULL)
    {
      b.Bind(pn, n);
      n->TopMax(tval);                           // record NOTE origin (if any)
    }

  // create any new nodes needed in halo (new bindings will obviate some)
  for (i = 0; i < ni; i++)
    if (!b.InKeys(pat.Item(i)))                  // any missing brings in all
    {
      src++;
      halo.Assert(pat, b, conf, src, tval);     
      actualize_halo(src);
      return src;
    }
  return 0;
}


//= Find some pattern nodes having only main node arguments.
// all pattern nodes are local, so main nodes must be linked to by bindings
// returns suitable node, NULL if none

const jhcNetNode *jhcWorkMem::args_bound (const jhcGraphlet& pat, const jhcBindings& b) const
{
  jhcNetNode *n;
  int i, j, na, ni = pat.NumItems();

  // search over all nodes in pattern
  for (i = 0; i < ni; i++)
  {
    // must not be already bound and have some args (mark > 0)
    n = pat.Item(i);
    if (n->mark > 0) 
    {
      // check that all args are bound to a main node
      na = n->NumArgs();
      for (j = 0; j < na; j++)
        if (!b.InKeys(n->Arg(j)))
          break;
      if (j >= na)
      {
        // prevent re-selection and return candidate
        n->mark = 0;
        return n;
      }
    }
  }
  return NULL;
}


//= Find a node in main memory with same arguments, negation, and lexical term as pattern node.
// returns NULL if nothing suitable, else first suitable (most recent)

jhcNetNode *jhcWorkMem::main_equiv (const jhcNetNode *pn, const jhcBindings& b, double conf) const
{
  jhcNetNode *mate = NULL;
  int i, na = pn->NumArgs();
  
  // consider all nodes in main memory
  while ((mate = NextNode(mate)) != NULL)
    if ((mate->Neg() == pn->Neg()) && (mate->Done() == pn->Done()) && (mate->Belief() >= conf) && 
        (mate->NumArgs() == na) && !mate->LexConflict(pn) && mate->SameWords(pn))
    {
      // check that all arguments match
      for (i = 0; i < na; i++)
        if (!mate->HasVal(pn->Slot(i), b.LookUp(pn->Arg(i))))
          break;
      if (i >= na)
        return mate;
    }
  return NULL;
}


//= Mark truth value of asserted rule conclusion with given source number.

void jhcWorkMem::actualize_halo (int src) const
{
  jhcNetNode *n = NULL;

  while ((n = halo.Next(n)) != NULL)
    if (n->pod == src)
    {
      if (n->LexNode())
        MarkBelief(n, 1.0);
      else
        n->Actualize(ver);
    }
}


//= Promote any halo items with given source to main memory.
// creates translation between original halo and new main nodes 
// typically used for the full result of some particular rule

void jhcWorkMem::PromoteHalo (jhcBindings& h2m, int s)
{
  jhcNetNode *n2, *n = NULL;
  const jhcNetNode *n0;
  int i, j, narg, hcnt = 0;

  // make a main node for each halo node using same lex and neg
  h2m.Clear();
  BuildIn(NULL);
  while ((n = halo.Next(n)) != NULL)
   if (n->pod == s)
   {
     n2 = MakeNode(n->Kind());     // sets proper generation
     n2->SetNeg(n->Neg());
     n2->SetDone(n->Done());
     n2->SetBelief(n->Default());  // actualize result pattern
     h2m.Bind(n, n2);
     n->pod = 0;                   // prevent re-selection later
     hcnt++;
   }

  // replicate structure of each halo node for replacement main node
  // rule result only has main nodes and halo nodes with same source (in h2m)
  for (i = 0; i < hcnt; i++)
  {
    // copy each argument from halo node to main node
    n0 = h2m.GetKey(i);
    narg = n0->NumArgs();
    n = h2m.GetSub(i);
    for (j = 0; j < narg; j++)
    {
      // if points to halo node, replace with new main node
      n2 = n0->Arg(j);
      if (n2->Inst() < 0)
        n2 = h2m.LookUp(n2);         // should never fail
      n->AddArg(n0->Slot(j), n2);
    } 
  }
}


//= Print halo assertions with given source (0 = all).
// prefixes it with "HALO N" and indents by lvl (but not prefix if lvl < 0)

void jhcWorkMem::PrintHalo (int s, int lvl) const
{
  jhcNetNode *n = NULL;
  char num[20] = "";
  int lvl2 = abs(lvl) + 2, kmax = 0, nmax = 0, rmax = 0, cnt = 0;

  // get field widths of everything that will be printed
  while ((n = halo.Next(n)) != NULL)
    if ((s <= 0) || (n->pod == s))
      n->TxtSizes(kmax, nmax, rmax);

  // print header
  if (s > 0)
    _itoa_s(s, num, 10);
  jprintf("%*sHALO %s", __max(0, lvl), "", num);

  // print all matching nodes
  n = NULL;
  while ((n = halo.Next(n)) != NULL)
    if ((s <= 0) || (n->pod == s))
      n->Print(lvl2, kmax, nmax, rmax, NULL, ((cnt++ <= 0) ? -1 : 0));   // cnt is hack!

  // blank line at end
  jprintf("\n\n");
}
