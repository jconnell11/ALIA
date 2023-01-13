// jhcNetNode.cpp : node in semantic network for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#include <string.h>
#include <ctype.h>

#include "Language/jhcMorphTags.h"
#include "Semantic/jhcBindings.h"      // since only spec'd as class in header
#include "Semantic/jhcGraphlet.h"      // since only spec'd as class in header
#include "Semantic/jhcNodePool.h"      // since only spec'd as class in header

#include "Semantic/jhcNetNode.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// attempts to make remaining network consistent by deleting references
// NOTE: should only be deleted through jhcNodePool manager!

jhcNetNode::~jhcNetNode ()
{
  int i;

  // remove any linkage between surface and deep cognates
  if (moor != NULL)
    moor->buoy = NULL;
  if (buoy != NULL)
    buoy->moor = NULL;

  // remove text payload (if any) and all incoming and outgoing links
  delete quote;
  for (i = 0; i < na; i++)
    args[i]->rem_prop(this);
  for (i = np - 1; i >= 0; i--)
    props[i]->rem_arg(this);
}


//= Remove a specific property and compact remaining list of properties.

void jhcNetNode::rem_prop (const jhcNetNode *item)
{
  int i, j;

  for (i = 0; i < np; i++)
    if (props[i] == item)
    {
      np--;
      for (j = i; j < np; j++)
      {
        props[j] = props[j + 1];
        anum[j]  =  anum[j + 1];
      }
      props[np] = NULL;
      anum[np] = 0;
    }
}


//= Remove a specific argument and compact remaining list of arguments.

void jhcNetNode::rem_arg (const jhcNetNode *item)
{
  char bare[20], strip[20];
  int i, j;

  // find all occurrences of items in list of arhs
  for (i = 0; i < na; i++)
    if (args[i] == item)
    {
      // see if any other args with same slot name (arity decrease)
      if (strcmp(links[i], "wrt") == 0)
        wrt--;
      else
      {
        // arguments "ref" and "ref2" count as only one kind for arity
        rem_num(bare, links[i], 20);
        for (j = 0; j < na; j++)
          if ((j != i) && (strcmp(bare, rem_num(strip, links[j], 20)) == 0))
            break;
        if ((na > 0) && (j >= na))
          na0--;
      }

      // compact the list to cover hole made by removal
      na--;
      for (j = i; j < na; j++)
      {
        args[j] = args[j + 1];
        strcpy_s(links[j], links[j + 1]); 
      }

      // make sure old end of the list is truly empty
      args[na] = NULL;
      links[na][0] = '\0';
    }
}


//= Default constructor initializes certain values.
// NOTE: should only be created through jhcNodePool manager!

jhcNetNode::jhcNetNode (int num, class jhcNodePool *pool)
{
  // basic configuration data
  *base = '\0';
  *lex  = '\0';
  *nick = '\0';
  quote = NULL;
  inv = 0;
  evt = 0;
  blf0 = 1.0;                // value to use when actualized
  blf  = 0.0;                // used to default to one

  // no arguments or properties, nothing else in list
  na = 0;
  na0 = 0;
  wrt = 0;
  np = 0;
  id = num;                  // assigned by jhcNodePool
  next = NULL;

  // LTM linkage
  moor = NULL;
  buoy = NULL;
  home = pool;               // less ambiguous than separator

  // default bookkeeping
  hash = 0;
  gen = 0;
  ref = 0;
  vis = 1;

  // default status 
  top = 0;
  keep = 1;
  mark = 0;
  ltm = 0;

  // no special grammar tags
  tags = 0;

  // not part of halo
  hrule = NULL;
  hbind = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                       Negation and Belief, Etc.                       //
///////////////////////////////////////////////////////////////////////////

//= Whether the node has any tags indicating it is an object.

bool jhcNetNode::NounTag () const
{
  return((tags & JTAG_NOUN) != 0);
}


//= Whether the node has any tags indicating it is an action.

bool jhcNetNode::VerbTag () const
{
  return((tags & JTAG_VERB) != 0);
}


//= Set the language I/O reference recency to the newest for its pool.
// NOTE: only for language input and output routines (i.e. user knows about node)

void jhcNetNode::MarkConvo ()
{
  if (home != NULL)
    ref = home->IncConvo();
}


//= Set type part of nickname for node and create full name.
// retains old separator if none given (defaults to '-')

void jhcNetNode::SetKind (const char *k, char sep)  
{
  char delim = sep;
  int bsz;

  // try to copy previous delimiter if none supplied
  if (delim == '\0')
  {
    bsz = (int) strlen(base);
    if (bsz < (int) strlen(nick))
      delim = nick[bsz];
    else
      delim = '-';
  }

  // make up kind and nickname strings
  strcpy_s(base, k);
  sprintf_s(nick, "%s%c%d", k, delim, abs(id));
}


//= Add a long string for regurgitation by echo output function.

void jhcNetNode::SetString (const char *txt)
{
  if (txt == NULL)
  {
    delete quote;
    quote = NULL;
    return;
  }
  if (quote == NULL) 
    quote = new char[200]; 
  strcpy_s(quote, 200, txt);
}


//= Set belief to value specified during creation.
// lets user statements be selectively accepted/rejected from working memory
// returns 1 if belief has changed, 0 if already at same value

int jhcNetNode::Actualize (int ver) 
{
  if (blf == blf0)
    return 0;
  blf = blf0;
  GenMax(ver);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Argument Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Tell number of uniquely named slots associated with this node.
// slots that have same prefix but different number only count once ("ref" and "ref2")
// can optionally omit "wrt" links (uses cached "na0" and "wrt" vars)
// NOTE: assumes this is a surface node (ignores buoy) 

int jhcNetNode::Arity (int all) const
{
  int cnt = na0, xtra = wrt;

  if (moor != NULL)
  {
    cnt = moor->na0;
    xtra = moor->wrt;
  }
  if ((all > 0) && (xtra > 0))
    cnt++;                             // omit for ako2 matching
  return cnt;
}


//= Check if this node or any of its arguments (recursively) is hypothetical.
// largely for wmem print out function

bool jhcNetNode::HypAny () const
{
  int i;

  if (Hyp())
    return true;
  for (i = 0; i < na; i++)
    if (Arg(i)->HypAny())
      return true;
  return false;
}


//= Return some argument of the current node (link type irrelevant).
// prefers moor so okay if surface node in tethered pair is not fleshed out
// NOTE: assumes this is a surface node, does not convert answer to surface

jhcNetNode *jhcNetNode::Arg (int i) const 
{
  if ((i < 0) || (i >= NumArgs()))
    return NULL;
  if (moor != NULL)
    return moor->args[i];
  return args[i];
}


//= Find link name associated with some argument of a surface node.
// prefers moor so okay if surface node in tethered pair is not fleshed out
// NOTE: assumes this is a surface node (ignores buoy) 

const char *jhcNetNode::Slot (int i) const
{
  if ((i < 0) || (i >= NumArgs()))
    return NULL;
  if (moor != NULL)
    return moor->links[i];
  return links[i];
}


//= Count the number of distinct fillers for given role.
// NOTE: assumes this is a surface node (ignores buoy) 

int jhcNetNode::NumVals (const char *slot) const
{
  int i, n = NumArgs(), cnt = 0;

  for (i = 0; i < n; i++)
    if (strcmp(Slot(i), slot) == 0)
      cnt++;
  return cnt;
}


//= Get the i'th filler for the given role.
// returns NULL if invalid index
// NOTE: assumes this is a surface node, does not convert answer to surface

jhcNetNode *jhcNetNode::Val (const char *slot, int i) const
{
  int j, n = NumArgs(), cnt = i;

  if ((n >= 0) && (i < n))
    for (j = 0; j < n; j++)
      if (strcmp(Slot(j), slot) == 0)
        if (cnt-- <= 0)
          return Arg(j);
  return NULL;
}


//= Add some other node as an argument with the given link name.
// generic arguments do not need any explicit name
// returns 1 for success, 0 if something full, -1 for bad format
// NOTE: assumes both are untethered surface nodes (ignores moor and buoy)

int jhcNetNode::AddArg (const char *slot, jhcNetNode *val)
{
  char bare[20], strip[20];
  int i;

  // check argument and make sure space exists
  if (val == NULL)
    return -1;
  if (HasVal(slot, val))               // ignore duplicates
    return 1;
  if (na >= amax)
    return jprintf(">>> More than %d arguments in jhcNetNode::AddArg !\n", amax);
  if (val->np >= pmax)
    return jprintf(">>> More than %d properties in jhcNetNode::AddArg !\n", pmax);

  // see if a new kind of link (boosts arity)
  if (strcmp(slot, "wrt") == 0)
    wrt++;
  else
  {
    // arguments "ref" and "ref2" count as only one kind for arity
    rem_num(bare, slot, 20);
    for (i = 0; i < na; i++)
      if (strcmp(bare, rem_num(strip, links[i], 20)) == 0)
        break;
    if ((na == 0) || (i >= na))
      na0++;
  }

  // add as argument to this node
  links[na][0] = '\0';
  if (slot != NULL) 
    strcpy_s(links[na], slot);
  args[na] = val;

  // add this node as a property of other node
  val->props[val->np] = this;
  val->anum[val->np] = na;

  // bump args of this node and props of other node
  na++;
  val->np += 1;
  return 1;
}


//= Remove any non-alphabetic suffix from a slot name and save result in "trim".

const char *jhcNetNode::rem_num (char *trim, const char *sname, int ssz) const
{
  char *end = trim;

  strcpy_s(trim, ssz, sname);
  while (*end != '\0')
    if (!isalpha(*end))
    {
      *end = '\0';
      break;
    }
    else
      end++;
  return trim;
} 


//= Replace existing argument with alternate node.
// leaves slot name and order of arguments the same for this node
// removes associated property from old value node
// NOTE: assumes both are untethered surface nodes (ignores moor and buoy)

void jhcNetNode::SubstArg (int i, jhcNetNode *val)
{
  if ((i < 0) || (i >= na) || (val == NULL) || (val == args[i]))
    return;

  // cleanly remove old argument and point to replacement instead
  args[i]->rem_prop(this);   
  args[i] = val;

  // add this node as a property of the replacement node
  val->props[val->np] = this;
  val->anum[val->np] = i;
  val->np += 1;
}


//= Make sure this node is at the end of the property list for given argument.
// guarantees that this node will be the first retrieved
// NOTE: assumes this is an untethered surface node (ignores moor and buoy)

void jhcNetNode::RefreshArg (int i) 
{
  jhcNetNode *val;
  int last, now, j;

  // see if this node is already the final property of the argument
  if ((i < 0) || (i >= na))
    return;
  val = args[i];
  last = val->np - 1;
  if (val->props[last] == this)
    return;
  
  // compact the argument's list of properties
  for (now = 0; now < last; now++)
    if (val->props[now] == this)
      break;
  for (j = now; j < last; j++)
  {
    val->props[j] = val->props[j + 1];
    val->anum[j]  = val->anum[j + 1];
  }

  // tack self onto end
  val->props[last] = this;
  val->anum[last] = i;
}


///////////////////////////////////////////////////////////////////////////
//                          Property Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Combined number of properties for both surface and deep cognates.
// NOTE: assumes this is a surface node (ignores buoy) 

int jhcNetNode::NumProps () const 
{
  if (moor != NULL)
    return(np + moor->np);
  return np;
}


//= Select one property out of total collection given a surface node.
// properties are stored in oldest-first order (typically retrieved in reverse)
// NOTE: assumes this is a surface node, does not convert answer to surface

jhcNetNode *jhcNetNode::Prop (int i) const
{
  int i2;

  if ((i < 0) || (i >= NumProps()))
    return NULL;
  if (moor == NULL)
    return props[i];
  i2 = i - moor->np;
  if (i2 < 0)
    return moor->props[i];
  if (i2 < np)                         // should always be true
    return props[i2];
  return NULL;
}


//= Return link name that goes with selected property,
// properties are stored in oldest-first order (typically retrieved in reverse)
// NOTE: assumes this is a surface node (ignores buoy) 

const char *jhcNetNode::Role (int i) const
{
  const jhcNetNode *fact;
  int i2;

  if ((i < 0) || (i >= NumProps()))
    return NULL;
  if (moor == NULL)
  {
    if ((fact = props[i]) == NULL)
      return NULL;
    return fact->links[anum[i]];
  }
  i2 = i - moor->np;    
  if (i2 < 0)
  {
    if ((fact = moor->props[i]) == NULL)
      return NULL;
    return fact->links[moor->anum[i]];    
  }
  if (i2 < np)                         // should always be true
  {
    if ((fact = props[i2]) == NULL)
      return NULL;
    return fact->links[anum[i2]];
  }
  return NULL;
}


//= Checks if role in i'th property is one of several items.
// largely for convenience in grounding commands
// NOTE: assumes this is a surface node (ignores buoy) 

bool jhcNetNode::RoleIn (int i, const char *v1, const char *v2, const char *v3, 
                         const char *v4, const char *v5, const char *v6) const
{
  return(RoleMatch(i, v1) || RoleMatch(i, v2) || RoleMatch(i, v3) || 
         RoleMatch(i, v4) || RoleMatch(i, v5) || RoleMatch(i, v6));
}


//= See if this node's i'th property has the node as its "role" argument.
// property node must also have matching "neg" and belief above "bth" 
// returns property node (surface version) if true, else NULL
// NOTE: assumes this is a surface node (ignores buoy) 

jhcNetNode *jhcNetNode::PropMatch (int i, const char *role, double bth, int neg) const
{
  jhcNetNode *p = PropSurf(i);

  if ((p != NULL) && (strcmp(Role(i), role) == 0) && (p->Belief() >= bth) && (p->Neg() == neg))
    return p;
  return NULL;
}


//= Count the number of nodes that have this node as a filler for given role.
// useful for determining if this node has a "tag" or a "cuz"
// NOTE: assumes this is a surface node (ignores buoy) 

int jhcNetNode::NumFacts (const char *role) const
{
  const jhcNetNode *p;
  int i, j, a, n = NumProps(), cnt = 0;

  for (i = 0; i < n; i++)
    if ((p = Prop(i)) != NULL)
    {
      a = p->NumArgs();
      for (j = 0; j < a; j++)
        if ((p->Arg(j) == this) && p->SlotMatch(j, role))
          cnt++;
    }
  return cnt;
}


//= Get the n'th node that has this node as a filler for the given role.
// useful for asking about this node relative to "ako" or "hq" 
// most recently added properties returned first
// returns NULL if invalid index
// NOTE: assumes this is a surface node (ignores buoy) 

jhcNetNode *jhcNetNode::Fact (const char *role, int n) const
{
  jhcNetNode *p;
  int i, j, a, total = NumProps(), cnt = n;

  if ((n >= 0) && (n < total))
    for (i = total - 1; i >= 0; i--)
      if ((p = Prop(i)) != NULL)
      {
        a = p->NumArgs();
        for (j = 0; j < a; j++)
          if ((p->Arg(j) == this) && p->SlotMatch(j, role))
            if (cnt-- <= 0)
              return p;
    }
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                      Long Term Memory Linkage                         //
///////////////////////////////////////////////////////////////////////////

//= Connect this surface node to some deep node.
// Note: a node can only be tethered to one other node, any previous linkage overwritten

void jhcNetNode::MoorTo (jhcNetNode *deep)
{
  jhcNetNode *s0;

  if (deep == moor)                    // already tethered
    return;
  if (deep == this)                    // should not happen
  {
    jprintf(">>> Cannot moor %s to itself!\n", nick);
    return;
  }
  if (deep != NULL)
  {
    if ((s0 = deep->buoy) != NULL) 
    {
      if (!s0->Halo())
        jprintf(">>> Unmooring previous %s (was %s) !\n", s0->nick, (s0->moor)->nick);
      s0->moor = NULL;
    }
    deep->buoy = this;
  }
  else if (moor != NULL)
    jprintf(">>> Unmooring self %s (was %s) !\n", nick, moor->nick);
  moor = deep;
}


//= Mark garbage collection state of associated surface node (if any).

void jhcNetNode::SetKeep (int val) 
{
  if (buoy != NULL)
    buoy->keep = val;
  else
    keep = val;
}


//= Check the garbage collection state of associated surface node (if any).

int jhcNetNode::Keep () const
{
  if (buoy != NULL)
    return buoy->keep;
  return keep;
}


///////////////////////////////////////////////////////////////////////////
//                          Simple Matching                              //
///////////////////////////////////////////////////////////////////////////

//= See if the node participates in the triple: <self> -slot-> val. 
// NOTE: assumes this is a surface node, auto-converts val up or down

bool jhcNetNode::HasVal (const char *slot, const jhcNetNode *val) const
{
  const jhcNetNode *a;
  int i, n = NumArgs();

  if ((val == NULL) || (slot == NULL) || (*slot == '\0'))
    return false;
  for (i = 0; i < n; i++)
    if (SlotMatch(i, slot))
    {
      a = Arg(i);
      if ((a == val) || (a == val->buoy) || (a == val->moor))
        return true;
    }
  return false;
}


//= See if the node participates in the triple: fact -role-> <self>. 
// NOTE: assumes fact is a surface node, auto-converts this up or down

bool jhcNetNode::HasFact (const jhcNetNode *fact, const char *role) const
{
  if (fact == NULL) 
    return false;
  return fact->HasVal(role, this);
}


//= See if two nodes shared exactly the same set of arguments.
// NOTE: assumes this is a surface node, but auto-converts argument nodes

bool jhcNetNode::SameArgs (const jhcNetNode *ref) const
{
  int i, n = NumArgs();

  if ((ref == NULL) || (ref->NumArgs() != n))
    return false;
  for (i = 0; i < n; i++)
    if (!ref->HasVal(Slot(i), Arg(i)))
      return false;
  return true;
}


//= See if given node has same arguments as the remapped arguments of this node.
// NOTE: assumes this is a surface node, but auto-converts argument nodes

bool jhcNetNode::SameArgs (const jhcNetNode *ref, const jhcBindings *b) const
{
  const jhcNetNode *a, *a2;
  int i, n = NumArgs();

  if ((ref == NULL) || (ref->NumArgs() != n))
    return false;
  for (i = 0; i < n; i++)
  {
    a = Arg(i);    
    if ((b != NULL) && ((a2 = b->LookUp(a)) != NULL))
      a = a2;
    if (!ref->HasVal(Slot(i), a))
      return false;
  }      
  return true;
}


//= Find property node with lex "word" and argument "role" pointing to this node.
// property node must have matching "neg" and belief above "bth"
// returns NULL if no such property found (does not check properties of property)
// NOTE: assumes this is a surface node, does not auto-convert return value

jhcNetNode *jhcNetNode::FindProp (const char *role, const char *word, int neg, double bth) const
{
  jhcNetNode *p;
  int i, n = NumProps();

  for (i = 0; i < n; i++)
  {
    p = Prop(i);
//    if (((p->Belief() >= bth) || ((bth == 0.0) && (p->Belief() == 0.0))) &&
//        (p->Neg() == neg) && RoleMatch(i, role) && p->LexMatch(word)) 
    if ((p->Neg() == neg) && (p->Belief() >= bth) && RoleMatch(i, role) && p->LexMatch(word)) 
      return p;
  }
  return NULL;
}


//= Find argument node with lex "word" and argument "slot" pointing from this node.
// argument node must have matching "neg" and belief above "bth"
// returns NULL if no such property found (does not check properties of property)
// NOTE: assumes this is a surface node, does not auto-convert return value

jhcNetNode *jhcNetNode::FindArg (const char *slot, const char *word, int neg, double bth) const
{
  jhcNetNode *a;
  int i, n = NumArgs();

  for (i = 0; i < n; i++)
  {
    a = Arg(i);
    if ((a->Neg() == neg) && (a->Belief() >= bth) && SlotMatch(i, slot) && a->LexMatch(word)) 
      return a;
  }
  return NULL;
}


//= Check whether given text label refers to any of the slots associated with this node.
// useful as a type checking tool (e.g. with "loc" or "name")

bool jhcNetNode::HasSlot (const char *slot) const
{
  int i, n = NumArgs();

  if (slot != NULL)
    for (i = 0; i < n; i++)
      if (strcmp(slot, Slot(i)) == 0)
        return true;
  return false;
}


//= See if node has a slot with any of the labels given. 

bool jhcNetNode::AnySlot (const char *v1, const char *v2, const char *v3, 
                          const char *v4, const char *v5, const char *v6) const
{
  return(HasSlot(v1) || HasSlot(v2) || HasSlot(v3) || 
         HasSlot(v4) || HasSlot(v5) || HasSlot(v6));
}


//= Check for each slot of this predicate that the reference node has a matching slot name.

bool jhcNetNode::SameSlots (const jhcNetNode *ref) const
{
  int i, n = NumArgs();

  for (i = 0; i < n; i++)
    if (!ref->HasSlot(Slot(i)))
      return false;
  return true;
}


///////////////////////////////////////////////////////////////////////////
//                 Predicate Terms and Reference Names                   //
///////////////////////////////////////////////////////////////////////////

//= Checks if predicate lexical term is one of several items.
// largely for convenience in grounding commands
// NOTE: moor and buoy will always have same lex 

bool jhcNetNode::LexIn (const char *v1, const char *v2, const char *v3, 
                        const char *v4, const char *v5, const char *v6) const
{
  return(LexMatch(v1) || LexMatch(v2) || LexMatch(v3) || LexMatch(v4) || LexMatch(v5) || LexMatch(v6));
}


//= Get a specific name out of all the names associated with this item.
// if "bth" > 0.0 then only returns non-negated words with belief over threshold
// most recently added terms returned first
// NOTE: assumes this is a surface node (ignores buoy) 

const char *jhcNetNode::Name (int i, double bth) const
{
  const jhcNetNode *p;
  int j, n = NumProps(), cnt = 0;

  if (i >= 0)
    for (j = n - 1; j >= 0; j--)
      if (RoleMatch(j, "name"))
      {
        p = Prop(j);
        if ((bth <= 0.0) || ((p->Neg() <= 0) && (p->Belief() >= bth)))
          if (cnt++ >= i)
            return p->Lex();
    }
  return NULL;
}


//= Checks if particular name is one of the references associated with this item.
// can alternatively check if the node is definitely NOT associated with some name
// NOTE: assumes this is a surface node (ignores buoy) 

bool jhcNetNode::HasName (const char *word, int tru_only) const
{
  const jhcNetNode *p;
  int i, n = NumProps();

  if (word != NULL)
    for (i = 0; i < n; i++)
      if (RoleMatch(i, "name"))
      {
        p = Prop(i);
        if (_stricmp(p->lex, word) == 0)     
          return((tru_only <= 0) || (p->Neg() <= 0));      // ignores belief
      }
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                           Writing Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get text field sizes needed to represent this node.
// can optionally get sizes for lex nodes (e.g. for bindings)
// NOTE: assumes this is an untethered surface node (ignores moor and buoy)

void jhcNetNode::NodeSize (int& k, int& n) const
{
  char num[20] = "";
  int len;

  len = (int) strlen(base);
  k = __max(k, len);
  _itoa_s(abs(id), num, 10);
  len = (int) strlen(num);
  n = __max(n, len);  
}


//= Estimate field widths for node kinds, instance numbers, and role names.
// this is specific to style = 0, style = 1 (properties) may be different
// variables k, n, and r need to be initialized externally (e.g. to zero)
// NOTE: assumes this is an untethered surface node (ignores moor and buoy)

void jhcNetNode::TxtSizes (int& k, int& n, int& r) const
{
  int i, len;

  NodeSize(k, n);
  for (i = 0; i < na; i++)
  {
    args[i]->NodeSize(k, n);
    len = (int) strlen(links[i]);
    r = __max(r, len);
  }
}


//= Report all arguments of this node including tags (no CR on last line).
// adds blank line and indents first line unless lvl < 0
// detail: -2 belief + tags, -1 belief, 0 no extras, 1 default belief, 2 default belief + tags
// always returns abs(lvl) for convenience (no check for file errors)
// NOTE: assumes this is an untethered surface node (ignores moor and buoy)

int jhcNetNode::Save (FILE *out, int lvl, int k, int n, int r, int detail, const jhcGraphlet *acc) const
{
  char arrow[80];
  const jhcNetNode *a;
  const char *slot;
  int i, j, na2, len, lvl2, kmax = k, nmax = n, rmax = r, ln = 0;

  // write out main node identifier
  if ((k < 1) || (n < 1) || (r < 1))
    TxtSizes(kmax, nmax, rmax);
  if (lvl >= 0)
    jfprintf(out, "\n%*s", lvl, "");
  jfprintf(out, "%*s", kmax + nmax + 1, Nick());
  lvl2 = abs(lvl) + kmax + nmax + 1;

  // tack on words, negation, and belief 
  ln = save_tags(out, lvl2, rmax, detail);

  // go through all arguments (keep rewriting arrow string)
  sprintf_s(arrow, " -%*s-> ", rmax, "");
  na2 = NumArgs();
  for (i = 0; i < na2; i++)
  {
    // possibly indent (if no head node)
    if (ln++ > 0)  
      jfprintf(out, "\n%*s", lvl2, "");

    // create labeled arrow to other node (strncpy_s would add an extra '\0')
    _strnset_s(arrow + 2, sizeof(arrow) - 2, '-', rmax);
    slot = Slot(i);
    len = (int) strlen(slot);
    for (j = 0; j < len; j++)
      arrow[j + 2] = slot[j];
    jfputs(arrow, out);
    a = Arg(i);                                  // not ArgSurf
    jfprintf(out, "%s", a->Nick());
  }
  return abs(lvl);
}


//= Writes out lexical terms, negation, and belief (if they exist) for node.
// detail: -2 belief + tags, -1 belief, 0 no extras, 1 default belief, 2 default belief + tags
// returns number of lines written

int jhcNetNode::save_tags (FILE *out, int lvl, int r, int detail) const
{
  char txt[10];
  int i, ln = 0;

  // possibly add associated predicate term
  if (*lex != '\0')
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s %s", -(r + 3), "-lex-", lex);
  }

  // possibly add literal (if exists, usually only part)
  if (quote != NULL)
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s %s", -(r + 3), "-str-", quote);
  }

  // add event (happened or did not) and negation lines
  if (evt > 0)
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s %d", -(r + 3), "-ach-", ((inv > 0) ? 0 : 1)); 
  }
  else if (inv > 0)
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s 1", -(r + 3), "-neg-");
  }

  // add belief line
  if ((detail < 0) && (blf <= 0.0))
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s 0", -(r + 3), "-ext-");            // add -ext- marker (hypothetical)
    if (blf0 != 1.0)
      jfprintf(out, "\n%*s %*s %s", lvl, "", -(r + 3), "-blf-", bfmt(txt, blf0));    
  }
  else if ((detail < 0) && (blf != 1.0) && (quote == NULL))
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s %s", -(r + 3), "-blf-", bfmt(txt, blf));
  }
  else if ((detail > 0) && (blf0 != 1.0))                  // normal mode shows blf0
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s %s", -(r + 3), "-blf-", bfmt(txt, blf0));    
  }

  // add grammatical tags
  if ((abs(detail) >= 2) && (tags != 0))
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s", -(r + 3), "-tag-");
    for (i = 0; i < JTV_MAX; i++)
      if ((tags & (0x01 << i)) != 0)
        jfprintf(out, " %s", JTAG_STR[i]);
  }
  return ln;
}


//= Print belief value with various numbers of digits.
// assumes string passed in is at least 10 chars long

const char *jhcNetNode::bfmt (char *txt, double val) const
{
  if (val < 0.0)
    sprintf_s(txt, 10, "%6.3f", val);
  else if (val == 0.0)
    strcpy_s(txt, 10, "0");
  else
    sprintf_s(txt, 10, "%6.4f", val);
  return txt;
}
