// jhcAliaRule.cpp : declarative implication in ALIA System 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#include <math.h>
#include <ctype.h>

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
  *gist = '\0';
  id = 0;
  lvl = 3;             // default = newly told
  conf = 1.0;

  // learned overrides
  *prov = '\0';
  pnum = 0;
  conf0 = 1.0;

  // run-time status
  wmem = NULL;
  nh = 0;
  show = 0;
}


///////////////////////////////////////////////////////////////////////////
//                            Simple Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Remember human readable utterance that generated this rule.

void jhcAliaRule::SetGist (const char *sent)
{
  char *end;

  *gist = '\0';
  if (sent == NULL)
    return;
  strcpy_s(gist, ((*sent == '"') ? sent + 1 : sent));
  gist[0] = (char) toupper(gist[0]);
  if ((end = strrchr(gist, '"')) != NULL)
    *end = '\0';
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
  jprintf(2, dbg, "Rule %d matching (%4.2f) ...\n", id, -mth);
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
  bth = -mth;                              // hypothetical for cond is ok
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
  const jhcNetNode *n;
  int i, dup, nb = b->NumPairs(), h = b->AnyHyp(), tval = 0, ver = 0;

  // find most recent NOTE associated with preconditions (if any)
  // then see if this same result has already been posted by some other rule
  hyp[mc - 1] = h;
  for (i = 0; i < nb; i++)
    if ((n = b->GetSub(i)) != NULL)
    {
      ver  = __max(ver, n->Generation());
      tval = __max(tval, n->top);   
    }
  if ((dup = same_result(m, mc, tval)) < 0)
    return jprintf(2, dbg, "%*s ignore - same effect as bindings[%d]\n", 2 * nb + 2, "", -dup);

  // same result but this set of bindings has more relevant tval for some precondition 
  if (dup > 0)
  {
    for (i = 0; i < nb; i++)
      m[dup].SetSub(i, b->LookUp(m[dup].GetKey(i)));       // in case order is different
    init_result(&(m[dup]), tval, ver, h);
    return jprintf(2, dbg, "%*s substitute - same effect as bindings[%d]\n", 2 * nb + 2, "", dup);
  }

  // otherwise create new result nodes in halo 
  jprintf(2, dbg, "%*s ... FULL MATCH = new bindings[%d]\n", 2 * nb + 1, "", mc - 1);
  wmem->AssertHalo(result, *b);
  init_result(b, tval, ver, h);

  // possibly show accepted rule result 
  if (show > 0) 
  {
    jprintf("  RULE %d%c ==>", id, 'a' + (hmax - mc));
    Inferred(inf, b);
    inf.Print(4, -1);
    jprintf("\n\n");
  }

  // shift to next set of bindings (this set preserved)
  if (mc <= 1)
    jprintf(">>> More than %d halo instantiations of Rule %d in jhcAliaRule::match_found !\n", hmax, id);
  else
    mc--;    
  return 1;                  
}


//= Whether most recent binding set gives a halo result identical to some earlier binding set.
// generally "b" has only precondition variables while m[j] also has (halo) result bindings 
// returns 0 if no duplicate, negative index if earlier has better tval, positive if lower tval

int jhcAliaRule::same_result (const jhcBindings *m, int mc, int t0) const
{
  const jhcBindings *b = &(m[mc - 1]);
  const jhcNetNode *pn, *n;
  int i, j, nb = b->NumPairs(), h = hyp[mc - 1], tval = 0;

  // reject if same result already asserted by another instantiation
  for (j = hmax - 1; j >= mc; j--)
    if (hyp[j] == h)
    {
      // look for some variable used in result but bound to a different substitution
      for (i = 0; i < nb; i++)
      {
        pn = b->GetKey(i);
        n = m[j].LookUp(pn);
        if ((b->GetSub(i) != n) && result_uses(pn))
          break;
        tval = __max(tval, n->top);    // get max "top" over preconditions
      }

      // if all relevants vars the same, check if some better top value
      if (i >= nb)       
        return((tval >= t0) ? -j : j); 
    }
  return 0;                            // nothing gave identical result
}


//= See if instantiated result will use the binding for some pattern node.

bool jhcAliaRule::result_uses (const jhcNetNode *key) const
{
  const jhcNetNode *item;
  int i, j, na, nr = result.NumItems();

  for (i = 0; i < nr; i++)
  {
    if ((item = result.Item(i)) == key)
      return true;
    na = item->NumArgs();
    for (j = 0; j < na; j++)
      if (item->Arg(j) == key)
        return true;
  }
  return false;
}


//= Update top marker and generation number, set initial belief, and record provenance.

void jhcAliaRule::init_result (jhcBindings *b, int tval, int ver, int zero) 
{
  const jhcNetNode *pn;
  jhcNetNode *n;
  int i, nr = result.NumItems();

  for (i = 0; i < nr; i++)
  {
    pn = result.Item(i);
    if ((n = b->LookUp(pn)) != NULL)
      if (n->Halo() && !cond.InDesc(n))
      {
        wmem->SetGen(n, ver);                    // inferred result recency
        n->TopMax(tval);                         // associate with some NOTE
        n->SetDefault(pn->Default());
        n->TmpBelief((zero > 0) ? 0.0 : n->Default());
        n->hrule = this;
        n->hbind = b;
      }
  }
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
// basically copies cond of step1 to cond of new rule, making equivalent nodes as needed
// also keeps a mapping "m2c" of correspondence between old and new nodes

void jhcAliaRule::AddCombo (jhcBindings& m2c, const jhcAliaRule& step1, const jhcBindings& b1)
{
  const jhcGraphlet *c1 = &(step1.cond);
  const jhcNetNode *mem;
  int i, nc = c1->NumItems();

  for (i = 0; i < nc; i++)
    if ((mem = b1.LookUp(c1->Item(i))) != NULL)  
      cond.AddItem(get_equiv(m2c, mem, 0));
}


//= Finish off consolidated rule based on single rule used as the second step.

void jhcAliaRule::LinkCombo (jhcBindings& m2c, const jhcAliaRule& step2, const jhcBindings& b2)
{
  const jhcGraphlet *c2 = &(step2.cond), *r2 = &(step2.result);
  const jhcNetNode *mem, *fact;
  int i, nc = c2->NumItems(), nr = r2->NumItems();

  // add non-halo step2 cond nodes and find most fragile intermediate (conf)
  conf = step2.conf;
  for (i = 0; i < nc; i++)
    if ((mem = b2.LookUp(c2->Item(i))) != NULL) 
    {
      // see if a required combo precondition (not from step1 result)
      if (!mem->Halo())
        cond.AddItem(get_equiv(m2c, mem, 0));              
      else
      {
        // get result belief from step1 rule (might have been applied to hyp)       
        fact = (mem->hbind)->FindKey(mem);
        if (fact->Belief() < conf)
          conf = fact->Belief();
        get_equiv(m2c, mem, 1);        // might be an arg in combo result      
      }
    }
  connect_args(cond, m2c);

  // copy step2 result and link in needed arguments
  for (i = 0; i < nr; i++)
    if ((mem = b2.LookUp(r2->Item(i))) != NULL)  
      result.AddItem(get_equiv(m2c, mem, 1));
  connect_args(result, m2c);
  result.RemAll(cond);
  result.ForceBelief(conf);
}


//= Get equivalent rule node (if exists) for main memory node, otherwise make up new one.
// new node has belief copied from original (for result confidences) or set to 1 (preconditions)
// incrementally builds "m2c" translation from original to any newly created nodes
// returns pointer to new (or old) node which can optionally be added to some description

jhcNetNode *jhcAliaRule::get_equiv (jhcBindings& m2c, const jhcNetNode *probe, int bcpy)
{
  const jhcNetNode *fact;
  jhcNetNode *equiv;
  double blf = 1.0;

  if ((bcpy > 0) && probe->Halo())
  {
    fact = (probe->hbind)->FindKey(probe);       // rule might have been applied to hyp
    blf = fact->Belief();
  }
  if ((equiv = m2c.LookUp(probe)) == NULL)
  {
    equiv = MakeNode(probe->Kind(), probe->Lex(), probe->Neg(), -blf);   
    m2c.Bind(probe, equiv);
  }
  return equiv;
}


//= Makes sure newly created rule nodes are connected in same pattern as original memory nodes.
// adds missing arguments to description (e.g. from intermediate results)

void jhcAliaRule::connect_args (jhcGraphlet& desc, const jhcBindings& m2c) const
{
  const jhcNetNode *mem;
  jhcNetNode *combo, *carg;
  int i, j, na;

  // check all items in description (more might be added during loop)
  for (i = 0; i < desc.NumItems(); i++)
  {
    // check arguments of associated main memory node 
    combo = desc.Item(i);
    mem = m2c.FindKey(combo);
    na = mem->NumArgs();
    for (j = 0; j < na; j++)
      if ((carg = m2c.LookUp(mem->Arg(j))) != NULL)
      {
        // link the description node to a corresponding argument
        combo->AddArg(mem->Slot(j), carg);             
        desc.AddItem(carg);
      }
  }
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

  if ((mate->Neg() != focus->Neg()) || !focus->LexMatch(mate) || (mate->NumArgs() != na))   
    return false;
  for (i = 0; i < na; i++)
    if (((focus->Arg(i))->Inst() != (mate->Arg(i))->Inst()) ||   // assumes same numbering!
        (strcmp(focus->Slot(i), mate->Slot(i)) != 0))
      return false;
  return true;
}    


//= Determine if rule uselessly infers X -> X (sometimes from consolidation).
// sees if result satisfies condition (and no more)

bool jhcAliaRule::Tautology () 
{
  jhcSituation sit;
  jhcBindings m;
  int mc = 1;

  // copy just precondition nodes to a new situation
  sit.BuildCond();
  sit.Assert(cond, m);
  m.Clear();

  // see if situation gets a match with rule result
  m.expect = cond.NumItems();
  sit.bth = -1.0;
  return(sit.MatchGraph(&m, mc, *(sit.Pattern()), result) > 0);
}


///////////////////////////////////////////////////////////////////////////
//                              File Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Read at current location in a file to fill in details of self.
// returns: 1 = okay, 0 = syntax error, -1 = end of file, -2 = file error 

int jhcAliaRule::Load (jhcTxtLine& in)
{
  const char *item;
  int ans;

  // required header ("RULE <pnum> - <gist>" where gist is optional)
  if (in.NextContent() == NULL)
    return -1;
  if (((item = in.Token()) == NULL) || (_stricmp(item, "RULE") != 0))
    return 0;
  if (((item = in.Token()) == NULL) || (sscanf_s(item, "%d", &pnum) != 1))
    return 0;
  if (((item = in.Token()) != NULL) && (strcmp(item, "-") == 0))
    SetGist(in.Head());

  // body of rule
  in.Flush();
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

  // get result confidence (defaults to 1.0)
  if (in.Begins("conf:"))
  {
    in.Skip("conf:");
    if (sscanf_s(in.Head(), "%lf", &conf) != 1)
      return 0;
    conf0 = conf;
    if (in.Next(1) == NULL)
      return 0;
  }

  // main consequent
  if (!in.Begins("then:"))
    return 0;
  in.Skip("then:");
  if ((ans = LoadGraph(&result, in, 1)) <= 0)
    return ans;
  result.ForceBelief(conf);
  result.ActualizeAll(0);              // needed for match_found
  return 1;
}


//= Save self out in machine readable form to current position in a file.
// detail: 0 no extras, 1 show belief, 2 show tags, 3 show both
// return: 1 = successful, 0 = bad format, -1 = file error

int jhcAliaRule::Save (FILE *out, int detail) const
{
  int i;

  // header ("RULE <id> - <gist>") and optional provenance
  if ((detail >= 2) && (*prov != '\0'))
    jfprintf(out, "// originally rule %d from %s\n\n", pnum, prov);  
  jfprintf(out, "RULE");
  if (id > 0)
    jfprintf(out, " %d", id);
  if ((detail >= 2) && (*gist != '\0'))
    jfprintf(out, " - \"%s\"", gist);
  jfprintf(out, "\n");

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
    jfprintf(out, "  conf: %5.3f\n", conf);

  // save consequent and add blank line
  jfprintf(out, "  then: ");
  result.Save(out, -8, detail);
  jfprintf(out, "\n");
  return((ferror(out) != 0) ? -1 : 1);
}

