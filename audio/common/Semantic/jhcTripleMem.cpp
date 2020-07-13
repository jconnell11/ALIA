// jhcTripleMem.cpp : collection of semantic triples with search operators
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "Semantic/jhcTripleMem.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcTripleMem::jhcTripleMem ()
{
  init_ptrs();
}


//= Default destructor does necessary cleanup.

jhcTripleMem::~jhcTripleMem ()
{
  BlankSlate();
}


//= Get rid of any allocated entities or facts.

void jhcTripleMem::BlankSlate ()
{
  jhcTripleNode *n, *n2;
  jhcTripleLink *t, *t2;

  // get rid of all links
  t = facts;
  while (t != NULL)
  {
    t2 = t->prev;    // backward list
    delete t;
    t = t2;
  }  

  // get rid of all nodes
  n = items;
  while (n != NULL)
  {
    n2 = n->next;    // forward list
    delete n;
    n = n2;
  }

  // set up pointers
  init_ptrs();
}


//= Set pointers up for start of a new session.

void jhcTripleMem::init_ptrs ()
{
  // no nodes
  items = NULL;
  gnum  = 1;

  // no links
  dawn  = NULL;
  facts = NULL;

  // no communication activity
  update = NULL;
  reply  = NULL;
  focus  = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                      Node and Link Modification                       //
///////////////////////////////////////////////////////////////////////////

//= Create a new item of some base kind and returns its identifier.
// if NULL string is given for kind then just assigns a number to node
// returns 1 if successful, 0 if failed

int jhcTripleMem::NewItem (char *id, const char *kind, int ssz)
{
  char mix[80];
  const jhcTripleNode *n;

  // build name including numeric suffix
  if (kind != NULL)
    sprintf_s(mix, "%s-%d", kind, gnum);
  else
    sprintf_s(mix, "%d", gnum);

  // create object and update counter if successful
  if ((n = add_node(mix)) == NULL)
    return 0;
  gnum++;

  // tell name assigned
  if (id != NULL)
    strcpy_s(id, ssz, n->Name());
  return 1;
}


//= Forces item to have a SINGLE slot with the given value.
// removes all other triples with same head and function
// looks for numerical suffix to decide between tags and fillers
// head with given id must already exist, as must filler node (if not tag)
// returns 1 if successful, 0 if fails

int jhcTripleMem::SetValue (const char *id, const char *fcn, const char *val)
{
  jhcTripleLink *t0, *t;
  jhcTripleNode *n, *n2 = NULL;

  // check that the mentioned topic and filler exist 
  if ((n = find_node(id)) == NULL)
    return 0;
  if (node_num(val) > 0)
    if ((n2 = find_node(val)) == NULL)
      return 0;

  // check for any other triples with the same slot name
  t = n->ArgList();
  while (t != NULL)
  {
    // set up for next cycle
    t0 = t;
    t = t->NextArg();
    if (_stricmp(t0->Fcn(), fcn) != 0)
      continue;

    // remove from history list (also splices out of args and props)
    pluck(t0);
    delete t0;
  }

  // add link with new value
  if (n2 != NULL)
    t = add_link(n, fcn, n2);
  else
    t = add_link(n, fcn, val);
  if (t == NULL)
    return 0;
  return 1;
}


//= Add another link to item with the given slot but a different value.
// looks for numerical suffix to decide between tags and fillers
// head with given id must already exist, as must filler node (if not tag)
// returns 1 if successful, 0 if fails

int jhcTripleMem::AddValue (const char *id, const char *fcn, const char *val)
{
  jhcTripleLink *t;
  jhcTripleNode *n, *n2 = NULL;
  int any = 0;

  // check that the mentioned topic and filler exist 
  if ((n = find_node(id)) == NULL)
    return 0;
  if (node_num(val) > 0)
    if ((n2 = find_node(val)) == NULL)
      return 0;

  // see if there are any other values for the slot
  t = n->ArgList();
  while (t != NULL)
  {
    if (_stricmp(t->Fcn(), fcn) == 0)
    {
      any = 1;
      break;
    }
    t = t->next;
  }

  // add link with extra value and mark as a multiple
  if (n2 != NULL)
    t = add_link(n, fcn, n2);
  else
    t = add_link(n, fcn, val);
  if (t == NULL)
    return 0;
  t->multi = any;
  return 1;
}


//= Create or alter a triple encoding the given fact.
// will automatically build topic and filler nodes if needed
// generally used to accept facts verbatim from remote host 
// returns 1 if successful, 0 if fails 

int jhcTripleMem::BuildTriple (const char *id, const char *fcn, const char *val, int append)
{
  jhcTripleNode *n, *n2 = NULL;

  // check that the mentioned topic exists (possibly create) 
  if ((n = find_node(id)) == NULL)
    if ((n = add_node(id)) == NULL)
      return 0;

  // check that mentioned filler exists (possibly create)
  if (node_num(val) > 0)
    if ((n2 = find_node(val)) == NULL)
      if ((n2 = add_node(val)) == NULL)
        return 0;

  // set or add given value
  if (append > 0)
    return AddValue(id, fcn, val);
  return SetValue(id, fcn, val);
}


///////////////////////////////////////////////////////////////////////////
//                      Node and Link Interrogation                      //
///////////////////////////////////////////////////////////////////////////

//= Tells if node of some generic base kind (does not check for existence).
// returns 1 if instance of kind, 0 if different kind, -1 if not a proper instance

int jhcTripleMem::NodeKind (const char *id, const char *kind) const
{
  const char *sep;
  int n, klen = (int) strlen(kind);

  // check for proper argument types
  if ((id == NULL) || (kind == NULL))
    return -1;
  if ((sep = strrchr(id, '-')) == NULL)
    return -1;

  // compare string segment in front of dash
  n = (int)(sep - id);
  if (n != klen)
    return 0;
  if (strncmp(id, kind, klen) != 0)
    return 0;
  return 1;
}


//= Determines how many values there are for a specific slot.
// can be used with GetValue to enumerate all possible values

int jhcTripleMem::NumVals (const char *id, const char *fcn) 
{
  const jhcTripleNode *n;
  const jhcTripleLink *t;
  int cnt = 0;

  // make sure topic exists
  n = find_node(id);
  if (n == NULL)
    return 0;

  // look through all arguments for matching slot name
  t = n->ArgList();
  while (t != NULL)
  {
    if (_stricmp(t->Fcn(), fcn) == 0)
      cnt++;
    t = t->NextArg();
  }
  return cnt;
}


//= Lookup up the i'th value for a given slot (binds a string).
// index i used to enumerate through possibilities (use NumVals)
// typical code: while (GetValue(id, fcn, val, i++)) > 0) {... id ...}
// returns 1 if suitable triple found, 0 if nothing matches ("val" unchanged)

int jhcTripleMem::GetValue (const char *id, const char *fcn, char *val, int i, int ssz) 
{
  const jhcTripleNode *n, *n2;
  const jhcTripleLink *t;
  int cnt = 0;

  // make sure topic node exists
  if (val == NULL)
    return 0;
  if ((n = find_node(id)) == NULL)
    return 0;

  // search for matching slot in argument list
  t = n->ArgList();
  while (t != NULL)
  {
    if (_stricmp(t->Fcn(), fcn) == 0)
      if (cnt++ == i)
        break;
    t = t->NextArg();
  }
  if (t == NULL)
    return 0;

  // get filler as a string (not a pointer)
  n2 = t->Fill();
  if (n2 != NULL)
    strcpy_s(val, ssz, n2->Name());
  else 
    strcpy_s(val, ssz, t->Tag());
  return 1;
}


//= Returns number of things for which the given slot has the given value.
// can be used with GetHead to enumerate all possible values
// NOTE: value must be a node, sort of meaningless for string tags

int jhcTripleMem::NumHead (const char *fcn, const char *val) 
{
  const jhcTripleNode *n2;
  const jhcTripleLink *t;
  int cnt = 0;

  // make sure value node exists
  if ((n2 = find_node(val)) == NULL)
    return 0;

  // count items in the list
  t = n2->PropList();
  while (t != NULL)
  {
    if (_stricmp(t->Fcn(), fcn) == 0)
      cnt++;
    t = t->NextProp();
  }
  return cnt;
}


//= Return thing for which the specified slot has the given value.
// index i used to enumerate through possibilities (use NumHead)
// typical code: while (GetHead(id, fcn, val, i++)) > 0) {... id ...}
// NOTE: value must be a node, sort of meaningless for string tags
// returns 1 if suitable triple found, 0 if nothing matches ("id" unchanged)

int jhcTripleMem::GetHead (char *id, const char *fcn, const char *val, int i, int ssz) 
{
  const jhcTripleNode *n, *n2;
  const jhcTripleLink *t;
  int cnt = 0;

  // make sure value node exists
  if (id == NULL)
    return 0;
  if ((n2 = find_node(val)) == NULL)
    return 0;

  // go a certain depth into the list
  t = n2->PropList();
  while (t != NULL)
  {
    if (_stricmp(t->Fcn(), fcn) == 0)
      if (cnt++ == i)
        break;
    t = t->NextProp();
  }
  if (t == NULL)
    return 0;

  // get head as a string (not a pointer)
  n = t->Head();
  strcpy_s(id, ssz, n->Name());
  return 1;
}


//= Get only relation head nodes that are of a particular kind.
// returns 1 if suitable triple found, 0 if nothing matches

int jhcTripleMem::GetHeadKind (char *id, const char *kind, const char *fcn, const char *val, int i, int ssz)
{
  int n = 0, cnt = 0;

  while (GetHead(id, fcn, val, n++, ssz) > 0)
    if (NodeKind(id, kind) > 0)
      if (cnt++ == i)
        return 1;
  return 0;
}


//= See if there is already a triple exactly matching description.
// value can be either a node name, a string tag, or a numeric value

int jhcTripleMem::MatchTriple (const char *id, const char *fcn, const char *val)
{
  const jhcTripleNode *n, *n2 = NULL;
  const jhcTripleLink *t;

  // check that the mentioned topic and filler exist
  if ((n = find_node(id)) == NULL)
    return 0;
  if (node_num(val) > 0)
    n2 = find_node(val);

  // walk down list of arguments of topic
  t = n->ArgList();
  while (t != NULL)
  {
    // if slot matches then check this value 
    if (_stricmp(t->Fcn(), fcn) == 0)
    {
      if (n2 != NULL) 
      {
        // both are same filler
        if (t->Fill() == n2) 
          return 1;
      }
      else if (t->Fill() == NULL)
      {
        // both are same tag
        if (_stricmp(t->Tag(), val) == 0)
          return 1; 
      }
    }
    t = t->NextArg();
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                       Low Level Node Functions                        //
///////////////////////////////////////////////////////////////////////////

//= Extract numeric suffix of node name (0 if none).

int jhcTripleMem::node_num (const char *id) const
{
  const char *sep;
  int n = 0;

  if ((sep = strrchr(id, '-')) != NULL)
    sscanf_s(sep + 1, "%d", &n);
  else
    sscanf_s(id, "%d", &n);
  return n;
}


//= Get mutable pointer to node with given name (if any).
// cannot be const because reorders object list for efficiency

jhcTripleNode *jhcTripleMem::find_node (const char *id)
{
  jhcTripleNode *n0, *n2, *n = items;

  if ((id == NULL) || (*id == '\0'))
    return NULL;

  // first search for some existing item (most recent first)
  while (n != NULL)
  {
    // check if found it
    if (_stricmp(n->Name(), id) == 0)    
      break;
    n0 = n;
    n = n->next;
  }

  // check if found and whether first in list
  if (n == NULL)
    return NULL;
  if (n != items)
  {
    // splice out of old position in list
    n2 = n->next;
    n0->next = n2;
    if (n2 != NULL)
      n2->prev = n0;

    // move to head of list for faster retrieval
    n->next = items;
    items = n;
  }
  return n;
}


//= Get pointer to item with given name (object list not rearranged).

const jhcTripleNode *jhcTripleMem::read_node (const char *id) const
{
  const jhcTripleNode *n = items;

  if ((id == NULL) || (*id == '\0'))
    return NULL;

  while (n != NULL)
  {
    if (_stricmp(n->Name(), id) == 0)    
      return n;
    n = n->next;
  }
  return NULL;
}


//= Create a new node with the specified name (verbatim).
// automatically updates gnum if needed

jhcTripleNode *jhcTripleMem::add_node (const char *name)
{
  jhcTripleNode *n;
  int i;

  // check for numeric suffix then create node
  if ((i = node_num(name)) > 0)
    gnum = __max(gnum, i + 1);
  if ((n = new jhcTripleNode(name)) == NULL)
    return NULL;

  // add node to the object list (most recent at head of "items" list)
  n->next = items;
  if (items != NULL)
    items->prev = n;
  items = n;
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                       Low Level Link Functions                        //
///////////////////////////////////////////////////////////////////////////

//= Add a new triple with the given node-based value.
// returns pointer if okay, NULL if fails

jhcTripleLink *jhcTripleMem::add_link (jhcTripleNode *n, const char *fcn, jhcTripleNode *n2)
{
  jhcTripleLink *t;

  // check that the mentioned topic and filler exist 
  if ((n == NULL) || (n2 == NULL))
    return NULL;

  // create a new fact (automatically update args and props)
  if ((t = new jhcTripleLink(n, fcn)) == NULL)
    return NULL;
  t->SetFill(n2);

  // add to history list
  push(t);
  return t;
}


//= Add a new triple with the given string-based value.
// returns pointer if okay, NULL if fails

jhcTripleLink *jhcTripleMem::add_link (jhcTripleNode *n, const char *fcn, const char *txt)
{
  jhcTripleLink *t;

  // check that the mentioned topic exists 
  if (n == NULL)
    return NULL;

  // create a new fact (automatically update args and props)
  if ((t = new jhcTripleLink(n, fcn)) == NULL)
    return NULL;
  t->SetTag(txt);

  // add to history list
  push(t);
  return t;
}


//= Removes given link from history list.

void jhcTripleMem::pluck (jhcTripleLink *t)
{
  jhcTripleLink *p = t->prev, *n = t->next;

  // fix list ends
  if (dawn == t)
    dawn = n;
  if (facts == t)
    facts = p;

  // fix various communication pointers
  if (update == t)
    update = n;
  if (reply == t)
    reply = n;
  if (focus == t)
    focus = n;

  // remove from double-linked history
  if (p != NULL)
    p->next = n;
  if (n != NULL)
    n->prev = p;

  // clear local list pointers for safety
  t->prev = NULL;
  t->next = NULL;
}


//= Moves given link to end of history list.
// assumes properly disconnected with "pluck" (or new)

void jhcTripleMem::push (jhcTripleLink *t)
{
  // tag on to end of history list ("facts" points to end)
  t->prev = facts;
  if (facts != NULL)
    facts->next = t;
  facts = t;

  // possibly fix list start (if only fact)
  if (dawn == NULL)
    dawn = t;

  // fix various communication pointers
  if (update == NULL)
    update = t;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Write out current list of objects to a file.
// returns number of items written, negative for error

int jhcTripleMem::DumpItems (const char *fname, const char *hdr) const
{
  FILE *out;
  const jhcTripleNode *n = items;
  int cnt = 0;

  // try opening file and writing header
  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  if (hdr != NULL)
    fprintf(out, "// %s\n", hdr);

  // write out all numbered objects
  while (n != NULL)
  {
    if (*(n->Name()) != '\0')
    {
      fprintf(out, "%s\n", n->Name()); 
      cnt++;
    }
    n = n->next;
  }
  
  // clean up and report
  fclose(out);
  return cnt;
}


//= Save all current triples in historical order to a file.
// returns number of facts written, negative for error

int jhcTripleMem::DumpHist (const char *fname, const char *hdr) const
{
  FILE *out;
  const jhcTripleLink *t = dawn;
  int cnt = 0;

  // try opening file and writing header
  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  if (hdr != NULL)
    fprintf(out, "// %s\n", hdr);

  // write out all triples
  while (t != NULL)
  {
    if (t->Tabbed(out) > 0)
      cnt++;
    t = t->next;
  }
  
  // clean up and report
  fclose(out);
  return cnt;
}


//= Print out current list of objects on console.

void jhcTripleMem::PrintItems () const
{
  const jhcTripleNode *n = items;

  // write header
  jprintf("----------------------\n");
  jprintf("Numbered items:\n");

  // write out all numbered objects
  while (n != NULL)
  {
    if (*(n->Name()) != '\0')
      jprintf("  %s\n", n->Name());
    n = n->next;
  }
  jprintf("\n");
}


//= Shows all the links involving some item.

void jhcTripleMem::PrintLinks (const char *id) const
{
  const jhcTripleNode *n;
  const jhcTripleLink *t, *t2;

  // print header
  jprintf("----------------------\n");
  jprintf("Links involving %s:\n", id);

  // check if node exists
  n = read_node(id);
  if (n == NULL)
  {
    jprintf("  no node found!\n\n");
    return;
  }

  // write out arguments
  t = n->ArgList();
  if (t != NULL)
    while (t != NULL)
    {
      t->Print(" ");
      t = t->NextArg();
    }

  // write out properties
  t2 = n->PropList();
  if (t2 != NULL)
    while (t2 != NULL)
    {
      t2->Print(" ");
      t2 = t2->NextProp();
    }

  // make sure somethign was written
  if ((t == NULL) && (t2 == NULL))
    jprintf("  no links found!\n\n");
  else
    jprintf("\n");
}


//= Shows all the links emanating from some item.

void jhcTripleMem::PrintArgs (const char *id) const
{
  const jhcTripleNode *n;
  const jhcTripleLink *t;

  // print header
  jprintf("----------------------\n");
  jprintf("Arguments of %s:\n", id);

  // check if node exists
  n = read_node(id);
  if (n == NULL)
  {
    jprintf("  no node found!\n\n");
    return;
  }

  // write out facts
  t = n->ArgList();
  if (t == NULL)
  {
    jprintf("  no outgoing links found!\n\n");
    return;
  }

  // write out all facts
  while (t != NULL)
  {
    t->Print(" ");
    t = t->NextArg();
  }
  jprintf("\n");
}


//= Shows all the links impinging on some item.

void jhcTripleMem::PrintProps (const char *id) const
{
  const jhcTripleNode *n;
  const jhcTripleLink *t;

  // print header
  jprintf("----------------------\n");
  jprintf("Properties of %s:\n", id);

  // check if node exists
  n = read_node(id);
  if (n == NULL)
  {
    jprintf("  no node found!\n\n");
    return;
  }

  // check for links
  t = n->PropList();
  if (t == NULL)
  {
    jprintf("  no incoming links found!\n\n");
    return;
  }

  // write out all links
  while (t != NULL)
  {
    t->Print(" ");
    t = t->NextProp();
  }
  jprintf("\n");
}


//= Print out all current triples on console.

void jhcTripleMem::PrintHist () const
{
  const jhcTripleLink *t;

  // write header
  jprintf("----------------------\n");
  jprintf("Triple history:\n");

  // write out all triples
  t = dawn;
  while (t != NULL)
  {
    t->Print(" ");
    t = t->next;
  }
  jprintf("\n");
}


//= Print out all the triples about to be sent to remote host.

void jhcTripleMem::PrintUpdate () const
{
  const jhcTripleLink *t;

  // write header
  jprintf("----------------------\n");
  jprintf("New pod for host:\n");

  // write out all triples
  t = update;
  while (t != NULL)
  {
    t->Print(" ");
    t = t->next;
  }
  jprintf("\n");
}



