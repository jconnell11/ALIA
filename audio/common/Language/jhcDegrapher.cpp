// jhcDegrapher.cpp : generates natural language string from network
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include <stdarg.h>

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
  mf = NULL;
  wmem = NULL;
  *phrase = '\0';
  dbg = 0;
//dbg = 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Formatted Output                           //
///////////////////////////////////////////////////////////////////////////

//= Get descriptive string to refer to some node.
// assumes listener's knowledge of situtation identical to robot's

const char *jhcDegrapher::node_ref (char *txt, int ssz, const jhcNetNode *n, int nom, const char *avoid) const
{
  // sanity check
  if ((txt == NULL) || (ssz <= 0) || (n == NULL) || (wmem == NULL))
    return NULL;
  jprintf(1, dbg, "node_ref: %s (nom %d, avoid %s)\n", n->Nick(), nom, avoid);

  // dispatch based on type
  *txt = '\0';  
  if (n->ObjNode() && (n->Done() == 0))
    return obj_ref(txt, ssz, n, nom, avoid);
  return pred_ref(txt, ssz, n, 0);
}


///////////////////////////////////////////////////////////////////////////
//                          Predicate Reference                          //
///////////////////////////////////////////////////////////////////////////

//= Describe a predication like a property or verb frame.
// returns phrase with head word and any required arguments (disambiguated)

const char *jhcDegrapher::pred_ref (char *txt, int ssz, const jhcNetNode *n, int inf) const
{
  char first[500] = "", second[500] = "";
  const char *slot;
  const jhcNetNode *val;
  int i, na = n->NumArgs(), direct = 0, rcnt = 0;

  // sanity check
  if ((txt == NULL) || (ssz <= 0) || (n == NULL))
    return NULL;
  jprintf(1, dbg, "pred_ref: %s\n", n->Nick());

  // get basic predicate or verb (possibly compound)
  // automatically adds subject or target of predication if needed
  full_pred(txt, ssz, n, inf);
  if (na <= 0)
    return txt;

  // special embedded infinitive ("know how to")
  for (i = 0; i < na; i++)
    if (strcmp(n->Slot(i), "how") == 0)
      return strcatf_s(txt, ssz, " how %s", pred_ref(first, 500, n->Arg(i), 1));

  // get any special first verb argument (only one allowed)
  for (i = 0; (i < na) && (*first == '\0'); i++)
  {
    slot = n->Slot(i);
    val = n->Arg(i);
    if ((strcmp(slot, "ref") == 0) ||                      // was "wrt"
        (val->ObjNode() && (strcmp(slot, "dest") == 0)))   // indirect object
      node_ref(first, 500, val, 0, NULL);
  }

  // if no indirect object then make sure direct object is rendered
  for (i = 0; (i < na) && (*first == '\0'); i++)
    if (strcmp(n->Slot(i), "obj") == 0)
    {
      node_ref(first, 500, n->Arg(i), 0, NULL);
      direct = 1;
    }

  // get any second verb argument (only one allowed)
  for (i = 0; (i < na) && (*second == '\0'); i++)
  {
    slot = n->Slot(i);
    val = n->Arg(i);
    if (((direct <= 0) && (strcmp(slot, "obj") == 0)) ||   // direct object (after indirect)
        (!val->ObjNode() && (strcmp(slot, "dest") == 0)))  // destination phrase
      node_ref(second, 500, val, 0, NULL);
    else if (strcmp(slot, "ref") == 0) 
      if (++rcnt == 2)                                     // "between" has 2 refs                   
        node_ref(second, 500, val, 0, NULL);    
  }

  // generated combined argument phrase at end
  if (*first != '\0')
    strcatf_s(txt, ssz, " %s", first);
  if (*second != '\0')
  {
    strcat_s(txt, ssz, ((rcnt >= 2) ? " and " : " "));     // "between 1 and 2"
    strcat_s(txt, ssz, second);
  }
  return txt;
}


//= Get predicate name with degree modifier and any conjoined descriptors.
// helps with vector-valued results like "black and white" by examining conjuctions
// assumes no other part of input string "txt" needs to be saved

const char *jhcDegrapher::full_pred (char *txt, int ssz, const jhcNetNode *n, int inf) const
{
  char ing[40];
  const jhcNetNode *mod, *part, *agt, *targ;
  int i, nc;

  // sanity check
  if ((txt == NULL) || (ssz <= 0) || (n == NULL) || (wmem == NULL))
    return NULL;
  jprintf(1, dbg, "  full_pred: %s\n", n->Nick());

  // operate on conjoined group in argument order (n might not be first)
  if ((mod = n->Fact("conj")) != NULL)
  {
    *txt = '\0';
    nc = mod->NumArgs();
    for (i = 0; i < nc; i++)
    { 
      // get term for this part of conjunction (e.g. "black")
      part = mod->Arg(i);
      strcat_s(txt, ssz, part->Lex());
     
      // add separator between terms (space, comma, or "and")
      if (i != (nc - 1))
        strcat_s(txt, ssz, ((nc > 2) ? ", " : " "));
      if (i == (nc - 2))
        strcat_s(txt, ssz, "and ");
    } 
  }
  else if (inf > 0)                              // infinitive
    sprintf_s(txt, ssz, "to %s", n->Lex());
  else if ((n->Val("obj") != NULL) || (n->Val("agt") != NULL) || 
           (n->Val("how") != NULL) || (n->Done() > 0))
  {
    // generate verb frame including performing agent
    if (((agt = n->Val("agt")) != NULL) && (agt != wmem->Robot()))
    {
      obj_ref(txt, ssz, agt, 1, NULL);
      if (n->Neg() > 0)
      {
        if (n->Done() > 0)  
          strcatf_s(txt, ssz, " did not %s", n->Lex());
        else
          strcatf_s(txt, ssz, " is not %s", mf->SurfWord(ing, n->Lex(), JTAG_VPROG));
      }
      else
        strcatf_s(txt, ssz, " is %s",  mf->SurfWord(ing, n->Lex(), JTAG_VPROG));
    }
    else if (n->Neg() > 0)
    {
      if (n->Done() > 0)
        sprintf_s(txt, ssz, "I couldn't %s", n->Lex());
      else if (n->LexMatch("know"))
        strcpy_s(txt, ssz, "I do not know");
      else
        sprintf_s(txt, ssz, "I am not %s", mf->SurfWord(ing, n->Lex(), JTAG_VPROG));
    }
    else
    {
      if (n->Done() > 0)
        sprintf_s(txt, ssz, "I %s", mf->SurfWord(ing, n->Lex(), JTAG_VPAST));
      else if (n->LexMatch("know"))
        strcpy_s(txt, ssz, "I know");
      else
        sprintf_s(txt, ssz, "I am %s", mf->SurfWord(ing, n->Lex(), JTAG_VPROG));
    }
  }
  else if (((targ = n->Val("hq"))  != NULL) || ((targ = n->Val("ako")) != NULL) || 
           ((targ = n->Val("loc")) != NULL))
  {
    // generate target description for predication avoiding predicate term
    obj_ref(txt, ssz, targ, 1, n->Lex());
    strcat_s(txt, ssz, " is ");
    if (n->Neg() > 0)
      strcat_s(txt, ssz, "not ");

    // prepend degree modifier (e.g. "very big")
    if ((mod = n->Fact("deg")) != NULL)
      strcatf_s(txt, ssz, "%s ", mod->Lex());           

    // add primary term
    if (n->Val("ako") != NULL)
      strcat_s(txt, ssz, "a ");
    strcat_s(txt, ssz, n->Lex());
  }
  else if (n->Lex() != NULL)
    strcpy_s(txt, ssz, n->Lex());                // e.g. "three" for cnt
  else
    strcpy_s(txt, ssz, "it");
  return txt;
}


///////////////////////////////////////////////////////////////////////////
//                            Object Reference                           //
///////////////////////////////////////////////////////////////////////////

//= Uniquely describe some object, adding adjectives if necessary.
// can optionally request nominative case for pronouns ("he" vs "him")
// a negative value for "nom" inhibits the use of any pronouns (except "me/I" and "you")
// can avoid a particular term to prevent saying "the red object is red"

const char *jhcDegrapher::obj_ref (char *txt, int ssz, const jhcNetNode *n, int nom, const char *avoid) const
{
  jhcNetRef nr;
  const char *ref;
  int i;

  // sanity check
  if ((txt == NULL) || (ssz <= 0) || (n == NULL) || (wmem == NULL))
    return NULL;
  jprintf(1, dbg, "obj_ref: %s (nom %d, avoid %s)\n", n->Nick(), nom, avoid);

  // see if part of a hypothetical description
  if (n->Hyp() && (n->Generation() > 0))     
    return hyp_ref(txt, ssz, n, avoid);

  // consider using pronoun if unambiguous
  if ((ref = pron_ref(txt, ssz, n, nom)) != NULL)
    return ref;

  // search memory using single names and types
  nr.MakeNode("obj");
  wmem->SetMode(2);
  if ((ref = name_ref(nr, n)) != NULL)
    return ref;
  if ((ref = add_kind(txt, ssz, nr, n, avoid)) != NULL)
    return ref;

  // keep trying to add adjectives to basic noun phrase
  for (i = 0; i < 3; i++)
    if ((ref = add_adj(txt, ssz, nr, n, avoid)) != NULL)
      break;  
  return txt;                // best description (may not be unique)
}


//= Try generating a pronoun reference for the given node.
// can optionally request nominative case ("he" vs "him")
// if nom < 0 then avoids everything except "me/I" and "you"

const char *jhcDegrapher::pron_ref (char *txt, int ssz, const jhcNetNode *n, int nom) const
{
  jhcNetNode *obj = NULL, *win = NULL;
  int best = 0;

  jprintf(1, dbg, "  pron_ref: %s (nom %d)\n", n->Nick(), nom);

  // see if referring to self or user
  if (n == wmem->Human())
    strcpy_s(txt, ssz, "you");
  else if (n == wmem->Robot())
    strcpy_s(txt, ssz, ((nom > 0) ? "I" : "me"));
  if (*txt != '\0')
  {
    jprintf(1, dbg, "    --> %s\n", txt);
    return txt;
  }
  
  // see if most recent thing mentioned
  if (nom < 0) 
    return NULL;                                           // not unique yet
  wmem->SetMode(0);
  while ((obj = wmem->Next(obj)) != NULL)
    if (obj->ObjNode() && !obj->Hyp())
      if (obj->LastRef() > best)
      {
        best = obj->LastRef();     
        win = obj;
      }
  if ((win != n) || (best <= 0))
    return NULL;                                           // not unique yet
  wmem->MarkRef(win);                                      // robot speech

  // alter pronoun based on gender (if any)
  if (chk_prop(n, "hq", "female", NULL))
    strcpy_s(txt, ssz, ((nom > 0) ? "she" : "her"));
  else if (chk_prop(n, "hq", "male", NULL) ||
           chk_prop(n, "ako", "person", NULL) ||           // "they" would need plural verb
           (n->Fact("name") != NULL))                      // has a name implies person
    strcpy_s(txt, ssz, ((nom > 0) ? "he" : "him"));
  else
    strcpy_s(txt, ssz, "it");
  jprintf(1, dbg, "    --> %s\n", txt);
  return txt;
}


//= Determine if node has a given property with high enough belief.
// quick semantic matcher (e.g. "hq" + "female"), does not consider modifiers of property
// if "desc" is non-NULL, found element can have belief = 0 but must be in description

bool jhcDegrapher::chk_prop (const jhcNetNode *n, const char *role, const char *label, 
                             const jhcGraphlet *desc) const
{
  const jhcNetNode *p;
  double th = ((desc == NULL) ? wmem->MinBlf() : 0.0);     // allow 0 for jhcNetRef
  int i;

  for (i = n->NumProps() - 1; i >= 0; i--)
    if ((p = n->PropMatch(i, role, th)) != NULL) 
      if (((desc == NULL) || desc->InDesc(p)) && p->LexMatch(label))
        return true;
  return false;  
}


//= See if any single name for node is unique or selective.
// assumes "nr" has single object node only, resets to this state at end
// returns reference word if successful, NULL otherwise

const char *jhcDegrapher::name_ref (jhcNetRef& nr, const jhcNetNode *n) const
{
  jhcNetNode *p, *obj = nr.Main();
  double th = wmem->MinBlf();
  int hok, i;

  jprintf(1, dbg, "  name_ref: %s\n", n->Nick());

  nr.RefMode(0);
  for (hok = 0; hok <= 1; hok++)                                          
    for (i = n->NumProps() - 1; i >= 0; i--)      
      if ((p = n->PropMatch(i, "name", th)) != NULL)
        if (p->Visible() && ((hok > 0) || !p->Halo()))
        {
          nr.AddProp(obj, "name", p->Lex());               // removed by NumMatch search
          if (nr.NumMatch(wmem, th, 1) <= 1)               
          {
            jprintf(1, dbg, "    --> %s\n", p->Lex());
            return p->Lex();                               // unique or most obvious 
          }
        }
  return NULL;                                             // not unique yet
}


//= See if any single kind for node make the selection unique.
// assumes "nr" has single object node only, retains most restrictive kind at end
// returns reference phrase if successful, NULL otherwise

const char *jhcDegrapher::add_kind (char *txt, int ssz, jhcNetRef& nr, const jhcNetNode *n, const char *avoid) const
{
  jhcNetNode *obj = nr.Main();
  const jhcNetNode *p;
  const char *kind = NULL;
  double th = wmem->MinBlf();
  int hok, i, hits, low;

  jprintf(1, dbg, "  add_kind: %s (avoid %s)\n", n->Nick(), avoid);

  // search through all AKO properties 
  for (hok = 0; hok <= 1; hok++)                                          
    for (i = n->NumProps() - 1; i >= 0; i--)
      if ((p = n->PropMatch(i, "ako", th)) != NULL)
        if (p->Visible() && !p->LexMatch(avoid) && ((hok > 0) || !p->Halo()))
        {
          // see if node with kind is unique or most recent match
          nr.AddProp(obj, "ako", p->Lex());                // removed by NumMatch search      
          hits = nr.NumMatch(wmem, th, 1);
          if (hits <= 1)
          {
            sprintf_s(txt, ssz, "the %s", p->Lex()); 
            jprintf(1, dbg, "    --> %s\n", txt);
            return txt;                                    // unique or most obvious
          }
       
          // otherwise remember most restrictive category
          if ((kind == NULL) || (hits < low))
          {
            kind = p->Lex();
            low = hits;
          }
        }

  // keep best category in description and phrase
  if (kind == NULL)
    strcpy_s(txt, ssz, "the thing");
  else
  {
    sprintf_s(txt, ssz, "the %s", kind);
    nr.AddProp(obj, "ako", kind);
  }
  jprintf(1, dbg, "    ++ %s\n", txt);
  return NULL;                                             // not unique yet
}


//= Add then next most selective adjective to description and check if unique.
// assumes "nr" has object node and possibly AKO and some earlier HQ's
// returns reference phrase, blank if out of choices, NULL if still ambiguous

const char *jhcDegrapher::add_adj (char *txt, int ssz, jhcNetRef& nr, const jhcNetNode *n, const char *avoid) const
{
  char tail[500];
  jhcNetNode *obj = nr.Main();
  const jhcNetNode *p;
  const char *qual = NULL;
  double th = wmem->MinBlf();
  int hok, i, hits, low;

  jprintf(1, dbg, "  add_adj: %s (avoid %s)\n", n->Nick(), avoid);

  // search through all HQ properties 
  for (hok = 0; hok <= 1; hok++)                                          
    for (i = n->NumProps() - 1; i >= 0; i--)                                 
      if ((p = n->PropMatch(i, "hq", th)) != NULL)
        if (p->Visible() && !p->LexMatch(avoid) && ((hok > 0) || !p->Halo()))
          if (!chk_prop(obj, "hq", p->Lex(), nr.Pattern()))    // avoid duplicates
          {
            nr.AddProp(obj, "hq", p->Lex());                   // removed by NumMatch search      
            hits = nr.NumMatch(wmem, th, 1);                   
            if (hits <= 1)
            {
              strcpy_s(tail, txt + 4);                         // strip "the"
              sprintf_s(txt, ssz, "the %s %s", p->Lex(), tail);
              jprintf(1, dbg, "    --> %s\n", txt);
              return txt;                                      // unique or most obvious
            }

            // otherwise remember most restrictive property
            if ((qual == NULL) || (hits < low))
            {
              qual = p->Lex();
              low = hits;
            }
          }

  // otherwise keep most selective one (if any) in description and phrase 
  if (qual == NULL)
    return txt;                                                // signal done                    
  strcpy_s(tail, txt + 4);                                     // strip "the"
  sprintf_s(txt, ssz, "the %s %s", qual, tail); 
  nr.AddProp(obj, "hq", qual);
  jprintf(1, dbg, "    ++ %s\n", txt);
  return NULL;                                                 // not unique yet
}


//= Describe hypothetical node using all properties in description.

const char *jhcDegrapher::hyp_ref (char *txt, int ssz, const jhcNetNode *n, const char *avoid) const
{
  char ref[40];
  const jhcNetNode *p = NULL;
  int i, np = n->NumProps();

  jprintf(1, dbg, "hyp_ref: %s (avoid %s)\n", n->Nick(), avoid);

  // if object has a name then ignore all other attributes
  for (i = np - 1; i >= 0; i--)      
    if ((p = n->PropMatch(i, "name")) != NULL)
      if (p->Visible() && p->Hyp() && !p->Halo() && !p->LexMatch(avoid))
      {
        strcpy_s(txt, ssz, p->Lex());
        return txt;
      }

  // start with determiner then add all adjectives
  strcpy_s(txt, ssz, "a ");
  for (i = np - 1; i >= 0; i--)      
    if ((p = n->PropMatch(i, "hq")) != NULL)
      if (p->Visible() && p->Hyp() && !p->Halo() && !p->LexMatch(avoid))
        strcatf_s(txt, ssz, "%s ", p->Lex());

  // add single base noun (if any)
  for (i = np - 1; i >= 0; i--)      
    if ((p = n->PropMatch(i, "ako")) != NULL)
      if (p->Visible() && p->Hyp() && !p->Halo() && !p->LexMatch(avoid))
        break;
  if (i >= 0)
    strcat_s(txt, ssz, p->Lex());
  else
    strcat_s(txt, ssz, "thing");

  // add any location relations afterward
  for (i = np - 1; i >= 0; i--)
    if ((p = n->PropMatch(i, "loc")) != NULL)
      if (p->Visible() && p->Hyp() && !p->Halo() && !p->LexMatch(avoid))
      {
        // add main preposition and one or two reference objects (e.g. "between")
        strcatf_s(txt, ssz, " %s %s", p->Lex(), node_ref(ref, 40, p->Val("ref"), 0, NULL));
        if (p->NumArgs() > 2)
          strcatf_s(txt, ssz, " and %s", node_ref(ref, 40, p->Val("ref", 1), 0, NULL));
      }
  return txt;
}


///////////////////////////////////////////////////////////////////////////
//                               Utilities                               //
///////////////////////////////////////////////////////////////////////////

//= Added a formatted printout to the end of some string with a specified size. 

char *jhcDegrapher::strcatf_s (char *txt, int ssz, const char *fmt, ...) const
{
  va_list args;
  int n;

  if (fmt != NULL)
  {
    n = (int) strlen(txt);
    va_start(args, fmt); 
    vsprintf_s(txt + n, ssz - n, fmt, args);
  }
  return txt;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Generate an output string based on a particular graphlet.

const char *jhcDegrapher::Generate (const jhcGraphlet& graph, jhcWorkMem& mem)
{
  jhcNetNode *n = NULL;
  int i, d, ni = graph.NumItems(), best = 0;

  // sanity check then bind arguments
  if (ni <= 0)
    return phrase;
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
  return form_sent(phrase, 500, n, 1);
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
//  strcpy_s(txt, ssz, n->Word());                 // needs to retrieve "ref" prop!!!
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
//  strcat_s(txt, ssz, n->Word());                 // needs to retrieve "ref" prop!!!
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
/*
  // check for proper name
  if (n->NumWords() > 0)
  {
    strcpy_s(txt, ssz, n->Word());                // needs to retrieve "ref" prop!!!
jprintf("np -> %s\n", txt);
    return txt;
  }
*/
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
  const jhcNetNode *owner = NULL;
  int i, np = n->NumProps();

  // see if any AKO properties are relative
  for (i = 0; i < np; i++)
    if (n->RoleMatch(i, "ako"))
      if ((owner = (n->Prop(i))->Val("wrt")) != NULL)
        break;

  // add proper possessive form for owner
  if (owner == NULL)
    *txt = '\0';
  else if (owner->LexMatch("me"))
    add_sp(txt, ssz, "my");
  else if (owner->LexMatch("you"))
    add_sp(txt, ssz, "your");
//  else if ((wd = owner->Word()) != NULL)     // proper name - need to search "ref" props!!!
//    add_sp(txt, ssz, wd, "'s");
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
      add_sp(txt, ssz, (n->Prop(i))->Lex());
  strcat_s(txt, ssz, n->Lex());
jprintf("adj -> %s\n", txt);
  return txt;
}


//= Supply proper base word respecting node tags.

const char *jhcDegrapher::form_noun (char *txt, int ssz, const jhcNetNode *n, UL32 tags) const
{
  strcpy_s(txt, ssz, n->Lex());
  if ((tags & JTAG_NPL) != 0)
    strcat_s(txt, ssz, "s");
  return txt;
}


