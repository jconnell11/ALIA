// jhcAliaRule.cpp : declarative implication in ALIA System 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#include "Reasoning/jhcAliaRule.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAliaRule::~jhcAliaRule ()
{
}


//= Default constructor initializes certain values.

jhcAliaRule::jhcAliaRule ()
{
  conf = 1.0;
  id = 0;
  lvl = 3;             // default = newly told
  src0 = 0;
  src1 = 0;
  next = NULL;
  show = 0;
}


///////////////////////////////////////////////////////////////////////////
//                            Source Attribution                         //
///////////////////////////////////////////////////////////////////////////

//= Determine if halo assertion was a result of this rule.
// useful when condensing a two step reasoning chain

bool jhcAliaRule::Asserted (const jhcNetNode *n) const
{
  if ((n == NULL) || (src1 <= 0))
    return false;
  return((n->pod >= src0) && (n->pod <= src1));
}


///////////////////////////////////////////////////////////////////////////
//                             Main Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Find all variable bindings that cause the situation to match memory.
// conditions must have blf >= mth, each successful match asserts result into halo 
// returns total number of results asserted

int jhcAliaRule::AssertMatches (jhcWorkMem& f, double mth, int dbg)
{
  jhcBindings b;
  int n, s1, s0 = f.NumHalo(), mc = 1;

  // initialize expected number of bindings
  b.Clear();
  b.expect = cond.NumItems();

  // do matching (wmem passed to match_found in member variable)
  wmem = &f;
  show = dbg;
  bth = mth;
  n = MatchGraph(&b, mc, cond, f);
  wmem = NULL;

  // record range of halo assertions made
  src0 = 0;
  src1 = 0;
  if ((s1 = f.NumHalo()) > s0)
  {
    src0 = s0;
    src1 = s1;
  }
  return n;
}


//= Instantiate result int halo using bindings given.
// ignores mc, assumes only one set of bindings in list
// wmem should be previously bound by FindMatches
// returns 1 if successful, 0 if problem

int jhcAliaRule::match_found (jhcBindings *m, int& mc) 
{
  const jhcNetNode *trig;
  int i, n, cnt, tval = 0;

  // find biggest "top" marking of triggering conditions
  n = m->NumPairs();
  for (i = 0; i < n; i++)
  {
    trig = m->GetSub(i);
    tval = __max(tval, trig->top);
  }

  // build network for result in halo
  if (wmem == NULL)
    return 0;
  cnt = wmem->AssertHalo(result, *m, conf, tval);

  // possibly show debugging info
  if (show > 0)
  {
    jprintf("  RULE %d ==> ", id);
    if (cnt > 0)
      wmem->PrintHalo(wmem->HaloMark(), -2);
    else
      jprintf("already known\n\n");
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              File Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Read at current location in a file to fill in details of self.
// returns: 1 = successful, 0 = syntax error, -1 = end of file, -2 = file error 

int jhcAliaRule::Load (jhcTxtLine& in)
{
  int ans;

  if (in.NextContent() == NULL)
    return -1;
  ClrTrans();
  ans = load_clauses(in);
  ClrTrans(0);
  return((in.Error()) ? -2: ans);
}


//= Extract "if", "unless" and "then" parts of rule.
// returns 1 if successful, 0 for format problem, -1 for file error

int jhcAliaRule::load_clauses (jhcTxtLine& in)
{
  int ans;

  // precondition
  if (!in.Begins("if:"))
    return 0;
  in.Skip("if:");
  if ((ans = LoadGraph(&cond, in)) <= 0)
    return ans;

  // caveats
  nu = 0;
  while (in.Begins("unless:"))
  {
    in.Skip("unless:");
    if ((ans = LoadGraph(&(unless[nu]), in)) <= 0)
      return ans;
    if (nu++ >= umax)
    {
      jprintf("More than %d caveats in jhcAliaRule::load_clauses!\n", umax);
      break;
    }
  }

  // get result confidence (defaults to 1.0)
  if (in.Begins("conf:"))
  {
    in.Skip("conf:");
    if (sscanf_s(in.Head(), "%lf", &conf) != 1)
      return 0;
    if (in.Next(1) == NULL)
      return 0;
  }

  // consequent
  if (!in.Begins("then:"))
    return 0;
  in.Skip("then:");
  return LoadGraph(&result, in);
}


//= Save self out in machine readable form to current position in a file.
// detail: 0 no extras, 1 show belief, 2 show tags, 3 show both
// return: 1 = successful, 0 = bad format, -1 = file error

int jhcAliaRule::Save (FILE *out, int detail) const
{
  char num[20] = "";
  int i;

  // rule number
  if (id > 0)
    _itoa_s(id, num, 10);
  jfprintf(out, "// RULE %s\n", num);

  // precondition
  jfprintf(out, "    if: ");
  cond.Save(out, -8, detail);
  jfprintf(out, "\n");

  // caveats
  for (i = 0; i < nu; i++)
  {
    jfprintf(out, "unless: ");
    unless[i].Save(out, -8, detail);
    jfprintf(out, "\n");
  }

  // confidence
  if (conf != 1.0)
    jfprintf(out, "  conf: %4.2f\n", conf);

  // consequent
  jfprintf(out, "  then: ");
  result.Save(out, -8, detail);
  jfprintf(out, "\n\n");
  return((ferror(out) != 0) ? -1 : 1);
}

