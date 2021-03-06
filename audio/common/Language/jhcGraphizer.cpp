// jhcGraphizer.cpp : turns parser alist into network structures
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

#include "Action/jhcAliaCore.h"        // common robot - only spec'd as class in header
#include "Action/jhcAliaPlay.h"

#include "Language/jhcMorphTags.h"     // common audio
#include "Language/jhcNetRef.h"        

#include "Language/jhcGraphizer.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcGraphizer::~jhcGraphizer ()
{
  ClearLast();
}


//= Default constructor initializes certain values.

jhcGraphizer::jhcGraphizer ()
{
  // system interface
  core = NULL;

  // dereferencing
  univ = NULL;
  skolem = NULL;
  create = 0;
  resolve = 0;

  // outputs
  rule = NULL;
  oper = NULL;
  dbg = 0;
//dbg = 1;             // to see call sequence for failed conversion
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Cleanup any rejected suggestions.

void jhcGraphizer::ClearLast ()
{
  delete rule;
  rule = NULL;
  delete oper;
  oper = NULL;
}


//= Build an appropriate structure based on given association list.
// return: 5 = op, 4 = rule, 3 = question, 2 = command, 1 = fact, 
//         0 = nothing, negative for error

int jhcGraphizer::Assemble (const char *alist)
{
  char body[amax], head[80] = "";
  int spact = 0;                       // no network created

  // sanity check 
  if (core == NULL) 
    return -1;
  if (alist == NULL)
    return 0;

  // determine if a full item has been found
  if (SplitFrag(head, body, alist) != NULL)
  {
    if (strcmp(head, "%Attn") == 0)
      spact = cvt_attn(body);
    else if (strcmp(head, "%Rule") == 0)
      spact = cvt_rule(body);
    else if (strcmp(head, "%Operator") == 0)
      spact = cvt_op(body);
  }

  // cleanup
  univ = NULL;
  skolem = NULL;
  return spact; 
}


///////////////////////////////////////////////////////////////////////////
//                            Attention Items                            //
///////////////////////////////////////////////////////////////////////////

//= Interpret association list to build an attention item.
//   %Attn -> chain (!dir or %play) or fact
// "bulk" member variable holds resulting jhcAliaChain
// returns 1 (fact) or 2 (command) or 3 (question) if successful, 0 for failure

int jhcGraphizer::cvt_attn (const char *alist) 
{
  char head[80], body[amax];
  jhcWorkMem *wmem = &(core->atree);
  jhcAliaChain *ch;
  jhcAliaDir *dir;

  // solve references against WMEM
  if (SplitFrag(head, body, alist) == NULL)
    return 0;
  univ = wmem;
  resolve = 0;

  // CHAIN - see if some sort of complex command (or question)
  if ((*head == '!') || (strcmp(head, "%play") == 0))
  {
    create = 0;
    if ((bulk = build_chain(alist, NULL, *wmem)) == NULL)
      return 0;
    if (strcmp(head, "!chk-t") == 0)
      return append_ynq(bulk, *wmem);
    if (strcmp(head, "!find-t") == 0)
      return append_whq(bulk, *wmem);
    return 2;
  }

  // FACT - build a single NOTE encapsulating factual assertion
  create = 1;
  dir = new jhcAliaDir;
  wmem->BuildIn(&(dir->key));
  if (build_fact(NULL, body, *wmem) == NULL)                   
  {
    // cleanup from error
    delete dir;
    wmem->BuildIn(NULL);
    return 0;
  }

  // embed NOTE in chain step
  (dir->key).MainProp();
  ch = new jhcAliaChain;
  ch->BindDir(dir);

  // prepend FINDs (if any)
  if (skolem != NULL)
    bulk = skolem->Append(ch);
  else
    bulk = ch;
  return 1;
}


//= Adds actions to announce verdict for a yes/no question.
// assumes last directive in sequence is the main CHK
// returns 3 if successful, 0 if last directive is not a CHK

int jhcGraphizer::append_ynq (jhcAliaChain *seq, jhcNodePool& pool) const
{
  jhcAliaChain *chk;

  // make sure last directive in sequence is a CHK
  if (seq != NULL) 
    if ((chk = seq->Last()) != NULL)
      if (chk->StepDir(JDIR_CHK))
      {
        // attach normal, alt, and fail continuations
        chk->cont = tell_step("affirm", pool);
        chk->alt  = tell_step("deny",   pool);
        chk->fail = tell_step("pass",   pool);
        return 3;
      }
  return 0;
}


//= Adds action for when telling answer to wh- question fails.
// assumes last directive in sequence is the main FIND
// always returns 3 for convenience

int jhcGraphizer::append_whq (jhcAliaChain *seq, jhcNodePool& pool) const
{
  jhcAliaChain *find;

  // no fail branch needed if no FIND in chain
  if (seq != NULL) 
    if ((find = seq->Penult()) != NULL)
      if (find->StepDir(JDIR_FIND))
        find->fail = tell_step("pass", pool);
  return 3;
}


//= Make a step consisting of a DO directive having a verb with no arguments.

jhcAliaChain *jhcGraphizer::tell_step (const char *verb, jhcNodePool& pool) const
{
  jhcAliaChain *step;
  jhcAliaDir *dir;
  jhcGraphlet *old;

  // build a DO directive embedded in a step
  step = new jhcAliaChain;
  dir  = new jhcAliaDir(JDIR_DO);
  step->BindDir(dir);
  
  // flesh out directive with given action
  old = pool.BuildIn(&(dir->key));
  pool.MakeNode("act", verb);
  pool.BuildIn(old);
  return step;
}


///////////////////////////////////////////////////////////////////////////
//                                Rules                                  //
///////////////////////////////////////////////////////////////////////////

//= Interpret association list to build a new rule.
//   %Rule -> $macro or [$cond $res] or [$cond-i $res-i] or [$cond-s $res-s] or [$res $cond]
// "rule" member variable holds result
// returns 4 if successful, 0 for failure

int jhcGraphizer::cvt_rule (const char *alist) 
{
  CallList(1, "cvt_rule", alist);

  // make a new rule
  rule = new jhcAliaRule;
  univ = rule;
  create = 1;
  resolve = 1;

  // determine which pattern was used
  if (build_fwd(*rule, alist)  || build_rev(*rule, alist)  ||
      build_ifwd(*rule, alist) || build_sfwd(*rule, alist) ||
      build_macro(*rule, alist))
  {
    // make sure result has actual beliefs (not all 0.0)
    (rule->result).ActualizeAll(0);
    return 4;
  }

  // cleanup from failure
  delete rule;
  rule = NULL;
  return 0;
}


//= Interpret association list where condition precedes result.

bool jhcGraphizer::build_fwd (jhcAliaRule& r, const char *alist) 
{
  char body[amax];
  const char *tail;

   // assemble condition part
  if ((tail = ExtractBody("$cond", body, alist)) == NULL)
    return false;
  CallList(1, "build_fwd", alist);
  if (build_sit(r, body) <= 0)
    return false;

  // assemble result part
  if (ExtractBody("$res", body, tail) == NULL)
    return false;
  if (build_graph(r.result, body, r) <= 0)
    return false;
  return true;
}


//= Interpret association list where result precedes condition.

bool jhcGraphizer::build_rev (jhcAliaRule& r, const char *alist)
{
  char body[amax];
  const char *tail;

  // assemble result part
  if ((tail = ExtractBody("$res", body, alist)) == NULL)
    return false;
  CallList(1, "build_rev", alist);
  if (build_graph(r.result, body, r) <= 0)
    return false;

  // assemble condition part
  if (ExtractBody("$cond", body, tail) == NULL)
    return false;
  if (build_sit(r, body) <= 0)
    return false;
  return true;
}


//= Interpret association list starting with an indefinite condition.

bool jhcGraphizer::build_ifwd (jhcAliaRule& r, const char *alist) 
{
  char next[200], body[amax];
  jhcNetNode *obj, *arg;
  const char *tail, *val;

  // look for proper condition type
  if ((tail = ExtractBody("$cond-i", body, alist)) == NULL)
    return false;
  CallList(1, "build_ifwd", alist);

  // assemble condition part  
  r.BuildIn(&(r.cond));
  if ((obj = build_obj(NULL, body, r)) == NULL)            
  {
    // ascribe property to unknown subject ("orange ... is a color")
    if (FragNextPair(body, next) == NULL)
      return false;
    if ((val = SlotGet(next, "HQ")) == NULL)
      return false;
    arg = r.MakeNode("obj");
    obj = r.AddProp(arg, "hq", val);
  }
  (r.cond).MainProp();

  // assemble result paty
  if (ExtractBody("$res-i", body, tail) == NULL)
    return false;
  r.BuildIn(&(r.result));
  if (add_cop(NULL, obj, body, r) == NULL)
    return false;
  (r.result).MainProp();
  return true;
}


//= Interpret association list starting with an indefinite plural condition.

bool jhcGraphizer::build_sfwd (jhcAliaRule& r, const char *alist) 
{
  char body[amax];
  jhcNetNode *obj;
  const char *tail;

  // assemble condition part (no naked properties)
  if ((tail = ExtractBody("$cond-s", body, alist)) == NULL)
    return false;
  CallList(1, "build_sfwd", alist);
  r.BuildIn(&(r.cond));
  if ((obj = build_obj(NULL, body, r)) == NULL)                
    return false;
  (r.cond).MainProp();

  // assemble result part
  if (ExtractBody("$res-s", body, tail) == NULL)
    return false;
  r.BuildIn(&(r.result));
  if (build_fact(NULL, body, r, obj) == NULL)                  
    return false;
  (r.result).MainProp();
  return true;
}


//= Make rule for pattern "X means Y".
// returns 1 if successful, 0 for failure

bool jhcGraphizer::build_macro (jhcAliaRule& r, const char *alist) const
{
  char pair[200], pair2[200], body[amax];
  jhcNetNode *n;
  const char *tail, *wd, *wd2;

  // look for proper condition type
  if (ExtractBody("$macro", body, alist) == NULL)
    return false;
  CallList(1, "build_macro", alist);

  // get two lexical terms to be related
  if ((tail = FragNextPair(body, pair)) == NULL)
    return 0;
  if ((wd = SlotGet(pair)) == NULL)
    return 0; 
  if (FragNextPair(tail, pair2) == NULL)
    return 0;
  if ((wd2 = SlotGet(pair2)) == NULL)
    return 0; 

  // create rule structure involving two "lex" properties
  r.BuildIn(&(r.cond));
  n = rule->MakeNode("sub", wd);
  r.BuildIn(&(r.result));
  r.AddLex(n, wd2);
  return 1;
}


//= Create a single graphlet out of one or more facts.
// used by the result part of rules (set BuildIn before calling)
// returns 1 if successful, 0 or negative for problems

int jhcGraphizer::build_graph (jhcGraphlet& gr, const char *alist, jhcNodePool& pool)
{
  char body[amax], head[80] = "";
  const char *tail = alist;
  int must = 0;

  CallList(1, "build_graph", alist);

  pool.BuildIn(&gr);
  while ((tail = SplitFrag(head, body, tail)) != NULL)
    if (strncmp(head, "%fact", 5) == 0)
    {
      jprintf(1, dbg, "-- ASSERT %d\n", ++must);
      if (build_fact(NULL, body, pool) == NULL)                
        return 0;
    }
  gr.MainProp();
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Operators                                //
///////////////////////////////////////////////////////////////////////////

//= Interpret association list to build a new operator.
//   %Operator -> [$trig $proc] or [$trig-n $proc]
// "oper" member variable holds result
// returns 5 if successful, 0 for failure

int jhcGraphizer::cvt_op (const char *alist) 
{
  char body[amax];
  const char *t2, *tail = alist;

  CallList(1, "cvt_op", alist);

  // try to create correct kind of operator (handles $trig-n)
  if ((oper = config_op(alist)) == NULL)
    return 0;
  univ = oper;

  // allow non-local exit
  while (1)
  {
    // fill in trigger from beginning clause (if any)
    create = 1;
    resolve = 1;
    if ((tail = ExtractBody("$trig", body, tail, 1)) == NULL)
      tail = alist;
    else if (build_sit(*oper, body, oper->KindTag()) <= 0)
      break;

    // fill in procedure (required unless this is a prohibition)
    create = 0;
    resolve = 0;
    if ((t2 = ExtractBody("$proc", body, tail)) != NULL)
    {
      if ((oper->meth = build_chain(body, oper->meth, *oper)) == NULL)
        break;      
      tail = t2;
    }
    if (oper->meth == NULL)            // prohibition will have PUNT
      break;

    // add to trigger from ending clause (if any)
    create = 1;
    resolve = 1;
    if ((tail = ExtractBody("$trig", body, tail)) != NULL)
      if (build_sit(*oper, body, oper->KindTag()) <= 0)
        break;

    // make sure some trigger was found
    if (((oper->cond).NumItems() <= 0) && (oper->nu <= 0))
      break;
    return 5;
  }

  // cleanup from some problem
  delete oper;
  oper = NULL;
  return 0;
}


//= Create a new operator with appropriate type of trigger condition (blank for now).
// if trigger is a prohibition ($trig-n) then cond type will be ANTE and default PUNT chain is added
// returns new operator or NULL for problem

jhcAliaOp *jhcGraphizer::config_op (const char *alist) const
{
  char entry[80];
  jhcAliaOp *op;
  const char *tail;
  int k = JDIR_NOTE, veto = 0;

  CallList(1, "config_op", alist, -1);

  // make sure operator has a triggering condition (and possibly a second)
  if (FindFrag(alist, "$trig-n") != NULL)
    veto = 1;
  tail = kind_op(k, alist, veto);
  kind_op(k, tail, veto);

  // create operator of proper kind and adjust preference
  op = new jhcAliaOp((JDIR_KIND) k);
  if (FindSlot(alist, "PREF", entry) != NULL)
    op->pref = pref_val(entry);

  // possibly add final PUNT directive for prohibitions
  if (veto > 0)
    op->meth = dir_step("punt");
  return op;
}


//= Determine a triggering directive type for this operator.
// changes value in "k" depending on what is found (no change if nothing definitive found)
// returns rest of alist if some determination made, NULL if some problem

const char *jhcGraphizer::kind_op (int& k, const char *alist, int veto) const
{
  char body[amax], head[80] = "";
  jhcAliaDir dcvt;
  const char *t2 = body, *tail = alist;

  CallList(1, "kind_op", alist, -1);

  // look for next trigger fragment
  while (strncmp(head, "$trig", 5) != 0)
    if ((tail = SplitFrag(head, body, tail)) == NULL)
      return NULL;

  // check specially for ANTE and POST operators first
  if (FragHasSlot(body, "BEFORE"))
    k = JDIR_ANTE;
  else if (FragHasSlot(body, "AFTER"))
    k = JDIR_POST;
  else
    while ((t2 = FragNextFrag(t2, head)) != NULL)
      if (*head == '!')
      {
        // otherwise directive type depends first command found 
        if ((k = dcvt.CvtKind(head + 1)) >= JDIR_MAX)
          return NULL;
        if ((k == JDIR_DO) && (veto > 0))            // prohibitions are ANTE
          k = JDIR_ANTE;                     
        break;
      }
  return tail;
}


//= Assemble a situation description with AND and UNLESS parts.
//   situation -> %fact or %fact-i (or possibly cmd like !do for op trigger)
// needs to interpret clauses sequential to resolve pronouns
// used for preconditions of both rules and operators (ktag != NULL)
// returns 1 if successful, 0 or negative if error

int jhcGraphizer::build_sit (jhcSituation& sit, const char *alist, const char *ktag) 
{
  char head[80], body[amax];
  jhcNetNode *cmd = NULL;
  const char *tail = alist;
  int cond, neg, prima, must = 0;

  CallList(1, "build_sit", alist);

  // conditions can be commands or facts
  while (1)
  {
    // check for "unless" before next getting clause
    if (NextEntry(tail, body) == NULL)
      break;
    cond = ((SlotMatch(body, "UNLESS")) ? 0 : 1);
    if ((tail = SplitFrag(head, body, tail)) == NULL)
      break;
    neg = ((HasSlot(body, "NEG")) ? 1 : 0);

    // determine type of clause
    if ((*head == '!') && (ktag != NULL))
    {
      // triggering condition (only for ops with ktag != NULL)
      jprintf(1, dbg, "-- %s\n", ktag);
      sit.BuildCond();
      if ((cmd = build_cmd(head, body, sit)) == NULL)
        return 0;
      sit.CmdHead(cmd);
      jprintf(1,dbg, "----\n\n");
    }
    else if (strncmp(head, "%fact", 5) != 0)
      continue;
    else if ((cond <= 0) || (neg > 0))
    {
      // prima facie absent ("unless" or "and not")
      if ((prima = sit.BuildUnless()) <= 0)
        return 0;
      jprintf(1, dbg, "-- UNLESS %d\n", prima);      
      if (build_fact(NULL, body, sit, NULL, cond) == NULL)     
        return 0;
      sit.UnlessHead();
      jprintf(1,dbg, "----\n\n");
    }
    else
    {
      // required condition
      sit.BuildCond();
      jprintf(1, dbg, "-- CONJUNCT %d\n", ++must);
      if (build_fact(NULL, body, sit, 0) == NULL)              
        return 0;
      jprintf(1, dbg, "----\n\n");
    }
  }

  // make sure a relation is the start of the cond graphlet
  if (cmd == NULL)
    sit.PropHead();
  return 1;
}


//= Turn qualifier ("could maybe") into numeric belief value.
// note that "could" is at end to give preference to "could maybe"

double jhcGraphizer::pref_val (const char *word) const
{
  char term[7][20] = {"must", "always", "definitely", "probably", "might", "maybe", "could"};
  double val[7]    = {  1.5,    1.3,        1.2,         0.8,       0.6,     0.3,     0.5  };
  int i;

  for (i = 0; i < 7; i++)
    if (strstr(word, term[i]) != NULL)
      return val[i];
  return 1.0;
} 


///////////////////////////////////////////////////////////////////////////
//                           Command Sequences                           //
///////////////////////////////////////////////////////////////////////////

//= Create a chain of activities, some sequential others potentially parallel.
// will append "ult" activity (can be NULL) to full chain built 
// returns NULL if some problem

jhcAliaChain *jhcGraphizer::build_chain (const char *alist, jhcAliaChain *ult, jhcNodePool& pool) 
{
  char entry[200] = "";
  jhcGraphlet temp;
  jhcAliaPlay *play = NULL;
  jhcAliaChain *ch0, *mini, *pod = NULL, *start = NULL, *ch = NULL;
  jhcAliaDir *dir = NULL;
  jhcNetNode *cmd;
  const char *tail = alist;

  CallList(1, "build_chain", alist);

  // handle sequence of actions in chain
  while ((tail = NextFrag(tail, entry)) != NULL)
    if (strcmp(entry, "%play") == 0)
    {
      // make the next chain step be a new play
      pod = new jhcAliaChain;
      play = new jhcAliaPlay;
      pod->BindPlay(play);
      if (ch == NULL)
        start = pod;
      else
        ch->cont = pod;  
      ch = NULL;
    }
    else if ((strcmp(entry, "%") == 0) && (play != NULL))
    {
      // finish off any current play
      ch = pod;
      play = NULL; 
    }
    else if (*entry == '!')
    {
      // make up new chain step which is a single directive
      ch0 = ch;
      pool.BuildIn(&temp);
      if ((ch = dir_step(entry + 1)) == NULL)
        break;
      dir = ch->GetDir();
      skolem = NULL;

      // complete action spec, set dir to NULL if success 
      pool.BuildIn(&(dir->key));
      if ((cmd = build_cmd(entry, tail, pool)) == NULL)
        break;
      (dir->key).SetMain(cmd);
      pool.BuildIn(NULL);
      dir = NULL;

      // prepend any generated FINDs (ch always at end)
      if (skolem == NULL)
        mini = ch;
      else if (strcmp(entry, "!find") != 0)
        mini = skolem->Append(ch);
      else
      {
        // throw away any partial FIND directive (skolem is more complete)
        delete ch;
        mini = skolem;
        ch = skolem->Last();
      }
      skolem = NULL;                   // re-initialize
      
      // add either as a required activity or tack onto end of chain
      if (play != NULL)
        play->AddReq(mini);
      else if (ch0 == NULL)
        start = mini;
      else
        ch0->cont = mini;  
      tail = FragClose(tail, 0);
    }

  // check for success 
  if ((dir == NULL) && (ch != NULL))
  {
    ch->cont = ult;
    return start;
  }

  // cleanup (chain automatically deletes payload)
  delete start;                       
  return NULL;
}


//= Create a new chain step consisting of a directive with some kind.

jhcAliaChain *jhcGraphizer::dir_step (const char *kind) const
{
  jhcAliaDir *dir;
  jhcAliaChain *ch;

  // make up new directive of proper kind based on beginning of string
  dir = new jhcAliaDir;
  if (strcmp(kind, "find-t") == 0)     // wh-question needs a tell at end
    dir->SetKind("do");           
  else if (dir->SetKind(kind) <= 0)
  {
    delete dir;
    return NULL;
  }

  // embed directive in new chain step
  ch = new jhcAliaChain;
  ch->BindDir(dir);
  return ch;
}


//= Fill in details of directive from remaining association list.
//    cmd -> !do or !chk or !chk-t or !find or !find-t
// returns main action node if successful, NULL for problem

jhcNetNode *jhcGraphizer::build_cmd (const char *head, const char *alist, jhcNodePool& pool)
{
  char body[amax];
  jhcNetNode *focus, *main;
  const jhcAliaChain *find;
  jhcAliaDir *dir;

  CallList(1, "build_cmd", alist, 0, head);

  // possibly convert question "X?" to command "Tell me X"
  if (strcmp(head, "!find-t") == 0)
  {
    // build_chain automatically pre-pends one or more FINDs using "skolem"
    if ((focus = build_query(alist, pool)) == NULL)
      return NULL;
    if (skolem != NULL)
      if ((find = skolem->Last()) != NULL)
        if ((dir = find->GetDir()) != NULL)
          dir->fass = 0;                                   // don't allow assumption

    // generate guts for a DO directive to tell about node found
    main = pool.MakeNode("act", "tell");
    main->AddArg("dest", (core->atree).Human());
    main->AddArg("obj", focus);
    return main;
  }

  // try building structure for other directives
  if (strncmp(head, "!do", 3) == 0)                       
    return build_do(alist, pool);   
  if (strncmp(head, "!chk", 4) == 0)                       // includes "!chk-t"
    if (ExtractBody("%fact", body, alist) != NULL) 
      return build_fact(NULL, body, pool);                 
  if (strncmp(head, "!find", 5) == 0)
    return build_query(alist, pool);
  return NULL;
}


//= Build structures for various types of wh-questions and return focal node.
//  query -> np or $q-ako or $q-hq or $q-lex or $q-loc or $q-cnt

jhcNetNode *jhcGraphizer::build_query (const char *alist, jhcNodePool& pool) 
{
  jhcNetRef nr(univ, (core->atree).MinBlf());
  char head[80], entry[200], body[amax];
  jhcNetNode *obj, *main;
  const char *t2, *tail, *kind = NULL;

  CallList(1, "build_query", alist);

  // figure out what type of question this is (object vs. property)
  if (SplitFrag(head, body, alist) == NULL)
    return NULL;
  if (*head != '$')
    return build_obj(NULL, alist, pool, NULL, 0, 0.0);
  tail = body;

  // get constraint on desired answer kind 
  if (strcmp(head, "$q-hq") == 0)                          // required for props
  {
    if ((tail = NextEntry(tail, entry)) == NULL)
      return NULL;
    if ((kind = SlotGet(entry, "AKO")) == NULL)
      return NULL;
  }
  else if (strcmp(head, "$q-ako") == 0)                    // optional for kinds
    if ((t2 = nsuper_kind(entry, 200, tail)) != NULL)
    {
      kind = entry;
      tail = t2;
    }

  // get referent (return it directly if seeking a label)
  if ((obj = build_obj(&t2, tail, pool, NULL, 0, 0.0)) == NULL)              
    return NULL;
  if (strcmp(head, "$q-lex") == 0)
    return obj;

  // add unknown property and possibly constrain its type
  main = nr.AddProp(obj, head + 3, NULL, 0, 0.0);
  if (kind != NULL)
    nr.AddProp(main, "ako", kind, 0, 0.0);                               // was -1.0
  return nr.FindMake(pool, 0, NULL, (core->atree).MinBlf(), &skolem);    // was find 1
}


///////////////////////////////////////////////////////////////////////////
//                             Action Phrases                            //
///////////////////////////////////////////////////////////////////////////

//= Create network structure for imperative verb phrase.
//   !do -> ACT or [ACT ACT-G] or SAY
// expects "do!" is stripped off front already
// returns pointer to newly created action node

jhcNetNode *jhcGraphizer::build_do (const char *alist, jhcNodePool& pool)
{
  char next[200];
  const char *val = NULL, *end = alist, *tail = alist;
  jhcNetNode *act, *iobj;
  UL32 t = 0;      
  int quote = 0, neg = 0;  

  CallList(1, "build_do", alist);

  // check for overall negation of verb
  if (FragHasSlot(tail, "NEG-V") || FragHasSlot(tail, "STOP"))
    neg = 1;

  // look for main verb but ignore placeholder "do something"
  while ((end = FragNextPair(end, next)) != NULL)
    if ((val = mf.VerbLex(t, next)) != NULL)                
      break;
  if (val == NULL)
    return NULL;
  if ((strcmp(val, "do something") == 0) || (strcmp(val, "do anything") == 0))
    val = NULL;

  // see if special "say" type then make node for action
  if (strncmp(next, "SAY", 3) == 0)
    quote = 1;
  act = pool.MakeNode("act", val, neg);
  act->tags = t;

  // attach all adverbial modifiers (could come before verb)
  while ((tail = FragNextPair(tail, next)) != NULL)
    if ((val = SlotGet(next, "DEG")) != NULL)
      tail = act_deg(act, val, tail, pool);
    else if ((val = SlotGet(next, "MOD")) != NULL)
      pool.AddProp(act, "mod", val);
    else if ((val = SlotGet(next, "AMT")) != NULL)
      pool.AddProp(act, "amt", val);
    else if ((val = SlotGet(next, "DIR")) != NULL)
      pool.AddProp(act, "dir", val);

  // add noun-like arguments or quoted string (comes after verb)
  if (quote > 0)
  {
    if ((iobj = build_obj(NULL, end, pool)) != NULL)       // iobj only (FINDs okay)
      act->AddArg("dest", iobj);
    add_quote(act, end, pool);
  }
  else 
  {
    act = add_args(act, end, pool);                        // iobj + dobj
    add_rels(act, end, pool);                              // adverbs
  }
  return act;
}


//= Build a sentence-like semantic network with subject and object(s).
// can optionally take pre-defined subject with alist being rest of statement
// if "pos" > 0 forces interpretation to be positive (i.e. can ignore <NEG> for unless clauses)

jhcNetNode *jhcGraphizer::build_fact (const char **after, const char *alist, jhcNodePool& pool, 
                                      jhcNetNode *subj, int pos)
{
  char word[80], pair[200];
  const char *t2, *post = alist, *tail = alist, *val = NULL;
  jhcNetNode *act = NULL, *agt = subj;
  double blf = 1.0;
  UL32 t = 0;      
  int neg = 0, past = 0;

  CallList(1, "build_fact", alist, 0, ((subj != NULL) ? subj->Nick() : ""));

  // see if copula vs. sentence with verb
  if (HasFrag(alist, "$add")) 
  {
    // build structure for add-on features ("is nice")
    if (agt == NULL)
      if ((agt = build_obj(&tail, tail, pool)) == NULL)
        return NULL;
    act = add_cop(&tail, agt, tail, pool, pos);
    if (after != NULL)
      if ((*after = FragClose(tail, 0)) == NULL) 
        *after = tail + strlen(tail);
    return act;
  }

  // check for overall negation of verb, past tense auxiliary, and belief
  if (pos <= 0)
    if (FragHasSlot(alist, "NEG-V") || FragHasSlot(alist, "NEG"))
      neg = 1;
  if (FragHasSlot(alist, "AUX-D"))
    past = 1;
  if (FindSlot(alist, "BLF", word) != NULL)
    blf = belief_val(word);

  // look for main verb (but also allow naked noun phrase)
  while ((post = FragNextPair(post, pair)) != NULL)
    if ((val = mf.VerbLex(t, pair)) != NULL)                
      break;
  if (val == NULL)
    return build_obj(after, alist, pool);
  if (past > 0)
    t = JTAG_VPAST;

  // make node for sentence using proper belief
  act = pool.MakeNode("act", val, neg, blf);
  if ((t & JTAG_VPAST) != 0)                               // was VPRES = 0
    act->SetDone(1);
  act->tags = t;

  // go back and see if some object at front 
  if (agt == NULL)
    agt = build_obj(&t2, alist, pool);
  if (agt != NULL)
    act->AddArg("agt", agt);

  // attach all adverbial modifiers (anywhere in sentence)
  while ((tail = FragNextPair(tail, pair)) != NULL)
    if ((val = SlotGet(pair, "DEG")) != NULL)
      tail = act_deg(act, val, tail, pool);
    else if ((val = SlotGet(pair, "MOD")) != NULL)
      pool.AddProp(act, "mod", val);
    else if ((val = SlotGet(pair, "AMT")) != NULL)
      pool.AddProp(act, "amt", val);
    else if ((val = SlotGet(pair, "DIR")) != NULL)
      pool.AddProp(act, "dir", val);

  // add noun-like arguments or quoted string (after verb)
  act = add_args(act, post, pool);
  add_rels(act, post, pool);
  if (after != NULL)
    if ((*after = FragClose(post, 0)) == NULL)   // end of enclosing fragment
      *after = post + strlen(post);
  return act;
}


//= Make nodes for adverbial descriptions with a degree ("very slowly").
// returns unused portion of alist

const char *jhcGraphizer::act_deg (jhcNetNode *act, const char *amt, const char *alist, jhcNodePool& pool) const
{
  char pair[200];
  jhcNetNode *prop;
  const char *val, *tail;

  CallList(1, "act_deg", alist, 0, amt);

  // possibly add an adverbial description to node
  if ((tail = FragNextPair(alist, pair)) == NULL)
    return alist;
  if ((val = SlotGet(pair, "MOD")) == NULL)
    return alist;

  // modify the adjectival descriptor (override pool defaults)
  prop = pool.AddProp(act, "mod", val);
  pool.AddProp(prop, "deg", amt);
  return tail;
}


//= Add a node which has a long literal string expansion.
// returns 1 if found, 0 for problem

int jhcGraphizer::add_quote (jhcNetNode *v, const char *alist, jhcNodePool& pool) const
{
  char next[200];
  jhcNetNode *q;
  const char *val, *tail = alist;

  CallList(1, "add_quote", alist, 0, v->Word());

  while ((tail = FragNextPair(tail, next)) != NULL)
    if ((val = SlotGet(next, "QUOTE", 0)) != NULL)
    {
      q = pool.MakeNode("txt");
      q->SetString(val);
      v->AddArg("obj", q);
      return 1;
    }
  return 0;
}


//= Look for direct object (or infinitive) plus indirect object and link them to verb.
// returns embedded infinitive command (if any), else main verb (passed in)

jhcNetNode *jhcGraphizer::add_args (jhcNetNode *v, const char *alist, jhcNodePool& pool)
{
  char entry[200] = "";
  const char *tail;
  jhcNetNode *obj2, *dobj = NULL, *iobj = NULL, *act = NULL;

  // sanity check 
  if (alist == NULL)
    return v;
  CallList(1, "add_args", alist, 0, v->Word());

  // look for first object
  if ((dobj = build_obj(&tail, alist, pool)) != NULL)
    if (*tail != '\0')
    {
      // look for second object (if any)
      if ((obj2 = build_obj(NULL, tail, pool)) != NULL)
      {
        // correct order is iobj then dobj so swap 
        iobj = dobj;
        dobj = obj2;
      }
      else if ((tail = NextFrag(tail, entry)) != NULL)
        if (strcmp(entry, "!do") == 0)
          if ((act = build_do(tail, pool)) != NULL)
          {
            // correct order is iobj then infinitive
            iobj = dobj;
            dobj = NULL;
          }
    }
        
  // attach arguments (if any)
  if (iobj != NULL)
    v->AddArg("dest", iobj);
  if (dobj != NULL)
    v->AddArg("obj", dobj);
  if (act != NULL)
    v->AddArg("cmd", act);
  return((act != NULL) ? act : v);
}


//= Add prepositional phrases modifiers (typically only one) to action.

void jhcGraphizer::add_rels (jhcNetNode *act, const char *alist, jhcNodePool& pool) 
{
  char entry[200] = "";
  const char *t2, *tail = alist;

  // sanity check 
  if ((alist == NULL) || (act == NULL))
    return;
  CallList(1, "add_rels", alist, 0, act->Word());

  // look for PP attached to main verb
  while ((tail = NextFrag(tail, entry)) != NULL)
    if (strcmp(entry, "$rel") == 0)
    {
      // determine type of PP and dispatch
      if ((t2 = FragNextPair(tail, entry)) == NULL)
        continue;
      if (SlotStart(entry, "LOC") > 0)                       
        add_place(&tail, act, entry, t2, pool);
      tail = FragClose(tail);
    }
}


///////////////////////////////////////////////////////////////////////////
//                             Object Phrases                            //
///////////////////////////////////////////////////////////////////////////

//= Create network structure for noun phrase.
// initially creates description in a NodeRef to check if a referent already exists
// can optionally force new description onto an old object in pool by setting "f0"
// spreads negation widely: not a big red dog -> not big & not red & not a dog
// "find": -1 = always create new item (create > 0)
//          0 = always make FIND
//          1 = resolve locally else make FIND
//          2 = resolve locally else create item (resolve > 0)
// returns pointer to newly created object node (and advances alist to after object)

jhcNetNode *jhcGraphizer::build_obj (const char **after, const char *alist, jhcNodePool& pool, 
                                     jhcNetNode *f0, int neg, double blf) 
{
  jhcNetRef nr(univ, (core->atree).MinBlf());
  char next[200] = "";
  const char *val, *tail;
  jhcNetNode *obj, *ref, *fact = NULL;
  UL32 t;
  int find = ((resolve > 0) ? 2 : 1); 

  CallList(1, "build_obj", alist, 1, ((f0 != NULL) ? f0->Nick() : NULL));

  // check if next thing is some sort of question object
  if ((tail = NextEntry(alist, next)) == NULL)
    return NULL;
  if (strncmp(next, "$q-", 3) == 0)
    return build_query(alist, pool);
  if (strcmp(next, "$add") == 0)
  {
    obj = nr.MakeNode("obj");
    if (add_cop(after, obj, alist, nr) == NULL)
      return NULL;
    return nr.FindMake(pool, find, NULL, blf, &skolem);
  } 

  // more standard types of object descriptions
  if (SlotStart(next, "ACT-G") > 0)
    return build_fact(after, alist, pool);
  if (strncmp(next, "%obj", 4) != 0)
    return NULL;
  if (strncmp(next, "%obj-i", 6) == 0)
    find = ((create > 0) ? -1 : 0);                 

  // add features to object node in temporary network 
  obj = nr.MakeNode("obj");
  while ((tail = FragNextPair(tail, next)) != NULL)
    if ((val = SlotGet(next, "REF", 0)) != NULL)               // reference ("you", "she")
      fact = ref_props(obj, nr, val, neg);
    else if ((val = SlotGet(next, "NAME", 0)) != NULL)         // proper noun ("Jim")
      fact = nr.AddLex(obj, val, neg, blf);                 
    else if ((val = mf.NounLex(obj->tags, next)) != NULL)      // base type ("dog") 
      fact = nr.AddProp(obj, "ako", val, neg, blf);                   
    else if ((val = SlotGet(next, "HQ")) != NULL)              // simple property ("big") 
      fact = nr.AddProp(obj, "hq", val, neg, blf);                    
    else if ((val = SlotGet(next, "DEG")) != NULL)             // degree property ("very red")
      fact = obj_deg(&tail, obj, val, tail, nr, neg, blf);            
    else if (SlotStart(next, "ACT-G") > 0)                     // participle ("sleeping")
    {
      fact = nr.AddProp(obj, "agt", mf.VerbLex(t, next), neg, blf, "act");
      fact->tags = t;
    }
    else if (SlotStart(next, "LOC") > 0)                       // location phrase ("at home")
      fact = add_place(&tail, obj, next, tail, nr, neg, blf);           
    else if ((val = SlotGet(next, "HAS")) != NULL)             // part description ("with a red top")
      fact = obj_has(&tail, obj, val, tail, nr, neg, blf);            

  // possibly link to existing node else create new graph
  if (after != NULL)
    *after = FragClose(alist);
  if ((&pool == rule) || (&pool == oper))
    nr.bth = -nr.bth;                                          // allow hypotheticals
  ref = nr.FindMake(pool, find, f0, blf, &skolem);

  // if properties being added to old node, return last such property
  if (f0 == NULL)
    return ref;
  return nr.LookUp(fact);
}


//= Add properties to object node based on pronoun used for reference.
// returns object node for convenience

jhcNetNode *jhcGraphizer::ref_props (jhcNetNode *n, jhcNodePool& pool, const char *pron, int neg) const
{
  // specify conversational role (can be negated) 
  if ((strcmp(pron, "you") == 0) || (strcmp(pron, "me") == 0) || (_stricmp(pron, "I") == 0))
    pool.AddLex(n, pron, neg);
  else if (neg > 0) 
    return n;

  // add extra features as long as not negated
  if ((strcmp(pron, "he") == 0) || (strcmp(pron, "him") == 0))
  {
    pool.AddProp(n, "hq",  "male");
    pool.AddProp(n, "ako", "person");
  }
  else if ((strcmp(pron, "she") == 0) || (strcmp(pron, "her") == 0))
  {
    pool.AddProp(n, "hq",  "female");
    pool.AddProp(n, "ako", "person");
  }
  return n;
}


//= Make nodes for adjectival descriptions with a degree ("very red").
// returns propery assertion, can also give unused portion of alist (after)

jhcNetNode *jhcGraphizer::obj_deg (const char **after, jhcNetNode *obj, const char *amt, const char *alist, 
                                   jhcNodePool& pool, int neg, double blf) 
{
  char pair[200];
  jhcNetNode *prop, *mod;
  const char *val, *tail;

  CallList(1, "obj_deg", alist, 0, amt);

  // figure out what kind of relation is being given a degree
  if (after != NULL)
    *after = alist;
  if ((tail = FragNextPair(alist, pair)) == NULL)
    return NULL;

  // modify a new adjectival descriptor 
  if ((val = SlotGet(pair, "HQ")) != NULL)
  {
    prop = pool.AddProp(obj, "hq", val, neg, blf);
    mod = pool.AddProp(prop, "deg", amt);
    if (after != NULL)
      *after = tail;
    return prop;
  }

  // modify a location descriptor (gets most recent "loc" property)
  if ((val = SlotGet(pair, "LOC")) != NULL)
  {
    prop = add_place(&tail, obj, pair, tail, pool, neg, blf);
    mod = pool.AddProp(prop, "deg", amt);
    if (after != NULL)
      *after = tail;
    return prop;
  }

  // unknown description type
  if (after != NULL)
    *after = tail;
  return NULL;
}


//= Make nodes for location phrases ("at home" or "between here and there").
// can be used with both NPs and VPs
// returns location assertion, can also give unused portion of alist (after)

jhcNetNode *jhcGraphizer::add_place (const char **after, jhcNetNode *obj, char *pair, const char *alist, 
                                     jhcNodePool& pool, int neg, double blf)
{
  jhcNetNode *ref, *prop;
  const char *t2, *tail, *rel = SlotGet(pair, "LOC");

  CallList(1, "add_place", alist, 0, pair);

  // add basic relation
  if (after != NULL)
    *after = alist;
  prop = pool.AddProp(obj, "loc", rel, neg, blf);

  // check if anchor object required (not needed for "here" or "in front")
  if ((SlotStart(pair, "LOC-0") > 0) || (SlotStart(pair, "LOC-V") > 0))
    return prop;
  if ((ref = build_obj(&tail, alist, pool)) == NULL)
    return prop;
  prop->AddArg("wrt", ref);                        // add reference object 
  if (after != NULL)
    *after = tail;

  // check if second anchor expected (e.g. "between")
  if (SlotStart(pair, "LOC-2") <= 0)
    return prop;
  if ((ref = build_obj(&t2, tail, pool)) == NULL)
    return prop;
  prop->AddArg("wrt", ref);                        // add second reference  
  if (after != NULL)
    *after = t2;
  return prop;
}


//= Make nodes for part phrases ("with a red top").
// returns possession relation, can also give unused portion of alist

jhcNetNode *jhcGraphizer::obj_has (const char **after, jhcNetNode *obj, const char *prep, const char *alist, 
                                  jhcNodePool& pool, int neg, double blf) 
{
  jhcNetNode *part, *prop;
  const char *tail;

  CallList(1, "obj_has", alist, 0, prep);

  // check for required part
  if (after != NULL)
    *after = alist;
  if ((part = build_obj(&tail, alist, pool)) == NULL)
    return NULL;

  // build required relation
  prop = pool.AddProp(obj, "has", prep, neg, blf);
  prop->AddArg("obj", part); 
  if (after != NULL)
   *after = tail;
  return prop;
}


//= Check for copula tail end (e.g. "is nice") and add features to node.
// features added directly since never need to check for a reference for this description
// if "pos" > 0 forces interpretation to be positive (i.e. can ignore <NEG> for unless clauses)
// returns last assigned property, can also give unused portion of alist (after)

jhcNetNode *jhcGraphizer::add_cop (const char **after, jhcNetNode *obj, const char *alist, jhcNodePool& pool, int pos) 
{
  char next[200], body[amax];
  jhcNetNode *f2, *fact = NULL;
  const char *val, *post, *tail;
  double blf = 1.0;                     // was 0.0
  UL32 t;
  int neg = 0, cr0 = create;  

  CallList(1, "add_cop", alist, 0, obj->Nick());

  // if following part is an addition then get first pair
  if ((tail = SplitFrag(next, body, alist)) == NULL)
    return NULL;
  if (after != NULL)
    *after = tail;
  tail = body;
  post = body;

  // go through all the pairs in this fragment
  while ((tail = FragNextPair(tail, next)) != NULL)
  {
    if ((val = SlotGet(next, "BLF")) != NULL)                    // overall belief ("usually")
      blf = belief_val(val);                             
    else if ((SlotStart(next, "NEG") > 0) && (pos <= 0))         // overall negation ("not")
      neg = 1;                                           
    else if ((val = SlotGet(next, "NAME", 0)) != NULL)           // proper name ("Groot")
      fact = pool.AddProp(obj, "lex", NULL, neg, blf, val);     
    else if ((val = SlotGet(next, "HQ")) != NULL)                // simple property ("big") 
      fact = pool.AddProp(obj, "hq", val, neg, blf);            
    else if ((val = SlotGet(next, "DEG")) != NULL)               // degree property ("very red")
      fact = obj_deg(&tail, obj, val, tail, pool, neg, blf);    
    else if (SlotStart(next, "LOC") > 0)                         // location phrase ("at home")    
      fact = add_place(&tail, obj, next, tail, pool, neg, blf);   
    else if (SlotStart(next, "ACT-G") > 0)                       // participle ("sleeping")
    {
      fact = pool.AddProp(obj, "agt", mf.VerbLex(t, next), neg, blf, "act");
      fact->tags = t;
    }
    post = tail;
  }

  // see if fragments left after discrete properties handled
  if (*post != '\0')
  {
    // check for super-type declaration ("a kind of dog")
    if (nsuper_kind(next, 200, post) != NULL)
      if ((f2 = pool.AddProp(obj->Fact("ako"), "ako", next)) != NULL)
        return f2;
              
    // check for indeterminate predicate nominal ("a dog") which is always hypothetical
    create = 1;
    if ((f2 = build_obj(NULL, post, pool, obj, neg, blf)) != NULL)    
    {
      create = cr0;
      return f2;
    }
  }
  return fact;
}


//= Extract the nound kind associated with a super-kind element.
// returns NULL if fails, else binds kind and returns remainder of list

const char *jhcGraphizer::nsuper_kind (char *kind, int ssz, const char *alist) const
{
  char entry[80] = "";
  const char *tail, *val;

  // look for correct start
  if ((tail = NextEntry(alist, entry)) == NULL)
    return NULL;
  if (strcmp(entry, "%n-super") != 0)
    return NULL;

  // pull out AKO portion and copy
  if ((tail = NextEntry(tail, entry)) == NULL)
    return NULL;
  if ((val = SlotGet(entry, "AKO")) == NULL)
    return NULL;
  strcpy_s(kind, ssz, val);
  return FragClose(tail, 0);
}


//= Turn qualifier ("usually") into numeric belief value.

double jhcGraphizer::belief_val (const char *word) const
{
  char term[13][20] = {"definitely", "always", "certainly", "usually",  "probably",     "likely", 
                          "may",     "might",  "sometimes", "possibly", "occasionally", "unlikely to be", "seldom"};
  double val[13]    = {    1.2,        1.2,       1.1,         0.9,         0.8,           0.7,     
                           0.5,        0.5,       0.5,         0.3,         0.3,           0.1,             0.1   };
  int i;

  for (i = 0; i < 13; i++)
    if (strcmp(word, term[i]) == 0)
      return val[i];
  return 1.0;
} 



