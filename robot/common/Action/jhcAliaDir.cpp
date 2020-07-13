// jhcAliaDir.cpp : directive states what sort of thing to do in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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
  delete meth;
}


//= Default constructor initializes certain values.

jhcAliaDir::jhcAliaDir (JDIR_KIND k)
{
  // default configuration
  kind = k;
  root = 0;

  // state
  core = NULL;
  meth = NULL;
  reset();
  verdict = -1;
  noisy = 0;
}


//= Clear out all previous method attempts and bindings.
// does not remove directive definition (kind and key) 
// returns 0 always for convenience

int jhcAliaDir::reset ()
{
  // nothing selected or running
  delete meth;
  meth = NULL;
  inst = -1;

  // current state (running)
  nri = 0;
  cand = 0;
  verdict = 0;
  punt = NULL;

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

  key.Clear();
  kind = ref.kind;
  pool.BuildIn(&key);
  if ((ans = pool.Assert(ref.key, b)) >= 0)      // negative is failure
  {
    ctx.Copy(*ctx2);                             // for cascading
    share_context(ctx2);
  }
  pool.BuildIn(NULL);
  return((ans < 0) ? 0 : 1);
}


//= Add in any adverbs or objects from higher-level call.
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
// useful for mark/sweep garbage collection (jhcAliaAttn::CleanMem)

void jhcAliaDir::MarkSeeds ()
{
  jhcNetNode *n;
  int i, j, nb, ni = key.NumItems();

  // keep all NRI bindings so no dangling pointers (or reused elements)
  jprintf(5, noisy, "  keeping %s[ %s ]\n", ktag[kind], key.MainTag());
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

  // mark all nodes in instantiated key (may not be in NRI yet)
  if (ni > 0)
    jprintf(5, noisy, "    directive key\n");
  for (i = 0; i < ni; i++)
  {
    n = key.Item(i);
    jprintf(5, noisy, "      %s\n", n->Nick());
    n->keep = 1;
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

int jhcAliaDir::Start (jhcAliaCore& all)
{
  int ver = (all.attn).Version();

  // set up internal state, assume something needs to run
  core = &all;
  noisy = core->noisy;
  reset();

// test for pre-emption     

  // possibly initialize 3 phase DO
  if (kind == JDIR_DO) 
  {
    jprintf(2, noisy, "  Converting DO->ANTE[ %s ] - init\n\n", key.Main()->Word());
    kind = JDIR_ANTE;                     
  }
  else if (kind == JDIR_FCN)
    inst = core->FcnStart(key.Main());     // negative inst records fail
  else if (kind == JDIR_NOTE)
  {
    key.ActualizeAll(ver);                 // assert pending truth values
    own = all.Percolate(*this);
  }

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
  int res;

  // test for PUNT (not allowed to abort during Start) and ADD
  if (core == NULL)
    return report(-2);
  if (kind == JDIR_PUNT)
    return report(-3);             
  if (kind == JDIR_ADD)                // accept new rule/op
    return report(core->Accept());

// test for pre-emption

  // possibly run kernel function 
  if (kind == JDIR_FCN)
    return report(core->FcnStatus(key.Main(), inst));

  // get all inferences based on NOTE then make sure some method active
  if (meth == NULL)
    return first_method();

  // see if current method has finished yet (also check for PUNT in method)
  jprintf(2, noisy, "Passing through %s[ %s ]\n", KindTag(), key.Main()->Tag());
  if ((res = meth->Status()) == 0)
    return verdict;
  if (res <= -3)
    return report(-3);

  // generally try next method, but special handling for DO and NOTE
  if (kind == JDIR_DO)
    return do_status(res);
  if ((kind == JDIR_NOTE) && (res > 0))    
    return report(1);                      // stop at first success?
  return next_method();
}


//= Get initial method to try for this directive.
// special handling for 3 phase DO, can nver fail from here

int jhcAliaDir::first_method ()
{
  // possibly print debugging delimiter
  if (noisy >= 2)
  {
    jprintf("~~~~~~~~ start ~~~~~~~~\n");
    Print();
    jprintf("\n");
  }

  // always try to find some applicable operator
  if (pick_method() > 0) 
    verdict = meth->Start(*core);                        // found -> run operator
  else if (kind == JDIR_ANTE)        
  {
    jprintf(2, noisy, "Converting ANTE->DO[ %s ] - no ops\n\n", key.Main()->Word());  
    kind = JDIR_DO;                                      // none -> advance to DO phase 
    (core->attn).MarkBelief(key.Main(), 1.0);
  }
  else if (kind == JDIR_POST)     
  {   
    jprintf(2, noisy, "Converting POST->DO[ %s ] - no ops\n\n", key.Main()->Word());  
    kind = JDIR_DO;
    verdict = ((key.Main()->Neg() <= 0) ? 1 : -2);       // none -> report saved result
  }
  else if (kind == JDIR_NOTE)                            // none -> okay if NOTE
    verdict = 1;
  else
    verdict = -2;                                        // none -> generally fail
  return report(verdict);
}


//= Handle operator finish for DO directive.

int jhcAliaDir::do_status (int res)
{
  // if this operator failed, try another one (if exists) 
  if (res == -2)                                             // -3 = stop backtrack
    if (pick_method() > 0)
    {
      verdict = meth->Start(*core);    
      return verdict;
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
  // possibly print debugging stack
  jprintf(2, noisy, "Continuing on %s[ %s ]\n\n", KindTag(), key.Main()->Tag());

  // see if any more applicable operators (most directives try all)
  if (pick_method() > 0)
    verdict = meth->Start(*core);                        // found -> run operator
  else if (kind == JDIR_ANTE)
  {
    jprintf(2, noisy, "Converting ANTE->DO[ %s ] - all tried\n\n", key.Main()->Word());
    kind = JDIR_DO;                                      // none -> advance to DO phase
    reset();
  }
  else if (kind == JDIR_POST)      
  {
    jprintf(2, noisy, "Converting POST->DO[ %s ] - all tried\n\n", key.Main()->Word());
    kind = JDIR_DO;
    verdict = ((key.Main()->Neg() <= 0) ? 1 : -2);       // none -> report saved result
  }
  else if (kind == JDIR_NOTE)                            // none -> okay if NOTE
    verdict = 1;
  else                    
    verdict = -2;                                        // none -> generally fail                           
  return report(verdict);
}


//= Announce success or failure of some branch for debugging.
// sets verdict to value given (often just verdict itself) and returns this

int jhcAliaDir::report (int val)
{
  verdict = val;
  if ((verdict != 0) && (kind == JDIR_NOTE))
    own = core->ZeroTop(*this);                      // block halo percolation
  if (noisy >= 1)
  {
    if (verdict < 0)
      jprintf("--- failure: %s[ %s ]\n", KindTag(), key.MainTag());
    else if (verdict > 0)
      jprintf("*** success: %s[ %s ]\n", KindTag(), key.MainTag());
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

  // see if some expansion vs. kernel fcn
  if (meth != NULL)
  {
    meth->Stop();
    delete meth;            // was instantied by pick_method
    meth = NULL;
  }
  else if (inst >= 0)
    inst = core->FcnStop(key.Main(), inst);

  // mark everything as forcibly stopped
  verdict = -1;
  return verdict;
}


///////////////////////////////////////////////////////////////////////////
//                            Method Selection                           //
///////////////////////////////////////////////////////////////////////////

//= Match directive along with memory and halo to find good operators.
// consults list of previously tried operators and bindings to avoid repeats
// automatically instantiates "act" or "fcn" with appropriate new entry
// returns 1 if something new found, 0 if out of ideas

int jhcAliaDir::pick_method ()
{
  jhcGraphlet ctx2;
  int cnt, sel;

  // see if it is time to give up on this directive
  if (core == NULL)
    return -1;
  if (nri >= hmax)
    return jprintf(">>> More than %d operators failed in jhcAliaDir::pick_method !\n", hmax);
//  jprintf(1, noisy, "            >>>>>> PICK <<<<<<\n\n");

  // pick among valid operators (can comment out max_spec)
  cnt = match_ops(sel);
  cnt = max_spec(sel);
  jprintf(1, noisy, "  %d choices left for %s[ %s ]\n", cnt, ktag[kind], key.MainTag());
  if (cnt <= 0)
    return 0;
  if (cnt > 1)
    sel = wtd_rand(core->Wild());

  // promote halo facts and rebind variables then record what is being tried
  core->MainMemOnly(match[sel]);
  op0[nri] = op[sel];
  m0[nri].Copy(match[sel]);
  nri++;
  if (noisy >= 4)
  {
    jprintf("  chose OP %d:\n", op[sel]->Inst());
    match[sel].Print(4);
    jprintf("\n");
  }

  // instantiate operator, possibly use method preference as base bid
  delete meth;                                      
  get_context(&ctx2, key.Main(), match[sel]);
  meth = core->CopyMethod(op[sel], match[sel], &ctx2);     // FIND ???
  if (root > 0) 
    core->SetPref(op[sel]->pref);
  if (noisy >= 1)
  {
    jprintf("\n    Expanding to:\n");
    meth->Print(4);
    jprintf("\n");
  }
  core->RecomputeHalo();                           
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
  int tol, i, j, misses = 0, cnt = 0;

  // allow progressively more inexact key matches if needed
  for (tol = 0; (tol <= misses) && (cnt <= 0); tol++)
    if (core->GetChoices(this, tol) > 0)
      for (i = omax - 1; i >= mc; i--)
      {
        // reject operators that have been tried before with equivalent bindings
        for (j = 0; j < nri; j++)
          if ((op[i] == op0[j]) && (op[i]->SameEffect(match[i], m0[j])))
          {
            jprintf(2, noisy, "  OP %d - reject as already tried\n", op[i]->Inst());
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
        jprintf("    OP %d: sp %d x %4.2f -> %2d%%\n", op[i]->Inst(), match[i].NumPairs(), op[i]->pref, pct);
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

int jhcAliaDir::Save (FILE *out, int lvl, int detail) 
{
  if ((kind < 0) || (kind >= JDIR_MAX))
    return 0;
  jfprintf(out, "%*s%4s[ ", lvl, "", ktag[kind]);
  key.Save(out, -(lvl + 6), detail);
  jfputs(" ]\n", out);
  return((ferror(out) != 0) ? -1 : 1);
}


