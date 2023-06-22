// jhcNetRef.cpp : holds a fragment of network to be looked up in main memory
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#include "Language/jhcMorphTags.h"     // common audio

#include "Language/jhcNetRef.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcNetRef::~jhcNetRef ()
{
}


//= Default constructor initializes certain values.

jhcNetRef::jhcNetRef (jhcNodePool *u, double bmin)
{
  univ = u; 
  bth = bmin;
  BuildIn(cond);
  focus = NULL;
  partial = NULL;
  refmode = 1;
  halo = 1;
//dbg = 3;               // local debug statements (2 or 3 for matcher)
}


///////////////////////////////////////////////////////////////////////////
//                       Language Interpretation                         //
///////////////////////////////////////////////////////////////////////////

//= Return working memory node matching description of main node in graphlet.
// attempts to resolve description to some pre-existing node if "fmode" > 0
// can optionally re-use "f0" node from "add" pool as place to copy structure
// automatically makes a new node, if needed, with all properties true
// "fmode": -1 = always create new item (not used as input)
//           0 = resolve locally else make FIND
//           1 = resolve locally else make BIND (create > 0)
//           2 = resolve locally else create item (resolve > 0)
//           3 = always make EACH/ANY

jhcNetNode *jhcNetRef::FindMake (jhcNodePool& add, int fmode, jhcNetNode *f0, 
                                 double blf, jhcAliaChain **skolem)
{
  jhcBindings b;
  jhcNetNode *var = NULL;
  const jhcNodeList *f2 = ((univ != &add) ? univ : NULL);
  int pend = (((skolem != NULL) && (*skolem != NULL)) ? 1 : 0); 
  int n0 = 0, find = fmode, got = 0, mc = 1; 

//dbg = 1;                             // quick debugging

  // possibly tell what is sought
  if (dbg >= 1)
  {
    jprintf("\nNetRef [find %d] >= %4.2f\n", find, bth);
    cond.Print("pattern", 2, 3);
  }

  // must have some accumulator in order to find new assertions
  partial = add.Accum();
  if (((find == 0) || (find == 1) || (find == 3)) && (partial == NULL))
  {
    jprintf(">>> No accumulator in jhcNetRef::FindMake !!!\n");
    return NULL;
  }

  // set up head node and clear best binding
  focus = cond.Main();
  if (f0 != NULL)
  {
    b.Bind(focus, f0);
    find = -1;                                   // skip skolem addition
  }
  win.Copy(b);
  limit = univ->LocalConvo();                    // search within sentence
  recent = limit;                                

  // generally look for existing node 
  b.expect = cond.NumItems();
  if ((find < 0) || (find >= 3) || ((got = MatchGraph(&b, mc, cond, add, f2)) <= 0))
  {
    // nothing found now (or did not look)
    if (partial != NULL)
      n0 = partial->NumItems();
    add.Assert(cond, win, blf, 0, univ);         // force belief
    if ((find == 0) || (find == 1) || (find == 3))
    {
      // add a new FIND to chain instead of creating
      if ((var = append_find(n0, blf, skolem, find)) == NULL)
        return NULL;
      jprintf(1, dbg, "  ==> %s for new %s\n", ((find > 0) ? "BIND" : "FIND"), var->Nick());
      var->MarkConvo();                          // user speech
      return var;
    }
  }

  // possibly tell result and source
  jprintf(1, dbg, " ==> %s %s %s\n", ((got > 0) ? "existing" : "created"), 
          win.LookUp(focus)->Nick(), ((pend > 0) ? "(purge FINDs)" : ""));
  var = win.LookUp(focus);
  var->MarkConvo();                              // user speech
  return var;
}


//= Construct an appropriate FIND/BIND directive from newly added description.
// generally makes a FIND directive unless assume > 0 (BIND can create if none found)
// gets added to end of chain (if any), returns head of chain

jhcNetNode *jhcNetRef::append_find (int n0, double blf, jhcAliaChain **skolem, int assume)
{
  jhcGraphlet trim;
  jhcBindings mt;
  jhcAliaChain *ch;
  jhcAliaDir *dir;
  jhcGraphlet *shell, *dk;
  const jhcNetNode *item;
  int i, nc, n = partial->NumItems();

  // create a new FIND directive for list
  if ((skolem == NULL) || (n <= n0))
    return NULL;
  ch = new jhcAliaChain;
  dir = new jhcAliaDir(JDIR_BIND);     // was FIND for assume <= 0      
  ch->BindDir(dir);

  // copy new parts of description (from Assert) to key of directive
  dk = &(dir->key);
  shell = univ->BuildIn(dk);
  partial->CutTail(trim, n0);
  univ->Assert(trim, mt, blf, 0, univ);      // copy tags?
  univ->BuildIn(shell);

  // remove any originally external nodes from skolem directive key
  nc = cond.NumItems();
  for (i = 0; i < nc; i++)
  {
    item = cond.Item(i);
    if (dk->InDesc(item))
      dk->RemItem(item);
  }

  // tack new FIND onto end of previous chain (if any)
  if (*skolem == NULL)
    *skolem = ch;
  else
    (*skolem)->Append(ch);
  return dk->Main();
}


//= Save the match with most recent node associated with focus.
// prospective match always in m[0], saves best bindings in "win"

int jhcNetRef::match_found (jhcBindings *m, int& mc) 
{
  jhcNetNode *sub, *mate = m->LookUp(focus);
  int i, nb = m->NumPairs(), when = mate->LastConvo();

  // do not increase count if same binding found through different path
  if ((when <= limit) || win.Same(*m))
    return 0;

  // handle special case of naked FIND/BIND with just a target variable: FIND[ x ]
  if ((cond.NumItems() == 1) && (focus->Lex() == NULL) && focus->ObjNode())    
    if (mate->String() || mate->Halo() || (filter_pron(mate) <= 0))
      return 0;

  // possibly reject variable matches involving halo nodes
  if (halo <= 0)
    for (i = 0; i < nb; i++)
    {
      sub = m->GetSub(i);
      if (sub->Halo())
        return 0;
    }

  // prefer most recently mentioned mate for focus
  jprintf(2, dbg, "MATCH - %s %s\n", mate->Nick(), ((when > recent) ? "keep!" : "ignore")); 
  if (when > recent)
  {
    recent = when;
    win.Copy(*m);
  }
  return 1;
}


//= Enforce any restrictions on naked node choice encoded by grammatical tags.
// saves best mate for focus in "pron" member variable and sets "recent" > 0
// returns 0 if rejected, 1 if best so far
// NOTE: modified form of jhcAliaDir::filter_pron (which does not need tag check)

int jhcNetRef::filter_pron (const jhcNetNode *mate)
{
  UL32 tags = focus->tags;

  // generally looking for a physical thing not a fact or idea
  if (!mate->ObjNode())                
  {
    if (tags != 0)                     // "that" has args
      return jprintf(2, dbg, "MATCH - but reject %s as predicate node\n", mate->Nick());
  }
  else if ((tags & JTAG_FEM) != 0)     // "she"
  {
    if ((mate->FindProp("hq", "male",   0, bth) != NULL) || ((mate->tags & JTAG_MASC) != 0) ||
        (mate->FindProp("hq", "female", 1, bth) != NULL))
      return jprintf(2, dbg, "MATCH - but reject %s as not female\n", mate->Nick());
  }
  else if ((tags & JTAG_MASC) != 0)    // "he"
  {
    if ((mate->FindProp("hq", "female", 0, bth) != NULL) || ((mate->tags & JTAG_FEM) != 0) ||
        (mate->FindProp("hq", "male",   1, bth) != NULL))
      return jprintf(2, dbg, "MATCH - but reject %s as not male\n", mate->Nick());
  }
  else if ((tags & JTAG_ITEM) != 0)    // "it"
  {
    if ((mate->FindProp("hq", "male",   0, bth) != NULL) || ((mate->tags & JTAG_MASC) != 0) ||
        (mate->FindProp("hq", "female", 0, bth) != NULL) || ((mate->tags & JTAG_FEM)  != 0))
      return jprintf(2, dbg, "MATCH - but reject %s as gendered\n", mate->Nick());
  }
  else if (tags == 0)                  // "them" has no args
    return jprintf(2, dbg, "MATCH - but reject %s as object node\n", mate->Nick());
  return 1;                
}


///////////////////////////////////////////////////////////////////////////
//                          Language Generation                          //
///////////////////////////////////////////////////////////////////////////
/*
//= See how many matches there are to description in "cond" graphlet.
// can optionally pop last few nodes off description when matching complete

int jhcNetRef::NumMatch (const jhcNodeList *wmem, double mth, int retract)
{
  jhcBindings b;
  int hits, mc = 1; 

  // possibly tell what is sought
  if (dbg >= 1)
  {
    jprintf("\nNumMatch >= %4.2f\n", mth);
    cond.Print("pattern", 2);
  }

  // set up matching parameters
  focus = cond.Main();
  b.expect = cond.NumItems();
  bth = mth;
  recent = -1;

  // do matching then possibly clean up
  hits = MatchGraph(&b, mc, cond, *wmem);
  cond.Pop(retract);
  return hits;
}
*/
