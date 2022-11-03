// jhcBindings.cpp : list of substitutions of one node for another
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2018 IBM Corporation
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

#include "Semantic/jhcBindings.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcBindings::~jhcBindings ()
{
  delete [] term;
  delete [] sub;
  delete [] key;
}


//= Default constructor initializes certain values.

jhcBindings::jhcBindings (const jhcBindings *ref)
{
  // make arrays on heap (keeps jhcAliaDir size manageable)
  key  = new const jhcNetNode * [bmax];
  sub  = new jhcNetNode * [bmax];
  term = new int [bmax];

  // initialize contents
  nb = 0;
  expect = 0;
  if (ref != NULL)
    Copy(*ref);
}


//= Make an exact copy of some other set of bindings, including order.
// returns pointer to self for convenience

jhcBindings *jhcBindings::Copy (const jhcBindings& ref)
{
  int nref = ref.nb;

  for (nb = 0; nb < nref; nb++)
  {
    key[nb]  = ref.key[nb];
    sub[nb]  = ref.sub[nb];
    term[nb] = ref.term[nb];
  }
  expect = ref.expect;
  return this;
}


//= See if any substitution node has a belief of zero.
// returns 1 if something is hypothetical, 0 if none are

int jhcBindings::AnyHyp () const
{
  int i;

  for (i = 0; i < nb; i++)
//    if (!key[i]->Hyp() && sub[i]->Hyp())
    if (sub[i]->Hyp())
      return 1;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                              List Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get the current node binding value for some node key.
// returns NULL if key not found

jhcNetNode *jhcBindings::LookUp (const jhcNetNode *k) const
{
  int i = index(k);

  return((i >= 0) ? sub[i] : NULL);
}


//= Get the index of the matching key.
// returns negative if no match found

int jhcBindings::index (const jhcNetNode *probe) const
{
  int i;

  if (probe != NULL)
    for (i = 0; i < nb; i++)
      if (probe == key[i])
        return i;
  return -1;
}


//= Do inverse lookup of node key for this node substitution (may not be unique).

const jhcNetNode *jhcBindings::FindKey (const jhcNetNode *subst) const
{
  int i;
  
  if (subst != NULL)
    for (i = 0; i < nb; i++)
      if (subst == sub[i])             // first key found is winner
        return key[i];
  return NULL;
}


//= Get lexical term associated with node, possibly looking up any "***-x" variable.

const char *jhcBindings::LexSub (const jhcNetNode *k) const
{
  if (k == NULL)
    return NULL;
  if (!k->LexVar())
    return k->Lex();
  return lookup_lex(k->Lex());
}


//= Scan through previous bindings to find substitution to make for this lex variable.

const char *jhcBindings::lookup_lex (const char *var) const
{
  int i;

  if (var != NULL)
    for (i = 0; i < nb; i++)
      if ((term[i] > 0) && (strcmp(key[i]->Lex(), var) == 0))
        return sub[i]->Lex();
  return NULL;
}


//= Find the hash bin (if any) associated with node's lexical term (after variable substitution).

int jhcBindings::LexBin (const jhcNetNode& k) const
{
  const char *var;
  int i;

  if (!k.LexVar())
    return k.Code();
  var = k.Lex();
  for (i = 0; i < nb; i++)
    if ((term[i] > 0) && (strcmp(key[i]->Lex(), var) == 0))
      return sub[i]->Code();
  return 0;
}


//= See if lexical term associated with mate is compatible with lexical term of focus.
// <pre>
//   focus  (bind)     mate     agree  <reason>
//  ---------------   -----     ---------------------
//    NULL             NULL      yes   <don't care>            
//    big              NULL       no   <not specific>
//    ***-1 (NULL)     NULL       no   <not specific>
//    ***-1 (big)      NULL       no   <not specific>
//    NULL            small      yes   <don't care>
//    big             small       no   <mismatch>
//    ***-1 (NULL)    small      yes   <add>
//    ***-1 (big)     small       no   <mismatch>
//    ***-1 (small)   small      yes   <match>
//    small           small      yes   <match>
// </pre>
// primarily used by jhcsituation::consistent

bool jhcBindings::LexAgree (const jhcNetNode *focus, const jhcNetNode *mate) const
{
  const char *flex = focus->Lex(), *mlex = mate->Lex();

  if (flex == NULL)
    return true;                                 // don't care about term
  if (mlex == NULL)
    return false;                                // not specific enough
  if (focus->LexVar()) 
    if ((flex = lookup_lex(flex)) == NULL)      
      return true;                               // can add as binding
  return(strcmp(flex, mlex) == 0);
}


//= Remember a particular key-value pair at the end of the current list.
// if NULL substitution (out) is given, key (in) remains in list but LookUp returns NULL
// generally returns number of bindings, including new, for use with TrimTo(N - 1) 
// return: N = number of bindings, 0 = no more room, -1 = duplicate key, -2 = bad key

int jhcBindings::Bind (const jhcNetNode *k, jhcNetNode *subst)
{
  // see if key already in list
  if (k == NULL)
    return -2;
  if (InKeys(k))
    return -1;

  // try adding a new key-substitution pair (and possibly bind variable lex as well)
  if (nb >= bmax)
    return jprintf(">>> More than %d pairs in jhcBindings:Bind !\n", bmax);
  key[nb]  = k;
  sub[nb]  = subst;
  term[nb] = 0;
  if (k->LexVar() && (lookup_lex(k->Lex()) == NULL))
    term[nb] = 1;
  nb++;
  return nb;
}


//= Remove most recently added bindings to retain only N.
// more useful than simply "Pop" since can revert to prior state
// returns 1 if okay, 0 if nothing to remove, -1 bad request

int jhcBindings::TrimTo (int n)
{
  if ((n < 0) || (n > nb))
    return -1;
  if (nb <= 0)
    return 0;
  nb = n;
  return 1;
}


//= Remove most recent binding, but only if it has the specified key.

void jhcBindings::RemFinal (jhcNetNode *k)
{
  if (nb > 0)
    if (key[nb - 1] == k)    
      nb--;
}


///////////////////////////////////////////////////////////////////////////
//                              List Functions                           //
///////////////////////////////////////////////////////////////////////////

//= See if some particular key has a binding.

bool jhcBindings::InKeys (const jhcNetNode *probe) const
{
  return(index(probe) >= 0);
}


//= See if some particular substitution is already associated with a key.

bool jhcBindings::InSubs (const jhcNetNode *probe) const
{
  int i;

  if (probe != NULL)
    for (i = 0; i < nb; i++)
      if (probe == sub[i])
        return true;
  return false;
}


//= Count nodes in a pattern that are not in the keys of these bindings.

int jhcBindings::KeyMiss (const jhcNodeList& f) const
{
  const jhcNetNode *node = NULL;
  int miss = f.Length();

  while ((node = f.NextNode(node)) != NULL)
    if (InKeys(node))
      miss--;
  return miss;
}


//= Count nodes in a pattern that are not in the substitutions of these bindings.

int jhcBindings::SubstMiss (const jhcNodeList& f) const
{
  const jhcNetNode *node = NULL;
  int miss = f.Length();

  while ((node = f.NextNode(node)) != NULL)
    if (InSubs(node))
      miss--;
  return miss;
}


///////////////////////////////////////////////////////////////////////////
//                              Bulk Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Tells whether this list is equivalent to reference bindings.
// this means the same keys go to the same values, independent of order
// useful for non-return inhibition checking of operators

bool jhcBindings::Same (const jhcBindings& ref) const
{
  const jhcNetNode *s;
  const char *var, *wd;
  int i;

  // must have exactly the same number of bindings
  if (nb != ref.nb)
    return false;

  // check all keys in local list
  for (i = 0; i < nb; i++)
  {
    // punt if key does not exist in reference or value is different
    if ((s = ref.LookUp(key[i])) == NULL)
      return false;
    if (s != sub[i])
      return false;

    // punt if lex variable does not exist in reference or value is different
    if (term[i] <= 0)
      continue;
    var = key[i]->Lex();    
    if ((wd = ref.lookup_lex(var)) == NULL)
      return false;
    if (strcmp(wd, lookup_lex(var)) != 0)
      return false;
  }
  return true;
}


//= Replace each value in list with its lookup in the reference bindings.
// self: obj-8 = obj-1 + ref: obj-1 = obj-237 --> self: obj-8 = obj-237 

void jhcBindings::ReplaceSubs (const jhcBindings& alt)
{
  jhcNetNode *s;
  int i;

  for (i = 0; i < nb; i++)
    if ((s = alt.LookUp(sub[i])) != NULL)
      sub[i] = s;                                // term[i] unchanged
}


//= List bindings in format "key = subst" where subst can be NULL.
// can optionally write header "<prefix> bindings:" instead of "bindings:"
// can optionally show only first "num" bindings instead of all
// if num < 0 then shows everything except the first bindings

void jhcBindings::Print (const char *prefix, int lvl, int num) const
{
  int start = ((num < 0) ? -num : 0);
  int stop  = ((num > 0) ? __min(num, nb) : nb);
  int i, k = 2, n = 1;

  // get print field widths
  for (i = start; i < stop; i++)
    key[i]->NodeSize(k, n);

  // print header (even if nothing to show)
  if ((prefix != NULL) && (*prefix != '\0'))
    jprintf("%*s%s bindings:\n", lvl, "", prefix);
  else
    jprintf("%*sBindings:\n", lvl, "");

  // go through selected key-sub pairs (might be none)
  for (i = start; i < stop; i++)
  {
    // list key node and substitution node then possibly lexical variable with value
    jprintf("%*s  %*s = %s\n", lvl, "", (k + n + 1), key[i]->Nick(), ((sub[i] != NULL) ? sub[i]->Nick() : "NULL"));
    if (term[i] > 0)
      jprintf("%*s  %*s = %s\n", lvl, "", (k + n + 1), key[i]->Lex(), sub[i]->Lex());
  }
}

