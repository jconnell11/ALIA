// jhcAliaAttn.cpp : holds attentional foci for ALIA system
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

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"

#include "Parse/jhcTxtLine.h"          // common audio

#include "Action/jhcAliaDir.h"         // common robot - since only spec'd as class in header

#include "Reasoning/jhcAliaAttn.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// NOTE: deletes all plays and attached elements

jhcAliaAttn::~jhcAliaAttn ()
{
  ClrFoci(0);
}


//= Default constructor initializes certain values.

jhcAliaAttn::jhcAliaAttn ()
{
  int i;

  for (i = 0; i < imax; i++)
    focus[i] = NULL;
  fill = 0;
  chock = 0;
  svc = -1;
  now = 0;
  ch0 = NULL;
  dir0 = NULL;
  self = NULL;
  user = NULL;
  noisy = 3;
}


//= Tells how many foci are still active (omits ones which are done).

int jhcAliaAttn::Active () const
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

int jhcAliaAttn::NextFocus ()
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

jhcAliaChain *jhcAliaAttn::FocusN (int n) const
{
  if ((n < 0) || (n >= fill))
    return NULL;
  return focus[n];
}


//= Tells whether the chain has been started yet or not.

bool jhcAliaAttn::NeverRun (int n) const
{
  if ((n < 0) || (n >= fill))
    return false;
  return((done[n] <= 0) && (active[n] == 0));
}


//= Gives a priority for actions connected to this focus.
// mostly based on weight with a slight boost for recency

int jhcAliaAttn::BaseBid (int n) const 
{
  if ((n < 0) || (n >= fill))
    return 0;
  return(ROUND(1000.0 * wt[n]) + boost[n]);
}


//= Mark the given focus as active or finished at the current time.
// this is typically when the chain verdict is zero

void jhcAliaAttn::SetActive (int n, int running)
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

int jhcAliaAttn::ServiceWt (double pref)
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

int jhcAliaAttn::ClrFoci (int init, const char *rname)
{
  char first[80];
  char *sep;
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
  user = NULL;
  self = NULL;

  // make nodes for participants in conversation
  self = MakeNode("self", "you", 0, -1.0);
  AddProp(self, "ako", "person", 0, -1.0);
  if ((rname != NULL) && (*rname != '\0'))
  {
    // copy robot's name (if any) to "self" node
    AddLex(self, rname, 0, -1.0);
    strcpy_s(first, rname);
    if ((sep = strchr(first, ' ')) != NULL)
    {
      *sep = '\0';
      AddLex(self, first, 0, -1.0);
    }
  }
  ShiftUser();
  return 1;
}


//= Find an old user with the given name or make up a new one.
// useful when face recognition notices a change
// returns value of "user" member variable for convenience

jhcNetNode *jhcAliaAttn::ShiftUser (const char *name)
{
  jhcNetNode *n = NULL;

  // see if named user already exists
  if ((name != NULL) && (*name != '\0'))
    while ((n = NextNode(n)) != NULL)
      if (n->HasWord(name, 1))
        return SetUser(n);

  // make up a new user node and add pronouns
  n = MakeNode("dude", NULL, 0, -1.0);
  if ((name != NULL) && (*name != '\0'))
    AddLex(n, name, 0, -1.0);
  return SetUser(n);
}


//= Force user to be some existing node.

jhcNetNode *jhcAliaAttn::SetUser (jhcNetNode *n)
{
  // reassign "I" and "me" to new node
  set_prons(0);
  user = n;
  set_prons(1);

  // make sure that personhood is marked
  AddProp(user, "ako", "person", 0, -1.0);
  AddProp(user, "ako", "user", 0, -1.0);
  return user;
}


//= Set reference words "I" and "me" of current user to some negation state.

void jhcAliaAttn::set_prons (int tru) 
{
  const char *word;
  int i, np;

  // sanity check
  if (user == NULL)
    return;

  // change negation value of first person pronouns (if they exist)
  np = user->NumProps();
  for (i = 0; i < np; i++)
    if ((word = user->LexBase(i)) != NULL)
      if ((strcmp(word, "I") == 0) || (strcmp(word, "me") == 0))
        (user->Prop(i))->SetNeg((tru > 0) ? 0 : 1);

  // if asserting, make sure first person pronouns exist
  if (tru <= 0)
    return;
  if (!user->HasWord("I"))
    AddLex(user, "I", 0, -1.0);
  if (!user->HasWord("me"))
    AddLex(user, "me", 0, -1.0);
}


//= Add item onto end of agenda list with some current importance.
// returns index of item if added, -1 if array is already full (item NOT deleted)
// NOTE: item will be deleted by this class if accepted (don't keep a pointer)

int jhcAliaAttn::AddFocus (jhcAliaChain *f, double pref)
{
  jhcAliaDir *d;
  int i;

  // sanity check
  if (fill >= imax)
  {
    jprintf(">>> More than %d foci in jhcAliaAttn::AddFocus !\n", imax);
    return -1;
  }

  // possibly announce action
  if (noisy >= 1)
  {    
    jprintf("---------------------------------\n");
    jprintf(">>> New focus %d:\n\n", fill);
    f->Print(2);
    jprintf("\n---------------------------------\n\n");
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


//= Promote result of any halo rules used to main memory and create NOTEs.
// alters original bindings so they reflect new main nodes
// returns number of NOTEs generated (rules used)

int jhcAliaAttn::ReifyRules (jhcBindings& b)
{
  jhcBindings h2m;
  int i, src, bcnt = b.NumPairs(), add = 0;

  // check for bindings involving a halo node
  for (i = 0; i < bcnt; i++)
    if ((src = (b.GetSub(i))->pod) > 0)
    {
      PromoteHalo(h2m, src);
      add++;
    }
  return add;
}


///////////////////////////////////////////////////////////////////////////
//                             External Interface                        //
///////////////////////////////////////////////////////////////////////////

//= Open up a potential top level focus NOTE directive for construction.
// can call MakeNode, AddProp, and AddLex to fill in, call FinishNote at end

void jhcAliaAttn::StartNote ()
{
  FinishNote(0);
  ch0 = new jhcAliaChain;
  dir0 = new jhcAliaDir;
  ch0->BindDir(dir0);
  BuildIn(&(dir0->key));
}


//= Add current note as a focus or delete it.
// return number of focus if added, -2 if deleted

int jhcAliaAttn::FinishNote (int keep)
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
//                               Maintenance                             //
///////////////////////////////////////////////////////////////////////////

//= Discards old foci, removes unused nodes, and enforces local consistency.
// must mark all seed nodes to retain before calling with gc > 0
// returns 1 if working memory has changed, 0 if same as last cycle

int jhcAliaAttn::Update (int gc)
{
  prune_foci();
  fluent_scan(0);
  if (gc > 0)
    clean_mem();
  ver++;                         // increase generation count
  return(Changed() ? 1 : 0);
}


//= Remove any expired items from list based on the current time.
// only keeps semantic network nodes attached to foci or active directives
// returns number of foci currently in list (for NextFocus)

int jhcAliaAttn::prune_foci ()
{
  int i = 0, ms = 30000;         // default 30 sec

  // remember cycle start time and remove expired foci
  now = jms_now();
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
  jprintf(3, noisy, "FOCI: %d active (%d inactive)\n\n", Active(), Inactive());
  chock = fill;
  return chock;
}


//= Remove particular item and re-compact list.

void jhcAliaAttn::rem_compact (int n)
{
  int i;

  // remove item from list
  if (noisy >= 2)
  {
    jprintf("--------------------------------------\n");
    jprintf(">>> Removing focus %d (%3.1f secs)\n\n", n, jms_secs(now, active[n]));
    focus[n]->Print(2);
    jprintf("\n--------------------------------------\n\n");
  }
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


//= Looks for change in truth values of predicates.
// sets belief of older version to zero in favor of newer version
// ignores negation when checking for equality to allow truth value flip
// returns number of predicates invalidated

int jhcAliaAttn::fluent_scan (int dbg)
{
  jhcNetNode *n2, *n = NULL;
  int cnt = 0;

  // look only at predicate nodes changed this cycle (might not be at head)
  while ((n = NextNode(n)) != NULL)
    if (Recent(n) && !n->Hyp() && !n->ObjNode())
    {
      // scan through all older predicates for any matches (might be earlier in list)
      n2 = NULL;
      while ((n2 = NextNode(n2)) != NULL)
        if (!Recent(n2) && !n2->Hyp() && !n2->ObjNode())
          if (n->SameArgs(n2) && (n->LexMatch(n2) || n->SharedWord(n2)))
          {
            if (cnt++ <= 0)
              jprintf(1, dbg, "Fluent scan:\n");
            jprintf(1, dbg, "  %s overrides %s\n", n->Nick(), n2->Nick());
            n2->SetBelief(0.0);
          }
    }
  if (cnt > 0)
    jprintf(1, dbg, "\n");
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                          Garbage Collection                           //
///////////////////////////////////////////////////////////////////////////

//= Keep only semantic network nodes attached to foci or active directives.
// generally additional seeds will have been marked by other components
// returns number of nodes removed

int jhcAliaAttn::clean_mem ()
{
  jhcNetNode *n = NULL;
  int i, dbg = ((noisy >= 5) ? 1 : 0);

  // all things are potential garbage
  jprintf(1, dbg, "\nCleaning memory ...\n");
  while ((n = NextNode(n)) != NULL)
    n->keep = ((n->keep > 0) ? 1 : 0);           // normalize values

  // mark definite keepers              
  for (i = 0; i < fill; i++)
    focus[i]->MarkSeeds();       
  if (self != NULL)
    self->keep = 1;
  if (user != NULL)
    user->keep = 1;         

  // scan all and expand marks to related nodes
  jprintf(1, dbg, "\n  retaining nodes:\n");
  n = NULL;
  while ((n = Next(n)) != NULL)
    if (n->keep == 1)
      keep_from(n, dbg);
  return rem_unmarked(dbg);
}


//= Mark this particular node and all things connected to it as non-garbage.
// generally external marks are 1 and these spreads marks are 2

void jhcAliaAttn::keep_from (jhcNetNode *anchor, int dbg)
{
  jhcNetNode *p;
  int i, n;

  // make sure node is not already marked or part of some other pool
  if ((anchor == NULL) || (anchor->keep > 1) || !InPool(anchor))
    return;
  jprintf(1, dbg, "    %s%s\n", ((anchor->keep <= 0) ? "  " : ""), anchor->Nick());  

  // mark node and all its arguments as being keepers 
  anchor->keep = 2;
  n = anchor->NumArgs();  
  for (i = 0; i < n; i++)
    keep_from(anchor->Arg(i), dbg);

  // mark most properties for retention
  n = anchor->NumProps();  
  for (i = 0; i < n; i++)
  {
    // skip if user speech act (might be marked from focus anyhow)
    p = anchor->Prop(i);
    if (strcmp(p->Kind(), "meta") == 0) 
      continue;

    // skip if no belief and no properties depend on it
    if ((p->Belief() == 0.0) && (p->NonLexCnt() == 0))
      continue;
    keep_from(p, dbg);
  }
}


//= Eliminate all facts not connected to marked active nodes.
// expects something else to mark some nodes as non-zero
// after GC all remaining nodes left in unmarked (0) state 
// returns number of nodes removed

int jhcAliaAttn::rem_unmarked (int dbg)
{
  jhcNetNode *tail, *n = Next(NULL);
  int cnt = 0;

  // get rid of anything not marked (0)
  while (n != NULL)
    if (n->keep > 0)
    {
      n->keep = 0;           // eligible on next round
      n = Next(n);
    }
    else
    {
      if (cnt++ <= 0)
        jprintf(1, dbg, "\n  forgetting nodes:\n");
      jprintf(1, dbg, "    %s\n", n->Nick());
      tail = Next(n);
      RemNode(n);
      n = tail;
   }

  // done
  if (cnt > 0)
    jprintf(1, dbg, "\n");
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                             File Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Read a list of focal elements from a file.
// appends to end of existing activities if add > 0
// returns number of top level foci added (not total agenda length)

int jhcAliaAttn::LoadFoci (const char *fname, int add)
{
  jhcTxtLine in;
  jhcAliaChain *f;
  int yack = noisy, ans = 1, n = 0;

  // possibly clear old stuff then try to open file
  if (add <= 0)
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

int jhcAliaAttn::SaveFoci (const char *fname)
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

int jhcAliaAttn::SaveFoci (FILE *out)
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


