// jhcTripleLink.cpp : properties of entities and relations between them
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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

#include <string.h>

#include "Semantic/jhcTripleLink.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.
// assumes n and fcn are non-NULL and point to valid items

jhcTripleLink::jhcTripleLink (jhcTripleNode *n, const char *fcn)
{
  jhcTripleLink *t, *t0 = NULL;

  // basic information
  topic = n;
  strcpy_s(slot, fcn);
  filler = NULL;
  *txt = '\0';

  // list defaults
  alist = NULL;
  plist = NULL;
  prev  = NULL;
  next  = NULL;
  multi = 0;

  if (n != NULL)
  {  
    // find END of argument list of topic 
    t = n->args;
    while (t != NULL)
    {
      t0 = t;
      t = t->alist;
    }
    
    // add new item
    if (t0 == NULL)
      n->args = this;
    else
      t0->alist = this;
  }
}


//= Default destructor does necessary cleanup.

jhcTripleLink::~jhcTripleLink ()
{
  // remove from argument and property lists
  rem_arg();
  rem_prop();

  // remove from double-linked history
  if (prev != NULL)
    prev->next = next;
  if (next != NULL)
    next->prev = prev;
}


//= Remove link from argument list of topic and set topic to NULL.

void jhcTripleLink::rem_arg ()
{
  jhcTripleLink *t, *t0 = NULL;

  if (topic == NULL)
    return;

  // find predecessor in list of arguments
  t = topic->args;
  while (t != NULL)
  {
    if (t == this)
      break;
    t0 = t;
    t = t->alist;
  }

  // splice out of argument list
  if (t0 == NULL)
    topic->args = alist;
  else
    t0->alist = alist;

  // locally invalidate topic node (still exists)
  topic = NULL;
  alist = NULL;
}


//= Remove link from property list of filler and set filler to NULL.

void jhcTripleLink::rem_prop ()
{
  jhcTripleLink *t, *t0 = NULL;

  if (filler == NULL)
    return;

  // find predecessor in list of properties for filler node
  t = filler->props;
  while (t != NULL)
  {
    if (t == this)
      break;
    t0 = t;
    t = t->plist;
  }

  // splice link out of property list for filler node
  if (t0 == NULL)
    filler->props = plist;
  else
    t0->plist = plist;

  // locally invalidate filler node (still exists)
  filler = NULL;
  plist = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Set the value of the slot to be the given node (overwrites old).
// returns 1 if changed, 0 if not needed

int jhcTripleLink::SetFill (jhcTripleNode *n2)
{
  jhcTripleLink *t, *t0 = NULL;

  // check given and previous values
  if (filler == n2)
    return 0;
  rem_prop();

  // set basic information
  filler = n2;
  *txt = '\0';

  // find END of property list of new value node
  t = filler->props;
  while (t != NULL)
  {
    t0 = t;
    t = t->plist;
  }

  // add new item
  if (t0 == NULL)
    filler->props = this;
  else
    t0->plist = this;
  return 1;
}


//= Set the value of the slot to be the given text (overwrites old).
// returns 1 if changed, 0 if not needed

int jhcTripleLink::SetTag (const char *tag)
{
  // check given and previous values
  if ((filler == NULL) && (_stricmp(txt, tag) == 0))
    return 0;
  rem_prop();

  // set basic information
  filler = NULL;
  strcpy_s(txt, tag);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Debugging Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Print a nice representation of the triple to the console.
// can optional prefix with tag string

void jhcTripleLink::Print (const char *tag) const
{
  char tail = ((multi > 0) ? '+' : '-');

  if (topic == NULL) 
    return;
  if (tag != NULL)
    jprintf("%s ", tag);
  if (filler != NULL)
    jprintf("%s %c-%s--> %s\n", topic->Name(), tail, slot, filler->Name());
  else 
    jprintf("%s %c-%s--> %s\n", topic->Name(), tail, slot, txt);
}


//= Save a version of the triple with tabs between components to a file.
// returns 1 if something written, 0 for problem

int jhcTripleLink::Tabbed (FILE *out) const
{
  char post[10] = "++";

  if ((out == NULL) || (topic == NULL))
    return 0;
  if (multi <= 0)
    *post = '\0';
  if (filler != NULL)
    fprintf(out, "%s\t%s\t%s\t%s\n", topic->Name(), slot, filler->Name(), post);
  else
    fprintf(out, "%s\t%s\t%s\t%s\n", topic->Name(), slot, txt, post);
  return 1;
}

