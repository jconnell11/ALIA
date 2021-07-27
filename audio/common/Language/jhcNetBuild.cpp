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

#include "Parse/jhcTxtLine.h"          // common audio

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
    (add->new_oper)->Save(log, 2);
  else if ((spact == 4) && (add != NULL) && (add->new_rule != NULL))
    (add->new_rule)->Save(log, 2);
  else if ((spact <= 3) && (spact >= 1) && (bulk != NULL))
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
  jhcAliaDir *dir = new jhcAliaDir();
  jhcNetNode *n;
 
  // fill in details of the speech act
  atree->BuildIn(&(dir->key));
  n = atree->MakeNode("meta", "understand", 1);
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
  jhcAliaChain *ch;
  jhcNetNode *input; 

  // make a new NOTE directive
  ch = build_tag(&input, NULL);
  atree->SetLex(input, "hail");

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
  atree->SetLex(input, "greet");

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
  atree->SetLex(input, "dismiss");

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
  atree->SetLex(input, "give");
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
// gives the opportunity to PUNT and disbelieve fact or reject command
// returns 3 for question, 2 for command, 1 for fact, 0 for problem

int jhcNetBuild::attn_tag (int spact, const char *alist) const
{
  jhcActionTree *atree = &(core->atree);
  jhcAliaChain *ch;
  jhcNetNode *input, *dummy;

  // make a new NOTE directive before bulk
  ch = build_tag(&input, alist);
  ch->cont = bulk;

  // specialize based on speech act then point to payload
  atree->SetLex(input, ((spact <= 2) ? "tell" : "ask"));
//  bulk->RefSteps(input, ((spact == 2) ? "cmd" : "obj"), *atree);
  dummy = atree->MakeNode((spact == 2) ? "act" : "fact");
  input->AddArg(((spact == 2) ? "cmd" : "obj"), dummy);
 
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

  // embed in chain then return pieces
  ch->BindDir(dir);
  if (node != NULL)
    *node = n;
  return ch;
}


///////////////////////////////////////////////////////////////////////////
//                               Utilities                               //
///////////////////////////////////////////////////////////////////////////

//= Read a file of potentential property values and makes auxilliary files.
// read from "kern.vals" with format:
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
// returns number of categories read and generates starter files "kern0.rules" and "kern0.ops"
// Note: these output files can be further processed with HarvestLex to give a starter ".sgm" file

int jhcNetBuild::AutoVals (const char *kern, const char *fcn) const
{
  jhcTxtLine txt;
  char fname[200], hi[40], lo[40], cat[40] = "";
  FILE *rules, *ops; 
  const char *val, *base;
  int nr = 0, np = 0, nc = 0;

  // open input and output files
  if (kern == NULL)
    return 0;
  sprintf_s(fname, "%s.vals", kern);
  if (!txt.Open(fname))
    return 0;
  sprintf_s(fname, "%s0.rules", kern);
  if (fopen_s(&rules, fname, "w") != 0)
    return txt.Close();
  sprintf_s(fname, "%s0.ops", kern);
  if (fopen_s(&ops, fname, "w") != 0)
  {
    fclose(rules);
    return txt.Close();
  }

  // write output file headers
  if ((val = strrchr(kern, '\\')) == NULL)
    val = kern;
  if ((base = strrchr(val, '/')) == NULL)
    base = val;
  fprintf(rules, "// Standard inferences for \"%s\" kernel\n", base + 1);
  fprintf(ops,   "// Basic interface to \"%s\" kernel\n", base + 1);

  // look for non-comment input lines with category prefix
  strcpy_s(fname, ((fcn != NULL) ? fcn : "vis"));
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

      // add delimiters to output files between categories
      fprintf(rules, "// ================================================\n\n");
      fprintf(ops,   "// ================================================\n\n");

      // create basic rules and ops for category
      if ((*lo != '\0') && (*hi != '\0'))
      {
        nr = range_rules(rules, cat, lo, hi, nr);
        np = range_ops(ops, fname, cat, np);
      }
      else
        np = list_ops(ops, fname, cat, np);
    }
    else if ((*lo == '\0') || (*hi == '\0'))
      nr = value_rules(rules, cat, txt.Token(), nr);
    else if (txt.Begins("-"))
      nr = alias_rules(rules, cat, lo, txt.Token() + 1, nr);
    else if (txt.Begins("+"))
      nr = alias_rules(rules, cat, hi, txt.Token() + 1, nr);
    txt.Next(1);  
  }    

  // cleanup
  fclose(ops);
  fclose(rules);
  txt.Close();
  return nc;
}


//= Create basic rules for interpreting values that are part of some category's range.

int jhcNetBuild::range_rules (FILE *out, const char *cat, const char *lo, const char *hi, int n) const
{
  char mid[40];
  int nr = n;

  // make values member of the category
  sprintf_s(mid, "medium %s", hi);
  nr = value_rules(out, cat, lo,  nr);
  nr = value_rules(out, cat, mid, -nr);
  nr = value_rules(out, cat, hi,  nr);
  fprintf(out, "// ------------------------------------------------\n\n");

  // base values are mutually exclusive
  nr = mutex_rule(out, lo,  mid, nr);
  nr = mutex_rule(out, lo,  hi,  nr);
  nr = mutex_rule(out, mid, lo,  nr);
  nr = mutex_rule(out, mid, hi,  nr);
  nr = mutex_rule(out, hi,  lo,  nr);
  nr = mutex_rule(out, hi,  mid, nr);
  return nr;
}


//= Assign value to this category and make category equivalent to "ness" version of value.
// always skips "ness" part if n is negative (e.g. for "medium big")

int jhcNetBuild::value_rules (FILE *out, const char *cat, const char *val, int n) const
{
  char ness[80];
  int nr = abs(n);

  // membership rule
  fprintf(out, "// RULE %d - %c%s is a %s\n", nr + 1, toupper(val[0]), val + 1, cat);
  fprintf(out, "  if:  hq-1 -lex-  %s\n", val);
  fprintf(out, "            -hq--> obj-1\n");
  fprintf(out, "then: ako-1 -lex-  %s\n", cat);
  fprintf(out, "            -ako-> hq-1\n\n");

  // create artificial category from value ("wide" -> "wideness")
  if (n < 0)
    return(nr + 1);
  mf.PropKind(ness, val);
  if (strcmp(ness, cat) == 0)
    return(nr + 1);
  
  // search rule
  fprintf(out, "// RULE %d - A %s is a %s\n", nr + 2, ness, cat);   
  fprintf(out, "  if: ako-1 -lex-  %s\n", ness);
  fprintf(out, "            -ako-> hq-1\n");
  fprintf(out, "then: ako-2 -lex-  %s\n", cat);
  fprintf(out, "            -ako-> hq-1\n\n");

  // result conversion rule
  fprintf(out, "// RULE %d - A %s is a %s\n", nr + 3, cat, ness);
  fprintf(out, "  if: ako-1 -lex-  %s\n", cat);
  fprintf(out, "            -ako-> hq-1\n");
  fprintf(out, "then: ako-2 -lex-  %s\n", ness);
  fprintf(out, "            -ako-> hq-1\n\n");
  return(nr + 3);
}


//= Assert that if the property has this value it cannot be the alternative value.

int jhcNetBuild::mutex_rule (FILE *out, const char *val, const char *alt, int n) const
{
  fprintf(out, "// RULE %d - If something is %s then it is not %s\n", n + 1, val, alt);
  fprintf(out, "  if: hq-1 -lex-  %s\n", val);
  fprintf(out, "           -hq--> obj-1\n");
  fprintf(out, "then: hq-2 -lex-  %s\n", alt);
  fprintf(out, "           -neg-  1\n");
  fprintf(out, "           -hq--> obj-1\n\n");
  return(n + 1);
}


//= Define two adjectival range terms as being equivalent.

int jhcNetBuild::alias_rules (FILE *out, const char *cat, const char *val, const char *alt, int n) const
{
  int nr = n;

  // add separator then basic membership rules for alternate
  fprintf(out, "// ------------------------------------------------\n\n");
  nr = value_rules(out, cat, alt, nr);

  // affirm alternate term
  fprintf(out, "// RULE %d - If something is %s then it is %s\n", nr + 1, val, alt);
  fprintf(out, "  if: hq-1 -lex-  %s\n", val);
  fprintf(out, "           -hq--> obj-1\n");
  fprintf(out, "then: hq-2 -lex-  %s\n", alt);
  fprintf(out, "           -hq--> obj-1\n\n");

  // refute alternate term
  fprintf(out, "// RULE %d - If something is not %s then it is not %s\n", nr + 2, val, alt);
  fprintf(out, "  if: hq-1 -lex-  %s\n", val);
  fprintf(out, "           -neg-  1\n");
  fprintf(out, "           -hq--> obj-1\n");
  fprintf(out, "then: hq-2 -lex-  %s\n", alt);
  fprintf(out, "           -neg-  1\n");
  fprintf(out, "           -hq--> obj-1\n\n");
  return(nr + 2);
}


//= Add operators that call functions to determine the value of some range property.

int jhcNetBuild::range_ops (FILE *out, const char *pre, const char *cat, int np) const
{
  // basic information finding
  fprintf(out, "// OP %d - To find the %s of something ...\n", np + 1, cat);
  fprintf(out, "trig:\n");
  fprintf(out, "  FIND[  hq-1 -hq--> obj-1\n");
  fprintf(out, "        ako-1 -lex-  %s\n", cat);
  fprintf(out, "              -ako-> hq-1 ]\n"); 
  fprintf(out, "  ----------\n");
  fprintf(out, "   FCN[ fcn-1 -lex-  %s_value\n", pre);
  fprintf(out, "              -str-  %s\n", cat);
  fprintf(out, "              -arg-> obj-1 ]\n\n");

  // indirect assertion testing (relies on mutexes)
  fprintf(out, "// OP %d - To check if something is some %s ...\n", np + 2, cat);
  fprintf(out, "trig:\n");
  fprintf(out, "   CHK[  hq-1 -hq--> obj-1\n");
  fprintf(out, "        ako-1 -lex-  %s\n", cat);
  fprintf(out, "              -ako-> hq-1 ]\n"); 
  fprintf(out, "  ----------\n");
  fprintf(out, "   FCN[ fcn-1 -lex-  %s_value\n", pre);
  fprintf(out, "              -str-  %s\n", cat);
  fprintf(out, "              -arg-> obj-1 ]\n\n");
  return(np + 2);
}


//= Add operators that call functions to determine the value of some category.

int jhcNetBuild::list_ops (FILE *out, const char *pre, const char *cat, int np) const
{
  char abbr[40];

  // basic information finding
  fprintf(out, "// OP %d - To find the %s of something ...\n", np + 1, cat);
  fprintf(out, "trig:\n");
  fprintf(out, "  FIND[  hq-1 -hq--> obj-1\n");
  fprintf(out, "        ako-1 -lex-  %s\n", cat);
  fprintf(out, "              -ako-> hq-1 ]\n"); 
  fprintf(out, "  ----------\n");
  fprintf(out, "   FCN[ fcn-1 -lex-  %s_%s\n", pre, cat);
  fprintf(out, "              -arg-> obj-1 ]\n\n");

  // assertion testing
  strncpy_s(abbr, cat, 3);
  fprintf(out, "// OP %d - To check if something is some %s ...\n", np + 2, cat);
  fprintf(out, "trig:\n");
  fprintf(out, "   CHK[  hq-1 -hq--> obj-1\n");
  fprintf(out, "        ako-1 -lex-  %s\n", cat);
  fprintf(out, "              -ako-> hq-1 ]\n"); 
  fprintf(out, "  ----------\n");
  fprintf(out, "   FCN[ fcn-1 -lex-  %s_%s_ok\n", pre, abbr);
  fprintf(out, "              -arg-> hq-1 ]\n\n");
  return(np + 2);
}


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

