// jhcNetRef.cpp : holds a fragment of network to be looked up in main memory
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#include "jhcNetRef.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcNetRef::~jhcNetRef ()
{

}


//= Default constructor initializes certain values.

jhcNetRef::jhcNetRef ()
{
  BuildIn(&cond);
  refmode = 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Return working memory node matching description of main node in graphlet.
// can optionally reuse "f0" node from "add" pool as place to copy structure
// automatically makes a new node, if needed, with all properties true

jhcNetNode *jhcNetRef::FindMake (jhcNodePool& add, int find, jhcNetNode *f0)
{
  jhcBindings b;
  int mc = 1;

  // set up head node and clear best binding
  focus = cond.Main();
  if (f0 != NULL)
    b.Bind(focus, f0);
  win.Copy(b);
  recent = -1;

  // possibly look for compatible existing node
  b.expect = cond.NumItems();
  bth = 0.0;                                                   // abstract
  if ((find <= 0) || (MatchGraph(&b, mc, cond, add) <= 0))
    add.Assert(cond, win, -1.0);                               // force to true
  return add.MarkRef(win.LookUp(focus));
}


//= Save the match with most recent node associated with focus.
// prospective match always in m[0], saves best bindings in "win"

int jhcNetRef::match_found (jhcBindings *m, int& mc) 
{
  jhcNetNode *mate = m->LookUp(focus);
  int when = mate->LastRef();

  if (when > recent)
  {
    recent = when;
    win.Copy(*m);
  }
  return 1;
}