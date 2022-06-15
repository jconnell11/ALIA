// jhcWorkMem.cpp : main temporary semantic network for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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
  // default eligibility for matching (no graphizer foreshadowing)
  vis0 = 0;

  // enable 20x faster jhcAssocMem::RefreshHalo
  MakeBins();
  halo.MakeBins();

  // halo control
  halo.NegID();
  nimbus = 0;
  mode = 0;

  // conversational agents
  self = NULL;
  user = NULL;
  InitPeople(NULL);
  clr_ext();

  // fact belief threshold
  skep = 0.5;           
}


//= Remove everything in main memory and halo as well as all external links.

void jhcWorkMem::Reset ()
{
  clr_ext();
  PurgeAll();
  ClearHalo();
}


//= Clear out all entries in external item translation array.

void jhcWorkMem::clr_ext ()
{
  int i;

  for (i = 0; i < 20; i++)
  {
    ext[i] = 0;
    nref[i] = NULL;
  }
}


///////////////////////////////////////////////////////////////////////////
//                       Conversation Participants                       //
///////////////////////////////////////////////////////////////////////////

//= Set up definitions of "self" and "user" in semantic net.
// Note: robot first and full name are added to grammars by jhcAliaSpeech::Reset

void jhcWorkMem::InitPeople (const char *rname)
{
  jhcNetNode *p;
  char first[80];
  char *sep;

  // make nodes for participants in conversation
  self = MakeNode("self", "me", 0, -1.0);
  self->Reveal();
  p = AddProp(self, "ako", "robot", 0, -1.0);
  p->Reveal();
  if ((rname != NULL) && (*rname != '\0'))
  {
    // copy robot's name (if any) to "self" node
    p = AddProp(self, "name", rname, 0, -1.0);
    p->Reveal();
    strcpy_s(first, rname);
    if ((sep = strchr(first, ' ')) != NULL)
    {
      *sep = '\0';
      p = AddProp(self, "name", first, 0, -1.0);
      p->Reveal();
    }
  }
  user = NULL;               // any old binding likely deleted
  ShiftUser();
}


//= Find an old user with the given name or make up a new one.
// useful when face recognition notices a change
// returns value of "user" member variable for convenience

jhcNetNode *jhcWorkMem::ShiftUser (const char *name)
{
  jhcNetNode *p, *n = NULL;

  // see if named user already exists
  if ((name != NULL) && (*name != '\0'))
    while ((n = NextNode(n)) != NULL)
      if (n->HasName(name, 1))
        return SetUser(n);

  // make up a new user node and add pronouns
  n = MakeNode("dude", NULL, 0, -1.0);
  n->Reveal();
  if ((name != NULL) && (*name != '\0'))
  {
    p = AddProp(n, "name", name, 0, -1.0);
    p->Reveal();
  }
  return SetUser(n);
}


//= Force user to be some existing node.

jhcNetNode *jhcWorkMem::SetUser (jhcNetNode *n)
{
  jhcNetNode *p;

  // sanity check
  if (n == user)
    return user;

  // reassign "you" to new node
  SetLex(user, "");
  user = n;
  SetLex(user, "you");

  // make sure that personhood is marked
  p = AddProp(user, "ako", "person", 0, -1.0);
  p->Reveal();
  return user;
}


///////////////////////////////////////////////////////////////////////////
//                              List Access                              //
///////////////////////////////////////////////////////////////////////////

//= Returns next node in list, transitioning from main to halo-1 to halo-2 if needed.
// call with prev = NULL to get first node, use SetMode(2) to include halo
// assumes all items in halo node pool have "id" < 0, tries non-halo first
// can restrict selection to just one hash bin, or use all if bin < 0

jhcNetNode *jhcWorkMem::NextNode (const jhcNetNode *prev, int bin) const
{
  jhcNetNode *n = NULL;

  // try all main memory nodes first
  if ((prev == NULL) || (prev->Inst() >= 0))
  {
    if ((n = Next(prev, bin)) != NULL)
      return n;
    if (mode <= 0)               // see if transition to halo allowed
      return NULL;
    n = halo.Pool(bin);          // get candidate node from halo   
  }
  else
    n = halo.Next(prev, bin);    // halo -> get another halo node 

  // skip high-numbered halo nodes if only 1-step inference allowed
  if (mode == 1)
    while (n != NULL) 
    {
      if (abs(n->Inst()) <= nimbus)
        break;
      n = halo.Next(n, bin);     // will try other buckets if bin < 0
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


//= How many potential matches there are with the same hash as reference.
// if reference has no lex, does not return hash = 0 count but TOTAL count
// will be slight over-estimate for halo-1 matching (some halo-2 counted)

int jhcWorkMem::SameBin (const jhcNetNode& focus, const jhcBindings *b) const
{
  int bin;

  // determine appropriate bin(s) for focus node
  if (focus.Lex() == NULL)
    bin = -1;
  else if (b != NULL)
    bin = b->LexBin(focus);
  else 
    bin = focus.Code();

  // get count in main memory and perhaps halo also
  if (mode <= 0)
    return BinCnt(bin);
  return(BinCnt(bin) + halo.BinCnt(bin));  
}


///////////////////////////////////////////////////////////////////////////
//                            Halo Functions                             //
///////////////////////////////////////////////////////////////////////////

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
    // then actualize result as visible and save translation
    n2 = MakeNode(n->Kind(), n->Lex(), n->Neg(), 1.0, n->Done());     
    n2->SetBelief(n->Default());  
    n2->Reveal();
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
      if (n2->Halo())
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
  jhcNetNode *n = NULL;

  // all things are potential garbage
  jprintf(1, dbg, "\nCleaning memory ...\n");
  while ((n = NextNode(n)) != NULL)
    n->keep = ((n->keep > 0) ? 1 : 0);           // normalize values

  // scan all and expand marks to related nodes
  jprintf(2, dbg, "\n  retaining nodes:\n");
  n = NULL;
  while ((n = Next(n)) != NULL)
    if (n->keep == 1)
      keep_from(n, dbg);

  // mark definite keepers (conversation participants)
  keep_party(self);
  keep_party(user);
  return rem_unmarked(dbg);                      // sweep out anything not marked
}


//= Special mark spreader for conversational participants.
// keeps only non-hypothetical HQ, AKO, and REF facts (others may get marked from foci)

void jhcWorkMem::keep_party (jhcNetNode *anchor) const
{
  jhcNetNode *prop, *arg, *deg;
  int i, j, n, n2;

  // definitely keep this node
  if (anchor == NULL)
    return;
  anchor->keep = 2;

  // scan through all its properties
  n = anchor->NumProps();  
  for (i = 0; i < n; i++)
  {
    prop = anchor->Prop(i);
    if (!prop->Hyp() && anchor->RoleIn(i, "name", "ako", "hq"))      
    {
      // keep this property and all arguments
      prop->keep = 2;
      n2 = prop->NumArgs();
      for (j = 0; j < n2; j++)
      {
        arg = prop->Arg(j);
        arg->keep = 1;                 // allow spreading from arg
      }

      // retain degree for properties like "very smart"
      n2 = prop->NumProps();
      for (j = 0; j < n2; j++)
      {
        deg = prop->Prop(j);
        if (!deg->Hyp() && prop->RoleMatch(j, "deg"))
          deg->keep = 2;
      }
    }
  }
}


//= Mark this particular node and all things connected to it as non-garbage.
// generally external marks are 1 and these spreads marks are 2

void jhcWorkMem::keep_from (jhcNetNode *anchor, int dbg) const
{
  int i, n;

  // make sure node is not already marked or part of some other pool
  if ((anchor == NULL) || (anchor->keep > 1) || !InPool(anchor))
    return;
  if ((anchor == self) || (anchor == user))                // handled separately
    return;
  jprintf(2, dbg, "    %s%s\n", ((anchor->keep <= 0) ? "  " : ""), anchor->Nick());  

  // mark node and all its arguments as being keepers 
  anchor->keep = 2;
  n = anchor->NumArgs();  
  for (i = 0; i < n; i++)
    keep_from(anchor->Arg(i), dbg);

  // mark all properties for retention
  n = anchor->NumProps();  
  for (i = 0; i < n; i++)
    keep_from(anchor->Prop(i), dbg);
}


//= Eliminate all facts not connected to marked active nodes.
// expects something else to mark some nodes as non-zero
// after GC all remaining nodes left in unmarked (0) state 
// returns number of nodes removed

int jhcWorkMem::rem_unmarked (int dbg)
{
  jhcNetNode *tail, *n = Next(NULL);
  int cnt = 0;

//dbg = 1;                   // quick way to list erasures

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
      rem_ext(n);            // for objects and faces
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
  
//= Make all elements of the description eligible for matching.
// ensures information becomes available only at proper times

void jhcWorkMem::RevealAll (const jhcGraphlet& desc)
{
  jhcNetNode *n;
  int i, ni = desc.NumItems(), cnt = 0;
 
  for (i = 0; i < ni; i++)
  {
    n = desc.Item(i);
    if (!n->Visible())
    {
      n->Reveal();
      cnt++;                           
    }
  }
  Dirty(cnt);                // for halo refresh
}


//= Override beliefs of any older main memory versions of nodes in description.
// sets belief of older versions negative in favor of newer version
// ignores sense negation when checking for equality to allow truth value flip
// ensures only one variant of predicate will have belief > 0 (generally most recent)
// if run incrementally, first non-zero match found should be the only one 
// returns number of non-halo assertions overridden

int jhcWorkMem::Endorse (const jhcGraphlet& desc, int dbg)
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
          if (n->LexMatch(n2) && n->SameArgs(n2))
          {
            // negate belief for any fact with same sense or opposite sense (neg)
            // prevents rule matching (node removal could leave dangling pointers)
            if (cnt++ <= 0)
              jprintf(1, dbg, "Endorse:\n");
            jprintf(1, dbg, "  %s overrides %s\n", n->Nick(), n2->Nick());
            n2->Suppress();
            break;
          }
    }
  }

  // mark whether any changes have occurred (mostly for halo recomputation)
  if (cnt > 0)
    jprintf(1, dbg, "\n");
  Dirty(cnt);
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                            External Nodes                             //
///////////////////////////////////////////////////////////////////////////

//= Link some external reference number to a particular node.
// kind: 0 = object, 1 = agent, 2 = surface
// returns 1 if successful, 0 if out of space

int jhcWorkMem::ExtLink (int rnum, jhcNetNode *obj, int kind)
{
  int i;

  // look for pre-existing entry
  for (i = 0; i < emax; i++)
    if ((cat[i] == kind) && (ext[i] == rnum))
      break;

  // alter or erase entry found
  if (i < emax)
  {
    if (obj != NULL)
      nref[i] = obj;
    else
    {
      ext[i] = 0;
      nref[i] = NULL;
    }
    return 1;
  }

  // add entry at first empty slot
  for (i = 0; i < emax; i++)
    if (nref[i] == NULL)
      break;
  if (i >= emax)
    return 0;
  cat[i] = kind;
  ext[i] = rnum;
  nref[i] = obj;
  return 1;
}


//= Find the first array entry which has given reference number.
// kind: 0 = object, 1 = agent, 2 = surface
// returns associated node or NULL if none

jhcNetNode *jhcWorkMem::ExtRef (int rnum, int kind) const
{
  int i;

  for (i = 0; i < emax; i++)
    if ((cat[i] == kind) && (ext[i] == rnum))
      return nref[i];
  return NULL;  
}


//= Find the first array entry which has given main memory node.
// kind: 0 = object, 1 = agent, 2 = surface
// returns associated reference number or 0 if none

int jhcWorkMem::ExtRef (const jhcNetNode *obj, int kind) const
{
  int i;

  if (obj != NULL)
    for (i = 0; i < emax; i++)
      if ((cat[i] == kind) && (nref[i] == obj))  
        return ext[i];
  return 0;  
}


//= Remove all entries associated with this node.

void jhcWorkMem::rem_ext (const jhcAliaDesc *obj) 
{
  int i;

  if (obj != NULL)
    for (i = 0; i < emax; i++)
      if (nref[i] == obj)
      {
        ext[i] = 0;
        nref[i] = NULL;
      }
}


//= Enumerate ID's for all items of a certain kind having an external link.
// start with last = 0 then feed in previous answer, returns 0 at end

int jhcWorkMem::ExtEnum (int last, int kind) const
{
  int i, ready = ((last <= 0) ? 1 : 0);
  
  for (i = 0; i < emax; i++)
    if (cat[i] == kind)
    {
      if (ready > 0)
        return ext[i];
      if ((last != 0) && (ext[i] == last))
        ready = 1;
    }
  return 0;
}
