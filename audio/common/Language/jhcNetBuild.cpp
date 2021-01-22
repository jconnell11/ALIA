// jhcNetBuild.cpp : turns parser alist into network structures
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

#include "Action/jhcAliaCore.h"        // common robot - only spec'd as class in header
#include "Action/jhcAliaPlay.h"

#include "Language/jhcNetBuild.h"


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= See if attention (to speech) should be renewed based on association list.
// basically looks for the presense of the ATTN non-terminal category
// mode: 0 = always, 1 = ATTN anywhere, 2 = ATTN at start, 3 = ATTN only (hail)

int jhcNetBuild::NameSaid (const char *alist, int mode) const
{
  const char *tail;

  // ignore wake-up for text input
  if (mode <= 0)
    return 1;

  // must have vocative somewhere
  if (!HasSlot(alist, "ATTN"))
    return 0;
  if (mode == 1)
    return 1;

  // must have vocative at beginning
  if ((tail = NextMatches(alist, "ATTN", 4)) == NULL)
    return 0;
  if (mode == 2)
    return 1;

  // must have no other entries
  if (*tail != '\0')
    return 0;
  return 1;
}


//= Build an appropriate structure based on given association list.
// return: 8 = farewell, 7 = greet, 6 = hail, 
//         5 = op, 4 = rule, 3 = question, 2 = command, 1 = fact, 
//         0 = nothing, negative for error

int jhcNetBuild::Convert (const char *alist)
{
  int spact;

  // sanity check then cleanup any rejected suggestions
  if (core == NULL) 
    return -1;
  add = NULL;                                // deleted elsewhere
  ClearLast();
  if ((alist == NULL) || (*alist == '\0'))               
    return huh_tag();                        // misheard utterance

  // generate core interpretation then add speech act
  spact = Assemble(alist);
  if ((spact >= 1) && (spact <= 3))
    return attn_tag(spact, alist);           // fact or command
  if ((spact >= 4) && (spact <= 5))
    return add_tag(spact, alist);            // new rule or operator

  // handle superficial speech acts
  if (HasSlot(alist, "HELLO"))               // simple greeting
    return greet_tag();
  if (HasSlot(alist, "BYE"))                 // simple farewell
    return farewell_tag();
  if (HasSlot(alist, "ATTN"))                // calling robot name
    return hail_tag();
  return huh_tag();                          // no network created
}


//= Record a summary of last sentence conversion process.
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
  if (spact == 8)
    fprintf(log, "-- farewell --\n\n");
  else if (spact == 7)
    fprintf(log, "-- greeting --\n\n");
  else if (spact == 6)
    fprintf(log, "-- hail --\n\n");
  else if ((spact == 5) && (add != NULL) && (add->new_oper != NULL))
    (add->new_oper)->Save(log);
  else if ((spact == 4) && (add != NULL) && (add->new_rule != NULL))
    (add->new_rule)->Save(log);
  else if ((spact <= 3) && (spact >= 1) && (bulk != NULL))
  {
    bulk->Save(log);
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
  jhcAliaDir *dir = new jhcAliaDir();
  jhcNetNode *n;
 
  // fill in details of the speech act
  atree->BuildIn(&(dir->key));
  n = atree->MakeNode("meta", "understand", 1);
  n->AddArg("agt", atree->Robot());               // in WMEM since NOTE
  n->AddArg("obj", atree->Human());               // in WMEM since NOTE
//  n->SetDone(1);

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
  jhcAliaChain *ch;
  jhcNetNode *input; 

  // make a new NOTE directive
  ch = build_tag(&input, NULL);
  atree->AddLex(input, "hail");

  // add completed structure to attention buffer
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return 6;
}


//= Generate speech act noting that the user wants to communicate.

int jhcNetBuild::greet_tag () const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch;
  jhcNetNode *input; 

  // make a new NOTE directive
  ch = build_tag(&input, NULL);
  atree->AddLex(input, "greet");

  // add completed structure to attention buffer
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return 7;
}


//= Generate speech act noting that the user is leaving.

int jhcNetBuild::farewell_tag () const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch;
  jhcNetNode *input; 

  // make a new NOTE directive
  ch = build_tag(&input, NULL);
  atree->AddLex(input, "dismiss");

  // add completed structure to attention buffer
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return 8;
}


//= Generate speech act followed by a request to add rule or operator.
// save core of ADD directive in "add" for convenience
// returns 4 for rule, 5 for operator (echoes input "kind")

int jhcNetBuild::add_tag (int spact, const char *alist) 
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch, *ch2;
  jhcNetNode *input, *r;
 
  // make a new NOTE directive
  ch = build_tag(&input, alist);
  atree->AddLex(input, "give");
  r = atree->MakeNode("obj");
  input->AddArg("obj", r);
  atree->AddProp(r, "ako", ((spact == 4) ? "rule" : "operator"));

  // tack on a generic ADD directive at the end
  ch2 = new jhcAliaChain;
  add = new jhcAliaDir(JDIR_ADD);
  ch2->BindDir(add);
  ch->cont = ch2;

  // move newly create rule or operator into directive (in case slow)
  if (spact == 4)
    add->new_rule = rule;
  else
    add->new_oper = oper;
  rule = NULL;               // prevent deletion by jhcGraphizer
  oper = NULL;

  // add completed structure to attention buffer
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return spact;
}


//= Insert NOTE directive about source of command or fact before actual statement.
// gives the opportunity to PUNT and disbelief fact or reject command
// returns 3 for question, 2 for command, 1 for fact, 0 for problem

int jhcNetBuild::attn_tag (int spact, const char *alist) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch;
  jhcNetNode *input;

  // make a new NOTE directive before bulk
  ch = build_tag(&input, alist);
  ch->cont = bulk;

  // specialize based on speech act
  atree->AddLex(input, ((spact <= 2) ? "tell" : "ask"));
  if (spact == 1)
    input->AddArg("obj", atree->MakeNode("fact"));
  else if (spact == 2)
    input->AddArg("cmd", atree->MakeNode("cmd"));
  else
    input->AddArg("obj", atree->MakeNode("query"));

  // add completed structure to attention buffer
  atree->AddFocus(ch);
  atree->BuildIn(NULL);
  return spact;
}


//= Build a chain consisting of a single NOTE directive about speech act.
// also returns pointer to main assertion in directive
// leaves jhcGraphlet accumulator of WMEM assigned to this directive

jhcAliaChain *jhcNetBuild::build_tag (jhcNetNode **node, const char *alist) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch = new jhcAliaChain;
  jhcAliaDir *dir = new jhcAliaDir();
  jhcNetNode *n;
 
  // fill in details of the speech act
  atree->BuildIn(&(dir->key));
  n = atree->MakeNode("meta");
  n->AddArg("agt", atree->Human());                 // in WMEM since NOTE
  n->AddArg("dest", atree->Robot());                // in WMEM since NOTE
  if ((alist != NULL) && HasSlot(alist, "POLITE"))
    atree->AddProp(n, "mod", "polite");
//  n->SetDone(1);

  // embed in chain then return pieces
  ch->BindDir(dir);
  if (node != NULL)
    *node = n;
  return ch;
}


///////////////////////////////////////////////////////////////////////////
//                               Utilities                               //
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
          if ((*wds == '\0') || (*wds == '\n'))
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
        else if (strncmp(node, "agt", 3) == 0)
          save_word(tag, nt, term);
        else if (strncmp(node, "act", 3) == 0)
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

