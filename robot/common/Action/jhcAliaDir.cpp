// jhcAliaDir.cpp : directive states what sort of thing to do in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

const char * const jhcAliaDir::ktag[JDIR_MAX] = {"NOTE", "DO",  "ANTE", "POST", "PUNT", 
                                                 "FCN",  "CHK",  "ACH", "KEEP", "FIND", "ADD"};


//= Default destructor does necessary cleanup.
// jhcAliaChain takes care of deleting rest of graph

jhcAliaDir::~jhcAliaDir ()
{
  jhcAliaCore *core;

  halt_subgoal();
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
  fass = 0;              // FIND never creates specfied item

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
  subset = 0;            // set if FIND sub-method needs CHK follow-up
  cand0 = 0;
//dbg = 1;               // local FIND debug statements (2 or 3 for matcher)
}


//= Clear out all previous method attempts and bindings.
// does not remove directive definition (kind and key) 
// typically used for switching between ANTE -> DO -> POST
// returns 0 always for convenience

int jhcAliaDir::reset ()
{
  // nothing selected or running (no stop needed)
  delete meth;
  meth = NULL;
  inst = -1;

  // current state (running)
  nri = 0;
  verdict = 0;
  wait = -1;

  // multi-method NOTE
  t0 = 0;
  obsess = 0.0;

  // no general node matches permitted
  own = 0;
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
  if ((ans = pool.Assert(ref.key, b)) >= 0)                    // negative is failure
    if ((kind == JDIR_DO) && (ctx2 != NULL))
    {
      ctx.Copy(*ctx2);                                         // for cascading
      share_context(ctx2);
    }

  // cleanup
  pool.BuildIn(NULL);
  return((ans < 0) ? 0 : 1);
}


//= Add in any adverbs or objects from higher-level call (for DO directives only).
// shares nodes already in wmem (e.g. mod-23 -mod-> act-11 and now mod-23 -mod-> act-62 also)

void jhcAliaDir::share_context (const jhcGraphlet *ctx2) const
{
  jhcNetNode *n, *old, *act = key.Main();
  int i, cnt;

  // sanity check
  if (ctx2 == NULL)
    return;
  old = ctx2->Main();

  // add in all old arguments (just extra pointers)
  cnt = old->NumArgs();
  for (i = 0; i < cnt; i++)
  {
    n = old->Arg(i);
    if (ctx2->InDesc(n))
      act->AddArg(old->Slot(i), n);
  }   
      
  // add in all old properties (just extra pointers)
  cnt = old->NumProps();
  for (i = 0; i < cnt; i++)
  {
    n = old->Prop(i);
    if (ctx2->InDesc(n))
      n->AddArg(old->Role(i), act);
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


//= Set mark to one for all nodes belonging to instantiated directives.
// useful for mark/sweep garbage collection (jhcActionTree::clean_mem)
// NOTE: also keep key substitutions, variable guesses?

void jhcAliaDir::MarkSeeds ()
{
  jhcNetNode *n;
  int i, j, nb, ki = key.NumItems(), fi = hyp.NumItems();

  // mark all nodes in instantiated key (may not be in NRI yet)
  jprintf(5, noisy, "    directive key %s[ %s ]\n", ktag[kind], key.MainTag());
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
  jhcActionTree *atree = &(core->atree);
  int ver = atree->Version();

  // set up internal state, assume something needs to run
  step = st;
  subst_key(step->Scope());                // substitute found variables
  noisy = core->noisy;
  reset();
  jprintf(2, noisy, "%*sstart %s[ %s ]\n", step->Level(), "", KindTag(), key.MainTag());

  // initialize various types of directive
  if (kind == JDIR_DO) 
  {
    jprintf(2, noisy, "  Converting DO->ANTE[ %s ] - init\n\n", key.Main()->Word());
    kind = JDIR_ANTE;                     
    key.Main()->SetDone(0);                // restart action if backtrack
    key.Main()->SetNeg(0);
  }
  else if (kind == JDIR_FCN)
    inst = core->FcnStart(key.Main());     // negative inst records fail
  else if (kind == JDIR_NOTE)
  {
    key.ActualizeAll(ver);                 // assert pending truth values
    atree->Endorse(key);
    own = core->Percolate(*this);
    t0 = jms_now();
    obsess = atree->CompareHalo(key);
  }
  else if ((kind == JDIR_FIND) || (kind == JDIR_CHK))
    init_cond();                           // situation to search for

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
  int res;

  // test for special cases
  if (kind == JDIR_PUNT)                                         // discontinue chain
    return report(-3);             
  if (kind == JDIR_ADD)                                          // accept new rule/op
  {
    res = core->Accept(new_rule, new_oper);
    new_rule = NULL;
    new_oper = NULL;
    return report(res);
  }
  if (kind == JDIR_FCN)                                          // run some function
    return report(core->FcnStatus(key.Main(), inst));      
  if ((kind == JDIR_DO) &&                                       // stop matching action
       key.Main()->Neg() && (key.Main()->Done() <= 0))              
    return report(core->HaltActive(key));

  // check for pre-emptive full matches to FIND and CHK criteria
  if (kind == JDIR_CHK)
    if ((res = seek_match()) != 0)
    {
      jprintf(2, noisy, "%*s### direct full CHK[ %s ] match = %d\n",  step->Level(), "", key.MainTag(), res);
      halt_subgoal();
      return report(res);
    }
  if (kind == JDIR_FIND)
    if ((res = seek_instance()) != 0)
    {
      jprintf(2, noisy, "%*s### direct full FIND[ %s ] match = %d\n", step->Level(), "", key.MainTag(), res);
      halt_subgoal();
      return report(res);
    }

  // find some operator to try if nothing selected yet
  if (meth == NULL)
  {
    if (wait >= 0)
      return next_method();
    wait = 1;                                    // no initial wait
    return first_method();
  }

  // see if current method has finished yet (also check for PUNT in method)
  jprintf(2, noisy, "%*sPassing through %s[ %s ]\n", step->Level(), "", KindTag(), key.MainTag());
  if ((res = meth->Status()) == 0)
    return verdict;
  if (res <= -3)
    return report(-3);

  // generally try next method, but special handling for DO and NOTE and FIND
  if (kind == JDIR_DO)
    return do_status(res, 1);
  if ((kind == JDIR_NOTE) && (res > 0) && 
      (jms_secs(jms_now(), t0) > obsess))    
    return report(1);                            // NOTE stops at first success unless obsessed
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
    return report(meth->Start(core, lvl + 1));             // found -> run operator
  if (found == 0)
    return report(0);

  // special handling for various types when no more operators
  if (kind == JDIR_ANTE)        
  {
    jprintf(2, noisy, "Converting ANTE->DO[ %s ] - no ops\n\n", key.Main()->Word());  
    kind = JDIR_DO;                                        // none -> advance to DO phase 
    (core->atree).MarkBelief(key.Main(), 1.0);
  }
  else if (kind == JDIR_POST)     
  {   
    jprintf(2, noisy, "Converting POST->DO[ %s ] - no ops\n\n", key.Main()->Word());  
    kind = JDIR_DO;
    verdict = ((key.Main()->Neg() <= 0) ? 1 : -2);         // none -> report saved result
  }
  else if (kind == JDIR_NOTE)                              // none -> okay if NOTE
    verdict = 1;
  else if (kind == JDIR_FIND) 
    verdict = assume_found();
  else
    verdict = -2;                                          // none -> generally fail
  return report(verdict);
}


//= Handle operator finish for DO directive.

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

  // mark failure or success of DO (checked later) 
  if (res < 0)
    key.Main()->SetNeg(1);
  key.Main()->SetDone(1);

  // advance to POST cleanup phase (only after all DO operators tried)
  jprintf(2, noisy, "Converting DO->POST[ %s ] - %s\n\n", key.Main()->Word(), ((res < 0) ? "failure" : "success"));
  kind = JDIR_POST;
  reset();
  return report(verdict);
}


//= Pick next best operator for directive.
// special handling for 3 phase DO

int jhcAliaDir::next_method ()
{
  jhcAliaCore *core = step->Core();
  int found, lvl = step->Level();

  // possibly print debugging stack
  jprintf(2, noisy, "Continuing on %s[ %s ]\n", KindTag(), key.MainTag());

  // see if any more applicable operators (most directives try all)
  if ((found = pick_method()) > 0)
    return report(meth->Start(core, lvl + 1));             // found -> run operator
  if (found == 0)
    return report(0);

  // special handling for various types when no more operators
  if (kind == JDIR_ANTE)
  {
    jprintf(2, noisy, "Converting ANTE->DO[ %s ] - all tried\n\n", key.Main()->Word());
    kind = JDIR_DO;                                        // none -> advance to DO phase
    reset();
  }
  else if (kind == JDIR_POST)      
  {
    jprintf(2, noisy, "Converting POST->DO[ %s ] - all tried\n\n", key.Main()->Word());
    kind = JDIR_DO;
    verdict = ((key.Main()->Neg() <= 0) ? 1 : -2);         // none -> report saved result
  }
  else if (kind == JDIR_NOTE)                              // none -> okay if NOTE
    verdict = 1;
  else if (kind == JDIR_FIND) 
    verdict = assume_found();
  else                    
    verdict = -2;                                          // none -> generally fail                           
  return report(verdict);
}


//= Announce success or failure of some branch for debugging.
// sets verdict to value given (often just verdict itself) and returns this

int jhcAliaDir::report (int val)
{
  jhcAliaCore *core = step->Core();
  const char *mark = "";
  int lvl = step->Level();

  // possibly announce a transition
  verdict = val;
  if (noisy >= 1)
  {
    if (key.MainNeg() > 0)
      if ((kind == JDIR_ANTE) || (kind == JDIR_DO) || (kind == JDIR_POST))
         mark = "*";
    if (verdict == -2)
      jprintf("%*s--- failure: %4s[ %s ]\n", lvl, "", KindTag(), key.MainTag());
    else if (verdict < 0) 
      jprintf("%*s::: terminated: %4s[ %s ]\n", lvl, "", KindTag(), key.MainTag());
    else if (verdict == 1)
      jprintf("%*s*** success: %4s[ %s%s ]\n", lvl, "", KindTag(), mark, key.MainTag());
    else if (verdict >= 2)
      jprintf("%*s~~~ disproved: %4s[ %s ]\n", lvl, "", KindTag(), key.MainTag());
  }

  // possibly clean up if no longer active (method already stopped and can be scrapped)
  if (verdict != 0)
  {
    if (kind == JDIR_NOTE)
      own = core->ZeroTop(*this);                          // block halo percolation
    delete meth;                                            
    meth = NULL;
  }
  return verdict;
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
    delete meth;            // created by pick_method or chk_method
    meth = NULL;
  }
  else if ((inst >= 0) && (core != NULL))
  {
    inst = core->FcnStop(key.Main(), inst);
    inst = -1;
  }
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
// this is a realtively clean wy to preserve bindings in the rest of chain

void jhcAliaDir::subst_key (const jhcBindings *sc)
{
  jhcNetNode *item, *arg, *sub;
  int j, na, i = 0;

  // skip if no new substitutions to make
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
}


//= Restore key graphlet to condition before subst_key was called.
// undoes editing and destructive modifications

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

  // see if it is time to give up on this directive or perhaps wait for NOTEs to post
  if (nri >= hmax)
    return jprintf(">>> More than %d operators failed in jhcAliaDir::pick_method !\n", hmax);
  if (wait++ < 1)
    return jprintf(2, noisy, "%*s... select wait: %4s[ %s ]\n", lvl, "", KindTag(), key.MainTag());
  wait = 0;

  // pick among valid operators (only maximally specific ones)
  all = match_ops(sel);
  cnt = max_spec(sel);
  if (cnt > 0)
    jprintf(1, noisy, "%*s %2d choice%s %4s[ %s ]", lvl, "", cnt, ((cnt != 1) ? "s:" : ": "), KindTag(), key.MainTag());
  if (cnt <= 0)
  {
    jprintf(2, noisy, "%*s  0 choices: %4s[ %s ]\n", lvl, "", KindTag(), key.MainTag());
    return -1;
  }
  if (cnt > 1)
    sel = wtd_rand(core->Wild());

  // promote halo facts and rebind variables then record what is being tried
  core->MainMemOnly(match[sel]);
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
  delete meth;                                      
  get_context(&ctx2, key.Main(), match[sel]);
  meth = core->CopyMethod(op[sel], match[sel], &ctx2);     
  if (root > 0) 
    core->SetPref(op[sel]->pref);
  if ((noisy >= 1) && (meth != NULL))
  {
    jprintf("  -- Expanding OP %d ...\n\n", op[sel]->OpNum());
    meth->Print(4);
    jprintf("\n");
  }

  // possibly simplify FIND/CHK requirements
  if ((kind == JDIR_FIND) || (kind == JDIR_CHK))
    reduce_cond(match[sel]);
  return 1;
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


//= Find all matching operators from procedural memory.
// binds "sel" to last valid operator found (useful if only one)
// returns number of valid matches

int jhcAliaDir::match_ops (int& sel)
{
  jhcAliaCore *core = step->Core();
  int i, j, cnt = 0;

  // look for a complete match
  sel = omax - 1;
  if (core->GetChoices(this) > 0)
    for (i = omax - 1; i >= mc; i--)
    {
      // reject operators that have been tried before with equivalent bindings
      for (j = 0; j < nri; j++)
        if ((op[i] == op0[j]) && (op[i]->SameEffect(match[i], m0[j])))
        {
          jprintf(2, noisy, "    OP %d - reject as already tried\n", op[i]->OpNum());
          op[i] = NULL;
          break;
        }

      // if not rejected, remember and bump count
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


//= Randomly select between remaining operators.
// returns index of match selected, negative for problem

int jhcAliaDir::wtd_rand (double wild) const
{
  double wt[omax];
  double v, lo, f, pick, avg = 0.0, sum = 0.0;
  int i, pct, cnt = 0;

  // initialize weights for all valid operators
  for (i = omax - 1; i >= mc; i--)
    if (op[i] != NULL) 
    {
      // set proportional to preference and specificity (might all be same)
      v = op[i]->pref * match[i].NumPairs();
      wt[i] = v;

      // collect statistics
      lo = ((cnt <= 0) ? v : __min(lo, v));
      avg += v;
      cnt++;
    }

  // skew distribution by altering standard deviation
  avg /= cnt;
  f = std_factor(lo, avg, cnt, wild);
  for (i = omax - 1; i >= mc; i--)
    if (op[i] != NULL) 
    {
      v = f * (wt[i] - avg) + avg;
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
        jprintf("    OP %d: sp %d x %4.2f -> %2d%%\n", op[i]->OpNum(), match[i].NumPairs(), op[i]->pref, pct);
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


//= Multiplier needed to make gap between hi and lo wrt lo k times bigger.
// choose f so (hi' - lo') / lo' = k * (hi - lo) / lo 
// where hi' = f * (hi - avg) + avg and lo' = f * (lo - avg) + avg

double jhcAliaDir::std_factor (double lo, double avg, int cnt, double wild) const
{
  double k, ratio, f, f2, bot = 0.02;  // 2% chance

  // convert wildness to desired hi/lo amplification
  // wild = 0 -> k = 1.0, wild = 1.0 -> k = 0.25, wild = -1.0 -> k = 4
  k = exp(-1.3863 * wild);

  // compute ideal standard deviation multiplier
  ratio = (1.0 - k) * lo / (k * avg);
  f = 1.0 / (1.0 + ratio);

  // make sure min does not go below bot fraction 
  f2 = (1.0 - cnt * bot) / (1.0 - lo / avg);
  f2 = __max(1.0, f2);
  return __min(f, f2);
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
    if ((n->Belief() == 0.0) || n->LexNode() || !wmem->InList(n))
      cond.AddItem(n);
  }

  // save copy for incremental FIND
  full.Copy(cond);
  subset = 0;
  cand0 = cand;              // FIND retry wants "new" guess
}


//= Reduce FIND/CHK requirements to just those in selected operator.
// triggers the switchover from FIND to CHK mode when satisfied, or early CHK termination

void jhcAliaDir::reduce_cond (const jhcBindings& match) 
{
  jhcNetNode *item;
  int i, ni = full.NumItems();

  cond.Clear();
  for (i = 0; i < ni; i++)
  {
    item = full.Item(i);
    if (match.InSubs(item))
      cond.AddItem(item);
  }
  if (cond.NumItems() < ni)
    subset = ((kind == JDIR_FIND) ? 1 : 2);      // FIND = 1, CHK = 2 (for delay)
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
    if (pat_confirm(cond) >= 2)
      return 2;
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
  core->MainMemOnly(match);                      // bring in halo facts

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


//= See if match confirms or refutes CHK criteria based on "neg" flags.
// saves first valid set of bindings in match[0] (prefers main over halo)

int jhcAliaDir::match_found (jhcBindings *m, int& mc)
{
  if (dbg >= 2)
    m[mc - 1].Print(0, "MATCH");
  if (mc > 0)
    mc--;                    // mc = 0 stops matcher
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              FIND Control                             //
///////////////////////////////////////////////////////////////////////////

//= See if criteria fully satisfied or possibly shift to check mode if reduced criteria.
// returns 1 if full match found, 0 if incomplete (partial or CHK), -2 if failed

int jhcAliaDir::seek_instance ()
{
  jhcAliaCore *core = step->Core();
  jhcBindings *scope = step->Scope();
  jhcNetNode *mate, *focus = cond.Main();
  int lvl = step->Level();

  // if starting FIND retry, see if time to try to imagine some instance
  jprintf(1, dbg, "seek_instance: %s %s\n", focus->Nick(), ((subset > 0) ? "(reduced)" : ""));
  if (cand0 >= gmax)
  {
    jprintf(1, dbg, "  << Already max guesses for: %s\n", focus->Nick());
    return assume_found();
  }

  // always check to see if original FIND is completely matched
  if ((mate = sat_criteria(full, cand0)) != NULL)
  {
    jprintf(1, noisy, "  << FULL MATCH: %s = %s\n", focus->Nick(), mate->Nick());
    guess[cand] = mate;                // overwrite any partial match
    mate->XferRef(*focus);             // delayed user speech (boost likelihood)
    scope->Bind(focus, mate);
    if (subset <= 0)
      cand++;
    return 1;
  }
  if (subset <= 0)
  {
    jprintf(1, dbg, "  << No obvious full match candidates: %s\n", focus->Nick());
    return 0;
  }

  // if reduced FIND criteria are completely matched possibly try this as a guess
  if ((mate = sat_criteria(cond, cand)) == NULL)
  {
    jprintf(1, dbg, "  << No obvious partial match candidates: %s\n", focus->Nick());
    return 0;
  }
  if (cand >= gmax)
  {
    jprintf(1, dbg, "  << Too many guesses for: %s\n", focus->Nick());
    return assume_found();
  }
  jprintf(1, noisy, "  << GUESS %d: %s = %s\n", cand + 1, focus->Nick(), mate->Nick());
  guess[cand++] = mate;

  // shift to CHK mode for remaining criteria
  halt_subgoal();                               // cleanly stop any on-going op
  if ((meth = chk_method()) == NULL)
    return 0;
  if (nri > 0)
    nri--;                                      // okay to re-use current op if CHK fails

  // start doing constructed CHK method
  if (noisy >= 1)
  {
    jprintf("  Partial FIND[ %s ] residual ...\n\n", key.MainTag());
    meth->Print(4);
    jprintf("\n");
  }
  meth->Start(core, lvl + 1);                    // enter CHK mode
  return 0;
}


//= Check for a full match for whatever criteria are listed in the description.
// exc = how many old guesses to exclude (e.g. already thoroughly tried)
// returns matching node for search variable if found

jhcNetNode *jhcAliaDir::sat_criteria (const jhcGraphlet& desc, int exc) 
{
  jhcBindings match;
  jhcAliaCore *core = step->Core();
  jhcBindings *scope = step->Scope();
  jhcWorkMem *wmem = &(core->atree);
  const jhcNetNode *focus = desc.Main();
  jhcNetNode *remap, *mate = NULL;
  int i, mc = 1;

  // set up matcher parameters
  bth = (core->atree).MinBlf();                  // must be non-hypothetical
  wmem->SetMode(2);                              // use halo inferences also
  chkmode = 0;                                   // "neg" is definitely important!
  match.expect = desc.NumItems();

  // search through all nodes to find a possible binding for focus
  while ((mate = wmem->NextNode(mate)) != NULL)
  {
    // see if this guess was tried before by some FIND
    if ((mate == focus) || mate->Hyp() || scope->InSubs(mate))
      continue;
    for (i = 0; i < exc; i++)
      if (mate == guess[i])
        break;
    if (i < exc)
      continue;

    // try to match rest of description if guess is new (-1 is success also)
    jprintf(2, dbg, "  try_mate: %s\n", mate->Nick());
    if (TryBinding(focus, mate, &match, mc, desc, *wmem) != 0) 
    {
      core->MainMemOnly(match);                 // bring in halo facts
      if ((remap = match.LookUp(focus)) != NULL)
        return remap;
      return mate;
    }
  }
  return NULL;
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
  jhcNetNode *item;
  jhcAliaChain *ch;
  jhcAliaDir *d2;
  int i, ni = key.NumItems(), nb = scope->NumPairs();

  // not working on a set of reduced criteria
  if ((subset <= 0) || (cand <= 0))
    return NULL;
  subset = 2;                          // enter CHK mode

  // candidate from jhcFindGuess guaranteed to satisfy part of CHK
  for (i = 0; i < ni; i++)
  {
    item = key.Item(i);
    if (!cond.InDesc(item))
      (ref.key).AddItem(item);
  }
  cond.Copy(full);                     // restore full criteria

  // set up to convert current guess and retain all previous subsitutions
  sc2.Bind(cond.Main(), guess[cand - 1]);
  for (i = 0; i < nb; i++)
  {
    item = scope->GetSub(i);
    sc2.Bind(item, item);
  }

  // return chain consisting of single CHK directive              
  ch = new jhcAliaChain;
  d2 = new jhcAliaDir;
  ch->BindDir(d2);
  d2->CopyBind(*wmem, ref, sc2);
  return ch;
}


//= Makes up item that fits FIND properties and records proper binding.
// FIND never fails for most top-level assertions and commands (fass > 0)
// returns 1 if newly asserted, -2 if already tried (fail)

int jhcAliaDir::assume_found ()
{
  jhcBindings mt;
  jhcAliaCore *core = step->Core();
  jhcNodePool *wmem = &(core->atree);
  jhcBindings *scope = step->Scope();
  jhcNetNode *mate, *focus = full.Main();
  jhcGraphlet *old;

  // restore full criteria (if needed)
  if (subset > 0)
  {
    cond.Copy(full);
    subset = 0;
  }

  // consider re-selecting earlier variables (some dependencies not explicit)
//  if ((step->Fallback() && (nsub > 0)) || !hyp.Empty() || (fass <= 0)) 
  if (step->Fallback() || !hyp.Empty() || (fass <= 0))
  {
    cand = 0;                // forget guesses from this round
    reset();                 // possibly pick same ops next time
    return -2;
  }

  // imagine a suitable object in WMEM (similar to NOTE) but only once
  old = wmem->BuildIn(&hyp);                 // needed for garbage collection
  wmem->Assert(key, mt, -1.0, 0, NULL);      // makes new nodes as needed
  wmem->BuildIn(old);

  // remember hypothesized item as current guess (no FIND retry so no NRI)
  mate = mt.LookUp(focus);
  mate->XferRef(*focus);                     // delayed user speech (boost likelihood)
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
  if (kind == JDIR_NOTE)
    key.Save(out, -(lvl + 6), -1);               // always show blf0 for NOTEs
  else
    key.Save(out, -(lvl + 6), detail);
  jfputs(" ]\n", out);
  return((ferror(out) != 0) ? -1 : 1);
}
