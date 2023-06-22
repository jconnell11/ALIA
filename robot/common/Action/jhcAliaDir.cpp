// jhcAliaDir.cpp : directive states what sort of thing to do in ALIA system
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

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"
#include "Interface/jrand.h"

#include "Action/jhcAliaCore.h"        // common robot

#include "Action/jhcAliaDir.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Shared array of directive names.
// strings must remain consistent with JDIR_KIND enumeration

const char * const jhcAliaDir::ktag[JDIR_MAX] = {"NOTE", "DO", "ANTE", "GATE", "PUNT", "GND", "WAIT", "ACH",
                                                 "FIND", "BIND", "EACH", "ANY", "CHK", "ESC", "ADD", "EDIT"};


//= Default destructor does necessary cleanup.
// jhcAliaChain takes care of deleting rest of graph
// nodes in key are reclaimed by garbage collector

jhcAliaDir::~jhcAliaDir ()
{
  jhcAliaCore *core;

  halt_subgoal();
  delete meth;               
  if (step != NULL)
  {
    core = step->Core();
    core->Remove(new_rule);
    core->Remove(new_oper);
  }
}


//= Default constructor initializes certain values.

jhcAliaDir::jhcAliaDir (JDIR_KIND k)
{
  // basic info
  kind = k;

  // payload for ADD
  new_rule = NULL;
  new_oper = NULL;

  // default configuration
  step = NULL;
  nsub = 0;
  root = 0;
  inum0 = 0;             // iteration number of innermost loop

  // processing state
  meth = NULL;
  reset();
  verdict = -1;

  // messages
  noisy = 0;             // sequence progress statements (defaulted from jhcAliaCore)
  dbg = 0;               // FIND processing steps (defaulted from jhcAliaCore)

  // FIND state
  cand = 0;              // guess number always increases (never reset)
  subset = 0;            // set if FIND or CHK sub-method needs CHK follow-up
  cand0 = 0;

  // NOTE perseverance
  t0 = 0;
  t1 = 0;
  fin = 0;
}


//= Clear out all previous method attempts and bindings.
// does not remove directive definition (kind and key) 
// typically used for switching between ANTE->GATE->DO 
// returns 0 always for convenience

int jhcAliaDir::reset ()
{
  // nothing selected or running (no stop needed)
  delete meth;
  meth = NULL;
  inst = -1;
  sel = -1;

  // reset FIND choices if new iteration of some loop
  if ((kind == JDIR_FIND) || (kind == JDIR_BIND))
    if ((step != NULL) && (inum0 != step->inum))
    {
      cand = 0;
      inum0 = step->inum;
    }

  // current state (running)
  nri = 0;
  verdict = 0;
  wait = -1;
  chk_state = 0;

  // no general node matches permitted
  own = 0;
  t0 = jms_now();
  return 0;
}


//= Given a string for some kind, return associated kind number.
// only looks at prefix of supplied name, e.g. "find-ako" matches "FIND"
// returns JDIR_MAX if bad string

JDIR_KIND jhcAliaDir::CvtKind (const char *name) const
{
  int k;

  for (k = 0; k < JDIR_MAX; k++)
    if (_strnicmp(name, ktag[k], strlen(ktag[k])) == 0)
      break;
  return((JDIR_KIND) k);
}


//= Find name string associated with some particular kind.

const char *jhcAliaDir::CvtTag (JDIR_KIND k) const
{
  if ((k < 0) || (k >= JDIR_MAX))
    return NULL;
  return ktag[k];
}


//= Determine the maximum subgoal depth for this part of the tree.
// cyc is unique request number

int jhcAliaDir::MaxDepth (int cyc)
{
  if (meth == NULL)
    return 1;
  return(meth->MaxDepth(cyc) + 1);
}


//= Determine number of simultaneous activities (possibly subgoaled).
// if leaf > 0 then only considers currently active goals not pass-thrus
// cyc is unique request number

int jhcAliaDir::NumGoals (int leaf, int cyc)
{
  int n;

  if (meth == NULL)
    return 1;
  n = meth->NumGoals(leaf, cyc);
  return((leaf > 0) ? n : n + 1);
}


///////////////////////////////////////////////////////////////////////////
//                                Building                               //
///////////////////////////////////////////////////////////////////////////

//= Copy description part (not bindings or NRI) of some other directive.
// backbone FSM graph structure handled by jhcAliaChain

void jhcAliaDir::Copy (const jhcAliaDir& ref)
{
  kind = ref.kind;
  key.Copy(ref.key);
}


//= Copy a prototype directive but substitute for the nodes in the binding list.
// returns 1 if successful, 0 if problem

int jhcAliaDir::CopyBind (jhcNodePool& pool, const jhcAliaDir& ref, jhcBindings& b, const jhcGraphlet *ctx2)
{
  int ans;

  // copy directive kind and set up to build key
  key.Clear();
  kind = ref.kind;
  pool.BuildIn(key);

  // copy bulk of directive
  if ((ans = pool.Assert(ref.key, b, 0.0)) >= 0)           // negative is failure (was 1.0)
    if ((kind == JDIR_DO) && (ctx2 != NULL))
    {
      ctx.Copy(*ctx2);                                     // for cascading
      share_context(pool, ctx2);
    }

  // cleanup
  pool.BuildIn(NULL);
  return((ans < 0) ? 0 : 1);
}


//= Add in any adverbs or objects from higher-level call (for DO directives only).
// shares nodes already in wmem (e.g. mod-23 -mod-> act-11 and now mod-23 -mod-> act-62 also)

void jhcAliaDir::share_context (jhcNodePool& pool, const jhcGraphlet *ctx2) 
{
  const jhcNetNode *old, *p, *d;
  jhcNetNode *p2, *d2, *act = key.MainAct();
  int i, j, cnt, cnt2;

  // sanity check
  if (ctx2 == NULL)
    return;
  if ((old = ctx2->MainAct()) == NULL)
    return;
/*
  // add in all old arguments (just extra pointers)
  cnt = old->NumArgs();
  for (i = 0; i < cnt; i++)
  {
    a = old->Arg(i);
    if (ctx2->InDesc(a))
    {
      act->AddArg(old->Slot(i), a);
      key.AddItem(a);
    }
  }   
*/
  // add in all listed properties of old action
  cnt = old->NumProps();
  for (i = 0; i < cnt; i++)
  {
    p = old->Prop(i);
    if (ctx2->InDesc(p) && (p->Val("fcn") == NULL))
    {
      // copy base modifier ("slowly")
      p2 = pool.CloneNode(*p, 0);
      p2->AddArg(old->Role(i), act);
      key.AddItem(p2);

      // add any intensifiers ("very")
      cnt2 = p->NumProps();
      for (j = 0; j < cnt2; j++)
      {
        d = p->Prop(j);
        if (ctx2->InDesc(d))
        {
          d2 = pool.CloneNode(*d, 0);
          d2->AddArg(p->Role(j), p2);
          key.AddItem(d2);
        }
      }
    }
  }   
}


//= Use a textual name (case insensitive) to determine kind for this directive.
// only looks at prefix of supplied tag, e.g. "find-ako" matches "FIND"
// returns 1 if valid choice, 0 if left unchanged (often JDIR_NOTE default)

int jhcAliaDir::SetKind (const char *tag)
{
  int k;

  for (k = 0; k < JDIR_MAX; k++)
    if (_strnicmp(tag, ktag[k], strlen(ktag[k])) == 0)
    {
      kind = (JDIR_KIND) k;
      return 1;
    }
  return 0;
}


//= Tells whether some node appears somewhere in description.

bool jhcAliaDir::Involves (const jhcNetNode *item) const
{
  const jhcNetNode *anchor;
  int i, j, na, n = key.NumItems();

  // sanity check
  if (item == NULL)
    return false;

  // go through parts of key graphlet
  for (i = 0; i < n; i++)
  {
    // make sure key item does not match
    if ((anchor = key.Item(i)) == item)
      return true;

    // make sure no argument of key item matches
    na = anchor->NumArgs();
    for (j = 0; j < na; j++)
      if (anchor->Arg(j) == item)
        return true;
  }
  return false;
}


//= Add main action of directive as an argument to source node.
// returns 1 if successful, 0 if probem (only used by jhcNetBuild)

int jhcAliaDir::RefDir (jhcNetNode *src, const char *slot, jhcNodePool& pool) const
{
  jhcNetNode *item;
  int i, n = key.NumItems(), ok = 1;

  // ad main command node
  if (kind == JDIR_DO)
    return src->AddArg(slot, KeyMain());

  // make up check action (never a directive itself)
  if (kind == JDIR_CHK)
  {
    item = pool.MakeAct("check");
    item->AddArg("obj", KeyMain());
    return src->AddArg(slot, item);
  }

  // make up find action (not for BIND from jhcNetRef)
  if (kind == JDIR_FIND)
  {
    item = pool.MakeAct("find");
    item->AddArg("obj", KeyMain());
    return src->AddArg(slot, item);
  }

  // add all predicate-like assertions from NOTE
  if (kind == JDIR_NOTE)
    for (i = 0; i < n; i++)
    {
      item = key.Item(i);
      if (!item->ObjNode())
        if (src->AddArg(slot, item) <= 0)
          ok = 0;
    }
  return ok;
}


//= Set mark to one for all nodes belonging to instantiated directives.
// useful for mark/sweep garbage collection (jhcActionTree::clean_mem)
// NOTE: also keep key substitutions, variable guesses?

void jhcAliaDir::MarkSeeds ()
{
  jhcNetNode *n;
  int i, j, nb, ki = key.NumItems(), hi = hyp.NumItems();

  // mark all nodes in key (may not be in NRI yet)
  if ((ki > 0) && (noisy >= 5))
    jprintf("    directive key %s[ %s ]\n", KindTag(), KeyTag());
  for (i = 0; i < ki; i++)
  {
    n = key.Item(i);
    jprintf(5, noisy, "      %s\n", n->Nick());
    n->SetKeep(1);
  }

  // mark all undo choices for variable substitution in key
  if ((nsub > 0) && (noisy >= 5))
    jprintf("    variable substitutions\n");
  for (i = 0; i < nsub; i++)
  {
    n = ((val0[i] != NULL) ? val0[i] : fact[i]);
    jprintf(5, noisy, "      %s\n", n->Nick());
    n->SetKeep(1);
  }

  // mark all current and previous guesses
  if ((cand > 0) && (noisy >= 5))
    jprintf("    variable guesses\n");
  for (i = 0; i < cand; i++)
  {
    n = guess[i];
    jprintf(5, noisy, "      %s\n", n->Nick());
    n->SetKeep(1);
  }    

  // mark all hypothetical structure assumed by FIND
  if ((hi > 0) && (noisy >= 5))
    jprintf("    FIND assumption\n");
  for (i = 0; i < hi; i++)
  {
    n = hyp.Item(i);
    jprintf(5, noisy, "      %s\n", n->Nick());
    n->SetKeep(1);
  }

  // keep all NRI bindings so no dangling pointers (or reused elements)
  if ((nri > 0) && (noisy >= 5))
    jprintf("    non-return inhibition\n");
  for (i = 0; i < nri; i++)
  {
    nb = m0[i].NumPairs();
    for (j = 0; j < nb; j++)
    {
      n = m0[i].GetSub(j);
      jprintf(5, noisy, "      %s\n", n->Nick());
      n->SetKeep(1);
    }
  }

  // see if some current expansion (even if finished)
  if (meth != NULL)
  {
    jprintf(5, noisy, "    bound method\n");
    meth->MarkSeeds(1);
  }
}


///////////////////////////////////////////////////////////////////////////
//                             Main Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Start processing this directive (does no operator picking).
// most directives need at least one call to Status() before completing
// so generally Start() should NOT fail, which imples FcnStart() should never fail
// always returns: 0 = working

int jhcAliaDir::Start (jhcAliaChain *st)
{
  jhcAliaCore *core = st->Core();
  jhcActionTree *wmem = &(core->atree);
  jhcBindings *scope = st->Scope();
  double s;
  int lvl = st->Level(), ver = wmem->Version();

  // set up internal state, assume something needs to run
  step = st;
  subst_key(scope);                              // substitute found variables
  noisy = core->noisy;
  dbg = core->finder;
//noisy = 2;                                     // control structure debugging
//dbg = 2;                                       // local FIND debug (2 or 3 for matcher)
  wmem->RevealAll(key);                          // make available for matching
  reset();     
  (core->mood).Launch(); 
  jprintf(2, noisy, "%*sstart %s[ %s ]\n", lvl, "", KindTag(), KeyTag());

  // initialize various types of directive
  if ((kind == JDIR_DO) && !key.Empty())
  {
    jprintf(2, noisy, "%*sConverting DO->ANTE[ %s ] - init\n\n", lvl, "", KeyTag());
    kind = JDIR_ANTE;                     
  }
  else if (kind == JDIR_GND)
    inst = core->GndStart(key.Main());            // negative inst records fail
  else if (kind == JDIR_NOTE)
  {
    if (step->inum > 0)                           // make new copy for each loop
      divorce_key(wmem, lvl);
    key.ActualizeAll(ver);                        // assert pending truth values
    wmem->Refresh(key);
    if (wmem->Endorse(key) <= 0)                  // too coarse?
    {
      s = wmem->CompareHalo(key, core->mood);     // update rule result beliefs
      (core->mood).Believe(s);
    }
    own = core->Percolate(key);                   // mark nodes for reactive ops
  }
  else if ((kind == JDIR_FIND) || (kind == JDIR_BIND) || 
           (kind == JDIR_EACH) || (kind == JDIR_ANY)  ||
           (kind == JDIR_CHK)  || (kind == JDIR_ESC))
    init_cond(ver);                               // situation to search for

  // report current status
  return report(verdict);
}


//= Build a new version of original key to be used on next loop iteration.
// copies structure of original graphlet into wmem using new nodes to make alternative version
// arguments that are outside original description list remain the same
// actual "key" is altered, original substituted key is stored in "full"

void jhcAliaDir::divorce_key (jhcWorkMem *pool, int lvl)
{
  jhcBindings b;
  const jhcNetNode *n;
  jhcNetNode *n2, *arg, *a2;
  int i, j, na, ni = key.NumItems();

  // copy all nodes listed in description (before substitutions)
  full.Copy(key);
  key.Clear();
  for (i = 0; i < ni; i++)
  {
    n = full.Item(i);
    n2 = pool->CloneNode(*n, 0);
    n2->Reveal();
    b.Bind(n, n2);
    key.AddItem(n2);
  }

  // relink all arguments (external args remain the same)
  for (i = 0; i < ni; i++)
  {
    n = full.Item(i);
    n2 = b.LookUp(n);
    na = n->NumArgs();
    for (j = 0; j < na; j++)
    {
      arg = n->ArgSurf(j);
      if ((a2 = b.LookUp(arg)) == NULL)
        a2 = arg;      
      n2->AddArg(n->Slot(j), a2);  
    }
  }
}


//= Continue running this directive (operator picking phase).
// "meth" or "inst" always bound at this point, "verdict" always 0
// assigns priority level in case function is a background async thread
// jhcAliaChain takes care of transferring activation within chain
// in most cases, an unbound alternate continuation is a failure
// this allows for sanity checks, i.e. don't continue if expectation fails
// yet CHKs in the expansion of other CHKs allow 2 to be an okay return value
// return: 1 (or 2) = done, 0 = working, -2 = fail, -3 = stop backtrack

int jhcAliaDir::Status ()
{
  jhcBindings d2o;
  jhcAliaCore *core = step->Core();
  jhcActionTree *atree = &(core->atree);
  const jhcNetNode *main;
  jhcAliaOp *op;
  int res, lvl = step->Level();

  // determine whether to record grounding kernel error messages
  if (lvl <= 0)
    core->SaveErr(1);
  if ((kind != JDIR_DO) && (kind != JDIR_GND) && (kind != JDIR_ACH))
    core->SaveErr(0);

  // test for special cases
  if (kind == JDIR_PUNT)                                   // discontinue chain
    return report(-3);             
  if ((kind == JDIR_DO) && key.Empty())                    // no-op always succeeds
    return report(1);
  if (kind == JDIR_ADD)                                    // accept new rule/op
  {
    if ((res = core->Accept(new_rule, new_oper)) > 0)
    {
      new_rule = NULL;                                     // save from d'tor deletion
      new_oper = NULL;
    }
    else if ((new_rule != NULL) && (res == -4))
      err_rule();                                          // reason for ADD failure
    return report((res > 0) ? 1 : -2);
  }
  if (kind == JDIR_GND)                                    // run some function
    return report(core->GndStatus(key.Main(), inst));      
  if ((kind == JDIR_DO) && (key.ActNeg() > 0))             // stop matching action
    return report(atree->HaltActive(key, this, core->NextBid()));
  if (kind == JDIR_EDIT)                                   // alter operator 
  {
    if ((op = atree->Motive(key, &main, &d2o)) == NULL)
      return report(-2);
    if (key.ActNeg() > 0)
      atree->AdjOpPref(op, -1);                            // ding preference
    else if (step->cont == NULL)
      atree->AdjOpPref(op, 1);                  
    if (core->OpEdit(*op, *main, d2o, step->cont) <= 0)    // replace step
      return report(-2);
    return report(1);
  }

  // see if directive already complete (ACH, WAIT, FIND/BIND/EACH/ANY)
  if ((res = chk_preempt()) != 0)
     return report(res);
  if ((t1 != 0) && (jms_elapsed(t1) > core->Retry()))      // try all operators again
  {
    jprintf(1, noisy, "@@@ retry intention for: NOTE[ %s ]\n", KeyTag());
    meth->Start(core, -(lvl + 1));
    t1 = 0;
    return report(0);
  }

  // find some operator to try if nothing selected yet
//  if (kind == JDIR_BIND)
//    return recall_assume();                      // never tries any operators (good?)
  if (kind == JDIR_WAIT)
    return report(0);                            // no associated operators
  if (meth == NULL)
  {
    if (wait >= 0)                               // wait inits to -1
      return next_method();
    wait = 2;                                    // no initial wait
    return first_method();
  }
  else if (chk_state == 1)                       // residual CHK needs halo to catch up
  {
    meth->Start(core, lvl + 1);                  // enter CHK mode 
    chk_state = 2;
    return report(0);
  }

  // see if current method has finished yet (also check for PUNT in method)
  jprintf(2, noisy, "%*sPassing through %s[ %s ] = %s\n", lvl, "", KindTag(), KeyTag(), KeyNick());
  if ((res = meth->Status()) == 0)
    return verdict;
  result[nri - 1] = res;                         // nri incremented in pick_method
  chk_state = 0;
  if (res <= -3)
    return report(-3);

  // maybe raise preference threshold
  if ((res == -2) && (wait <= 0))
    (core->mood).BumpMinPref(1);                 

  // generally try next method, but special handling for DO and NOTE 
  if (kind == JDIR_DO)
    return do_status(res, 1);
  if (kind == JDIR_GATE)
  {
    jprintf(2, noisy, "%*sConverting GATE->DO[ %s ] - operator applied\n\n", lvl, "", KeyTag());
    reset();                                     // clear nri for DO alter_pref
    kind = JDIR_DO;                              // none -> advance to DO phase 
    if (res < 0)
      return report(-1);
    return report(0);
  }
  if ((kind == JDIR_NOTE) && (res == -2))
  {
    if (t1 != 0)                                 // already waiting for retry
      return report(0);
    if ((core->Stretch(op0[0]->Budget()) - jms_elapsed(t0)) < core->Retry())
      return report(1);
    jprintf(1, noisy, "### Pause intention for: NOTE[ %s ]\n", KeyTag());
    t1 = jms_now();
    return report(0);
  }
  if (kind == JDIR_NOTE)                         // NOTE only tries one operator ever
    return report(1);                            
  return next_method();                          // ANTE, GATE, FIND, CHK run all
}


//= Check for pre-emptive full matches to ACH, WAIT, CHK, and FIND criteria.
// also checks if NOTE is no longer valid
// returns 0 to continue, non-zero for result to report

int jhcAliaDir::chk_preempt ()
{
  int lvl = step->Level(), res = 1;                        // assume early success

  // check for various sorts of matches
  if ((kind == JDIR_ACH) || (kind == JDIR_WAIT))           // see if goal achieved
  {
    if (pat_confirm(key, -1) > 0)
      jprintf(2, noisy, "%*s### direct full %s[ %s ] match\n",  lvl, "", KindTag(), KeyTag());
    else
      res = 0;
  }
  else if ((kind == JDIR_CHK) || (kind == JDIR_ESC))       // look for match or anti-match
  {
    if ((res = seek_match()) != 0)
      jprintf(2, noisy, "%*s### direct full %s[ %s ] match = %d\n",  lvl, "", KindTag(), KeyTag(), res);
  }
  else if ((kind == JDIR_FIND) || (kind == JDIR_BIND) ||   // look for fortuitous matches
           (kind == JDIR_EACH) || (kind == JDIR_ANY))
  {
    if ((res = me_you()) == 0)
      if ((res = seek_instance()) != 0)
        if (res > 0)
          jprintf(2, noisy, "%*s### direct full %s[ %s ] match = %d\n", lvl, "", KindTag(), KeyTag(), res);
  }
/*
  else if (kind == JDIR_NOTE)                              // check for change in key
  {
    if (key.Moot())
      jprintf(2, noisy, "%*s### mooted %s[ %s ]\n", lvl, "", KindTag(), KeyTag());
    else
      res = 0;
  }
*/
  else
    res = 0;                                               // no match so continue

  // possibly stop any on-going method
  if (res != 0)
    halt_subgoal();
  if (kind == JDIR_ESC)
    return((res == 1) ? -2 : 1);                           // ESC item true -> fail
  return res;
}


//= Get initial method to try for this directive.
// special handling for 3 phase DO, can never fail from here

int jhcAliaDir::first_method ()
{
  jhcAliaCore *core = step->Core();
  int found, lvl = step->Level();

  // possibly print debugging delimiter
  if (noisy >= 2)
  {
    jprintf("\n~~~~~~~~ first selection ~~~~~~~~\n");
    Print();
  }

  // always try to find some applicable operator 
  if ((found = pick_method()) > 0) 
  {
    if ((kind == JDIR_FIND) || (kind == JDIR_BIND) ||      // keep generating answers
        (kind == JDIR_EACH) || (kind == JDIR_ANY))
      meth->Enumerate();
    return report(meth->Start(core, lvl + 1));             // found -> run operator
  }
  if (found == 0)                                           
    return report(0);                                      // waiting

  // no more operators - special handling for various types
  if (kind == JDIR_ANTE) 
  {
    jprintf(2, noisy, "%*sConverting ANTE->GATE[ %s ] - no more ops\n\n", lvl, "", KeyTag());
    alter_pref();
    reset();                                               // clear nri for GATE alter_pref
    kind = JDIR_GATE;                                      // none -> advance to GATE phase 
  }
  else if (kind == JDIR_GATE)        
  {
    jprintf(2, noisy, "%*sConverting GATE->DO[ %s ] - no more ops\n\n", lvl, "", KeyTag());
    alter_pref();
    reset();                                               // clear nri for DO alter_pref
    kind = JDIR_DO;                                        // none -> advance to DO phase 
  }
  else if ((kind == JDIR_FIND) || (kind == JDIR_BIND) ||
           (kind == JDIR_EACH) || (kind == JDIR_ANY))
    verdict = recall_assume();                             // clears guesses
  else if (((kind == JDIR_CHK) || (kind == JDIR_ESC)) && (chk0 > 0))
    chk0 = 0;                                              // fact does not have to be new
  else if (kind == JDIR_ESC)
    verdict = 1;                                           // ESC fact unknown -> continue
  else if (kind == JDIR_NOTE)
    verdict = 1;                                           // NOTE always succeeds
  else 
    verdict = -2;                                          // none -> generally fail
  return report(verdict);
}


//= Handle operator finish for DO directive.
// res = 2, 1, or -2 but never 0 (working), -1 (forced stop), or -3 (stop backtrack)
// can prevent selecting different operator if more <= 0

int jhcAliaDir::do_status (int res, int more)
{
  jhcAliaCore *core = step->Core();
  int found, lvl = step->Level();

  // if this operator failed, try another one (if exists) 
  if ((res == -2) && (more > 0))                           // -3 = stop backtrack
  {
    if ((found = pick_method()) > 0)
      return report(meth->Start(core, lvl + 1));    
    if (found == 0)
      return report(0);
  }
  return report(res);                                      
}


//= Pick next best operator for directive.
// special handling for 3 phase DO

int jhcAliaDir::next_method ()
{
  jhcAliaCore *core = step->Core();
  int found, lvl = step->Level();

  // possibly print debugging stack
  jprintf(2, noisy, "Continuing on %s[ %s ]\n", KindTag(), KeyTag());

  // see if any more applicable operators (most directives try all)
  if ((found = pick_method()) > 0)
  {
    if ((kind == JDIR_FIND) || (kind == JDIR_BIND) ||      // keep generating answers
        (kind == JDIR_EACH) || (kind == JDIR_ANY))
      meth->Enumerate();
    return report(meth->Start(core, lvl + 1));             // found -> run operator
  }
  if (found == 0)
    return report(0);                                      // waiting

  // no more operators - special handling for various types
  if (kind == JDIR_ANTE)
  {
    jprintf(2, noisy, "Converting ANTE->GATE[ %s ] - all tried\n\n", KeyTag());
    alter_pref();  
    reset();                                               // clear nri for GATE alter_pref
    kind = JDIR_GATE;                                      // none -> advance to GATE phase
  }
  else if (kind == JDIR_GATE)                              // unlikely
  {
    jprintf(2, noisy, "Converting GATE->DO[ %s ] - all tried\n\n", KeyTag());
    alter_pref();  
    reset();                                               // clear nri for DO alter_pref
    kind = JDIR_DO;                                        // none -> advance to DO phase
  }
  else if ((kind == JDIR_FIND) || (kind == JDIR_BIND) ||
           (kind == JDIR_EACH) || (kind == JDIR_ANY))
    verdict = recall_assume();                             // clears guesses
  else if (kind == JDIR_NOTE)
    verdict = 1;                                           // NOTE always succeeds
  else 
    verdict = -2;                                          // none -> generally fail
  return report(verdict);
}


//= Announce success or failure of some branch for debugging.
// sets verdict to value given (often just verdict itself) and returns this

int jhcAliaDir::report (int val)
{
  const char hdr[6][20] = {"... blocked", "--- failure", "::: dismiss", "", 
                           "*** success", "~~~ escaped"};
  jhcAliaCore *core = step->Core();
  int lvl = step->Level();

  // possibly announce a transition (make indenting line up by level)
  if ((noisy >= 1) && (val >= -3) && (val <= 2) && (val != 0))
    jprintf("%*s%s %4s[ %s ] : %s\n", lvl, "", hdr[val + 3], KindTag(), KeyTag(), KeyNick());

  // clean up if no longer active (method already stopped and can be scrapped)
  if (val != 0)
  {
    // possibly update expected time for free choice reaction 
    if ((kind == JDIR_NOTE) && (nri > 0))
    {
      op0[0]->AdjTime(jms_elapsed(t0));
      jprintf(2, noisy, "  TIME: operator %d --> duration %3.1f + %3.1f\n", 
              op0[0]->OpNum(), op0[0]->Time(), op0[0]->Dev());
    }

    // possibly adjust operator preference and modulate mood
    if (val > 0)
    {
      alter_pref();                              // only when top level goal succeeds
      (core->mood).Win();
    }
    else if (val == -2)                          // ignore Stop (val == -1)
      (core->mood).Lose();

    // old failure reason is moot since this action succeeded
    if ((val > 0) && (kind == JDIR_DO))
      (core->atree).ClrFail();

    // increment loop count (needed to reset later FIND/BINDs)
    if ((val == 1) && ((kind == JDIR_EACH) || (kind == JDIR_ANY)))
      step->inum = ++inum0;

    // possibly block halo percolation then remove working memory anchor
    own = 0;
//    delete meth;
//    meth = NULL;
    inst = -1;                                   // reset() would bash nri for FIND                     
    fin = jms_now();
  }

  // convert some successes into failures
  verdict = -2;
  if ((val == 2) && (step->alt == NULL) && (step->alt_fail > 0))
    jprintf(1, noisy, "%*s--- failure: %4s[ %s ]  revised since no alt branch\n", lvl, "", KindTag(), KeyTag());
  else if ((val == 1) && step->Variations() && (kind != JDIR_FIND))             
    jprintf(1, noisy, "%*s--- failure: %4s[ %s ]  revised to generate variants\n", lvl, "", KindTag(), KeyTag());
  else
    verdict = val;                               // restore if reset() erases 
  return verdict;
}


//= Possibly increase or decrease current operator preference based on experience.
// preference value encodes both "select at all" and "select instead of"
// goes backward through OPs tried rewarding successes and punishing failures

void jhcAliaDir::alter_pref () const
{
  jhcAliaCore *core = step->Core();
  const jhcActionTree *atree = &(core->atree);
  int i, win = 0;         

  // possibly announce entry
  if (nri <= 0)
    return;
  jprintf(2, noisy, "alter_pref: %s[ % s ] success\n", KindTag(), KeyTag());

  // go through OPs - may be many successes and failures for ANTE and ACH
  for (i = nri - 1; i >= 0; i--)
  {
    jprintf(2, noisy, "  %d: OP %d -> verdict %d (pref %4.2f)\n", i, op0[i]->OpNum(), result[i], op0[i]->Pref());
    if (result[i] > 0)
    {
      // move successful OPs closer to nominal preference
      win = 1;
      if (op0[i]->Pref() < atree->RestPref())
        atree->AdjOpPref(op0[i], 1);
    }
    else if ((result[i] == -2) && (win > 0))
      // decrease selection probability of failed OPs if something later succeeded
      atree->AdjOpPref(op0[i], -1);
  }
}


//= Record complaint about new rule user just tried to add
// <pre>
//    NOTE[ ako-1 -lex-  duplicate
//                -ako-> rule-1 ]
// </pre>

void jhcAliaDir::err_rule ()
{
  jhcAliaNote *rpt = step->ATree();
  jhcAliaDesc *fail;

  rpt->StartNote();
  fail = rpt->NewProp(KeyMain(), "ako", "duplicate");
  rpt->FinishNote(fail);
}


//= Courtesy signal to activity that it is no longer needed. 
// returns current non-zero verdict (fail, stop, cont, or alt)

int jhcAliaDir::Stop ()
{
  // see if already done
  if (verdict != 0)
    return verdict;
  halt_subgoal();

  // mark everything as forcibly stopped
  verdict = -1;
  return report(verdict);
}


//= Quickly terminate any activity undertaken by this directive.
// handles both method expansions and kernel functions

void jhcAliaDir::halt_subgoal ()
{
  jhcAliaCore *core = ((step == NULL) ? NULL : step->Core());

  if (meth != NULL)
  {
    meth->Stop();
    if (nri > 0)
      result[nri - 1] = 1;             // top level FIND or CHK succeeded
  }
  else if ((inst >= 0) && (core != NULL))
    inst = core->GndStop(key.Main(), inst);
}


///////////////////////////////////////////////////////////////////////////
//                            Variable Scoping                           //
///////////////////////////////////////////////////////////////////////////

//= Change directive key by substituting in variable bindings from prior FINDs.
// destructively changes nodes in key graphlet but does not eliminate any 
// this is a relatively clean way to preserve bindings in the rest of chain

void jhcAliaDir::subst_key (jhcBindings *sc)
{
  jhcAliaCore *core = step->Core();
  jhcActionTree *wmem = &(core->atree);
  jhcNetNode *item, *arg, *sub;
  int j, na, i = 0;

  // undo any substitutions from earlier guesses
  wmem->Dirty(nsub);                   
  revert_key();                                
  if ((sc == NULL) || sc->Empty())     
    return;
  if (cand > 0)                        // remove any previous guess
    sc->RemKey(key.Main());

  // go through items in graphlet (which may be changing)
  while (i < key.NumItems())   
  {
    item = key.Item(i);
    if (sc->InKeys(item))
    {
      // remove entry from graphlet but keep record
      key.RemItem(i);
      if (nsub >= smax)
        jprintf(">>> More than %d alterations in jhcAliaDir::subst_key!\n");
      else
      {
        fact[nsub] = item;
        anum[nsub] = -1;
        val0[nsub] = NULL;
        nsub++;
      }
      continue;
    }

    // possibly replace arguments of this item
    na = item->NumArgs();
    for (j = 0; j < na; j++)
    {
      arg = item->Arg(j);
      if ((sub = sc->LookUp(arg)) != NULL)
        if (sub != arg)
        {
          // destructivey modify node but keep record
          item->SubstArg(j, sub);
          if (nsub >= smax)
            jprintf(">>> More than %d alterations in jhcAliaDir::subst_key!\n");
          else
          {
            fact[nsub] = item;
            anum[nsub] = j;
            val0[nsub] = arg;
            nsub++;
          }
        }
    }
    i++;
  }

  // make sure halo rule bindings reflect any changes
  wmem->Dirty(nsub);
}


//= Restore key graphlet to condition before subst_key was called.
// undoes editing and destructive modifications (should update halo)

void jhcAliaDir::revert_key ()
{
  int i;

  if ((kind == JDIR_NOTE) && !full.Empty())
    key.Copy(full);
  for (i = 0; i < nsub; i++)
    if (val0[i] == NULL)
      key.AddItem(fact[i]);
    else
      fact[i]->SubstArg(anum[i], val0[i]);
  nsub = 0;
}


///////////////////////////////////////////////////////////////////////////
//                            Method Selection                           //
///////////////////////////////////////////////////////////////////////////

//= Match directive along with memory and halo to find good operators.
// consults list of previously tried operators and bindings to avoid repeats
// returns 1 if something new found, 0 if none but waiting, -1 if out of ideas
// NOTE: bulk of actual work is in jhcProcMem::FindOps and jhcAliaOp::FindMatches

int jhcAliaDir::pick_method ()
{
  jhcAliaCore *core = step->Core();
  jhcGraphlet ctx2;
  int all, cnt, lvl = step->Level();

  // see if too many methods tried or perhaps wait (2 cycles) for NOTEs to post then halo rules to run 
  if (nri >= hmax)
    return jprintf(">>> More than %d operators failed in jhcAliaDir::pick_method !\n", hmax);
  if (wait++ < 2)
    return jprintf(2, noisy, "%*s... select wait: %s[ %s ]\n", lvl, "", KindTag(), KeyTag());
  wait = 0;

  // find maximally specific operators that apply
  if (lvl > core->MaxStack())
    return jprintf("%*s>>> Subgoal stack too deep for %s[ %s ] !\n", lvl, "", KindTag(), KeyTag());
  all = match_ops(sel);
  cnt = max_spec(sel);
  if (cnt <= 0)
  {
    if (noisy >= 2) 
      jprintf("%*s  No more applicable operators for %s[ %s ]\n", lvl, "", KindTag(), KeyTag());
    else if ((noisy >= 1) && (nri <= 0) && 
             ((kind == JDIR_DO) || (kind == JDIR_ACH) || (kind == JDIR_CHK) || (kind == JDIR_ESC)))
      jprintf("%*s  No applicable operators for %s[ %s ]\n", lvl, "", KindTag(), KeyTag());
    return -1;
  }

  // choose among valid options (if several)
  if (cnt > 1)
    sel = wtd_rand((core->atree).Wild());
  else
    jprintf(2, noisy, "    OP %d: sp %d x %4.2f -> 100%%\n", op[sel]->OpNum(), match[sel].NumPairs(), op[sel]->Pref());
  if (noisy >= 4)
  {
    jprintf("Selected OP %d for %s[ %s ]:\n", op[sel]->OpNum(), KindTag(), KeyTag());
    match[sel].Print(2);
    jprintf("\n");
  }

  // promote halo facts and rebind variables then record what is being tried
  core->MainMemOnly(match[sel], 0);
  op0[nri] = op[sel];
  m0[nri].Copy(match[sel]);
  nri++;

  // instantiate operator, possibly use method preference as base bid
  delete meth;
  meth = NULL;                                   // save only most recent
  if (kind == JDIR_DO)                           // copy main verb modifiers
    get_context(&ctx2, key.MainAct(), match[sel]);            
  meth = core->CopyMethod(op[sel], match[sel], &ctx2);     
  if (root > 0) 
    core->BidPref(op[sel]->Pref());

  // show expansion of operator chosen (possibly a persistent intention)
  if ((noisy >= 1) && (meth != NULL))
  {
    jprintf("%*sApplying OP %d to %s[ %s ]", lvl, "", op[sel]->OpNum(), KindTag(), KeyTag());
    if (cnt > 1)
      jprintf(" (%d choices)", cnt);
    jprintf(" ...\n");
    if (kind == JDIR_NOTE) 
    {
      jprintf("\n++++++++++++++++++++++++++++++++++++\n");
      jprintf(">>> Set intention (%3.1f secs)\n\n", core->Stretch(op[sel]->Budget()));
      meth->Print(lvl + 2, 0);
      jprintf("++++++++++++++++++++++++++++++++++++\n\n");
    }
    else
    {
      jprintf("\n");
      meth->Print(lvl + 2, 0);
      jprintf("\n");
    }
  }

  // possibly simplify FIND/BIND requirements to allow OPs to match
  if ((kind == JDIR_FIND) || (kind == JDIR_BIND) ||
      (kind == JDIR_EACH) || (kind == JDIR_ANY) ||         // ACH also?
      (kind == JDIR_CHK)  || (kind == JDIR_ESC))
    subset = reduce_cond(op[sel], match[sel]);
  meth->avoid = op[sel];                         // suppress direct recursion
  return 1;
}


//= Find all matching operators from procedural memory.
// binds "sel" to last valid operator found (useful if only one)
// returns number of valid matches
// NOTE: non-return inhibition should really be about methods not operators

int jhcAliaDir::match_ops (int& sel)
{
  jhcAliaCore *core = step->Core();
  int i, j, cnt = 0;

  // look for a complete match
  sel = omax - 1;
  if (core->GetChoices(this) > 0)
    for (i = omax - 1; i >= mc; i--)
    {
      // reject operator if it was used to create current method
      if (op[i] == step->avoid)
      {
        jprintf(2, noisy, "    OP %d - reject as direct recursion\n", op[i]->OpNum());
        op[i] = NULL;
        continue;
      }

      // reject operators that have been tried before with equivalent bindings
      for (j = 0; j < nri; j++)
        if ((op[i] == op0[j]) && (op[i]->SameEffect(match[i], m0[j])))
        {
          jprintf(2, noisy, "    OP %d - reject as already tried\n", op[i]->OpNum());
          op[i] = NULL;
          break;
        }

      // if not rejected (all prior OPs are different), remember and bump count
      if (j >= nri)
      {
        sel = i;
        cnt++;
      }
    }
  return cnt;
}


//= Winnow suggested operators to keep only most specific.
// if most specific fails on early round, less specific will be chosen later
// returns number left, sel is bound to last valid choice

int jhcAliaDir::max_spec (int& sel)
{
  int i, np, cnt = 0, top = 0;

  // find highest degree of specificity
  for (i = omax - 1; i >= mc; i--)
    if (op[i] != NULL) 
    {
      np = match[i].NumPairs();
      top = __max(np, top);
    }

  // disqualify operators that are less specific
  for (i = omax - 1; i >= mc; i--)
    if (op[i] == NULL) 
      continue;
    else if (match[i].NumPairs() < top)
      op[i] = NULL;
    else
    {
      sel = i;
      cnt++;
    }
  return cnt;
}


//= Randomly select between remaining operators using exponential weighting.
// assumes max specificity (max_spec) has been used so ignores that part
// uses wt = e^1.73*(pref/wild) = 5.64^(pref/wild) with wild = 0.25 to 1.0
// <pre>
// diff = pref1 - pref2, result = prob(op1) / prob(op2)
//
//    wild     0.70     0.20    
//   ------  -------  --------  
//    1.00     1.5x     1.1x       explore (all ops about equal)
//    0.50    11.3x     2.0x       normal
//    0.25   128.0x     4.0x       safety  (usually highest pref)
//
// </pre>
// returns index of match selected, negative for problem

int jhcAliaDir::wtd_rand (double wild) const
{
  double wt[omax];
  double spread = 1.73;                                    // move to jhcAliaCore?
  double v, pick, f = spread / wild, sum = 0.0;
  int i, pct;

  // initialize weights for all valid operators
  for (i = omax - 1; i >= mc; i--)
    if (op[i] != NULL) 
    {
      v = exp(f * op[i]->Pref());
      wt[i] = v;
      sum += v;
    }

  // possibly report selection percentages
  if (noisy >= 2)
    for (i = omax - 1; i >= mc; i--)
      if (op[i] != NULL)
      {
        pct = ROUND(100.0 * wt[i] / sum);
        pct = __min(pct, 99);
        jprintf("    OP %d: sp %d x %4.2f -> %2d%%\n", op[i]->OpNum(), match[i].NumPairs(), op[i]->Pref(), pct);
      }

  // randomly choose an operator based on weights
  pick = sum * jrand();
  for (i = omax - 1; i >= mc; i--)
    if (op[i] != NULL)
    {
      pick -= wt[i];
      if (pick <= 0.0)
        return i;
    }
  return -1;
}


//= Add the focal node and most of its unbound arguments and properties to context.

void jhcAliaDir::get_context (jhcGraphlet *ctx2, jhcNetNode *focus, const jhcBindings& b) const
{
  jhcNetNode *p, *d;
  int i, j, cnt, cnt2;

  // always add the given item as "main" item and retain previous adverbs
  if (ctx2 == NULL)
    return;
  ctx2->Copy(ctx);
  ctx2->ReplaceAct(focus);
/*
  // add all relevant associated arguments (skip if part of matching)
  cnt = focus->NumArgs();
  for (i = 0; i < cnt; i++)
  {
    a = focus->Arg(i);
    if (key.InDesc(a) && !b.InSubs(a))
      ctx2->AddItem(a);
  }
*/
  // add all relevant associated properties 
  cnt = focus->NumProps();
  for (i = 0; i < cnt; i++)
    if (focus->RoleIn(i, "mod", "dir"))          // adverbs (amt? loc?)
    { 
      // base modifier ("slowly"), skip if part of matching
      p = focus->Prop(i);
      if (key.InDesc(p) && !b.InSubs(p))
      {
        ctx2->AddItem(p);
        cnt2 = p->NumProps();
        for (j = 0; j < cnt2; j++)
          if (p->RoleIn(j, "deg"))               // degree ("very")        
          {
            d = p->Prop(j);
            if (key.InDesc(d) && !b.InSubs(d))
              ctx2->AddItem(d);
          }
      }
    }
}


///////////////////////////////////////////////////////////////////////////
//                              CHK Control                              //
///////////////////////////////////////////////////////////////////////////

//= Use description in FIND/CHK directives to set up matching criteria.
// this is only to test candidate, not for matching operators

void jhcAliaDir::init_cond (int ver)
{
  const jhcAliaCore *core = step->Core();
  const jhcNodePool *wmem = &(core->atree);  
  jhcNetNode *n;
  int i, ni = key.NumItems();

  // removes already instantiated variables so they must match fixed values
  // must retain lex nodes since some have belief > 0 for aliases
  for (i = 0; i < ni; i++)
  {
    n = key.Item(i);
    if ((n->Belief() == 0.0) || !wmem->InList(n))
      cond.AddItem(n);
  }

  // save copy for incremental FIND
  full.Copy(cond);
  subset = 0;
  cand0 = cand;              // FIND retry wants "new" guess

  // new-ness required for CHK or ESC in first phase
  chk0 = 0;
  if ((kind == JDIR_CHK) || (kind == JDIR_ESC)) 
    chk0 = ver + 1;
}


//= Reduce FIND/BIND/CHK requirements to just those in selected operator.
// triggers the switchover from FIND/BIND to CHK mode when satisfied, or early CHK termination
// returns positive if new cond is different from original (minimal generation for FIND/BIND)

int jhcAliaDir::reduce_cond (const jhcAliaOp *op, const jhcBindings& match) 
{
  const jhcAliaCore *core = step->Core();
  const jhcActionTree *wmem = &(core->atree);
  const jhcNetNode *item, *a;
  int i, j, na, ni = full.NumItems();

  // set termination condition to match what OP promises
  cond.CopyBind(*(op->Pattern()), match);

  // preserve FIND guesses (originally external arguments)
  for (i = 0; i < ni; i++)
  {
    item = full.Item(i);
    na = item->NumArgs();
    for (j = 0; j < na; j++)
    {
      a = item->Arg(j);
      if (!full.InDesc(a))
        cond.RemItem(a);
    }
  }

  // if anything missing then mark this as a subset
  for (i = 0; i < ni; i++)
  {
    item = full.Item(i);
    if (!cond.InDesc(item))
      return(wmem->Version() + 1);     // facts should be new
  }
  return 0;
}


//= See if CHK description fully matched or known to be unmatchable.
// always checks to see if original CHK is completely matched or violated
// then checks to see if reduced criteria (if any) are clearly violated
// returns 2 if known false, 1 if known true, 0 if still working.

int jhcAliaDir::seek_match ()
{
  int res;

  if ((res = pat_confirm(full, 1)) != 0)
    return res;
  if (subset > 0)
    if ((res = pat_confirm(cond, 1)) != 0)
      subset = 0;            // stop long methods like "look around"
  return 0;
}


//= See if CHK pattern is confirmed (all true) or refuted (some false).
// chk > 0 ignores negations, chk < 0 matches negation for ACH
// returns 0 if unknown, 1 for full match, 2 for blocked match

int jhcAliaDir::pat_confirm (const jhcGraphlet& desc, int chk)
{
  jhcBindings match;
  jhcAliaCore *core = step->Core();
  jhcWorkMem *wmem = &(core->atree);
  const jhcNetNode *focus, *mate;
  int i, ni = desc.NumItems(), mc = 1;

  // possibly announce entry
  jprintf(2, dbg, "pat_confirm %s~~~~~~~~~~~~~~~~~~~~~~~~~~\n", ((subset > 0) ? "- subset " : ""));

  // set up matcher parameters
  bth = (core->atree).MinBlf();                  // must be non-hypothetical
  wmem->MaxBand(3);                              // use halo inferences also
  chkmode = chk;                                 // possibly ignore "neg" conflicts
  match.expect = ni;

  // find full match to key possibly regardless of "neg" flags
  // assumes either only one match will be found or all are equivalent
  if (MatchGraph(&match, mc, desc, *wmem) <= 0)
    return 0;
  core->MainMemOnly(match, 0);                   // bring in halo facts

  // determine if bindings confirm or refute description 
  if (chk > 0)
    for (i = 0; i < ni; i++)
    {
      focus = desc.Item(i);
      if ((mate = match.LookUp(focus)) != NULL)
        if (focus->Neg() != mate->Neg())
          return 2;                              // "alternate" continuation
    }
  return 1;                                      // normal continuation
}


//= Called by MatchGraph for CHK when a full set of bindings is found.
// sees if match confirms or refutes CHK criteria based on "neg" flags
// saves first valid set of bindings in match[0] (prefers main over halo)

int jhcAliaDir::complete_chk (jhcBindings *m, int& mc)
{
  const jhcBindings *b = &(m[mc - 1]);
  const jhcNetNode *n;
  int i, nb = b->NumPairs(), gval = 0;

  // possibly reject match for CHK if not new enough
  if (chk0 > 0)
  {
    for (i = 0; i < nb; i++)
      if ((n = b->GetSub(i)) != NULL)
        gval = __max(gval, n->Generation());   
    if (gval < chk0)
      return jprintf(2, dbg, "MATCH - but reject as not new enough\n");
  }

  // keep current match and start filling next
  if (dbg >= 2)
    b->Print("MATCH");
  if (mc > 0)
    mc--;                    // mc = 0 always stops matcher
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              FIND Control                             //
///////////////////////////////////////////////////////////////////////////

//= Check for simple FIND/BIND with lex of "me" or "you".
// returns 1 if mate is bound, 0 for irrelevant, -1 for second choice

int jhcAliaDir::me_you ()
{
  jhcBindings *scope = step->Scope();
  jhcAliaCore *core = step->Core();
  jhcWorkMem *wmem = &(core->atree);
  jhcNetNode *mate, *focus = cond.Main();

  if (!UserSelf())
    return 0;
  if (cand > 0)
  {
    jprintf(1, dbg, "  << Only one reasonable candidate for: %s\n", focus->Nick());
    return -1;
  }
  mate = ((focus->LexMatch("me")) ? wmem->Robot() : wmem->Human());
  guess[cand++] = mate;
  jprintf(1, noisy, "\n  << UNIQUE: %s = %s\n\n", focus->Nick(), mate->Nick());
  scope->Bind(focus, mate);
  return 1;
}


//= See if FIND criteria fully satisfied or possibly shift to CHK mode if reduced criteria.
// returns 1 if full match found, 0 if incomplete (partial or CHK), -2 if failed

int jhcAliaDir::seek_instance ()
{
  jhcBindings *scope = step->Scope();
  jhcNetNode *mate, *focus = cond.Main();
  int cmax = (((kind == JDIR_FIND) || (kind == JDIR_BIND)) ? gmax : emax);

  // if starting FIND retry, see if time to try to imagine some instance
  if (focus == NULL)
    return 1;
  jprintf(1, dbg, "seek_instance: %s %s\n", focus->Nick(), ((subset > 0) ? "(reduced)" : ""));
  if (cand0 >= cmax)
  {
    jprintf(1, dbg, "  << Already max candidates for: %s\n", focus->Nick());
    return assume_found();
  }

  // always check to see if original FIND is completely matched (opportunistic)
  if ((mate = sat_criteria(full, cand0, 0, 0)) == NULL)
    if ((recent >= 0) && (pron != NULL))                       // for naked FIND[ x ]
      mate = pron;
  if (mate != NULL)
  {
    if ((cand <= 0) || (guess[cand - 1] != mate))
    {
      mate->XferConvo((cand > 0) ? guess[cand - 1] : focus);   // conversation target
      guess[cand++] = mate;                                    // overwrite any partial match
    }
    if (noisy >= 1)
    {
      jprintf("\n  << %s", (((kind == JDIR_EACH) || (kind == JDIR_ANY)) ? "LOOP" : "GUESS"));
      jprintf(" %d: %s = %s", cand, focus->Nick(), mate->Nick());
      if (mate->Moored())
        jprintf(" (%s)", (mate->Deep())->Nick());
      jprintf("\n\n");
    }
    scope->Bind(focus, mate);
    pron_gender(mate, 1);                                     // possibly note sex
    subset = 0;
    return 1;
  }
  if (subset <= 0)
  {
    jprintf(1, dbg, "  << No obvious full match candidates for: %s\n", focus->Nick());
    return 0;
  }

  // if reduced FIND criteria are completely matched possibly try this as a guess
  if ((cand >= cmax) && (chk_state <= 0))
  {
    jprintf(1, dbg, "  << Too many candidates for: %s\n", focus->Nick());
    return assume_found();
  }
  mate = sat_criteria(cond, cand, subset, 0);
  if (mate == NULL)
  {
    jprintf(1, dbg, "  << No obvious partial match candidates for: %s\n", focus->Nick());
    return 0;
  }
  mate->XferConvo((cand > 0) ? guess[cand - 1] : focus);       // conversation target
  guess[cand++] = mate;
  jprintf(1, noisy, "\n  << CONSIDER: %s = %s\n\n", focus->Nick(), mate->Nick());
    
  // shift to CHK mode for remaining criteria
  halt_subgoal();                                // cleanly stop any on-going op
  if ((meth = chk_method()) == NULL)
    return 0;
  if (nri > 0)
    nri--;                                       // okay to re-use current op if CHK fails

  // start doing constructed CHK method 
  if (noisy >= 1)
  {
    jprintf("  Partial %s[ %s ] residual ...\n\n", KindTag(), KeyTag());
    meth->Print(4);
    jprintf("\n");
  }
  chk_state = 1;                                 // start meth at next Status() call
  return 0;
}


//= Check for a full match for whatever FIND criteria are listed in the description.
// skip = how many old guesses to exclude (e.g. already thoroughly tried)
// after = 1 if only accept facts after version recorded in subset variable
// ltm = 1 searchs declarative memory instead of working memory
// takes first match governed by node ordering in list (cf. jhcNodePool::Refresh)
// returns matching node for search variable if found

jhcNetNode *jhcAliaDir::sat_criteria (const jhcGraphlet& desc, int skip, int after, int ltm) 
{
  jhcGraphlet desc2;
  jhcBindings match;
  jhcAliaCore *core = step->Core();
  jhcActionTree *atree = &(core->atree);
  jhcNetNode *item, *arg, *mate;
  int i, j, na, ni = desc.NumItems(), mc = 1;

  // possibly announce entry
  jprintf(2, dbg, "sat_criteria %s~~~~~~~~~~~~~~~~~~~~~~~~~~\n", ((subset > 0) ? "- subset " : ""));

  // add each external argument to new graphlet and establish a self-binding
  desc2.Copy(desc);
  for (i = 0; i < ni; i++)
  {
    item = desc.Item(i);
    na = item->NumArgs();
    for (j = 0; j < na; j++)
    {
      arg = item->Arg(j);
      if (!desc.InDesc(arg))
      {
        desc2.AddItem(arg);
        match.Bind(arg, arg);
      }
    }
  }

  // record problem description
  match.expect = desc2.NumItems();               // total variables in pattern
  focus = desc.Main();                           // free variable sought
  find0 = after;                                 // only accept new facts
  exc = skip;                                    // exclude previous guesses

  // set up matcher parameters and status
  bth = atree->MinBlf();                         // must be non-hypothetical
  atree->MaxBand(3);                             // use halo inferences also
  chkmode = 0;                                   // "neg" is definitely important!
  recent = -1;                                   // no pronoun match yet

  // extract new guess from full match (complete_find rejects some)
  if (ltm <= 0)
  {
    if (MatchGraph(&match, mc, desc2, *atree) <= 0)
      return NULL;
    if (recent >= 0)
      return pron;                               // should already be in wmem
    core->MainMemOnly(match, 0);                 // bring any halo facts into wmem
    return match.LookUp(focus);
  }

  // if LTM match found then promote portions into wmem
  if (MatchGraph(&match, mc, desc2, core->dmem) <= 0)
    return NULL;
  item = match.LookUp(focus);
  (core->dmem).Refresh(item);                    // ensure first next time
  mate = lift_key();                             // copy entire key structure
  mate->MoorTo(item);
  jprintf(":- RECALL creates %s (%s) for memory %s\n", mate->Nick(), mate->LexStr(), item->Nick());
  atree->NoteSolo(mate);
  return mate;
}


//= Called by MatchGraph for FIND/BIND when a full set of bindings is found.
// saves first valid set of bindings in match[0] (prefers main over halo)

int jhcAliaDir::complete_find (jhcBindings *m, int& mc)
{
  const jhcBindings *b = &(m[mc - 1]);
  jhcBindings *scope = step->Scope();
  const jhcNetNode *n;
  jhcNetNode *mate = b->LookUp(focus);
  int i, nb = b->NumPairs(), gval = 0;

  // reject if this guess was tried before by some FIND (must be non-hypothetical)
  if ((mate == focus) || scope->InSubs(mate) || !mate->Visible() || !mate->Sure(bth))
    return 0;
  for (i = 0; i < exc; i++)
    if (mate == guess[i])
      return jprintf(2, dbg, "MATCH - but reject %s as already tried\n", mate->Nick());

  // handle special case of naked FIND/BIND with just a target variable: FIND[ x ]
  if ((cond.NumItems() == 1) && (focus->Lex() == NULL) && focus->ObjNode())    
  {
    if (!mate->String() && (mate->LastConvo() > 0))
      filter_pron(mate);
    return 0;                // keeps altering match bindings (does not stop)
  }

  // possibly reject match for partial FIND/BIND if not new enough
  if (find0 > 0)
  {
    for (i = 0; i < nb; i++)
      if ((n = b->GetSub(i)) != NULL)
        gval = __max(gval, n->Generation());   
    if (gval < find0)
      return jprintf(2, dbg, "MATCH - but reject %s as not new enough\n", mate->Nick());
  }

  // accept and record match
  if (dbg >= 2)
    b->Print("MATCH");
  if (mc > 0)
    mc--;                    // takes first match: mc = 0 stops matcher
  return 1;                
}


//= Enforce any restrictions on naked node choice encoded by grammatical tags.
// saves best mate for focus in "pron" member variable and sets "recent" > 0
// returns 0 if rejected, 1 if best so far

int jhcAliaDir::filter_pron (jhcNetNode *mate)
{
  UL32 tags = focus->tags;
  int when = mate->LastConvo();

  // generally looking for a physical thing not a fact or idea
  if (!mate->ObjNode())                
  {
    if (tags != 0)                     // "that" has args
      return jprintf(2, dbg, "MATCH - but reject %s as predicate node\n", mate->Nick());
  }
  else if ((tags & JTAG_FEM) != 0)     // "she"
  {
    if ((mate->FindProp("hq", "male", 0, bth) != NULL) ||
        (mate->FindProp("hq", "female", 1, bth) != NULL))
      return jprintf(2, dbg, "MATCH - but reject %s as not female\n", mate->Nick());
  }
  else if ((tags & JTAG_MASC) != 0)    // "he"
  {
    if ((mate->FindProp("hq", "female", 0, bth) != NULL) ||
        (mate->FindProp("hq", "male", 1, bth) != NULL))
      return jprintf(2, dbg, "MATCH - but reject %s as not male\n", mate->Nick());
  }
  else if ((tags & JTAG_ITEM) != 0)    // "it"
  {
    if ((mate->FindProp("hq", "male", 0, bth) != NULL) ||
        (mate->FindProp("hq", "female", 0, bth) != NULL))
      return jprintf(2, dbg, "MATCH - but reject %s as gendered\n", mate->Nick());
  }
  else if (tags == 0)                  // "them" has no args
    return jprintf(2, dbg, "MATCH - but reject %s as object node\n", mate->Nick());

  // if most recent then save as current best candidate
  jprintf(2, dbg, "MATCH - %s %s\n", mate->Nick(), ((when > recent) ? "<== BEST" : "ignore")); 
  if (when <= recent)
    return 0;
  recent = when;
  pron = mate;           
  return 1;                
}


//= If naked FIND with gendered pronoun ("he" or "she") then add appropriate fact.
// uses member variables "focus", "cond", and "step"

void jhcAliaDir::pron_gender (jhcNetNode *mate, int note)
{
  char sex[40] = "male";
  jhcAliaCore *core = step->Core();
  jhcActionTree *atree = &(core->atree);
  jhcNetNode *fact;
  int tags = focus->tags;

  // make sure naked FIND[ x ] was sought
  if ((cond.NumItems() != 1) || (focus->Lex() != NULL) || !focus->ObjNode())
    return;

  // figure out which gender object should be
  if ((tags & JTAG_FEM) != 0)
    strcpy_s(sex, "female");
  else if ((tags & JTAG_MASC) == 0)
    return;

  // add feature if not already present
  if (mate->FindProp("hq", sex, 0, atree->MinBlf()) != NULL)
    return;
  fact = atree->AddProp(mate, "hq", sex);
  if (note > 0)
    atree->NoteSolo(fact);
}


//= Create method for FIND directive consising of CHK of parts not found.
// returns a suitable CHK chain (delete elsewhere), NULL if not needed

jhcAliaChain *jhcAliaDir::chk_method ()
{
  jhcAliaDir ref(JDIR_CHK);
  jhcBindings sc2;
  jhcAliaCore *core = step->Core();
  jhcNodePool *wmem = &(core->atree);
  const jhcBindings *scope = step->Scope();
  jhcNetNode *item, *a;
  jhcAliaChain *ch;
  jhcAliaDir *d2;
  int i, j, nc, na, ni = key.NumItems(), nb = scope->NumPairs();

  // not working on a set of reduced criteria
  if ((subset <= 0) || (cand <= 0))
    return NULL;
  subset = wmem->Version() + 1;        // if only new things desired

  // candidate from guaranteed to satisfy part of CHK
  for (i = 0; i < ni; i++)
  {
    item = key.Item(i);
    if (!cond.InDesc(item))
      (ref.key).AddItem(item);
  }
  cond.Copy(full);                     // restore full criteria

  // retain all previous substitutions (including recent guess)
  sc2.Bind(cond.Main(), guess[cand - 1]);
  for (i = 0; i < nb; i++)
  {
    item = scope->GetSub(i);
    sc2.Bind(item, item);
  }

  // retain all external arguments
  nc = cond.NumItems();
  for (i = 0; i < nc; i++)  
  { 
    item = cond.Item(i);
    na = item->NumArgs();
    for (j = 0; j < na; j++)
    {
      a = item->Arg(j);
      if (!cond.InDesc(a))
        sc2.Bind(a, a);
    }
  }

  // return chain consisting of single CHK directive              
  ch = new jhcAliaChain;
  d2 = new jhcAliaDir;
  ch->BindDir(d2);
  d2->CopyBind(*wmem, ref, sc2);
  return ch;
}


//= Trying matching FIND/BIND specification to long term memory or making new node.
// returns 1 if new guess, -2 if out of options

int jhcAliaDir::recall_assume ()
{
  jhcBindings *scope = step->Scope();
  jhcNetNode *mate, *focus = cond.Main();
  int cmax = (((kind == JDIR_FIND) || (kind == JDIR_BIND)) ? gmax : emax);

  // possibly announce entry then check if matching should be performed
  jprintf(1, dbg, "recall_assume: %s\n", focus->Nick());
  if (cand0 >= cmax)
  {
    jprintf(1, dbg, "  << Already max guesses for: %s\n", focus->Nick());
    return assume_found();
  }

  // determine if original FIND is completely matched against LTM
  subset = 0;
  if ((mate = sat_criteria(full, cand0, 0, 1)) == NULL)
  {
    jprintf(1, dbg, "  << No obvious LTM match candidates for: %s\n", focus->Nick());
    return assume_found();
  }

  // accept first match found as a new guess
  if ((cand <= 0) || (guess[cand - 1] != mate))
  {
    mate->XferConvo((cand > 0) ? guess[cand - 1] : focus);     // conversation target
    guess[cand++] = mate;                                      // overwrite any partial match
  }
  if (noisy >= 1)
  {
    jprintf("\n  << %s", (((kind == JDIR_EACH) || (kind == JDIR_ANY)) ? "LOOP" : "GUESS"));
    jprintf(" %d: %s = %s (%s)\n\n", cand, focus->Nick(), mate->Nick(), (mate->Deep())->Nick());
  }
  scope->Bind(focus, mate);
  return 1;
}


//= Makes up item that fits FIND properties and records proper binding.
// BIND guaranteed to generate at least one candidate (even if made up)
// returns 1 if newly asserted, -2 if already tried (fail)

int jhcAliaDir::assume_found ()
{
  jhcAliaCore *core = step->Core();
  jhcActionTree *wmem = &(core->atree);
  jhcBindings *scope = step->Scope();
  jhcNetNode *mate, *focus = full.Main();
  int sub0 = subset;

  // restore full criteria (if needed)
  if (subset > 0)
  {
    cond.Copy(full);
    subset = 0;
  }

  // EACH and ANY return "alt" when out of choices (if EACH had none then "fail")
  if (kind == JDIR_EACH) 
    return((cand > 0) ? 2 : -2);
  if (kind == JDIR_ANY)
    return 2;

  // consider re-selecting earlier variables (some dependencies not explicit)
  // no new or recyled item if already made new one, backup possible, or FIND completely struck out
  if (!hyp.Empty() || step->Fallback() || (kind == JDIR_FIND))
  {
    if (cand > 0)
      focus->XferConvo(guess[cand - 1]);     // revoke guess as conversation target
    cand = 0;                                // forget guesses from this round
    reset();                                 // possibly pick same ops next time
    return -2;                               // will backtrack if fallback exists
  }

  // when BIND runs out of guesses but had several, resubmit original (best) full guess
  if ((cand > 0) && (sub0 <= 0))
  {
    mate = guess[0];                         // revert to initial most probable guess
    mate->XferConvo(guess[cand - 1]);        // revoke current guess as conversation target
    scope->RemFinal(focus);
    scope->Bind(focus, mate);                
    hyp.AddItem(wmem->MakeNode("dummy"));    // prevent future backtracking to this BIND
    jprintf(1, noisy, "\n  << REPEAT: %s = %s (initial choice)\n\n", focus->Nick(), mate->Nick());
    return 1;
  }

  // just imagine whole network specified by key
  mate = lift_key();
  jprintf(1, noisy, "\n  << ASSUME: %s = %s (new)\n\n", focus->Nick(), mate->Nick());
  pron_gender(mate, 1);
  return 1;
}


//= Make up a new piece of network in working memory that exactly matches key. 
// returns the newly made node that matches the main node of the key

jhcNetNode *jhcAliaDir::lift_key ()
{
  jhcBindings mt;
  jhcAliaCore *core = step->Core();
  jhcActionTree *wmem = &(core->atree);
  jhcBindings *scope = step->Scope();
  jhcNetNode *mate, *focus = full.Main();
  jhcGraphlet *old;
  int i, n = scope->NumPairs();

  // imagine a suitable object in WMEM (similar to NOTE) but only once (!hyp.Empty())
  // only does this for a BIND (not FIND) that has been unable to make any guesses (cand = 0)
  for (i = 0; i < n; i++)
  {
    mate = scope->GetSub(i);
    mt.Bind(mate, mate);                     // retain all FIND/BIND substitutions
  }
  old = wmem->BuildIn(hyp);                  // needed for garbage collection
  wmem->Assert(key, mt, -1.0, 0, NULL);      // makes new nodes as needed
  wmem->BuildIn(old);
  wmem->RevealAll(hyp);                      // make available for matching

  // remember hypothesized item as current guess (no FIND retry so no NRI)
  mate = mt.LookUp(focus);
  mate->XferConvo((cand > 0) ? guess[cand - 1] : focus);   // conversation target
  scope->Bind(focus, mate);                                
  return mate;
}


///////////////////////////////////////////////////////////////////////////
//                           Execution Tracing                           //
///////////////////////////////////////////////////////////////////////////

//= Look for all in-progress activities matching graph and possibly stop them.
// needs "skip" since originally negated DO[ desc ] has been made positive
// returns 1 if any matching found, 0 if nothing relevant

int jhcAliaDir::HaltActive (const jhcGraphlet& desc, const jhcAliaDir *skip, int halt)
{
  jhcAliaDir d2;
  jhcAliaCore *core = step->Core();
  jhcAliaOp *op;
  double bth = (core->atree).MinBlf();
  int any = 0;

  // make sure this is a command that is still running
  if ((kind != JDIR_DO) && (kind != JDIR_ANTE) && (kind != JDIR_GATE))
    return 0;
  if ((this == skip) || (verdict != 0) || (key.ActNeg() > 0))
    return 0;

  // pretend description is an operator being matched to a copy of this key
  op = core->Probe();
  op->InitCond(desc);
  d2.Copy(*this);

  // see if any matches found, possibly using halo facts to fill in
  (core->atree).MaxBand(3);
  d2.mc = omax;
  if (op->FindMatches(d2, core->atree, -bth, 1) > 0)
  {
    // possibly force failure of directive
    if (halt > 0)
      Stop();
    any = 1;                           
  }

  // see if description matches current method instead
  if (meth != NULL)
    if (meth->HaltActive(desc, skip, halt) > 0)
      any = 1;
  return any;
}


//= Find the reason for the most recently started activity compatible with the description.
// cyc is unique request number for tracing graphs with loops, prev is most recent caller
// binds best match directive and its caller, done: 0 = ongoing only, 1 = any non-failure

void jhcAliaDir::FindCall (const jhcAliaDir **act, const jhcAliaDir **src, jhcBindings *d2a, 
                           const jhcGraphlet& desc, UL32& start, int done, const jhcAliaDir *prev, int cyc)
{
  jhcAliaDir twin;
  jhcAliaCore *core;
  jhcAliaOp *op;
  double bth;

  // sanity check                      // never tried has step = NULL
  if (step == NULL)
    return;
  core = step->Core();                                     
  bth = (core->atree).MinBlf();                            

  // if not a plausible candidate, see if last employed method has something
  if ((kind != JDIR_DO) || (t0 < start) || (verdict <= -2) || ((done <= 0) && (verdict != 0)))  
  {
    if (meth != NULL)
      meth->FindCall(act, src, d2a, desc, start, done, this, cyc);
    return;
  }
  
  // convert description to an operator then match against copy of this 
  op = core->Probe();
  op->InitCond(desc);
  twin.Copy(*this);                    // has disposable match history

  // see if any matches found, possibly using halo facts to fill in
  (core->atree).MaxBand(3);
  twin.mc = omax;
  if (op->FindMatches(twin, core->atree, -bth, 1) > 0)
  {
    if (act != NULL)
      *act = this;
    if (src != NULL)
      *src = prev;
    if (d2a != NULL)
      d2a->Copy(twin.match[omax - 1]);
    start = t0;                       
  }

  // examine most recent method tried
  if (meth != NULL)
    meth->FindCall(act, src, d2a, desc, start, done, this, cyc);
}


///////////////////////////////////////////////////////////////////////////
//                             File Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Read at current location in a file to fill in details of self.
// returns: 2 = ok + all done, 1 = successful, 0 = syntax error, -1 = end of file, -2 = file error 

int jhcAliaDir::Load (jhcNodePool& pool, jhcTxtLine& in)
{
  int k, ans = 0;

  // re-use previously peeked line or get new line from file
  if (in.Next() == NULL)
    return 0;

  // determine type (e.g. "CHK[")
  for (k = 0; k < JDIR_MAX; k++)
    if (in.Begins(ktag[k]))
      break;
  if (k >= JDIR_MAX) 
    return 0;
  kind = (JDIR_KIND) k;

  // encode condition description starting on same line
  in.Skip(ktag[kind], 1);
  in.Clean();
  if (kind == JDIR_PUNT)                         // no internal description
    ans = ((in.First("]")) ? 2 : 1);     
  else if ((kind == JDIR_DO) && in.First("]"))   // no-op = empty DO[ ]
    ans = 2;
  else
    ans = pool.LoadGraph(key, in);
  in.Flush();
  return((ans >= 2) ? 1 : __min(0, ans));
}


//= Save self out in machine readable form to current position in a file.
// detail: 0 no extras, 1 show belief, 2 show tags, 3 show both
// return: 1 = successful, 0 = bad format, -1 = file error

int jhcAliaDir::Save (FILE *out, int lvl, int detail) const
{
  if ((kind < 0) || (kind >= JDIR_MAX))
    return 0;
  jfprintf(out, "%*s%4s[ ", lvl, "", ktag[kind]);
  if (((kind == JDIR_FIND) || (kind == JDIR_BIND) || (kind == JDIR_EACH) || (kind == JDIR_ANY)) && 
      (key.NumItems() == 1))
    key.Save(out, -(lvl + 6), detail | 0x02);    // always show tags
  else
    key.Save(out, -(lvl + 6), detail);
  jfputs(" ]\n", out);
  return((ferror(out) != 0) ? -1 : 1);
}


//= Print directive with some annotation in front of it.

void jhcAliaDir::Print (const char *tag, int lvl, int detail)
{
  if ((tag == NULL) || (*tag =='\0'))
    jprintf("%*sdirective:\n", lvl, "");
  else
    jprintf("%*s%s:\n", lvl, "", tag);
  if (((kind == JDIR_FIND) || (kind == JDIR_BIND) || (kind == JDIR_EACH) || (kind == JDIR_ANY)) && 
      (key.NumItems() == 1))
    Save(stdout, lvl + 2, detail | 0x02);        // always show tags
  else
    Save(stdout, lvl + 2, detail);
}
