// jhcNetBuild.cpp : turns parser alist into network structures
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

#include "Action/jhcAliaChain.h"       // common robot 
#include "Action/jhcAliaCore.h"        // since only spec'd as class in header
#include "Action/jhcAliaPlay.h"

#include "Parse/jhcTxtLine.h"          // common audio
#include "Reasoning/jhcActionTree.h"   // since only spec'd as class in header

#include "Language/jhcNetBuild.h"


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= See if attention (to speech) should be renewed based on association list.
// basically looks for the presense of the ATTN non-terminal category
// mode: 0 = always, 1 = ATTN anywhere, 2 = ATTN at start, 3 = ATTN only (hail)
// NOTE: better rejection of initial yes/no (if parsable) than jhcGramExec version 

int jhcNetBuild::NameSaid (const char *alist, int mode) const
{
  char slot[40];
  const char *tail;

  // ignore wake-up for text input
  if (mode <= 0)
    return 1;

  // must have vocative somewhere
  if (!HasSlot(alist, "ATTN"))
    return 0;
  if (mode == 1)
    return 1;

  // must have vocative at beginning (leading "yes" or "no" allowed)
  tail = NextSlot(alist, slot, NULL, 1, 40, 0);
  if (match_any(slot, "YES", "NO", "HQ"))
    NextSlot(tail, slot, NULL, 1, 40, 0);
  if (strcmp(slot, "AKO") == 0)                  // for "idiot" or "good boy"
    NextSlot(tail, slot, NULL, 1, 40, 0);
  if (strcmp(slot, "ATTN") != 0)
    return 0;
  if (mode == 2)
    return 1;

  // must have no other entries
  if (*tail != '\0')
    return 0;
  return 1;
}


//= Build an appropriate structure based on given association list.
// also save input utterance for new rules or operators
// return: 10 = vocabulary, 9 = farewell, 8 = greet, 7 = hail, 
//         6 = op, 5 = rule, 4 = revision, 3 = question, 2 = command, 1 = fact, 
//         0 = nothing, negative for error

int jhcNetBuild::Convert (const char *alist, const char *sent)
{
  jhcActionTree *atree;
  jhcAliaChain *ch;
  const char *unk;
  int spact;

  // sanity check then cleanup any rejected suggestions
  if (core == NULL) 
    return -1;
  atree = &(core->atree);
  add = NULL;                                // deleted elsewhere
  ClearLast();
  unk = (core->vc).Confused();
  if ((alist == NULL) || (*alist == '\0'))               
  {
    if (*unk != '\0')
      return unk_tag(unk);                   // unknown word
    return huh_tag();                        // misheard utterance
  }

  // handle user introduction by name (always believes user)
  if (HasFrag(alist, "$intro"))
  {
    intro_name(alist);                       // assign user name
    return greet_tag();
  }

  // generate core interpretation then add speech act
  spact = Assemble(alist);
  if ((spact >= 1) && (spact <= 3))
    return attn_tag(spact, alist);           // fact or command
  if (spact == 4)
    return rev_tag(spact, alist);            // operator revision
  if ((spact >= 5) && (spact <= 6))
    return add_tag(spact, alist, sent);      // new rule or operator

  // look for naked kudo phrases (always believes user)
  if ((ch = feedback(spact, alist)) != NULL)
  {
    atree->AddFocus(ch);
    return 1;
  }

  // handle superficial speech acts
  if (HasSlot(alist, "HELLO"))               // simple greeting
    return greet_tag();
  if (HasSlot(alist, "BYE"))                 // simple farewell
    return farewell_tag();
  if (HasSlot(alist, "ATTN"))                // calling robot name
    return hail_tag();
  return huh_tag();                          // no network created
}


//= Possibly change to new user node given name or restriction on name.

void jhcNetBuild::intro_name (const char *alist) const
{
  char name[80];
  jhcActionTree *atree = &(core->atree);
  jhcNetNode *user = atree->Human();
  int neg = 0;

  // get name and whether asserted or denied
  if (FindSlot(alist, "NAME", name) == NULL)
    return; 
  if (HasSlot(alist, "NEG"))
    neg = 1;

  // possibly change user node 
  if (atree->NameClash(user, name, neg))
    user = atree->SetUser((neg <= 0) ? atree->FindName(name) : NULL);

  // add name and person facts to network
  atree->StartNote();
  atree->AddName(user, name, neg);
  atree->AddProp(user, "ako", "person", 0, 1.0, 1);
  atree->FinishNote();
}


//= Generate a NOTE directive expressing user opinion of current performance.
// looks for standalone kudos: HQ, HQ AKO, and AKO
// as well as possibly embedded kudos: ACC, REJ, YES, and NO

jhcAliaChain *jhcNetBuild::feedback (int spact, const char *alist) const
{
  char first[40], val[40], prop[40] = "hq", term[40] = "";
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch;
  jhcAliaDir *dir;
  int neg = 0;    

  // get feedback type and sign (likely from revision)
  if (AnySlot(alist, "ACC REJ"))
    strcpy_s(term, "good");
  else if (AnySlot(alist, "YES NO"))
    strcpy_s(term, "correct");        
  if (AnySlot(alist, "REJ NO"))
    neg = 1;

  // look for explicit kudo at front ("clever girl" or "idiot")    
  NextSlot(alist, first, val, 1);
  if (match_any(first, "HQ", "AKO"))
  {
    strcpy_s(term, val);                         // use actual word
    if (strcmp(first, "AKO") == 0)          
      strcpy_s(prop, "ako");
  }
  else if (match_any(first, "YES", "NO") && 
           ((spact == 2) || (spact == 6)))
    strcpy_s(term, "good");                      // change default             
  else if (*term == '\0')
    return NULL;

  // build NOTE directive with info and encapsulate in a step
  dir = new jhcAliaDir;
  ch = new jhcAliaChain;
  atree->BuildIn(dir->key);
  atree->AddProp(atree->Robot(), prop, term, neg);
  ch->BindDir(dir);
  return ch;
}


//= Record a summary of last sentence conversion process.
// basically shows what was produced by jhcGraphizer for last sentence
// part of jhcNetBuild because needs access to "add" and "bulk"

void jhcNetBuild::Summarize (FILE *log, const char *sent, int nt, int spact) const
{
  // make sure there is somewhere to record stuff
  if (log == NULL)
    return;

  // record overall parsing result
  fprintf(log, ".................................................\n\n");
  fprintf(log, "\"%s\"\n\n", sent);
  if (nt == 0)
    fprintf(log, "*** NO PARSE ***\n\n");
  else if (nt > 1)
    fprintf(log, "*** %d parses ***\n\n", nt);

  // record interpretation result
  if (spact == 9)
    fprintf(log, "-- farewell --\n\n");
  else if (spact == 8)
    fprintf(log, "-- greeting --\n\n");
  else if (spact == 7)
    fprintf(log, "-- hail --\n\n");
  else if ((spact == 6) && (add != NULL) && (add->new_oper != NULL))
    (add->new_oper)->Save(log);
  else if ((spact == 5) && (add != NULL) && (add->new_rule != NULL))
    (add->new_rule)->Save(log);
  else if ((spact <= 4) && (spact >= 1) && (bulk != NULL))
  {
    bulk->Save(log, 2);
    fprintf(log, "\n");
  }
  else if (nt > 0)
    fprintf(log, "-- nothing --\n\n");
}


///////////////////////////////////////////////////////////////////////////
//                              Speech Acts                              //
///////////////////////////////////////////////////////////////////////////

//= Generate speech act NOTE for incomprehensible input.
// <pre>
//   NOTE[ input-1 -lex-  understand
//                 -asp-  neg
//                 -agt-> self-1
//                 -obj-> user-3 ]
// </pre>
// always returns 0 for convenience

int jhcNetBuild::huh_tag () const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = new jhcAliaChain;
  jhcAliaDir *dir = new jhcAliaDir;
  jhcNetNode *n;
 
  // fill in details of the speech act
  atree->BuildIn(dir->key);
  n = atree->MakeAct("understand", 1);
  n->AddArg("agt", atree->Robot());               // in WMEM since NOTE
  n->AddArg("obj", atree->Human());               // in WMEM since NOTE

  // add completed structure to attention buffer
  ch->BindDir(dir);
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return 0;
}


//= Generate speech act noting that the robot's name was called.

int jhcNetBuild::hail_tag () const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = build_tag(NULL, "hail", NULL, 1);

  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return 7;
}


//= Generate speech act noting that the user wants to communicate.

int jhcNetBuild::greet_tag () const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = build_tag(NULL, "greet", NULL, 1);

  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return 8;
}


//= Generate speech act noting that the user is leaving.

int jhcNetBuild::farewell_tag () const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = build_tag(NULL, "dismiss", NULL, 1);

  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return 9;
}


//= Generate speech act NOTE for word not in vocabulary.
// <pre>
//   NOTE[ input-1 -lex-  know
//                 -asp-  neg
//                 -agt-> self-1
//                 -obj-> txt-1 
//           txt-1 -str-  xxx 
//           ako-1 -lex-  word
//                 -ako-> txt-1]
// </pre>
// always returns 10 for convenience

int jhcNetBuild::unk_tag (const char *word) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = new jhcAliaChain;
  jhcAliaDir *dir = new jhcAliaDir;
  jhcNetNode *n, *w;
 
  // fill in details of the speech act
  atree->BuildIn(dir->key);
  n = atree->MakeAct("know", 1);
  n->AddArg("agt", atree->Robot());               // in WMEM since NOTE
  w = atree->MakeNode("txt");
  w->SetString(word);
  n->AddArg("obj", w);
  atree->AddProp(w, "ako", "word");

  // add completed structure to attention buffer
  ch->BindDir(dir);
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return 10;
}


//= Generate speech act followed by a request to add rule or operator.
// save core of ADD directive in "add" for convenience
// returns 5 for rule, 6 for operator (echoes input "kind")

int jhcNetBuild::add_tag (int spact, const char *alist, const char *sent) 
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch, *steps, *tail;
  jhcNetNode *input, *item;
 
  // make a new NOTE directive for speech act
  ch = build_tag(&input, "give", alist, 1);
  item = atree->MakeNode((spact == 5) ? "rule" : "op");
  input->AddArg("obj", item);
  atree->AddProp(item, "ako", ((spact == 5) ? "rule" : "operator"));

  // possibly tack on user feedback ("yes" or "no") after speech act
  if ((tail = feedback(spact, alist)) != NULL)
    ch->cont = tail;
  else
    tail = ch;

  // make a new ADD directive to add rule or operator
  steps = new jhcAliaChain;
  add = new jhcAliaDir(JDIR_ADD);
  (add->key).AddItem(item);            // dummy node
  steps->BindDir(add);
  steps->fail = exp_fail(item);        // failed for some reasone

  // move newly create rule or operator into directive (in case slow)
  if (spact == 5)
  {
    rule->SetGist(no_fluff(sent, alist));
    add->new_rule = rule;
  }
  else
  {
    oper->SetGist(no_fluff(sent, alist));
    add->new_oper = oper;
  }
  rule = NULL;                         // prevent deletion by jhcGraphizer
  oper = NULL;

  // combine with preamble and transfer structure to attention buffer
  tail->cont = steps;
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return spact;
}


//= Insert NOTE directive about source of command or fact before actual statement.
// gives the opportunity to PUNT and disbelieve fact or reject command
// returns 4 for valid revision, 0 for problem

int jhcNetBuild::rev_tag (int spact, const char *alist) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch, *tail;
  jhcNetNode *input, *item;

  // make a new NOTE directive for speech act
  ch = build_tag(&input, "revise", alist, 0);
  item = atree->MakeNode("op");
  input->AddArg("obj", item);
  atree->AddProp(item, "ako", "operator");

  // possibly tack on user feedback ("yes" or "no") after speech act
  if ((tail = feedback(spact, alist)) != NULL)
    ch->cont = tail;
  else
    tail = ch;

  // tack on a play encapsulating the bulk sequence
  // then add completed structure to attention buffer
  tail->cont = guard_plan(bulk, item);
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return 4;
}


//= Insert NOTE directive about source of command or fact before actual statement.
// gives the opportunity to PUNT and disbelieve fact or reject command
// returns 3 for question, 2 for command, 1 for fact, 0 for problem

int jhcNetBuild::attn_tag (int spact, const char *alist) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch, *tail;
  jhcNetNode *input, *item;

  // make a new NOTE directive for speech act
  // question "ask act", command "tell act", fact "tell obj"
  ch = build_tag(&input, ((spact >= 3) ? "ask" : "tell"), alist, 1);
  item = atree->MakeNode("plan");
  input->AddArg(((spact >= 2) ? "act" : "obj"), item);

  // possibly tack on user feedback ("yes" or "no") after speech act
  if ((tail = feedback(spact, alist)) != NULL)
    ch->cont = tail;
  else
    tail = ch;

  // tack on a play encapsulating the bulk sequence
  // then add completed structure to attention buffer
  tail->cont = guard_plan(bulk, item);
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return spact;
}


//= Build a chain consisting of a single NOTE directive about speech act.
// also returns pointer to main assertion in directive
// leaves jhcGraphlet accumulator of WMEM assigned to this directive

jhcAliaChain *jhcNetBuild::build_tag (jhcNetNode **node, const char *fcn, const char *alist, int dest) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = new jhcAliaChain;
  jhcAliaDir *dir = new jhcAliaDir;
  jhcNetNode *n;
 
  // fill in details of the speech act
  atree->BuildIn(dir->key);
  n = atree->MakeAct(fcn);
  n->AddArg("agt", atree->Human());                 // in WMEM since NOTE
  if (dest > 0)
    n->AddArg("dest", atree->Robot());              // in WMEM since NOTE
  if ((alist != NULL) && HasSlot(alist, "POLITE"))
    atree->AddProp(n, "mod", "polite");

  // embed in chain then return pieces
  ch->BindDir(dir);
  if (node != NULL)
    *node = n;
  return ch;
}


//= Strip off any preamble and leading or trailing attention words.

const char *jhcNetBuild::no_fluff (const char *sent, const char *alist)
{
  char slot[40];
  const char *tail, *start = sent;
  int i;

  // look at initial non-terminals in association list
  tail = NextSlot(alist, slot, NULL, 1, 40, 0);
  if (match_any(slot, "YES", "NO", "HQ"))
  {
    // skip over first word in sentence
    start = strchr(sent, ' ');
    while (*start++ != '\0')
      if (*start != ' ')
        break;
    tail = NextSlot(tail, slot, NULL, 1, 40, 0);
  }
  if (strcmp(slot, "ATTN") == 0)
  {
    // skip over next word in sentence
    start = strchr(start, ' ');
    while (*start++ != '\0')
      if (*start != ' ')
        break;
  }

  // find final non-terminal in association list
  *slot = '\0';
  while (tail != NULL)
    tail = NextSlot(tail, slot, NULL, 0, 40, 0);
  if (strcmp(slot, "ATTN") != 0)
    return start;

  // strip off last word from sentence
  strcpy_s(trim, start);
  i = (int) strlen(trim);
  while (--i > 0)
    if (trim[i] == ' ')
      break;
  while (--i > 0)
    if (trim[i] != ' ')
      break;
  trim[i + 1] = '\0';
  return trim;
}


//= Generate a TRAP directive encapsulating payload (symbolic node "ref").
// returns chain step with its overall fail branch being an explanation

jhcAliaChain *jhcNetBuild::guard_plan (jhcAliaChain *steps, jhcNetNode *plan) const
{
  jhcAliaChain *ch = steps;
  jhcAliaPlay *pod;

  // encapsulate plan in a play unless just a single activity play
  if ((steps->GetPlay() == NULL) || (steps->cont != NULL))
  {
    ch = new jhcAliaChain;
    pod = new jhcAliaPlay;
    ch->BindPlay(pod);
    pod->AddReq(steps);
  }

  // request explanation on failure of anything in pod
  ch->fail = exp_fail(plan);
  return ch;
}


//= Add a request to explain the failure of some action.

jhcAliaChain *jhcNetBuild::exp_fail (jhcNetNode *plan) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = new jhcAliaChain;
  jhcAliaDir *cry = new jhcAliaDir(JDIR_DO);
  jhcNetNode *exp, *prob;

  atree->BuildIn(cry->key);
  exp = atree->MakeAct("explain");
  prob = atree->MakeAct("fail");
  prob->AddArg("act", plan);
  exp->AddArg("obj", prob);
  atree->BuildIn(NULL);
  ch->BindDir(cry);
  return ch;
}


///////////////////////////////////////////////////////////////////////////
//                           Value Range Rules                           //
///////////////////////////////////////////////////////////////////////////

//= Reads a file of potentential property values and makes auxilliary files.
// reads from "kern.vals" with format:
// <pre>
// 
//   =width : narrow wide    // value "width" with lo = "narrow" and hi = "wide" (exclusive)
//     -skinny               // alias for lo value
//      +fat                 // alias for hi value
// 
//   =color                  // "color" category
//     red                   // one non-exclusive color value 
//     yellow                // another non-exclusive value
//     green
// 
// </pre>    
// values for a property are: very <lo>, <lo>, medium <hi>, <hi>, very <hi>
// for colloquial phrasing sometimes <lo> and <hi> have to be reversed, e.g. "medium close"
// returns number of categories read and generates starter files "kern0.rules" and "kern_v0.rules"
// Note: these output files can be further processed with HarvestLex to give a starter "kern0.sgm" file
//       gists might have phrases like "farer" instead of "farther" if "kern.sgm" file is incomplete

int jhcNetBuild::AutoVals (const char *kern) 
{
  jhcTxtLine txt;
  char fname[200], hi[40], lo[40], cat[40] = "";
  FILE *rules, *imply;  
  const char *val, *base;
  int nr = 0, ni = 0, nc = 0;

  // open input and output files
  if (kern == NULL)
    return 0;
  sprintf_s(fname, "%s.vals", kern);
  if (!txt.Open(fname))
    return 0;
  sprintf_s(fname, "%s0.rules", kern);
  if (fopen_s(&rules, fname, "w") != 0)
    return txt.Close();
  sprintf_s(fname, "%s_v0.rules", kern);
  if (fopen_s(&imply, fname, "w") != 0)
  {
    fclose(rules);
    return txt.Close();
  }

  // write output file headers
  if ((val = strrchr(kern, '\\')) != NULL)
    val++;
  else
    val = kern;
  if ((base = strrchr(val, '/')) != NULL)
    base++;
  else
    base = val;
  fprintf(rules, "// Category definitions and rules for %s kernel\n", base);
  fprintf(rules, "// ========================================================\n\n");
  fprintf(imply, "// Inferences between category values in %s kernel\n", base);
  fprintf(imply, "// ========================================================\n\n");

  // look for non-comment input lines with category prefix
  while (txt.NextContent() != NULL)
  {
    // see if new category, term alias, or possible value
    if (txt.Begins("="))
    {
      // save category name and range limits (if any)
      strcpy_s(cat, txt.Token(1) + 1);
      txt.Token();                               // delimiter ignored
      txt.Token(lo, 1);                          
      txt.Token(hi, 1);                          
      nc++;

      // insert delimiters between categories in basic rules
      if (nr > 0)
        fprintf(rules, "// ------------------------------------------------\n\n");

      // ranges with hi and lo vals (no imply rules for things like colors)
      if ((*lo != '\0') && (*hi != '\0'))                  
      {
        if (ni > 0)
          fprintf(imply, "// ================================================\n\n");
        nr = range_rules(rules, cat, lo, hi, nr);          
        ni = exclude_rules(imply, lo, hi, ni);
      }
    }
    else if (*cat != '\0') 
    {
      if ((*lo == '\0') || (*hi == '\0'))      
      {            
        // enumerations like color
        val = txt.Token();
        nr = value_rules(rules, cat, val, -nr);     
      }
      else if (txt.Begins("-"))
        ni = alias_rules(imply, cat, lo, txt.Token() + 1, ni);
      else if (txt.Begins("+"))
        ni = alias_rules(imply, cat, hi, txt.Token() + 1, ni);
    }
    txt.Next(1);  
  }    

  // add separator for user extras then cleanup
  fprintf(rules, "// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
  fprintf(imply, "// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n\n");
  fclose(imply);
  fclose(rules);
  txt.Close();
  return nc;
}


//= Create basic rules for interpreting values that are part of some category's range.

int jhcNetBuild::range_rules (FILE *out, const char *cat, const char *lo, const char *hi, int n) const
{
  char mid[40];
  int nr = n;

  sprintf_s(mid, "medium %s", hi);
  nr = value_rules(out, cat, lo,  nr);
  nr = value_rules(out, cat, mid, -nr);
  nr = value_rules(out, cat, hi,  nr);
  return nr;
}


//= Assign value to this category and make category equivalent to "ness" version of value.
// always skips "ness" part if n is negative (e.g. for "medium big")

int jhcNetBuild::value_rules (FILE *out, const char *cat, const char *val, int n) const
{
  char ness[80];
  int nr = abs(n);

  // membership rule
  fprintf(out, "RULE %d - \"%c%s is a %s\"\n", nr + 1, toupper(val[0]), val + 1, cat);
  fprintf(out, "    if:  hq-1 -lex-  %s\n", val);
  fprintf(out, "              -hq--> obj-1\n");
  fprintf(out, "  then: ako-1 -lex-  %s\n", cat);
  fprintf(out, "              -ako-> hq-1\n\n");

  // create artificial category from value ("wide" -> "wideness")
  if (n < 0)
    return(nr + 1);
  mf.PropKind(ness, val);
  if (strcmp(ness, cat) == 0)          // skip "thick" -> "thickness"
    return(nr + 1);
  
  // search rule
  fprintf(out, "RULE %d - \"A %s is a %s\"\n", nr + 2, ness, cat);   
  fprintf(out, "    if: ako-1 -lex-  %s\n", ness);
  fprintf(out, "              -ako-> hq-1\n");
  fprintf(out, "  then: ako-2 -lex-  %s\n", cat);
  fprintf(out, "              -ako-> hq-1\n\n");

  // result conversion rule
  fprintf(out, "RULE %d - \"A %s is a %s\"\n", nr + 3, cat, ness);
  fprintf(out, "    if: ako-1 -lex-  %s\n", cat);
  fprintf(out, "              -ako-> hq-1\n");
  fprintf(out, "  then: ako-2 -lex-  %s\n", ness);
  fprintf(out, "              -ako-> hq-1\n\n");
  return(nr + 3);
}


//= Have one value in a range precludes object from having another value in range.

int jhcNetBuild::exclude_rules (FILE *out, const char *lo, const char *hi, int n) const
{
  char mid[40];
  int nr = n;

  // properties (first in category so no delimiter)
  sprintf_s(mid, "medium %s", hi);
  nr = mutex_rule(out, lo,  mid, nr);
  nr = mutex_rule(out, lo,  hi,  nr);
  nr = mutex_rule(out, mid, lo,  nr);
  nr = mutex_rule(out, mid, hi,  nr);
  nr = mutex_rule(out, hi,  lo,  nr);
  nr = mutex_rule(out, hi,  mid, nr);

  // comparisons
  fprintf(out, "// ------------------------------------------------\n\n");
  nr = opposite_rule(out, lo, hi, nr);
  nr = opposite_rule(out, hi, lo, nr);
  return nr;
}


//= Assert that if the property has this value it cannot be the alternative value.

int jhcNetBuild::mutex_rule (FILE *out, const char *val, const char *alt, int n) const
{
  fprintf(out, "RULE %d - \"If something is %s then it is not %s\"\n", n + 1, val, alt);
  fprintf(out, "    if: hq-1 -lex-  %s\n", val);
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "  then: hq-2 -lex-  %s\n", alt);
  fprintf(out, "             -neg-  1\n");
  fprintf(out, "             -hq--> obj-1\n\n");
  return(n + 1);
}


//= Opposite extremes of the comparison range cannot both be true.
// gists might have phrases like "more far" instead of "farther" if "kern.sgm" file is incomplete

int jhcNetBuild::opposite_rule (FILE *out, const char *v1, const char *v2, int n) const
{
  char c1[40], c2[40]; 

  fprintf(out, "RULE %d - \"If something is %s than something else then that something is %s than it\"\n",
               n + 1, mf.SurfWord(c1, v1, JTAG_ACOMP), mf.SurfWord(c2, v2, JTAG_ACOMP)); 
  fprintf(out, "    if: hq-1 -lex-  %s\n", v1);
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "             -alt-> obj-2\n");
  fprintf(out, "  then: hq-2 -lex-  %s\n", v2);
  fprintf(out, "             -hq--> obj-2\n");
  fprintf(out, "             -alt-> obj-1\n\n");
  return(n + 1);
}


//= Define two adjectival range terms as being equivalent.

int jhcNetBuild::alias_rules (FILE *out, const char *cat, const char *val, const char *alt, int n) const
{
  char vc[80], ac[80];;
  int nr = n;

  // add separator then basic membership rules for alternate
  fprintf(out, "// ------------------------------------------------\n\n");
  nr = value_rules(out, cat, alt, nr);

  // affirm alternate term
  fprintf(out, "RULE %d - \"If something is %s then it is %s\"\n", ++nr, val, alt);
  fprintf(out, "    if: hq-1 -lex-  %s\n", val);
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "  then: hq-2 -lex-  %s\n", alt);
  fprintf(out, "             -hq--> obj-1\n\n");
  fprintf(out, "RULE %d - \"If something is %s then it is %s\"\n", ++nr, alt, val);
  fprintf(out, "    if: hq-1 -lex-  %s\n", alt);
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "  then: hq-2 -lex-  %s\n", val);
  fprintf(out, "             -hq--> obj-1\n\n");

  // refute alternate term
  fprintf(out, "RULE %d - \"If something is not %s then it is not %s\"\n", ++nr, val, alt);
  fprintf(out, "    if: hq-1 -lex-  %s\n", val);
  fprintf(out, "             -neg-  1\n");
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "  then: hq-2 -lex-  %s\n", alt);
  fprintf(out, "             -neg-  1\n");
  fprintf(out, "             -hq--> obj-1\n\n");
  fprintf(out, "RULE %d - \"If something is not %s then it is not %s\"\n", ++nr, alt, val);
  fprintf(out, "    if: hq-1 -lex-  %s\n", alt);
  fprintf(out, "             -neg-  1\n");
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "  then: hq-2 -lex-  %s\n", val);
  fprintf(out, "             -neg-  1\n");
  fprintf(out, "             -hq--> obj-1\n\n");

  // equivalence of comparatives
  mf.SurfWord(vc, val, JTAG_ACOMP); 
  mf.SurfWord(ac, alt, JTAG_ACOMP);
  fprintf(out, "RULE %d - \"If something is %s than something else then it is %s than it\"\n", ++nr, vc, ac);
  fprintf(out, "    if: hq-1 -lex-  %s\n", val);
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "             -alt-> obj-2\n");
  fprintf(out, "  then: hq-2 -lex-  %s\n", alt);
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "             -alt-> obj-2\n\n");
  fprintf(out, "RULE %d - \"If something is %s than something else then it is %s than it\"\n", ++nr, ac, vc);
  fprintf(out, "    if: hq-1 -lex-  %s\n", alt);
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "             -alt-> obj-2\n");
  fprintf(out, "  then: hq-2 -lex-  %s\n", val);
  fprintf(out, "             -hq--> obj-1\n");
  fprintf(out, "             -alt-> obj-2\n\n");
  return nr;
}


///////////////////////////////////////////////////////////////////////////
//                         Vocabulary Generation                         //
///////////////////////////////////////////////////////////////////////////

//= Get potential lexicon used by a set of operators and rules.
// examines files "kern.ops" and "kern.rules" (if they exist)
// generates file "kern.sgm0" with likely categories (cannot find mass nouns)
// expects node base names to be indicative (e.g. hg-1 ako-7 act-3 agt-12)
// use jhcMoprhFcns::LexDeriv then LexBase to fix up irregular morphologies 
// returns number of words listed in output file
// NOTE: will not get terms in volunteered events or those used internally!

int jhcNetBuild::HarvestLex (const char *kern) 
{
  char fname[200];
  const char *n2, *name = kern;

  // clear counts
  nn = 0;
  na = 0;
  nt = 0;
  nv = 0;
  nm = 0;
  nd = 0;

  // pull words from sources
  sprintf_s(fname, "%s.ops", kern);
  scan_lex(fname);
  sprintf_s(fname, "%s.rules", kern);
  scan_lex(fname);
  sprintf_s(fname, "%s_v.rules", kern);
  scan_lex(fname);

  // generate output grammar file
  name = kern;
  if ((n2 = strrchr(name, '/')) != NULL)
    name = n2 + 1;
  if ((n2 = strrchr(name, '\\')) != NULL)
    name = n2 + 1;
  sprintf_s(fname, "%s0.sgm", kern);
  return gram_cats(fname, name);
}


//= Find all open class words based on semantic networks in given file.
// assumes "-lex-" properties always come first so next to node name
// expects node base names to be indicative (e.g. hg-1 ako-7 act-3 agt-12)

int jhcNetBuild::scan_lex (const char *fname)
{
  char line[200], term[40], node[40] = "";
  FILE *in;
  const char *sep, *wds, *end, *last, *hint;

  // try opening file
  if (fopen_s(&in, fname, "r") != 0)
    return 0;

  // look for non-comment lines with a lexical term label
  while (fgets(line, 200, in) != NULL)
    if (strncmp(line, "//", 2) != 0)
      if ((sep = strstr(line, "-lex-")) != NULL)
      {
        // find associated word(s) 
        wds = sep + 5;
        while (isalnum(*(++wds)) == 0)
          if ((*wds == '\0') || (*wds == '\n') || (*wds == '*'))
            break;
        if (isalnum(*wds) == 0)
          continue;
        end = wds + 1;
        last = end;
        while ((*end != '\0') && (*end != '\n')) 
        {
          if (isalnum(*end) != 0)
            last = end;
          end++;
        }
        strncpy_s(term, wds, last - wds + 1);
        if ((strcmp(term, "me") == 0) || (strcmp(term, "you") == 0))
          continue;

        // get category type from previous node label
        last = sep;
        while (--last > line)
          if (isalnum(*last) != 0)
            break;
        hint = last;
        while ((isalnum(*hint) != 0) || (*hint == '-'))
          if (--hint < line)
            break;
        hint++;
        if (isalnum(*hint) != 0)
          strncpy_s(node, hint, last - hint + 1);

        // add term to some list based on node kind
        if (strncmp(node, "ako", 3) == 0)
          save_word(noun, nn, term);
        else if (strncmp(node, "hq", 2) == 0)
          save_word(adj, na, term);
        else if (strncmp(node, "name", 4) == 0)
          save_word(tag, nt, term);
        else if (strncmp(node, "fcn", 3) == 0)
          save_word(verb, nv, term);
        else if (strncmp(node, "mod", 3) == 0)
          save_word(mod, nm, term);
        else if (strncmp(node, "dir", 3) == 0)
          save_word(dir, nd, term);
      }

  // cleanup
  fclose(in);
  return 1;
}


//= Save term in list if new and enough room.
// automatically updates count for list

void jhcNetBuild::save_word (char (*list)[40], int& cnt, const char *term) const
{
  int i;

  if (cnt >= wmax)
    return;
  for (i = 0; i < cnt; i++)
    if (strcmp(list[i], term) == 0)
      return;
  strcpy_s(list[cnt], term);
  cnt++;
}


//= Dump accumulated words into a properly formatted grammar file.
// returns total number of words listed

int jhcNetBuild::gram_cats (const char *fname, const char *label) const
{
  FILE *out;
  int i, n = nn + na + nt, v = nv + nm + nd, total = n + v;

  // sanity check then try opening file
  if (total <= 0)
    return 0;
  if (fopen_s(&out, fname, "w") != 0)
    return jprintf(">>> Could not open output file: %s !\n", fname);

  // generate header
  fprintf(out, "// terms associated with \"%s\" ops and rules\n", label);
  fprintf(out, "// ================================================\n\n");

  // nouns
  fprintf(out, "// singular nouns\n\n");
  fprintf(out, "=[AKO]\n");
  for (i = 0; i < nn; i++)
    fprintf(out, "  %s\n", noun[i]);
  fprintf(out, "\n\n");

  // mass nouns (not harvested)
  fprintf(out, "// mass nouns (like \"a rice\")\n\n");
  fprintf(out, "=[AKO-M]\n");
  fprintf(out, "\n\n");

  // adjectives
  fprintf(out, "// adjectives\n\n");
  fprintf(out, "=[HQ]\n");
  for (i = 0; i < na; i++)
    fprintf(out, "  %s\n", adj[i]);
  fprintf(out, "\n\n");

  // names
  fprintf(out, "// proper nouns\n\n");
  fprintf(out, "=[NAME]\n");
  for (i = 0; i < nt; i++)
    fprintf(out, "  %s\n", tag[i]);
  fprintf(out, "\n\n");

  // adverbs
  fprintf(out, "// -----------------------------------------\n\n");
  fprintf(out, "// modifiers\n\n");
  fprintf(out, "=[MOD]\n");
  for (i = 0; i < nm; i++)
    fprintf(out, "  %s\n", mod[i]);
  fprintf(out, "\n\n");

  // directions
  fprintf(out, "// directions\n\n");
  fprintf(out, "=[DIR]\n");
  for (i = 0; i < nd; i++)
    fprintf(out, "  %s\n", dir[i]);
  fprintf(out, "\n\n");

  // verbs
  fprintf(out, "// imperative verbs\n\n");
  fprintf(out, "=[ACT]\n");
  for (i = 0; i < nv; i++)
    fprintf(out, "  %s\n", verb[i]);
  fprintf(out, "\n\n");

  // morphology placeholder (English)
  fprintf(out, "// ================================================\n\n");
  fprintf(out, "// irregular morphologies (npl, acomp, asup, vpres, vprog, vpast)\n\n");
  fprintf(out, "=[XXX-morph]\n\n");

  // cleanup
  fclose(out);
  return total;
}

