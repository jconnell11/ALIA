// jhcNetBuild.cpp : turns parser alist into network structures
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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
// return: 7 = farewell, 6 = greet, 5 = hail, 
//         4 = op, 3 = rule, 2 = command, 1 = fact, 
//         0 = nothing, negative for error

int jhcNetBuild::Convert (const char *alist)
{
  int ans;

  // sanity check then cleanup any rejected suggestions
  if (core == NULL) 
    return -1;
  ClearLast();
  if ((alist == NULL) || (*alist == '\0'))               
    return huh_tag();                        // misheard utterance

  // generate core interpretation then add speech act
  ans = Assemble(alist);
  if ((ans == 1) || (ans == 2))
    return attn_tag(alist);                  // fact or command
  if ((ans == 3) || (ans == 4))
    return add_tag(ans, alist);              // new rule or operator

  // handle superficial speech acts
  if (HasSlot(alist, "HELLO"))               // simple greeting
    return greet_tag();
  if (HasSlot(alist, "BYE"))                 // simple farewell
    return farewell_tag();
  if (HasSlot(alist, "ATTN"))                // calling robot name
    return hail_tag();
  return huh_tag();                          // no network created
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
  jhcAliaAttn *attn = &(core->attn);
  jhcAliaChain *ch = new jhcAliaChain;
  jhcAliaDir *dir = new jhcAliaDir();
  jhcNetNode *n;
 
  // fill in details of the speech act
  attn->BuildIn(&(dir->key));
  n = attn->MakeNode("meta", "understand", 1);
  n->AddArg("agt", attn->self);
  n->AddArg("obj", attn->user);
  n->SetDone(1);

  // add completed structure to attention buffer
  ch->BindDir(dir);
  attn->AddFocus(ch);
  attn->BuildIn(NULL);
  return 0;
}


//= Generate speech act noting that the robot's name was called.

int jhcNetBuild::hail_tag () const
{
  jhcAliaAttn *attn = &(core->attn);
  jhcAliaChain *ch;
  jhcNetNode *input; 

  // make a new NOTE directive
  ch = build_tag(&input, NULL);
  attn->AddLex(input, "hail");

  // add completed structure to attention buffer
  attn->AddFocus(ch);
  attn->BuildIn(NULL);
  return 5;
}


//= Generate speech act noting that the user wants to communicate.

int jhcNetBuild::greet_tag () const
{
  jhcAliaAttn *attn = &(core->attn);
  jhcAliaChain *ch;
  jhcNetNode *input; 

  // make a new NOTE directive
  ch = build_tag(&input, NULL);
  attn->AddLex(input, "greet");

  // add completed structure to attention buffer
  attn->AddFocus(ch);
  attn->BuildIn(NULL);
  return 6;
}


//= Generate speech act noting that the user is leaving.

int jhcNetBuild::farewell_tag () const
{
  jhcAliaAttn *attn = &(core->attn);
  jhcAliaChain *ch;
  jhcNetNode *input; 

  // make a new NOTE directive
  ch = build_tag(&input, NULL);
  attn->AddLex(input, "dismiss");

  // add completed structure to attention buffer
  attn->AddFocus(ch);
  attn->BuildIn(NULL);
  return 7;
}


//= Generate speech act followed by a request to add rule or operator.
// returns 3 for rule, 4 for operator (echoes input "kind")

int jhcNetBuild::add_tag (int kind, const char *alist) const
{
  jhcAliaAttn *attn = &(core->attn);
  jhcAliaChain *ch, *add;
  jhcAliaDir *dir;
  jhcNetNode *input, *r;
 
  // make a new NOTE directive
  ch = build_tag(&input, alist);
  attn->AddLex(input, "give");
  r = attn->MakeNode("obj");
  input->AddArg("obj", r);
  attn->AddProp(r, "ako", ((kind == 3) ? "rule" : "operator"));

  // tack on a generic ADD directive at the end
  add = new jhcAliaChain;
  dir = new jhcAliaDir(JDIR_ADD);
  add->BindDir(dir);
  ch->cont = add;

  // add completed structure to attention buffer
  attn->AddFocus(ch);
  attn->BuildIn(NULL);
  return kind;
}


//= Insert NOTE directive about source of command or fact before actual statement.
// gives the opportunity to PUNT and disbelief fact or reject command
// returns 2 for command, 1 for fact, 0 for problem

int jhcNetBuild::attn_tag (const char *alist) const
{
  jhcAliaAttn *attn = &(core->attn);
  const jhcAliaDir *d0 = bulk->GetDir();
  jhcAliaChain *ch;
  jhcNetNode *input; 

  // make a new NOTE directive before bulk
  ch = build_tag(&input, alist);
  ch->cont = bulk;
  attn_args(input, bulk);

  // add completed structure to attention buffer
  attn->AddFocus(ch);
  attn->BuildIn(NULL);
  if ((d0 != NULL) && (d0->Kind() == JDIR_NOTE))     // might be play
    return 1;
  return 2;
}


//= Build a chain consisting of a single NOTE directive about speech act.
// also returns pointer to main assertion in directive
// leaves jhcGraphlet accumulator of WMEM assigned to this directive

jhcAliaChain *jhcNetBuild::build_tag (jhcNetNode **node, const char *alist) const
{
  jhcAliaAttn *attn = &(core->attn);
  jhcAliaChain *ch = new jhcAliaChain;
  jhcAliaDir *dir = new jhcAliaDir();
  jhcNetNode *n;
 
  // fill in details of the speech act
  attn->BuildIn(&(dir->key));
  n = attn->MakeNode("meta");
  n->AddArg("agt", attn->user);
  n->AddArg("dest", attn->self);
  if ((alist != NULL) && HasSlot(alist, "POLITE"))
    attn->AddProp(n, "mod", "polite");
  n->SetDone(1);

  // embed in chain then return pieces
  ch->BindDir(dir);
  if (node != NULL)
    *node = n;
  return ch;
}


//= Add important parts of bulk as arguments to what user told.

void jhcNetBuild::attn_args (jhcNetNode *input, const jhcAliaChain *bulk) const
{
  jhcAliaAttn *attn = &(core->attn);
  const jhcAliaChain *ch = bulk;
  const jhcAliaDir *d = bulk->GetDir();
  const jhcGraphlet *g;
  jhcNetNode *n;
  JDIR_KIND kind = JDIR_MAX;
//  int i, ni = 0, ask = 0;
  int ask = 0;
  
  // mark overall type of input
  if (d != NULL)
  {
    // determine whether this is a question
    kind = d->Kind();
    if ((kind == JDIR_CHK) || (kind == JDIR_FIND))
      ask = 1;

    // get attributes of directive key
    g = &(d->key);
    n = g->Main();
//    ni = g->NumItems();
  }
  if (ask > 0)
    attn->AddLex(input, "ask");
  else
    attn->AddLex(input, "tell");

  // see if question, fact, or string of commands
  if (kind == JDIR_CHK)
     input->AddArg("ynq", n);      // yes-no question
  else if (kind == JDIR_FIND)
     input->AddArg("whq", n);      // wh- question
  else if (kind == JDIR_NOTE)
     input->AddArg("obj", attn->MakeNode("data"));
  /*
    for (i = 0; i < ni; i++)
    {
      // add all predicates to what was told (including lone lex)
      n = g->Item(i);
      if ((n->NumArgs() > 0) && ((ni == 1) || !n->LexNode()))
        input->AddArg("obj", n);
    }
  */
  else
    while (ch != NULL)
    {
      // add all explicit actions to what was requested
      if ((d = ch->GetDir()) != NULL)
        if (d->Kind() == JDIR_DO)
          input->AddArg("cmd", (d->key).Main());
      ch = ch->cont;
    }
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

