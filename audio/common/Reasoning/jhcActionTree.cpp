// jhcActionTree.cpp : holds attentional foci for ALIA system
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

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"

#include "Parse/jhcTxtLine.h"          // common audio
#include "Reasoning/jhcAliaRule.h"

#include "Action/jhcAliaDir.h"         // common robot - since only spec'd as class in header

#include "Reasoning/jhcActionTree.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// NOTE: deletes all plays and attached elements

jhcActionTree::~jhcActionTree ()
{
  ClrFoci(0);
}


//= Default constructor initializes certain values.

jhcActionTree::jhcActionTree ()
{
  int i;

  // surprise parameters
  drill = 1.3;         // threshold for explict surprise NOTE
  dwell = 5.0;         // obsession with contradiction (sec)
  calm = 1.0;          // standard surprise decay (sec)

  // rule adjustment parameters
  binc = 0.1;          
  bdec = 0.1;

  // parallel action status
  for (i = 0; i < imax; i++)
    focus[i] = NULL;
  fill = 0;
  chock = 0;
  svc = -1;
  now = 0;

  // external interface
  ch0 = NULL;
  dir0 = NULL;

  // mood default
  surp = 0.0;

  // debugging
  noisy = 3;           // usually copied from jhcAliaCore (=1)
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


///////////////////////////////////////////////////////////////////////////
//                          List Manipulation                            //
///////////////////////////////////////////////////////////////////////////

//= Returns the index of the next newest foci (negative when done).
// searches down from end of list (as recorded at beginning of cycle)
// foci added on this cycle are not serviced, removal only at end of cycle 

int jhcActionTree::NextFocus ()
{
  int i, nf = chock;

  // find next unselected active focus (tail first)
  for (i = nf - 1; i >= 0; i--)
    if ((mark[i] <= 0) && (done[i] <= 0))
      break;

  // mark focus as used
  if (i >= 0)
    mark[i] = 1;
  svc = i;
  return svc;
}


//= Get a pointer to a particular item in the list.

jhcAliaChain *jhcActionTree::FocusN (int n) const
{
  if ((n < 0) || (n >= fill))
    return NULL;
  return focus[n];
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

void jhcActionTree::SetActive (int n, int running)
{
  if ((n < 0) || (n >= fill))
    return;
  if (running > 0)
    active[n] = now;
  else
    done[n] = 1;
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

//= Removes all items from attention list.
// can optionally clear memory and make new "self" and "user" nodes
// always returns 1 for convenience

int jhcActionTree::ClrFoci (int init, const char *rname)
{
  int i;

  // clear focal items
  for (i = 0; i < fill; i++)
  {
    delete focus[i];
    focus[i] = NULL;
  }
  fill = 0;
  chock = 0;
  svc = -1;
  if (init <= 0)
    return 1;

  // clear halo and working memory
  FinishNote(0);
  ClearHalo();
  PurgeAll();
  InitPeople(rname);
  return 1;
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
  UL32 t;
  int i, age, worst, drop = -1;

  // find complete focus that has been around longest
  t = jms_now();
  for (i = 0; i < fill; i++)
    if (done[i] > 0)
    {
      age = jms_diff(t, active[i]);
      if ((drop < 0) || (age > worst))
      {
        worst = age;
        drop = i;
      }
    }

  // try to remove the focus
  if (drop < 0)
    return 0;
  rem_compact(i);
  return 1;
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
  UL32 prev = now;
  double dt;
  int i;

  // find current elapsed time and decay surprise value
  now = jms_now();
  dt = jms_secs(now, prev);
  if (dt > 0.0)
    surp -= surp * calm * dt;

  // clean up old foci and old nodes in main memory
  prune_foci();
  for (i = 0; i < fill; i++)
    focus[i]->MarkSeeds();             // make sure to keep these
  if (gc > 0)     
    CleanMem(noisy - 4);

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

  // copy relevant part of each next focus to this focus
  for (i = n; i < fill; i++)
  {
    // status
    focus[i] = focus[i + 1];
    done[i]  = done[i + 1];
    mark[i]  = mark[i + 1];

    // importance
    wt[i]    = wt[i + 1];
    boost[i] = boost[i + 1];

    // timing
    active[i] = active[i + 1];
  }

  // shrink list size
  fill--;
}


///////////////////////////////////////////////////////////////////////////
//                             Halo Interaction                          //
///////////////////////////////////////////////////////////////////////////

//= Determines how unexpected some situation is relative to halo expectation.
// in jhcActionTree not jhcWorkMem because it can add new NOTE foci
// returns fascination time for corresponding NOTE directive (in secs)

double jhcActionTree::CompareHalo (const jhcGraphlet& key)
{
  jhcAliaDesc *evt;
  const jhcNetNode *mate;
  jhcNetNode *focus, *fact;
  const jhcAliaRule *r;
  const jhcBindings *b;
  double s, blf, blf2, blf3, lo, hi = 0.0;
  int i, ni = key.NumItems();

  // examine each element of the situation 
  for (i = 0; i < ni; i++)
  {
    // compare against ALL matching predictions in halo
    focus = key.Item(i);
    blf = focus->Belief();
    mate = NULL;
    lo = -1.0;
    while ((mate = halo_equiv(focus, mate)) != NULL)
    {
      // possibly tune relevant rule's output belief for this prediction
      blf2 = mate->Belief();
      if (((r = mate->hrule) != NULL) && ((b = mate->hbind) != NULL))
      {
        fact = r->Wash(b->FindKey(mate));
        if ((blf2 >= MinBlf()) && (mate->Neg() != focus->Neg()))
          fact->SetBelief(blf2 - bdec);
        else if ((blf2 < MinBlf()) && (mate->Neg() == focus->Neg()))
          fact->SetBelief(blf2 + binc);
        if ((blf3 = fact->Belief()) != blf2)
          jprintf(1, noisy, "ADJUST: rule %d --> %s %s to %4.2f\n", r->RuleNum(), 
                  ((blf3 > blf2) ? "raise" : "lower"), fact->Nick(), blf3);
      }

      // calculate change of belief relative to selected halo prediction
      if (focus->Neg() == mate->Neg())
        s = fabs(blf - blf2);                        // expected to some degree
      else
        s = blf + blf2;                              // contradiction
      if (lo < 0.0)
        lo = s;
      else
        lo = __min(s, lo);                           // unexpectedness of element
    }

    // possibly generate a NOTE about this fact if highly unexpected
    if (lo > drill)
    {
      StartNote();
      evt = NewNode("act", "surprise", 0, 1.0);      // not "done"
      AddArg(evt, "agt", focus);
      AddArg(evt, "obj", Self());
      FinishNote();
    }
    hi = __max(lo, hi);                              // combine across whole key
  }

  // alter global "surprise" to affect things like facial expression
  surp = __max(surp, hi);
  return(0.5 * dwell * surp);                        // also boosts later NOTEs
}


//= Look through halo to find some node with similar arguments.

const jhcNetNode *jhcActionTree::halo_equiv (const jhcNetNode *n, const jhcNetNode *h0) const
{
  const jhcNetNode *h = h0;
  int i, na = n->NumArgs();

  // look for superficially similar node in halo portion
  while ((h = halo.Next(h)) != NULL)
    if ((h->Belief() > 0.0) && (h->Done() == n->Done()) && 
        (h->NumArgs() == na) && !h->LexConflict(n) && h->SameWords(n))
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
// returns number of NOTEs generated (rules used)

int jhcActionTree::ReifyRules (jhcBindings& b, int note)
{
  jhcBindings h2m, b2;
  jhcAliaChain *ch;
  jhcAliaDir *dir;
  jhcNetNode *item;
  int fcnt = 0;

  // progressively find all halo nodes that need to move to wmem
  while ((item = pick_halo(b, h2m, 0)) != NULL)
  {
    // move halo nodes in precondition of rule that yielded this item
    PromoteHalo(h2m, item->hbind);
    b.ReplaceSubs(h2m);
    if (note <= 0)
      continue;

    // instantiate result of rule using main memory nodes
    dir = new jhcAliaDir;
    b2.CopyReplace(item->hbind, h2m);
    (item->hrule)->Inferred(dir->key, b2);

    // embed NOTE in chain and add to attention queue
    ch = new jhcAliaChain;
    ch->BindDir(dir);
    AddFocus(ch);
    fcnt++;
  }
  return fcnt;
}


//= Find some halo fact that needs to be moved to working memory.
// assumes original bindings "b" already augmented by halo migrations in "h2m"
// returns NULL when all substitutions are in wmem 

jhcNetNode *jhcActionTree::pick_halo (const jhcBindings& b, const jhcBindings& h2m, int stop) const
{
  jhcBindings b2;
  jhcNetNode *sub, *deep;
  int i, j, lex, bcnt = b.NumPairs();

  // look for halo nodes among binding substitutions
  if (stop > 0)
    bcnt = __min(stop, bcnt);
  for (j = 0; j <= 1; j++)
    for (i = 0; i < bcnt; i++)
    {
      // try basic nodes first, lexical nodes later (if any left)
      sub = b.GetSub(i);
      if (!sub->Halo())
        continue;
      lex = ((sub->LexNode()) ? 1 : 0);
      if (lex != j)
        continue;

      // check if source rule itself used any halo facts to trigger
      if (stop <= 0)
      {
        b2.CopyReplace(sub->hbind, h2m);
        if ((deep = pick_halo(b2, h2m, (sub->hrule)->NumPat())) != NULL)
          return deep;
      }
      return sub;
    }
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                             External Interface                        //
///////////////////////////////////////////////////////////////////////////

//= Open up a potential top level focus NOTE directive for construction.
// can call MakeNode, AddProp, and AddLex to fill in, call FinishNote at end

void jhcActionTree::StartNote ()
{
  FinishNote(0);
  ch0 = new jhcAliaChain;
  dir0 = new jhcAliaDir;
  ch0->BindDir(dir0);
  BuildIn(&(dir0->key));
}


//= Add current note as a focus or delete it.
// return number of focus if added, -2 if deleted

int jhcActionTree::FinishNote (int keep)
{
  int ans = -2;

  // rearrange items for nicer look  
  if ((ch0 == NULL) || (dir0 == NULL))
    return ans;
  (dir0->key).MainProp();
  
  // add as focus or abort construction
  if (keep > 0)
    ans = AddFocus(ch0);
  else
    delete ch0;              // automatically deletes dir0 also

  // general cleanup
  BuildIn(NULL);
  ch0 = NULL;
  dir0 = NULL;
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
    ClrFoci(0);
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
        jprintf("Bad syntax at line %d in: %s\n", in.Last(), fname);
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
    if (s->Save(out, 0, NULL, 3) > 0)
      n++;
    jfprintf(out, "\n");
  }
  jfprintf(out, "\n");
  return n;
}


