// jhcDeclMem.cpp : long term factual memory for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2022 Etaoin Systems
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

#include "Reasoning/jhcActionTree.h"   // since only spec'd as class in hdr

#include "Reasoning/jhcDeclMem.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcDeclMem::~jhcDeclMem ()
{
}


//= Default constructor initializes certain values.

jhcDeclMem::jhcDeclMem ()
{
  int i;

  // pre-sorted node access
  MakeBins();

  // section starting IDs
  for (i = 0; i < 4; i++)
    first[i] = 0;

  // special node nicknames and LTM-dependence
  sep0 = ':';
  ltm0 = 1;

  // scoring thresholds
  ath  = 1;
  rth  = 3;
  alts = 3;

  // messages for selection and retrieval
  enc = 0;
  ret = 0;
//enc = 3;
//ret = 3;
}


///////////////////////////////////////////////////////////////////////////
//                          Explicit Formation                           //
///////////////////////////////////////////////////////////////////////////

//= Create a graphlet in long term memory which is relatively unique.
// updates deep memory structure to improve uniqueness of references
// returns 1 if successfully encoded, negative for problem.

int jhcDeclMem::Remember (jhcNetNode *fact)
{
  jhcGraphlet desc;
  jhcBindings xfer;
  jhcNetNode *n, *n2;
  int i, j, ni, na, nb, nb0 = -1;

  // possibly announce entry and cached belief threshold
  if ((fact == NULL) || (atree == NULL))
    return -1;
  jprintf(1, enc, "\nREMEMBER: %s (%s)\n", fact->Nick(), fact->LexStr());
  bth = atree->MinBlf();

  // build a unique description from working memory facts
  add_node(desc, fact, NULL, 1);
  ni = desc.NumItems();
  if (enc >= 1)
  {
    jprintf("\n");
    desc.Print("  LTM pattern");
    jprintf("\n");
  }

  // repeatedly scan through description until all facts migrated 
  jprintf(1, enc, "  Migrating %d nodes to LTM ...\n", ni);
  while ((nb = xfer.NumPairs()) > nb0)
  {
    // first migrate object nodes (no args) then work up from there
    nb0 = nb;
    for (i = 0; i < ni; i++)
    {
      // check if node already migrated or in LTM 
      n = desc.Item(i);
      if (xfer.LookUp(n) != NULL)
        continue;
      if ((n2 = n->Moor()) != NULL)
      {
        jprintf(1, enc, "    %s = %s\n", n->Nick(), n2->Nick());
        xfer.Bind(n, n2);
        continue;
      }

      // check if all arguments have been migrated
      na = n->NumArgs();
      for (j = 0; j < na; j++)
        if (xfer.LookUp(n->ArgSurf(j)) == NULL)
          break;
      if (j < na)
        continue;

      // find existing LTM equivalent or make new node 
      n2 = ltm_node(n, xfer);
      xfer.Bind(n, n2); 
    }
  }

  // make sure all nodes have equivalents
  if (nb != ni)
  {
    jprintf(">>> Only migrated %d of %d nodes in jhcDeclMem::Remember!\n", nb, ni);
    return -1;
  }
  if ((n2 = xfer.LookUp(fact)) != NULL)
    jprintf("-: ENCODED %s as memory %s\n", fact->Nick(), n2->Nick());
  return 1;
}


//= Find or make equivalent long term memory node for object or predicate.
// will automatically link equivalent LTM predicate arguments using bindings 

jhcNetNode *jhcDeclMem::ltm_node (const jhcNetNode *n, const jhcBindings& xfer)
{
  jhcGraphlet desc;
  jhcNetNode *gnd, *a2, *n2 = NULL;
  const char *role;
  int i, j, np, na = n->NumArgs();

  // see if similar LTM predication already exists
  if (na > 0)
  {
    // starting point = LTM equivalent for any argument (always exists)
    gnd = xfer.LookUp(n->ArgSurf(0));
    role = n->Slot(0);
    np = gnd->NumProps();
    for (i = 0; i < np; i++)      
    {
      // property might match if it has same slot to ground node 
      n2 = gnd->Prop(i);                      
      if (gnd->RoleMatch(i, role))
        if ((n2->NumArgs() == na) && lex_equiv(n, n2) && (n2->Done() == n->Done()))
        {
          // check if all arguments point to same nodes
          for (j = 0; j < na; j++) 
            if (!n2->HasVal(n->Slot(j), xfer.LookUp(n->Arg(j))))
              break;
          if (j < na)
            continue;

          // if contradiction then change "is" to "was" and skip
          if (n2->Neg() == n->Neg())
            break;                                      
          n2->SetDone(1);             
        }
    }
  
    // return any exact match found (after updating LTM belief)
    if (i < np)                        
    {
      jprintf(1, enc, "    %s = %s [%d]\n", n->Nick(), n2->Nick(), na);
      n2->SetBelief(n->Belief());     
      return n2;
    }
  }
    
  // instead create new LTM node and connect to all arguments
  n2 = CloneNode(*n);
  if (n->LexMatch("you"))
    SetLex(n2, "");
  jprintf(1, enc, "    %s -> %s (new)\n", n->Nick(), n2->Nick());
  for (i = 0; i < na; i++)
  {
    a2 = xfer.LookUp(n->ArgSurf(i));
    role = n->Slot(i);
    n2->AddArg(role, a2);
    jprintf(2, enc, "      -%s-> %s\n", role, a2->Nick());
  }
  return n2;
}


//= Add discriminative pattern for given node to overall description.
// returns amount to increment full match score for this augmentation

int jhcDeclMem::add_node (jhcGraphlet& desc, jhcNetNode *n, const jhcNetNode *src, int rels) const
{
  jhcGraphlet desc2;
  int sc;

  if (!n->ObjNode())
    return add_pred(desc, n, src, rels);
  if (desc.InDesc(n))
    return 0;
  sc = elab_obj(desc2, n, src, rels);
  jprintf(3, enc, "  __done ELAB_OBJ: %s (%s)\n\n", n->Nick(), n->LexStr());
  desc.Append(desc2);
  return sc;
}


//= Add given predicate, its modifiers, and its elaborated arguments to description.
// returns amount match score is increment by augmented description

int jhcDeclMem::add_pred (jhcGraphlet& desc, jhcNetNode *pred, const jhcNetNode *src, int rels) const
{
  jhcGraphlet desc2;
  jhcNetNode *p, *a;
  int i, np = pred->NumProps(), na = pred->NumArgs(), sc = 1;

  // add predicate and figure out matching value
  if (desc.InDesc(pred))
    return 0;
  desc.AddItem(pred);
  if (pred->Val("name") == src)
    sc += 2;
  else if (pred->Val("ako") == src)
    sc += 1;

  // possibly announce entry
  if (src == NULL)
    jprintf(3, enc, "      add_pred: %s (%s)\n", pred->Nick(), pred->LexStr());
  else
    jprintf(3, enc, "      add_pred: %s (%s) from %s\n", pred->Nick(), pred->LexStr(), src->Nick());   

  // get all adverbs (stack is generally only two deep)
  if (np > 0)
    jprintf(3, enc, "        modifiers: %s (%s)\n", pred->Nick(), pred->LexStr());
  for (i = 0; i < np; i++)
    if (pred->RoleIn(i, "deg", "mod"))
      if ((p = pred->PropSurf(i)) != src)
        if (p->Visible() && atree->InList(p) && (p->NumArgs() == 1))
          sc += add_pred(desc, p, pred, 0);         
  if (np > 0)
    jprintf(3, enc, "        __done modifiers: %s (%s)\n", pred->Nick(), pred->LexStr());
  
  // expand compound predicates and elaborate root arguments (possibly with rels)
  if ((na > 1) || ((na > 0) && (src == NULL))) 
    jprintf(3, enc, "        arguments: %s (%s)\n", pred->Nick(), pred->LexStr());
  for (i = 0; i < na; i++)
    if ((a = pred->ArgSurf(i)) != src)
      add_node(desc, a, pred, rels);
  if ((na > 1) || ((na > 0) && (src == NULL))) 
    jprintf(3, enc, "        __done arguments: %s (%s)\n", pred->Nick(), pred->LexStr());
  return sc;
}


//= Build good local description of object using various unary WMEM properties.
// adds object, discriminative properties, and possibly relations with elaborated arguments 
// returns max score for matching local object description 

int jhcDeclMem::elab_obj (jhcGraphlet& desc2, jhcNetNode *obj, const jhcNetNode *src, int rels) const
{
  int done, sc = 0, ties = -1;

  // initialize description new standalone description
  jprintf(2, enc, "  ELAB_OBJ: %s %s\n", obj->Nick(), ((rels > 0) ? "+ rels" : ""));
  desc2.Init(obj);

  // add name (if any) and kind (if any)
  if (obj_prop(sc, ties, desc2, obj, src, "name") > 0)                 
    return sc;
  if (obj_prop(sc, ties, desc2, obj, src, "ako") > 0)                  
    return sc;

  // add more unary properties until unique in LTM
  while ((done = obj_prop(sc, ties, desc2, obj, src, "")) >= 0)     
    if (done >= 1)
      return sc;

  // possibly add relations if not unique yet
  if (rels > 0)
    while (obj_prop(sc, ties, desc2, obj, src, NULL) == 0);
  return sc;
}


//= Add to description the most unique property associated with the object.
// can constrain to predicates where object has given role, or allow relations also (role = NULL)
// returns 1 if sufficiently unique, 0 if something added, negative if nothing to add

int jhcDeclMem::obj_prop (int& sc, int& t0, jhcGraphlet& desc, const jhcNetNode *obj, const jhcNetNode *src, const char *role) const
{
  jhcGraphlet extra;
  jhcNetNode *p, *win = NULL;
  int any = (((role == NULL) || (*role == '\0')) ? 1 : 0), sc0 = sc;
  int i, info, t, ni = desc.NumItems(), np = obj->NumProps(), bump = 0, best = -1;

  // look for a property where the object has the given role (if any)
  jprintf(3, enc, "    obj_prop: %s <-%s-\n", obj->Nick(), ((role == NULL) ? "rel" : ((*role == '\0') ? "any" : role)));
  for (i = 0; i < np; i++)
    if ((any > 0) || obj->RoleMatch(i, role))
    {
      // make sure this is a valid new assertion in working memory with correct arity
      p = obj->PropSurf(i);
      if (desc.InDesc(p) || !p->Visible() || !atree->InList(p) || (p->Neg() > 0) || (p->Belief() < bth) || (p == src))
        continue;
      if (((role != NULL) && (p->NumArgs() > 1)) || ((role == NULL) && (p->NumArgs() < 2)))
        continue;

      // test augmented description with predicate mods and elaborated args (no rels ever)
      info = add_pred(desc, p, obj, 0);
      t = num_tied(desc);

      // see if more selective than previous winner
      if ((best < 0) || (t < best))
      {
        desc.CutTail(extra, ni);       // save augmentation
        bump = info;
        best = t;
        win = p;                       // for message only
      }
      else
        desc.TrimTo(ni);               // discard
    }

  // if more selective then update description with augmentation
  if ((best < 0) || ((sc0 >= rth) && (best >= t0)))
    return -1;
  desc.Append(extra);
  sc += bump;
  t0 = best;
  jprintf(2, enc, "      --> prop %s (%s) added: sc = %d, ties = %d\n", win->Nick(), win->LexStr(), sc, best);
  return(((best == 0) && (sc >= rth)) ? 1 : 0);
}


//= Determine number of long term memory nodes that completely match the augemented graphlet.
// adds modifiers to starter predicate and elaborates arguments as needed
// returns full match score plus part of description to be added

int jhcDeclMem::num_tied (const jhcGraphlet& desc) const
{
  const jhcNetNode *focus = desc.Main(), *mate = NULL;
  int bin = ((focus->LexMatch("you")) ? 0 : focus->Code()), ties = 0;

  while ((mate = NextNode(mate, bin)) != NULL)
    if (equiv_nodes(focus, mate, NULL))
      if (equiv_props(focus, mate, &desc))
        ties++;
  return ties;
}


//= Determine if nodes represent similar predicates (largely lexical term).
// any arguments other than "obj" must also have this basic equivalence

bool jhcDeclMem::equiv_nodes (const jhcNetNode *focus, const jhcNetNode *mate, const jhcNetNode *obj) const
{
  const jhcNetNode *a;
  const char *slot;
  int i, j, na = focus->NumArgs(), na2 = mate->NumArgs();

  // check for same basic predicate structure then same lexical term
  if ((focus->Neg() != mate->Neg()) || (focus->Done() != mate->Done()) || (focus->Arity() != mate->Arity()))
    return false;
  if (!lex_equiv(focus, mate))
    return false;

  // make sure that arguments (if any) are roughly compatible
  for (i = 0; i < na; i++)
    if ((a = focus->ArgSurf(i)) != obj)
    {
      // find some argument of mate with same slot that matches
      slot = focus->Slot(i);
      for (j = 0; j < na2; j++)
        if (mate->SlotMatch(j, slot))
          if (equiv_nodes(a, mate->ArgSurf(j), NULL))
            break;
      if (j >= na2)
        return false;                  // possible matches exhausted
    }
  return true;
}


//= Determine if all focus properties listed in the description have equivalents for the prospective mate.
// requires any modifier properties to be the same (provided that they are in description)

bool jhcDeclMem::equiv_props (const jhcNetNode *focus, const jhcNetNode *mate, const jhcGraphlet *desc) const
{
  const jhcNetNode *p;
  const char *role;
  int i, j, np = focus->NumProps(), np2 = mate->NumProps();

  // examine all focal properties (unary or n-ary) listed in description
  for (i = 0; i < np; i++)
  {
    p = focus->PropSurf(i);
    if ((desc == NULL) || desc->InList(p))
    {
      // look for equivalent property associated with mate 
      role = focus->Role(i);
      for (j = 0; j < np2; j++)
        if (mate->RoleMatch(j, role))
          if (equiv_nodes(p, mate->PropSurf(j), focus))
            if (equiv_props(p, mate->PropSurf(j), desc))
              break;
      if (j >= np2)
        return false;                  // possible matches exhausted
    }
  }
  return true;
}


///////////////////////////////////////////////////////////////////////////
//                             Familiarity                               //
///////////////////////////////////////////////////////////////////////////

//= Attempt to tether extant object nodes in main memory to long-term items.
// returns number of items (re-)tethered.

int jhcDeclMem::DejaVu () 
{
  jhcNetNode *mate, *n = NULL;
  int cnt = 0;

jtimer(15, "DejaVu");
  // set up for surface node scan
  if (atree == NULL)
    return 0;
  bth = atree->MinBlf();
  atree->SetMode(0);
  jprintf(1, ret, "\nDejaVu");

  // go through all object nodes (no lex -> bin = 0)
  while ((n = atree->NextNode(n, 0)) != NULL)     
    if ((n->Belief() > bth) && n->ObjNode())     
      if ((mate = Recognize(n, bth)) != NULL)
        cnt += tether(n, mate);

  // handle "me" and "you" separately (not bin 0)
  n = atree->Human();
  if ((mate = Recognize(n, bth)) != NULL)
    cnt += tether(n, mate); 
  n = atree->Robot();
  if ((mate = Recognize(n, bth)) != NULL)
    cnt += tether(n, mate);
  jprintf(1, ret, "\n");
jtimer_x(15);
  return cnt;
}


//= Add or swap tethering of working memory node.
// returns 1 always for convenience

int jhcDeclMem::tether (jhcNetNode *focus, jhcNetNode *win)
{
  jprintf(":- RECOGNIZE %s as memory %s", focus->Nick(), win->Nick());
  if (focus->Moored() && (focus->Deep() != win))
    jprintf(" (instead)");
  jprintf("\n");
  focus->MoorTo(win);
  Refresh(win);                                  // ensure first next time
  atree->NoteSolo(focus);
  return 1;
}


//= Find the best match to WMEM focus node in long-term memory facts.
// returns best new mate for node, NULL for no change
// Note: this is a tree partial matcher vs a graph full matcher (cf. jhcSituation)

jhcNetNode *jhcDeclMem::Recognize (jhcNetNode *focus, double qth) const
{
  jhcNetNode *win = NULL, *mate = NULL;
  int sum, bin = ((focus->LexMatch("you")) ? 0 : focus->Code()), hi = 0, tie = 0;

  // find potential pairing with highest score 
  jprintf(1, ret, "\n  Recognize %s ---------------------------------\n", focus->Nick());
  while ((mate = NextNode(mate, bin)) != NULL)
    if (!mate->Buoyed() || (mate->Buoy() == focus))        // not allowed to steal
    {
      // find score of mate
      if ((sum = score_nodes(focus, mate)) < 0)
        continue;
      sum += score_unary(focus, mate, qth);
      sum += score_rels(focus, mate, qth);
      if (sum > 0)
        jprintf(2, ret, "  %2d = score for %s vs %s\n", sum, focus->Nick(), mate->Nick());
      else
        jprintf(2, ret, "     . score for %s vs %s\n", focus->Nick(), mate->Nick());

      // remember best pairing
      if (sum > hi)                                        // prefer first found
      {
        win = mate;
        hi = sum;
        tie = 0;
      }
      else if ((hi > 0) && (sum == hi))
      {
        if (mate == focus->Deep())                         // prefer no change
          win = mate;
        tie++;
      }
    }

  // check if match is sufficiently good 
  jprintf(1, ret, "  ==> hi = %d (%s), tie = %d\n", hi, ((win != NULL) ? win->Nick() : "none"), tie);
  if ((hi < rth) || (tie > alts) || (win == focus->Deep()))
    return NULL;
  return win;
}


//= See if basic properties of two nodes are compatible.
// returns negative for mis-match, initial score otherwise

int jhcDeclMem::score_nodes (const jhcNetNode *focus, const jhcNetNode *mate) const
{
  if (!focus->SameSlots(mate) || (focus->Neg() != mate->Neg()) || (focus->Done() != mate->Done()))
    return -1;
  if (focus->LexMatch("you") && (mate->Lex() == NULL))     // part of lex_equiv
    return 0;
  if (!focus->LexSame(mate))                               // other part of lex_equiv
    return -1;
  if (mate->LexMatch("me"))
    return 3;                                              // like a name
  return 0;
}


//= Count the number of unary properties (and modifiers) a pairing has in common.
// assumes mate is in DMEM and focus is in WMEM (can use halo but not LTM-dependent) 

int jhcDeclMem::score_unary (const jhcNetNode *focus, const jhcNetNode *mate, double qth) const
{
  const jhcNetNode *p, *p2;
  const char *role;
  int np = focus->NumProps(), np2 = mate->NumProps();
  int i, j, sc0, sc, best, sum = 0;

  // look at each appropriate unary property of focus
  jprintf(3, ret, "  >properties of %s vs %s\n", focus->Nick(), mate->Nick());
  for (i = 0; i < np; i++)
  {
    p = focus->PropSurf(i);
    if ((p->Belief() >= qth) && (p->NumArgs() <= 1) && (p->ltm <= 0))   
    {
      // look for similar property associated with LTM mate
      best = -1;
      role = focus->Role(i);
      for (j = 0; j < np2; j++)
        if (mate->RoleMatch(j, role))
        {
          // check for lex or slots mismatch
          p2 = mate->PropSurf(j);
          if ((sc0 = score_nodes(p, p2)) < 0)    
            continue;

          // remember score of best match for this property
          jprintf(3, ret, "      >>> check modifiers of %s (%s)\n", p->Nick(), p->LexStr());
          sc = sc0 + score_unary(p, p2, qth);
          jprintf(3, ret, "    <<< modifiers of %s checked\n", p->Nick());
          best = __max(sc, best);
        }

      // some properties more important than others
      if (best < 0)
        continue;                                // no match found
      if (strcmp(role, "name") == 0)
        best += 3;
      else if (strcmp(role, "ako") == 0)
        best += 2;
      else
        best++;

      // combine multiple properties
      jprintf(3, ret, "      %d = best match of property %s (%s)\n", best, p->Nick(), p->LexStr());
      sum += best;                               
    }
  }
  jprintf(3, ret, "    --> %d property sum for %s\n", sum, focus->Nick());
  return sum;
}


//= See how many relations a pairing has in common including the match score for arguments.
// NOTE: does not respect relation convergence/divergence since sharing of arguments is ignored 

int jhcDeclMem::score_rels (const jhcNetNode *focus, const jhcNetNode *mate, double qth) const
{
  const jhcNetNode *p, *p2;
  const char *role;
  int np = focus->NumProps(), np2 = mate->NumProps();
  int i, j, sc0, sc2, sc, best, sum = 0;

  // look at each appropriate n-ary property of focus
  jprintf(3, ret, "   + rels of %s vs %s\n", focus->Nick(), mate->Nick());
  for (i = 0; i < np; i++)
  {
    p = focus->PropSurf(i);
    if ((p->Belief() >= qth) && (p->NumArgs() > 1) && (p->ltm <= 0) &&
        (p->AnySlot("alt", "loc", "src", "wrt") || p->LexIn("have")))
    {
      // look for similar relation involving LTM mate
      best = -1;
      role = focus->Role(i);
      for (j = 0; j < np2; j++)
        if (mate->RoleMatch(j, role))
        {
          // determine basic compatibility (lex and slots)
          p2 = mate->PropSurf(j);
          if ((sc0 = score_nodes(p, p2)) < 0)             
            continue;

          // check for bad or vague arguments
          jprintf(3, ret, "      ___ check arguments of %s (%s)\n", p->Nick(), p->LexStr());
          if ((sc2 = score_args(p, p2, focus, qth)) < 0)  
          {
            jprintf(3, ret, "    ___ barf on args of %s !\n", p->Nick());
            continue;
          }
          jprintf(3, ret, "    ___ arguments of %s checked\n", p->Nick());

          // check any modifiers
          jprintf(3, ret, "      >>> check modifiers of %s (%s)\n", p->Nick(), p->LexStr());
          sc = sc0 + sc2 + score_unary(p, p2, qth);      
          jprintf(3, ret, "    <<< modifiers of %s checked\n", p->Nick());

          // save score of best matched mate relation
          best = __max(sc, best);
        }

      // some relations more important than others in combination
      if (best < 0)
        continue;                                // no match found
      if (strcmp(role, "ako") == 0)
        best += 2;
      else
        best++;
      jprintf(3, ret, "      %d = best match of rel %s (%s)\n", best, p->Nick(), p->LexStr());
      sum += best;                               
    }
  }
  jprintf(3, ret, "    --> %d rel sum for %s\n", sum, focus->Nick());
  return sum;
}


//= See if relation arguments are compatible and well enough described.
// returns negative if one or more are bad, else sum of goodnesses

int jhcDeclMem::score_args (const jhcNetNode *focus, const jhcNetNode *mate, const jhcNetNode *obj, double qth) const
{
  const jhcNetNode *a, *a2;
  int i, sc0, sc, na = focus->NumArgs(), sum = 0;

  // look at each argument of focus
  for (i = 0; i < na; i++)
    if ((a = focus->ArgSurf(i)) != obj)
    {
      // find equivalent argument (same slot) for mate
      if ((a2 = mate->Val(focus->Slot(i))) == NULL)
        return -3;
      if ((sc0 = score_nodes(a, a2)) < 0)
        return -2;

      // determine if description is specific enough
      jprintf(3, ret, "      >>> ARG %s: check properties\n", a->Nick());
      sc = sc0 + score_unary(a, a2, qth);
      jprintf(3, ret, "    <<< ARG %s: properties checked\n", a->Nick()); 
      jprintf(3, ret, "      %d = ARG match for %s\n", sc, a->Nick());

      // add specificity scores for all arguments
      if (sc < ath)
        return -1;
      sum += sc;
    }
  jprintf(3, ret, "      ==> %d ARG sum for rel %s (%s)\n", sum, focus->Nick(), focus->LexStr());
  return sum;
}


///////////////////////////////////////////////////////////////////////////
//                               Spotlight                               //
///////////////////////////////////////////////////////////////////////////

//= Make a halo node for all non-surface properties of WMEM moored nodes.
// also make halo nodes for relations and the properties of their arguments
// essentially adds all facts that are "almost" evident about recognized objects

void jhcDeclMem::GhostFacts () const
{
  jhcNetNode *n = NULL;

  if (atree == NULL)
    return;
  atree->SetMode(0);
  while ((n = atree->NextNode(n)) != NULL)
    if (n->Moored())
      buoy_preds(n->Moor(), n->top, 1);
  atree->Border();                               // end of nearly factual nodes
}


//= Make a halo node for all non-surface properties of this node and recurse.

void jhcDeclMem::buoy_preds (jhcNetNode *n, int tval, int rels) const
{
  jhcNetNode *p, *a, *h = n->Buoy();
  int i, j, na, np = n->NumProps();

  // add halo equivalent of base node if predication (not object)
  if (!n->ObjNode())
  {
    if (h == NULL)
    {
      h = atree->CloneHalo(*n);
      h->ltm = 1;                                          // mark LTM-dependence
      h->MoorTo(n);       
    }                                   
    if ((tval > 0) && h->Halo())
      if ((h->top <= 0) || (tval < h->top))                // keep earliest NOTE
        h->top = tval;
  }

  // scan through all unary and n-ary properties of base node
  for (i = 0; i < np; i++)
  {
    // add if property and maybe if relation
    p = n->Prop(i);                                        // not surf
    if (p->Home(this) && ((rels > 0) || (p->NumArgs() <= 1)))  
      buoy_preds(p, tval, 0);                              // include modifiers       

    // if relation added then add all arguments 
    if (rels <= 0)
      continue;
    na = p->NumArgs();
    for (j = 0; j < na; j++)
    {
      a = p->Arg(j);                                       // not surf
      if (a->Home(this) && (a != n))
        buoy_preds(a, tval, 0);                            // include unary properties
    }
  }
}


///////////////////////////////////////////////////////////////////////////
//                            File Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Read a list of long term facts from a file.
// appends to existing facts unless add <= 0
// level: 0 = kernel, 1 = extras, 2 = previous accumulation
// for proper level-based saving must load in order starting with lowest level
// typically give base file name like "KB/kb_072721_1038", fcn adds ".facts"
// assumes most important nodes in each hash bin are listed first 
// returns number of facts read, 0 or negative for problem

int jhcDeclMem::LoadFacts (const char *base, int add, int rpt, int level)
{
  char hdr[80], full[200];
  const char *tail, *fname = base;
  FILE *in;
  int i, n, nt = 100;

  // possibly clear old stuff (even if bad file) 
  if (add <= 0)
  {
    PurgeAll();
    for (i = 0; i < 4; i++)
      first[i] = 0;
  }

  // try to open file
  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.facts", base);
    fname = full;
  }
  if (fopen_s(&in, fname, "r") != 0)
  {
    if (rpt < 3)
      jprintf("  >>> Could not read fact file: %s !\n", fname);
    return -1;
  }

  // see if there is a hint about number of translations needed
  fgets(hdr, 80, in);
  if ((tail = strstr(hdr, "Nodes = ")) != NULL)
    sscanf_s(tail + 8, "%d", &nt);
  fclose(in);

  // try reading facts from file and record starting ID for next level(s)
  n = jhcNodePool::Load(fname, add, nt);
  if (level >= 0) 
    for (i = level; i < 3; i++)
      first[i + 1] = abs(LastLabel()) + 1;     

  // possibly announce result
  if (n > 0)
    jprintf(2, rpt, "  %3d long-term facts  from: %s\n", n, fname);
  else
    jprintf(2, rpt, "   -- long-term facts  from: %s\n", fname);
  return n;
}


//= Save all current facts at or above some level to a file.
// typically give base file name like "KB/kb_072721_1038", fcn adds ".facts"
// level: 0 = kernel, 1 = extras, 2 = previous accumulation, 3 = newly added
// saves most recent/important nodes in each two letter hash bin first 
// returns number of facts saved, zero or negative for problem

int jhcDeclMem::SaveFacts (const char *base, int level) const
{
  char full[200];
  const char *fname = base;
  FILE *out;
  int cnt, lvl = __max(0, __min(level, 3));

  if (strchr(base, '.') == NULL)
  {
    sprintf_s(full, "%s.facts", base);
    fname = full;
  }
  if (fopen_s(&out, fname, "w") != 0)
  {
    jprintf("  >>> Could not write fact file: %s !\n", fname);
    return -1;
  }
  if (level >= 2)
  {
    fprintf(out, "// newly learned facts not in KB0 or KB2\n");
    fprintf(out, "// ======================================\n");
  }
  fprintf(out, "// Nodes = %d max", NodeCnt());
  cnt = save_bins(out, -1, first[lvl]);
  fclose(out);
  return cnt;
}

