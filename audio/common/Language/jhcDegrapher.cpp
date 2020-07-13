// jhcDegrapher.cpp : generates natural language string from network
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include "Language/jhcMorphTags.h"

#include "Language/jhcDegrapher.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcDegrapher::~jhcDegrapher ()
{

}


//= Default constructor initializes certain values.

jhcDegrapher::jhcDegrapher ()
{
  *out = '\0';
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Generate an output string based on a particular graphlet.

const char *jhcDegrapher::Generate (const jhcGraphlet& graph, const jhcWorkMem& mem)
{
  jhcNetNode *n;
  int i, d, ni = graph.NumItems(), best = 0;

  // sanity check then bind arguments
  if (ni <= 0)
    return out;
  gr = &graph;
  wmem = &mem;

  // clear all node markers
  for (i = 0; i < ni; i++)
  {
    n = gr->Item(i);
    n->mark = 0;
  }

  // mark each node with its depth
  for (i = 0; i < ni; i++)
  {
    d = follow_args(gr->Item(i));
    best = __max(best, d);
  }

  // pick first node with greatest depth as starting point
  for (i = 0; i < ni; i++)
  {
    n = gr->Item(i);
    if (n->mark >= best)
      break;
  }
  return form_sent(out, 500, n, 1);
}


//= Figure out max depth of arguments from current node. 

int jhcDegrapher::follow_args (jhcNetNode *n) const
{
  jhcNetNode *n2;
  int i, d, na = n->NumArgs(), best = 0;

  if (n->mark > 0)
    return n->mark;
  for (i = 0; i < na; i++)
  {
    n2 = n->Arg(i);
    if (!gr->InDesc(n2))
      continue;
    d = follow_args(n2);
    best = __max(best, d);
  }
  n->mark = ++best;
  return best;
}


//= Descend along arugment links until first conjunction, verb, or noun found.
// returns full sentence starting from found element, NULL if nothing found

const char *jhcDegrapher::form_sent (char *txt, int ssz, const jhcNetNode *n, int top) const
{
  const char *tmp;
  int i, na = n->NumArgs();

jprintf("sent[%s]\n", n->Nick());
  // test for verb or noun tags, or conjunction arguments
  if ((n->tags & JTAG_VERB) != 0) 
    return form_vp(txt, ssz, n);
  if ((n->tags & JTAG_NOUN) != 0)
    return form_np(txt, ssz, n);
  for (i = 0; i < na; i++)
  {
    tmp = n->Slot(i);
    if ((strcmp(tmp, "conj") == 0) || (strcmp(tmp, "disj") == 0))
      return form_conj(txt, ssz, n);
  }

  // try descending along each argument link until first success
  for (i = 0; i < na; i++)
    if ((tmp = form_sent(txt, ssz, n->Arg(i), 0)) != NULL)
      return tmp;

  // if top node then recite literally, else give up
  if (top > 0)
    return form_intj(txt, ssz, n);
  return NULL;
}
  

//= Simplest case just echoes lexical term.

const char *jhcDegrapher::form_intj (char *txt, int ssz, const jhcNetNode *n) const
{
jprintf("intj[%s]\n", n->Nick());
  strcpy_s(txt, ssz, n->Word());
jprintf("intj -> %s\n", txt);
  return txt;
}


//= For a conjunction/disjunction render each element with final conjunction type.

const char *jhcDegrapher::form_conj (char *txt, int ssz, const jhcNetNode *n) const
{
  char item[200];
  int i, last = n->NumArgs() - 1;

jprintf("conj[%s]\n", n->Nick());
  // add early elements with separator after each (unless only 2)
  *txt = '\0';
  for (i = 0; i < last; i++)
  {
    strcat_s(txt, ssz, form_sent(item, 200, n->Arg(i), 0));
    if (last > 1)
      strcat_s(txt, ssz, ",");
    strcat_s(txt, ssz, " ");
  }

  // add connective then last element
  strcat_s(txt, ssz, n->Word());
  strcat_s(txt, ssz, " ");
  *item = '\0';
  strcat_s(txt, ssz, form_sent(item, 200, n->Arg(last), 0));
jprintf("conj -> %s\n", txt);
  return txt;
}


//= For verb phrase get subject, indirect object, direct object and modifiers.

const char *jhcDegrapher::form_vp (char *txt, int ssz, const jhcNetNode *n) const
{
  *txt = '\0';
  return txt;
}


//= For verb get proper form that respects node tags.

const char *jhcDegrapher::form_verb (char *txt, int ssz, const jhcNetNode *n, UL32 tags) const
{
  *txt = '\0';
  return txt;
}


//= For nouns get determiner, adjectives, base kinds and trailing prepositional phrases.

const char *jhcDegrapher::form_np (char *txt, int ssz, const jhcNetNode *n) const
{
  char frag[200];
  int i, np = n->NumProps(), base = 0;

jprintf("np[%s]\n", n->Nick());
  // check for proper name
  if (n->NumWords() > 0)
  {
    strcpy_s(txt, ssz, n->Word());
jprintf("np -> %s\n", txt);
    return txt;
  }

  // add possessive or determiner at front
  if (*form_poss(txt, ssz, n) == '\0')
    form_det(txt, ssz, n);
  
  // add determiner then add normal adjectives
  for (i = 0; i < np; i++)
    if (n->RoleMatch(i, "hq"))
      add_sp(txt, ssz, form_adj(frag, 200, n->Prop(i)));

  // add base kind(s)
  for (i = 0; i < np; i++)
    if (n->RoleMatch(i, "ako"))
    {
      if (base++ > 0)
        strcat_s(txt, ssz, " ");
      strcat_s(txt, ssz, form_noun(frag, 200, n->Prop(i), n->tags));
    }
jprintf("np -> %s\n", txt);
  return txt;
}


//= Possibly add a possessive in lieu of a determiner.

const char *jhcDegrapher::form_poss (char *txt, int ssz, const jhcNetNode *n) const
{
  char frag[200];
  const jhcNetNode *owner;
  const char *wd;
  int i, np = n->NumProps();

  // see if any AKO properties are relative
  for (i = 0; i < np; i++)
    if (n->RoleMatch(i, "ako"))
      if ((owner = (n->Prop(i))->Val("wrt")) != NULL)
        break;

  // add proper possessive form for owner
  if (owner == NULL)
    *txt = '\0';
  else if (owner->HasWord("me"))
    add_sp(txt, ssz, "my");
  else if (owner->HasWord("you"))
    add_sp(txt, ssz, "your");
  else if ((wd = owner->Word()) != NULL)     // proper name
    add_sp(txt, ssz, wd, "'s");
  else
    add_sp(txt, ssz, form_ref(frag, 200, owner), "'s");       
  return txt;
}


//= Generate a multi-word description as a reference to something.

const char *jhcDegrapher::form_ref (char *txt, int ssz, const jhcNetNode *n) const
{
  strcpy_s(txt, ssz, "something");
  return txt;
}


//= Add static string, possibly with a suffix, and then a space afterward.
// returns pointer to original string with additions at end

const char *jhcDegrapher::add_sp (char *txt, int ssz, const char *w, const char *suf) const
{
  strcat_s(txt, ssz, w);
  if (suf != NULL)
    strcat_s(txt, ssz, suf);
  strcat_s(txt, ssz, " ");
  return txt;
}


//= Supply proper determiner based on noun number.
// returns empty string if determiner not needed

const char *jhcDegrapher::form_det (char *txt, int ssz, const jhcNetNode *n) const
{
  if ((n->tags & JTAG_NMASS) != 0)
    *txt = '\0';
  else if ((n->tags & JTAG_DEF) != 0)
    strcpy_s(txt, ssz, "the ");
  else if ((n->tags & JTAG_NPL) != 0)
    strcpy_s(txt, ssz, "some ");
  else
    strcpy_s(txt, ssz, "a ");
  return txt;
}


//= Build adjectival phrase including intensifier and reference (if needed).

const char *jhcDegrapher::form_adj (char *txt, int ssz, const jhcNetNode *n) const
{
  int i, np = n->NumProps();

jprintf("adj[%s]\n", n->Nick());
  *txt = '\0';
  for (i = 0; i < np; i++)
    if (n->RoleMatch(i, "deg"))
      add_sp(txt, ssz, (n->Prop(i))->Word());
  strcat_s(txt, ssz, n->Word());
jprintf("adj -> %s\n", txt);
  return txt;
}


//= Supply proper base word respecting node tags.

const char *jhcDegrapher::form_noun (char *txt, int ssz, const jhcNetNode *n, UL32 tags) const
{
  strcpy_s(txt, ssz, n->Word());
  if ((tags & JTAG_NPL) != 0)
    strcat_s(txt, ssz, "s");
  return txt;
}



