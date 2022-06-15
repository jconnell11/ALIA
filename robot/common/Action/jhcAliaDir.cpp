// jhcAliaDir.cpp : directive states what sort of thing to do in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#include "Reasoning/jhcAliaOp.h"       // common audio
#include "Reasoning/jhcAliaRule.h"

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"
#include "Interface/jrand.h"

#include "Action/jhcAliaCore.h"        // common robot - since only spec'd as class in header

#include "Action/jhcAliaDir.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Shared array of directive names.
// strings must remain consistent with JDIR_KIND enumeration

const char * const jhcAliaDir::ktag[JDIR_MAX] = {"NOTE", "DO",   "ANTE", "PUNT", "FCN",  "CHK", 
                                                 "ACH",  "KEEP", "BIND", "FIND", "NONE", "ADD", "TRY"};


//= Default destructor does necessary cleanup.
// jhcAliaChain takes care of deleting rest of graph
// nodes in key are reclaimed by garbage collector

jhcAliaDir::~jhcAliaDir ()
{
  jhcAliaCore *core;

  halt_subgoal();
  delete meth;               // primarily for TRY
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

  // processing state
  meth = NULL;
  reset();
  verdict = -1;
  noisy = 0;             // sequence progress statements (defaulted from core)

  // FIND state
  cand = 0;              // guess number always increases (never reset)
  subset = 0;            // set if sub-method needs CHK follow-up
  gtst = 0;
  cand0 = 0;

  // NOTE perseverance
  t0 = 0;
  t1 = 0;

//dbg = 1;               // local FIND debug statements (2 or 3 for matcher)
}


//= Clear out all previous method attempts and bindings.
// does not remove directive definition (kind and key) 
// typically used for switching between ANTE -> DO 
// returns 0 always for convenience

int jhcAliaDir::reset ()
{
  // nothing selected or running (no stop needed)
  inst = delete_meth();

  // current state (running)
  nri = 0;
  verdict = 0;
  wait = -1;
  chk_state = 0;

  // no general node matches permitted
  own = 0;
  return 0;
}


//= Never delete pre-bound method for TRY directive.
// always returns -1 for convenience (e.g. inst)

int jhcAliaDir::delete_meth ()
{
  if (kind != JDIR_TRY)
  {
    delete meth;
    meth = NULL;
  }
  return -1;
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

int jhcAliaDir::MaxDepth () const
{
  if (meth == NULL)
    return 1;
  return(meth->MaxDepth() + 1);
}


//= Determine number of simultaneous activities (possibly subgoaled).
// if leaf > 0 then only considers currently active goals not pass-thrus

int jhcAliaDir::NumGoals (int leaf) const
{
  int n;

  if (meth == NULL)
    return 1;
  n = meth->NumGoals(leaf);
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
  pool.BuildIn(&key);

  // copy bulk of directive
  if ((ans = pool.Assert(ref.key, b)) >= 0)      // negative is failure
    if ((kind == JDIR_DO) && (ctx2 != NULL))
    {
      ctx.Copy(*ctx2);                           // for cascading
      share_context(ctx2);
    }

  // cleanup
  pool.BuildIn(NULL);
  return((ans < 0) ? 0 : 1);
}


//= Add in any adverbs or objects from higher-level call (for DO directives only).
// shares nodes already in wmem (e.g. mod-23 -mod-> act-11 and now mod-23 -mod-> act-62 also)

void jhcAliaDir::share_context (const jhcGraphlet *ctx2) 
{
  jhcNetNode *n, *old, *act = key.Main();
  int i, cnt;

  // sanity check
  if (ctx2 == NULL)
    return;
  if ((old = ctx2->Main()) == NULL)
    return;

  // add in all old arguments (just extra pointers)
  cnt = old->NumArgs();
  for (i = 0; i < cnt; i++)
  {
    n = old->Arg(i);
    if (ctx2->InDesc(n))
    {
      act->AddArg(old->Slot(i), n);
      key.AddItem(n);
    }
  }   
      
  // add in all old properties (just extra pointers)
  cnt = old->NumProps();
  for (i = 0; i < cnt; i++)
  {
    n = old->Prop(i);
    if (ctx2->InDesc(n))
    {
      n->AddArg(old->Role(i), act);
      key.AddItem(n);
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
    item = pool.MakeNode("act", "check");
    item->AddArg("obj", KeyMain());
    return src->AddArg(slot, item);
  }

  // make up find action (not for BIND from jhcNetRef)
  if (kind == JDIR_FIND)
  {
    item = pool.MakeNode("act", "find");
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
  int i, j, nb, ki = key.NumItems(), fi = hyp.NumItems();

  // mark all nodes in instantiated key (may not be in NRI yet)
  jprintf(5, noisy, "    directive key %s[ %s ]\n", KindTag(), KeyTag());
  for (i = 0; i < ki; i++)
  {
    n = key.Item(i);
    jprintf(5, noisy, "      %s\n", n->Nick());
    n->keep = 1;
  }

  // mark all undo choices for variable substitution in key
  if (nsub > 0)
    jprintf(5, noisy, "    variable substitutions\n");
  for (i = 0; i < nsub; i++)
  {
    n = ((val0[i] != NULL) ? val0[i] : fact[i]);
    jprintf(5, noisy, "      %s\n", n->Nick());
    n->keep = 1;
  }

  // mark all current and previous guesses
  if (cand > 0)
    jprintf(5, noisy, "    variable guesses\n");
  for (i = 0; i < cand; i++)
  {
    jprintf(5, noisy, "      %s\n", guess[i]->Nick());
    guess[i]->keep = 1;
  }    

  // mark all structure assumed by FIND
  if (fi > 0)
    jprintf(5, noisy, "    FIND assumption\n");
  for (i = 0; i < fi; i++)
  {
    n = hyp.Item(i);
    jprintf(5, noisy, "      %s\n", n->Nick());
    n->keep = 1;
  }

  // keep all NRI bindings so no dangling pointers (or reused elements)
  if (nri > 0)
    jprintf(5, noisy, "    non-return inhibition\n");
  for (i = 0; i < nri; i++)
  {
    nb = m0[i].NumPairs();
    for (j = 0; j < nb; j++)
    {
      n = m0[i].GetSub(j);
      jprintf(5, noisy, "      %s\n", n->Nick());
      n->keep = 1;
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
  double s;
  int lvl = st->Level(), ver = wmem->Version();

  // set up internal state, assume something needs to run
  step = st;
  subst_key(step->Scope());                      // substitute found variables
  noisy = core->noisy;
//noisy = 2;                                     // control structure debugging
  wmem->RevealAll(key);                          // make available for matching
  reset();                          
  (core->mood).Launch(); 
  jprintf(2, noisy, "%*sstart %s[ %s ]\n", step->Level(), "", KindTag(), KeyTag());

  // initialize various types of directive
  if (kind == JDIR_TRY)
  {
    if (noisy >= 1)
    {
      jprintf("    Expanding TRY[ %s ] ...\n", KeyTag());
      jprintf("_______________________________________\n");
      jprintf(">>> Bulk sequence\n\n");
      meth->Print(4, 0);
      jprintf("_______________________________________\n\n");
    }
    meth->Start(core, lvl + 1);
  }
  else if (kind == JDIR_DO) 
  {
    jprintf(2, noisy, "  Converting DO->ANTE[ %s ] - init\n\n", KeyTag());
    kind = JDIR_ANTE;                     
    key.Main()->SetDone(0);                       // restart action if backtrack
    key.Main()->SetNeg(0);
  }
  else if (kind == JDIR_FCN)
    inst = core->FcnStart(key.Main());            // negative inst records fail
  else if (kind == JDIR_NOTE)
  {
    key.ActualizeAll(ver);                        // assert pending truth values
    wmem->Refresh(key);
    if (wmem->Endorse(key) <= 0)                  // too coarse?
    {
      s = wmem->CompareHalo(key, core->mood);     // update rule result beliefs
      (core->mood).Believe(s);
    }
    own = core->Percolate(*this);
    t0 = jms_now();
  }
  else if ((kind == JDIR_FIND) || (kind == JDIR_BIND) || (kind == JDIR_CHK))
    init_cond();                                  // situation to search for

  // report current status
  return report(verdict);
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
  jhcAliaCore *core = step->Core();
  int res, lvl = step->Level();

  // test for special cases
  if (kind == JDIR_PUNT)                                         // discontinue chain
    return report(-3);             
  if (kind == JDIR_ADD)                                          // accept new rule/op
  {
    if ((res = core->Accept(new_rule, new_oper)) > 0)
    {
      new_rule = NULL;                                           // save from d'tor deletion
      new_oper = NULL;
    }
    else if ((new_rule != NULL) && (res == -3))
      err_rule();                                                // reason for ADD failure
    return report((res > 0) ? 1 : -2);
  }
  if (kind == JDIR_FCN)                                          // run some function
    return report(core->FcnStatus(key.Main(), inst));      
  if ((kind == JDIR_DO) &&                                       // stop matching action
       key.Main()->Neg() && (key.Main()->Done() <= 0))              
    return report(core->HaltActive(key));

  // check for pre-emptive full matches to FIND and CHK criteria (or NOTE is no longer valid)
  if (kind == JDIR_CHK)
    if ((res = seek_match()) != 0)
    {
      jprintf(2, noisy, "%*s### direct full CHK[ %s ] match = %d\n",  step->Level(), "", KeyTag(), res);
      halt_subgoal();
      return report(res);
    }
  if ((kind == JDIR_FIND) || (kind == JDIR_BIND))
    if ((res = seek_instance()) != 0)
    {
      jprintf(2, noisy, "%*s### direct full %s[ %s ] match = %d\n", step->Level(), "", KindTag(), KeyTag(), res);
      halt_subgoal();
      return report(res);
    }
  if (kind == JDIR_NOTE)
    if (key.Moot())
    {
      jprintf(2, noisy, "%*s### mooted %s[ %s ]\n", step->Level(), "", KindTag(), KeyTag());
      halt_subgoal();
      return report(1);
    }
  if ((t1 != 0) && (jms_elapsed(t1) > core->Retry()))
  {
    jprintf(1, noisy, "@@@ Retry intention for: NOTE[ %s ]\n", KeyTag());
    meth->Start(core, -(lvl + 1));
    t1 = 0;
    return report(0);
  }

  // find some operator to try if nothing selected yet
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
  jprintf(2, noisy, "%*sPassing through %s[ %s ]\n", step->Level(), "", KindTag(), KeyTag());
  if ((res = meth->Status()) == 0)
    return verdict;
  result[nri - 1] = res;                         // nri increment in pick_method
  chk_state = 0;
  if (res <= -3)
    return report(-3);

  // generally try next method, but special handling for DO and NOTE and FIND
  if (kind == JDIR_DO)
    return do_status(res, 1);
  if (kind == JDIR_TRY)                          // TRY method was pre-bound
    return report(res);                          
  if ((kind == JDIR_NOTE) && (step->cont == NULL) && (res == -2))
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
  return next_method();                          // ANTE, POST, FIND, CHK run all
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
    jprintf("~~~~~~~~ first selection ~~~~~~~~\n");
    Print();
    jprintf("\n");
  }

  // always try to find some applicable operator 
  if ((found = pick_method()) > 0) 
  {
    if ((kind == JDIR_FIND) || (kind == JDIR_BIND))        // keep generating answers
      meth->Enumerate();
    return report(meth->Start(core, lvl + 1));             // found -> run operator
  }
  if (found == 0)                                           
    return report(0);                                      // waiting

  // no more operators - special handling for various types
  if (kind == JDIR_ANTE)        
  {
    jprintf(2, noisy, "Converting ANTE->DO[ %s ] - no more ops\n\n", KeyTag());
    alter_pref();
    kind = JDIR_DO;                                        // none -> advance to DO phase 
    reset();                                               // clear nri for DO alter_pref
    (core->atree).MarkBelief(key.Main(), 1.0);
  }
  else if ((kind == JDIR_FIND) || (kind == JDIR_BIND))
    verdict = assume_found();                              // clears guesses
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

  // mark failure or success of DO
  if (res > 0)
    alter_pref(); 
//  else
//    key.Main()->SetNeg(1);
//  key.Main()->SetDone(1);
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
    if ((kind == JDIR_FIND) || (kind == JDIR_BIND))        // keep generating answers
      meth->Enumerate();
    return report(meth->Start(core, lvl + 1));             // found -> run operator
  }
  if (found == 0)
    return report(0);                                      // waiting

  // no more operators - special handling for various types
  if (kind == JDIR_ANTE)
  {
    jprintf(2, noisy, "Converting ANTE->DO[ %s ] - all tried\n\n", KeyTag());
    alter_pref();  
    kind = JDIR_DO;                                        // none -> advance to DO phase
    reset();                                               // clear nri for DO alter_pref
  }
  else if ((kind == JDIR_FIND) || (kind == JDIR_BIND))
    verdict = assume_found();                              // clears guesses
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
  jhcAliaCore *core = step->Core();
  int lvl = step->Level(), detail = 1;

  // possibly announce a transition
  if (noisy >= 1)
  {
    if (val == -2)
      jprintf("%*s--- failure: %4s[ %s ]\n",    lvl, "", KindTag(), KeyTag());
    else if (val < 0) 
      jprintf("%*s::: terminated: %4s[ %s ]\n", lvl, "", KindTag(), KeyTag());
    else if (val == 1)
      jprintf("%*s*** success: %4s[ %s ]\n",    lvl, "", KindTag(), KeyTag());
    else if (val >= 2)
      jprintf("%*s~~~ disproved: %4s[ %s ]\n",  lvl, "", KindTag(), KeyTag());
  }

  // clean up if no longer active (method already stopped and can be scrapped)
  if (val != 0)
  {
    // possibly update expected time for free choice reaction 
    if ((kind == JDIR_NOTE) && (nri > 0))
    {
      op0[0]->AdjTime(jms_elapsed(t0));
      jprintf(1, detail, "  ADJUST: operator %d --> duration %3.1f + %3.1f\n", op0[0]->OpNum(), op0[0]->Time(), op0[0]->Dev());
    }

    // possibly adjust operator preference and modulate mood
    if (val > 0)
    {
      alter_pref();                              // only when top level goal succeeds
      (core->mood).Win();
    }
    else if (val == -2)                          // ignore Stop (val == -1)
      (core->mood).Lose();

    // possibly block halo percolation then remove working memory anchor
    if (kind == JDIR_NOTE)
      own = core->ZeroTop(*this);     
    inst = delete_meth();                        // reset() would bash nri for FIND                     
  }

  // convert some successes into failures
  verdict = -2;
  if (((val == 2) && (step->alt == NULL)))
    jprintf(1, noisy, "%*s--- failure:    CHK[ %s ]  revised since no alt branch\n", lvl, "", KeyTag());
  else if ((val == 1) && step->Variations() && (kind != JDIR_FIND))             
    jprintf(1, noisy, "%*s--- failure: %4s[ %s ]  revised to generate variants\n", lvl, "", KindTag(), KeyTag());
  else
    verdict = val;                              // restore if reset() erases 
  return verdict;
}


//= Possibly increase or decrease current operator preference based on experience.
// preference value encodes both "select at all" and "select instead of"
// goes backward through OPs tried rewarding successes and punishing failures

void jhcAliaDir::alter_pref () const
{
  jhcAliaCore *core = step->Core();
  const jhcActionTree *atree = &(core->atree);
  double nominal = 1.0;
  int i, win = 0, detail = 1;         

  // possibly announce entry
  if (nri > 0)
    jprintf(2, detail, "alter_pref: %s[ % s ] success\n", KindTag(), KeyTag());

  // go through OPs - may be many successes and failures for ANTE and POST
  for (i = nri - 1; i >= 0; i--)
  {
    jprintf(2, detail, "  %d: OP %d -> verdict %d (pref %4.2f)\n", i, op0[i]->OpNum(), result[i], op0[i]->Pref());
    if (result[i] > 0)
    {
      // move successful OPs closer to nominal preference
      win = 1;
      if (op0[i]->Pref() < nominal)
      {
        op0[i]->AdjPref(atree->pinc);
        jprintf(1, detail, "  ADJUST: operator %d --> raise pref to %4.2f\n", op0[i]->OpNum(), op0[i]->Pref());
        (core->mood).Prefer(atree->pinc);
      }
    }
    else if ((result[i] == -2) && (win > 0))
    {
      // decrease selection probability of failed OPs if something later succeeded
      op0[i]->AdjPref(-(atree->pdec));
      jprintf(1, detail, "  ADJUST: operator %d --> lower pref to %4.2f\n", op0[i]->OpNum(), op0[i]->Pref());
      (core->mood).Prefer(-atree->pdec);
    }
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

  // mark everything as forcibly stopped (no cleanup POST)
  if (kind == JDIR_DO)
  {
    key.Main()->SetNeg(1);
    key.Main()->SetDone(1);  
  }
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
    delete_meth();                     // created by pick_method or chk_method
    if (nri > 0)
      result[nri - 1] = 1;             // top level FIND or CHK succeeded
  }
  else if ((inst >= 0) && (core != NULL))
    inst = core->FcnStop(key.Main(), inst);
}


//= Look for all in-progress activities matching graph and possibly stop them.
// returns 1 if found (and stopped) all, 0 if nothing matching found

int jhcAliaDir::FindActive (const jhcGraphlet& desc, int halt)
{
  jhcAliaCore *core = step->Core();
  jhcAliaDir d2;
  jhcAliaOp *op;

  // make sure this is a command that is still running
  if ((verdict != 0) || ((kind != JDIR_DO) && (kind != JDIR_ANTE)))
    return 0;

  // pretend description is an operator being matched to a copy of this key
  op = core->Probe();
  op->Init(desc);
  d2.Copy(*this);

  // see if any matches found, possibly using halo facts to fill in
  (core->atree).SetMode(2);
  d2.mc = omax;
  if (op->FindMatches(d2, core->atree, (core->atree).MinBlf()) > 0)
  {
    // possibly force failure of directive
    if (halt > 0)
      Stop();
    return 1;
  }

  // see if description matches current method instead
  if (meth != NULL)
    return meth->FindActive(desc, halt);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                            Variable Scoping                           //
///////////////////////////////////////////////////////////////////////////

//= Change directive key by substituting in variable bindings from prior FINDs.
// destructively changes nodes in key graphlet but does not eliminate any 
// this is a relatively clean way to preserve bindings in the rest of chain

void jhcAliaDir::subst_key (const jhcBindings *sc)
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
// automatically instantiates "act" or "fcn" with appropriate new entry
// returns 1 if something new found, 0 if none but waiting, -1 if out of ideas

int jhcAliaDir::pick_method ()
{
  jhcAliaCore *core = step->Core();
  jhcGraphlet ctx2;
  int all, cnt, sel, lvl = step->Level();

  // see if too many methods tried or perhaps wait (2 cycles) for NOTEs to post then halo rules to run 
  if (nri >= hmax)
    return jprintf(">>> More than %d operators failed in jhcAliaDir::pick_method !\n", hmax);
  if (wait++ < 2)
    return jprintf(2, noisy, "%*s... select wait: %4s[ %s ]\n", lvl, "", KindTag(), KeyTag());
  wait = 0;

  // pick among valid operators (only maximally specific ones)
  all = match_ops(sel);
  cnt = max_spec(sel);
  if (cnt > 0)
    jprintf(1, noisy, "%*s %2d choice%s %4s[ %s ]", lvl, "", cnt, ((cnt != 1) ? "s:" : ": "), KindTag(), KeyTag());
  if (cnt <= 0)
  {
    jprintf(2, noisy, "%*s  0 choices %4s[ %s ]\n", lvl, "", KindTag(), KeyTag());
    return -1;
  }
  if (cnt > 1)
    sel = wtd_rand(core->Wild());

  // promote halo facts and rebind variables then record what is being tried
  core->MainMemOnly(match[sel], 0);
  op0[nri] = op[sel];
  m0[nri].Copy(match[sel]);
  nri++;
  if (noisy >= 4)
  {
    jprintf("\n  chose OP %d:\n", op[sel]->OpNum());
    match[sel].Print(4);
    jprintf("\n");
  }

  // instantiate operator, possibly use method preference as base bid
  delete_meth();                                      
  if (kind == JDIR_DO)                           // copy main verb modifiers
    get_context(&ctx2, key.Main(), match[sel]);            
  meth = core->CopyMethod(op[sel], match[sel], &ctx2);     
  if (root > 0) 
    core->SetPref(op[sel]->Pref());

  // show expansion of operator chosen (possibly a persistent intention)
  if ((noisy >= 1) && (meth != NULL))
  {
    if ((kind == JDIR_NOTE) && (step->cont == NULL))      
    {
      jprintf("    Expanding OP %d ...\n", op[sel]->OpNum());
      jprintf("_______________________________________\n");
      jprintf(">>> Set intention (%3.1f secs)\n\n", core->Stretch(op[sel]->Budget()));
      meth->Print(4, 0);
      jprintf("_______________________________________\n\n");
    }
    else
    {
      jprintf("    Expanding OP %d ...\n\n", op[sel]->OpNum());
      meth->Print(4, 0);
      jprintf("\n");
    }
  }

  // possibly simplify FIND/BIND requirements
  if ((kind == JDIR_CHK) || (kind == JDIR_FIND) || (kind == JDIR_BIND))
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
  jhcNetNode *n;
  int i, cnt;

  // always add the given item as "main" item and retain previous adverbs
  if (ctx2 == NULL)
    return;
  ctx2->Copy(ctx);
  ctx2->ReplaceMain(focus);

  // add all relevant associated arguments (skip if part of matching)
  cnt = focus->NumArgs();
  for (i = 0; i < cnt; i++)
  {
    n = focus->Arg(i);
    if (key.InDesc(n) && !b.InSubs(n))
      ctx2->AddItem(n);
  }

  // add all relevant associated properties (skip if part of matching)
  cnt = focus->NumProps();
  for (i = 0; i < cnt; i++)
    if (focus->RoleIn(i, "mod", "dir", "loc"))         // adverbs
    { 
      n = focus->Prop(i);
      if (key.InDesc(n) && !b.InSubs(n))
        ctx2->AddItem(n);
    }
}


///////////////////////////////////////////////////////////////////////////
//                              CHK Control                              //
///////////////////////////////////////////////////////////////////////////

//= Use description in FIND/CHK directives to set up matching criteria.
// this is only to test candidate, not for matching operators

void jhcAliaDir::init_cond ()
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
      return(wmem->Version() + 1);
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

  if ((res = pat_confirm(full)) != 0)
    return res;
  if (subset > 0)
    if ((res = pat_confirm(cond)) != 0)
    {
      if (res >= 2)          // failed clause fails conjunction (questionable?)
        return 2;
      subset = 0;            // stop long methods like "look around"
    }
  return 0;
}


//= See if pattern is confirmed (all true) or refuted (some false).
// returns 0 if unknown, 1 for full match, 2 for blocked match

int jhcAliaDir::pat_confirm (const jhcGraphlet& desc)
{
  jhcBindings match;
  jhcAliaCore *core = step->Core();
  jhcWorkMem *wmem = &(core->atree);
  const jhcNetNode *focus, *mate;
  int i, mc = 1, ni = desc.NumItems();

  // set up matcher parameters
  bth = (core->atree).MinBlf();                  // must be non-hypothetical
  wmem->SetMode(2);                              // use halo inferences also
  chkmode = 1;                                   // ignore "neg" conflicts
  match.expect = ni;

  // find full match to key irregardless of "neg" flags
  // assumes either only one match will be found or all are equivalent
  if (MatchGraph(&match, mc, desc, *wmem) <= 0)
    return 0;
  core->MainMemOnly(match, 0);                   // bring in halo facts

  // determine if bindings confirm or refute description
  for (i = 0; i < ni; i++)
  {
    focus = desc.Item(i);
    if ((mate = match.LookUp(focus)) != NULL)
      if (focus->Neg() != mate->Neg())
        return 2;                                // "alternate" continuation
  }
  return 1;                                      // normal continuation
}


//= Called by MatchGraph for CHK when a full set of bindings is found.
// sees if match confirms or refutes CHK criteria based on "neg" flags
// saves first valid set of bindings in match[0] (prefers main over halo)

int jhcAliaDir::complete_chk (jhcBindings *m, int& mc)
{
  if (dbg >= 2)
    m[mc - 1].Print("MATCH");
  if (mc > 0)
    mc--;                    // mc = 0 always stops matcher
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              FIND Control                             //
///////////////////////////////////////////////////////////////////////////

//= See if criteria fully satisfied or possibly shift to check mode if reduced criteria.
// returns 1 if full match found, 0 if incomplete (partial or CHK), -2 if failed

int jhcAliaDir::seek_instance ()
{
  jhcBindings *scope = step->Scope();
  jhcNetNode *mate, *focus = cond.Main();

  // if starting FIND retry, see if time to try to imagine some instance
  if (focus == NULL)
    return 1;
  jprintf(1, dbg, "seek_instance: %s %s\n", focus->Nick(), ((subset > 0) ? "(reduced)" : ""));
  if (cand0 >= gmax)
  {
    jprintf(1, dbg, "  << Already max guesses for: %s\n", focus->Nick());
    return assume_found();
  }

  // always check to see if original FIND is completely matched (opportunistic)
  if ((mate = sat_criteria(full, cand0, 0)) == NULL)
    if ((recent >= 0) && (pron != NULL))                       // for naked FIND[ x ]
      mate = pron;
  if (mate != NULL)
  {
    if ((cand <= 0) || (guess[cand - 1] != mate))
    {
      mate->XferRef((cand > 0) ? guess[cand - 1] : focus);     // conversation target
      guess[cand++] = mate;                                    // overwrite any partial match
    }
    jprintf(1, noisy, "  << GUESS %d: %s = %s\n", cand, focus->Nick(), mate->Nick());
    scope->Bind(focus, mate);
    subset = 0;
    return 1;
  }
  if (subset <= 0)
  {
    jprintf(1, dbg, "  << No obvious full match candidates: %s\n", focus->Nick());
    return 0;
  }

  // if reduced FIND criteria are completely matched possibly try this as a guess
  if ((cand >= gmax) && (chk_state <= 0))
  {
    jprintf(1, dbg, "  << Too many guesses for: %s\n", focus->Nick());
    return assume_found();
  }
  mate = sat_criteria(cond, cand, subset);
  if (mate == NULL)
  {
    jprintf(1, dbg, "  << No obvious partial match candidates: %s\n", focus->Nick());
    return 0;
  }
  mate->XferRef((cand > 0) ? guess[cand - 1] : focus);         // conversation target
  guess[cand++] = mate;
  jprintf(1, noisy, "  << CONSIDER: %s = %s\n", focus->Nick(), mate->Nick());
    
  // shift to CHK mode for remaining criteria
  halt_subgoal();                                // cleanly stop any on-going op
  if ((meth = chk_method()) == NULL)
    return 0;
  if (nri > 0)
    nri--;                                       // okay to re-use current op if CHK fails

  // start doing constructed CHK method 
  if (noisy >= 1)
  {
    jprintf("  Partial FIND[ %s ] residual ...\n\n", KeyTag());
    meth->Print(4);
    jprintf("\n");
  }
  chk_state = 1;                                 // start meth at next Status() call
  return 0;
}


//= Check for a full match for whatever criteria are listed in the description.
// skip = how many old guesses to exclude (e.g. already thoroughly tried)
// after = 1 if only accept facts after version recorded in subset variable
// returns matching node for search variable if found

jhcNetNode *jhcAliaDir::sat_criteria (const jhcGraphlet& desc, int skip, int after) 
{
  jhcGraphlet desc2;
  jhcBindings match;
  jhcAliaCore *core = step->Core();
  jhcWorkMem *wmem = &(core->atree);
  const jhcNetNode *item;
  jhcNetNode *arg;
  int i, j, na, ni = desc.NumItems(), mc = 1;

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
  gtst = after;                                  // only accept new facts
  exc = skip;                                    // exclude previous guesses

  // set up matcher parameters
  bth = wmem->MinBlf();                          // must be non-hypothetical
  wmem->SetMode(2);                              // use halo inferences also
  chkmode = 0;                                   // "neg" is definitely important!

  // extract new guess from full match (complete_find rejects some)
  recent = -1;                                   // no pronoun match yet
  if (MatchGraph(&match, mc, desc2, *wmem) <= 0)
    return NULL;
  core->MainMemOnly(match, 0);                   // bring in halo facts
  return match.LookUp(focus);
}


//= Called by MatchGraph for FIND/BIND when a full set of bindings is found.
// saves first valid set of bindings in match[0] (prefers main over halo)

int jhcAliaDir::complete_find (jhcBindings *m, int& mc)
{
  const jhcBindings *b = &(m[mc - 1]);
  jhcBindings *scope = step->Scope();
  const jhcNetNode *n;
  jhcNetNode *mate = b->LookUp(focus);
  int i, when, nb = b->NumPairs(), gval = 0;

  // reject if this guess was tried before by some FIND (must be non-hypothetical)
  if ((mate == focus) || scope->InSubs(mate) || !mate->Visible() || !mate->Sure(bth))
    return 0;
  for (i = 0; i < exc; i++)
    if (mate == guess[i])
      return 0;

  // handle special case of naked FIND/BIND with just a target variable
  // Note: largely borrowed from jhcNetRef::match_found
  if ((cond.NumItems() == 1) && (focus->Lex() == NULL) && (focus->NumArgs() <= 0))
  {
    if (mate->String() || !mate->ObjNode())
      return 0;
    when = mate->LastRef();
    jprintf(2, dbg, "MATCH - %s %s\n", mate->Nick(), ((when > recent) ? "keep!" : "ignore")); 
    if (when > recent)
    {
      recent = when;
      pron = mate;
    }
    return 1;
  }

  // possibly reject match for partial FIND/BIND if not new enough
  if (gtst > 0)
  {
    for (i = 0; i < nb; i++)
      if ((n = b->GetSub(i)) != NULL)
        gval = __max(gval, n->Generation());   
    if (gval < gtst)
      return 0;
  }

  // accept and record match
  if (dbg >= 2)
    b->Print("MATCH");
  if (mc > 0)
    mc--;                    // mc = 0 always stops matcher
  return 1;
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


//= Makes up item that fits FIND properties and records proper binding.
// BIND guaranteed to generate at least one candidate (even if made up)
// returns 1 if newly asserted, -2 if already tried (fail)

int jhcAliaDir::assume_found ()
{
  jhcBindings mt;
  jhcAliaCore *core = step->Core();
  jhcActionTree *wmem = &(core->atree);
  jhcBindings *scope = step->Scope();
  jhcNetNode *mate, *focus = full.Main();
  jhcGraphlet *old;
  int i, n = scope->NumPairs();

  // restore full criteria (if needed)
  if (subset > 0)
  {
    cond.Copy(full);
    subset = 0;
  }

  // consider re-selecting earlier variables (some dependencies not explicit)
  // no new or recyled item if already made new one, backup possible, or FIND completely struck out
  if (!hyp.Empty() || step->Fallback() || ((kind == JDIR_FIND) && (cand <= 1)))
  {
    if (cand > 0)
      focus->XferRef(guess[cand - 1]);       // revoke guess as conversation target
    cand = 0;                                // forget guesses from this round
    reset();                                 // possibly pick same ops next time
    return -2;                               // will backtrack if fallback exists
  }

  // when BIND runs out of guesses but had several, resubmit original (best) guess
  if (cand > 0)
  {
    mate = guess[0];                         // revert to initial most probable guess
    mate->XferRef(guess[cand - 1]);          // revoke current guess as conversation target
    scope->RemFinal(focus);
    scope->Bind(focus, mate);
    hyp.AddItem(wmem->MakeNode("dummy"));    // prevent future backtracking to this BIND
    jprintf(1, noisy, "  << REPEAT: %s = %s (initial choice)\n", focus->Nick(), mate->Nick());
    return 1;
  }

  // imagine a suitable object in WMEM (similar to NOTE) but only once (!hyp.Empty())
  // only does this for a BIND (not FIND) that has been unable to make any guesses (cand = 0)
  for (i = 0; i < n; i++)
  {
    mate = scope->GetSub(i);
    mt.Bind(mate, mate);                     // retain all FIND/BIND substitutions
  }
  old = wmem->BuildIn(&hyp);                 // needed for garbage collection
  wmem->Assert(key, mt, -1.0, 0, NULL);      // makes new nodes as needed
  wmem->BuildIn(old);
  wmem->RevealAll(hyp);                      // make available for matching

  // remember hypothesized item as current guess (no FIND retry so no NRI)
  mate = mt.LookUp(focus);
  mate->XferRef((cand > 0) ? guess[cand - 1] : focus);     // conversation target
  scope->Bind(focus, mate);
  jprintf(1, noisy, "  << ASSUME: %s = %s (new)\n", focus->Nick(), mate->Nick());
  return 1;
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
  if (kind == JDIR_PUNT)                 // PUNT[ ] takes no description
    ans = ((in.First("]")) ? 2 : 1);     
  else
    ans = pool.LoadGraph(&key, in);
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
  Save(stdout, lvl + 2, detail);
}
