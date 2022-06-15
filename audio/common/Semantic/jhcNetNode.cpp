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

#include "Language/jhcMorphTags.h"
#include "Semantic/jhcBindings.h"      // since only spec'd as class in header
#include "Semantic/jhcGraphlet.h"      // since only spec'd as class in header

#include "Semantic/jhcNetNode.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.
// attempts to make remaining network consistent by deleting references
// can only be deleted through jhcNodePool manager

jhcNetNode::~jhcNetNode ()
{
  int i;

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
        for (j = 0; j < na; j++)
          if ((j != i) && (strcmp(links[j], links[i]) == 0))
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
// can only be created through jhcNodePool manager

jhcNetNode::jhcNetNode ()
{
  // basic configuration data
  *base = '\0';
  *lex  = '\0';
  *nick = '\0';
  quote = NULL;
  inv = 0;
  evt = 0;
  blf0 = 1.0;                  // value to use when actualized
  blf  = 0.0;                  // used to default to one

  // no arguments or properties, nothing else in list
  na = 0;
  na0 = 0;
  wrt = 0;
  np = 0;
  id = 0;
  next = NULL;

  // default bookkeeping
  hash = 0;
  gen = 0;
  ref = 0;
  vis = 1;

  // default status 
  top = 0;
  keep = 1;
  mark = 0;

  // no special grammar tags
  tags = 0;

  // not part of halo
  hrule = NULL;
  hbind = NULL;
}


//= Add a long string for regurgitation by echo output function.

void jhcNetNode::SetString (const char *wds)
{
  if (wds == NULL)
  {
    delete quote;
    quote = NULL;
    return;
  }
  if (quote == NULL) 
    quote = new char[200]; 
  strcpy_s(quote, 200, wds);
}


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

//= Count the number of distinct fillers for given role.
// if role is NULL then tells total argument count

int jhcNetNode::NumVals (const char *slot) const
{
  int i, cnt = 0;

  for (i = 0; i < na; i++)
    if (strcmp(links[i], slot) == 0)
      cnt++;
  return cnt;
}


//= Get the n'th filler for the given role.
// returns NULL if invalid index

jhcNetNode *jhcNetNode::Val (const char *slot, int n) const
{
  int i, cnt = n;

  if ((n >= 0) && (n < na))
    for (i = 0; i < na; i++)
      if (strcmp(links[i], slot) == 0)
        if (cnt-- <= 0)
          return args[i];
  return NULL;
}


//= Add some other node as an argument with the given link name.
// generic arguments do not need any explicit name
// returns 1 for success, 0 if something full, -1 for bad format

int jhcNetNode::AddArg (const char *slot, jhcNetNode *val)
{
  int i;

  // check argument and make sure space exists
  if (val == NULL)
    return -1;
  if (HasVal(slot, val))              // ignore duplicates
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
    for (i = 0; i < na; i++)
      if (strcmp(links[i], slot) == 0)
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


//= Replace existing argument with alternate node.
// leaves slot name and order of arguments the same for this node
// removes associated property from old value node

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

//= Checks if role in i'th property is one of several items.
// largely for convenience in grounding commands

bool jhcNetNode::RoleIn (int i, const char *v1, const char *v2, const char *v3, 
                         const char *v4, const char *v5, const char *v6) const
{
  return(RoleMatch(i, v1) || RoleMatch(i, v2) || RoleMatch(i, v3) || 
         RoleMatch(i, v4) || RoleMatch(i, v5) || RoleMatch(i, v6));
}


//= See if this node's i'th property has the node as its "role" argument.
// property node must also have matching "neg" and belief above "bth" 
// returns property node if true, else NULL

jhcNetNode *jhcNetNode::PropMatch (int i, const char *role, double bth, int neg) const
{
  jhcNetNode *p = Prop(i);

  if ((p != NULL) && (strcmp(Role(i), role) == 0) && (p->blf >= bth) && (p->inv == neg))
    return p;
  return NULL;
}


//= Count the number of nodes that have this node as a filler for given role.
// useful for determining if this node has a "tag" or a "cuz"
// if role is NULL then tells total property count

int jhcNetNode::NumFacts (const char *role) const
{
  jhcNetNode *p;
  int i, j, cnt = 0;

  for (i = 0; i < np; i++)
  {
    p = props[i];
    for (j = 0; j < p->na; j++)
      if ((p->args[j] == this) && (strcmp(p->links[j], role) == 0))
        cnt++;
  }
  return cnt;
}


//= Get the n'th node that has this node as a filler for the given role.
// useful for asking about this node relative to "ako" or "hq" 
// most recently added properties returned first
// returns NULL if invalid index

jhcNetNode *jhcNetNode::Fact (const char *role, int n) const
{
  jhcNetNode *p;
  int i, j, cnt = n;

  if ((n >= 0) && (n < np))
    for (i = np - 1; i >= 0; i--)
    {
      p = props[i];
      for (j = 0; j < p->na; j++)
        if ((p->args[j] == this) && (strcmp(p->links[j], role) == 0))
          if (cnt-- <= 0)
            return p;
    }
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                          Simple Matching                              //
///////////////////////////////////////////////////////////////////////////

//= See if the node participates in the triple: <self> -slot-> val. 

bool jhcNetNode::HasVal (const char *slot, const jhcNetNode *val) const
{
  int i;

  if ((val == NULL) || (slot == NULL) || (*slot == '\0'))
    return false;
  for (i = 0; i < na; i++)
    if ((args[i] == val) && (strcmp(links[i], slot) == 0))
      return true;
  return false;
}


//= See if the node participates in the triple: prop -role-> <self>. 

bool jhcNetNode::HasFact (const jhcNetNode *fact, const char *role) const
{
  if (fact == NULL) 
    return false;
  return fact->HasVal(role, this);
}


//= See if two nodes shared exactly the same set of arguments.

bool jhcNetNode::SameArgs (const jhcNetNode *ref) const
{
  int i;

  if ((ref == NULL) || (ref->na != na))
    return false;
  for (i = 0; i < na; i++)
    if (!ref->HasVal(links[i], args[i]))
      return false;
  return true;
}


//= See if given node has same arguments as the remapped arguments of this node.

bool jhcNetNode::SameArgs (const jhcNetNode *ref, const jhcBindings *b) const
{
  const jhcNetNode *a, *a2;
  int i;

  if ((ref == NULL) || (ref->na != na))
    return false;
  for (i = 0; i < na; i++)
  {
    a = args[i];    
    if ((b != NULL) && ((a2 = b->LookUp(a)) != NULL))
      a = a2;
    if (!ref->HasVal(links[i], a))
      return false;
  }      
  return true;
}


//= Find property node with lex "word" and argument "role" pointing to this node.
// property node must have matching "neg" and belief above "bth"
// however if asking for a hypothetical node (bth = 0) then match must have blf = 0
// returns NULL if no such property found (does not check properties of property)

jhcNetNode *jhcNetNode::FindProp (const char *role, const char *word, int neg, double bth) const
{
  jhcNetNode *p;
  int i;

  for (i = 0; i < np; i++)
  {
    p = props[i];
//    if (((p->blf >= bth) || ((bth == 0.0) && (p->blf == 0.0))) &&
//        (p->inv == neg) && RoleMatch(i, role) && p->LexMatch(word)) 
    if ((p->inv == neg) && (p->blf >= bth) && RoleMatch(i, role) && p->LexMatch(word)) 
      return p;
  }
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                 Predicate Terms and Reference Names                   //
///////////////////////////////////////////////////////////////////////////

//= Checks if predicate lexical term is one of several items.
// largely for convenience in grounding commands

bool jhcNetNode::LexIn (const char *v1, const char *v2, const char *v3, 
                        const char *v4, const char *v5, const char *v6) const
{
  return(LexMatch(v1) || LexMatch(v2) || LexMatch(v3) || LexMatch(v4) || LexMatch(v5) || LexMatch(v6));
}


//= Get a specific name out of all the names associated with this item.
// if "bth" > 0.0 then only returns non-negated words with belief over threshold
// most recently added terms returned first

const char *jhcNetNode::Name (int i, double bth) const
{
  const jhcNetNode *p;
  int j, cnt = 0;

  if (i >= 0)
    for (j = np - 1; j >= 0; j--)
    {
      p = Prop(j);
      if (RoleMatch(j, "name") && ((bth <= 0.0) || ((p->inv <= 0) && (p->blf >= bth))))
        if (cnt++ >= i)
          return p->Lex();
    }
  return NULL;
}


//= Checks if particular name is one of the references associated with this item.
// can alternatively check if the node is definitely NOT associated with some name

bool jhcNetNode::HasName (const char *word, int tru_only) const
{
  int i;

  if (word != NULL)
    for (i = 0; i < np; i++)
      if (strcmp(Role(i), "name") == 0)
        if (_stricmp(props[i]->lex, word) == 0)     
          return((tru_only <= 0) || (props[i]->inv <= 0));     // ignores belief
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                           Writing Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get text field sizes needed to represent this node.
// can optionally get sizes for lex nodes (e.g. for bindings)

void jhcNetNode::NodeSize (int& k, int& n) const
{
  char num[20];
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

int jhcNetNode::Save (FILE *out, int lvl, int k, int n, int r, int detail, const jhcGraphlet *acc) const
{
  char arrow[80];
  int i, j, len, lvl2, kmax = k, nmax = n, rmax = r, ln = 0;

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
  for (i = 0; i < na; i++)
  {
    // possibly indent (if no head node)
    if (ln++ > 0)  
      jfprintf(out, "\n%*s", lvl2, "");

    // create labeled arrow to other node (strncpy_s would add an extra '\0')
    _strnset_s(arrow + 2, sizeof(arrow) - 2, '-', rmax);
    len = (int) strlen(links[i]);
    for (j = 0; j < len; j++)
      arrow[j + 2] = (links[i])[j];
    jfputs(arrow, out); 
    jfprintf(out, "%s", args[i]->Nick());
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
