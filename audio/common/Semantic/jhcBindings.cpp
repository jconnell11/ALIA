// jhcBindings.cpp : list of substitutions of one node for another
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2018 IBM Corporation
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
}


//= Default constructor initializes certain values.

jhcBindings::jhcBindings (const jhcBindings *ref)
{
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
    key[nb] = ref.key[nb];
    sub[nb] = ref.sub[nb];
  }
  expect = ref.expect;
  return this;
}


///////////////////////////////////////////////////////////////////////////
//                              List Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get the current binding value for some key.
// returns NULL if key not found

jhcNetNode *jhcBindings::LookUp (const jhcNetNode *probe) const
{
  int i = index(probe);

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

  // try adding a new key-substitution pair
  if (nb >= bmax)
    return jprintf(">>> More than %d pairs in jhcBindings:Bind !\n", bmax);
  key[nb] = k;
  sub[nb] = subst;
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
  }
  return true;
}


//= Replace each value in list with its lookup in the reference bindings.
// self: obj-8 = obj-1 + ref: obj-1 = obj-237 --> self: obj-8 = obj-237 

void jhcBindings::ReplaceSubs (const jhcBindings& ref)
{
  jhcNetNode *s;
  int i;

  for (i = 0; i < nb; i++)
    if ((s = ref.LookUp(sub[i])) != NULL)
      sub[i] = s;
}


//= List bindings in format "key = subst".

void jhcBindings::Print (int lvl, const char *prefix) const
{
  int i, k = 0, n = 0, k2 = 0, n2 = 0;

  // get print field widths
  for (i = 0; i < nb; i++)
  {
    key[i]->NodeSize(k, n, 1);
    sub[i]->NodeSize(k2, n2, 1);
  }

  // print header
  if ((prefix != NULL) && (*prefix != '\0'))
    jprintf("%*s%s bindings:\n", lvl, "", prefix);
  else
    jprintf("%*sBindings:\n", lvl, "");

  // print key-sub pairs
  for (i = 0; i < nb; i++)
    jprintf("%*s  %*s = %*s\n", lvl, "", 
            (k + n + 1), key[i]->Nick(), -(k2 + n2 + 1), sub[i]->Nick());
}
