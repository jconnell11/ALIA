// jhcWorkMem.cpp : main temporary semantic network for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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
  halo.NegID();        // negative IDs and newest at list end
  rim = 0;
  nimbus = 0;
  mode = 0;

  // conversational agents
  self = NULL;
  user = NULL;
  InitPeople(NULL);
  clr_ext();

  // fact belief threshold
  skep = 0.5;    

  // debugging
  noisy = 1;           // defaulted from jhcAliaCore
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
// use MaxBand(3) to include halo, member variable "mode" limits last
// retrieval order by bands: 0->1->2->3, call with prev = NULL to get first node
// can restrict selection to just one hash bin, or use all if bin < 0
// 
// main may have missing IDs and be shuffled by recency
// halo has no gaps and is strictly ascending by creation
// actual order of main and halo scrambled by bin splitting
// <pre>
//
// Conceptual diagram of node order in each bin based on IDs
// 
//   main:  obj-22
//          obj-2      BAND 0: current conscious facts
//          obj-9              (ordered by recency)
//          obj-17
// 
//   halo:  obj+3
//          obj+4      BAND 1: LTM ghost facts
//          obj+7
//                  <--- rim = 7
//          obj+8
//          obj+10     BAND 2: one rule inferences
//          obj+12
//                  <--- nimbus = 14
//          obj+16
//          obj+20     BAND 3: two rule inferences
//          obj+21
//
// </pre>

jhcNetNode *jhcWorkMem::NextNode (const jhcNetNode *prev, int bin) const
{
  jhcNetNode *n = NULL;
  int id;

  // sanity check 
  if (mode < 0)
    return NULL;

  // get candidate for next node
  if ((prev != NULL) && prev->Halo())            // continue in halo
    n = halo.Next(prev, bin);              
  else if ((n = Next(prev, bin)) == NULL)        // continue in main
    if (mode > 0)
      n = halo.Pool(bin);                        // shift to halo

  // possibly sufficient
  if ((n == NULL) || !n->Halo())
    return n;

  // skip halo nodes if not in valid range (band0 - mode)
  while (n != NULL)
  {
    id = abs(n->Inst());
    if (((mode == 1) && (id <= rim)) || ((mode == 2) && (id <= nimbus)) || (mode == 3))
      break;                                     // id just right so keep
    else if (bin < 0)
      n = halo.NextPool(n);                      // id too high so shift bin
    else
      n = NULL;                                  // id too high so punt
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
         (((mode == 1) || (mode == 2)) && (n->Inst() < -nimbus)));
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


//= Tell if some node is in a given partition of memory based on instance number.
// part: 0 = main memory, 1 = LTM ghost facts, 2 = halo single rule, 3 = halo double rule
// member variable "rim" = instance of last LTM, "nimbus" = last single rule halo
// Note: see NextNode() for diagram of memory layout

bool jhcWorkMem::InBand (const jhcNetNode *n, int part) const
{
  int id;

  // check for LTM memory then main memory vs halo split
  if ((n == NULL) || (!n->Home(this) && !n->Home(&halo)))
    return true;                           
  if (!n->Halo())
    return(part == 0);
  id = -(n->Inst());

  // check against recorded boundaries in halo
  if (part == 1)
    return(id <= rim);
  if (part == 2)
    return((id > rim) && (id <= nimbus));
  if (part == 3)
    return(id > nimbus);
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                            Halo Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Tell if a node is visible and in either main memory or an LTM ghost fact.
// LTM ghost facts have halo instance numbers less than "rim"  
// generally ghost facts are dis-preferred for language generation and tried second

bool jhcWorkMem::VisMem (const jhcNetNode* n, int ghost) const
{
  if ((n == NULL) || !n->Visible())
    return false;
  if (ghost <= 0)
    return InList(n);
  return(halo.InList(n) && (abs(n->Inst()) <= rim));
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
    n->SetKeep((n->Keep() > 0) ? 1 : 0);         // normalize values

  // scan all and expand marks to related nodes
  jprintf(2, dbg, "\n  retaining nodes:\n");
  n = NULL;
  while ((n = Next(n)) != NULL)
    if (n->Keep() == 1)
      keep_from(n, dbg);

  // mark definite keepers (conversation participants)
  keep_party(self);
  keep_party(user);
  return rem_unmarked(dbg);                      // sweep out anything not marked
}


//= Special mark spreader for conversational participants.
// keeps only non-hypothetical HQ, AKO, and REF facts (others may get marked from foci)
// NOTE: assumes given anchor node is WMEM (not DMEM)

void jhcWorkMem::keep_party (jhcNetNode *anchor) const
{
  jhcNetNode *prop, *arg, *deg;
  int i, j, n, n2;

  // definitely keep this node
  if (anchor == NULL)
    return;
  anchor->SetKeep(2);

  // scan through all its properties
  n = anchor->NumProps();  
  for (i = 0; i < n; i++)
  {
    prop = anchor->PropSurf(i);
    if (!prop->Hyp() && InPool(prop) && anchor->RoleIn(i, "name", "ako", "hq", "wrt"))      
    {
      // keep this property and all arguments
      prop->SetKeep(2);
      n2 = prop->NumArgs();
      for (j = 0; j < n2; j++)
      {
        arg = prop->ArgSurf(j);
        arg->SetKeep(1);               // allow spreading from arg
      }

      // retain degree for properties like "very smart"
      n2 = prop->NumProps();
      for (j = 0; j < n2; j++)
      {
        deg = prop->PropSurf(j);
        if (!deg->Hyp() && InPool(deg) && prop->RoleMatch(j, "deg"))
          deg->SetKeep(2);
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
  if ((anchor == NULL) || (anchor->Keep() > 1) || !InPool(anchor))
    return;
  if ((anchor == self) || (anchor == user))                // handled separately
    return;
  jprintf(2, dbg, "    %s%s\n", ((anchor->Keep() <= 0) ? "  " : ""), anchor->Nick());  

  // mark node and all its arguments as being keepers 
  anchor->SetKeep(2);
  n = anchor->NumArgs();  
  for (i = 0; i < n; i++)
    keep_from(anchor->ArgSurf(i), dbg);

  // mark all properties for retention
  n = anchor->NumProps();  
  for (i = 0; i < n; i++)
    keep_from(anchor->PropSurf(i), dbg);
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
    if (n->Keep() > 0)
    {
      n->SetKeep(0);         // eligible for deletion on next round
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
  jhcNetNode *former = NULL;
  int i;

  // look for pre-existing entry
  for (i = 0; i < emax; i++)
    if ((cat[i] == kind) && (ext[i] == rnum))
      break;

  // alter or erase entry found
  if (i < emax)
  {
    former = nref[i];
    if (obj == former)
      return 0;              // no changes
    if (obj != NULL)
      nref[i] = obj;
    else
    {
      ext[i] = 0;            // free up entry
      nref[i] = NULL;
    }
    return ann_link(obj, former, kind, rnum);
  }

  // add entry at first empty slot
  if (obj == NULL)
    return 0;
  for (i = 0; i < emax; i++)
    if (nref[i] == NULL)
      break;
  if (i >= emax)
    return 0;
  cat[i] = kind;
  ext[i] = rnum;
  nref[i] = obj;
  return ann_link(obj, former, kind, rnum);
}


//= Tell linkage between item and semantic network node.
// returns 1 always for convenience

int jhcWorkMem::ann_link (const jhcNetNode *obj, const jhcNetNode *former, int kind, int rnum) const
{
  char item[3][40] = {"object", "head", "surface"};

  if ((kind < 0) || (kind > 2) || (noisy < 1))
    return 1;
  if (obj == NULL)
    jprintf("  .. unlinking tracked %s from %s %s\n", item[kind], former->Nick(), ((former == user) ? "(user)" : ""));
  else if (former == NULL)
    jprintf("  .. linking tracked %s to %s %s\n", item[kind], obj->Nick(), ((obj == user) ? "(user)" : ""));
  else
    jprintf("  .. switching tracked %s for %s %s\n", item[kind], obj->Nick(), ((obj == user) ? "(user)" : ""));
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


///////////////////////////////////////////////////////////////////////////
//                          Debugging Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Print all nodes in the order they would be enumerated.
// can set MaxBand() to print just some bands

int jhcWorkMem::PrintRaw (int hyp) const
{
  jhcNetNode *n = NULL;
  int kmax = 3, nmax = 1, rmax = 3;

  // sanity check
  if (mode < 0)
    return jprintf("\nMEMORY: bad band specs!\n");

  // get print field sizes
  while ((n = NextNode(n, -1)) != NULL)
    if (n->Visible() && ((hyp > 0) || !n->HypAny()))
      n->TxtSizes(kmax, nmax, rmax);

  // write descriptive banner
  if (mode == 0)
    jprintf("\nBAND 0");
  else
    jprintf("\nBANDS 0-%d", mode);
  jprintf(" (rim %d, nimbus %d) =", rim, nimbus);

  // print all as: node -link-> arg and list blf not blf0
  n = NULL;
  while ((n = NextNode(n, -1)) != NULL)
    if (n->Visible() && ((hyp > 0) || !n->HypAny()))
      n->Save(stdout, 2, kmax, nmax, rmax, -2);                
  jprintf("\n");
  return 1;
}
