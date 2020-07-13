// jhcSituation.cpp : semantic network description to be matched
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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
  bth = 0.5;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Match a semantic network fragment to assertions in working memory.
// "m" is typically an array of "mc" bindings, one for each match 
// if "mc" > 0 then checks caveats before invoking callback function
// "f" is a set of facts to match against first, "f2" is for remainder
// triggering facts "f" can be missing up to "tol" nodes wrt "pat"
// typically if f2 is present, f is the trigger clause of an operator
// if this is not the case, then matching nodes from f need blf >= bth
// changed from const method to let match_found alter jhcNetRef::recent
// returns total number of matches for which match_found succeeded

int jhcSituation::MatchGraph (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                              const jhcNodeList& f, const jhcNodeList *f2, int tol) 
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
      if (MatchGraph(&m2, mc2, unless[i], ((f2 != NULL) ? *f2 : f), NULL, tol) > 0)
        return 0;
    }

    // current set of bindings is suitable
    return match_found(m, mc);
  }

  // otherwise pick some new pattern node and try to match it to memory
  // returns negative if no candidate, 0 if no matches to picked candidate
  if ((cnt = try_props(m, mc, pat, f, f2, tol)) < 0)
    if ((cnt = try_args(m, mc, pat, f, f2, tol)) < 0)
      cnt = try_bare(m, mc, pat, f, f2, tol);
  if (cnt > 0)
    return cnt;

  // for operator, if trigger fully matched then try rest with wmem
  if (f2 != NULL)
    return MatchGraph(m, mc, cond, *f2, NULL, tol);
  return 0;                                                // pattern is unmatchable
}


//= Tries to match an unbound node which is a property of something already bound.
// returns -1 if no proper focus, else total number of matches that caused invocations

int jhcSituation::try_props (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                             const jhcNodeList& f, const jhcNodeList *f2, int tol) 
{
  jhcBindings *b = m + __max(0, mc - 1);
  const jhcNetNode *anchor, *val, *focus = NULL;
  const char *role;
  int i, n, np, pnum, cnt = 0;

  // get a bound node from query graphlet (np static)
  n = b->NumPairs();
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

  // consider properties of anchor's binding as candidates (np might change during loop)
  for (i = 0; i < val->NumProps(); i++)
    if (strcmp(val->Role(i), role) == 0)
    {
      // continue matching with selected mate for focus
      n = try_binding(focus, val->Prop(i), m, mc, pat, f, f2, tol);
      if (n < 0)
        return 1;
      cnt += n;
    }
  return cnt;
}


//= Tries to match an unbound node which is an argument of something already bound.
// returns -1 if no proper focus, else total number of matches that caused invocations

int jhcSituation::try_args (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                            const jhcNodeList& f, const jhcNodeList *f2, int tol) 
{
  jhcBindings *b = m + __max(0, mc - 1);
  const jhcNetNode *anchor, *fact, *focus = NULL;
  const char *slot;
  int i, n, na, anum, cnt = 0;

  // get a bound node from query graphlet (na static)
  n = b->NumPairs();
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

  // consider arguments of anchor's binding as candidates (na might change during loop)
  for (i = 0; i < fact->NumArgs(); i++)
    if (strcmp(fact->Slot(i), slot) == 0)
    {
      // continue matching with selected mate for focus
      n = try_binding(focus, fact->Arg(i), m, mc, pat, f, f2, tol);
      if (n < 0)
        return 1;
      cnt += n;
    }
  return cnt;
}


//= Tries to match an unbound node in pattern.
// returns -1 if no proper focus, else total number of matches that caused invocations

int jhcSituation::try_bare (jhcBindings *m, int& mc, const jhcGraphlet& pat, 
                            const jhcNodeList& f, const jhcNodeList *f2, int tol) 
{
  jhcBindings *b = m + __max(0, mc - 1);
  const jhcNetNode *focus = NULL;
  jhcNetNode *mate = NULL;
  int i, any, n = pat.NumItems(), cnt = 0;

  // pick an unbound node, preferably with a lexical tag (n static)
  for (any = 0; ((any <= 1) && (focus == NULL)); any++)
    for (i = 0; i < n; i++)
    {
      // first pass requires lexical value (second takes any)
      focus = pat.Item(i);
      if (!b->InKeys(focus) && ((any > 0) || !focus->Blank()))
        break;
      focus = NULL;
    }

  // make sure some node to be bound
  if (focus == NULL)
    return -1;

  // consider nodes with matching labels as candidates (NextNode list might change during loop)
  while ((mate = f.NextNode(mate)) != NULL)
  {
    // continue matching with selected mate for focus
    n = try_binding(focus, mate, m, mc, pat, f, f2, tol);
    if (n < 0)
      return 1;
    cnt += n;
  }
  return cnt;
}


//= Binds focus to mate then continues to try to find full match of pattern.
// returns number of matches found, -1 if "unless" clause is matched 

int jhcSituation::try_binding (const jhcNetNode *focus, jhcNetNode *mate, jhcBindings *m, int& mc, 
                               const jhcGraphlet& pat, const jhcNodeList& f, const jhcNodeList *f2, int tol)
{
  int i, nb, n = __max(0, mc - 1), cnt = 0;

  // make sure superficial pairing is okay
  if (f2 != NULL)
  {
    // matching operator condition against directive
    if (!f.InList(mate))
      return 0;
    if (!consistent(mate, focus, m + n, 0.0))
      return 0;
  }
  else if (!consistent(mate, focus, m + n, bth))     // min belief value
    return 0;

  // add pair to all remaining bindings (all nb are the same)
  for (i = 0; i <= n; i++)
    nb = m[i].Bind(focus, mate);    

  // try to complete pattern (stop after first match for caveat)
  cnt = MatchGraph(m, mc, pat, f, f2, tol);
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

bool jhcSituation::consistent (const jhcNetNode *mate, const jhcNetNode *focus, const jhcBindings *b, double th) const
{
  const jhcNetNode *val, *fact;
  int i, n;

  // check node instrinsic characteristics and whether belief is high enough
  // also cannot use same term for different "variables"
  if ((mate->Neg() != focus->Neg()) || (mate->Done() != focus->Done()) || mate->LexConflict(focus))
    return false;
  if (!mate->ObjNode() && (mate->Belief() < th))
    return false;
  if (b->InSubs(mate))
    return false;

  // see if finding referents inside a rule or operator
  if (refmode > 0)
  {
    // conversation participants are special ("you" can match "someone" but "someone" cannot match "you")
    if (mate->HasWord("you", 1) && !focus->HasWord("you", 1))
      return false;
    if ((mate->HasWord("me", 1) || mate->HasWord("I", 1)) && !focus->HasWord("me", 1) && !focus->HasWord("I", 1))
      return false;
  }

  // check that mate is consistent with all bound arguments
  n = focus->NumArgs();
  for (i = 0; i < n; i++)
    if ((val = b->LookUp(focus->Arg(i))) != NULL)
      if (!mate->HasVal(focus->Slot(i), val))
        return false;
      
  // check that mate is consistent with all bound properties
  n = focus->NumProps();
  for (i = 0; i < n; i++)
    if ((fact = b->LookUp(focus->Prop(i))) != NULL)
      if (!mate->HasFact(fact, focus->Role(i)))
        return false;
  return true;
}

