// jhcDegrapher.cpp : generates natural language string from network
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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
  noisy = 0;                 // for subroutine calls 
  dbg = 0;                   // for num_match steps
//noisy = 2;                 
//dbg = 3;                   
}


///////////////////////////////////////////////////////////////////////////
//                            Formatted Output                           //
///////////////////////////////////////////////////////////////////////////

//= Get string for name associated with object.
// does not mark user as an antecedent for a pronoun (would always be "you")

const char *jhcDegrapher::NameRef (jhcNetNode *n) const
{
  if ((n == NULL) || (wmem == NULL))
    return NULL;
  if ((n != wmem->Human()) && (n != wmem->Robot()))
    n->MarkConvo();                                        // robot speech
  return n->Name(0, wmem->MinBlf());
}


//= Get descriptive string to refer to some node.
// assumes listener's knowledge of situtation identical to robot's

const char *jhcDegrapher::node_ref (char *txt, int ssz, jhcNetNode *n, int nom, const char *avoid) 
{
  // sanity check
  if ((txt == NULL) || (ssz <= 0) || (n == NULL) || (wmem == NULL))
    return NULL;
  jprintf(1, noisy, "node_ref: %s (nom %d, avoid %s)\n", n->Nick(), nom, avoid);

  // simplest case where node is a string (e.g. unknown word)
  if (n->String())
  {
    sprintf_s(txt, ssz, "'%s'", n->Literal());
    return txt;
  }

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

const char *jhcDegrapher::pred_ref (char *txt, int ssz, jhcNetNode *n, int inf) 
{
  char first[500] = "", second[500] = "";
  const char *slot;
  jhcNetNode *val;
  int i, na = n->NumArgs(), direct = 0, rcnt = 0;

  // sanity check
  if ((txt == NULL) || (ssz <= 0) || (n == NULL))
    return NULL;
  jprintf(1, noisy, "pred_ref: %s\n", n->Nick());

  // get basic predicate or verb (possibly compound)
  // automatically adds subject or target of predication if needed
  full_pred(txt, ssz, n, inf);
  if (na <= 0)
  {
    n->MarkConvo();                                        // robot speech
    return txt;
  }

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
    else if ((strcmp(slot, "ref") == 0) || (strcmp(slot, "ref2") == 0))
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
  n->MarkConvo();                                          // robot speech
  return txt;
}


//= Get predicate name with degree modifier and any conjoined descriptors.
// helps with vector-valued results like "black and white" by examining conjuctions
// assumes no other part of input string "txt" needs to be saved

const char *jhcDegrapher::full_pred (char *txt, int ssz, const jhcNetNode *n, int inf) 
{
  const jhcNetNode *multi;
  jhcNetNode *targ;

  // sanity check
  if ((txt == NULL) || (ssz <= 0) || (n == NULL) || (wmem == NULL))
    return NULL;
  jprintf(1, noisy, "  full_pred: %s\n", n->Nick());

  // dispatch for various forms
  if ((multi = n->Fact("conj")) != NULL)
    list_conj(txt, ssz, multi);
  else if (inf > 0)                              // infinitive
    sprintf_s(txt, ssz, "to %s", n->Lex());
  else if ((n->Val("obj") != NULL) || (n->Val("agt") != NULL) || 
           (n->Val("how") != NULL) || (n->Done() > 0))
    agt_verb(txt, ssz, n);
  else if (((targ = n->Val("hq"))  != NULL) || ((targ = n->Val("ako")) != NULL) || 
           ((targ = n->Val("loc")) != NULL))
    copula(txt, ssz, targ, n);
  else if (n->Lex() != NULL)
    strcpy_s(txt, ssz, n->Lex());                // e.g. "three" for cnt
  else
    strcpy_s(txt, ssz, "it");
  return txt;
}


//= Render a conjunction of nodes as a list of properties.
// list conjoined group in argument order (n might not be first)
// useful for question "What color is it?" --> "yellow, white, and black"

void jhcDegrapher::list_conj (char *txt, int ssz, const jhcNetNode *multi) const
{
  const jhcNetNode *part;
  int i, nc = multi->NumArgs();

  jprintf(1, noisy, "    list_conj: %s\n", multi->Nick());

  // start with empty list
  *txt = '\0';
  for (i = 0; i < nc; i++)
  { 
    // get term for this part of conjunction (e.g. "black")
    part = multi->Arg(i);
    strcat_s(txt, ssz, part->Lex());
   
    // add separator between terms (space, comma, or "and")
    if (i != (nc - 1))
      strcat_s(txt, ssz, ((nc > 2) ? ", " : " "));
    if (i == (nc - 2))
      strcat_s(txt, ssz, "and ");
  } 
}


//= Generate verb frame including performing agent (but no objects).
// example: "the big bird" + "is not grabbing" ...

void jhcDegrapher::agt_verb (char *txt, int ssz, const jhcNetNode *n) 
{
  char ing[40];  
  jhcNetNode *agt = n->Val("agt");

  jprintf(1, noisy, "    agt_verb: %s\n", n->Nick());

  if ((agt != NULL) && (agt != wmem->Robot()))
  {
    // action by agent other than the robot 
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
    // failed action with robot as agent 
    if (n->Done() > 0)
      sprintf_s(txt, ssz, "I couldn't %s", n->Lex());
    else if (n->LexMatch("know"))
      strcpy_s(txt, ssz, "I do not know");
    else
      sprintf_s(txt, ssz, "I don't %s", n->Lex());         // not progressive
  }
  else
  {
    // normal action with robot as agent
    if (n->Done() > 0)
      sprintf_s(txt, ssz, "I %s", mf->SurfWord(ing, n->Lex(), JTAG_VPAST));
    else if (n->LexMatch("know"))
      strcpy_s(txt, ssz, "I know");
    else
      sprintf_s(txt, ssz, "I am %s", mf->SurfWord(ing, n->Lex(), JTAG_VPROG));
  }
}


//= Make a statement about some property of an object.
// example: "the big dog" + "is not" + "very ferocious"

void jhcDegrapher::copula (char *txt, int ssz, jhcNetNode *targ, const jhcNetNode *n)
{
  const jhcNetNode *mod = n->Fact("deg");

  jprintf(1, noisy, "    copula: %s (target %s)\n", n->Nick(), targ->Nick());

  // generate target description for predication avoiding predicate term
  obj_ref(txt, ssz, targ, 1, n->Lex());
  strcat_s(txt, ssz, " is ");
  if (n->Neg() > 0)
    strcat_s(txt, ssz, "not ");

  // add primary term after prepending any degree modifier (e.g. "very big")
  if (mod != NULL)
    strcatf_s(txt, ssz, "%s ", mod->Lex());           
  if (n->Val("ako") != NULL)
    strcat_s(txt, ssz, "a ");
  if (n->Lex() != NULL)
    strcat_s(txt, ssz, n->Lex());
}


///////////////////////////////////////////////////////////////////////////
//                            Object Reference                           //
///////////////////////////////////////////////////////////////////////////

//= Uniquely describe some object, adding simple adjectives if necessary.
// can optionally request nominative case for pronouns ("he" vs "him")
// a negative value for "nom" inhibits the use of any pronouns (except "me/I" and "you")
// can avoid a particular term to prevent saying "the red object is red"
// returns best description text (which may not be unique)
// Note: maybe extend name_ref to allow further adjectives (e.g. "fat Dan")

const char *jhcDegrapher::obj_ref (char *txt, int ssz, jhcNetNode *n, int nom, const char *avoid) 
{
  const char *ref;
  int i;

  // sanity check
  if ((txt == NULL) || (ssz <= 0) || (n == NULL) || (wmem == NULL))
    return NULL;
  jprintf(1, noisy, "obj_ref: %s (nom %d, avoid %s)\n", n->Nick(), nom, avoid);

  // see if part of a hypothetical description
  if (n->Hyp() && (n->Generation() > 0))     
    return hyp_ref(txt, ssz, n, avoid);

  // consider using pronoun if unambiguous
  if ((ref = pron_ref(txt, ssz, n, nom)) != NULL)
    return ref;

  // search memory using single names and types
  ClrCond();
  MakeNode("obj");
  wmem->MaxBand(3);
  if ((ref = name_ref(n)) != NULL)
  {
    n->MarkConvo();                                        // robot speech
    strcpy_s(txt, ssz, ref);
    return txt;
  }
  if (add_kind(txt, ssz, n, avoid) != NULL)
  {
    n->MarkConvo();                                        // robot speech
    return txt;
  }

  // keep trying to add adjectives to basic noun phrase
  for (i = 0; i < 3; i++)
    if (add_adj(txt, ssz, n, avoid) != NULL)
      break;  
  n->MarkConvo();                                          // robot speech
  return txt;                                              
}


//= Try generating a pronoun reference for the given node.
// can optionally request nominative case ("he" vs "him")
// if nom < 0 then avoids everything except "me/I" and "you"

const char *jhcDegrapher::pron_ref (char *txt, int ssz, const jhcNetNode *n, int nom) const
{
  jhcNetNode *obj = NULL, *win = NULL;
  int best = 0;

  jprintf(1, noisy, "  pron_ref: %s (nom %d)\n", n->Nick(), nom);

  // see if referring to self or user
  if (n == wmem->Human())
    strcpy_s(txt, ssz, "you");
  else if (n == wmem->Robot())
    strcpy_s(txt, ssz, ((nom > 0) ? "I" : "me"));
  if (*txt != '\0')
  {
    jprintf(1, noisy, "    --> %s\n", txt);
    return txt;
  }
  
  // see if most recent thing mentioned
  if (nom < 0) 
    return NULL;                                           // not unique yet
  wmem->MaxBand(0);
  while ((obj = wmem->Next(obj)) != NULL)
    if (obj->ObjNode() && !obj->Hyp())
      if (obj->LastConvo() > best)
      {
        best = obj->LastConvo();     
        win = obj;
      }
  if ((win != n) || (best <= 0))
    return NULL;                                           // not unique yet

  // alter pronoun based on gender (if any)
  if (chk_prop(n, "hq", "female", NULL))
    strcpy_s(txt, ssz, ((nom > 0) ? "she" : "her"));
  else if (chk_prop(n, "hq", "male", NULL) ||
           chk_prop(n, "ako", "person", NULL) ||           // "they" would need plural verb
           (n->Fact("name") != NULL))                      // has a name implies person
    strcpy_s(txt, ssz, ((nom > 0) ? "he" : "him"));
  else
    strcpy_s(txt, ssz, "it");
  jprintf(1, noisy, "    --> %s\n", txt);
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

const char *jhcDegrapher::name_ref (const jhcNetNode *n) 
{
  jhcNetNode *p, *obj = cond.Main();
  double th = wmem->MinBlf();
  int g, i;

  jprintf(1, noisy, "  name_ref: %s\n", n->Nick());

  // search through all NAME properties
  for (g = 0; g <= 1; g++)                                 // allow ghost fact names
    for (i = n->NumProps() - 1; i >= 0; i--)      
      if ((p = n->PropMatch(i, "name", th)) != NULL)
        if (wmem->VisMem(p, g))
        {
          // see if node with name is unique 
          AddProp(obj, "name", p->Lex());                  // removed by num_match search
          if (num_match(1) <= 1)               
          {
            jprintf(1, noisy, "    --> %s\n", p->Lex());
            return p->Lex();                               // unique or most obvious 
          }
        }
  return NULL;                                             // not unique yet
}


//= See if any single kind for node make the selection unique.
// assumes "nr" has single object node only, retains most restrictive kind at end
// returns reference phrase if successful, NULL otherwise

const char *jhcDegrapher::add_kind (char *txt, int ssz, const jhcNetNode *n, const char *avoid) 
{
  char word[80];
  const jhcNetNode *p, *ref, *kind = NULL;
  jhcNetNode *ako, *own, *obj = cond.Main();
  const char *name;
  double th = wmem->MinBlf();
  int i, poss, hits, det, low;

  jprintf(1, noisy, "  add_kind: %s (avoid %s)\n", n->Nick(), avoid);

  // search through all AKO properties 
  for (i = n->NumProps() - 1; i >= 0; i--)
    if ((p = n->PropMatch(i, "ako", th)) != NULL)
      if (wmem->VisMem(p, 0) && !p->LexMatch(avoid))
      {
        // check for possessives me, you, or some named thing
        poss = 1;
        if ((ref = p->Val("wrt")) != NULL)
        {
          if (ref->LexMatch("me"))
            poss = 2;
          else if (ref->LexMatch("you"))
            poss = -2;
          else if (ref->Name(0, th) != NULL)
            poss = 3;
        }

        // evaluate enhanced description (new nodes removed by num_match search)
        ako = AddProp(obj, "ako", p->Lex());                     
        if (poss != 1)
        {
          own = MakeNode("obj", ref->LexStr());
          ako->AddArg("wrt", own);
          if (poss == 3)
            AddProp(own, "name", ref->Name(0, th));
        }
        hits = num_match(abs(poss));
                    
        // remember most restrictive category, stop if unique
        if ((kind == NULL) || (hits < low))
        {
          kind = p;
          det = poss;
          low = hits;
          if (low <= 1)                                    
            break;
        }
      }

  // simple case of no category found
  if (kind == NULL)
  {
    strcpy_s(txt, ssz, "the thing");
    jprintf(1, noisy, "    ++ %s\n", txt);
    return NULL;                                         
  }

  // keep best category in description and phrase
  ako = AddProp(obj, "ako", kind->Lex());
  if (det != 1)
  {
    // reconstruct possessive specification
    ref = kind->Val("wrt");
    own = MakeNode("agt", ref->Lex());
    ako->AddArg("wrt", own);
    if (det == 2)
      sprintf_s(txt, ssz, "my %s", kind->Lex());
    else if (det == -2)
      sprintf_s(txt, ssz, "your %s", kind->Lex());
    else 
    {
      name = ref->Name(0, th);
      AddProp(own, "name", name);
      sprintf_s(txt, ssz, "%s %s", mf->SurfWord(word, name, JTAG_NPOSS), kind->Lex());
    }
  }
  else
    sprintf_s(txt, ssz, "the %s", kind->Lex());

  // report whether unique or not
  jprintf(1, noisy, "    %s %s\n", ((low <= 1) ? "-->" : "++"), txt);
  return((low <= 1) ? txt : NULL);                                         
}


//= Add then next most selective simple adjective to description and check if unique.
// assumes "nr" has object node and possibly AKO and some earlier HQ's
// returns reference phrase, blank if out of choices, NULL if still ambiguous

const char *jhcDegrapher::add_adj (char *txt, int ssz, const jhcNetNode *n, const char *avoid) 
{
  char det[80], tail[500];
  jhcNetNode *obj = cond.Main();
  const jhcNetNode *p;
  const char *sp, *qual = NULL;
  double th = wmem->MinBlf();
  int i, hits, low;

  jprintf(1, noisy, "  add_adj: %s (avoid %s)\n", n->Nick(), avoid);

  // search through all HQ properties (ignore comparatives and superlatives)
  for (i = n->NumProps() - 1; i >= 0; i--)                                 
    if ((p = n->PropMatch(i, "hq", th)) != NULL)
      if (wmem->VisMem(p, 0) && !p->LexMatch(avoid) && (p->NumArgs() == 1))
        if (!chk_prop(obj, "hq", p->Lex(), &cond))             // avoid duplicates
        {
          // evaluate aggregated description (new nodes removed by num_match search)
          AddProp(obj, "hq", p->Lex());                        
          hits = num_match(1);

          // remember most restrictive property, stop if unique
          if ((qual == NULL) || (hits < low))
          {
            qual = p->Lex();
            low = hits;
            if (low <= 1)
              break;
          }
        }

  // simple case of no new property found (signal done)
  if (qual == NULL)
    return txt;                                        

  // keep most selective property in description and phrase 
  AddProp(obj, "hq", qual);
  if ((sp = strchr(txt, ' ')) != NULL)
  {
    // generally strip and save determiner like "my" or "Dave's"
    strncpy_s(det, txt, (int)(sp - txt));
    strcpy_s(tail, sp + 1);
    sprintf_s(txt, ssz, "%s %s %s", det, qual, tail); 
  }
  else
    strcpy_s(txt, ssz, qual);                              // should never occur

  // reprot whether unique or not
  jprintf(1, noisy, "    ++ %s\n", txt);
  return((low <= 1) ? txt : NULL); 
}


//= Describe hypothetical node using all properties in description.

const char *jhcDegrapher::hyp_ref (char *txt, int ssz, const jhcNetNode *n, const char *avoid) 
{
  char ref[40];
  const jhcNetNode *p = NULL;
  int i, np = n->NumProps();

  jprintf(1, noisy, "hyp_ref: %s (avoid %s)\n", n->Nick(), avoid);

  // if object has a name then ignore all other attributes
  for (i = np - 1; i >= 0; i--)      
    if ((p = n->PropMatch(i, "name")) != NULL)
      if (wmem->VisMem(p, 0) && !p->LexMatch(avoid))
      {
        strcpy_s(txt, ssz, p->Lex());
        return txt;
      }

  // start with determiner then add all adjectives
  strcpy_s(txt, ssz, "a ");
  for (i = np - 1; i >= 0; i--)      
    if ((p = n->PropMatch(i, "hq")) != NULL)
      if (wmem->VisMem(p, 0) && !p->LexMatch(avoid))
        strcatf_s(txt, ssz, "%s ", p->Lex());

  // add single base noun (if any)
  for (i = np - 1; i >= 0; i--)      
    if ((p = n->PropMatch(i, "ako")) != NULL)
      if (wmem->VisMem(p, 0) && !p->LexMatch(avoid))
        break;
  if (i >= 0)
    strcat_s(txt, ssz, p->Lex());
  else
    strcat_s(txt, ssz, "thing");

  // add any location relations afterward
  for (i = np - 1; i >= 0; i--)
    if ((p = n->PropMatch(i, "loc")) != NULL)
      if (wmem->VisMem(p, 0) && !p->LexMatch(avoid))
      {
        // add main preposition and one or two reference objects (e.g. "between")
        strcatf_s(txt, ssz, " %s %s", p->Lex(), node_ref(ref, 40, p->Val("ref"), 0, NULL));
        if (p->NumArgs() > 2)
          strcatf_s(txt, ssz, " and %s", node_ref(ref, 40, p->Val("ref2"), 0, NULL));
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


//= See how many matches there are to description in "cond" graphlet.
// pops last few nodes off description when matching complete

int jhcDegrapher::num_match (int strip)
{
  jhcBindings b;
  int hits, mc = 1; 

  // possibly tell what is sought
  bth = wmem->MinBlf();
  if (dbg >= 2)
  {
    jprintf("num_match >= %4.2f\n", bth);
    cond.Print("pattern", 2);
  }

  // do matching then clean up description
  b.expect = cond.NumItems();
  hits = MatchGraph(&b, mc, cond, *wmem);
  cond.Pop(strip);
  jprintf(2, noisy, "    hits = %d\n", hits);
  return hits;
}
