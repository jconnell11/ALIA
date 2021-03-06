// jhcNetRef.cpp : holds a fragment of network to be looked up in main memory
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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
// attempts to resolve description to some pre-existing node if "find" > 0
// can optionally re-use "f0" node from "add" pool as place to copy structure
// automatically makes a new node, if needed, with all properties true
// "find": -1 = always create new item (create > 0)
//          0 = always make FIND
//          1 = resolve locally else make FIND
//          2 = resolve locally else create item (resolve > 0)

jhcNetNode *jhcNetRef::FindMake (jhcNodePool& add, int find, jhcNetNode *f0, 
                                 double blf, jhcAliaChain **skolem)
{
  jhcBindings b;
  jhcNetNode *var = NULL;
  const jhcNodeList *f2 = ((univ != &add) ? univ : NULL);
  int pend = (((skolem != NULL) && (*skolem != NULL)) ? 1 : 0); 
  int n0, got = 0, mc = 1; 

//dbg = 1;                             // quick debugging

  // possibly tell what is sought
  if (dbg >= 1)
  {
    jprintf("\nNetref [%d] >= %4.2f\n", find, bth);
    cond.Print(2, "pattern");
  }

  // must have some accumulator in order to find new assertions
  partial = add.Accum();
  if (partial == NULL)
  {
    jprintf(">>> no accumulator in FindMake !!!\n");
    return NULL;
  }

  // set up head node and clear best binding
  focus = cond.Main();
  if (f0 != NULL)
    b.Bind(focus, f0);
  win.Copy(b);
  recent = -1;

  // always look for exisiting node if find > 0
  b.expect = cond.NumItems();
  if ((find <= 0) || ((got = MatchGraph(&b, mc, cond, add, f2)) <= 0))
  {
    // nothing found now (or did not look)
    n0 = partial->NumItems();
    add.Assert(cond, win, blf, 0, univ);         // force belief
    if ((find == 0) || (find == 1))
    {
      // add a new FIND to chain instead of creating
      var = append_find(n0, blf, skolem);
      jprintf(1, dbg, "  ==> %s from new FIND\n", var->Nick());
      return add.MarkRef(var);                   // user speech
    }
  }
  else if (skolem != NULL)
  {
    // any earlier FINDs not needed if reference resolved
    delete *skolem;
    *skolem = NULL;
  }

  // possibly tell result and source
  jprintf(1, dbg, " ==> %s %s %s\n", ((got > 0) ? "existing" : "created"), 
          win.LookUp(focus)->Nick(), ((pend > 0) ? "(purge FINDs)" : ""));
  return add.MarkRef(win.LookUp(focus));         // user speech
}


//= Construct an appropriate FIND directive from newly added description.
// gets added to end of chain (if any), returns head of chain

jhcNetNode *jhcNetRef::append_find (int n0, double blf, jhcAliaChain **skolem)
{
  jhcGraphlet trim;
  jhcBindings mt;
  jhcAliaChain *ch;
  jhcAliaDir *dir;
  jhcGraphlet *shell;
  int n = partial->NumItems();

  // create a new FIND directive for list
  if ((skolem == NULL) || (n <= n0))
    return NULL;
  ch = new jhcAliaChain;
  dir = new jhcAliaDir(JDIR_FIND);
  dir->fass = 1;                       // allow creation if not found
  ch->BindDir(dir);

  // copy new parts of description (from Assert) to key of directive
  shell = univ->BuildIn(&(dir->key));
  partial->CutTail(trim, n0);
  univ->Assert(trim, mt, blf, 0, univ);      
  univ->BuildIn(shell);

  // tack new FIND onto end of previous chain (if any)
  if (*skolem == NULL)
    *skolem = ch;
  else
    (*skolem)->Append(ch);
  return (dir->key).Main();
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
//                          Language Genration                           //
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
    cond.Print(2, "pattern");
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

