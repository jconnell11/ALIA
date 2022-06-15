// jhcSituation.cpp : semantic network description to be matched
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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

#include "Semantic/jhcSituation.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSituation::~jhcSituation ()
{
}


//= Default constructor initializes certain values.

jhcSituation::jhcSituation ()
{
  nu = 0;
  refmode = 0;
  chkmode = 0;
  bth = 0.5;
  dbg = 0;
}


//= Initialize condition to be a copy of graphlet with external nodes.
// useful for jhcAliaDir::HaltAction

void jhcSituation::Init (const jhcGraphlet& desc)
{
  cond.Copy(desc);
  nu = 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Match a semantic network fragment to assertions in working memory.
// "m" is typically an array of "mc" bindings, one for each match 
// if "mc" > 0 then checks caveats before invoking callback function
// "f" is a set of facts to match against first, "f2" is for remainder
// typically if f2 is present, f is the trigger clause of an operator
// if this is not the case, then matching nodes from f need blf >= bth
// changed from const method to let match_found alter jhcNetRef::recent
// returns total number of complete matches found

int jhcSituation::MatchGraph (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                              const jhcNodeList& f, const jhcNodeList *f2) 
{
  jhcBindings m2;
  jhcBindings *b = m + __max(0, mc - 1);
  int i, cnt, mc2 = 0;

  // see if current instance fully matched
  if (b->Complete())
  {
    // if testing caveat, report blockage
    if (mc <= 0)
      return 1;

    // otherwise check that none of the caveats are matched
    // always use wmem (f2) for unless parts of operators
    for (i = 0; i < nu; i++)
    {
      m2.Copy(*b);
      m2.expect += unless[i].NumItems();
      if (MatchGraph(&m2, mc2, unless[i], ((f2 != NULL) ? *f2 : f), NULL) > 0)
        return 0;
    }

    // current set of bindings is suitable
    return match_found(m, mc);
  }

  // otherwise pick some new pattern node and try to match it to memory
  // returns negative if no candidate, 0 if no matches to picked candidate
  if ((cnt = try_props(m, mc, pat, f, f2)) < 0)
    if ((cnt = try_args(m, mc, pat, f, f2)) < 0)
      if (f.NumBins() > 1)
        cnt = try_hash(m, mc, pat, f, f2);
      else      
        cnt = try_bare(m, mc, pat, f, f2);
  if (cnt > 0)
    return cnt;

  // for operator, if trigger fully matched then try rest with wmem
  if (f2 != NULL)
  {
    jprintf(2, dbg, "%*s~ alternate node list ~\n", b->NumPairs() * 2, "");
    return MatchGraph(m, mc, pat, *f2, NULL);
  }
  return 0;                         // pattern cannot be fully matched
}


//= Tries to match an unbound node which is a property of something already bound.
// returns -1 if no proper focus, else total number of matches that caused invocations

int jhcSituation::try_props (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                             const jhcNodeList& f, const jhcNodeList *f2) 
{
  jhcBindings *b = m + __max(0, mc - 1);
  const jhcNetNode *val, *anchor = NULL, *focus = NULL;
  const char *role;
  int i, np, pnum, n = b->NumPairs(), cnt = 0;

  // get a bound node from query graphlet (np static)
  for (i = 0; ((i < n) && (focus == NULL)); i++)
  {
    // look through its properties for an unbound node 
    anchor = b->GetKey(i);
    np = anchor->NumProps();
    for (pnum = 0; pnum < np; pnum++)
    {
      // selected node must be part of pattern
      focus = anchor->Prop(pnum);
      if (!b->InKeys(focus) && pat.InDesc(focus))
        break;
      focus = NULL;
    }
  }

  // make sure some node to be bound
  if (focus == NULL)
    return -1;
  role = anchor->Role(pnum);
  val = b->LookUp(anchor);
  jprintf(2, dbg, "%*s  try_props: %s (from %s)\n", 2 * n, "", focus->Nick(), anchor->Nick());

  // consider properties of anchor's binding as candidates (most recent first)
  np = val->NumProps();
  for (i = np - 1; i >= 0; i--)
    if (val->RoleMatch(i, role))
    {
      // continue matching with selected mate for focus
      n = try_binding(focus, val->Prop(i), m, mc, pat, f, f2);
      if (n < 0)
        return 1;
      cnt += n;
    }
  return cnt;
}


//= Tries to match an unbound node which is an argument of something already bound.
// returns -1 if no proper focus, else total number of matches that caused invocations

int jhcSituation::try_args (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                            const jhcNodeList& f, const jhcNodeList *f2) 
{
  jhcBindings *b = m + __max(0, mc - 1);
  const jhcNetNode *fact, *anchor = NULL, *focus = NULL;
  const char *slot;
  int i, na, anum, n = b->NumPairs(), cnt = 0;

  // get a bound node from query graphlet (na static)
  for (i = 0; ((i < n) && (focus == NULL)); i++)
  {
    // look through its arguments for an unbound node
    anchor = b->GetKey(i);
    na = anchor->NumArgs();
    for (anum = 0; anum < na; anum++)
    {
      // selected node must be part of pattern
      focus = anchor->Arg(anum);
      if (!b->InKeys(focus) && pat.InDesc(focus))
        break;
      focus = NULL;
    }
  }

  // make sure some node to be bound
  if (focus == NULL)
    return -1;
  slot = anchor->Slot(anum);
  fact = b->LookUp(anchor);
  jprintf(2, dbg, "%*s  try_args: %s (from %s)\n", 2 * n, "", focus->Nick(), anchor->Nick());

  // consider arguments of anchor's binding as candidates (na might change during loop)
  for (i = 0; i < fact->NumArgs(); i++)
    if (strcmp(fact->Slot(i), slot) == 0)
    {
      // continue matching with selected mate for focus
      n = try_binding(focus, fact->Arg(i), m, mc, pat, f, f2);
      if (n < 0)
        return 1;
      cnt += n;
    }
  return cnt;
}


//= Tries to match an unbound node in pattern.
// returns -1 if no proper focus, else total number of matches that caused invocations

int jhcSituation::try_bare (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                            const jhcNodeList& f, const jhcNodeList *f2) 
{
  jhcBindings *b = m + __max(0, mc - 1);
  const jhcNetNode *focus = NULL;
  jhcNetNode *mate = NULL;
  int scan, i, n = pat.NumItems(), cnt = 0;

  // find an unbound node to start matching
  for (scan = 0; ((scan <= 3) && (focus == NULL)); scan++)
    for (i = 0; i < n; i++)
    {
      // scan: 0 = has literal argument, 1 = has literal property, 2 = has lexical term, 3 = any
      focus = pat.Item(i);
      if (!b->InKeys(focus))
        if (((scan <= 0) && pat.ArgOut(focus))          || ((scan == 1) && pat.PropOut(focus)) ||
            ((scan == 2) && (b->LexSub(focus) != NULL)) ||  (scan >= 3))
          break;
      focus = NULL;
    }

  // make sure some node to be bound
  if (focus == NULL)
    return -1;
  jprintf(2, dbg, "%*s  try_bare: %s initial focus\n", b->NumPairs() * 2, "", focus->Nick());

  // consider nodes with matching labels as candidates (NextNode list might change during loop)
  while ((mate = f.NextNode(mate)) != NULL)
  {
    // continue matching with selected mate for focus
    n = try_binding(focus, mate, m, mc, pat, f, f2);
    if (n < 0)
      return 1;
    cnt += n;
  }
  return cnt;
}


//= Picks the pattern node with the least possible matches and tries only those.
// returns -1 if no proper focus, else total number of matches that caused invocations

int jhcSituation::try_hash (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                            const jhcNodeList& f, const jhcNodeList *f2) 
{
  jhcBindings *b = m + __max(0, mc - 1);
  const jhcNetNode *item, *focus = NULL;
  jhcNetNode *mate = NULL;
  int i, occ, best, bin, n = pat.NumItems(), cnt = 0;

  // find the unbound node with fewest potential matches to start
  for (i = 0; i < n; i++)
  {
    item = pat.Item(i);
    if (!b->InKeys(item))
    {
      if ((occ = f.SameBin(*item, b)) <= 0)         
        break;                                   // pattern unmatchable!
      if ((focus == NULL) || (occ < best))
      {
        focus = item;
        best = occ;
      }
    }
  }

  // make sure some node to be bound and some possibiliites exist
  if ((focus == NULL) || (occ <= 0))
    return -1;
  jprintf(2, dbg, "%*s  try_hash: %s initial focus (%d)\n", b->NumPairs() * 2, "", focus->Nick(), best);

  // only consider nodes with matching hashes as candidate matches
  bin = ((b->LexSub(focus) == NULL) ? -1 : focus->Code());
  while ((mate = f.NextNode(mate, bin)) != NULL)
  {
    // continue matching with selected mate for focus
    n = try_binding(focus, mate, m, mc, pat, f, f2);
    if (n < 0)
      return 1;
    cnt += n;
  }
  return cnt;
}


//= Binds focus to mate then continues to try to find full match of pattern.
// useful matcher entry point for jhcFindGuess to check basic instantiation
// returns number of matches found, -1 if "unless" clause is matched 

int jhcSituation::try_binding (const jhcNetNode *focus, jhcNetNode *mate, jhcBindings *m, int& mc, 
                               const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2)
{
  int rc, i, nb = 0, n = __max(0, mc - 1), lvl = 2 * m[n].NumPairs(), cnt = 0;

  // sanity check
  if (!mate->Visible())
    return 0;

  // make sure superficial pairing is okay
  if (f2 != NULL)
  {
    // matching operator condition against directive
    if (!f.InList(mate))
      return jprintf(3, dbg, "%*s   mate = %s (%4.2f) not in list\n", lvl, "", mate->Nick(), mate->Blf(bth));
    if ((rc = consistent(mate, focus, pat, m + n, -fabs(bth))) <= 0)
      return jprintf(3, dbg, "%*s   mate = %s (%4.2f) --> fails %d\n", lvl, "", mate->Nick(), mate->Blf(bth), rc);
  }
  else if (f.Prohibited(mate))
    return jprintf(3, dbg, "%*s   mate = %s (%4.2f) prohibited\n", lvl, "", mate->Nick(), mate->Blf(bth));
  else if ((rc = consistent(mate, focus, pat, m + n, bth)) <= 0)     // min belief value
    return jprintf(3, dbg, "%*s   mate = %s (%4.2f) --> fails %d\n", lvl, "", mate->Nick(), mate->Blf(bth), rc);

  // add pair to all remaining bindings (all nb are the same)
  jprintf(3, dbg, "%*s   mate = %s (%4.2f)\n", lvl, "", mate->Nick(), mate->Blf(bth));
  for (i = 0; i <= n; i++)
    nb = m[i].Bind(focus, mate);    

  // try to complete pattern (stop after first match for caveat)
  cnt = MatchGraph(m, mc, pat, f, f2);
  if ((cnt > 0) && (mc <= 0))      
    return -1;                

  // remove pair for backtrack (mc might change if successful match)
  // nb - 1 used since jhcAliaRule adds bindings during AssertHalo
  n = __max(0, mc - 1);  
  for (i = 0; i <= n; i++)
    m[i].TrimTo(nb - 1);
  return cnt;
}


//= Check if mate and focus are consistent given current bindings.
// normally accept only blf >= th, if th < 0 then accept blf >= -th or blf = 0.0 (exactly)
// returns 1 if okay, otherwise zero or negative indicating where it failed

int jhcSituation::consistent (const jhcNetNode *mate, const jhcNetNode *focus, const jhcGraphlet& pat, const jhcBindings *b, double th) const
{
  const jhcNetNode *arg, *val, *fact;
  int i, n;

  // prevent use of same term for different "variables" 
  if (b->InSubs(mate))                         
    return -9;

  // sense of predicate should be the same and belief must be high enough (or hypothetical) 
  if (!focus->ObjNode())  
  {
    if ((chkmode <= 0) && (mate->Neg() != focus->Neg()))             // ignore "neg" for CHK 
      return -8;
    if (!mate->Sure(th))
      return -7;
    if (focus->Arity() != mate->Arity(0))                            // "father" matches "father of"
      return -6;
    if (mate->Done() != focus->Done())
      return -5;
  }

  // any action must be in the same state and actual predicate terms must be the same 
  if (!b->LexAgree(focus, mate))
    return -4;
  
  // see if finding referents inside a rule or operator
  if (refmode > 0)
  {
    // conversation participants are special ("you" can match "someone" but "someone" cannot match "you")
    if (mate->LexMatch("you") && !focus->LexMatch("you"))
      return -3;
    if (mate->LexMatch("me") && !focus->LexMatch("me"))
      return -2;
  }

  // check that mate is consistent with all arguments (even externals)
  n = focus->NumArgs();
  for (i = 0; i < n; i++)
  {
    arg = focus->Arg(i);
    val = ((pat.InList(arg)) ? b->LookUp(arg) : arg);          // must match literals also
    if (val != NULL)
      if (!mate->HasVal(focus->Slot(i), val))
        return -1;
  }
      
  // check that mate is consistent with all bound properties
  n = focus->NumProps();
  for (i = 0; i < n; i++)
    if ((fact = b->LookUp(focus->Prop(i))) != NULL)
      if (!mate->HasFact(fact, focus->Role(i)))
        return 0;
  return 1;
}


//= Simple way to find equivalent node in working memory based on some description.
// create local pattern of nodes (can point externally) then match this to wmem
// need to call BuildCond() before any NewNode() or NewProp() calls
// simplified version of jhcNetRef useful for composing NOTEs in grounding functions
// returns wmem node for focus (if found)

jhcNetNode *jhcSituation::FindRef (const jhcNetNode *focus, const jhcNodeList& wmem)
{
  jhcBindings b;
  int mc = 1;

  b.expect = cond.NumItems();
  if (MatchGraph(&b, mc, cond, wmem) > 0)
    return b.LookUp(focus);
  return NULL;
}   


