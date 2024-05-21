// jhcGraphizer.cpp : turns parser alist into network structures
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "Language/jhcNetRef.h"        // common audio

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
  bulk = NULL;
  dbg = 0;             
//dbg = 3;             // to see call sequence for failed conversion
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
// return: 6 = op, 5 = rule, 4 = revision, 3 = question, 2 = command, 1 = fact, 
//         0 = nothing, negative for error

int jhcGraphizer::Assemble (const char *alist)
{
  char body[amax], head[80] = "";
  int spact = 0;                       // default = no network created

  // sanity check 
  if (core == NULL) 
    return -1;
  if (alist == NULL)
    return 0;

  // determine if a full item has been found
  if (SplitFrag(head, body, alist) != NULL)
  {
    if (strcmp(head, "%Immediate") == 0)
      spact = cvt_imm(body);
    else if (strcmp(head, "%Revision") == 0)
      spact = cvt_rev(body);
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
//   %Immediate -> chain (!dir or %play) or fact
// "bulk" member variable holds resulting jhcAliaChain
// returns 1 (fact) or 2 (command) or 3 (question) if successful, 0 for failure

int jhcGraphizer::cvt_imm (const char *alist) 
{
  char head[80], body[amax];
  jhcGraphlet temp;
  jhcWorkMem *wmem = &(core->atree);
  jhcNetNode *main;
  jhcAliaChain *ch;
  jhcAliaDir *dir;

  call_list(1, "cvt_imm", alist);

  // solve references against WMEM
  if (SplitFrag(head, body, alist) == NULL)
    return 0;
  wmem->InitConvo();                             // start of sentence
  univ = wmem;
  resolve = 0;

  // CHAIN - see if some sort of complex command (or question)
  if ((*head == '!') || (strncmp(head, "%play", 5) == 0))
  {
    create = 0;
    if ((bulk = build_chain(alist, NULL, *wmem)) == NULL)
      return 0;
    if (strcmp(head, "!chk-t") == 0)
      return append_ynq(bulk, *wmem);
    if (strcmp(head, "!find-t") == 0) 
      return 3;
    if (strcmp(head, "!find-c") == 0)
      return append_exist(bulk, *wmem);
    if (strcmp(head, "!find") == 0)
      return append_find(bulk, *wmem);
    return 2;
  }

  // FACT - build a single NOTE encapsulating factual assertion
  root = NULL;
  loop = NULL;
  create = 1;
  dir = new jhcAliaDir;
  wmem->BuildIn(dir->key);
  if (strcmp(head, "%fact-n") == 0)
    main = build_name(body, *wmem);
  else
    main = build_fact(NULL, body, *wmem);        // does not actualize
  if (main == NULL)
  {
    // cleanup from error
    delete dir;
    wmem->BuildIn(NULL);
    return 0;
  }
  main->MarkConvo();                             // user speech ("that")

  // embed NOTE in chain step and close out any pending loop
  (dir->key).MainProp();
  ch = new jhcAliaChain;
  ch->BindDir(dir);
  if (loop != NULL)
    ch->cont = loop;

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
        // attach normal and alt continuations
        chk->cont = tell_step("affirm", pool);
        chk->alt  = tell_step("deny",   pool);
        return 3;
      }
  return 0;
}


//= Adds action for confirming or denying the existence of something.
// assumes last directive in sequence is an empty find proceeding by the main FIND
// always returns 3 for convenience

int jhcGraphizer::append_exist (jhcAliaChain *seq, jhcNodePool& pool) const
{
  jhcAliaChain *find;

  // no fail branch needed if no FIND in chain
  if (seq != NULL) 
    if ((find = seq->Penult()) != NULL)
      if (find->StepDir(JDIR_FIND))
      {
        delete find->cont;
        find->cont = tell_step("affirm", pool);
        find->fail = tell_step("deny", pool);
      }
  return 3;
}


//= Adds action for confirming shift of attention to specified object.
// assumes last directive in sequence is the main FIND
// returns 3 if a question, 2 if really a command

int jhcGraphizer::append_find (jhcAliaChain *seq, jhcNodePool& pool) const
{
  jhcAliaChain *find, *tail = seq;

  // sanity check then create confirmation action step
  if (seq == NULL) 
    return 3;

  // see if inside some sort of loop
  if (multi != NULL)
  {
    if (multi->alt == NULL)
    {
      // exit "for" multi-step loop 
      multi->alt = tell_step("confirm", pool);
      return 3;
    }
    tail = multi;
  }
  else if (root != NULL)
  {
    if (root->alt == NULL)
    {
      // exit outermost loop
      root->alt = tell_step("confirm", pool);
      return 3;
    }
    tail = root;
  }
    
  // go to end of linear tail section
  if ((find = tail->Last()) != NULL)
    if (find->StepDir(JDIR_FIND))
    {
      find->cont = tell_step("confirm", pool);
      return 3;
    }
  return 2;
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
  old = pool.BuildIn(dir->key);
  pool.MakeAct(verb);
  pool.BuildIn(old);
  return step;
}


///////////////////////////////////////////////////////////////////////////
//                           Operator Revision                           //
///////////////////////////////////////////////////////////////////////////

//= Interpret association list to create instructions for revising some operator.
//   %Rule -> action description + $proc, if no description then summarizes $proc
// "bulk" member variable holds resulting jhcAliaChain
// returns 4 if successful, 0 for failure

int jhcGraphizer::cvt_rev (const char *alist) 
{
  char entry[200];
  jhcWorkMem *wmem = &(core->atree);
  jhcAliaDir *dir, *dir2;
  jhcAliaChain *ch;
  const char *tail;
  jhcNetNode *act;
  const jhcNetNode *fcn;

  call_list(1, "cvt_rev", alist);

  // always build chain with an initial EDIT directive
  bulk = new jhcAliaChain;
  dir = new jhcAliaDir(JDIR_EDIT);
  bulk->BindDir(dir);
  univ = wmem;

  // interpret description of prior action (if any)
  wmem->BuildIn(dir->key);
  create = 1;
  resolve = 1;
  if ((tail = NextFrag(alist, entry)) != NULL)
    if (strncmp(entry, "!do", 3) == 0)
      if ((act = build_do(tail, *wmem)) != NULL)
        if (HasSlot(alist, "NEG-V"))
          act->SetNeg(1);
  wmem->BuildIn(NULL);

  // get alternate procedure (none for preference adjust or action modification)
  create = 0;
  resolve = 0;
  if ((tail = FindFrag(alist, "$proc")) != NULL)
    if ((ch = build_chain(tail, NULL, *wmem)) != NULL)     // PREF ingnored
      bulk->cont = ch;
  if (!(dir->key).Empty())
    return 4;

  // if EDIT was empty, copy just verb (no args) from first procedure step
  ch = bulk;
  while ((ch = ch->cont) != NULL)
    if ((dir2 = ch->GetDir()) != NULL)
      if (dir2->Kind() == JDIR_DO)
        if ((fcn = dir2->KeyMain()) != NULL)
          if (fcn->Val("fcn") != NULL)
          {
            wmem->BuildIn(dir->key);
            wmem->MakeAct(fcn->Lex());
            wmem->BuildIn(NULL);
            return 4;
          }

  // clean up from error
  delete bulk;
  bulk = NULL;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                                Rules                                  //
///////////////////////////////////////////////////////////////////////////

//= Interpret association list to build a new rule.
//   %Rule -> $macro or [$cond $res] or [$cond-i $res-i] or [$cond-s $res-s] or [$res $cond]
// "rule" member variable holds result
// returns 5 if successful, 0 for failure

int jhcGraphizer::cvt_rule (const char *alist) 
{
  call_list(1, "cvt_rule", alist);

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
    (rule->result).RemAll(rule->cond);
    rule->conf = (rule->result).MinBelief();
    (rule->result).ForceBelief(rule->conf);
    (rule->result).ActualizeAll(0);              // needed for jhcAliaRule::match_found
    return 5;
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
  call_list(1, "build_fwd", alist);
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
  call_list(1, "build_rev", alist);
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
  jhcNetNode *focus;
  const char *tail, *t2, *val;

  // look for proper condition type
  if ((tail = ExtractBody("$cond-i", body, alist)) == NULL)
    return false;
  call_list(1, "build_ifwd", alist);

  // assemble condition part  
  r.BuildIn(r.cond);
  if ((focus = build_obj(NULL, body, r)) == NULL)            
  {
    // ascribe property or manner to unknown item ("orange ... is a color")
    if ((t2 = FragNextPair(body, next)) == NULL)
      return false;
    if ((val = SlotGet(next, "HQ")) != NULL)               
    {
      focus = r.MakeNode("hq", val);                       // quoted adjective
      focus->AddArg("hq", r.MakeNode("obj"));             
    }
    else if ((val = SlotGet(next, "MOD")) != NULL)         
    {
      focus = r.MakeNode("mod", val);                      // quoted adverb
      focus->AddArg("mod", r.MakeNode("act"));             
    }
    else
      return false;
  }
  (r.cond).MainProp();

  // assemble result part
  if (ExtractBody("$res-i", body, tail) == NULL)
    return false;
  r.BuildIn(r.result);
  if (add_cop(NULL, focus, body, r) == NULL)
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
  call_list(1, "build_sfwd", alist);
  r.BuildIn(r.cond);
  if ((obj = build_obj(NULL, body, r)) == NULL)                
    return false;
  (r.cond).MainProp();

  // assemble result part
  if (ExtractBody("$res-s", body, tail) == NULL)
    return false;
  r.BuildIn(r.result);
  if (build_fact(NULL, body, r, obj) == NULL)                  
    return false;
  (r.result).MainProp();
  return true;
}


//= Make rule for pattern "X means Y".
// intended for adjectives (HQ) and adverbs (MOD and DIR)
// returns 1 if successful, 0 for failure

bool jhcGraphizer::build_macro (jhcAliaRule& r, const char *alist) const
{
  char pair[200], pair2[200], body[amax], kind[40], base[40];
  jhcNetNode *prop, *arg;
  const char *tail, *wd, *wd2;

  // look for proper condition type
  if (ExtractBody("$macro", body, alist) == NULL)
    return false;
  call_list(1, "build_macro", alist);

  // get two lexical terms to be related
  if ((tail = FragNextPair(body, pair)) == NULL)
    return 0;
  if ((wd = SplitPair(kind, pair, 1)) == NULL)
    return 0; 
  if (FragNextPair(tail, pair2) == NULL)
    return 0;
  if ((wd2 = SlotGet(pair2)) == NULL)
    return 0; 

  // handle verb super-classes
  if (strcmp(kind, "act-g") == 0)
  {
    r.BuildIn(r.cond);
    arg = rule->MakeAct(mf.BaseWord(base, wd, JTAG_VPROG));
    r.BuildIn(r.result);
    rule->AddProp(arg, "fcn", mf.BaseWord(base, wd2, JTAG_VPROG));
    return 1;
  } 

  // create rule structure involving two properties and one argument
  r.BuildIn(r.cond);
  prop = rule->MakeNode(kind, wd);
  arg = rule->MakeNode((strcmp(kind, "hq") == 0) ? "obj" : "act");
  prop->AddArg(kind, arg);
  r.BuildIn(r.result);
  rule->AddProp(arg, kind, wd2); 
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

  call_list(1, "build_graph", alist);

  pool.BuildIn(gr);
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
// returns 6 if successful, 0 for failure

int jhcGraphizer::cvt_op (const char *alist) 
{
  char body[amax];
  const char *t2, *tail = alist;

  call_list(1, "cvt_op", alist);

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
    resolve = 0;                       // was 1
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
    return 6;
  }

  // cleanup from some problem
  delete oper;
  oper = NULL;
  return 0;
}


//= Create a new operator with appropriate type of trigger condition (blank for now).
// if trigger is a prohibition ($trig-n) then cond type will be GATE and default PUNT chain is added
// returns new operator or NULL for problem

jhcAliaOp *jhcGraphizer::config_op (const char *alist) const
{
  char entry[80];
  jhcAliaOp *op;
  const char *tail;
  int k = JDIR_NOTE, veto = 0;

  call_list(1, "config_op", alist, -1);

  // make sure operator has a triggering condition (and possibly a second)
  if (FindFrag(alist, "$trig-n") != NULL)
    veto = 1;
  else if (FindFrag(alist, "$trig-p") != NULL)
    veto = -1;
  tail = kind_op(k, alist, veto);
  kind_op(k, tail, veto);

  // create operator of proper kind and adjust preference
  op = new jhcAliaOp((JDIR_KIND) k);
  if (FindSlot(alist, "PREF", entry) != NULL)
    op->pref = pref_val(entry);

  // possibly add final PUNT/PASS directive for prohibitions/permissions
  if (veto > 0)
    op->meth = dir_step("punt");
  else if (veto < 0)
    op->meth = dir_step("do");                   // empty description
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

  call_list(1, "kind_op", alist, -1);

  // look for next trigger fragment
  while (strncmp(head, "$trig", 5) != 0)
    if ((tail = SplitFrag(head, body, tail)) == NULL)
      return NULL;

  // generally directive type depends first command found 
  while ((t2 = FragNextFrag(t2, head)) != NULL)
    if (*head == '!')
    {
      if ((k = dcvt.CvtKind(head + 1)) >= JDIR_MAX)
        return NULL;
      if ((k == JDIR_DO) && (veto != 0))           // prohibitions/permissions are GATE
        k = JDIR_GATE;                     
      break;
    }

  // check specially for ANTE and NONE advice operators
  if ((k == JDIR_DO) && FragHasSlot(body, "BEFORE"))
    k = JDIR_ANTE;
//  if ((k == JDIR_FIND) && (FragHasSlot(body, "FAIL") || FragHasSlot(body, "BEFORE")))
//    k = JDIR_NONE;
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
  int cond, prima, must = 0;

  call_list(1, "build_sit", alist);

  // conditions can be commands or facts
  while (1)
  {
    // check for "unless" before next getting clause
    if (NextEntry(tail, body) == NULL)
      break;
    cond = ((SlotMatch(body, "UNLESS")) ? 0 : 1);
    if ((tail = SplitFrag(head, body, tail)) == NULL)
      break;

    // determine type of clause
    if ((*head == '!') && (ktag != NULL))
    {
      // triggering condition (only for ops with ktag != NULL)
      jprintf(1, dbg, "-- %s\n", ktag);
      sit.BuildCond();
      cmd = build_cmd(head, body, sit);
//      if ((cmd = build_cmd(head, body, sit)) == NULL)
//        return 0;
//      sit.CmdHead(cmd);
      jprintf(1, dbg, "----\n\n");
    }
    else if (strncmp(head, "%fact", 5) != 0)
      continue;
    else if (cond <= 0) 
    {
      // prima facie absent ("unless" or "and not")
      if ((prima = sit.BuildUnless()) <= 0)
        return 0;
      jprintf(1, dbg, "-- UNLESS %d\n", prima);      
      if (build_fact(NULL, body, sit, NULL, cond) == NULL)     
        return 0;
      sit.UnlessHead();
      jprintf(1, dbg, "----\n\n");
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
//   start = entry step of full chain that was built
//   pod   = current play step being built (if any)
//   play  = current multi-path play payload from pod
//   ch0   = previous step that should continue into current one
//   ch    = current bare directive step being built (if any)
//   mini  = current full directive step with leading skolem fcns added
// returns NULL if some problem else full mini-graph of actions plus skolem

jhcAliaChain *jhcGraphizer::build_chain (const char *alist, jhcAliaChain *ult, jhcNodePool& pool) 
{
  char entry[200] = "";
  jhcAliaChain *mini, *start = NULL, *pod = NULL, *ch = NULL;
  const char *tail = alist;
  int guard = 0;

  call_list(1, "build_chain", alist);

  // reset implicit iteration
  root = NULL;
  loop = NULL;
  multi = NULL;

  // construct chain sequence from multiple actions 
  while ((tail = NextFrag(tail, entry)) != NULL)
    if (strncmp(entry, "%play", 5) == 0)
    {
      // make the next chain step be a new play
      pod = play_step(guard, tail, pool);
      if (ch == NULL)
        start = pod;
      else
        ch->cont = pod;  
      ch = NULL;
    }
    else if ((strcmp(entry, "%fact") == 0) && (guard > 0))
      tail = FragClose(tail, 0);
    else if ((strcmp(entry, "%") == 0) && (pod != NULL))
    {
      // finish off any current play
      ch = pod;
      pod = NULL; 
    }
    else if (*entry == '!')
    {
      // create sequence of directives (usually BINDs then a DO)
      if (strcmp(entry, "!for") == 0)
      {
        if ((mini = overt_loop(tail, pool)) == NULL)
          break;
        ch = mini->Last();
      }
      else
      {
        if ((mini = single_cmd(entry, tail, pool)) == NULL)
          break;
        ch = connect_loop(ch, mini);
      }

      // add either as a required activity or tack onto end of chain
      if (pod != NULL)
      {
        pod->PlayAct(mini, guard);
        ch = NULL;
      }
      else if (start == NULL)          // no steps in chain yet
        start = mini;
      tail = FragClose(tail, 0);
    }

  // check for success (non-NULL tail means build_cmd failed) 
  if ((tail == NULL) && (start != NULL))
  {
    finish_loop(ch, ult);
    return start;
  }

  // cleanup (chain automatically deletes payload)
  delete start;                      
  return NULL;
}


//= Make a step containing a play and possibly set main condition to a WAIT.
// returns activity addition mode: 0 = main, 1 = guard, 2 = looping guard

jhcAliaChain *jhcGraphizer::play_step (int& mode, const char *alist, jhcNodePool& pool)
{
  char body[amax], test[80] = "";
  jhcGraphlet temp;
  jhcAliaChain *ch, *pod = new jhcAliaChain;
  jhcAliaPlay *play = new jhcAliaPlay;
  jhcAliaDir *dir;
  jhcNetNode *evt;
  const char *tail;

  // look for early termination condition (else add main activities later)
  pod->BindPlay(play);
  mode = 0;
  if ((tail = FragFindSlot(alist, "STAY", test)) == NULL)
    return pod;
  if (ExtractBody("%fact", body, tail) == NULL)
    return pod;

  // extract condition to wait for (negate if "while")
  skolem = NULL;
  pool.BuildIn(temp);
  if ((evt = build_fact(NULL, body, pool)) == NULL)
    return pod;
  if (strcmp(test, "while") == 0)
    evt->SetNeg((evt->Neg()) ? 0 : 1);
  pool.BuildIn(NULL);
  
  // install WAIT directive as main activity 
  ch = new jhcAliaChain;
  dir = new jhcAliaDir(JDIR_WAIT);
  ch->BindDir(dir);
  (dir->key).Copy(temp);
  if (skolem != NULL)
    play->AddReq(skolem->Append(ch));
  else
    play->AddReq(ch);
  skolem = NULL;

  // other activities are guards (possibly looped)
  mode = ((FragHasSlot(alist, "KEEP")) ? 2 : 1);
  return pod;
}


//= Build controlling EACH or ANY for some loop with multi-step body.
// returns directive with suitable prefixed BINDs (if any)

jhcAliaChain *jhcGraphizer::overt_loop (const char *alist, jhcNodePool& pool)
{
  jhcAliaChain *mini;

  skolem = NULL;
  if (build_obj(NULL, alist, pool) == NULL)
    return NULL;
  mini = skolem;
  multi = mini->Last();
  root = NULL;
  loop = NULL;
  skolem = NULL;             // clean up
  return mini;
}


//= Create chain for a single command prefixed by any necessary BINDs.
// returns first step in sequence (might be a BIND), NULL for problem

jhcAliaChain *jhcGraphizer::single_cmd (const char *entry, const char *alist, jhcNodePool& pool)
{ 
  jhcGraphlet key0;
  jhcAliaChain *ch, *mini;
  jhcAliaDir *dir;

  // get complete action specification (incl. building BINDs)
  skolem = NULL;
  pool.BuildIn(key0);
  build_cmd(entry, alist, pool);
//  if ((cmd = build_cmd(entry, alist, pool)) == NULL)
//    return NULL; 
//  key0.SetMain(cmd);
  pool.BuildIn(NULL);

  // make up new chain step which is a single directive
  if ((ch = dir_step(entry + 1)) == NULL)
    return NULL;
  dir = ch->GetDir();
  (dir->key).Copy(key0);

  // prepend any generated BINDs (main command always at end)
  if (skolem == NULL)
    mini = ch;
  else if (strcmp(entry, "!find") != 0)
    mini = skolem->Append(ch);
  else
  {
    // throw away any partial FIND directive (skolem is more complete)
    delete ch;
    mini = skolem;
  }
  skolem = NULL;                       // clean up
  return mini;
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
  else if (strcmp(kind, "ach") == 0)
    dir->SetKind("ach");
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


//= Wire new chain into any ongoing loop structure.
// ch is previous last step, mini is start of new command 
// multi, root, and loop are bookkeeping member variables for iteration
// returns step (sometimes NULL) to attach any next step to

jhcAliaChain *jhcGraphizer::connect_loop (jhcAliaChain *ch, jhcAliaChain *mini)
{
  jhcAliaChain *last = mini->Last();

  // add in any implicit looping jumps 
  if ((loop != NULL) && (loop != multi))
  {
    last->cont = loop;                 // repeat main loop 
    last = NULL;                       // continuation already set
    loop = NULL;
  }
  else if (root != NULL) 
  {
    root->alt = mini;                  // jump here instead when done
    root = NULL;
  }

  // tack new composite step onto the sequence somewhere
  if ((multi != NULL) && (multi->cont == NULL))
    multi->cont = mini;
  else if (ch != NULL)                 // possibly NULL after loop 
    ch->cont = mini;
  return last;
}


//= Make sure explicit "for" loops get final jump to beginning.
// can also add in some extra actions (ult) before loopback
// assumes ch is never NULL

void jhcGraphizer::finish_loop (jhcAliaChain *ch, jhcAliaChain *ult)
{
  // setup final loopback of multiple actions "for ..."
  if (multi != NULL)
  {
    if (ch != NULL)
      ch->cont = multi;
    else if (root != NULL)
      root->alt = multi;
  }

  // tack any extra set of actions onto end
  if (ult != NULL)
  {
    if (root != NULL)                  // main was a loop
      root->alt = ult;
    else if (ch->cont == NULL)
      ch->cont = ult;
  }
}


//= Fill in details of directive from remaining association list.
//    cmd -> !do or !chk or !chk-t or !find or !find-t
// returns main action node if successful, NULL for problem

jhcNetNode *jhcGraphizer::build_cmd (const char *head, const char *alist, jhcNodePool& pool)
{
  char body[amax];
  jhcNetNode *focus, *main, *dest;
  jhcGraphlet *acc;

  call_list(1, "build_cmd", alist, 0, head);

  // possibly convert question "X?" to command "Tell me X"
  if (strcmp(head, "!find-t") == 0)
  {
    // build_chain automatically pre-pends one or more FINDs using "skolem"
    if ((focus = build_query(alist, pool)) == NULL)
      return NULL;
    demote_bind();                                         // no assumption

    // generate guts for a DO directive to tell about node found
    if ((acc = pool.Accum()) != NULL)
      acc->Clear();                                        // sometimes me/you?
    main = pool.MakeAct("tell");
    main->AddArg("obj", focus);
    dest = pool.AddProp(main, "dest", "to");
    dest->AddArg("ref", (core->atree).Human());
    return main;
  }

  // try building structure for other directives
  if (strncmp(head, "!ach", 3) == 0)                       
    return build_ach(alist, pool);   
  if (strncmp(head, "!do", 3) == 0)                       
    return build_do(alist, pool);   
  if (strncmp(head, "!chk", 4) == 0)                       // includes "!chk-t"
  {
    if (ExtractBody("%fact-n", body, alist) != NULL) 
      return build_name(body, pool);                 
    if (ExtractBody("%fact", body, alist) != NULL) 
    {
      if ((focus = build_fact(NULL, body, pool)) == NULL)
        return NULL;
      if (HasSlot(alist, "AUX-D"))                         // for "did you X"
        focus->SetDone(1);
      return focus;
    }
  }
  if (strncmp(head, "!find", 5) == 0)                      // includes "!find-c"
  {
    if ((focus = build_query(alist, pool)) != NULL)
      demote_bind();                                       // no assumption
    return focus;
  }
  return NULL;
}


//= Build structures for various types of wh-questions and return focal node.
//  query -> np or $q-mod or $q-ako or $q-hq or $q-name or $q-loc or $q-cnt or $q-has

jhcNetNode *jhcGraphizer::build_query (const char *alist, jhcNodePool& pool) 
{
  jhcNetRef nr(univ, (core->atree).MinBlf());
  char head[80], ness[80], entry[200], body[amax];
  jhcNetNode *obj2, *main, *act, *obj = NULL;
  jhcGraphlet *acc;
  const char *t2, *tail, *kind = NULL;
  UL32 t;
  int qcnt = 0;

  call_list(1, "build_query", alist);

  // figure out what type of question this is (object vs. property)
  if (SplitFrag(head, body, alist) == NULL)
    return NULL;
  if (*head != '$')
    return build_obj(NULL, alist, pool);
  tail = body;

  // look for adverbial modifier of reference statement
  if (strcmp(head, "$q-mod") == 0)
  {
    act = build_fact(NULL, tail, nr);
    main = nr.AddProp(act, "mod", NULL);
    nr.CmdHead(main);
    return nr.FindMake(pool, 0, NULL, (core->atree).MinBlf(), &skolem); 
  }

  // get constraint on desired answer kind 
  if (strcmp(head, "$q-hq") == 0)                          // required for props
  {
    // find first slot-value pair
    t2 = tail;
    while ((t2 = NextEntry(t2, entry)) != NULL)
    {
      if (!IsFrag(entry))
        break;
      t2 = FragClose(t2, 0);
    }
    if (t2 != NULL)
      if ((kind = SlotGet(entry, "AKO")) == NULL)
      {
        // "how big" sets kind = "bigness" for FIND
        if ((kind = SlotGet(entry, "HQ")) == NULL)
          return NULL;
        kind = mf.PropKind(ness, kind);
      }   
  }
  else if (strcmp(head, "$q-ako") == 0)                    // optional for kinds
    if ((t2 = nsuper_kind(entry, 200, tail)) != NULL)
    {
      kind = entry;
      tail = t2;
    }

  // get referent (generally resolving or creating a BIND in skolem)
  if (strcmp(head, "$q-name") == 0)
    obj = obj_owner(FragStart(tail), pool);                // series of possessives
  if (strcmp(head, "$q-cnt") == 0)
    qcnt = 1;
  if (obj == NULL)
    obj = build_obj(&t2, FragStart(tail), pool, NULL, 1.0, qcnt);
  if (strcmp(head, "$q-desc") == 0)
  {
    // return directly if seeking an object description
    if ((NextEntry(tail, entry) == NULL) || (SlotStart(entry, "ACT-G") <= 0))
      return obj;

    // make new sought item as the object of some verb phrase
    main = nr.MakeNode("obj");
    act = nr.MakeAct(mf.VerbLex(t, entry));
    act->tags = t;
    act->AddArg("agt", obj);
    act->AddArg("obj", main);
    return nr.FindMake(pool, 0, NULL, (core->atree).MinBlf(), &skolem);
  }

  // for counting replace head of BIND in skolem with count fact
  if (qcnt > 0)
  {
    main = pool.AddProp(obj, "cnt", NULL, 0, 1.0); 
    if (skolem == NULL)                                    // needs some description
      return NULL;
    if ((acc = skolem->LastKey()) != NULL)
      acc->SetMain(main);
    return main;
  }

  // make up top level entity to find (with constraints)
  if (strcmp(head, "$q-has") == 0)
  {
    // if two objects present, require the second to be possessed by the first 
    if ((obj2 = build_obj(NULL, FragStart(tail), pool)) != NULL)
    {
      acc = pool.BuildIn(skolem->LastKey());
      act = pool.MakePoss(obj, obj2);
      pool.BuildIn(acc);
      return obj;                                          // return owner
    }

    // else make up new owner and require him to possess the original object
    act = nr.MakePoss(nr.MakeNode("agt"), obj);
    return nr.FindMake(pool, 0, NULL, (core->atree).MinBlf(), &skolem);
  }

  // add unknown property and possibly constrain its type
  main = nr.AddProp(obj, head + 3, NULL);                  // extract slot name
  if (strcmp(head, "$q-src") == 0)
  {
    nr.SetLex(main, "from");
    obj2 = nr.MakeNode("obj");
    main->AddArg("ref", obj2);
    nr.CmdHead(obj2);                                      // find source referent
  }
  else if (strcmp(head, "$q-loc") == 0)
    main->AddArg("ref", nr.MakeNode("obj"));
  if (kind != NULL)
    nr.AddProp(main, "ako", kind);
  return nr.FindMake(pool, 0, NULL, (core->atree).MinBlf(), &skolem);    
}


//= Do not allow final query to assume answer (i.e. FIND not BIND).

void jhcGraphizer::demote_bind () const
{
  const jhcAliaChain *bind;
  jhcAliaDir *dir;

  if (skolem != NULL)
    if ((bind = skolem->Last()) != NULL)
      if ((dir = bind->GetDir()) != NULL)
        dir->SetKind(JDIR_FIND);                        
}


///////////////////////////////////////////////////////////////////////////
//                             Action Phrases                            //
///////////////////////////////////////////////////////////////////////////

//= Create network structure for achievement goal.
//   !ach -> %fact
// expects "!ach" is stripped off front already
// returns pointer to newly created goal main verb

jhcNetNode *jhcGraphizer::build_ach (const char *alist, jhcNodePool& pool)
{
  char body[amax], head[80] = "";

  call_list(1, "build_ach", alist);

  if (SplitFrag(head, body, alist) != NULL)
    if (strcmp(head, "%fact") == 0)
      return build_fact(NULL, body, pool);     
  return NULL;
}


//= Create network structure for imperative verb phrase.
//   !do -> ACT or [ACT-D ACT-G] or ACT-2 or SAY
// expects "!do" is stripped off front already
// returns pointer to newly created action node

jhcNetNode *jhcGraphizer::build_do (const char *alist, jhcNodePool& pool)
{
  char next[200];
  const char *val = NULL, *end = alist, *tail = alist;
  jhcNetNode *act, *iobj, *dest;
  UL32 t = 0;      
  int quote = 0, neg = 0;  

  call_list(1, "build_do", alist);

  // check for overall negation of verb or stop command
  if (FragHasSlot(tail, "NEG-V") || FragHasSlot(tail, "STOP"))
    neg = 1;

  // look for main verb but ignore placeholder "do something"
  while ((end = FragNextPair(end, next)) != NULL)
    if ((val = mf.VerbLex(t, next)) != NULL)                
      break;
  if (val == NULL)                    
    return NULL;
  if (match_any(val, "do something", "do anything"))
    val = NULL;

  // see if special "say" type then make node for action
  if (strncmp(next, "SAY", 3) == 0)
    quote = 1;
  act = pool.MakeAct(val, neg);
  act->tags = t;

  // attach all adverbial modifiers (could come before verb)
  while ((tail = FragNextPair(tail, next)) != NULL)
    if ((val = SlotGet(next, "DEG")) != NULL)
      tail = act_deg(act, val, tail, pool);
    else if ((val = SlotGet(next, "MOD")) != NULL)
      pool.AddProp(act, "mod", val);
    else if ((val = SlotGet(next, "DIR")) != NULL)
      pool.AddProp(act, "dir", val);
    else if ((val = SlotGet(next, "AMT")) != NULL)
      pool.AddProp(act, "amt", val);
    else if ((val = SlotGet(next, "^INT")) != NULL)
      tail = act_amt(act, val, tail, pool);

  // add noun-like arguments or quoted string (comes after verb)
  if (quote > 0)
  {
    if ((iobj = build_obj(NULL, end, pool)) != NULL)       // iobj only (FINDs okay)
    {
      dest = pool.AddProp(act, "dest", "to");
      dest->AddArg("ref", iobj);
    }
    add_quote(act, end, pool);
  }
  else 
  {
    act = add_args(act, end, pool);                        // iobj + dobj
    add_rels(act, end, pool);                              // adverbs + pp
  }
  return act;
}


//= Build an assertion about some word being the name of something.
// handles %fact-n expansion (assumes already stripped off)
// returns pointer to main name assertion

jhcNetNode *jhcGraphizer::build_name (const char *alist, jhcNodePool& pool)
{
  char val[80];
  jhcNetNode *dude;

  call_list(1, "build_name", alist);

  if (FindSlot(alist, "NAME", val) == NULL)
    return NULL;
//  if ((dude = build_obj(NULL, alist, pool)) == NULL)
//    if ((dude = build_obj(NULL, FragStart(alist), pool)) == NULL)
  if ((dude = obj_owner(alist, pool)) == NULL)
    dude = pool.MakeNode("agt");
  return pool.AddProp(dude, "name", val);
}


//= Build a sentence-like semantic network with subject and object(s).
// can optionally take pre-defined subject with alist being rest of statement
// if "pos" > 0 forces interpretation to be positive (i.e. can ignore <NEG> for unless clauses)
// returns pointer to main verb assertion

jhcNetNode *jhcGraphizer::build_fact (const char **after, const char *alist, jhcNodePool& pool, 
                                      jhcNetNode *subj, int pos)
{
  char word[80], pair[200];
  const char *t2, *post = alist, *tail = alist, *val = NULL;
  jhcNetNode *act = NULL, *agt = subj;
  jhcGraphlet *acc;
  double blf = 1.0;
  UL32 t = 0;      
  int neg = 0, past = 0;

  call_list(1, "build_fact", alist, 0, ((subj != NULL) ? subj->Nick() : ""));

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
    if ((acc = pool.Accum()) != NULL)
      if ((acc->Main())->Val("fcn") == NULL)
        acc->SetMain(act);                       // for "that"
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
  act = pool.MakeAct(val, neg, blf);
  if ((t & JTAG_VPAST) != 0)                     // was VPRES = 0
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
    else if ((val = SlotGet(pair, "DIR")) != NULL)
      pool.AddProp(act, "dir", val);
    else if ((val = SlotGet(pair, "AMT")) != NULL)
      pool.AddProp(act, "amt", val);
    else if ((val = SlotGet(pair, "^INT")) != NULL)
      tail = act_amt(act, val, tail, pool);

  // add noun-like arguments or quoted string (after verb)
  act = add_args(act, post, pool);
  add_rels(act, post, pool);
  if (after != NULL)
    if ((*after = FragClose(post, 0)) == NULL)   // end of enclosing fragment
      *after = post + strlen(post);
  if ((acc = pool.Accum()) != NULL)
    if ((acc->Main())->Val("fcn") == NULL)
      acc->SetMain(act);                         // for "that"
  return act;
}


//= Make nodes for adverbial descriptions with a degree ("very slowly" or "far down").
// returns unused portion of alist

const char *jhcGraphizer::act_deg (jhcNetNode *act, const char *amt, const char *alist, jhcNodePool& pool) const
{
  char pair[200], slot[80];
  const char *val, *tail;

  call_list(1, "act_deg", alist, 0, amt);

  // possibly add an adverbial description to node
  if ((tail = FragNextPair(alist, pair)) == NULL)
    return alist;
  val = SplitPair(slot, pair, 1);

  // modify the adverbial descriptor ("mod" or "dir" pr "amt")
  if ((strcmp(slot, "mod") != 0) && (strcmp(slot, "dir") != 0) && (strcmp(slot, "amt") != 0))
     return alist;
  pool.AddDeg(act, slot, val, amt); 
  return tail;
}


//= Make nodes for adverbial descriptions with an amount ("22 inches").
// returns unused portion of alist

const char *jhcGraphizer::act_amt (jhcNetNode *act, const char *num, const char *alist, jhcNodePool& pool) const
{
  char pair[200], cnt[20], measure[40];
  jhcNetNode *prop;
  const char *units, *tail;

  call_list(1, "act_amt", alist, 0, num);

  // look for the reuired measurement units
  if ((tail = FragNextPair(alist, pair)) == NULL)
    return alist;
  if ((units = SlotGet(pair, "AKO-S")) == NULL)
    return alist;

  // modify the adjectival descriptor (override pool defaults)
  prop = pool.AddProp(act, "amt", mf.BaseWord(measure, units, JTAG_NPL));
  pool.AddProp(prop, "cnt", parse_int(cnt, num, 20));
  return tail;
}


//= Add a node which has a long literal string expansion.
// returns 1 if found, 0 for problem

int jhcGraphizer::add_quote (jhcNetNode *v, const char *alist, jhcNodePool& pool) const
{
  char next[200];
  jhcNetNode *q;
  const char *val, *tail = alist;

  call_list(1, "add_quote", alist, 0, v->Nick());

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
  jhcNetNode *swap, *dest, *dobj = NULL, *iobj = NULL, *act = NULL;

  // sanity check 
  if (alist == NULL)
    return v;
  call_list(1, "add_args", alist, 0, v->Nick());

  // look for first object (remove any adverbs directly after main verb)
  tail = StripPairs(alist);                               
  if ((dobj = build_obj(&tail, tail, pool)) != NULL)
    if (*tail != '\0')
    {
      // look for second object (if any)
      if ((iobj = build_obj(NULL, tail, pool)) != NULL)
      {
        // correct order is iobj then dobj so swap (except if "here" or "there") 
        if (!HasSlot(StripEntry(tail), "REF-L", 1))
        {
          swap = iobj;
          iobj = dobj;
          dobj = swap;
        }
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
  if (dobj != NULL)
    v->AddArg("obj", dobj);
  if (act != NULL)
    v->AddArg("act", act);
  if (iobj != NULL)
  {
    dest = pool.AddProp(v, "dest", "to");
    dest->AddArg("ref", iobj);
  }
  return((act != NULL) ? act : v);
}


//= Add prepositional phrases modifiers (typically only one) to action.

void jhcGraphizer::add_rels (jhcNetNode *act, const char *alist, jhcNodePool& pool) 
{
  char entry[200], entry2[200];
  jhcNetNode *obj, *tool, *src, *dest;
  const char *t2, *tail = alist;

  // sanity check 
  if ((alist == NULL) || (act == NULL))
    return;
  call_list(1, "add_rels", alist, 0, act->Nick());

  // look for PP attached to main verb
  while ((tail = NextFrag(tail, entry)) != NULL)
  {
    if (strncmp(entry, "$rel", 4) == 0)
    {
      if ((t2 = FragNextPair(tail, entry)) != NULL)
      {
        // determine if location phrase 
        if (SlotStart(entry, "LOC") > 0)                       
          add_place(&tail, act, entry, t2, pool);
      }
      else if ((obj = build_obj(&t2, tail, pool)) != NULL)
      {
        // add "with" adjunct instead
        tool = pool.AddProp(act, "with", "with");
        tool->AddArg("ref", obj);
        tail = t2;
      }
    }
    else if (strcmp(entry, "$src") == 0)         // adverbial modifier
    {
      if ((obj = build_obj(&t2, tail, pool)) != NULL)
      {
        // start location as a "from" prepositional phrase
        src = pool.AddProp(act, "src", "from");
        src->AddArg("ref", obj);
        tail = t2;
      }
      else if ((t2 = FragNextPair(tail, entry2)) != NULL)
        if (SlotStart(entry2, "LOC") > 0)                       
        {
          // start location as a location phrase ("from on top of") 
          obj = add_place(&tail, obj, entry2, t2, pool);
          obj->AddArg("src", act);
        }
    }
    else if (strcmp(entry, "$dest") == 0)        // intrinsic verb argument
    {
      if ((obj = build_obj(&t2, tail, pool)) != NULL)
      {
        // end location (also indirect object) as a noun
        dest = pool.AddProp(act, "dest", "to");
        dest->AddArg("ref", obj);
        tail = t2;
      }
      else if ((t2 = FragNextPair(tail, entry2)) != NULL)
        if (SlotStart(entry2, "LOC") > 0)                       
        {
          // end location as a prepositional phrase 
          obj = add_place(&tail, obj, entry2, t2, pool);
          obj->AddArg("dest", act);
        }
    }
    tail = FragClose(tail, 0);                   // ignore rest of fragment 
  }
}


///////////////////////////////////////////////////////////////////////////
//                             Object Phrases                            //
///////////////////////////////////////////////////////////////////////////

//= Create network structure for noun phrase.
// initially creates description in a NodeRef to check if a referent already exists
// can optionally force new description onto an old object in pool by setting "f0"
// spreads negation widely: not a big red dog -> not big & not red & not a dog
// "find": -1 = always create new item (not used)
//          0 = resolve locally else make FIND
//          1 = resolve locally else make BIND (create > 0)
//          2 = resolve locally else create item (resolve > 0)
//          3 = always make EACH/ANY
// if qcnt > 0 then just builds hypothetical description in wmem (never a skolem)
// returns pointer to newly created object node (and advances alist to after object)
 
jhcNetNode *jhcGraphizer::build_obj (const char **after, const char *alist, jhcNodePool& pool, 
                                     jhcNetNode *f0, double blf, int qcnt) 
{
  jhcNetRef nr(univ, (core->atree).MinBlf());
  char next[200], word[80] = "";
  jhcNetNode *obj, *ref, *kind = NULL, *fact = NULL;
  const char *spec;
  int find = 0;

  call_list(1, "build_obj", alist, 0, ((f0 != NULL) ? f0->Nick() : NULL));

  // determine jhcNetRef::FindMake fmode
  if (qcnt > 0)
    find = 0;
  else if (resolve > 0) 
    find = 2;
  else if (create > 0)
    find = 1;

  // check if next thing is embedded clause (e.g. "remember that X")
  if ((spec = NextEntry(alist, next)) == NULL)
    return NULL;
  if (strcmp(next, "%fact") == 0)                
  {
    create = 1;                                  
    return build_fact(after, spec, pool);        
  }

  // check for some sort of question object or copular fragment
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
  if (strncmp(next, "%obj", 4) != 0)             // naked HQ handled in build_ifwd
    return NULL;
  if (strncmp(next, "%obj-i", 6) == 0)
    find = ((create > 0) ? -1 : 0);                 

  // CORE: create object node and add adjectival features then
  // add leading possesives and trailing fact-like complements
  obj = nr.MakeNode("obj");
  kind = obj_desc(&fact, obj, spec, nr, blf);
  obj_poss(obj, kind, spec, nr);
  obj_comp(&fact, obj, spec, nr);

  // check for implicit looping mode ("each/every X" or plural noun)
  if ((find == 0) || (find == 1))
  {
    FindSlot(spec, "ENUM", word, 1);
    if (match_any(word, "each", "every") ||
        (((obj->tags & JTAG_NPL) != 0) && (strcmp(word, "any of the") != 0)))
      find = 3;
  }

  // possibly link to existing node else create new graph
  if (after != NULL)
    *after = FragClose(alist);
//  if ((&pool == rule) || (&pool == oper))
    nr.bth = -nr.bth;                                      // allow hypotheticals
//  if ((nr.NumPat() == 1) && (obj->tags == 0))              // for naked "the thing"
//    obj->tags = JTAG_ITEM;                                 //   (but fouls mem_encode.tst)
  ref = nr.FindMake(pool, find, f0, blf, &skolem);

  // if properties being added to old node, return last such property
  if ((find == 3) && (qcnt <= 0) && (skolem != NULL))
    setup_loop(word);                                      // jump structure
  if (f0 == NULL)
    return ref;
  return nr.LookUp(fact);
}


//= Attach any leading possessive phrases to object node in temporary network.
// also assigns base AKO class "kind" to object (if known from obj_desc)

void jhcGraphizer::obj_poss (jhcNetNode *obj, jhcNetNode *kind, const char *alist, jhcNodePool& pool)
{
  jhcNetNode *ref;

  call_list(2, "obj_poss", alist, 0, obj->Nick());

  if (kind != NULL)                                            // add base type from last noun
    kind->AddArg("ako", obj);
  if ((ref = obj_owner(alist, pool)) != NULL)
  {
    if (kind != NULL)
      kind->AddArg("wrt", ref);                                // possible role function
    else
      pool.MakePoss(ref, obj);
  }
}


//= Add any trailing fact-like modifiers (e.g. participles) to temporary network.
// possibly changes "fact" if it adds something

void jhcGraphizer::obj_comp (jhcNetNode **fact, jhcNetNode *obj, const char *alist, jhcNodePool &pool)
{
  char next[200];
  jhcNetNode *ref;

  call_list(2, "obj_comp", alist, 0, obj->Nick());

  if (ExtractBody("%fact-m", next, alist) != NULL) 
    if ((ref = build_fact(NULL, next, pool)) != NULL)
      ref->AddArg("obj", *fact);
  if (ExtractBody("$src", next, alist) != NULL)
    if ((ref = build_obj(NULL, next, pool)) != NULL)
    {
      *fact = pool.AddProp(obj, "src", "from");
      (*fact)->AddArg("ref", ref);
    }

// *** many more complements !!!

}


//= Change last skolem FIND to EACH/ANY and install finished jump.
// sets member variable "loop" and possibly "root"
// always returns find = 3 for convenience

int jhcGraphizer::setup_loop (const char *word)
{
  jhcAliaChain *pick = skolem->Last();
  jhcAliaDir *dir = pick->GetDir();

  call_list(2, "setup_loop", NULL, 0, word);

  // reconfigure most recent FIND directive to be EACH or ANY
  if (strcmp(word, "any") == 0)
    dir->SetKind(JDIR_ANY);
  else
    dir->SetKind(JDIR_EACH);         

  // restart outer loop when no more items
  pick->alt_fail = 0;                            // succeeds if alt = NULL
  pick->alt = loop;
  loop = pick;
  if (root == NULL)
    root = pick;                                 // root = outermost loop
  return 3;
}  


///////////////////////////////////////////////////////////////////////////
//                      Basic Object Description                         //
///////////////////////////////////////////////////////////////////////////

//= Add properties to temporary network for object based on elements in description.
// returns base noun AKO assertion if something found ("last" is last property added)

jhcNetNode *jhcGraphizer::obj_desc (jhcNetNode **last, jhcNetNode *obj, const char *alist, jhcNodePool& pool, double blf)
{
  char next[200];
  jhcNetNode *own, *fact = obj, *kind = NULL;
  const char *val, *tail = alist;
  UL32 t;
  int neg = 0;

  call_list(2, "obj_desc", alist, 0, obj->Nick());

  // examine association list for various properties
  while ((tail = FragNextPair(tail, next)) != NULL)
  {
    // record negation for next property
    if (SlotStart(next, "NEG") > 0)                            
    {
      neg = 1;
      continue;
    }

    // add a property of some type
    if ((val = SlotGet(next, "REF", 0)) != NULL)               // pronoun ("you", "she")
      fact = ref_props(obj, pool, val);
    else if ((val = SlotGet(next, "WRT", 0)) != NULL)          // possessive pronoun ("my")
      fact = ref_props(obj, pool, val);
    else if (SlotStart(next, "NAME") > 0)                      // proper noun ("Jim", "Jim's")
      fact = pool.AddProp(obj, "name", mf.NounLex(t, next), neg, blf);
    else if ((val = mf.NounLex(obj->tags, next)) != NULL)      // noun mod or type ("baseball bat") 
    {
      own = pool.MakeNode("kind", val, neg, blf);
      if (kind != NULL)                                        // previous was noun modifier
        kind->AddArg("of", own);                              
      kind = own;                                              // save as potential base type
    }
    else if (SlotGet(next, "HQ-EST") != NULL)                  // superlative ("biggest")
    {
      fact = pool.AddProp(obj, "hq", mf.AdjLex(t, next), neg, blf);
      fact->AddArg("alt", pool.MakeNode("obj", "all"));
      fact->tags = t;
    } 
    else if (SlotGet(next, "HQ-ER") != NULL)                   // comparison ("taller")
    {
      fact = adj_comp(&tail, obj, mf.AdjLex(t, next), tail, pool, neg, blf);
      fact->tags = t;
    }
    else if ((val = SlotGet(next, "HQ")) != NULL)              // simple property ("big") 
      fact = pool.AddProp(obj, "hq", val, neg, blf);                    
    else if ((val = SlotGet(next, "DEG")) != NULL)             // degree property ("very red")
      fact = obj_deg(&tail, obj, val, tail, pool, neg, blf);            
    else if (SlotStart(next, "ACT-G") > 0)                     // participle ("sleeping")
    {
      fact = pool.MakeAct(mf.VerbLex(t, next), neg, blf);
      fact->AddArg("agt", obj);
      fact->tags = t;
    }
    else if ((val = SlotGet(next, "HAS")) != NULL)             // part description ("with a red top")
      fact = obj_has(&tail, obj, val, tail, pool, neg, blf);            
    else if (SlotStart(next, "LOC") > 0)                       // location phrase ("at home")
      fact = add_place(&tail, obj, next, tail, pool, neg, blf);           

    // property always eats any pending negation
    neg = 0;
  }

  // bind last fact added and return kind for object (if known)
  if (last != NULL)
   *last = fact;
  return kind;
}


//= Add properties to object node based on pronoun used for reference.
// returns object node for convenience

jhcNetNode *jhcGraphizer::ref_props (jhcNetNode *n, jhcNodePool& pool, const char *pron) const
{
  call_list(3, "ref_props", NULL, 0, pron);

  // specify conversational role (direct match of unique lex)
  if (match_any(pron, "you", "your", "yours"))
    pool.SetLex(n, "me");                                  // robot (swapped perspective)
  else if (match_any(pron, "me", "I", "my", "mine"))
    pool.SetLex(n, "you");                                 // human (swapped perspective)

  // add grammar tag to help with naked FINDs
  if (match_any(pron, "she", "her", "hers"))
    n->tags = JTAG_FEM;
  else if (match_any(pron, "he", "him", "his"))
    n->tags = JTAG_MASC;
  else if (match_any(pron, "it", "its", "they", "them", "their"))
    n->tags = JTAG_ITEM;
  else if (strcmp(pron, "here") == 0)
    n->tags = JTAG_HERE;
  else if (strcmp(pron, "there") == 0)
    n->tags = JTAG_THERE;
  return n;
}


//= Object has some property relative to some other object (e.g. "bigger").
// returns propery assertion, can also give unused portion of alist (after)

jhcNetNode *jhcGraphizer::adj_comp (const char **after, jhcNetNode *obj, const char *comp, const char *alist, 
                                    jhcNodePool& pool, int neg, double blf) 
{
  jhcNetNode *ref, *prop;
  const char *tail;

  call_list(3, "adj_comp", alist, 0, comp);

  if ((ref = build_obj(&tail, alist, pool)) == NULL)
    ref = pool.MakeNode("obj");                            // always required
  prop = pool.AddProp(obj, "hq", comp, neg, blf);
  prop->AddArg("alt", ref);                      
  if (after != NULL)
    *after = tail;
  return prop;
}


//= Make nodes for adjectival descriptions with a degree ("very red" or "very close").
// returns propery assertion, can also give unused portion of alist (after)

jhcNetNode *jhcGraphizer::obj_deg (const char **after, jhcNetNode *obj, const char *amt, const char *alist, 
                                   jhcNodePool& pool, int neg, double blf) 
{
  char pair[200];
  jhcNetNode *prop, *mod;
  const char *val, *tail;

  call_list(3, "obj_deg", alist, 0, amt);

  // figure out what kind of relation is being given a degree
  if (after != NULL)
    *after = alist;
  if ((tail = FragNextPair(alist, pair)) == NULL)
    return NULL;

  // modify a new adjectival descriptor 
  if ((val = SlotGet(pair, "HQ")) != NULL)
  {
    mod = pool.AddDeg(obj, "hq", val, amt, neg, blf);
    if (after != NULL)
      *after = tail;
    return mod;
  }

  // modify a location descriptor (gets most recent "loc" property)
  if ((val = SlotGet(pair, "LOC")) != NULL)
  {
//    prop = add_place(&tail, obj, pair, tail, pool, neg, 0.0);
    prop = add_place(&tail, obj, pair, tail, pool, neg, blf);
    mod = pool.AddProp(prop, "deg", amt, neg, blf);
    if (after != NULL)
      *after = tail;
    return mod;
  }

  // unknown description type
  if (after != NULL)
    *after = tail;
  return NULL;
}


//= Make nodes for part phrases ("with a red top").
// returns possession relation, can also give unused portion of alist

jhcNetNode *jhcGraphizer::obj_has (const char **after, jhcNetNode *obj, const char *prep, const char *alist, 
                                   jhcNodePool& pool, int neg, double blf) 
{
  jhcNetNode *part;
  const char *tail;

  call_list(3, "obj_has", alist, 0, prep);

  // check for required part
  if (after != NULL)
    *after = alist;
  if ((part = build_obj(&tail, alist, pool)) == NULL)
    return NULL;

  // build required relation
  if (after != NULL)
   *after = tail;
  return pool.MakePoss(obj, part, neg, blf);
}


//= Make nodes for location phrases ("at home" or "between here and there").
// can be used with both NPs and VPs
// returns location assertion, can also give unused portion of alist (after)

jhcNetNode *jhcGraphizer::add_place (const char **after, jhcNetNode *obj, char *pair, const char *alist, 
                                     jhcNodePool& pool, int neg, double blf)
{
  jhcNetNode *ref, *prop;
  const char *t2, *tail, *rel = SlotGet(pair, "LOC");

  call_list(3, "add_place", alist, 0, pair);

  // add basic relation (naked if destination of verb)
  if (after != NULL)
    *after = alist;
  prop = pool.MakeNode("loc", rel, neg, blf);
  if (obj != NULL)
    prop->AddArg("loc", obj);

  // check if anchor object required (not needed for "here" or "in front" or "down")
  if ((SlotStart(pair, "LOC-0") > 0) || (SlotStart(pair, "LOC-V") > 0))
    return prop;
  if ((ref = build_obj(&tail, alist, pool)) == NULL)
    return prop;
  prop->AddArg("ref", ref);                        // add reference object 
  if (after != NULL)
    *after = tail;

  // check if second anchor expected (e.g. "between")
  if (SlotStart(pair, "LOC-2") <= 0)
    return prop;
  if ((ref = build_obj(&t2, tail, pool)) == NULL)
    return prop;
  prop->AddArg("ref2", ref);                       // add second reference  
  if (after != NULL)
    *after = t2;
  return prop;
}


///////////////////////////////////////////////////////////////////////////
//                         Copula Interpretation                         //
///////////////////////////////////////////////////////////////////////////

//= Check for copula tail end (e.g. "is nice") and add features to node.
// features added directly since never need to check for a reference for this description
// if "pos" > 0 forces interpretation to be positive (i.e. can ignore <NEG> for unless clauses)
// returns last assigned property, can also give unused portion of alist (after)

jhcNetNode *jhcGraphizer::add_cop (const char **after, jhcNetNode *obj, const char *alist, jhcNodePool& pool, int pos) 
{
  char next[200], body[amax], frag[200];
  jhcNetNode *f2, *kind, *fact = NULL;
  const char *val, *post, *tail;
  double blf = 1.0;                     // was 0.0
  UL32 t;
  int neg = 0, cr0 = create;  

  call_list(1, "add_cop", alist, 0, obj->Nick());

  // if following part is an addition then get first pair
  if ((tail = SplitFrag(next, body, alist)) == NULL)
    return NULL;
  if (after != NULL)
    *after = tail;

  // get overall negation and belief
  tail = body;
  while ((tail = FragNextPair(tail, next)) != NULL)
  {
    if ((val = SlotGet(next, "BLF")) != NULL)                  // overall belief ("usually")
      blf = belief_val(val);                             
    else if ((SlotStart(next, "NEG") > 0) && (pos <= 0))       // overall negation ("not")
      neg = 1;                                           
  }

  // check for NAKED possessive phrase ("the bowl is Jon's dog's")
  if ((f2 = obj_owner(body, pool)) != NULL)
  {
    if ((kind = obj->Fact("ako")) != NULL)
    {
      fact = pool.MakeNode("kind", kind->LexStr(), neg, blf);  // never change original  
      fact->AddArg("ako", obj);
      fact->AddArg("wrt", f2);                             
    }
    else
      fact = pool.MakePoss(f2, obj, neg, blf);
    if (after != NULL)
      *after = FragClose(tail, 0);
    return fact;
  }

  // check for $src prepositional phrase
  if ((tail = SplitFrag(next, frag, body)) != NULL)
    if (strcmp(next, "$src") == 0)
      if ((f2 = build_obj(&tail, frag, pool)) != NULL)
      {
        fact = pool.AddProp(obj, "src", "from", neg, blf);
        fact->AddArg("ref", f2);
        if (after != NULL)
          *after = FragClose(tail, 0);
        return fact;
      }

  // go through all the pairs in this fragment
  post = body;
  tail = body;
  while ((tail = FragNextPair(tail, next)) != NULL)
  {
    if ((val = SlotGet(next, "NAME", 0)) != NULL)                // proper name ("Groot")
      fact = pool.AddProp(obj, "name", val, neg, blf);   
    else if ((val = SlotGet(next, "HQ-EST")) != NULL)            // superlative ("biggest")
    {
      fact = pool.AddProp(obj, "hq", mf.AdjLex(t, next), neg, blf);
      fact->AddArg("alt", pool.MakeNode("obj", "all"));
      fact->tags = t;
    } 
    else if ((val = SlotGet(next, "HQ-ER")) != NULL)             // comparative ("bigger than")
    {
      fact = adj_comp(&tail, obj, mf.AdjLex(t, next), tail, pool, neg, blf);     
      fact->tags = t;
    }
    else if ((val = SlotGet(next, "HQ")) != NULL)                // simple property ("big") 
      fact = pool.AddProp(obj, "hq", val, neg, blf);            
    else if ((val = SlotGet(next, "DEG")) != NULL)               // degree property ("very red")
      fact = obj_deg(&tail, obj, val, tail, pool, neg, blf);
    else if (SlotStart(next, "LOC") > 0)                         // location phrase ("at home")    
      fact = add_place(&tail, obj, next, tail, pool, neg, blf);   
    else if (SlotStart(next, "ACT-G") > 0)                       // participle ("sleeping")
    {
      fact = pool.MakeAct(mf.VerbLex(t, next), neg, blf);
      fact->AddArg("agt", obj);
      fact->tags = t;
    }
    post = tail;
  }

  // see if fragments left after discrete properties handled
  if (*post != '\0')
  {
    // check for a more complex participial phrase
    if ((tail = FindFrag(post, "%fact-g")) != NULL)
      fact = build_fact(NULL, tail, pool, obj);

    // check for super-type declaration ("a kind of dog")
    if (nsuper_kind(next, 200, post) != NULL)
      if ((f2 = pool.AddProp(obj->AnyFact("ako"), "ako", next)) != NULL)
        return f2;

    // check for indeterminate predicate nominal ("a dog") which is always hypothetical
    create = 1;
    if ((f2 = build_obj(NULL, post, pool, obj, blf)) != NULL)    // used to take neg
    {
      create = cr0;
      if ((kind = f2->AnyFact("ako")) != NULL)                   // base kind assertion 
      {
        kind->SetNeg(neg);                                       // "tigers are not dogs"
        return kind;
      }
      return f2;
    }
  }
  return fact;
}


//= Generate skolem FIND directives for a chain of %obj-p possessive fragments.
// returns the ultimate owner for the object which is being modified with <desc-p>

jhcNetNode *jhcGraphizer::obj_owner (const char *alist, jhcNodePool& pool)
{
  char next[200], poss[200];
  const char *tail = alist;
  jhcNetNode *item, *spec, *owner = NULL;
  jhcGraphlet *old;

  call_list(3, "obj_owner", alist);

  // make up skolem FIND for each object description
  while ((tail = ExtractFrag(next, poss, tail)) != NULL)
    if (strncmp(next, "%obj-p", 6) == 0)
      if ((item = build_obj(NULL, poss, pool)) != NULL)
      {
        if (owner != NULL)
        {
          // add to description owner from previous possessive
          if ((spec = item->Fact("ako")) != NULL)
            spec->AddArg("wrt", owner);          // possible role function
          else
          {
            old = pool.BuildIn(skolem->LastKey());
            pool.MakePoss(owner, item);
            pool.BuildIn(old);
          }
        }
        owner = item;                            // becomes owner of next item
      }
  return owner;
}


//= Turn qualifier ("usually") into numeric belief value.

double jhcGraphizer::belief_val (const char *word) const
{
  char term[13][20] = {"definitely", "always", "certainly", "usually",  "probably",     "likely", 
                          "may",     "might",  "sometimes", "possibly", "occasionally", "unlikely to be", "seldom"};
  double val[13]    = {    1.2,        1.2,       1.1,         0.9,         0.8,           0.7,     
                           0.5,        0.5,       0.5,         0.3,         0.3,           0.1,             0.1   };
  int i;

  call_list(3, "belief_val", NULL, 0, word);

  for (i = 0; i < 13; i++)
    if (strcmp(word, term[i]) == 0)
      return val[i];
  return 1.0;
} 


//= Extract the noun kind associated with a super-kind element.
// returns NULL if fails, else binds kind and returns remainder of list

const char *jhcGraphizer::nsuper_kind (char *kind, int ssz, const char *alist) const
{
  char entry[80] = "";
  const char *tail, *val;

  call_list(3, "nsuper_kind", alist, 0, kind);

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


///////////////////////////////////////////////////////////////////////////
//                            Number Strings                             //
///////////////////////////////////////////////////////////////////////////

//= Convert a textual description of an integer into a numeric string 0-99.
// txt = "twenty two" -> cnt = "22" and txt = "-3.14" -> cnt = "3"
// Note: simplified form derived from class jhcParseNum

const char *jhcGraphizer::parse_int (char *cnt, const char *txt, int ssz) const
{
  char t[8][40] = {"twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"};
  char d[19][40] = {"one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", 
                    "eleven", "twelve", "thirteen", "fourteen", "fifteen", 
                    "sixteen", "seventeen", "eighteen", "nineteen"};
  char first[80];
  char *second;
  double fnum;
  int i, j, val = 0;

  call_list(3, "parse_int", NULL, 0, txt);

  // check for existing numeric version (possibly a signed float)
  if (sscanf_s(txt, "%lf", &fnum) == 1)
  {
    val = abs((int) fnum);
    val = __min(val, 99);
    sprintf_s(cnt, ssz, "%d", val);
    return cnt;
  }

  // check for 1-19 (and ignore any other words)
  strcpy_s(first, txt);
  if ((second = strchr(first, ' ')) != NULL)
    *second++ = '\0';
  for (i = 0; i < 19; i++)
    if (strcmp(first, d[i]) == 0)
    {
      sprintf_s(cnt, ssz, "%d", i + 1);
      return cnt;
    }

  // check for tens digit in 20-99
  for (i = 0; i < 8; i++)
    if (strcmp(first, t[i]) == 0)
    {
      // check for ones digit (if any)
      val = 10 * (i + 2);
      if (second != NULL)
        for (j = 0; j < 9; j++)
          if (strcmp(second, d[j]) == 0)
          {
            val += (j + 1);
            break;
          }
      break;
    }

  // generate string for value (might be zero)
  sprintf_s(cnt, ssz, "%d", val);
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                              Utilities                                //
///////////////////////////////////////////////////////////////////////////

//= See if some string matches to any of the listed variants.
// assumes strings txt, val, and val2 are non-NULL

bool jhcGraphizer::match_any (const char *txt, const char *val, const char *val2, const char *val3, 
                              const char *val4, const char *val5, const char *val6) const
{
  return((strcmp(txt, val) == 0) || (strcmp(txt, val2) == 0) || 
         ((val3 != NULL) && (strcmp(txt, val3) == 0)) || ((val4 != NULL) && (strcmp(txt, val4) == 0)) ||
         ((val5 != NULL) && (strcmp(txt, val5) == 0)) || ((val6 != NULL) && (strcmp(txt, val6) == 0)));
}


//= General conditional debugging message removes tabs from alist.
// skip: -1 = full alist, 0 = trim at next frag, 1 = trim at end of this frag 
// mostly used by jhcGraphizer

void jhcGraphizer::call_list (int lvl, const char *fcn, const char *alist, int skip, const char *entry) const
{
  if (dbg < lvl)
    return;
  if (entry == NULL)
    jprintf("%s\n  ", fcn);
  else
    jprintf("%s [%s]\n  ", fcn, entry);
  PrintList(alist, NULL, skip);
  if ((alist != NULL) && (*alist != '\0'))
    jprintf("\n");
}




