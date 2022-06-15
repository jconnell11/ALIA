// jhcAliaChain.cpp : sequence backbone for activities in an FSM chain
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
#include <ctype.h>

#include "Interface/jms_x.h"           // common video

#include "Action/jhcAliaCore.h"        // common robot - since only spec'd as class in header    
#include "Action/jhcAliaDir.h"         
#include "Action/jhcAliaPlay.h"

#include "Action/jhcAliaChain.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// WARNING: destroys payload and all steps in connected graph!

jhcAliaChain::~jhcAliaChain ()
{
  // detect loops and delete only once
  if (cut <= 0)               
    cut_loops();

  // get rid of payload object (whichever)
  delete d;
  delete p;

  // get rid of rest of (branched) chain
  delete cont;
  delete alt; 
  delete fail;
}


//= Turn graph into tree for clean deletion.

void jhcAliaChain::cut_loops ()
{
  // mark this step if not already examined
  if (cut > 0)
    return;
  cut = 1;

  // remove continuation pointers to already marked steps 
  if (cont != NULL)
  {
    if (cont->cut > 0)
      cont = NULL;
    else
      cont->cut_loops();          // recurse
  }
  if (alt != NULL)
  {
    if (alt->cut > 0)
      alt = NULL;
    else
      alt->cut_loops();           // recurse
  }
  if (fail != NULL)
  {
    if (fail->cut > 0)
      fail = NULL;
    else
      fail->cut_loops();          // recurse
  }
}


//= Default constructor initializes certain values.

jhcAliaChain::jhcAliaChain ()
{
  // clear flags and payload pointers
  cut = 0;
  idx = -1;
  d = NULL;
  p = NULL;
 
  // no main or alternate continuation yet
  fail = NULL;
  cont = NULL;
  alt  = NULL;
  alt_fail = 1;

  // no numbered jumps
  fnum = 0;
  cnum = 0;
  anum = 0;

  // calling environment
  core = NULL;
  level = 0;
  backstop = NULL;
  spew = 0;

  // set up payload to run
  avoid = NULL;
  prev = 0;
  done = 0;
}


//= Get access to main action tree (i.e. working memory plus foci).

jhcActionTree *jhcAliaChain::ATree () 
{
  if (core == NULL)
    return NULL;
  return &(core->atree);
}


///////////////////////////////////////////////////////////////////////////
//                            Configuration                              //
///////////////////////////////////////////////////////////////////////////

//= Tell if step contains a directive and it is of the given type.

bool jhcAliaChain::StepDir (int kind) const
{
  return((d != NULL) && (d->kind == (JDIR_KIND) kind));
}


//= Add main node of each valid directive to arguments of source node.
// only used by jhcNetBuild with init = 1 

void jhcAliaChain::RefSteps (jhcNetNode *src, const char *slot, jhcNodePool& pool, int init)
{
  jhcAliaChain *ch;
  int i, n;

  // mark all steps for one time use (in case of loops)
  if (init > 0)
    clr_labels(1);
  if (idx > 0)
    return;

  // add payload of step (actual work done by RefDir)
  if (d != NULL)
    d->RefDir(src, slot, pool);
  else if (p != NULL)
  {
    n = p->NumReq();
    for (i = 0; i < n; i++)
    {
      ch = p->ReqN(i);
      ch->RefSteps(src, slot, pool, 0);
    }
    n = p->NumSimul();
    for (i = 0; i < n; i++)
    {
      ch = p->SimulN(i);
      ch->RefSteps(src, slot, pool, 0);
    }
  }
    
  // check for all sorts of continuations
  if (cont != NULL)
    cont->RefSteps(src, slot, pool, 0);
  if (alt != NULL)
    alt->RefSteps( src, slot, pool, 0);
  if (fail != NULL)
    fail->RefSteps(src, slot, pool, 0);
}


//= Go to the (N-1)th normal continuation in chain.
// does not necessarily go to the label N since a play only counts as 1 step
// returns NULL if chain too short

jhcAliaChain *jhcAliaChain::StepN (int n) 
{
  if (n < 0)
    return NULL;
  if (n == 1)
    return this;
  if (cont == NULL)
    return NULL;
  return cont->StepN(n - 1);
}


//= Return second to last step in normal continuation path.

jhcAliaChain *jhcAliaChain::Penult () 
{
  jhcAliaChain *prev = NULL, *step = this;

  while (step->cont != NULL)
  {
    prev = step;
    step = step->cont;
  }
  return prev;
}


//= Return last step in normal continuation path.

jhcAliaChain *jhcAliaChain::Last () 
{
  jhcAliaChain *step = this;
 
  while (step->cont != NULL)
    step = step->cont;
  return step;
}


//= Return directive key of last step in normal continuation path.

jhcGraphlet *jhcAliaChain::LastKey ()
{
  const jhcAliaChain *last = Last();
  jhcAliaDir *dir;

  if (last != NULL)
    if ((dir = last->GetDir()) != NULL)
      return &(dir->key);
  return NULL;
}


//= Add a new step to end of normal continuation path.

jhcAliaChain *jhcAliaChain::Append (jhcAliaChain *tackon)
{
  jhcAliaChain *end = Last();

  end->cont = tackon;
  return this;
}


//= Determine the maximum subgoal depth for this part of the tree.

int jhcAliaChain::MaxDepth () const
{
  // if still working then look at payload
  if ((done == 0) && (d != NULL))
    return d->MaxDepth();
  if ((done == 0) && (p != NULL))
    return p->MaxDepth();
  
  // if finished check where control was passed
  if ((done == 1) && (cont != NULL))
    return cont->MaxDepth();
  if ((done == 2) && (alt != NULL))
    return alt->MaxDepth();
  if ((done == -2) && (fail != NULL))
    return fail->MaxDepth();
  return 1; 
}


//= Determine number of simultaneous activities (possibly subgoaled).
// if leaf > 0 then only considers currently active goals not pass-thrus

int jhcAliaChain::NumGoals (int leaf) const
{
  // if still working then look at payload
  if ((done == 0) && (d != NULL))
    return d->NumGoals(leaf);
  if ((done == 0) && (p != NULL))
    return p->NumGoals(leaf);
  
  // if finished check where control was passed
  if ((done == 1) && (cont != NULL))
    return cont->NumGoals(leaf);
  if ((done == 2) && (alt != NULL))
    return alt->NumGoals(leaf);
  if ((done == -2) && (fail != NULL))
    return fail->NumGoals(leaf);
  return 0; 
}


///////////////////////////////////////////////////////////////////////////
//                              Building                                 //
///////////////////////////////////////////////////////////////////////////

//= Copy a prototype chain but substitute for the nodes in the binding list.
// allocates a new structure with the same branching as this one
// returns NULL if problem, caller must deallocate returned structure

jhcAliaChain *jhcAliaChain::Instantiate (jhcNodePool& mem, jhcBindings& b, const jhcGraphlet *ctx)
{
  jhcAliaChain *seen[100];
  jhcAliaChain *ans;
  int i, n = 0;

  // clear loop back array and all step marks
  for (i = 0; i < 100; i++)
    seen[i] = NULL;
  clr_labels(1);

  // if copy fails then deallocate all parts created
  if ((ans = dup_self(n, seen, mem, b, ctx)) == NULL)
    for (i = 0; i < n; i++)
      delete seen[i];
  return ans;
}


//= Create a new step that is copy of this one including links to other steps.
// either returns cached copy or updates index and returns new instance

jhcAliaChain *jhcAliaChain::dup_self (int& node, jhcAliaChain *seen[], jhcNodePool& mem, 
                                      jhcBindings& b, const jhcGraphlet *ctx)
{
  jhcAliaChain *act, *a2, *s2;
  jhcAliaDir *d2;
  jhcAliaPlay *p2;
  int i, n;

  // check if step already copied, or graph too big
  if (idx > 0)
    return seen[idx - 1];
  if (node >= 100)
    return NULL;

  // make a new step and cache it at idx-1
  s2 = new jhcAliaChain;
  seen[node] = s2;
  idx = ++node;

  // duplicate details of payload
  if (d != NULL)
  {
    d2 = new jhcAliaDir;
    s2->d = d2;
    if (d2->CopyBind(mem, *d, b, ctx) <= 0)
      return NULL;
  }
  else if (p != NULL)
  {
    // make and bind empty new play
    p2 = new jhcAliaPlay;
    s2->p = p2;
    
    // copy main activities 
    n = p->NumReq();
    for (i = 0; i < n; i++)
    {
      act = p->ReqN(i);
      if ((a2 = act->dup_self(node, seen, mem, b, ctx)) == NULL)
        return NULL;
      p2->AddReq(a2);
    }

    // copy guard activities
    n = p->NumSimul();
    for (i = 0; i < n; i++)
    {
      act = p->SimulN(i);
      if ((a2 = act->dup_self(node, seen, mem, b, ctx)) == NULL)
        return NULL;
      p2->AddSimul(a2);
    }
  }

  // copy rest of chain (or graph) as needed
  s2->alt_fail = alt_fail;
  if (cont != NULL)
    s2->cont = cont->dup_self(node, seen, mem, b, ctx);
  if (alt != NULL)
    s2->alt = alt->dup_self(node, seen, mem, b, ctx);
  if (fail != NULL)
    s2->fail = fail->dup_self(node, seen, mem, b, ctx);
  return s2;
}


//= Clear all copy flags anywhere in connected graph.
// sets all idx member variable to zero

void jhcAliaChain::clr_labels (int head)
{
  jhcAliaChain *s;
  int i, n;

  // see if this step already hit
  if ((head <= 0) && (idx == 0)) 
    return;
  idx = 0;

  // clear flags in any bound play (could be in jhcAliaPlay)
  if (p != NULL)
  {
    // go through main activities 
    n = p->NumReq();
    for (i = 0; i < n; i++)
    {
      s = p->ReqN(i);
      s->clr_labels(0);
    }

    // go through guard activities
    n = p->NumSimul();
    for (i = 0; i < n; i++)
    {
      s = p->SimulN(i);
      s->clr_labels(0);
    }
  }

  // clear flags in all attached steps (full graph)
  if (cont != NULL)
    cont->clr_labels(0);
  if (alt != NULL)
    alt->clr_labels(0);
  if (fail != NULL)
    fail->clr_labels(0);
}


//= Tells whether some node appears somewhere in chain.

bool jhcAliaChain::Involves (const jhcNetNode *item, int top) 
{
  jhcAliaChain *s;
  int i, n;

  // sanity check
  if (item == NULL)
    return false;

  // possibly clear markers then see if this step already checked
  if (top > 0)
    clr_labels(1);
  if (idx > 0)
    return false;
  idx = 1;               // mark as checked

  // check directive payload (if any)
  if ((d != NULL) && d->Involves(item))
    return true;

  // check play payload (if any)
  if (p != NULL)
  {
    // go through main activities 
    n = p->NumReq();
    for (i = 0; i < n; i++)
    {
      s = p->ReqN(i);
      if (s->Involves(item, 0))
        return true;
    }

    // go through guard activities
    n = p->NumSimul();
    for (i = 0; i < n; i++)
    {
      s = p->SimulN(i);
      if (s->Involves(item, 0))
        return true;
    }
  }

  // check all attached steps (full graph)
  return(((cont != NULL) && cont->Involves(item, 0)) ||
         ((alt  != NULL) &&  alt->Involves(item, 0)) ||
         ((fail != NULL) && fail->Involves(item, 0)));
}


//= Set mark to one for all nodes belonging instantiated chain.
// useful for mark/sweep garbage collection (jhcActionTree::clean_mem)

void jhcAliaChain::MarkSeeds (int head)
{
  // check for looping then record visit
  if (head > 0)
    clr_labels(1);
  else if (idx > 0)         
    return;
  idx = 1;

  // mark nodes in payload 
  if (d != NULL)
    d->MarkSeeds();
  else if (p != NULL)
    p->MarkSeeds();

  // mark nodes in rest of chain
  if (cont != NULL)
    cont->MarkSeeds(0);
  if (alt != NULL)
    alt->MarkSeeds(0);
  if (fail != NULL)
    fail->MarkSeeds(0);
}


//= Allow chain to act as a generator where it backtracks even on success.
// forces terminal step(s) to always fail (chain is typically a method for FIND)
// spew starts as 0 but no way to reset it to zero after this

void jhcAliaChain::Enumerate ()
{
  // check for looping then check if this is a final step
  if (spew > 0)
    return;
  spew = 2;
  if ((cont == NULL) && (alt == NULL) && (fail == NULL))
    return;
  spew = 1;

  // try all other branches
  if (cont != NULL)
    cont->Enumerate();
  if (alt != NULL)
    alt->Enumerate();
  if (fail != NULL)
    fail->Enumerate();
}
 

///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Start processing this sequence given core and current level.
// used for a new chain with no FIND retry or previous scoping
// negative level used to partially restart any initial play
// returns: 0 = working, -2 = fail 

int jhcAliaChain::Start (jhcAliaCore *all, int lvl)
{
  core = all;
  level = abs(lvl);
  mt0 = jms_now();                     // start of method
  scoping.Clear();
  backstop = NULL;
  return start_payload(lvl);
}


//= Start processing this sequence with environment from prior step.
// FIND retry calls with prior = NULL to retain previous values
// returns: 0 = working, -2 = fail 

int jhcAliaChain::Start (jhcAliaChain *prior)
{
  jhcAliaDir *d0;

  // possibly remove failed binding if this is a FIND retry
  if (prior == NULL)
    scoping.Pop();
  else
  {
    // copy basic information
    core = prior->core;
    level = prior->level;
    mt0 = prior->mt0;
    scoping.Copy(prior->scoping);

    // possibly set a new backtracking point if coming from a FIND/BIND
    d0 = prior->GetDir();
    if ((d0 != NULL) && d0->ConcreteFind())
      backstop = prior;
    else
      backstop = prior->backstop;
  }
  return start_payload(level);
}


//= Start processing whatever payload is bound (directive or play).
// negative level used to partially restart any initial play
// returns: 0 = working, -2 = fail 

int jhcAliaChain::start_payload (int lvl)
{
  prev = 0;
  if (d != NULL)
    done = d->Start(this);
  else if (p != NULL)
    done = p->Start(core, lvl);
  return done;
}


//= Continue running this sequence (focus > 0 if top-level attention item).
// return: 1 (or 2) = done, 0 = working, -2 = fail, -3 = hard fail (PUNT)

int jhcAliaChain::Status ()
{
  const jhcAliaDir *d0;
  int first = ((prev == 0) ? 1 : 0);

  // remember current state for next call
  prev = done;

  // see if activation should be passed to next step 
  if ((done == 1) && (cont != NULL))
    return((first > 0) ? cont->Start(this) : cont->Status());
  if (done == 2) 
  {
    if (alt != NULL)
      return((first > 0) ? alt->Start(this) : alt->Status());
    return((alt_fail > 0) ? -2 : 2);
  }
  if ((done == -2) && (fail != NULL))
    return((first > 0) ? fail->Start(this) : fail->Status());

  // run payload if still active 
  if (done == 0)
  {
    // dispatch to correct type 
    if (d != NULL)
      done = d->Status();
    else if (p != NULL) 
      done = p->Status();

    // if payload fails, unwind to most recent FIND (if not too far committed yet)
//    if ((done == -2) && (backstop != NULL) && (jms_elapsed(mt0) <= core->Dither()))
if ((done == -2) && (backstop != NULL))
{
  double secs = jms_elapsed(mt0);
  jprintf("@@@ possible retry - %4.2f secs [%4.2f]\n", secs, core->Dither());
  if (secs <= core->Dither())


    {
      if ((core != NULL) && ((d0 = backstop->d) != NULL))
        jprintf(1, core->noisy, "\n%*s@@@ unwind and retry %s[ %s ]\n", level, "", d0->KindTag(), d0->KeyTag());
      return backstop->Start(NULL);
    }

}

    // if method for FIND/BIND can be restarted, use as a generator/enumerator
    if ((d != NULL) && ((d->kind == JDIR_FIND) || (d->kind == JDIR_BIND)))     
      if ((done == 1) && (spew >= 2))
      {
        if (core != NULL)
          jprintf(1, core->noisy, "\n%*s@@@ generate variants FIND[ %s ]\n", level, "", d->KeyTag());
        return Start(NULL);
      }

    // see if control will be transferred next cycle
    if (((done ==  1) && (cont != NULL)) ||
        ((done ==  2) && (alt  != NULL)) ||
        ((done == -2) && (fail != NULL)))
      return 0;
  }

  // if control not passed, report local status
  return done;
}


//= Courtesy signal to activity that it is no longer needed. 
// returns current non-zero state (fail, stop, cont, or alt)

int jhcAliaChain::Stop ()
{
  // see if activation passed to next step
  if ((done == 1) && (cont != NULL))
    return cont->Stop();
  else if ((done == 2) && (alt != NULL))
    return alt->Stop();
  else if ((done == -2) && (fail != NULL))
    return fail->Stop();

  // stop payload if still active
  if (done == 0)
  {
    if (d != NULL)
      d->Stop();
    else if (p != NULL)
      p->Stop();
    done = -1;
  }

  // failed, stopped, or end of chain
  return done;
}


//= Look for all in-progress activities matching graph and possibly stop them.
// returns 1 if found (and stopped) all, 0 if nothing matching found
// NOTE: ignores loops for now

int jhcAliaChain::FindActive (const jhcGraphlet& desc, int halt)
{
  // check payload (if this step active)
  if (done == 0)
  {
    if (d != NULL)
      return d->FindActive(desc, halt);
    if (p != NULL)
      return p->FindActive(desc, halt);
  }

  // try continuations (depending on which branch chosen)
  if ((done == 1) && (cont != NULL))
    return cont->FindActive(desc, halt);
  if ((done == 2) && (alt != NULL))
    return alt->FindActive(desc, halt);
  if ((done == -2) && (fail != NULL))
    return fail->FindActive(desc, halt);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                           Reading Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Read at current location in a file to fill in details of self.
// generally called for head of chain, others steps use build_chain
// steps are listed in continuation order (alternate is a jump or ...)
// <pre>
// general format:
//     step1               first directive or play in sequence
//  ~~~ 1                  labelled step
//     step2               
//     % 2                 alternate continuation goto label
//     step3
//     ...                chain end (required since no continuation)
//  ~~~ 2
//     >>>                  play start (optional if top level)
//     +++                  required activity (optional if first)
//       step4
//       step5
//     +++                  second required activity
//       step6
//     ===                  parallel activity (always required)
//       step7
//       step8
//     ===                  second parallel activity
//       step9
//     <<<                  play end (optional if top level)
//     step10
//     @ 1                jump goto label (loop)
//     ...                chain end (optional at end or after jump)
// </pre>
// comment markers are "//" or ";"
// ignores lines starting marker and everything after marker on a line
// returns: 2 = ok + all done, 1 = successful, 0 = syntax error, -1 = end of file, -2 = file error 

int jhcAliaChain::Load (jhcNodePool& pool, jhcTxtLine& in, int play)
{
  jhcAliaChain *fix[100], *label[100];
  int ans, n = 0;

  // ignore blank lines at beginning (but must have something)
  if (in.NextContent() == NULL)
    return -1;

  // create main continuation chain then fix-up jumps
  ans = build_chain(pool, label, fix, n, in);
  if ((play <= 0) && (ans > 2))                    
    ans = 0;                                  // play marker but not in play
  else if (link_graph(fix, n, label) <= 0)
    ans = 0;

  // report parse status
  return((in.Error()) ? -2 : ans);
}


//= Assign main continuation for this step, checking for alt jumps and loops.
// if step is labelled, then definition is cached in "label" array
// saves steps with a numeric jump in "fix" array and increments count "n"
// steps to be fixed have "fail" label in "fnum", "alt" label in "anum", and "cont" label in "cnum" 
// returns: 5 = play end, 4 = new parallel, 3 = new required, 
//          2 = ok + all done, 1 = successful, 0 = syntax error, -1 = end of file

int jhcAliaChain::build_chain (jhcNodePool& pool, jhcAliaChain *label[], jhcAliaChain *fix[], int& n, jhcTxtLine& in)
{
  jhcAliaChain *s2;
  int ans, num, stop = 0;

  // interpret line number (if any) then load up directive or play
  if ((ans = get_payload(pool, label, in)) != 1)
    return ans;
  if (in.Next(1) == NULL)
    return 2;

  // check for alternate CHK continuation jump (e.g. "% 15") - must be first 
  if ((d != NULL) && d->HasAlt() && in.First("%"))
  {
    // see if simple ending
    in.Skip(1);
    if (strncmp(in.Head(), "...", 3) == 0)
      alt_fail = 0;
    else
    {
      // get number part
      if (sscanf_s(in.Head(), "%d", &num) != 1)
        return 0;
      if ((num <= 0) || (num >= 100))
        return 0;

      // add self to fix list and leave indicator
      fix[n++] = this;
      anum = num;
    }

    // replace used-up line (jump @ or end ... can follow) 
    if (in.Next(1) == NULL)
      return -1;
  }

  // check for fail continuation jump (e.g. "# 22") - must come after alt 
  if (in.First("#"))
  {
    // get number part
    in.Skip(1);
    if (sscanf_s(in.Head(), "%d", &num) != 1)
      return 0;
    if ((num <= 0) || (num >= 100))
      return 0;

    // add self to fix list and leave indicator
    if (anum <= 0)
      fix[n++] = this;
    fnum = num;

    // replace used-up line (alt % or jump @ or end ... can follow)
    if (in.Next(1) == NULL)
      return -1;
  }

  // see if normal continuation is a jump (e.g. "@ 11")
  if (in.First("@"))
  {
    // get number part
    in.Skip(1);
    if (sscanf_s(in.Head(), "%d", &num) != 1)
      return 0;
    if ((num <= 0) || (num >= 100))
      return 0;

    // add self to fix list and leave indicator
    // then replace used-up line (end ... can follow)
    if ((anum <= 0) && (fnum <= 0))
      fix[n++] = this;
    cnum = num;
    if (in.Next(1) == NULL)
      return 2;
  }

  // see if chain ends (e.g. "...")
  if (strncmp(in.Head(), "...", 3) == 0)
  {
    // remember ended then replace used-up line
    stop = 1;
    if (in.Next(1) == NULL)
      return 2;
  }

  // check for blank line (end) else make new step for continuation 
  if (in.Blank())
    return 2;
  s2 = new jhcAliaChain;
  if ((ans = s2->build_chain(pool, label, fix, n, in)) <= 0)
    return ans;
  if (s2->Empty())
    delete s2;
  else if ((cnum <= 0) && (stop <= 0))
    cont = s2;
  return ans;
}


//= Configure internal directive or play starting at current file position.
// if step is labelled, then definition is cached in "label" array
// returns: 5 = play end, 4 = new parallel, 3 = new required, 
//          2 = ok + all done, 1 = successful, 0 = syntax error, -1 = end of file

int jhcAliaChain::get_payload (jhcNodePool& pool, jhcAliaChain *label[], jhcTxtLine& in)
{
  int num, ans;

  // re-use previously peeked line or get new line from file
  if (in.Next() == NULL)
    return -1;

  // check for delimiters associated with plays (never follow label)
  if (in.Begins("+++"))
    return in.Flush(3);
  if (in.Begins("==="))
    return in.Flush(4);
  if (in.Begins("<<<"))
    return in.Flush(5);
  
  // check for step number (e.g. "~~~ 12")
  if (in.Begins("~~~"))
  {
    // cache self in label array under number read
    in.Skip(4);
    if (sscanf_s(in.Head(), "%d", &num) != 1)
      return 0;
    if ((num <= 0) || (num >= 100))
      return 0;
    label[num] = this;

    // replace line just consumed
    if (in.Next(1) == NULL)
      return -1;
  }

  // possibly build play starting with next line
  if (in.Begins(">>>"))
  {
    in.Flush();
    p = new jhcAliaPlay;
    return p->Load(pool, in);
  }

  // build directive re-using this line
  d = new jhcAliaDir;
  if ((ans = d->Load(pool, in)) <= 0)
    return ans;
  return 1;
}


//= Substitute pointer to real step for place that were just numbered previously.
// steps to be fixed have "fail" label in "fnum", "alt" label in "anum", and "cont" label in "cnum" 
// numbers correspond to steps in "label" array, "fnum" and "cnum" and "anum" reset to 0
// returns 1 if successful, 0 if some links missing (replaced by NULL)

int jhcAliaChain::link_graph (jhcAliaChain *fix[], int n, jhcAliaChain *label[])
{
  jhcAliaChain *s;
  int i, loc, ans = 1;

  // go through all steps that need to be fixed
  for (i = 0; i < n; i++)
  {
    s = fix[i];
    if ((loc = s->anum) > 0)
    {
      // replace numeric alt with real step
      s->alt = label[loc];
      s->anum = 0;
    }
    if ((loc = s->cnum) > 0)
    {
      // replace numeric cont with real step
      if (label[loc] == NULL)
        ans = 0;
      s->cont = label[loc];
      s->cnum = 0;
    }
    if ((loc = s->fnum) > 0)
    {
      // replace numeric fail with real step
      if (label[loc] == NULL)
        ans = 0;
      s->fail = label[loc];
      s->fnum = 0;
    }
  }
  return ans;
}


///////////////////////////////////////////////////////////////////////////
//                           Writing Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Save self out in machine readable form to current position in a file.
// "lvl" determines how much indenting, but lvl < 0 saves only this step
// "step" is the next step number expected, negative if needs end delimiter
// negates idx field (step label) when step has been reported 
// detail: 0 no extras, 1 show belief, 2 show tags, 3 show both
// generally returns expected next step number but
// top level returns 1 if successful, 0 for bad format, -1 for file error

int jhcAliaChain::Save (FILE *out, int lvl, int *step, int detail)
{
  int *st = step;
  int ans, label = 1, sp = ((lvl >= 0) ? lvl : -(lvl + 1));

  // possibly label all steps (negatives are jump targets)
  if (step == NULL)
  {
    label_all();
    neg_jumps();
    st = &label;
  }

  // possibly terminate last chain 
  if (*st < 0)
    jfprintf(out, "%*s ...\n", lvl, "");

  // see if a label should be emitted for this step
  if ((lvl >= 0) && (idx < 0))
    jfprintf(out, "%*s ~~~ %d\n", lvl, "", -idx); 

  // see what kind of payload is present
  *st = abs(*st) + 1;
  if (d != NULL)
  {
    // CHK directive might need an alternate, all directives can have a "fail" handler
    if ((ans = d->Save(out, sp, detail)) <= 0)
      return ans;
    if (d->HasAlt() && (alt != NULL))
      jfprintf(out, "%*s   %% %d\n", lvl, "", abs(alt->idx));
    if (fail != NULL)
      jfprintf(out, "%*s   # %d\n", lvl, "", abs(fail->idx));
  }
  else if (p != NULL)
  {
    // plays can also have "fail" handlers
    if ((ans = p->Save(out, sp, st)) <= 0)
      return ans;
    if (fail != NULL)
      jfprintf(out, "%*s   # %d\n", lvl, "", abs(fail->idx));
  }

  // stop early if only a single step requested
  if (lvl >= 0)
  {
    // check if next step exists 
    if (cont == NULL)
      *st = -(*st);                                              // request terminator
    else if (abs(cont->idx) != *st)
      jfprintf(out, "%*s   @ %d\n", lvl, "", abs(cont->idx));    // add jump to label
    else if ((ans = cont->Save(out, sp, st, detail)) <= 0)
      return ans;

    // go back and start chasing from alternate continuation (if any)
    if (alt != NULL)
      if (abs(alt->idx) >= *st)
        if ((ans = alt->Save(out, sp, st, detail)) <= 0)
          return ans;

    // go back and start chasing from fail handler (if any)
    if (fail != NULL)
      if (abs(fail->idx) >= *st)
        if ((ans = fail->Save(out, sp, st, detail)) <= 0)
          return ans;
  }
  return((ferror(out) != 0) ? -1 : 1);     
}


//= Give each step a unique number (also useful for debugging).
// includes both step that is play and steps within play 
// SaveChain reports steps in this order (search is same)
// steps that are jump targets are given a negative number
// shared variable "mark" holds last label assigned 
// returns total number of steps in chain

int jhcAliaChain::label_all (int *mark)
{
  jhcAliaChain *act;
  int *m = mark;
  int i, n, label = 0;

  // clear flags if first step
  if (mark == NULL)
  {
    clr_labels(1);
    m = &label;
  }

  // see if this step already hit
  if (idx > 0)
    return(*m);
  idx = ++(*m);

  // label steps within a play (could be in jhcAliaPlay)
  if (p != NULL)
  {
    // go through main activities of play first
    n = p->NumReq();
    for (i = 0; i < n; i++)
    {
      act = p->ReqN(i);
      act->label_all(m);
    }

    // go through guard activities of play second
    n = p->NumSimul();
    for (i = 0; i < n; i++)
    {
      act = p->SimulN(i);
      act->label_all(m);
    }
  }

  // assign labels in all attached steps (full graph)
  if (cont != NULL)
    cont->label_all(m);      // first to ensure consecutive numbering
  if (alt != NULL)
    alt->label_all(m);
  if (fail != NULL)
    fail->label_all(m);
  return(*m);
}


//= Negate idx of steps that are jump targets.
// assumes nodes have just been numbered by label_all
// used by SaveChain to know when to emit labels

void jhcAliaChain::neg_jumps (int *step)
{
  jhcAliaChain *s2;
  int *st = step;
  int i, n, label = 1;

  // start expected step number enumerator
  if (step == NULL)
    st = &label;
  (*st)++;

  // test steps of any bound play (could be in jhcAliaPlay)
  if (p != NULL)
  {
    // go through main activities first
    n = p->NumReq();
    for (i = 0; i < n; i++)
    {
      s2 = p->ReqN(i);
      s2->neg_jumps(st);
    }

    // go through guard activities second
    n = p->NumSimul();
    for (i = 0; i < n; i++)
    {
      s2 = p->SimulN(i);
      s2->neg_jumps(st);
    }
  }

  // see if continuation is to not to the very next state
  if (cont != NULL) 
  {
    if ((cont->idx > 0) && (cont->idx != *st))
      cont->idx = -(cont->idx);
    if (abs(cont->idx) >= *st)
      cont->neg_jumps(st);    
  }

  // alternate will always be a jump
  if (alt != NULL)
  {
    if (alt->idx > 0)
      alt->idx = -(alt->idx);
    if (abs(alt->idx) >= *st)
      alt->neg_jumps(st);
  }

  // fail will always be a jump
  if (fail != NULL)
  {
    if (fail->idx > 0)
      fail->idx = -(fail->idx);
    if (abs(fail->idx) >= *st)
      fail->neg_jumps(st);
  }
}


