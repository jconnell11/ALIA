// jhcNodePool.cpp : singly-linked list of semantic network nodes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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
    delete [] cap;
    delete [] bucket;
  }
}


//= Default constructor initializes certain values.

jhcNodePool::jhcNodePool ()
{
  // direction of numbering
  sep0 = '-';
  dn = 0;
 
  // initial visibility of new nodes and LTM dependence
  vis0 = 1;
  ltm0 = 0;

  // no hashing, just main pool (= bucket[0])
  nbins = 1;
  bucket = &pool;
  cap = &last;
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
    cap[i] = NULL;
    pop[i] = 0;
  }
  psz = 0;
  label = 0;

  // collections and generations
  acc = NULL;
  ver = 1;
  refnum = 0;
  ref0 = 0;

  // changes to members
  xadd = 0;
  xarg = 0;
  xdel = 0;
  xmod = 0;
  xltm = 0;
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
  cap = new jhcNetNode * [nbins];
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


//= Figure out which bin a particular node label belongs in. 
// assigns to one of 26 * 25 + 25 + 1 = 676 buckets 

int jhcNodePool::LexHash (const char *wd) const
{
  int v0, v1;

  if ((wd == NULL) || (wd[0] == '\0'))
    return 0; 
  v0 = (int)(tolower(wd[0]) - 'a');
  v0 = __max(0, __min(v0, 25));
  v1 = (int)(tolower(wd[1]) - 'a');    // including '\0'
  v1 = __max(0, __min(v1, 25));
  return((26 * v0) + v1 + 1);
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
      n = n->NodeTail();
      delete n0;
      xdel++;
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
  if (ref == NULL)
    return Pool(bin);
  if (ref->NodeTail() != NULL)
    return ref->NodeTail();
  if (bin < 0)
    return NextPool(ref);
  return NULL;
}


//= Possibly progress to another bucket to find something.

jhcNetNode *jhcNodePool::NextPool (const jhcNetNode *ref) const
{
  int i;

  for (i = ref->Code() + 1; i < nbins; i++)
    if (bucket[i] != NULL)
      return bucket[i];
  return NULL;
}


//= Determine how many actual nodes are in the pool (and visible).
// "psz" tells how many nodes have been created -- but some may already be deleted
// if "hyp" <= 0 then omits hypothetical items (those with blf <= 0)

int jhcNodePool::NodeCnt (int hyp) const
{
  const jhcNetNode *n;
  int i, cnt = 0;

  for (i = 0; i < nbins; i++)
  {
    n = bucket[i];
    while (n != NULL)
    {
      if (n->Visible() && ((hyp > 0) || !n->HypAny()))
        cnt++;
      n = n->NodeTail();
    }
  }
  return cnt;
}


//= Tell if anything about collection of nodes has changed since last call.
// returns number of changes (irrespective of type)

int jhcNodePool::Changes ()
{
  int sum = xadd + xdel + xarg + xmod + xltm;

  xadd = 0;
  xarg = 0;
  xdel = 0;
  xmod = 0;
  xltm = 0;
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
      arg->TopMax(tval);
      arg->GenMax(ver);                          // re-check fluents
      if (!mate->HasVal(focus->Slot(j), arg))
        mate->AddArg(focus->Slot(j), arg); 
      if ((acc != NULL) && !pat.InList(probe))
        acc->RemItem(arg);                       // important for ACH
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
    focus = MakeNode(n->Kind(), b.LexSub(n), n->Neg(), n->Default(), n->Done());  
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
 

//= Move node to head of list for searching, irrespective of ID.
// returns 1 if successful, 0 if node not from this pool

int jhcNodePool::Refresh (jhcNetNode *n)
{
  int i, na, bin;

  // make sure most this property is the most likely to be enumerated 
  if (n == NULL)
    return 0;
  na = n->NumArgs();
  for (i = 0; i < na; i++)
    n->RefreshArg(i);

  // change position to head of list (if needed)
  bin = ((nbins <= 1) ? 0 : n->Code());
  if (bucket[bin] == n)
    return 1;
  if (rem_from_list(bin, n) <= 0)
    return 0;
  add_to_list(bin, n, 0);
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

//= Add a locally managed node of some type to beginning (or end) of list.
// initial belief is generally zero, call Actualize to get value set here
// but if "def" is negative, then "blf" is immediately set also
// returns a new node if successful else NULL

jhcNetNode *jhcNodePool::MakeNode (const char *kind, const char *word, int neg, double def, int done)
{
  jhcNetNode *item;
  int id0 = label + 1, id = ((dn <= 0) ? id0 : -id0);

  // make sure nothing with that number already exists
  // any new node at beginning of list unless negative IDs mode
  if ((item = create_node(kind, id, 0, 0, dn)) == NULL)
    return NULL;
  
  // bind some other fields
  item->GenMax(ver);                // useful for CHK directive and fluents
  item->SetNeg(neg);
  item->SetDone(done);
  item->SetDefault(fabs(def));      // usually needs to be actualized
  if (def < 0.0)
    item->SetBelief(fabs(def));     // force belief right now
  if (word != NULL)
    update_lex(item, word, dn);
  return item;
}


//= Create a new node with the given base kind and exact instance number.
// generally orders list so HIGHEST absolute ids (newest) toward head
// always creates nodes in bucket[0], moved later when assigned a lex
// generally new nodes come at beginning of list, rev > 0 puts them at end 
// returns NULL if a node with the given id already exists

jhcNetNode *jhcNodePool::create_node (const char *kind, int id, int chk, int omit, int rev)
{
  jhcNetNode *n2, *n = Pool();

  // check if desired id is possible in this space
  if (((id < 0) && (dn <= 0)) || ((id > 0) && (dn > 0)))
    return NULL;

  // barf if node already exists (have to look in all buckets)
  if (chk > 0)
    while (n != NULL)
    {
      if (n->Inst() == id)
        return NULL;
      n = Next(n);
    }

  // make a new one, set basic type, and build nick name
  n2 = new jhcNetNode(id, this);
  label = __max(label, abs(id));
  n2->SetKind(((kind != NULL) ? kind : "unk"), sep0);

  // tack onto node list (hash code unknown so far)
  add_to_list(0, n2, rev);
  psz++;

  // possibly add to current accumulator graphlet
  if ((omit <= 0) && (acc != NULL))
    acc->AddItem(n2);
  n2->Reveal(vis0);                    // can default to invisible
  n2->ltm = ltm0;                      // whether LTM-dependent
  xadd++;
  return n2;
}


//= Generally add a node to the front of a bucket list, but add at end if rev > 0.
// assumes node is not currently in list and has already been extracted from any previous list

void jhcNodePool::add_to_list (int h, jhcNetNode *n, int rev)
{
  if (bucket[h] == NULL)
  {
    bucket[h] = n;                     // add as first and last item
    cap[h] = n;
  }
  else if (rev <= 0) 
  {
    n->SetTail(bucket[h]);
    bucket[h] = n;                     // add at front
  }
  else 
  {
    cap[h]->SetTail(n);                // add at end
    cap[h] = n;
  }
  pop[h] += 1;                         // always one more item in bucket
}


//= Create equivalent node to some reference with same basic lex, neg, and belief.
// does not copy over any arguments (or properties)

jhcNetNode *jhcNodePool::CloneNode (const jhcNetNode& n, int bset)
{
  jhcNetNode *n2 = MakeNode(n.Kind(), n.Lex(), n.Neg(), n.Default(), n.Done());

  if ((bset > 0) && (n2 != NULL))
    n2->SetBelief(n.Belief());
  return n2;
}


//= Add a basic action frame (no arguments) with given verb name. 
// calls arranged so verb label generally comes first in any graphlet 

jhcNetNode *jhcNodePool::MakeAct (const char *word, int neg, double def, int done)
{
  jhcNetNode *act, *fcn = NULL;

  if ((word != NULL) && (*word != '\0'))
    fcn = MakeNode("fcn", word);                 // always neg = 0, def = 1.0
  act = MakeNode("act", NULL, neg, def, done);
  if (fcn != NULL)
    fcn->AddArg("fcn", act);
  return act;
}


//= Make a duplicate of a command along with any modifiers like speed or position.
// returned node is always positive imperative and agent-less (i.e. omits -agt-> "me")
// Note: only used by jhcIntrospect::why_run

jhcNetNode *jhcNodePool::CloneAct (const jhcNetNode *act, int neg)
{
  const jhcNetNode *fcn;
  jhcNetNode *act2;
  int i = 0;

  // find action verb (limit to original query)
  while ((fcn = act->Fact("fcn", i++)) != NULL)
    if (!fcn->Halo() && fcn->Hyp())
      break;

  // make a duplicate action using this verb
  if (fcn == NULL)
    act2 = MakeAct(NULL);
  else
    act2 = MakeAct(fcn->Lex());
  act2->AddArg("obj", act->Val("obj"));
  act2->AddArg("dest", act->Val("dest"));

  // add selected adverbs
  copy_mods(act2, act);
  return act2;
}


//= Hierarchically copy all properties of src node (an action) to destination node.
// skips "fcn" (verb) since already copied and "arg" since part of grounding call

void jhcNodePool::copy_mods (jhcNetNode *dest, const jhcNetNode *src) 
{
  const jhcNetNode *p;
  jhcNetNode *p2, *a;
  int i, j, na, np = src->NumProps();

  // sanity check
  if ((dest == NULL) || (src == NULL))
    return;

  // look for adverbial properties (incl. locations)
  np = src->NumProps();
  for (i = 0; i < np; i++)
    if (src->RoleIn(i, "mod", "dir", "amt", "deg", "cnt") ||
        src->RoleIn(i, "loc", "src"))
    {
      p = src->Prop(i);
      if (!p->Halo() && p->Hyp())                // limit to original query
      {
        // copy basic property node
        p2 = MakeNode(p->Kind(), p->Lex(), p->Neg(), 1.0, p->Done());

        // copy original arguments (mostly)
        na = p->NumArgs();
        for (j = 0; j < na; j++) 
        {
          a = p->Arg(j);
          p2->AddArg(p->Slot(j), ((a == src) ? dest : a));
        }

        // copy any higher-level modifiers (e.g. "very")
        copy_mods(p2, p);
      }
    }
}


//= Make up suitable fact about some owner possessing some object.
// convenience function primarily for jhcGraphizer

jhcNetNode *jhcNodePool::MakePoss (jhcNetNode *owner, jhcNetNode *obj, int neg, double def, int done)
{
  jhcNetNode *fact;

  if ((owner == NULL) || (obj == NULL))
    return NULL;
  fact = MakeNode("has", NULL, neg, def, done);
  AddProp(fact, "fcn", "have");
  fact->AddArg("agt", owner);
  fact->AddArg("obj", obj);
  return fact;
}


//= Create a new node to represent a property of this node.
// initial belief is generally zero, call Actualize to get value set here
// but if "def" is negative, then "blf" is immediately set also
// if "chk" > 0 then skips if property already exists (does not check its modifiers)
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
  xarg++;
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
  xarg += 2;
  return prop;
}


//= Create equivalent node in this pool for a node in some other pool.
// copies various basic properties to new surface for convenience:
//    base  -> Kind()
//    lex   -> LexStr()
//    inv   -> Neg()
//    blf0  -> Default()
//    ach   -> Done()
// Note: a node can only be tethered to one other node, any previous linkage overwritten

jhcNetNode *jhcNodePool::BuoyFor (jhcNetNode *deep)
{
  jhcNetNode *surf;
  
  if (InList(deep))
    return deep;
  surf = CloneNode(*deep);
  surf->SetBelief(deep->Belief());
  if (deep->ObjNode())       
    surf->MoorTo(deep);      // non-object nodes also?
  surf->Reveal();            // wmem default is invisible
  xltm++;
  return surf;
}


//= Change the predicate term associated with a node.
// generally moves node to head of bin list unless negative IDs mode

void jhcNodePool::SetLex (jhcNetNode *head, const char *txt)
{
  if ((head != NULL) && (txt != NULL))
    update_lex(head, txt, dn);
}


//= Determine search bin for node based on first few letters of lex.
// re-files node under the appropriate bin (no lex -> bin = 0)
// generally new nodes come at beginning of list, rev > 0 puts them at end 

void jhcNodePool::update_lex (jhcNetNode *n, const char *wd, int rev)
{
  int h, h0 = __max(0, __min(n->Code(), 675));

  // copy word if different than before
  if (strcmp(wd, n->LexStr()) == 0)
    return;
  n->SetWord(wd);

  // record hash (always) and exit if not hashed
  h = LexHash(wd);
  n->SetHash(h);
  if ((h == h0) || (nbins <= 1))
    return;

  // move node into new bucket (at front or end)
  rem_from_list(h0, n);
  add_to_list(h, n, rev);
}


///////////////////////////////////////////////////////////////////////////
//                             List Editing                              //
///////////////////////////////////////////////////////////////////////////

//= Delete a particular node from the pool.
// returns 1 if found, 0 if not in pool

int jhcNodePool::RemNode (jhcNetNode *n)
{
  int bin, found;

  if (n == NULL)
    return 0;
  bin = ((nbins <= 1) ? 0 : n->Code());
  found = rem_from_list(bin, n);
  delete n;
  if (found <= 0)
    return 0;
  xdel++;
  return 1;
}


//= Cleanly remove item from some bucket and update all list bookkeeping.
// returns 1 if item was found, 0 if not in list

int jhcNodePool::rem_from_list (int h0, jhcNetNode *n)
{
  jhcNetNode *prev = NULL, *list = bucket[h0];

  // find entry in old bucket 
  while (list != NULL)
  {
    if (list == n)
      break;
    prev = list;
    list = list->NodeTail();
  }

  // splice out of old list (if found)
  if (list == NULL)
    return 0;
  if (prev == NULL)
    bucket[h0] = n->NodeTail();
  else
    prev->SetTail(n->NodeTail());
  n->SetTail(NULL);                    // for safety
  pop[h0] -= 1;

  // correct last item pointer (if needed)
  if (cap[h0] != n)
    return 1;
  if (prev != NULL)
    while ((list = prev->NodeTail()) != NULL)
      prev = list;
  cap[h0] = prev;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                               Utilities                               //
///////////////////////////////////////////////////////////////////////////

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
      n = n->NodeTail();
    }
  }

  // possibly create a new node
  if (make <= 0)
    return NULL;
  return create_node(kind, id, 1, omit, dn);
}


//= Break a desription like "obj-27" into "obj" and 27.
// for halo elements like "act+34" gives "act" and -34
// returns -1 if bad format

int jhcNodePool::parse_name (char *kind, const char *desc, int ssz)
{
  char num[20] = "";
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

  if ((mid = strchr(desc, sep0)) == NULL)
    if ((mid = strpbrk(desc, "-+:")) == NULL)
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
  bin = ((nbins <= 1) ? 0 : ref->Code());
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
  return((n != NULL) && n->Home(this));
/*
  const jhcNetNode *p;

  if (n == NULL)
    return false;
  p = Pool(n->Code());
  while (p != NULL)
  {
    if (p == n)
      return true;
    p = p->NodeTail();
  }
  return false;
*/
}


///////////////////////////////////////////////////////////////////////////
//                           Writing Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Save all nodes sorted by instance number for human perusal.
// if "hyp" <= 0 then omits hypothetical items (those with blf <= 0)
// returns: positive = number of nodes saved, 0 = format problem, -1 = file problem
// NOTE: to preserve recency order on reload use SaveBin instead

int jhcNodePool::Save (const char *fname, int lvl, int hyp) const
{
  FILE *out;
  int ans;

  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  ans = sort_nodes(out, lvl, 0, hyp);        
  fclose(out);
  return((ferror(out) != 0) ? -1 : ans);
}


//= Rummages through all bins to return nodes in sorted order.
// all returned nodes will have instance numbers >= imin
// generally only for debugging (i.e. human consumption like Print)
// if "hyp" <= 0 then omits hypothetical items (those with blf <= 0)
// returns: 1 = success, 0 = format problem, -1 = file problem

int jhcNodePool::sort_nodes (FILE *out, int lvl, int imin, int hyp) const
{
  jhcNetNode *win, *n = NULL;
  int i, best, kmax = 3, nmax = 1, rmax = 3, last = 0;

  // get print field sizes and clear marks
  if (psz <= 0)
    return 0;
  while ((n = NextNode(n)) != NULL)
    if (n->Visible() && ((hyp > 0) || !n->HypAny()))
      n->TxtSizes(kmax, nmax, rmax);

  // save all as: node -link-> arg and list blf not blf0
  while (1)
  {
    // find the next lowest numbered node (higher than last)
    win = NULL;
    n = NULL;
    while ((n = NextNode(n)) != NULL)
      if (n->Visible() && ((hyp > 0) || !n->HypAny()))
        if ((i = abs(n->Inst())) >= imin)
          if ((i > last) && ((win == NULL) || (i < best)))
          {
            win = n;
            best = i;
          }

    // possibly print it then search again
    if (win == NULL)
      break;
    if ((win->NumArgs() > 0) || (win->Lex() != NULL) || win->String() || 
        (win->Neg() != 0) || (win->Belief() != 1.0) || (win->tags != 0))
      win->Save(out, lvl, kmax, nmax, rmax, -2);                
    last = best;
  }

  // terminate last line 
  jfprintf(out, "\n");
  return 1;
}


//= Save all nodes in one or all bins to a file in listed order.
// this is the order in which they would be matched (e.g. Refresh)
// nodes must be visible but can be hypothetical or nor
// returns: positive = number of nodes saved, 0 = format problem, -1 = file problem

int jhcNodePool::SaveBin (const char *fname, int bin, int imin) const
{
  FILE *out;
  int cnt;

  if (fopen_s(&out, fname, "w") != 0)
    return -1;
  cnt = save_bins(out, bin, imin); 
  fclose(out);
  return((ferror(out) != 0) ? -1 : cnt);
}


//= Save all nodes from a particular bin (or all if bin < 0).
// returns: positive = nodes daved, 0 = format problem, -1 = file problem

int jhcNodePool::save_bins (FILE *out, int bin, int imin) const
{
  jhcNetNode *n = NULL;
  int kmax = 3, nmax = 1, rmax = 3, cnt = 0;
 
  // get print field sizes and clear marks
  if (psz <= 0)
    return 0;
  while ((n = NextNode(n, bin)) != NULL)
    if (n->Visible() && (abs(n->Inst()) >= imin))
      n->TxtSizes(kmax, nmax, rmax);

  // save all as: node -link-> arg and list blf not blf0
  n = NULL;
  while ((n = NextNode(n, bin)) != NULL)
    if (n->Visible() && (abs(n->Inst()) >= imin))
      if ((n->NumArgs() > 0) || (n->Lex() != NULL) || n->String() || 
          (n->Neg() != 0) || (n->Belief() != 1.0) || (n->tags != 0))
      {
        n->Save(out, 0, kmax, nmax, rmax, -2);       
        cnt++;
      }

  // terminate last line
  jfprintf(out, "\n");
  return cnt;
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

int jhcNodePool::Load (const char *fname, int add, int nt)
{
  jhcTxtLine in;
  int ans, psz0;

  // possibly clear all assertions then try opening file
  if (add <= 0)
    PurgeAll();
  if (!in.Open(fname))
    return -2;
  ClrTrans(nt);
  psz0 = psz;

  // get assertions then cleanup
  if (in.NextContent() != NULL)                  // skip blank or node count
  {
    ans = Load(in, 1);
    ClrTrans(0);
    if (ans <= 0)
    {
      jprintf("Syntax error at line %d in jhcNodePool::Load\n", in.Last());
      return -1;
    }
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
      return 1;
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
  int ver, neg, ext = 1;

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
    n->SetDone(1);
    if (sscanf_s(arg, "%d", &ver) != 1)          // success or failure
      return 0;
    n->SetNeg((ver > 0) ? 0 : 1);
  }
  else if (strcmp(slot, "neg") == 0)
  {
    if (sscanf_s(arg, "%d", &neg) != 1)
      return 0;
    n->SetNeg(neg);
  }
  else if (strcmp(slot, "ext") == 0)
  {
    n->TmpBelief(0.0);                            // blf
    ext = 0;                                      // only set blf0 later
  }
  else if (strcmp(slot, "blf") == 0)
  {
    if (sscanf_s(arg, "%lf", &val) != 1)  
      return 0;
    n->SetDefault(val);                           // blf0
    if (ext > 0)
      n->TmpBelief(val);                          // blf
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
  char txt[200];
  int i, n, ans = 1;

  // get rest of line into locally modifiable string
  strcpy_s(txt, in.Clean());
  n = (int) strlen(txt);

  // walk backward from end until non-whitespace
  for (i = n - 1; i >= 0; i--)
    if (txt[i] == ']')
      ans = 2;                  // bracket found
    else if (txt[i] != ' ')
      break;

  // barf if nothing left, else terminate string early
  txt[i + 1] = '\0';
  item->SetString(txt);
  return((i < 0) ? 0 : ans);
}


//= Extract multiple words of text, ignoring trailing whitespace.
// returns 2 if trailing bracket, 1 if something, 0 if nothing

int jhcNodePool::get_lex (jhcNetNode *item, jhcTxtLine& in) 
{
  char txt[80];
  int i, n, ans = 1;

  // copy most of string to destination
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

  // associate the word or phrase with this item (node at end of list)
  update_lex(item, txt, 1);
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
  n = create_node(kind, ++label, 1, omit, 1);    // node at end of list
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

int jhcNodePool::LoadGraph (jhcGraphlet& g, jhcTxtLine& in, int tru)
{
  int ans;

  BuildIn(g); 
  ans = Load(in, tru);
  BuildIn(NULL);
  return ans;
}

