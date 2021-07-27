// jhcAliaOp.cpp : advice on what to do given some stimulus or desire
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

#include "Interface/jhcMessage.h"   // common video

#include "Action/jhcAliaDir.h"      // common robot

#include "Reasoning/jhcAliaOp.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaOp::~jhcAliaOp ()
{
  delete meth;
}


//= Default constructor initializes certain values.

jhcAliaOp::jhcAliaOp (JDIR_KIND k)
{
  kind = k;
  id = 0;
  lvl = 3;             // default = newly told
  pref = 1.0;
  next = NULL;
  meth = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Find all variable bindings that cause this operator to match.
// needs belief threshold "mth" for match, and "tol" number of missing items allowed
// count of bindings and actual sets stored in given directive
// assumes initial directive "bcnt" member variable has been initialized 
// returns total number of bindings filled, negative for problem

int jhcAliaOp::FindMatches (jhcAliaDir& dir, const jhcWorkMem& f, double mth) 
{
  const jhcNetNode *item, *focus = cond.Main();
  jhcNetNode *mate = NULL;
  JDIR_KIND k = dir.kind;
  int i, occ, bin, found, nc = cond.NumItems(), best = 0, cnt = 0;

  // main node of NOTE not special, so pick most constrained instead
  jprintf(2, dbg, "Operator %d matching (%4.2f) ...\n", id, mth);
  if ((k == JDIR_NOTE) && (f.NumBins() > 1))
  {  
    for (i = 0; i < nc; i++) 
    {
      item = cond.Item(i);
      if ((occ = f.SameBin(*item)) <= 0)
        return 0;                                // pattern unmatchable!
      if ((best <= 0) || (occ < best))
      {
        focus = item;
        best = occ;
      }
    }
  }

  // set control parameters
  jprintf(2, dbg, "  try_mate: %s initial focus\n", focus->Nick()); 
  bin = ((focus->Lex() == NULL) ? -1 : focus->Code());
  omax = dir.MaxOps();
  tval = dir.own;
  bth = (((k == JDIR_CHK) || (k == JDIR_FIND) || (k == JDIR_BIND)) ? -mth : mth);

  // generally require main nodes (i.e. verb) of directives to match
  if (k == JDIR_CHK)
    while ((mate = (dir.key).NextNode(mate)) != NULL)
    {
      // CHK triggers can start matching anywhere in description
      if ((found = try_mate(focus, mate, dir, f)) < 0)
        return found;
      cnt += found;
    }
  else if (k == JDIR_NOTE)
    while ((mate = f.NextNode(mate, bin)) != NULL)
    {
      // NOTE triggers match anything in memory (including halo)
      // checks for relatedness at end (i.e. tval in match_found)
      if ((found = try_mate(focus, mate, dir, f)) < 0)
        return found;
      cnt += found;
    }
  else                       // most directives (DO, FIND, ...)
    cnt = try_mate(focus, dir.KeyMain(), dir, f);    
  return cnt;
}


//= Given some candidate for main condition node, find all bindings that let directive match.
// returns total number of bindings filled, negative for problem
 
int jhcAliaOp::try_mate (const jhcNetNode *focus, jhcNetNode *mate, jhcAliaDir& dir, const jhcWorkMem& f) 
{
  const jhcGraphlet *trig = &(dir.key);
  jhcBindings *m = dir.match;
  int i, n = cond.NumItems();

  // sanity check
  if (mate == NULL) 
    return -1;               // stops all OP matching
  if (!mate->Visible())
    return 0;
  jprintf(2, dbg, "   mate = %s (%4.2f)", mate->Nick(), mate->Belief());

  // test main node compatibility (okay with blank nodes)
  if ((kind == JDIR_NOTE) && !mate->Sure(bth))
    return jprintf(2, dbg, " -> bad belief\n");
  if ((mate->Neg() != focus->Neg()) || (mate->Done() != focus->Done()) || 
      ((focus->Lex() != NULL) && !focus->LexMatch(mate)))
    return jprintf(2, dbg, " -> bad neg, done, or lex\n");
  if (mate->Arity() != focus->Arity())
    return jprintf(2, dbg, " -> different arity\n");

  // force binding of initial items and set trigger size
  jprintf(2, dbg, "\n");
  first = dir.mc;
  for (i = 0; i < first; i++)
  {
    m[i].Clear();
    m[i].Bind(focus, mate);
    m[i].expect = n;
  }

  // start core matcher as a one step process if NOTE, else two step
  if (dir.Kind() == JDIR_NOTE)
    return MatchGraph(m, dir.mc, cond, f, NULL);
  return MatchGraph(m, dir.mc, cond, *trig, &f);
}


//= Complete set of bindings has been found so save to array and decrement size.

int jhcAliaOp::match_found (jhcBindings *m, int& mc) 
{
  jhcBindings *b = &(m[mc - 1]);
  const jhcNetNode *k, *n;
  int i, nb = b->NumPairs();

  // typically checking an unless clause
  if (mc <= 0)
    return 0;

  // if NOTE trigger, check that at least one non-object node has proper relevance
  if (tval > 0)
  {
    for (i = 0; i < nb; i++)
    {
      k = b->GetKey(i);
      n = b->GetSub(i);
      if (!k->ObjNode() && (n != NULL) && (n->top == tval))
        break;
    }
    if (i >= nb)
      return jprintf(3, dbg, "%*s reject - no trigger related to new NOTE\n", 2 * nb + 1, "");
  }

  // make sure proposed action not already in list ("first" set in try_match)
  // since this is within an operator, all pref's will be the same
  for (i = mc; i < first; i++)
    if (SameEffect(*b, m[i]))
      return jprintf(3, dbg, "%*s reject - same effect as a bindings[%d]\n", 2 * nb + 1, "", i);
  
  // accept bindings and shift to next set
  jprintf(3, dbg, "%*s ... FULL MATCH = bindings[%d]\n", 2 * nb + 1, "", mc - 1);
  if (mc <= 1)
    jprintf(">>> More than %d applicable operators in jhcAliaOp::match_found !\n", omax);
  else
     mc--;
  return 1;
}


//= Tells if two sets of bindings yield identical action.
// can happen if some bound item is not used in action part
// also useful for checking non-return inhibition in directive

bool jhcAliaOp::SameEffect (const jhcBindings& b1, const jhcBindings& b2) const
{
  const jhcNetNode *k;
  int i, n = b1.NumPairs();

  if (meth != NULL)
    for (i = 0; i < n; i++)
    {
      k = b1.GetKey(i);
      if (meth->Involves(k) && (b2.LookUp(k) != b1.GetSub(i)))
        return false;
    }
  return true;     
}


///////////////////////////////////////////////////////////////////////////
//                              File Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Read at current location in a file to fill in details of self.
// returns: 1 = successful, 0 = syntax error, -1 = end of file, -2 = file error

int jhcAliaOp::Load (jhcTxtLine& in)
{
  int ans;

  if (in.NextContent() == NULL)
    return -1;
  ClrTrans();
  ans = load_pattern(in);
  ClrTrans(0);
  return((in.Error()) ? -2: ans);
}


//= Extract "trig", "unless" and method parts of operator.
// returns 1 if successful, 0 for format problem, -1 for file error

int jhcAliaOp::load_pattern (jhcTxtLine& in)
{
  jhcAliaDir dir;
  int ans;

  // get trigger condition as a directive and copy important parts
  if (!in.Begins("trig:"))
    return 0;
  in.Flush();
  if ((ans = dir.Load(*this, in)) <= 0)         
    return ans;
  kind = dir.kind;
  cond.Copy(dir.key);

  // check for caveats
  nu = 0;
  if (in.Next() == NULL)
    return 0;
  while (in.Begins("unless:"))
  {
    in.Skip("unless:");
    if ((ans = LoadGraph(&(unless[nu]), in)) <= 0)
      return ans;
    if (nu++ >= umax)
    {
      jprintf(">>> More than %d caveats in jhcAliaOp::Load !\n", umax);
      break;
    }
  }

  // get selection preference (defaults to 1.0)
  if (in.Begins("pref:"))
  {
    in.Skip("pref:");
    if (sscanf_s(in.Head(), "%lf", &pref) != 1)
      return 0;
    if (in.Next(1) == NULL)
      return 0;
  }

  // get associated action sequence
  if (!in.Begins("----"))
    return 0;
  in.Flush();
  meth = new jhcAliaChain;
  if ((ans = meth->Load(*this, in)) <= 0)        
    return ans;
  return 1;
}


//= Save self out in machine readable form to current position in a file.
// detail: 0 no extras, 1 show belief, 2 show tags, 3 show both
// return: 1 = successful, 0 = bad format, -1 = file error

int jhcAliaOp::Save (FILE *out, int detail)
{
  jhcAliaDir dir;
  int i;

  // operator number
  if (id > 0)
    jfprintf(out, "// OPERATOR %d\n", id);
  else
    jfprintf(out, "// OPERATOR\n");

  // trigger graphlet (converted to directive)
  jfprintf(out, "trig:\n");
  dir.kind = kind;
  (dir.key).Copy(cond);
  dir.Save(out, 2, detail);

  // caveats
  for (i = 0; i < nu; i++)
  {
    jfprintf(out, "unless: ");
    unless[i].Save(out, -8, detail);
    jfprintf(out, "\n");
  }

  // selection preference
  if (pref != 1.0)
    jfprintf(out, "pref: %4.2f\n", pref);

  // associated expansion
  jfprintf(out, "-----------------\n");
  if (meth != NULL)
    meth->Save(out, 2, NULL, detail);
  jfprintf(out, "\n");
  return((ferror(out) != 0) ? -1 : 1);
}

