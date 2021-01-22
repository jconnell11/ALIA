// jhcWorkMem.cpp : main temporary semantic network for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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


#include "Action/jhcAliaDir.h"         // common robot

#include "Interface/jms_x.h"           // common video

#include "Reasoning/jhcAliaRule.h"     // common audio - since only spec'd as class in header

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
  // halo control
  halo.NegID();
  nimbus = 0;
  mode = 0;

  // conversational agents
  self = NULL;
  user = NULL;
  InitPeople(NULL);

  // fact belief threshold
  skep = 0.5;           
}


///////////////////////////////////////////////////////////////////////////
//                       Conversation Participants                       //
///////////////////////////////////////////////////////////////////////////

//= Set up definitions of "self" and "user" in semantic net.

void jhcWorkMem::InitPeople (const char *rname)
{
  char first[80];
  char *sep;

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
}


//= Find an old user with the given name or make up a new one.
// useful when face recognition notices a change
// returns value of "user" member variable for convenience

jhcNetNode *jhcWorkMem::ShiftUser (const char *name)
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

jhcNetNode *jhcWorkMem::SetUser (jhcNetNode *n)
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

void jhcWorkMem::set_prons (int tru) 
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


///////////////////////////////////////////////////////////////////////////
//                              List Access                              //
///////////////////////////////////////////////////////////////////////////

//= Returns next node in list, transitioning from main to halo-1 to halo-2 if needed.
// call with prev = NULL to get first node, use SetMode(2) to include halo
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
    {
      n = halo.Pool();
      if (mode == 1) 
        while (n != NULL) 
        {
          // skip high-numbered nodes if only 1-step inference allowed
          if (abs(n->Inst()) <= nimbus)
            break;
          n = Next(n);
        }
    }
  return n;
}


//= Tell if a node comes from currently valid section of memory.
// useful when matching against just main, main plus halo-1, or everything
// makes sure jhcSituation::try_props and try_args remain in correct region

bool jhcWorkMem::Prohibited (const jhcNetNode *n) const
{
  return((n == NULL) || 
         ((mode <= 0) && (n->Inst() < 0)) || 
         ((mode == 1) && (n->Inst() < -nimbus)));
}


///////////////////////////////////////////////////////////////////////////
//                            Halo Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Make up network in halo for some pattern (typically result of rule).

void jhcWorkMem::AssertHalo (const jhcGraphlet& pat, jhcBindings& b, jhcAliaRule *r)
{
  jhcNetNode *n;
  const jhcNetNode *pn;
  int i, nb, nb0 = b.NumPairs(), tval = 0;

  // make up new halo nodes for result (and record source NOTE)
  for (i = 0; i < nb0; i++)
  {
    n = b.GetSub(i);
    tval = __max(tval, n->top);                  
  }
  halo.Assert(pat, b, 0.0, tval);    

  // set initial beliefs and record provenance for new parts
  nb = b.NumPairs();
  for (i = nb0; i < nb; i++)
  {
    pn = b.GetKey(i);
    n = b.GetSub(i);
    if (n->Halo())
    {
      n->SetBelief(pn->Belief());
      n->hrule = r;
      n->hbind = &b;
    }
  }
}


//= Find the version of a halo node with the highest belief and invalidate all others.
// if run incrementally, first non-zero match found should be the only one 
// keeps the version (NEG or positive) with the highest belief, others all zeroed
// similar to Endorse but does always defer to the most recent assertion

void jhcWorkMem::MaxHalo (jhcNetNode *n)
{
  jhcNetNode *n2 = NULL;
  double b;

  // ignore object and word nodes
  if ((n == NULL) || !n->Halo() || n->Hyp() || n->ObjNode())
    return;
  b = n->Belief();

  // scan through all others for first match with positive belief
  while ((n2 = halo.Next(n2)) != NULL)
    if ((n2 != n) && !n2->Hyp() && !n2->ObjNode())
      if (n->SameArgs(n2) && (n->LexMatch(n2) || n->SharedWord(n2)))
      {
jprintf("MaxHalo: similar %s (%4.2f) and %s (%4.2f)\n", n->Nick(), b, n2->Nick(), n2->Belief()); 
        // negate belief for any fact with same sense or opposite sense (neg)
        // prevents rule matching (node removal could leave dangling pointers)
        if (b > n2->Belief())
          n2->Hide();
        else
          n->Hide();                   // earlier wins if tied
        break;
      }
}


//= Make suitably connected main memory node for each halo node in bindings.
// saves mapping of correspondences between halo nodes and replacements in "h2m"
// does not change original rule bindings "b" (still contains halo references)
// used with jhcActionTree::ReifyRules

void jhcWorkMem::PromoteHalo (jhcBindings& h2m, const jhcBindings& b)
{
  jhcBindings b2;
  jhcNetNode *n2, *n;
  const jhcNetNode *n0;
  int i, j, hcnt, narg, nb = b.NumPairs(), h0 = h2m.NumPairs();

  // make a main node for each halo node using same lex and neg
  BuildIn(NULL);
  b2.CopyReplace(b, h2m);              // subs from earlier promotions
  for (i = 0; i < nb; i++)
  {
    // see if this substitution is a halo node
    n = b2.GetSub(i);
    if (!n->Halo())
      continue;

    // make equivalent wmem node with proper generation
    n2 = MakeNode(n->Kind());     
    n2->SetNeg(n->Neg());
    n2->SetDone(n->Done());

    // actualize result and save translation
    n2->SetBelief(n->Default());  
    h2m.Bind(n, n2);
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
      n2 = n0->Arg(j);
      if (n2->Inst() < 0)
        n2 = h2m.LookUp(n2);         // should never fail
      n->AddArg(n0->Slot(j), n2);
    } 
  }
}


///////////////////////////////////////////////////////////////////////////
//                           Garbage Collection                          //
///////////////////////////////////////////////////////////////////////////

//= Resolve any contradictions and possibly remove old nodes from main memory.
// keeps only semantic network nodes attached to foci or active directives.
// generally additional seeds will have been marked by other components
// returns number of nodes removed

int jhcWorkMem::CleanMem (int dbg)
{
  jhcNetNode *self = Robot(), *user = Human(), *n = NULL;

  // all things are potential garbage
  jprintf(1, dbg, "\nCleaning memory ...\n");
  while ((n = NextNode(n)) != NULL)
    n->keep = ((n->keep > 0) ? 1 : 0);           // normalize values

  // mark definite keepers              
  if (self != NULL)
    self->keep = 1;
  if (user != NULL)
    user->keep = 1;         

  // scan all and expand marks to related nodes
  jprintf(2, dbg, "\n  retaining nodes:\n");
  n = NULL;
  while ((n = Next(n)) != NULL)
    if (n->keep == 1)
      keep_from(n, dbg);
  return rem_unmarked(dbg);
}


//= Mark this particular node and all things connected to it as non-garbage.
// generally external marks are 1 and these spreads marks are 2

void jhcWorkMem::keep_from (jhcNetNode *anchor, int dbg)
{
  jhcNetNode *p;
  int i, n;

  // make sure node is not already marked or part of some other pool
  if ((anchor == NULL) || (anchor->keep > 1) || !InPool(anchor))
    return;
  jprintf(2, dbg, "    %s%s\n", ((anchor->keep <= 0) ? "  " : ""), anchor->Nick());  

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

int jhcWorkMem::rem_unmarked (int dbg)
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
        jprintf(1, dbg, "\n  FORGETTING nodes:\n");
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
//                           Truth Maintenance                           //
///////////////////////////////////////////////////////////////////////////
  
//= Override beliefs of any older main memory versions of nodes in description.
// sets belief of older versions negative in favor of newer version
// ignores sense negation when checking for equality to allow truth value flip
// ensures only one variant of predicate will have belief > 0 (generally most recent)
// if run incrementally, first non-zero match found should be the only one 
// similar to MaxHalo but current assertions always override previous ones

void jhcWorkMem::Endorse (const jhcGraphlet& desc, int dbg)
{
  const jhcNetNode *n;
  jhcNetNode *n2;
  int i, ni = desc.NumItems(), cnt = 0;

  // look through all description items (except object nodes and words)
  for (i = 0; i < ni; i++)
  {
    n = desc.Item(i);
    if (!n->Hyp() && !n->ObjNode())
    {
      // scan through all others for first match with non-zero belief
      n2 = NULL;
      while ((n2 = Next(n2)) != NULL)
        if ((n2 != n) && !n2->Hyp() && !n2->ObjNode())
          if (n->SameArgs(n2) && (n->LexMatch(n2) || n->SharedWord(n2)))
          {
            // negate belief for any fact with same sense or opposite sense (neg)
            // prevents rule matching (node removal could leave dangling pointers)
            if (cnt++ <= 0)
              jprintf(1, dbg, "Endorse:\n");
            jprintf(1, dbg, "  %s overrides %s\n", n->Nick(), n2->Nick());
            n2->Hide();
            break;
          }
    }
  }

  // mark whether any changes have occurred (mostly for halo recomputation)
  if (cnt > 0)
    jprintf(1, dbg, "\n");
  Dirty(cnt);
}

