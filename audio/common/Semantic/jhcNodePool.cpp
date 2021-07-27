// jhcNodePool.cpp : singly-linked list of semantic network nodes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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

#include <ctype.h>
#include <math.h>

#include "Language/jhcMorphTags.h"     // common audio
#include "Semantic/jhcGraphlet.h"      // since only spec'd as class in header

#include "Semantic/jhcNodePool.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcNodePool::~jhcNodePool ()
{
  ClrTrans(0);
  PurgeAll();
  if (nbins > 1)
  {
    delete [] pop;
    delete [] bucket;
  }
}


//= Default constructor initializes certain values.

jhcNodePool::jhcNodePool ()
{
  // direction of numbering
  dn = 0;

  // initial visibility of new nodes
  vis0 = 1;

  // no hashing, just main pool (= bucket[0])
  nbins = 1;
  bucket = &pool;
  pop = &ncnt;
  init_pool();

  // extras for loading
  trans = NULL;
  surf = NULL;
  tmax = 0;
  nt = 0;
}


//= Sets up an empty set of nodes and removes any graphlet.

void jhcNodePool::init_pool ()
{
  int i;

  // basic list and numbering
  for (i = 0; i < nbins; i++)
  {
    bucket[i] = NULL;
    pop[i] = 0;
  }
  psz = 0;
  label = 0;

  // collections and generations
  acc = NULL;
  ver = 1;
  rnum = 0;

  // changes to members
  add = 0;
  arg = 0;
  del = 0;
  mod = 0;
}


//= Convert to hashed version with multiple buckets for faster matching.
// typically called right after construction, before any nodes created

void jhcNodePool::MakeBins ()
{
  // if not currently hashed, dump all exisitng nodes
  if (nbins > 1)
    return;
  PurgeAll();

  // make up buckets and initialize as empty
  nbins = 676;
  bucket = new jhcNetNode * [nbins];
  pop = new int [nbins];
  init_pool();
}


//= Tell how many items potentially have the same hash code as given.
// if bin < 0 then tells total number of nodes in whole pool

int jhcNodePool::BinCnt (int bin) const
{
  int i, cnt = 0;

  if (bin >= nbins)              // invalid hash
    return 0;                          
  if (nbins <= 1)                // only one bin exists
    return pop[0];                     
  if (bin >= 0)
    return pop[bin];
  for (i = 0; i < nbins; i++)    // add up all bins
    cnt += pop[i];                     
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                             List Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Removes all nodes in list (beware dangling pointers).
// next allocated node gets id = 1 or -1 (if dn > 0)

void jhcNodePool::PurgeAll ()
{
  jhcNetNode *n, *n0;
  int i;

  for (i = 0; i < nbins; i++)
  {
    n = bucket[i];
    while (n != NULL)
    {
      n0 = n;
      n = n->next;
      delete n0;
      del++;
    }
  }
  init_pool();
}


//= Get a starting point to enumerate nodes from.
// can preselect nodes whose hash falls in a certain bin
// if bin < 0 then will try all bins until a non-empty one found

jhcNetNode *jhcNodePool::Pool (int bin) const
{
  int i;

  // see if starting bucket has something in it
  if (nbins <= 1)
    return bucket[0];
  if (bin >= nbins)
    return NULL;
  if (bin >= 0)
    return bucket[bin];

  // possibly progress to other buckets to find something
  for (i = 0; i < nbins; i++)
    if (bucket[i] != NULL)
      return bucket[i];
  return NULL;
}


//= Get next node in current bin (or whole pool if bin < 0).

jhcNetNode *jhcNodePool::Next (const jhcNetNode *ref, int bin) const 
{
  int i;

  // generally follow chain of next pointers
  if (ref == NULL)
    return Pool(bin);
  if (ref->next != NULL)
    return ref->next;
  if (bin >= 0)
    return NULL;

  // possibly progress to other buckets to find something
  for (i = ref->hash + 1; i < nbins; i++)
    if (bucket[i] != NULL)
      return bucket[i];
  return NULL;
}


//= Determine how many actual nodes are in the pool (and visible).
// "psz" tells how many nodes have been created -- but some may already be deleted

int jhcNodePool::NodeCnt () const
{
  const jhcNetNode *n;
  int i, cnt = 0;

  for (i = 0; i < nbins; i++)
  {
    n = bucket[i];
    while (n != NULL)
    {
      if (n->vis > 0)
        cnt++;
      n = n->next;
    }
  }
  return cnt;
}


//= Tell if anything about collection of nodes has changed since last call.
// returns number of changes (irrespective of type)

int jhcNodePool::Changes ()
{
  int sum = add + del + arg + mod;

  add = 0;
  arg = 0;
  del = 0;
  mod = 0;
  return sum;
}


///////////////////////////////////////////////////////////////////////////
//                             Main Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Instantiate some pattern in memory using the given bindings.
// bindings are augmented to include new nodes built during graphlet copy
// if conf = 0 belief remains the same (either lookup old or copied new)
// if conf < 0 belief is immediately set to value (as opposed to setting default)
// returns 1 if successful, negative for problem

int jhcNodePool::Assert (const jhcGraphlet& pat, jhcBindings& b, double conf, int tval, const jhcNodeList *univ)
{
  jhcNetNode *focus, *mate, *probe, *arg;
  int i, j, na, n = pat.NumItems();

  // go through all nodes in the pattern
  for (i = 0; i < n; i++)
  {
    // get main or halo node related to this item
    focus = pat.Item(i);
    if ((mate = lookup_make(focus, b, univ)) == NULL)
      return -1;
    if ((conf < 0.0) && (focus->Default() > 0.0))
      mate->SetBelief(-conf);
    mate->TopMax(tval);                          // may not have args yet
    mate->GenMax(ver);                           // re-check fluents

    // check all arguments of this item
    na = focus->NumArgs();
    for (j = 0; j < na; j++)
    {
      // add argument if missing (e.g. extra "wrt" or node outside pattern)
      probe = focus->Arg(j);
      if ((arg = lookup_make(probe, b, univ)) == NULL)
        return -2;
//      if (conf < 0.0)
//        arg->SetBelief(-conf);
      arg->TopMax(tval);
      arg->GenMax(ver);                          // re-check fluents
      if (!mate->HasVal(focus->Slot(j), arg))
        mate->AddArg(focus->Slot(j), arg); 
    }
  }
  return 1;
}


//= Get equivalent node from bindings else make new node in this pool.
// NOTE: generally lexical terms will have their own nodes in the bindings

jhcNetNode *jhcNodePool::lookup_make (jhcNetNode *n, jhcBindings& b, const jhcNodeList *univ) 
{
  jhcNetNode *focus = b.LookUp(n);

  if ((focus == NULL) && ((univ == NULL) || !univ->InList(n)))
  {
    // make a new node similar to reference (adds to acc)
    focus = MakeNode(n->base, n->lex, n->inv, n->blf0, n->evt);  
    focus->tags = n->tags;
    focus->SetString(n->Literal());
    b.Bind(n, focus);                  // might be used later
    if (focus->Halo())
      focus->SetDefault(0.0);          // default blf = 0 in halo
  }
  else 
  {
    // node already exists in pool or external universe
    if (focus == NULL)
      focus = n;
    if (acc != NULL)
      acc->AddItem(focus);
  }
  return focus;
}


//= Set generation of node to current pool version, or some particular value.

jhcNetNode *jhcNodePool::SetGen (jhcNetNode *n, int v) const
{
  if (n != NULL) 
    n->GenMax((v > 0) ? v : ver);
  return n;
}
 

//= If pre-exsiting item re-used, update its recency and make sure it is in any graphlet.

jhcNetNode *jhcNodePool::MarkRef (jhcNetNode *n)
{
  if (n == NULL) 
    return NULL; 
  n->ref = ++rnum;
  if (acc != NULL) 
    acc->AddItem(n); 
  return n;
}


//= Move node to head of list for searching, irrespective of ID.
// returns 1 if successful, 0 if node not from this pool

int jhcNodePool::Refresh (jhcNetNode *n)
{
  jhcNetNode *prev;
  int bin;

  // see if already at head of list and find node before this one in list
  if (n == NULL)
    return 0;
  bin = ((nbins <= 1) ? 0 : n->hash);
  prev = bucket[bin];
  if (prev == n)
    return 1;
  while (prev != NULL)
  {
    if (prev->next == n)
      break;
    prev = prev->next;
  }

  // splice out of old position then add in again at head of list
  if (prev == NULL)
    return 0;                          // node not in pool
  prev->next = n->next;
  n->next = bucket[bin];
  bucket[bin] = n;
  return 1;
}


//= Make sure all elements in description appear near head of node list.
// list position determines order objects are tried by jhcSituation matcher

void jhcNodePool::Refresh (const jhcGraphlet& gr)
{
  int i, ni = gr.NumItems();

  for (i = 0; i < ni; i++)
    Refresh(gr.Item(i));
}


///////////////////////////////////////////////////////////////////////////
//                           Basic Construction                          //
///////////////////////////////////////////////////////////////////////////

//= Add a locally managed node of some type to end of list.
// initial belief is generally zero, call Actualize to get value set here
// but if "def" is negative, then "blf" is immediately set also
// returns a new node if successful else NULL

jhcNetNode *jhcNodePool::MakeNode (const char *kind, const char *word, int neg, double def, int done)
{
  jhcNetNode *item;
  int id0 = label + 1, id = ((dn <= 0) ? id0 : -id0);

  // make sure nothing with that number already exists
  if ((item = create_node(kind, id, 0, 0)) == NULL)
    return NULL;
  
  // bind some other fields
  item->GenMax(ver);                // useful for CHK directive and fluents
  item->inv = neg;
  item->evt = done;
  item->SetDefault(fabs(def));      // usually needs to be actualized
  if (def < 0.0)
    item->SetBelief(fabs(def));     // force belief right now
  if (word != NULL)
    update_lex(item, word);
  return item;
}


//= Create a new node with the given base kind and exact instance number.
// generally orders list so HIGHEST absolute ids (newest) toward head
// always creates nodes in bucket[0], moved later when assigned a lex
// returns NULL if a node with the given id already exists

jhcNetNode *jhcNodePool::create_node (const char *kind, int id, int chk, int omit)
{
  jhcNetNode *n2, *n = Pool();

  // check if desired id is possible in this space
  if (((id < 0) && (dn <= 0)) || ((id > 0) && (dn > 0)))
    return NULL;

  // barf if node already exists (have to look in all buckets)
  if (chk > 0)
    while (n != NULL)
    {
      if (n->id == id)
        return NULL;
      n = Next(n);
    }

  // make a new one, set basic type, and build nick name
  n2 = new jhcNetNode;
  n2->id = id;
  label = __max(label, abs(id));
  strcpy_s(n2->base, ((kind != NULL) ? kind : "unk"));
  sprintf_s(n2->nick, "%s%+d", n2->Kind(), -(n2->Inst())); 

  // tack onto head of list (hash code unknown so far)
  n2->next = bucket[0];  
  bucket[0] = n2;
  pop[0] += 1;
  psz++;

  // possibly add to current accumulator graphlet
  if ((omit <= 0) && (acc != NULL))
    acc->AddItem(n2);
  n2->vis = vis0;                      // can default to invisible
  add++;
  return n2;
}


//= Create a new node to represent a property of this node.
// initial belief is generally zero, call Actualize to get value set here
// but if "def" is negative, then "blf" is immediately set also
// if "chk" > 0 then skips if property already exists (does not check its properties)
// "args" is number of distinct slot names the predicate will eventually have
// returns a pointer to the appropriate property node or NULL if error

jhcNetNode *jhcNodePool::AddProp (jhcNetNode *head, const char *role, const char *word,
                                  int neg, double def, int chk, int args)
{
  jhcNetNode *item;

  // sanity check then search for existing (update recency if found)
  if (head == NULL)
    return NULL;
  if (chk > 0)
    if ((item = head->FindProp(role, word, neg, fabs(def))) != NULL)
      if (item->Arity() == args)
        return SetGen(item);

  // try to create a new property
  if (head->PropsFull())
    return NULL;
  item = MakeNode(role, word, neg, def);
  item->AddArg(role, head);
  arg++;
  return item;
}


//= Create nodes to represent properties with degree modifiers like "very tall".
// initial belief is generally zero, call Actualize to get value set here
// but if "def" is negative, then "blf" is immediately set also
// if "chk" > 0 then skips if property with degree already exists (does not check further)
// "args" is number of distinct slot names the base predicate will eventually have
// returns pointer to the appropriate base property node (not degree) or NULL if error

jhcNetNode *jhcNodePool::AddDeg (jhcNetNode *head, const char *role, const char *word, 
                                 const char *amt, int neg, double def, int chk, int args)
{
  jhcNetNode *prop, *mod;

  // sanity check and degenerate case
  if (head == NULL)
    return NULL;
  if ((amt == NULL) || (*amt == '\0'))
    return AddProp(head, role, word, neg, def, chk);

  // search for existing (update recency if found)
  if (chk > 0)
    if ((prop = head->FindProp(role, word, neg, fabs(def))) != NULL)
      if (prop->Arity() == args)
        if ((mod = prop->FindProp("deg", amt, neg, fabs(def))) != NULL)
          if (mod->Arity() == 1)
          {
            SetGen(mod);
            return SetGen(prop);
          }

  // create a new property with given degree
  if (head->PropsFull())
    return NULL;
  prop = MakeNode(role, word, neg, def);
  prop->AddArg(role, head);
  mod = MakeNode("deg", amt, neg, def);
  mod->AddArg("deg", prop);
  arg += 2;
  return prop;
}


//= Change the predicate term associated with a node.

void jhcNodePool::SetLex (jhcNetNode *head, const char *txt)
{
  if ((head != NULL) && (txt != NULL))
    update_lex(head, txt);
}


//= Determine search bin for node based on first few letters of lex.
// re-files node under the appropriate bin

void jhcNodePool::update_lex (jhcNetNode *n, const char *wd)
{
  jhcNetNode *list, *prev = NULL;
  int v0, v1, h, h0 = __max(0, __min(n->hash, 675));

  // copy word if different than before
  if (strcmp(wd, n->lex) == 0)
    return;
  strcpy_s(n->lex, wd);

  // assign to one of 26 * 25 + 25 + 1 = 676 buckets 
  if (wd[0] == '\0')
    h = 0;
  else
  {
    v0 = (int)(tolower(wd[0]) - 'a');
    v0 = __max(0, __min(v0, 25));
    v1 = (int)(tolower(wd[1]) - 'a');    // including '\0'
    v1 = __max(0, __min(v1, 25));
    h = (26 * v0) + v1 + 1;
  }

  // record hash (always) and exit if not hashed
  n->hash = h;
  if ((h == h0) || (nbins <= 1))
    return;

  // find entry in old bucket 
  list = bucket[h0];
  while (list != NULL)
  {
    if (list == n)
      break;
    prev = list;
    list = list->next;
  }

  // splice out of old list (if found)
  if (list != NULL)
  {
    if (prev == NULL)
      bucket[h0] = n->next;
    else
      prev->next = n->next;
    pop[h0] -= 1;
  }

  // add to head of new list
  n->next = bucket[h];
  bucket[h] = n;
  pop[h] += 1;
}


///////////////////////////////////////////////////////////////////////////
//                             List Editing                              //
///////////////////////////////////////////////////////////////////////////

//= Delete a particular node from the pool.
// returns 1 if found, 0 if not in pool

int jhcNodePool::RemNode (jhcNetNode *n)
{
  jhcNetNode *now, *prev = NULL;
  int bin;

  // sanity check
  if (n == NULL)
    return 0;
  bin = ((nbins <= 1) ? 0 : n->hash);

  // find preceding node in list
  now = bucket[bin];
  while (now != NULL)
  {
    if (now == n)
      break;
    prev = now;
    now = now->next;
  }

  // splice out of list
  if (now == NULL)
    return 0;                // not in pool
  if (prev == NULL)
    bucket[bin] = n->next;
  else
    prev->next = n->next;
  pop[bin] -= 1;

  // delete node
  delete n;
  del++;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                               Searching                               //
///////////////////////////////////////////////////////////////////////////

//= Find a node associated with the particular person's name.
// checks first for full name then for just first name (part before space)
// return most recent matching node, NULL if none

jhcNetNode *jhcNodePool::FindName (const char *full) const
{
  char first[80];
  jhcNetNode *n, *p;
  char *sep;
  int i;

  // sanity check
  if ((full == NULL) || (*full == '\0'))
    return NULL;

  // search for full name (most recent at HEAD of list)
  for (i = 0; i < nbins; i++)
  {
    n = bucket[i];
    while (n != NULL)
    {
      if ((n->Neg() <= 0) && n->LexMatch(full))
        if ((p = n->Val("ref")) != NULL)
          return p;
      n = n->next;
    }
  }

  // get just first name
  strcpy_s(first, full);
  if ((sep = strchr(first, ' ')) == NULL)
    return NULL;

  // search for first name (most recent at end of list)
  for (i = 0; i < nbins; i++)
  {
    n = bucket[i];
    while (n != NULL)
    {
      if ((n->Neg() <= 0) && n->LexMatch(first))
        if ((p = n->Val("ref")) != NULL)
          return p;
      n = n->next;
    }
  }
  return NULL;
}


//= Find a node with the given nickname in pool.
// can optionally create a new node if nothing found
// returns NULL if not found or cannot make due to conflict

jhcNetNode *jhcNodePool::FindNode (const char *desc, int make, int omit)
{
  char kind[40];
  jhcNetNode *n;
  int i, id;

  // break description into kind and id number
  if ((id = parse_name(kind, desc, 40)) < 0)
    return NULL;
  if (((id < 0) && (dn <= 0)) || ((id > 0) && (dn > 0)))
    return NULL;

  // look for existing node that matches description
  for (i = 0; i < nbins; i++)
  {
    n = bucket[i];
    while (n != NULL)
    {
      if (n->Inst() == id) 
      {
        // make sure ID not used by some different kind of node (common mistake)
        if (strcmp(n->Kind(), kind) == 0)
          return n;
        if (make > 0)
          jprintf(">>> Cannot make %s because %s%+d exists in jhcNodePool::FindNode !\n", desc, n->Kind(), -(n->Inst()));
        return NULL;
      }
      n = n->next;
    }
  }

  // possibly create a new node
  if (make <= 0)
    return NULL;
  return create_node(kind, id, 1, omit);
}


//= Break a desription like "obj-27" into "obj" and 27.
// for halo elements like "act+34" gives "act" and -34
// returns -1 if bad format

int jhcNodePool::parse_name (char *kind, const char *desc, int ssz)
{
  char num[20];
  const char *mid, *end;
  int n, nid;

  // get kind part 
  if ((mid = extract_kind(kind, desc, ssz)) == NULL)
    return -1;

  // try to get id number in "kind-inst" pattern
  end = mid + 1;
  while (*end != '\0')
  {
    if (strchr(" /t/n", *end) != NULL)
      break;
    end++;
  }
  n = (int)(end - mid);
  if ((n <= 0) || (n >= 20))
    return -1;
  strncpy_s(num, mid, n);              // always adds terminator

  // convert string to number (normally negative)
  if (sscanf_s(num, "%d", &nid) != 1)
    return -1;
  return -nid;
}


//= Get just the kind part out of a compound reference (e.g. "foo-23" -> "foo").
// returns pointer to delimiter in string, NULL if bad format

const char *jhcNodePool::extract_kind (char *kind, const char *desc, int ssz)
{
  const char *mid;
  int n;

  if ((mid = strchr(desc, '-')) == NULL)
    if ((mid = strchr(desc, '+')) == NULL)
      return NULL;
  n = (int)(mid - desc);
  if ((n <= 0) || (n >= ssz))
    return NULL;
  strncpy_s(kind, ssz, desc, n);       // always adds terminator
  return mid;
}


//= Get a mutable pointer to some element in pool based on an immutable pointer.
// used by jhcActionTree to retrieve rule result fact given a binding key

jhcNetNode *jhcNodePool::Wash (const jhcNetNode *ref) const
{
  jhcNetNode *n = NULL;
  int bin;

  if (ref == NULL)
    return NULL;
  bin = ((nbins <= 1) ? 0 : ref->hash);
  while ((n = Next(n, bin)) != NULL)
    if (n == ref)
      return n;
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                           Virtual Overrides                           //
///////////////////////////////////////////////////////////////////////////

//= Tell if particular node is a member of this pool.
// simply chases linearly down list to end

bool jhcNodePool::InList (const jhcNetNode *n) const
{
  const jhcNetNode *p;

  if (n == NULL)
    return false;
  p = Pool(n->hash);
  while (p != NULL)
  {
    if (p == n)
      return true;
    p = p->next;
  }
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                           Writing Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Save all nodes in order by instance number (descending).
// returns: 1 = success, 0 = format problem, -1 = file problem

int jhcNodePool::Save (const char *fname, int lvl) const
{
  FILE *out;
  int ans;

  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  ans = save_nodes(out, lvl); 
  fclose(out);
  return((ferror(out) != 0) ? -1 : ans);
}


//= Rummages through all bins to return nodes in sorted order.
// returns: 1 = success, 0 = format problem, -1 = file problem

int jhcNodePool::save_nodes (FILE *out, int lvl) const
{
  jhcNetNode *win, *n = NULL;
  int i, best, kmax = 3, nmax = 1, rmax = 3, last = label + 1;
 
  // get print field sizes and clear marks
  if (psz <= 0)
    return 0;
  while ((n = NextNode(n)) != NULL)
    if (n->Visible())
      n->TxtSizes(kmax, nmax, rmax);

  // save all as: node -link-> arg and list blf not blf0
  while (1)
  {
    // find the next highest numbered node (lower than last)
    win = NULL;
    n = NULL;
    while ((n = NextNode(n)) != NULL)
      if (n->Visible())
      {
        i = n->Inst();
        if ((i < last) && ((win == NULL) || (i > best)))
        {
          win = n;
          best = i;
        }
      }
 
    // possibly print it then search again
    if (win == NULL)
      break;
    if ((win->NumArgs() > 0) || (win->Lex() != NULL) || (win->quote != NULL) || 
        (win->inv != 0) || (win->blf != 1.0) || (win->tags != 0))
      win->Save(out, lvl, kmax, nmax, rmax, -2);       
    last = best;
  }

  // terminate last line
  jfprintf(out, "\n");
  return 1;
}


//= Save all nodes in one or all bins to a file in listed order.
// this is the order in which they would be matched (e.g. Refresh)
// returns: 1 = success, 0 = format problem, -1 = file problem

int jhcNodePool::SaveBin (const char *fname, int bin, int lvl) const
{
  FILE *out;
  int ans;

  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  ans = save_bins(out, bin, lvl); 
  fclose(out);
  return((ferror(out) != 0) ? -1 : ans);
}


//= Save all nodes from a particular bin (or all if bin < 0).
// returns: 1 = success, 0 = format problem, -1 = file problem

int jhcNodePool::save_bins (FILE *out, int bin, int lvl) const
{
  jhcNetNode *n = NULL;
  int kmax = 3, nmax = 1, rmax = 3;
 
  // get print field sizes and clear marks
  if (psz <= 0)
    return 0;
  while ((n = NextNode(n, bin)) != NULL)
    if (n->Visible())
      n->TxtSizes(kmax, nmax, rmax);

  // save all as: node -link-> arg and list blf not blf0
  n = NULL;
  while ((n = NextNode(n, bin)) != NULL)
    if (n->Visible())
      if ((n->NumArgs() > 0) || (n->Lex() != NULL) || (n->quote != NULL) || 
          (n->inv != 0) || (n->blf != 1.0) || (n->tags != 0))
        n->Save(out, lvl, kmax, nmax, rmax, -2);       

  // terminate last line
  jfprintf(out, "\n");
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Reading Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Clear out table of translations from string names to actual nodes.
// lets input file have both "hq-1" and "ako-1" and "obj-1" (for instance)

void jhcNodePool::ClrTrans (int n)
{
  int i;

  // alway get rid of old arrays
  delete [] trans;
  if (surf != NULL)
    for (i = 0; i < tmax; i++)
      delete [] surf[i];
  delete [] surf;

  // clear variables
  trans = NULL;
  surf = NULL;
  tmax = 0;
  nt = 0;

  // possibly create new arrays
  if (n <= 0)
    return;
  trans = new jhcNetNode * [n];
  surf = new char * [n];
  for (i = 0; i < n; i++)
    surf[i] = new char [40];
  tmax = n;  
}


//= Read a number of nodes from a file, possibly appending them to graph.
// returns number of nodes added, -1 = format problem, -2 = file problem

int jhcNodePool::Load (const char *fname, int add)
{
  jhcTxtLine in;
  int ans, psz0 = psz;

  if (add <= 0)
    PurgeAll();
  if (!in.Open(fname))
    return -2;
  ClrTrans();
  ans = Load(in);
  ClrTrans(0);
  if (ans <= 0)
  {
    jprintf("Syntax error at line %d in jhcNodePool::Load\n", in.Last());
    return -1;
  }
  return(psz - psz0);
}


//= Read at current location in a file to fill in details of self.
// stops after first syntax error, some nodes may be only partially filled
// can optionally set default belief to 1.0 if "tru" > 0
// returns: 2 = ok + delimiter, 1 = success, 0 = bad format, -1 = file problem

int jhcNodePool::Load (jhcTxtLine& in, int tru)
{
  char arrow[40];
  const char *desc, *link;
  jhcNetNode *topic = NULL;
  int ans, sz0 = psz;

  // keep reading through file until terminator or end
  while (1)
  {
    // re-use previously peeked line or get new line from file
    if ((in.Next() == NULL) || in.TagLine())
      return((psz > sz0) ? 1 : 0);

    // possibly get new topic node (always need one)
    if (in.Blank())
    {
      topic = NULL;
      in.Flush();
      continue;
    }
    if ((topic = chk_topic(topic, in, tru)) == NULL)
      return 0;
    if (acc != NULL)
      acc->AddItem(topic);

    // handle as relation-oriented (allow naked nodes)
    ans = 1;
    if (in.Begins("]"))
      ans = 2;
    else if ((desc = in.Token()) != NULL)
    {
      strcpy_s(arrow, desc); 
      link = link_name(arrow);
      ans = parse_arg(topic, link, in, tru);
    }

    // line finished, check for bad format or bracket terminator
    in.Flush();
    if (ans != 1)
      return ans;
  }
  return((in.Error()) ? -1 : 1); 
}


//= Parse and bind a new topic node (if any) at front of line.
// advances string pointer to portion after node found
// can optionally set default belief to 1.0 if "tru" > 0
// return topic unaltered if nothing there, NULL if present but not resolved

jhcNetNode *jhcNodePool::chk_topic (jhcNetNode *topic, jhcTxtLine& in, int tru)
{
  const char *desc;

  // see if only -x-> or -x- style connector on line (no <-x- anymore)
  // else try to make a new node if reasonable pattern
  if (in.First("-"))
    return topic;
  if ((desc = in.Token()) == NULL)
    return topic;
  return find_trans(desc, tru);
}


//= Modify string to get word embedded in arrow.
// returns a pointer to start of string

const char *jhcNodePool::link_name (char *arrow) const
{
  char *end, *start = arrow;

  while (*start != '\0')
  {
    if (*start != '-')
      break;
    start++;
  }
  if ((end = strchr(start, '-')) != NULL)
    *end = '\0';
  return start;
}


//= Interpret line as node -link-> arg possibly with tags.
// "-lex-" link actually results in a new property being added
// can optionally set default belief to 1.0 if "tru" > 0
// return: 2 = ok + delimiter, 1 = success, 0 = bad format

int jhcNodePool::parse_arg (jhcNetNode *n, const char *slot, jhcTxtLine& in, int tru)
{
  const char *arg;
  jhcNetNode *n2;
  double val;
  int ver;

  // possibly handle multi-word lexical item and grammatical tags
  if (strcmp(slot, "str") == 0)
    return get_str(n, in);  
  if (strcmp(slot, "lex") == 0)
    return get_lex(n, in);
  if (strcmp(slot, "tag") == 0)
    return get_tags(n->tags, in);

  // try to interpret next chunk of text as some tag
  if ((arg = in.Token()) == NULL)
    return 0;
  if (strcmp(slot, "ach") == 0)
  {
    n->evt = 1;
    if (sscanf_s(arg, "%d", &ver) != 1)          // success or failure
      return 0;
    n->inv = ((ver > 0) ? 0 : 1);
  }
  else if (strcmp(slot, "neg") == 0)
  {
    if (sscanf_s(arg, "%d", &(n->inv)) != 1)
      return 0;
  }
  else if (strcmp(slot, "blf") == 0)
  {
    if (sscanf_s(arg, "%lf", &val) != 1)  
      return 0;
    n->SetBelief(val);                           // both blf and blf0
  }
  else
  {
    // otherwise add specified node as an argument
    if ((n2 = find_trans(arg, tru)) == NULL)
      return 0;
    n->AddArg(slot, n2);
    arg++;
  }

  // check for bracket at end
  return((in.First("]")) ? 2 : 1);
}


//= Extract multiple words of quotation and save in node.
// returns 2 if trailing bracket, 1 if something, 0 if nothing

int jhcNodePool::get_str (jhcNetNode *item, jhcTxtLine& in) 
{
  char *txt;
  int i, n, ans = 1;

  // copy most of string to destination
  item->SetString(in.Clean());
  txt = item->quote;
  n = (int) strlen(txt);

  // walk backward from end until non-whitespace
  for (i = n - 1; i >= 0; i--)
    if (txt[i] == ']')
      ans = 2;                  // bracket found
    else if (txt[i] != ' ')
      break;

  // barf if nothing left, else terminate string early
  if (i < 0)
    return 0;
  txt[i + 1] = '\0';
  return ans;
}


//= Extract multiple words of text, ignoring trailing whitespace.
// returns 2 if trailing bracket, 1 if something, 0 if nothing

int jhcNodePool::get_lex (jhcNetNode *item, jhcTxtLine& in) 
{
  char txt[80];
  int i, n, ans = 1;

  // check for negation then copy most of string to destination
  if (in.Begins("*"))
  {
    item->inv = 1;                     // negative assertion
    in.Skip("*");
  }
  strcpy_s(txt, in.Clean());
  n = (int) strlen(txt);

  // walk backward from end until non-whitespace
  for (i = n - 1; i >= 0; i--)
    if (txt[i] == ']')
      ans = 2;                         // bracket found
    else if (txt[i] != ' ')
      break;

  // barf if nothing left, else terminate string early
  if (i < 0)
    return 0;
  txt[i + 1] = '\0';

  // associate the word or phrase with this item 
  update_lex(item, txt);
  return ans;
}


//= Set bit vector "tags" based on text found in rest of line.
// returns 2 if trailing bracket, 1 if all good, 0 if some bad term

int jhcNodePool::get_tags (UL32& tags, jhcTxtLine& in) const
{
  const char *arg;
  int i;

  // scan through each term on this line 
  tags = 0;
  while ((arg = in.Token()) != NULL)
  {
    // check for special ending else compare to know tags
    if (*arg == ']')
      return 2;
    for (i = 0; i < JTV_MAX; i++)
      if (strcmp(arg, JTAG_STR[i]) == 0)
        break;

    // set bit corresponding to tag found (if valid)
    if (i >= JTV_MAX)
      return 0;
    tags |= (0x01 << i);
  }
  return 1;
}


//= Find or make node for given surface string.
// can optionally set default belief to 1.0 if "tru" > 0

jhcNetNode *jhcNodePool::find_trans (const char *name, int tru)
{
  char bare[40], kind[40];
  const char *desc = bare;
  jhcNetNode *n;
  int i, omit = 0;

  // strip off any parentheses
  strcpy_s(bare, name);
  if (*name == '(')
  {
    i = (int) strlen(bare);
    bare[i - 1] = '\0';
    desc = bare + 1;
    omit = 1;
  }

  // try lookup first (if table exists)
  if (tmax <= 0)
    return FindNode(desc, 1);
  for (i = 0; i < nt; i++)
    if (strcmp(desc, surf[i]) == 0)
      return trans[i];

  // make a new node with given kind but consistent ID
  if (nt >= tmax)
  {
    jprintf(">>> More than %d translations in jhcNodePool::find_trans !\n", tmax);
    return NULL;
  }
  if (extract_kind(kind, desc, 40) == NULL)
    return NULL;
  n = create_node(kind, ++label, 1, omit);
  if (tru > 0)
    n->SetBelief(1.0);
  
  // add pair to translation table
  trans[nt] = n;
  strcpy_s(surf[nt], 40, desc);
  nt++;
  return n;
}


//= Load a network description and accumulate it in some graphlet.
// can optionally set default belief to 1.0 if "tru" > 0
// returns number of nodes added, -1 = format problem, -2 = file problem

int jhcNodePool::LoadGraph (jhcGraphlet *g, jhcTxtLine& in, int tru)
{
  int ans;

  BuildIn(g); 
  ans = Load(in, tru);
  BuildIn(NULL);
  return ans;
}

