// jhcNetBuild.cpp : turns parser alist into network structures
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020-2026 Etaoin Systems
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
// mode: <= 0 always on, 1 ATTN anywhere, 2 ATTN at start, 3 ATTN only (hail)
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
// most inputs generate: speech-act + payload + user-response

JSP_ACT jhcNetBuild::Convert (const char *alist, const char *sent)
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *main;
  jhcAliaPlay *pod;
  const char *unk;
  JSP_ACT spact;

  // sanity check then cleanup any rejected suggestions
  if (core == NULL) 
    return JSP_ERR;
  add = NULL;                                    // deleted elsewhere
  ClearLast();
  unk = (core->vc).Confused();
  if ((alist == NULL) || (*alist == '\0'))               
  {
    if (*unk != '\0')
      return unk_tag(unk);                       // unknown word
    return huh_tag();                            // misheard utterance
  }

  // generate core interpretation (remains in "bulk")
  spact = Assemble(alist);

  // handle superficial speech acts
  if (HasSlot(alist, "HELLO"))         // simple greeting
    return greet_tag();
  if (HasSlot(alist, "BYE"))           // simple farewell
    return farewell_tag();
  if (HasSlot(alist, "ATTN"))          // calling robot name
    return hail_tag();

  // look for naked kudo phrases (always believes user)
  if ((main = feedback(spact, alist)) != NULL)
  {
    atree->AddFocus(main);
    return JSP_FACT;
  }

  // check for new rule or operator
  if ((spact == JSP_RULE) || (spact == JSP_OP))
    return add_tag(spact, alist, sent);       
  if (bulk == NULL)                              // needed for other spacts
    return huh_tag();                   

  // encapsulate sequence in a try/catch play (even single cmd and plays)
  main = new jhcAliaChain;
  pod = new jhcAliaPlay;                       
  main->BindPlay(pod);
  pod->AddReq(bulk);
  pod->SetModel(spact);                          // mark for speculation

  // add trailing user response (might change interpretation)
  if (spact == JSP_YNQ)
    append_ynq(main, *atree);
  else if (spact == JSP_WHQ) 
    append_whq(main, *atree);
  else if (spact == JSP_EXQ)
    append_exist(main, *atree);
  else if (spact == JSP_FIND)
    append_find(main, *atree);

  // add leading speech act 
  if ((spact == JSP_FACT) || (spact == JSP_CMD) || (spact == JSP_YNQ) || 
      (spact == JSP_WHQ)  || (spact == JSP_EXQ) || (spact == JSP_FIND))
    return attn_tag(spact, main, alist);         // fact or command or question
  if (spact == JSP_REV)
    return rev_tag(spact, main, alist);          // operator revision
  return huh_tag();                              // should never get here
}


//= Generate a NOTE directive expressing user opinion of current performance.
// looks for standalone kudos: HQ, HQ AKO, and AKO
// as well as possibly embedded kudos: ACC, REJ, YES, and NO

jhcAliaChain *jhcNetBuild::feedback (JSP_ACT spact, const char *alist) const
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
           ((spact == JSP_CMD) || (spact == JSP_OP)))
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

void jhcNetBuild::Summarize (FILE *log, const char *sent, int nt, JSP_ACT spact) const
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
  if ((spact == JSP_HAIL) || (spact == JSP_HI) || (spact == JSP_BYE))
    fprintf(log, "-- %s --\n\n", sp_desc[spact]);
  else if (((spact == JSP_FACT) || (spact == JSP_CMD) || (spact == JSP_REV) ||
            (spact == JSP_YNQ)  || (spact == JSP_WHQ) || (spact == JSP_EXQ) || (spact == JSP_FIND))
           && (bulk != NULL))
  {
    fprintf(log, "-- %s --\n\n", sp_desc[spact]);
    bulk->Save(log);
    fprintf(log, "\n");
  }
  else if ((spact == JSP_OP) && (add != NULL) && (add->new_oper != NULL))
  {
    (add->new_oper)->Save(log); 
    fprintf(log, "\n");
  }
  else if ((spact == JSP_RULE) && (add != NULL) && (add->new_rule != NULL))
  {
    (add->new_rule)->Save(log);
    fprintf(log, "\n");
  }
  else if (nt > 0)
    fprintf(log, "-- nothing --\n\n");
}


///////////////////////////////////////////////////////////////////////////
//                            User Responses                             //
///////////////////////////////////////////////////////////////////////////

//= Adds actions to announce verdict for a yes/no question.

void jhcNetBuild::append_ynq (jhcAliaChain *main, jhcNodePool& pool) const
{
  main->cont = tell_step("affirm", pool);
  main->alt  = tell_step("deny",   pool);
  main->fail = tell_step("pass",   pool);
}


//= Adds action for when telling answer to wh- question fails.

void jhcNetBuild::append_whq (jhcAliaChain *main, jhcNodePool& pool) const
{
  jhcAliaChain *seq, *end, *resp;
  jhcAliaPlay *guard;
  jhcAliaDir *find, *dir;
  jhcNetNode *focus, *tell, *dest;
  jhcGraphlet key;

  // get free variable of final FIND 
  if ((find = main->GetDir()) == NULL)
  {
    if ((guard = main->GetPlay()) == NULL)
      return;
    if ((seq = guard->ReqN(0)) == NULL)
      return;
    if ((end = seq->Last()) == NULL)
      return;
    if ((find = end->GetDir()) == NULL)
      return;
  }
  if (find->kind != JDIR_FIND)
    return;
  focus = find->KeyMain();

  // generate guts for a DO directive to tell about node found
  dir = new jhcAliaDir(JDIR_DO);
  pool.BuildIn(&(dir->key));
  tell = pool.MakeAct("tell");
  tell->AddArg("obj", focus);
  dest = pool.AddProp(tell, "dest", "to");
  dest->AddArg("ref", (core->atree).Human());
  pool.BuildIn(NULL);

  // connect "tell" and failure steps
  resp = new jhcAliaChain;
  resp->BindDir(dir);
  main->cont = resp;
  main->fail = tell_step("pass", pool);
}


//= Adds action for confirming or denying the existence of something.

void jhcNetBuild::append_exist (jhcAliaChain *seq, jhcNodePool& pool) const
{
  seq->cont = tell_step("affirm", pool);
  seq->fail = tell_step("deny", pool);
}


//= Adds action for confirming shift of attention to specified object.

void jhcNetBuild::append_find (jhcAliaChain *seq, jhcNodePool& pool) const
{
  seq->cont = tell_step("confirm", pool);
  seq->fail = tell_step("apologize", pool);           
}


//= Make a step consisting of a DO directive having a verb with no arguments.

jhcAliaChain *jhcNetBuild::tell_step (const char *verb, jhcNodePool& pool) const
{
  jhcAliaChain *step;
  jhcAliaDir *dir;
  jhcGraphlet *old;

  // build a DO directive embedded in a step
  step = new jhcAliaChain;
  dir  = new jhcAliaDir(JDIR_DO);
  step->BindDir(dir);
  
  // flesh out directive with given action
  old = pool.BuildIn(dir->key);
  pool.MakeAct(verb);
  pool.BuildIn(old);
  return step;
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

JSP_ACT jhcNetBuild::huh_tag () const
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
  return JSP_NONE;
}


//= Generate speech act noting that the robot's name was called.

JSP_ACT jhcNetBuild::hail_tag () const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = build_tag(NULL, "hail", NULL, 0);

  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return JSP_HAIL;
}


//= Generate speech act noting that the user wants to communicate.

JSP_ACT jhcNetBuild::greet_tag () const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = build_tag(NULL, "greet", NULL, 0);

  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return JSP_HI;
}


//= Generate speech act noting that the user is leaving.

JSP_ACT jhcNetBuild::farewell_tag () const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = build_tag(NULL, "dismiss", NULL, 0);

  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return JSP_BYE;
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

JSP_ACT jhcNetBuild::unk_tag (const char *word) const
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
  return JSP_WORD;
}


//= Generate speech act followed by a request to add rule or operator.
// save core of ADD directive in "add" for convenience

JSP_ACT jhcNetBuild::add_tag (JSP_ACT spact, const char *alist, const char *sent) 
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch, *steps, *tail, *main;
  jhcAliaPlay *pod;
  jhcNetNode *input, *item;
 
  // make a new NOTE directive for speech act
  ch = build_tag(&input, "give", alist, 1);
  item = atree->MakeNode((spact == JSP_RULE) ? "rule" : "op");
  input->AddArg("obj", item);
  atree->AddProp(item, "ako", ((spact == JSP_RULE) ? "rule" : "operator"));

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

  // move newly create rule or operator into directive (in case slow)
  if (spact == JSP_RULE)
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

  // encapsulate single command in play (helps speculative OP inference)
  main = new jhcAliaChain;
  pod = new jhcAliaPlay;            
  main->BindPlay(pod);
  pod->AddReq(steps);

  // combine with preamble and transfer structure to attention buffer
  tail->cont = main;
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return spact;
}


//= Insert NOTE directive about source of command or fact before actual statement.
// gives the opportunity to PUNT and disbelieve fact or reject command

JSP_ACT jhcNetBuild::rev_tag (JSP_ACT spact, jhcAliaChain *main, const char *alist) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch, *tail;
  jhcNetNode *input, *item;

  // make a new NOTE directive for speech act
  ch = build_tag(&input, "revise", alist, -1);
  item = atree->MakeNode("op");
  input->AddArg("obj", item);
  atree->AddProp(item, "ako", "operator");

  // possibly tack on user feedback ("yes" or "no") after speech act
  if ((tail = feedback(spact, alist)) != NULL)
    ch->cont = tail;
  else
    tail = ch;

  // explain any failure at end
  tail->cont = main;
  main->cont = ack_meta(item);
  main->fail = exp_fail(item);

  // add completed structure to attention buffer
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return JSP_REV;
}


//= Insert NOTE directive about source of command or fact before actual statement.
// gives the opportunity to PUNT and disbelieve fact or reject command

JSP_ACT jhcNetBuild::attn_tag (JSP_ACT spact, jhcAliaChain *main, const char *alist) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch, *tail;
  jhcNetNode *input, *item;
  bool query = (spact != JSP_FACT) && (spact != JSP_CMD);

  // leading NOTE: question = "ask act", command = "tell act", fact = "tell obj"
  ch = build_tag(&input, (query ? "ask" : "tell"), alist, 1);
  if (spact == JSP_FACT)
  {
    item = atree->MakeNode("fact");
    input->AddArg("obj", item);
  }
  else
  {
    item = atree->MakeNode("plan");
    input->AddArg("act", item);
  }

  // possibly tack on user feedback ("yes" or "no") after speech act
  if ((tail = feedback(spact, alist)) != NULL)
    ch->cont = tail;
  else
    tail = ch;

  // explain any failure at end (except for facts and find requests)
  tail->cont = main;
  if ((spact != JSP_FACT) && (spact != JSP_FIND))
    main->fail = exp_fail(item);                    

  // add completed structure to attention buffer
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
  jhcNetNode *n, *iobj;
 
  // fill in details of the speech act
  atree->BuildIn(dir->key);
  n = atree->MakeAct(fcn);
  n->AddArg("agt", atree->Human());  
  if (dest == 0)
    n->AddArg("obj", atree->Robot());  
  else if (dest > 0)
  {
    iobj = atree->AddProp(n, "dest", "to");
    iobj->AddArg("ref", atree->Robot());    
  }         
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


//= Signal to user that new or revised rule or operator was successfully incorporated.

jhcAliaChain *jhcNetBuild::ack_meta (jhcNetNode *item) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = new jhcAliaChain;
  jhcAliaDir *dir = new jhcAliaDir(JDIR_DO);
  jhcNetNode *act;
 
  ch->BindDir(dir);
  atree->BuildIn(dir->key);
  act = atree->MakeAct("acknowledge");
  act->AddArg("obj", item);
  atree->BuildIn(NULL);
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
  prob = atree->MakeAct("fail", 0, 1.0, 1);
  prob->AddArg("act", plan);
  exp->AddArg("obj", prob);
  atree->BuildIn(NULL);
  ch->BindDir(cry);
  return ch;
}


//= Announce that the command (spact = 2) has completed successfully.
// Note: disrupts normal conversation, better for long tasks (e.g. wander)

jhcAliaChain *jhcNetBuild::ann_done (jhcNetNode *plan) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = new jhcAliaChain;
  jhcAliaDir *win = new jhcAliaDir(JDIR_DO);
  jhcNetNode *ann, *done;

  atree->BuildIn(win->key);
  ann = atree->MakeAct("announce");
  done = atree->MakeAct("succeed", 0, 1.0, 1);
  done->AddArg("act", plan);
  ann->AddArg("obj", done);
  atree->BuildIn(NULL);
  ch->BindDir(win);
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
        while ((*end != '\0') && (strchr("\n\r\x0A", *end) == NULL)) 
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

