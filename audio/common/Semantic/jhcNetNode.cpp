// jhcNetNode.cpp : node in semantic network for ALIA system
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

#include <string.h>

#include "Language/jhcMorphTags.h"
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
        anum[j] = anum[j + 1];
      }
    }
}


//= Remove a specific argument and compact remaining list of arguments.

void jhcNetNode::rem_arg (const jhcNetNode *item)
{
  int i, j;

  for (i = 0; i < na; i++)
    if (args[i] == item)
    {
      na--;
      for (j = i; j < na; j++)
      {
        args[j] = args[j + 1];
        strcpy_s(links[j], links[j + 1]); 
      }
    }
}


//= Default constructor initializes certain values.
// can only be created through jhcNodePool manager

jhcNetNode::jhcNetNode ()
{
  // basic configuration data
  *base = '\0';
  *nick = '\0';
  quote = NULL;
  inv = 0;
  evt = 0;
  blf0 = 1.0;                  // value to use when actualized
  blf  = 0.0;                  // used to default to one

  // no arguments or properties, nothing else in list
  na = 0;
  np = 0;
  id = 0;
  next = NULL;

  // default bookkeeping
  gen = 0;
  ref = 0;

  // default status 
  pod = 0;
  top = 0;
  keep = 1;
  mark = 0;

  // no special grammar tags
  tags = 0;
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
  if (ver > 0)
    gen = ver;
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


//= See if two nodes shared exactly the same set of arguments.

bool jhcNetNode::SameArgs (const jhcNetNode *ref) const
{
  int i;

  if ((ref == NULL) || (ref->na != na))
    return false;
  for (i = 0; i < na; i++)
    if (!ref->HasVal(Slot(i), Arg(i)))
      return false;
  return true;
}


//= Add some other node as an argument with the given link name.
// generic arguments do not need any explicit name
// returns 1 for success, 0 if something full, -1 for bad format

int jhcNetNode::AddArg (const char *slot, jhcNetNode *val)
{
  // check argument and make sure space exists
  if (val == NULL)
    return -1;
  if (HasVal(slot, val))              // ignore duplicates
    return 1;
  if (na >= amax)
    return jprintf(">>> More than %d arguments in jhcNetNode::AddArg !\n", amax);
  if (val->np >= pmax)
    return jprintf(">>> More than %d properties in jhcNetNode::AddArg !\n", pmax);

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


//= Return the n'th non-lexical property of the node.
// most recently added properties returned first

jhcNetNode *jhcNetNode::NonLex (int i) const
{
  jhcNetNode *n;
  int j, cnt = 0;

  if (i >= 0)
    for (j = np - 1; j >= 0; j--)
    {
      n = props[j];
      if (!n->LexNode())
        if (cnt++ >= i)
          return n;
    }
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


//= See if the node participates in the triple: prop -role-> <self>. 

bool jhcNetNode::HasFact (const jhcNetNode *fact, const char *role) const
{
  if (fact == NULL) 
    return false;
  return fact->HasVal(role, this);
}


///////////////////////////////////////////////////////////////////////////
//                            Associated Words                           //
///////////////////////////////////////////////////////////////////////////

//= See if this is a lexical tag node versus a normal property.

bool jhcNetNode::LexNode () const 
{
  return((na == 1) && (strcmp(links[0], "lex") == 0));
}


//= Make sure both are lexical tag nodes and that their intrincis words match.
// returns false if neither is a lexical tag node (!LexConflict would return true)
// NOTE: this is for checking lexical tagging nodes themselves

bool jhcNetNode::LexMatch (const jhcNetNode *ref) const
{
  return(LexNode() && ref->LexNode() && (strcmp(base, ref->base) == 0));
}


//= If either is a lexical tag, make sure associated terms match.
// returns false if neither is a lexical node (!LexMatch would return true)
// NOTE: this is for checking lexical tagging nodes themselves (cf. SameWords)

bool jhcNetNode::LexConflict (const jhcNetNode *ref) const
{
  if (!LexNode() && !ref->LexNode())
    return false;
  if (!LexNode() || !ref->LexNode())
    return true;
  return(_stricmp(base, ref->base) != 0);
}


//= Checks a particular property to see if it is a lexical tagging.
// returns associated term if a tag, NULL otherwise

const char *jhcNetNode::LexBase (int i) const
{
  if (strcmp(Role(i), "lex") == 0)
    return Prop(i)->base;
  return NULL;
}


//= Checks if particular word is one of the tags associated with this item.
// can alternatively check if the node is definitely NOT associated with some word

bool jhcNetNode::HasWord (const char *word, int tru_only) const
{
  const char *wd;
  int i;

  if (word != NULL)
    for (i = 0; i < np; i++)
      if ((wd = LexBase(i)) != NULL)
        if (_stricmp(wd, word) == 0)     
          return((tru_only <= 0) || (props[i]->inv <= 0));  // ignores belief
  return false;
}


//= Checks if lexical tag is one of several items.
// largely for convenience in grounding commands

bool jhcNetNode::WordIn (const char *v1, const char *v2, const char *v3, 
                        const char *v4, const char *v5, const char *v6) const
{
  return(HasWord(v1) || HasWord(v2) || HasWord(v3) || HasWord(v4) || HasWord(v5) || HasWord(v6));
}


//= See how many of the properties are lexical tags.

int jhcNetNode::NumWords () const
{
  int i, cnt = 0;

  for (i = 0; i < np; i++)
    if (LexBase(i) != NULL)
      cnt++;
  return cnt;
}


//= Get a specific tag out of all the words associated with this item.
// if "bth" > 0.0 then only returns non-negated words with belief over threshold
// most recently added terms returned first

const char *jhcNetNode::Word (int i, double bth) const
{
  const char *wd;
  int j, cnt = 0;

  if (i >= 0)
    for (j = np - 1; j >= 0; j--)
      if ((wd = LexBase(j)) != NULL)
        if ((bth <= 0.0) || ((inv <= 0) && (blf >= bth)))
          if (cnt++ >= i)
            return wd;
  return NULL;
}


//= Return first word associated with this node, or node name if no words.
// mostly for printing things

const char *jhcNetNode::Tag () const
{
  const char *wd = Word(0);

  return((wd != NULL) ? wd : nick);
}


//= Both nodes must have all the same lexical terms associated with them.
// returns true if all terms match or neither node is a lexical tag
// NOTE: this is for use with normal predicate nodes (cf. LexConflict)

bool jhcNetNode::SameWords (const jhcNetNode *ref) const
{
  const char *wd;
  int i;

  if (NumWords() != ref->NumWords())
    return false;
  for (i = 0; i < np; i++)
    if ((wd = LexBase(i)) != NULL)
      if (!ref->HasWord(wd))
        return false;
  return true;
}


//= Nodes must share at least one word.
// NOTE: this is for use with normal predicate nodes (cf. LexConflict)

bool jhcNetNode::SharedWord (const jhcNetNode *ref) const
{
  const char *wd;
  int i;

  for (i = 0; i < np; i++)
    if ((wd = LexBase(i)) != NULL)
      if (ref->HasWord(wd))
        return true;
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                           Writing Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get text field sizes needed to represent this node.
// can optionally get sizes for lex nodes (e.g. for bindings)

void jhcNetNode::NodeSize (int& k, int& n, int bind) const
{
  char num[20];
  int len;

  if ((bind <= 0) && LexNode())
    return;
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
// detail: 0 no extras, 1 show belief, 2 show tags, 3 show both
// always returns abs(lvl) for convenience (no check for file errors)

int jhcNetNode::Save (FILE *out, int lvl, int k, int n, int r, const jhcGraphlet *acc, int detail) const
{
  char arrow[80];
  int i, j, len, lvl2, kmax = k, nmax = n, rmax = r, ln = 0;

  // term tagging nodes handled specially
  if (LexNode())
    return naked_lex(out, lvl, k, n, r, acc, detail);

  // write out main node identifier
  if ((k < 1) || (n < 1) || (r < 1))
    TxtSizes(kmax, nmax, rmax);
  if (lvl >= 0)
    jfprintf(out, "\n%*s", lvl, "");
  jfprintf(out, "%*s", kmax + nmax + 1, Nick());
  lvl2 = abs(lvl) + kmax + nmax + 1;

  // tack on words, negation, and belief
  ln = save_tags(out, lvl2, rmax, acc, detail);

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
    jfprintf(out, "%*s", -(kmax + nmax + 1), args[i]->Nick());
  }
  return abs(lvl);
}


//= Writes out lexical terms, negation, and belief (if they exist) for node.
// detail: 0 no extras, 1 show belief, 2 show tags, 3 show both
// returns number of lines written

int jhcNetNode::save_tags (FILE *out, int lvl, int r, const jhcGraphlet *acc, int detail) const
{
  const jhcNetNode *p;
  const char *wd;
  int i, ln = 0;

  // possibly add literal (if exists, usually only part)
  if (quote != NULL)
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s %s", -(r + 3), "-str-", quote);
  }

  // possibly add associated word(s) if they are part of graphlet
  for (i = 0; i < np; i++)
  {
    p = Prop(i);
    if ((acc == NULL) || acc->InDesc(p))
      if ((wd = LexBase(i)) != NULL)
      {
        if (ln++ > 0)
          jfprintf(out, "\n%*s", lvl, "");
        jfprintf(out, " %*s %s%s", -(r + 3), "-lex-", 
                 ((p->inv > 0) ? "* " : ""), wd);          // lex never an event
        if (((detail & 0x01) != 0) && (p->blf != 1.0))
          jfprintf(out, " (%6.4f)", p->blf);
      }
  }

  // add event (success or failure), negation, and belief lines
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
  if (((detail & 0x01) != 0) && (blf != 1.0) && (quote == NULL))
  {
    if (ln++ > 0)
      jfprintf(out, "\n%*s", lvl, "");
    jfprintf(out, " %*s %6.4f", -(r + 3), "-blf-", blf);
  }

  // add grammatical tags
  if (((detail & 0x02) != 0) && (tags != 0))
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


//= Decide what to print for a lexical tagging node (sometimes nothing).
// detail: 0 no extras, 1 show belief, 2 show tags, 3 show both
// detail: -1 special print of naked lex (hack for alias rules!)
// always returns abs(lvl) for convenience 

int jhcNetNode::naked_lex (FILE *out, int lvl, int k, int n, int r, const jhcGraphlet *acc, int detail) const
{
  const jhcNetNode *named = args[0];
  int kmax = k, nmax = n, rmax = r;

  // check if thing being named is in graphlet somewhere else
  if ((detail >= 0) && ((acc == NULL) || acc->InDesc(named)))
    return lvl;
  if ((k < 1) || (n < 1) || (r < 1))
  {
    named->NodeSize(kmax, nmax);
    rmax = __max(rmax, 3);
  }

  // print out thing being named
  if (lvl >= 0)
    jfprintf(out, "\n%*s", lvl, "");
  jfprintf(out, "%*s", kmax + nmax + 1, named->Nick());

  // print the associated lexical term (lex never an event)
  jfprintf(out, " %*s %s%s", -(rmax + 3), "-lex-", ((inv > 0) ? "* " : ""), base);
          
  if (((detail & 0x01) != 0) && (blf != 1.0))
    jprintf(" (%6.4f)", blf);
  return abs(lvl);
}

