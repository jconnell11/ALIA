// jhcActionTree.cpp : holds attentional foci for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"
#include "Interface/jprintf.h"

#include "Parse/jhcTxtLine.h"          // common audio
#include "Reasoning/jhcAliaOp.h"       // since only spec'd as class in header
#include "Reasoning/jhcAliaRule.h"

#include "Action/jhcAliaDir.h"         // common robot
#include "Action/jhcAliaPlay.h"

#include "Reasoning/jhcActionTree.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// NOTE: deletes all plays and attached elements

jhcActionTree::~jhcActionTree ()
{
  ClrFoci();
}


//= Default constructor initializes certain values.

jhcActionTree::jhcActionTree ()
{
  int i;

  // parallel action status
  for (i = 0; i < imax; i++)
    focus[i] = NULL;
  fill = 0;
  chock = 0;
  svc = -1;
  now = 0;

  // goal count requisition number
  req = 0;

  // surprise parameters
  drill = 1.3;         // threshold for explict surprise NOTE
  dwell = 5.0;         // obsession with contradiction (sec)
  calm  = 1.0;         // standard surprise decay (sec)

  // thresholds and adjustment parameters
  LoadCfg();
  pess = pth0;
  wild = wsc0;
}


//= Tells how many foci are still active (omits ones which are done).

int jhcActionTree::Active () const
{
  int i, cnt = 0;

  for (i = 0; i < fill; i++)
    if (done[i] <= 0)
      cnt++;
  return cnt;
}


//= Determine the maximum subgoal depth across all active foci.

int jhcActionTree::MaxDepth () 
{
  int i, deep, cyc = ++req, win = 0;

  for (i = 0; i < fill; i++)
    if (done[i] <= 0)
      if ((deep = focus[i]->MaxDepth(cyc)) > win)
        win = deep;
  return win;
}


//= Determine number of activities (possibly subgoaled) across all active foci.
// if leaf > 0 then only considers currently active goals not pass-thrus

int jhcActionTree::NumGoals (int leaf)
{
  int i, cyc = ++req, cnt = 0;         

  for (i = 0; i < fill; i++)
    if (done[i] <= 0)
      cnt += focus[i]->NumGoals(leaf, cyc);
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for adjusting rule confidence and operator preference.
// NOTE: these parameters affect personality

int jhcActionTree::adj_params (const char *fname)
{
  jhcParam *ps = &aps;
  int ok;

  ps->SetTag("tree_adj", 0);
  ps->NextSpecF( &bth0,   0.5, "Min belief threshold default");
  ps->NextSpecF( &cinc,   0.1, "Correct rule confidence up");
  ps->NextSpecF( &cdec,   0.1, "Wrong rule confidence down");
  ps->NextSpecF( &pth0,   0.5, "Min preference thresh default");
  ps->NextSpecF( &pinc,   0.1, "Marginal op preference up");
  ps->NextSpecF( &pdec,   0.1, "Failed op preference down");

  ps->NextSpecF( &fresh, 30.0, "Action lookback limit (sec)");
  ps->NextSpecF( &wsc0,   0.5, "Wildness default value");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read just deployment specific values from a file.e.

int jhcActionTree::LoadCfg (const char *fname)
{
  int ok = 1;

  ok &= adj_params(fname);
  return ok;
}


//= Write current deployment specific values to a file.

int jhcActionTree::SaveCfg (const char *fname) const
{
  int ok = 1;

  ok &= aps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                     Rule and Operator Adjustment                      //
///////////////////////////////////////////////////////////////////////////

//= Increase rule confidence given the halo belief vs a newly asserted version.
// conf0 = result halo fact belief (never changed) which is original rule confidence
// returns signed amount by which confidence was changed 

double jhcActionTree::inc_conf (jhcAliaRule *r, double conf0) const 
{
  double c = conf0 + cinc;             // max boost wrt original conf

  if (conf0 >= MinBlf())                 
    return 0.0;
  return AdjRuleConf(r, __min(c, 1.2)); 
}


//= Decrease rule confidence given halo belief vs newly asserted contradiction.
// conf0 = result halo fact belief (never changed) which is original rule confidence
// returns signed amount by which confidence was changed 

double jhcActionTree::dec_conf (jhcAliaRule *r, double conf0) const
{
  double c = conf0 - cinc;             // max ding wrt original conf

  if (conf0 < MinBlf())                
    return 0.0;
  return AdjRuleConf(r, __max(0.1, c));   
}


//= Set the confidence of the given rule's result to be some new value.
// returns signed amount by which confidence was changed 

double jhcActionTree::AdjRuleConf (jhcAliaRule *r, double cf) const
{
  double chg;
  
  if (r == NULL)
    return 0.0;
  chg = r->SetConf(cf);                // might already be cf
  if ((noisy >= 1) && (chg != 0.0))
    jprintf("  ADJUST: rule %d --> %s conf to %4.2f\n", 
            r->RuleNum(), ((chg > 0.0) ? "raise" : "lower"), r->Conf());
  return chg;                    
}


//= Alter the preference of the given operator to be either higher or lower.
// returns signed amount by which preference was changed 

double jhcActionTree::AdjOpPref (jhcAliaOp *op, int up, int show) const
{
  double chg;

  if (op == NULL) 
    return 0.0;
  chg = op->SetPref(op->Pref() + ((up > 0) ? pinc : -pdec));
  if ((noisy >= 1) && (chg != 0.0))
  {
    jprintf("  ADJUST: operator %d --> %s pref to %4.2f\n", 
            op->OpNum(), ((chg > 0.0) ? "raise" : "lower"), op->Pref());
    if (show > 0)
    {
      jprintf("\n.................................\n");
      op->Print(0);
      jprintf(".................................\n\n");
    }
  }
  return chg;
}


///////////////////////////////////////////////////////////////////////////
//                          List Manipulation                            //
///////////////////////////////////////////////////////////////////////////

//= Returns the index of the next newest foci (negative when done).
// searches down from end of list (as recorded at beginning of cycle)
// foci added on this cycle are not serviced, removal only at end of cycle 

int jhcActionTree::NextFocus ()
{
  int i;

  // find next unselected active focus (tail first)
  for (i = chock - 1; i >= 0; i--)
    if ((mark[i] <= 0) && (done[i] <= 0))
      break;

  // mark focus as used
  if (i >= 0)
    mark[i] = 1;
  svc = i;                   // important for ServiceWt
  return svc;
}


//= Get a pointer to a particular item in the list.

jhcAliaChain *jhcActionTree::FocusN (int n) const
{
  if ((n < 0) || (n >= fill))
    return NULL;
  return focus[n];
}


//= Return last explicit error message associated with the current focus.
// explicit errors typically only useful for DO directives

jhcGraphlet *jhcActionTree::Error () 
{
  if ((svc < 0) || (svc >= fill) || err[svc].Empty())
    return NULL;
  return &(err[svc]);
}


//= Tells whether the chain has been started yet or not.

bool jhcActionTree::NeverRun (int n) const
{
  if ((n < 0) || (n >= fill))
    return false;
  return((done[n] <= 0) && (active[n] == 0));
}


//= Gives a priority for actions connected to this focus.
// mostly based on weight with a slight boost for recency

int jhcActionTree::BaseBid (int n) const 
{
  if ((n < 0) || (n >= fill))
    return 0;
  return(ROUND(1000.0 * wt[n]) + boost[n]);
}


//= Mark the given focus as active or finished at the current time.
// this is typically when the chain verdict is zero
// looks up index of given focus since original service number might change

void jhcActionTree::SetActive (const jhcAliaChain *s, int running)
{
  int i;

  for (i = 0; i < fill; i++)
    if (focus[i] == s)
      break;
  if (i >= fill)
    return;
  if (running > 0)
    active[i] = now;
  else
    done[i] = 1;
}


//= Change the weight of the focus currently being serviced.
// private member variable "svc" set by NextFocus
// returns updated base bid for the current focus

int jhcActionTree::ServiceWt (double pref)
{
  if ((svc < 0) || (svc >= fill))
    return 0;
  wt[svc] = pref;
  return BaseBid(svc);
}


///////////////////////////////////////////////////////////////////////////
//                             List Modification                         //
///////////////////////////////////////////////////////////////////////////

//= Clear all actions and reset state for the beginning of a sequence.
// also clears memory and makes new "self" and "user" nodes

void jhcActionTree::ResetFoci (const char *rname)
{
  // get rid of all pending actions
  ClrFoci();

  // clear halo and working memory
  nkey.Clear();
  BuildIn(NULL);
  Reset();
  InitPeople(rname);
 
  // reset current time stamp and thresholds
  now = 0;
  SetMinBlf(bth0);
  SetMinPref(pth0);
  SetWild(wsc0);
}


//= Removes all items from attention list.

void jhcActionTree::ClrFoci ()
{
  int i;

  // reset goal count requisition number
  req = 0; 

  // clear focal items
  for (i = 0; i < fill; i++)
  {
    delete focus[i];
    focus[i] = NULL;
    err[i].Clear();
  }
  fill = 0;
  chock = 0;
  blame = 1;                 // record specific failure reasons
  svc = -1;
}


//= Add item onto end of agenda list with some current importance.
// returns index of item if added, -1 if array is already full (item NOT deleted)
// NOTE: item will be deleted by this class if accepted (don't keep a pointer)

int jhcActionTree::AddFocus (jhcAliaChain *f, double pref)
{
  jhcAliaDir *d;
  int i;

  // sanity check
  if (fill >= imax)
    if (drop_oldest() <= 0)
    {
      jprintf(">>> More than %d foci in jhcActionTree::AddFocus !\n", imax);
      return -1;
    }

  // possibly announce action
  if (noisy >= 1)
  {    
    jprintf("___________________________________\n");
    jprintf(">>> New active focus %-3d           \\\n\n", Active() + 1);
    f->Print(2);
    jprintf("___________________________________/\n\n");
  }

  // add to list and mark unfinished and unselected
  focus[fill] = f;
  done[fill] = 0;            
  mark[fill] = 0;

  // set up to copy method preference to weight for NOTEs
  wt[fill] = pref;
  if ((d = f->GetDir()) != NULL)
    if (d->kind == JDIR_NOTE)
      d->root = 1;

  // importance boost computed from older items
  boost[fill] = 0;
  for (i = fill - 1; i >= 0; i--)
    if (done[i] <= 0)
      break;
  if (i >= 0)
    boost[fill] = boost[i] + 1;

  // timing (zero active marks beginning)
  active[fill] = 0;      
  return fill++;
}


//= Remove oldest finished focus to make room for something new.
// returns 1 if space available, 0 if all foci are active

int jhcActionTree::drop_oldest ()
{
  int i, age, worst, drop = -1;

  // find complete focus that has been around longest
  for (i = 0; i < fill; i++)
    if (done[i] > 0)
    {
      age = jms_diff(now, active[i]);
      if ((drop < 0) || (age > worst))
      {
        worst = age;
        drop = i;
      }
    }

  // try to remove the focus
  if (drop < 0)
    return 0;
  rem_compact(drop);
  return 1;
}


//= Create a new NOTE directive containing a single item.

void jhcActionTree::NoteSolo (jhcNetNode *n)
{
  StartNote();
  nkey.AddItem(n);
  FinishNote();
}


///////////////////////////////////////////////////////////////////////////
//                               Maintenance                             //
///////////////////////////////////////////////////////////////////////////

//= Discards old foci, removes unused nodes, and enforces local consistency.
// must mark all seed nodes to retain before calling with gc > 0
// need to call this to clean up main memory before recomputing halo
// returns positive if working memory has changed, 0 if same as last cycle

int jhcActionTree::Update (int gc)
{
  int i;

  // record current time and clean up old foci
  now = jms_now();
  prune_foci();

  // clean up old nodes in main memory
  if (gc > 0)
  {
//    prune_foci();
    for (i = 0; i < fill; i++)
    {
      focus[i]->MarkSeeds();           // make sure to keep these
      err[i].MarkSeeds();
    }
    CleanMem(noisy - 4);
  }

  // update generation number and report 
  ver++; 
  return Changes();
}


//= Remove any expired items from list based on the current time.
// only keeps semantic network nodes attached to foci or active directives
// returns number of foci currently in list (for NextFocus)

int jhcActionTree::prune_foci ()
{
  int i = 0, ms = 30000;         // default 30 sec

  // remember cycle start time and remove expired foci
  while (i < fill)
    if ((done[i] > 0) && (jms_diff(now, active[i]) > ms))     
      rem_compact(i);   
    else
    {
      // mark focus as eligible to be tried again
      mark[i] = 0;                    
      i++;
    }

  // remember how many items at start of cycle
//  jprintf(3, noisy, "FOCI: %d active (%d inactive)\n\n", Active(), Inactive());
  chock = fill;
  return chock;
}


//= Remove particular item and re-compact list.

void jhcActionTree::rem_compact (int n)
{
  int i;

  // remove item from list
  if (noisy >= 2)
  {
    jprintf("\n::::::::::::::::::::::::::::::::::::::::\n");
    jprintf(">>> Removing inactive focus %d\n\n", Inactive());
    focus[n]->Print(2);
    jprintf("\n::::::::::::::::::::::::::::::::::::::::\n\n");
  }
  else if (noisy == 1)
    jprintf(">>> Removing inactive focus %d\n", Inactive());
  delete focus[n];
  err[n].Clear();
  fill--;
  chock--;

  // copy relevant part of each next focus to this focus
  for (i = n; i < fill; i++)
  {
    // status
    err[i].Copy(err[i + 1]);
    focus[i] = focus[i + 1];
    done[i]  = done[i + 1];
    mark[i]  = mark[i + 1];

    // importance
    wt[i]    = wt[i + 1];
    boost[i] = boost[i + 1];

    // timing
    active[i] = active[i + 1];
  }
  focus[fill] = NULL;                  // for safety
  err[fill].Clear();
  if (svc > n)
    svc--;                             // important for ServiceWt
}


///////////////////////////////////////////////////////////////////////////
//                             Halo Interaction                          //
///////////////////////////////////////////////////////////////////////////

//= Determines how unexpected some situation is relative to halo expectation.
// in jhcActionTree not jhcWorkMem because it can add new NOTE foci
// only looks at nodes with belief > 0 so ignores inferences based on hypotheticals
// returns surprise encountered for matching graphlet

double jhcActionTree::CompareHalo (const jhcGraphlet& key, jhcAliaMood& mood)
{
//  jhcAliaDesc *evt;
  const jhcNetNode *mate;
  jhcNetNode *focus;
  jhcAliaRule *r;
  double s, blf, halo, chg, lo, surp = 0.0;
  int i, ni = key.NumItems(), hit = 0, miss = 0;

  // examine each non-object element of the situation 
  for (i = 0; i < ni; i++)
  {
    // compare against ALL matching predictions in halo
    focus = key.Item(i);
    if (focus->ObjNode())
      continue;
    blf = focus->Belief();
    mate = NULL;
    lo = -1.0;
    while ((mate = halo_equiv(focus, mate)) != NULL)
    {
      // calculate surprise relative to selected halo prediction
      halo = mate->Belief();
      if (focus->Neg() == mate->Neg())
        s = fabs(blf - halo);                    // expected to some degree
      else
        s = blf + halo;                          // full contradiction
      if (lo < 0.0)
        lo = s;
      else
        lo = __min(s, lo);                       // unexpectedness of element
      surp = __max(lo, surp);                    // combine across whole key

      // check if halo rule's prediction was correct or wrong (1-step or 2-step)
      r = mate->hrule;
      if (halo >= MinBlf())
      {
        jprintf(3, noisy, "%s (%4.2f) from RULE %d [%d] %s %s (%4.2f)\n", 
                mate->Nick(), halo, r->RuleNum(), ((InBand(mate, 2)) ? 2 : 3),  
                ((mate->Neg() == focus->Neg()) ? "agrees with" : "opposes"), focus->Nick(), blf);
        if (mate->Neg() == focus->Neg())
          hit++;
        else
          miss++;
      }

      // for 1-step inferences (result in band 2) credit assignment is clear
      //   if correct and halo belief < current threshold then increment rule conf
      //   if wrong and halo belief >= current threshold then decrement rule conf
      // any given rule will only be adjusted in one direction (based on original conf)
      if (InBand(mate, 2))
      {
        if (mate->Neg() == focus->Neg()) 
          chg = inc_conf(r, halo);               // tests halo against skep
        else  
          chg = dec_conf(r, halo);               // tests halo against skep
        mood.RuleAdj(chg);
        if ((chg != 0.0) && (noisy >= 1))
          jprintf("  ADJUST: rule %d --> %s conf to %4.2f\n", r->RuleNum(), 
                  ((chg > 0.0) ? "raise" : "lower"), r->Conf());
      }
    }

    // possibly generate a NOTE about this fact if highly unexpected
/*
    if (lo > drill)
    {
      StartNote();
      evt = NewAct("surprise");                  // done = 0 -> "surprising"
      AddArg(evt, "agt", focus);
      AddArg(evt, "obj", Self());
      FinishNote();
    }
    hi = __max(lo, hi);                          // combine across whole key
*/
  }

  // feedback to main belief threshold if it could have been used
  mood.RuleEval(hit, miss, surp);
  return surp;                                   // no longer needed
}


//= Look through halo to find some node with similar arguments (ignores negation).
// only looks at nodes with belief > 0 so ignores inferences based on hypotheticals

const jhcNetNode *jhcActionTree::halo_equiv (const jhcNetNode *n, const jhcNetNode *h0) const
{
  const jhcNetNode *h = h0;
  int i, na = n->NumArgs(), bin = n->Code();

  // look for superficially similar node in halo portion (not ghost fact)
  while ((h = halo.Next(h, bin)) != NULL)
    if (!InBand(h, 1))
      if ((h->Belief() > 0.0) && (h->Done() == n->Done()) && 
          (h->NumArgs() == na) && h->LexMatch(n))
      {
        // check that all arguments match exactly
        for (i = 0; i < na; i++)
          if (!h->HasVal(n->Slot(i), n->Arg(i)))
            break;
        if (i >= na)
          return h;
      }
  return NULL;
}


//= Promote result of any halo rules used to main memory and create NOTEs.
// alters original bindings so they reflect new main nodes generated
// never alters halo provenance markers:
//   suppose rule 1 -> A 0.7 + B 0.8   so A provenance => rule 1
//   while   rule 2  ->        B 0.9   so B provenance => rule 2
// current rule depends on A & B so instantiate both rule 1 and 2 (higher conf for B)
// select item = A so instantiate rule 1 and make NOTE (save B-halo = B-wmem in h2m)
// next select item = B so instantiate rule 2 and make NOTE (use B-halo = B-wmem in NOTE)
// in jhcActionTree not jhcWorkMem because it can add new NOTE foci
// can generate no NOTEs (note = 0), for all facts (1), or only for directly relevant facts (2)
// returns number of NOTEs generated (rules used)

int jhcActionTree::ReifyRules (jhcBindings& b, int note)
{
  jhcBindings h2m, b2;
  jhcAliaChain *ch;
  jhcAliaDir *dir;
  jhcNetNode *item;
  int step, fcnt = 0;

  // progressively find all non-wmem nodes that need to move to wmem
  while ((item = pick_non_wmem(step, b, h2m, 0)) != NULL)
  {
    // move non-wmem nodes in precondition of rule that yielded this item
    if (item->hbind != NULL)
      promote_all(h2m, *(item->hbind));
    else                                         
    {
      // for halo-ized properties of moored items
      b2.Clear();
      b2.Bind(item, item);             
      promote_all(h2m, b2);
    }
    b.ReplaceSubs(h2m);
    if ((note <= 0) || (step < note))
      continue;

    // instantiate result of rule using main memory nodes
    dir = new jhcAliaDir;
    b2.CopyReplace(item->hbind, h2m);
    (item->hrule)->Inferred(dir->key, b2);

    // embed NOTE in chain and add to attention queue
    jprintf(1, noisy, "\n");
    ch = new jhcAliaChain;
    ch->BindDir(dir);
    AddFocus(ch);
    fcnt++;
  }
  return fcnt;
}


//= Find some non-wmem fact that needs to be moved to working memory.
// assumes original bindings "b" already augmented by halo migrations in "h2m"
// sets step = 2 if directly relevant, step = 1 if precursor to relevant
// returns NULL when all substitutions are in wmem 

jhcNetNode *jhcActionTree::pick_non_wmem (int& step, const jhcBindings& b, const jhcBindings& h2m, int stop) const
{
  jhcBindings b2;
  jhcNetNode *sub, *mid;
  int i, d2, bcnt = b.NumPairs();

  // look for halo nodes among binding substitutions
  if (stop > 0)
    bcnt = __min(stop, bcnt);
  for (i = 0; i < bcnt; i++)
  {
    // check that substitution is not in working memory
    sub = b.GetSub(i);
    if (InMain(sub))
      continue;

    // check if source rule itself (if any) used any halo facts to trigger 
    if ((stop <= 0) && (sub->hrule != NULL))
    {
      b2.CopyReplace(sub->hbind, h2m);
      if ((mid = pick_non_wmem(d2, b2, h2m, (sub->hrule)->NumPat())) != NULL)
      {
        step = 1;
        return mid;
      }
    }
    step = 2;
    return sub;
  }
  return NULL;
}

//= Make suitably connected main memory node for each halo or LTM node in bindings.
// saves mapping of correspondences between input nodes and replacements in "h2m"
// does not change original rule bindings "b" (still contains halo/LTM references)
// used with jhcActionTree::ReifyRules

void jhcActionTree::promote_all (jhcBindings& h2m, const jhcBindings& b)
{
  jhcBindings b2;
  jhcNetNode *n2, *n;
  const jhcNetNode *n0;
  int i, j, hcnt, narg, nb = b.NumPairs(), h0 = h2m.NumPairs();

  // make a main node for each non-wmem node using same lex and neg
  BuildIn(NULL);
  b2.CopyReplace(b, h2m);              // subs from earlier promotions
  for (i = 0; i < nb; i++)
  {
    // make wmem node for this substitution (if any required arguments)
    n = b2.GetSub(i);
    promote(h2m, n);
    narg = n->NumArgs();
    for (j = 0; j < narg; j++)
      promote(h2m, n->ArgSurf(j));
  }

  // replicate structure of each halo node for replacement main node
  hcnt = h2m.NumPairs();
  for (i = h0; i < hcnt; i++)
  {
    // copy each argument from halo node to main node
    n0 = h2m.GetKey(i);
    narg = n0->NumArgs();
    n = h2m.GetSub(i);
    for (j = 0; j < narg; j++)
    {
      // if points to halo node, replace with new main node
      n2 = n0->ArgSurf(j);
      if (!InMain(n2))
        n2 = h2m.LookUp(n2);         // should never fail
      n->AddArg(n0->Slot(j), n2);
    } 
  }
}


//= Make equivalent wmem node, actualize result as visible, and save translation.
// returns 1 if new node made, 0 if not needed

int jhcActionTree::promote (jhcBindings& h2m, jhcNetNode *n)
{
  jhcNetNode *n2, *deep = n->Deep();

  // make sure not already in working memory (ensures halo or LTM)
  if (InMain(n) || h2m.InKeys(n))
    return 0;

  // make new main memory node and remember correspondence
  n2 = MakeNode(n->Kind(), n->Lex(), n->Neg(), 1.0, n->Done());     
  n2->SetBelief(n->Default());  
  n2->Reveal();
  h2m.Bind(n, n2);

  // special handling if LTM node is brought in
  if (n->ObjNode() && !deep->Halo())
  {
    jprintf(1, noisy, "\n:- PROMOTE creates %s (%s) for memory %s\n", n2->Nick(), n2->LexStr(), deep->Nick());
    n2->MoorTo(deep);
    NoteSolo(n2);
  }
  else
    jprintf(1, noisy, " + creating %s (%s) for halo %s\n", n2->Nick(), n2->LexStr(), n->Nick());
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Execution Tracing                           //
///////////////////////////////////////////////////////////////////////////

//= Look for ALL in-progress activities matching graph and cause them to terminate.
// return 1 if all stopped (or no matches), -2 if cannot stop some

int jhcActionTree::HaltActive (const jhcGraphlet& desc, const jhcAliaDir *skip, int bid)
{
  jhcNetNode *act = desc.MainAct();
  jhcAliaChain *ch;
  int i, n = NumFoci(), ans = 1;

  // make sure pattern to match is not negated
  if (act == NULL)
    return 0;
  act->SetNeg(0);

  // scan through each focus (but ignore calling directive)
  for (i = 0; i < n; i++)
    if ((ch = FocusN(i)) != NULL)
    {
      // stop if current focus has high enough priority (else mark problem)
      if (bid >= BaseBid(i))
        ch->HaltActive(desc, skip, 1);
      else if (ch->HaltActive(desc, skip, 0) > 0)    
        ans = -2;                     
    }

  // restore description negation
  act->SetNeg(1);
//  main->SetBelief(1.0);
  return ans;
}


//= Determine operator which initiated action matching description.
// binds mapping from description to operator variables and corresponding operator action node 
// returns operator responsible (if any)
// Note: generally inadvisable to directly expose src directive or call tree details

jhcAliaOp *jhcActionTree::Motive (const jhcGraphlet& desc, const jhcNetNode **main, jhcBindings *d2o)
{
  jhcBindings d2a;
  jhcAliaOp *op;
  const jhcAliaDir *act, *src;
  const jhcBindings *o2m, *m2a;
  const jhcNetNode *a, *m, *co;
  int i, nb;

  // set defaults 
  if (d2o != NULL)
    d2o->Clear();
  if (main != NULL)
    *main = NULL;

  // look for match to recent action and get caller
  act = find_call(&src, &d2a, desc, 1);
  if (src == NULL)
    return NULL;
  op = src->LastOp();
  o2m = src->LastVars();
  m2a = act->StepScope();
  nb = d2a.NumPairs();

  // create mapping from description vars to operator vars
  if (d2o != NULL)
    for (i = 0; i < nb; i++)  
    {
      a = d2a.GetSub(i);
      if ((m = m2a->FindKey(a)) == NULL)         // undo any FIND guess
        m = a;
      co = o2m->FindKey(m);
      d2o->Bind(d2a.GetKey(i), op->Wash(co));
    }

  // look up equivalent operator node for action found 
  if (main != NULL)
    *main = o2m->FindKey(act->KeyAct());         // never from FIND
  return op;
} 


//= Find the call pattern for the most recently started activity compatible with the description.
// returns best match directive and binds its caller (if any), done: 0 = ongoing only, 1 = any non-failure
// Note: provides on-demand definite verb phrase anaphora resolution

const jhcAliaDir *jhcActionTree::find_call (const jhcAliaDir **src, jhcBindings *d2a, const jhcGraphlet& desc, int done) 
{
  jhcNetNode *main = desc.MainAct();
  const jhcAliaDir *act = NULL;
  jhcAliaChain *ch;
  UL32 start = 0;
  int i, neg, cyc, n = NumFoci();

  // no match found yet
  if (src != NULL)
    *src = NULL;
  if (d2a != NULL)
    d2a->Clear();

  // make sure pattern to match is not negated 
  if (main == NULL)
    return NULL;
  if ((neg = main->Neg()) > 0)
    main->SetNeg(0);
  if (fresh > 0.0)
    start = jms_now() - ROUND(1000.0 * fresh);

  // look through all foci for most recently started match
  cyc = ++req;
  for (i = 0; i < n; i++)
    if ((ch = FocusN(i)) != NULL)
      ch->FindCall(&act, src, d2a, desc, start, done, NULL, cyc);

  // restore any description negation 
  if (neg > 0)                         
    main->SetNeg(1);
  return act;
}


//= Find directive responsible for failure of current focus.

const jhcAliaDir *jhcActionTree::FindFail () const
{
  const jhcAliaChain *step = Current();
  const jhcAliaPlay *ward = NULL;

  // find relevant main play - assumes top level with only continuations
  while ((step = step->cont) != NULL)
    if ((ward = step->GetPlay()) != NULL)
      if (ward->Overall() < 0)
        break;

  // find directive within failed main play
  if (step != NULL)
    return play_prob(ward);
  return NULL;
}


//= Find first failing activity within a play.

const jhcAliaDir *jhcActionTree::play_prob (const jhcAliaPlay *play) const
{
  jhcAliaChain *seq;
  const jhcAliaDir *dir;
  int i, n = play->NumReq();

  // look for failures in main activities
  for (i = 0; i < n; i++)
    if (play->ReqStatus(i) < 0)
      return failed_dir(play->ReqN(i));

  // look for first finished guard activity
  n = play->NumSimul();
  for (i = 0; i < n; i++)
    if (play->SimulStatus(i) != 0)     
    {
      // prefer a failed step
      seq = play->SimulN(i);
      if ((dir = failed_dir(seq)) != NULL)
        return dir;

      // otherwise blame last thing done
      while (seq != NULL)
      {
        seq = seq->Last();
        if ((dir = seq->GetDir()) != NULL)
          return dir;
        seq = (seq->GetPlay())->ReqN(0);
      }
    }
  return NULL;    
}


//= Find first failing directive in given sequence.

const jhcAliaDir *jhcActionTree::failed_dir (jhcAliaChain *start) const
{
  jhcAliaChain *step = start;
  const jhcAliaDir *dir;
  int v, cyc = start->LastReq() + 1;

  // follow actual execution path based on saved verdicts
  while (step != NULL)
  {
    if (step->LastReq() == cyc)                  // detect looping
      return NULL;
    step->SetReq(cyc);
    if ((v = step->Verdict()) < 0)            
    {
      // if BIND/FIND retry fails, advance to following action
      if ((step->cont == NULL) || ((step->cont)->Verdict() == 0))
      {
        if ((dir = step->GetDir()) != NULL)
          return dir;
        return play_prob(step->GetPlay());
      }
    }
    else if (v == 0)                             // step still running
      return NULL;
    step = ((v == 2) ? step->alt : step->cont);
  }
  return NULL;
} 


///////////////////////////////////////////////////////////////////////////
//                           External Interface                          //
///////////////////////////////////////////////////////////////////////////

//= Open up a potential top level focus NOTE directive for construction.
// can call MakeNode, AddProp, and AddLex to fill in, call FinishNote at end

void jhcActionTree::StartNote ()
{
  nkey.Clear();
  BuildIn(nkey);
}


//= Find node in main memory that matches description so far.
// typically used to find or make some part of the robot, such as its arm
// if equivalent exists then description is erased, else focus node is returned

jhcAliaDesc *jhcActionTree::Resolve (jhcAliaDesc *focus)
{
  jhcSituation sit;
  jhcBindings b;
  int mc = 1;

  MaxBand(0);
  b.expect = nkey.NumItems();
  if (sit.MatchGraph(&b, mc, nkey, *this) <= 0)
    return focus;
  nkey.Clear();
  return b.LookUp(dynamic_cast<jhcNetNode *>(focus));
}


//= Make sure node is visible and non-hypotheical then mark it as eligible for FIND.
// needed when new node is volunteered by grounding kernel outside of NOTE

void jhcActionTree::NewFound (jhcAliaDesc *obj) const 
{
  jhcNetNode *item = dynamic_cast<jhcNetNode *>(obj);

  item->SetBelief(1.0);
  item->Reveal();
  SetGen(item);
}


//= Add current note as a focus, possibly marking some part as the main error.
// since depth-first subroutine calls per focus only one "blame" var needed
// return number of focus if added, -2 if empty

int jhcActionTree::FinishNote (jhcAliaDesc *fail)
{
  jhcAliaChain *ch0;
  jhcAliaDir *d0;
  int ans = -2;

  // make sure something was started then rearrange if needed (could add)
  if (nkey.Empty())
    return ans;
//  if (fail != NULL)
//    nkey.SetMain(dynamic_cast<jhcNetNode *>(fail));
  nkey.MainProp();           
  
  // possibly set failure message for top level focus
  if ((fail != NULL) && (svc >= 0) && (svc < fill))
    if (blame > 0)
    {
      jprintf(1, noisy, "Recording main task failure reason:\n");
      err[svc].Copy(nkey);
    }

  // add NOTE as new attentional focus
  ch0 = new jhcAliaChain;
  d0 = new jhcAliaDir;
  (d0->key).Copy(nkey);
  ch0->BindDir(d0);
  ans = AddFocus(ch0);

  // general cleanup
  BuildIn(NULL);
  nkey.Clear();
  return ans;
}


///////////////////////////////////////////////////////////////////////////
//                             File Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Read a list of focal elements from a file.
// appends to end of existing activities if app > 0
// returns number of top level foci added (not total agenda length)

int jhcActionTree::LoadFoci (const char *fname, int app)
{
  jhcTxtLine in;
  jhcAliaChain *f;
  int yack = noisy, ans = 1, n = 0;

  // possibly clear old stuff then try to open file
  if (app <= 0)
    ClrFoci();
  if (!in.Open(fname))
    return -1;
  ClrTrans();

  // try reading chains from file  
  noisy = 0;
  while (ans >= 0)
  {
    // make and load a new chain, 
    f = new jhcAliaChain;
    if ((ans = f->Load(*this, in, 0)) <= 0)
    {
      // delete and purge input if parse error 
      if (!in.End())
        jprintf(">>> Bad syntax at line %d in: %s\n", in.Last(), fname);
      delete f;
      if (in.NextBlank() == NULL)
        break;
    }
    else if (AddFocus(f) < 0)
    {
      // if buffer full then delete chain and quit
      delete f;
      break;
    }
    else
      n++;         // successfully added
  }

  // restoring debugging printout
  ClrTrans(0);
  noisy = yack;
  return n;
}


//= Save all current focal items to a file.
// return: 1 = successful, 0 = error

int jhcActionTree::SaveFoci (const char *fname)
{
  FILE *out;
  int ans;

  if (fopen_s(&out, fname, "w") != 0)
    return 0;
  ans = SaveFoci(out);
  fclose(out);
  return ans;
}


//= Save self out in machine readable form to current position in a file.
// lists items from current top priority down to lowest priority
// returns number of foci listed

int jhcActionTree::SaveFoci (FILE *out)
{
  char age[40];
  jhcAliaChain *s;
  int win, n = 0;

  // go through foci in priority order
  while ((win = NextFocus()) >= 0)
  {
    // report number and stats on focus
    if (active[win] == 0)
      strcpy_s(age, "new");
    else
      sprintf_s(age, "age = %5.3f", jms_secs(now, active[win]));
    jfprintf(out, "// FOCUS %d: imp = %d, %s\n", n + 1, wt[win], age);

    // dump contents of focus
    s = FocusN(win);
    if (s->Save(out, 0, NULL, 2) > 0)
      n++;
    jfprintf(out, "\n");
  }
  jfprintf(out, "\n");
  return n;
}


