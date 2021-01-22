// jhcAliaRule.cpp : declarative implication in ALIA System 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#include <math.h>

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
  // core information
  next = NULL;
  id = 0;
  lvl = 3;             // default = newly told

  // run-time status
  nh = 0;
  show = 0;
}


///////////////////////////////////////////////////////////////////////////
//                             Main Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Find all variable bindings that cause the situation to match memory.
// conditions must have blf >= mth or blf = 0.0 (useful for hypotheticals)
// each successful match asserts result into halo with blf from rule
// several versions of a fact (and its negation) can be simultaneously present
// returns total number of results newly asserted

int jhcAliaRule::AssertMatches (jhcWorkMem& f, double mth, int add, int noisy)
{
  int i, mc, nh0, ni = cond.NumItems();

  // possibly preserve bindings from previous round (e.g. halo-1)
  if (add <= 0)
    nh = 0;
  mc = hmax - nh;
  nh0 = nh;

  // initialize expected number of bindings for all potential instantiations
  for (i = 0; i < mc; i++)
  {
    hinst[i].Clear();
    hinst[i].expect = ni;
  }

  // do matching (wmem passed to match_found in member variable)
  wmem = &f;
  show = noisy;
  bth = -mth;                              // blf = 0 for cond is ok
  nh += MatchGraph(hinst, mc, cond, f);     
  wmem = NULL;
  return(nh - nh0);
}


//= Instantiate result in halo using bindings given.
// wmem should be previously bound by FindMatches
// returns 1 if successful, 0 if match not useful

int jhcAliaRule::match_found (jhcBindings *m, int& mc) 
{
  jhcGraphlet inf;
  jhcBindings *b = &(m[mc - 1]);

  // reject if same result already asserted by another instantiation
  // otherwise build network for result (no arbitration with other halo facts)
  if ((wmem == NULL) || same_result(m, mc))
    return 0;
  wmem->AssertHalo(result, *b, this);

  // possibly show accepted rule result 
  if (show > 0) 
  {
    jprintf("  RULE %d%c ==>", id, 'a' + (hmax - mc));
    Inferred(inf, b);
    inf.Print(4, 1);
    jprintf("\n\n");
  }

  // shift to next set of bindings (this set preserved)
  if (mc <= 1)
    jprintf(">>> More than %d halo instantiations in jhcAliaRule::match_found !\n", hmax);
  else
    mc--;                    
  return 1;                  
}


//= Whether most recent binding set gives a halo result identical to some earlier binding set.

bool jhcAliaRule::same_result (const jhcBindings *m, int mc) const
{
  const jhcBindings *b = &(m[mc - 1]);
  const jhcNetNode *key;
  int i, j, n0 = b->NumPairs();

  // reject if same result already asserted by another instantiation
  for (j = hmax - 1; j >= mc; j--)
  {
    for (i = 0; i < n0; i++)
    {
      // look for some result variable bound to a different substitution
      key = b->GetKey(i);
      if (result.InDesc(key) && (b->GetSub(i) != m[j].LookUp(key))) 
        break;
    }
    if (i >= n0)             // all relevants vars the same
      return true;
  }
  return false;              // nothing gave identical result
}


//= Fill supplied graphlet with full rule results using supplied bindings.

void jhcAliaRule::Inferred (jhcGraphlet& key, const jhcBindings& b) const
{
  jhcNetNode *item, *sub;
  int i, ni = result.NumItems();

  // copy all final results
  for (i = 0; i < ni; i++)
  {
    item = result.Item(i);
    if ((sub = b.LookUp(item)) != NULL)
      key.AddItem(sub);
    else
      key.AddItem(item);
  }
}


///////////////////////////////////////////////////////////////////////////
//                           Halo Consolidation                          //
///////////////////////////////////////////////////////////////////////////

//= Augment consolidated rule based on a rule used as one of the first steps.
// also keeps a mapping "m2c" of correspondence between old and new nodes

void jhcAliaRule::AddCombo (jhcBindings& m2c, const jhcAliaRule& step1, const jhcBindings& b1)
{
  const jhcGraphlet *c1 = &(step1.cond);
  const jhcNetNode *mem;
  int i, nc = c1->NumItems();

  // check each halo node used in rule instantiation
  for (i = 0; i < nc; i++)
    if ((mem = b1.LookUp(c1->Item(i))) != NULL)  
      add_equiv(m2c, cond, mem);
}


//= Finish off consolidated rule based on rule used as the second step.

void jhcAliaRule::LinkCombo (jhcBindings& m2c, const jhcAliaRule& step2, const jhcBindings& b2)
{
  const jhcGraphlet *c2 = &(step2.cond), *r2 = &(step2.result);
  const jhcNetNode *mem;
  jhcNetNode *combo;
  double cf = 0.0;
  int i, nc = c2->NumItems(), nr = r2->NumItems();

  // add non-halo step2 cond nodes and find most fragile intermediate
  for (i = 0; i < nc; i++)
    if ((mem = b2.LookUp(c2->Item(i))) != NULL) 
      if (!mem->Halo())
        add_equiv(m2c, cond, mem);
      else if ((cf <= 0.0) || (mem->Belief() < cf))
        cf = mem->Belief();
  insert_args(m2c, cond);

  // copy step2 result (note that insert_args may change nr)
  for (i = 0; i < nr; i++)
    if ((mem = b2.LookUp(r2->Item(i))) != NULL)  
      add_equiv(m2c, result, mem);
  insert_args(m2c, result);

  // no result can be more sure than most fragile intermediate 
  nr = result.NumItems();
  for (i = 0; i < nr; i++)
  {
    combo = result.Item(i);
    if (!combo->LexNode() && (combo->Belief() > cf))
      combo->SetBelief(cf);
  }
}


//= Add nodes to description if some argument (or lex property) of a node is missing.
// builds "m2c" translation from original to newly created nodes

void jhcAliaRule::insert_args (jhcBindings& m2c, jhcGraphlet& desc)
{
  const jhcNetNode *mem, *lex;
  jhcNetNode *combo;
  int i, j, np, na;

  // check all items in description (more might be added during loop)
  for (i = 0; i < desc.NumItems(); i++)
  {
    // add argument links inside rule if node is known (should always be)
    combo = desc.Item(i);
    mem = m2c.FindKey(combo);

    // make sure lex property is copied
    np = mem->NumProps();
    for (j = 0; j < np; j++)
    {
      lex = mem->Prop(j);
      if (lex->LexNode())
        add_equiv(m2c, desc, lex);
    }

    // make sure all arguments are copied
    na = mem->NumArgs();
    for (j = 0; j < na; j++)
      combo->AddArg(mem->Slot(j), add_equiv(m2c, desc, mem->Arg(j)));
  }
}


//= Get equivalent rule node (if exists) for main memory node, otherwise make up new one.
// builds "m2c" translation from original to newly created nodes
// node always added to "desc" graphlet 

jhcNetNode *jhcAliaRule::add_equiv (jhcBindings& m2c, jhcGraphlet& desc, const jhcNetNode *probe)
{
  jhcNetNode *equiv;

  if ((equiv = m2c.LookUp(probe)) == NULL)
  {
    equiv = MakeNode(probe->Kind(), NULL, probe->Neg(), -(probe->Belief()));
    m2c.Bind(probe, equiv);
  }
  desc.AddItem(equiv);
  return equiv;
}


//= Determine if some other rule essentially matches this one.
// ignores differences in belief between nodes in result
// only guards against EXACT duplicate with items in SAME order
// really only works on duplicate consolidation attempts

bool jhcAliaRule::Identical (const jhcAliaRule& ref) const
{
  const jhcGraphlet *c = &(ref.cond), *r = &(ref.result); 
  int i, nc = cond.NumItems(), nr = result.NumItems();

  // check that parts are the same size
  if ((c->NumItems() != nc) || (r->NumItems() != nr))
    return false;

  // see if preconditions are nearly identical
  for (i = 0; i < nc; i++)
    if (!same_struct(cond.Item(i), c->Item(i)))
      break;
  if (i < nc)
    return false;

  // see if results are nearly identical
  for (i = 0; i < nr; i++)
    if (!same_struct(result.Item(i), r->Item(i)))
      break;
  return(i >= nr);
}


//= Check if nodes from two rules are roughly similar and have similar argument structure.
// really only works on duplicate consolidation attempts

bool jhcAliaRule::same_struct (const jhcNetNode *focus, const jhcNetNode *mate) const
{
  int i, na = focus->NumArgs();

  if ((mate->Neg() != focus->Neg()) || mate->LexConflict(focus) || (mate->NumArgs() != na))   
    return false;
  for (i = 0; i < na; i++)
    if (((focus->Arg(i))->Inst() != (mate->Arg(i))->Inst()) ||   // assumes same numbering!
        (strcmp(focus->Slot(i), mate->Slot(i)) != 0))
      return false;
  return true;
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


//= Extract "if", "unless", and "then" parts of rule.
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

  // main consequent
  if (!in.Begins("then:"))
    return 0;
  in.Skip("then:");
  if ((ans = LoadGraph(&result, in, 1)) <= 0)
    return ans;
  return 1;
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

  // save consequent and add blank line
  jfprintf(out, "  then: ");
  result.Save(out, -8, detail | 0x01);
  jfprintf(out, "\n\n");
  return((ferror(out) != 0) ? -1 : 1);
}

