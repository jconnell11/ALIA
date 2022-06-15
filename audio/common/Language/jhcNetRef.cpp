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
  BuildIn(&cond);
  focus = NULL;
  partial = NULL;
  refmode = 1;
//dbg = 1;               // local debug statements (2 for matcher)
}


///////////////////////////////////////////////////////////////////////////
//                       Language Interpretation                         //
///////////////////////////////////////////////////////////////////////////

//= Return working memory node matching description of main node in graphlet.
// attempts to resolve description to some pre-existing node if "fmode" > 0
// can optionally re-use "f0" node from "add" pool as place to copy structure
// automatically makes a new node, if needed, with all properties true
// "fmode": -1 = always create new item (not used)
//           0 = always make FIND
//           1 = always make BIND (create > 0)
//           2 = resolve locally else create item (resolve > 0)

jhcNetNode *jhcNetRef::FindMake (jhcNodePool& add, int fmode, jhcNetNode *f0, 
                                 double blf, jhcAliaChain **skolem)
{
  jhcBindings b;
  jhcNetNode *var = NULL;
  const jhcNetNode *main = cond.Main();
  const jhcNodeList *f2 = ((univ != &add) ? univ : NULL);
  int pend = (((skolem != NULL) && (*skolem != NULL)) ? 1 : 0); 
  int n0, find = fmode, got = 0, mc = 1; 

//dbg = 1;                             // quick debugging

  // never try to FIND/BIND something with no description (primarily "it")
  // Note: now generates FIND[ x ] which is resolved in jhcAliaDir::complete_find
//  if ((find >= 0) && (cond.NumItems() == 1) && (main->Lex() == NULL) && (main->NumArgs() <= 0))
//    find = 2;
  
  // possibly tell what is sought
  if (dbg >= 1)
  {
    jprintf("\nNetref [%d] >= %4.2f\n", find, bth);
    cond.Print("pattern", 2);
  }

  // must have some accumulator in order to find new assertions
  partial = add.Accum();
  if (((find == 0) || (find == 1)) && (partial == NULL))
  {
    jprintf(">>> no accumulator in jhcNetRef::FindMake !!!\n");
    return NULL;
  }

  // set up head node and clear best binding
  focus = cond.Main();
  if (f0 != NULL)
    b.Bind(focus, f0);
  win.Copy(b);
  recent = -1;

  // always look for existing node if find > 1 (was 0 prior to 11/21) 
  b.expect = cond.NumItems();
  if ((find <= 1) || ((got = MatchGraph(&b, mc, cond, add, f2)) <= 0))
  {
    // nothing found now (or did not look)
    if (partial != NULL)
      n0 = partial->NumItems();
    add.Assert(cond, win, blf, 0, univ);         // force belief
    if ((find == 0) || (find == 1))
    {
      // add a new FIND to chain instead of creating
      if ((var = append_find(n0, blf, skolem, find)) == NULL)
        return NULL;
      jprintf(1, dbg, "  ==> %s from new %s\n", ((find > 0) ? "BIND" : "FIND"), var->Nick());
      return add.MarkRef(var);                   // user speech
    }
  }

  // possibly tell result and source
  jprintf(1, dbg, " ==> %s %s %s\n", ((got > 0) ? "existing" : "created"), 
          win.LookUp(focus)->Nick(), ((pend > 0) ? "(purge FINDs)" : ""));
  return add.MarkRef(win.LookUp(focus));         // user speech
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
  dir = new jhcAliaDir((assume > 0) ? JDIR_BIND : JDIR_FIND);     
  ch->BindDir(dir);

  // copy new parts of description (from Assert) to key of directive
  dk = &(dir->key);
  shell = univ->BuildIn(dk);
  partial->CutTail(trim, n0);
  univ->Assert(trim, mt, blf, 0, univ);      
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
  jhcNetNode *mate = m->LookUp(focus);
  int when = mate->LastRef();

  // outer graphlet items are usually incomplete so reject them
  if (mate->String())
    return 0;
  if ((partial != NULL) && partial->InList(mate))
    return 0;
  if (focus->ObjNode() && !mate->ObjNode())      // too restrictive?
    return 0;

  // prefer most recently mentioned mate for focus
  jprintf(2, dbg, "MATCH - %s %s\n", mate->Nick(), ((when > recent) ? "keep!" : "ignore")); 
  if (when > recent)
  {
    recent = when;
    win.Copy(*m);
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Language Generation                          //
///////////////////////////////////////////////////////////////////////////

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

