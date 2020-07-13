// jhcGramRule.cpp : expansion of a non-terminal in CFG grammar
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

#include "Parse/jhcGramRule.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcGramRule::jhcGramRule ()
{
  // basic info
  *head = '\0';
  tail = NULL; 
  status = 1;
  id = 0;

  // parse state
  dot = NULL; 
  w0 = 0; 
  wn = 0;

  // enumeration
  next = NULL; 
  mark = 0;
}


//= Default destructor does necessary cleanup.
// removes all elements in expansion also

jhcGramRule::~jhcGramRule ()
{
  jhcGramStep *s2, *s = tail;

  while (s != NULL)
  {
    s2 = s->tail;
    delete s;
    s = s2;
  }
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Copy rule expansion node by node into self.
// does not affect "mark" field at all
// returns 1 if successful, 0 for allocation failure

int jhcGramRule::CopyState (const jhcGramRule *r) 
{
  jhcGramStep *st, *st0 = NULL, *rt = r->tail;

  // copy result and initialize dot to end
  if (r == NULL)
    return 0;
  strcpy_s(head, r->head);
  status = r->status;
  id = r->id;
  dot = NULL;

  // copy expansion
  while (rt != NULL)
  {
    // make new expansion node 
    if ((st = new jhcGramStep) == NULL)
      return 0;
    strcpy_s(st->symbol, rt->symbol);
    st->non = rt->non;
    st->back = rt->back;

    // add to expansion list
    if (st0 != NULL)
      st0->tail = st;
    else
      tail = st;

    // see if this is the dotted location
    if (r->dot == rt)
      dot = st;

    // move on to next element of expansion
    rt = rt->tail;
    st0 = st;
  }

  // copy rest of parsing state (dot already set)
  w0 = r->w0;
  wn = r->wn;
  return 1;
}


//= See if exactly the same expansion as some other rule.
// this only looks as the expansion, not parsing state
// ignores status, id, and mark as well
// returns 1 if same, 0 if different

int jhcGramRule::SameRule (const jhcGramRule *ref) const
{
  const jhcGramStep *rt, *t = tail;

  // simple tests
  if (ref == NULL)
    return 0;
  if (_stricmp(head, ref->head) != 0)
    return 0;

  // check expansion one step at a time
  rt = ref->tail;
  while ((t != NULL) && (rt != NULL))  
  {
    if ((t->non != rt->non) || 
        (_stricmp(t->symbol, rt->symbol) != 0))
      return 0;
    t = t->tail;
    rt = rt->tail;
  }

  // see if any leftovers
  if ((t != NULL) || (rt != NULL))
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              File Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Dump rule head to file in SGM compatible format.

void jhcGramRule::Topic (FILE *out) const
{
  if (out == NULL)
    return;
  fprintf(out, "[=%s]\n", head);
}


//= Dump expansion part of rule to file in SGM compatible format.

void jhcGramRule::Expand (FILE *out) const
{
  jhcGramStep *t = tail;

  if ((out == NULL) || (t == NULL))
    return;
  fprintf(out, " ");
  while (t != NULL)
  {
    if (t->non > 0)
      fprintf(out, " <%s>", t->symbol);
    else
      fprintf(out, " %s", t->symbol);
    t = t->tail;
  }
  fprintf(out, "\n");
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Show rule on one line, possibly following some tag string.

void jhcGramRule::PrintRule (const char *tag) const
{
  jhcGramStep *t = tail;

  // beginning of line
  if (tag != NULL)
    jprintf("%s ", tag);
  jprintf("<%s>  <-- ", head);

  // elements of expansion
  while (t != NULL)
  {
    if (t->non > 0)
      jprintf(" <%s>", t->symbol);
    else
      jprintf(" %s", t->symbol);
    t = t->tail;
  }
  jprintf("\n");
}


//= Print out item interpreted as a chart parser state.
// assumes "mark" field holds unique state number

void jhcGramRule::PrintState (const char *tag) const
{
  jhcGramStep *t = tail;

  // beginning of line
  if (tag != NULL)
    jprintf("%s ", tag);
  jprintf("[S%03d]: %d-%d  <%s> = ", id, w0, wn, head);

  // sequence of terminals and non-terminals
  while (t != NULL)
  {
    // show where processing currently is
    jprintf("%s", ((t == dot) ? "." : " "));

    // show terminal or non-terminal
    if (t->non <= 0)
      jprintf("%s", t->symbol);
    else
    {
      // show back pointer for non-terminal
      jprintf("<%s>", t->symbol);
      if (t->back != NULL)
        jprintf("[S%03d]", (t->back)->id);
      else
        jprintf("[]");
    }
    t = t->tail;
  }
  
  // perhaps rule is fully matched now
  if (dot == NULL)
    jprintf(".");
  jprintf("\n");
}

