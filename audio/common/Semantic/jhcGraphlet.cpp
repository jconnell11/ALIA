// jhcGraphlet.cpp : a collection of specific semantic nodes and links
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#include "Semantic/jhcGraphlet.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcGraphlet::~jhcGraphlet ()
{
}


//= Default constructor initializes certain values.

jhcGraphlet::jhcGraphlet ()
{
  *blank = '\0';
  ni = 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Configuration                            //
///////////////////////////////////////////////////////////////////////////

//= Copy some other description using local nodes.

void jhcGraphlet::Copy (const jhcGraphlet& ref)
{
  int i;

  // copy items
  ni = ref.ni;
  for (i = 0; i < ni; i++)
    desc[i] = ref.desc[i];
}


//= Copy some other description but use node substitutions from bindings.
// returns 1 if successful, 0 if some reference nodes had no bindings

int jhcGraphlet::CopyBind (const jhcGraphlet& ref, const jhcBindings& sub)
{
  jhcNetNode *alt;
  int i, ok = 1;

  // copy items
  ni = ref.ni;
  for (i = 0; i < ni; i++)
    if ((alt = sub.LookUp(ref.desc[i])) == NULL)
      ok = 0;
    else
      desc[i] = alt;
  return ok;
}


//= Add some node (local or remote) to description.
// returns item if successful, NULL for problem

jhcNetNode *jhcGraphlet::AddItem (jhcNetNode *item)
{
  if (item == NULL)
    return NULL;
  if (ni >= gmax)
  {
    jprintf(">>> More than %d items in jhcGraphlet::Add !\n", gmax);
    return NULL;
  }
  if (!InList(item))
    desc[ni++] = item;
  return item;
}


//= Designate which node in graphlet is the most important.
// returns main node if successful, NULL if not enough space to add

jhcNetNode *jhcGraphlet::SetMain (jhcNetNode *main)
{
  int i;

  // sanity check
  if (main == NULL)
    return NULL;

  // find node in description else add it
  for (i = 0; i < ni; i++)
    if (desc[i] == main)
      break;
  if (i >= ni)
    if (AddItem(main) == NULL)
      return NULL;

  // move it to the front (swap with original)
  if (i > 0)
  {
    desc[i] = desc[0];
    desc[0] = main;
  }
  return main;
}


//= Remove old main node (if any) and set main to given node.
// returns 1 if successful, 0 for problem

int jhcGraphlet::ReplaceMain (jhcNetNode *main)
{
  if (main == NULL)
    return 0;
  desc[0] = main;
  if (ni <= 0)
   ni = 1;
  return 1;
}


//= Make the last node added be the main node of the graphlet.

jhcNetNode *jhcGraphlet::MainLast ()
{
  return SetMain(Item(NumItems() - 1));
}


//= Make sure the main node is not a naked object.
// rejects:  obj-4, self-1 -lex- you

jhcNetNode *jhcGraphlet::MainProp ()
{
  jhcNetNode *main = Main();
  int i;

  if (main != NULL)
    if (main->LexNode() || (!main->VerbTag() && (main->NumArgs() <= 0)))
      for (i = 0; i < ni; i++)
        if (!desc[i]->LexNode() && (desc[i]->VerbTag() || (desc[i]->NumArgs() > 0)))
          return SetMain(desc[i]);
  return main;
}


//= Set belief of all nodes to their pending values (blf0).
// returns number of changes made

int jhcGraphlet::ActualizeAll (int ver) const
{
  int i, chg = 0;

  for (i = 0; i < ni; i++)
    chg += desc[i]->Actualize(ver);
  return chg;
}


///////////////////////////////////////////////////////////////////////////
//                              List Access                              //
///////////////////////////////////////////////////////////////////////////

//= Returns next node in list, transitioning from halo to main if needed.
// call with prev = NULL to get first node

jhcNetNode *jhcGraphlet::NextNode (const jhcNetNode *prev) const
{
  int i;

  // search for index of previous node
  if (prev == NULL)
    return desc[0];
  for (i = 0; i < ni; i++)
    if (desc[i] == prev)
      break;

  // report next node in array (if any)
  if (++i < ni)
    return desc[i];
  return NULL;
}


//= Determine whether a given node is already in the description list.

bool jhcGraphlet::InList (const jhcNetNode *n) const
{
  int i;

  for (i = 0; i < ni; i++)
    if (desc[i] == n)
      return true;
  return false; 
}


///////////////////////////////////////////////////////////////////////////
//                           Writing Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Save description focusing on relations.
// assumes first line indented properly if lvl <= 0 (no CR on last line)
// detail: 0 no extras, 1 show belief, 2 show tags, 3 show both
// return: 1 = success, 0 = bad format, -1 = file error

int jhcGraphlet::Save (FILE *out, int lvl, int detail) const
{
  const jhcNetNode *n;
  int i, lvl2 = lvl, kmax = 3, nmax = 1, rmax = 3;

  // ignore if empty, else get printing field sizes
  if (ni <= 0)                 
    return 0;
  for (i = 0; i < ni; i++)
    desc[i]->TxtSizes(kmax, nmax, rmax);

  // format as (node -link-> arg), allow main node to be naked
  for (i = 0; i < ni; i++)
  {
    n = desc[i];
    if ((i == 0) || (n->NumArgs() > 0) || !n->Blank() ||             
        (n->Literal() != NULL) || ((n->tags != 0) && (detail >= 2)))
      lvl2 = n->Save(out, lvl2, kmax, nmax, rmax, this, detail);
  }
  return((ferror(out) != 0) ? -1 : 1);
}


