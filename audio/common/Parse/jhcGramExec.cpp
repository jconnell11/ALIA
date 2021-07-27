// jhcGramExec.cpp : Earley chart parser controller for CFG grammars
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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
#include <string.h>
#include <ctype.h>

#include "Parse/jhcGramExec.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcGramExec::jhcGramExec ()
{
  // current version
  ver = 1.60;

  // max number of words that "+" can match
  dict_n = 5; 

  // grammar
  gram = NULL;
  parse_clear();

  // parsing
  chart = NULL;
  rem_chart();
}


//= Default destructor does necessary cleanup.

jhcGramExec::~jhcGramExec ()
{
  parse_cleanup();
}


//= Print out full configuration data for current system.
// returns overall status of system (number of rules)

int jhcGramExec::PrintCfg () 
{
  int n = NumRules();

  jprintf("Earley chart parser, version %4.2f\n", ver);
  if (n > 0)
    jprintf("  %d grammar rules from: %s\n", n, gfile);
  else
    jprintf(">>> No grammar rules loaded!\n");
  jprintf("\n");
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                              Grammar Setup                            //
///////////////////////////////////////////////////////////////////////////

//= Remembers grammar to load but does NOT load it yet.
// used as default when calling LoadGrammar(NULL)
// generally "gfile" is only the first grammar loaded
// NOTE: most common pattern is SetGrammar then Init 

void jhcGramExec::SetGrammar (const char *fname, ...)
{
  va_list args;

  if ((fname == NULL) || (*fname == '\0'))
    return;
  va_start(args, fname); 
  vsprintf_s(gfile, fname, args);
  if (strchr(gfile, '.') == NULL)
    strcat_s(gfile, ".sgm");
}


//= Get rid of any loaded grammar rules but generally keep file name.

void jhcGramExec::ClearGrammar (int keep)
{
  char first = *gfile;

  parse_clear();
  if (keep > 0)
    *gfile = first;  
}


//= Load a recognition grammar from a generic file.
// ignores duplicate expansions (e.g. noun added twice)
// appends new rules if some other grammar(s) already loaded
// to get rid of old rules first call ClearGrammar()
// all rules initially unmarked (i.e. not active top level)
// returns 0 if some error, else 1
// NOTE: most common pattern is SetGrammar then Init 

int jhcGramExec::LoadGrammar (const char *fname, ...)
{
  char append[200];
  char *gf = (char *)((*gfile == '\0') ? &gfile : &append);
  va_list args;
  int rc;

  // assemble file name
  if (fname == NULL)
    gf = gfile;
  else if (*fname == '\0')
    return 0;
  else
  {
    va_start(args, fname); 
    vsprintf_s(gf, 200, fname, args);
    if (strchr(gf, '.') == NULL)
      strcat_s(gf, 200, ".sgm");
  }

  // try loading
  if ((rc = parse_load(gf)) > 0)
    return 1;
  return rc;
}


//= Write current (expanded) rule set to a file in SGM format. 
// returns number of rules written, negative for error

int jhcGramExec::DumpRules (const char *fname, ...)
{
  va_list args;
  char dump[500];
  FILE *out;
  jhcGramRule *r2, *r = gram;
  int cnt = 0;

  // assemble file name
  if ((fname == NULL) || (*fname == '\0'))
    return 0;
  va_start(args, fname); 
  vsprintf_s(dump, fname, args);

  // try opening file then clear marks on all rules
  if (fopen_s(&out, dump, "w") != 0)
    return -1;  
  while (r != NULL)
  {
    r->mark = 0;
    r = r->next;
  }

  // find active rules in grammar
  r = gram;
  while (r != NULL)
  {
    if ((r->mark <= 0) && (r->status > 0))
    {
      // print out head and this expansion
      r->Topic(out);
      r->Expand(out);
      r->mark = 1;
      cnt++;

      // find all other rules with same head
      r2 = gram;
      while (r2 != NULL)
      {
        if ((r2->mark <= 0) && (r2->status > 0))
          if (strcmp(r2->head, r->head) == 0)
          {
            // print just expansion
            r2->Expand(out);
            r2->mark = 1;
            cnt++;
          }
        r2 = r2->next;
      }
      fprintf(out, "\n");
    }
    r = r->next;
  }

  // cleanup
  fclose(out);
  return cnt;
}


//= Print all current (expanded) rules to console.

void jhcGramExec::ListRules () const
{
  jhcGramRule *r = gram;

  while (r != NULL)
  {
    if (r->status > 0)
      r->PrintRule();
    r = r->next;
  }
}


//= Determine how many expanded rules are in use.

int jhcGramExec::NumRules () const
{
  jhcGramRule *r = gram;
  int cnt = 0;

  while (r != NULL)
  {
    if (r->status > 0)
      cnt++;
    r = r->next;
  }
  return cnt;
}


//= Activate (val = 1) or deactivate (val = 0) a grammar rule.
// use NULL as name to mark all top level rules
// returns 0 if could not find rule, else 1

int jhcGramExec::MarkRule (const char *name, int val)
{
  if (val <= 0)
    return parse_disable(name);
  return parse_enable(name);
}


//= Add another valid expansion for some non-terminal.
// returns 2 if full update, 1 if not in file, 0 or negative for failure

int jhcGramExec::ExtendRule (const char *name, const char *phrase)
{
  return parse_extend(name, phrase);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Parse input according to grammar to give one or more trees.
// automatically picks most specific interpretation as default tree
// returns number of top level interpretations (0 if out of grammar)
// about 11 ms on 3 GHz i5 for 3000 parse states (7 possible parses)

int jhcGramExec::Parse (const char *sent)
{
  int i, w, k, n, w0, k0, n0;

  // do basic work (assume correct top level rules already activated)
  *norm = '\0';
  if (parse_analyze(sent) > 1)
    for (i = 0; i < nt; i++)
    {
      // pick interpretation with most words (least wildcards),
      // least dictation, and most compact derivation (fewest nodes)
      w = WildCards(i);
      k = DictItems(i);
      n = Nodes(i);
      if ((i == 0) || (w < w0) || 
         ((w == w0) && (k < k0)) ||
         ((w == w0) && (k == k0) && (n < n0)))
      {
        w0 = w; 
        k0 = k;
        n0 = n;
        tree = i;
      }
    }

  // get input string with canonical capitalization
  if (nt > 0)
    if (normalize(0, nth_full(tree)) > 0)
      norm[strlen(norm) - 1] = '\0';    
  txt2.SetSource(norm);
  return nt;
}


//= Accumulate terminal symbols from selected parse into normalized string.
// returns number of next word to be inserted into string

int jhcGramExec::normalize (int n0, const jhcGramRule *r)
{
  char wd[40];
  const jhcGramStep *s;
  int n = n0;

  // sanity check
  if (r == NULL)
    return n0;

  // check all step in rule
  s = r->tail;
  while (s != NULL)
  {
    // check if step is itself a rule
    if (s->non > 0)
      n = normalize(n, s->back);
    else
    {
      // if wildcard then get raw word, else copy terminal
      if (strcmp(s->symbol, "#") == 0)
        strcat_s(norm, Span(wd, n, n, 1));     // raw input
      else
        strcat_s(norm, s->symbol);

      // add space after word and advance index
      strcat_s(norm, " ");
      n++;
    }
    s = s->tail;
  }
  return n;
}


//= Tell how many words matched wildcards in a particular interpretation.

int jhcGramExec::WildCards (int n) const
{
  return wild_cnt(nth_full(n));
}


//= Count number of wildcards in expansion of this rule.

int jhcGramExec::wild_cnt (const jhcGramRule *s) const
{
  const jhcGramStep *t;
  int cnt = 0;

  if (s == NULL)
    return 0;

  // go through all steps in rule expansion
  t = s->tail;
  while (t != NULL)
  {
    // add one for each "#" wildcard found (only kind)
    if (t->non > 0)
      cnt += wild_cnt(t->back);
    else if (strcmp(t->symbol, "#") == 0)
      cnt++;
    t = t->tail;
  }
  return cnt;
}


//= Tell how many dictated phrases are in a particular interpretation.

int jhcGramExec::DictItems (int n) const
{
  return dict_cnt(nth_full(n));
}


//= Count number of contiguous sequences of dictated words.

int jhcGramExec::dict_cnt (const jhcGramRule *s) const
{
  const jhcGramStep *t;
  int prev = 0, cnt = 0;

  if (s == NULL)
    return 0;

  // go through all steps in rule expansion
  t = s->tail;
  while (t != NULL)
  {
    // find sequences of wildcards WITHIN each expansion
    if (t->non > 0)
      cnt += dict_cnt(t->back);
    else if (strcmp(t->symbol, "#") == 0)
    {
      // increment only once per sequence "# # # ..."
      if (prev <= 0)
        cnt++;
      prev = 1;
    }
    else
      prev = 0;              // non "#" element so sequence ends
    t = t->tail;
  }
  return cnt;
}


//= Count number of non-terminal nodes in a particular interpretation.

int jhcGramExec::Nodes (int n) const
{
  return node_cnt(nth_full(n));
}


//= Count number of non-terminal modes in expansion of this rule.

int jhcGramExec::node_cnt (const jhcGramRule *s) const
{
  const jhcGramStep *t;
  int cnt = 1;

  if (s == NULL)
    return 0;

  // go through all steps in rule expansion
  t = s->tail;
  while (t != NULL)
  {
    // add number of nodes under each non-terminal encountered
    if (t->non > 0)
      cnt += node_cnt(t->back);
    t = t->tail;
  }
  return cnt;
}


//= Choose some interpretation if more than one found.
// returns tree to be examined (check if same as selected)

int jhcGramExec::PickTree (int n)
{
  if (parse_top(n) > 0)
    tree = n;
  return tree;
}


//= Clear tree and chart in preparation for next input.
// keeps loaded grammar and rule markings

void jhcGramExec::ClrTree ()
{
  rem_chart();
}


///////////////////////////////////////////////////////////////////////////
//                           Association List                            //
///////////////////////////////////////////////////////////////////////////

//= Returns the non-terminal associated with the root of the parse tree.
// returns pointer to internal string **WHICH MAY GET REWRITTEN**
// copy the returned string if you want it to persist

const char *jhcGramExec::Root ()
{
  parse_top(tree);
  parse_focus(frag, 80);
  return frag;
}


//= Moves focus to highest important (capitalized) non-terminal.
// returns name of this rule (i.e. typically the category)
// returns pointer to internal string **WHICH MAY GET REWRITTEN**
// copy the returned string if you want it to persist

const char *jhcGramExec::TopCat ()
{
  parse_top(tree);
  if (tree_major(frag, 80) > 0)
    return frag;
  return NULL;
}


//= Do depth first search of tree until first all capitalized rule is found.

int jhcGramExec::tree_major (char *ans, int ssz)
{
  // see if current focus is a winner
  parse_focus(ans, ssz);
  if (all_caps(ans) > 0)
    return 1;

  // try going down further
  if (parse_down() > 0)
  {
    if (tree_major(ans, ssz) > 0)
      return 1;
    parse_up();
  }

  // else try next part of current expansion
  if (parse_next() > 0)
    return tree_major(ans, ssz);
  return 0;
}


//= Generates a string encoding an association list of slots and values.
// finds all major (capitalized) categories and first non-terminal underneath (if any)
// e.g. "GRAB=nil SIZE=red COLOR=red" for "Grab the big red block" all prefixed by tabs
// can optionally return a fake semi-phonetic word for dictation items

char *jhcGramExec::AssocList (char *alist, int close, int ssz)
{
  *alist = '\0';
  if (chart == NULL)
    return alist;
  parse_top(tree);
  tree_slots(alist, close, ssz);
  return alist;
}


//= Do depth first search of tree to find capitalized nodes and children.
// if first character of a node is "^" then bind covered string instead
// prefixes "!", "$", and "%" emit tag but do not block descent
// will mark ends of !, $, and % phrases if close > 0

void jhcGramExec::tree_slots (char *alist, int close, int ssz)
{
  char node[40], val[500];
  int first, last;

  // see if current node is a phrase boundary
  parse_focus(node, 40);
  if (strchr("!$%", *node) != NULL)
  {
    append(alist, "\t", ssz);
    append(alist, node, ssz);
  }

  // see if current focus is a winner (capitalized)
  if (all_caps(node) > 0)
  {
    // add rule name (needs to have leading tab always)
    append(alist, "\t", ssz);
    append(alist, node, ssz);
    append(alist, "=", ssz);

    // add first subcategory (or full node expansion)
    if ((*node != '^') && (parse_down() > 0))
    {
      // minor node name verbatim
      parse_focus(node, 40);
      append(alist, node, ssz);
      parse_up();
    }
    else
    {
      // possibly clean up dictation results
      parse_span(&first, &last);
      txt2.Span(val, first, last, 0);
      strcat_s(alist, ssz, val);
    }
  }
  else if (parse_down() > 0)
  {
    // try going down further
    tree_slots(alist, close, ssz);
    parse_up();
  }

  // possibly mark phrase ending
  if ((close > 0) && (strchr("!$%", *node) != NULL))
  {
    node[1] = '\0';
    append(alist, "\t", ssz);
    append(alist, node, ssz);
  }

  // try next part of current expansion
  if (parse_next() > 0)
    tree_slots(alist, close, ssz);
}


//= Test whether all alphabetic characters are uppercase.

int jhcGramExec::all_caps (const char *name) const
{
  const char *c = name;

  if (name == NULL)
    return 0;

  while (*c != '\0')
    if (islower(*c))
      return 0;
    else
      c++;
  return 1;
}


//= Alternative to strcat, which sometimes gives crazy results.

void jhcGramExec::append (char *dest, const char *extra, int ssz) const
{
  int n = (int) strlen(dest);

  strcpy_s(dest + n, ssz - n, extra);
}


//= See if attention (to speech) should be renewed based on input sentence.
// mode: 0 = always, 1 = ATTN anywhere, 2 = ATTN at start, 3 = ATTN only (hail)

int jhcGramExec::NameSaid (const char *sent, int mode) const
{
  const char *tail = sent;
  int i, len, sz;

  // simplest cases
  if (mode <= 0)
    return 1;
  if (*sent == '\0')
    return 0;

  // strip off initial "Hey" if present
  if ((_strnicmp(tail, "Hey", 3) == 0) && (isalpha(tail[3]) == 0))
  {
    tail += 4;
    while ((*tail != '\0') && (isalpha(*tail) == 0))
      tail++;
  }

  // see if sentence begins with a name for the robot
  for (i = 0; i < nn; i++)
  {
    len = (int) strlen(alert[i]);
    if ((_strnicmp(tail, alert[i], len) == 0) && (isalpha(tail[len]) == 0)) 
    {
      // see if this is the ONLY thing said
      if (mode >= 3)
      {
        tail += len;
        while (*tail != '\0')
          if (isalpha(*tail++) != 0)
            return 0;
      }
      return 1;
    }
  }

  // strip any final punctuation 
  if (mode > 1)
    return 0;
  sz = (int) strlen(sent);
  if ((sz > 1) && (isalpha(sent[sz - 1]) == 0))
    sz--;
  tail = sent + sz;

  // see if sentence ends with a name for the robot
  for (i = 0; i < nn; i++)
  {
    len = (int) strlen(alert[i]);
    if ((len <= sz) && (_strnicmp(tail - len, alert[i], len) == 0) && 
        ((len == sz) || (isalpha(*(tail - len - 1)) == 0)))
      return 1;
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Debugging                                //
///////////////////////////////////////////////////////////////////////////

//= Print out sections of parse tree for debugging.
// if top positive then resets to tree root, else starts at current node

void jhcGramExec::PrintTree (int top)
{
  if (top > 0)
    parse_top(tree);
  print_focus(0, 0, 0);
  jprintf("\n");
}


//= Go depth-first through parse tree showing non-terminals and expansions

void jhcGramExec::print_focus (int indent, int start, int end)
{
  char val[500], node[80], leader[80] = "";
  int i, first, last;

  // check for valid node then get surface coverage
  if (parse_focus(node, 40) <= 0)
    return;
  parse_span(&first, &last);

  // build indentation white space 
  for (i = 0; i < indent; i++)
    strcat_s(leader, "  ");

  // print any leading terminals then rule name
  if ((indent > 0) && (first > start))
    jprintf("%s%s\n", leader, txt2.Span(val, start, first - 1, 0));
  jprintf("%s<%s>\n", leader, node);

  // expand non-terminal or just print surface words
  if (parse_down() > 0)
  {
    print_focus(indent + 1, first, last);
    parse_up();
  } 
  else
    jprintf("%s  %s\n", leader, txt2.Span(val, first, last, 0));

  // go on to next non-terminal or print trailing terminals
  if (parse_next() > 0)
    print_focus(indent, last + 1, end);
  else if ((indent > 0) && (last < end))
    jprintf("%s%s\n", leader, txt2.Span(val, last + 1, end, 0));
}


///////////////////////////////////////////////////////////////////////////
//                        Parsing Configuration                          //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of DLL and possibly other information.
// returns pointer to input string for convenience

const char *jhcGramExec::parse_version (char *spec, int ssz) const
{
  if (spec != NULL)
    sprintf_s(spec, ssz, "%4.2f jhcGramExec", ver); 
  return spec;
}


//= Loads all common grammar and parsing parameters based on the file given.
// the single configuration file can point to other files as needed (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcGramExec::parse_setup (const char *cfg_file)
{
  // ignores arguments
  return 1;
}


//= Start accepting utterances to parse according to some grammar(s).
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

int jhcGramExec::parse_start (int level, const char *log_file)
{
  if (log_file == NULL)
    return 1;
  if (*log_file != '\0')
    return jprintf_open(log_file, 1);
  return jprintf_open();                  // for "" filename
}


//= Stop accepting utterances and clean up all objects and files.
// only call this at the end of a processing session (REQUIRED)

void jhcGramExec::parse_cleanup ()
{
  jprintf_close();
  rem_chart();
  parse_clear();
}


///////////////////////////////////////////////////////////////////////////
//                        Grammar Construction                           //
///////////////////////////////////////////////////////////////////////////

//= Load a certain (or additional) grammar from a file.
// ignores duplicate expansions (e.g. noun added twice)
// appends new rules if some other grammar(s) already loaded
// initially all rules are disabled (call jhcGramExec::parse_enable)
// returns 2 if appended, 1 if exclusive, 0 or negative for some error

int jhcGramExec::parse_load (const char *grammar)
{
  FILE *in;
  char dir[200], extra[200], text[500];
  char rname[80] = "";
  char *start, *end;

  // try opening file 
  if (fopen_s(&in, grammar, "r") != 0)
    return jprintf(">>> Could not open %s in jhcGramExec::parse_load!\n", grammar);

  // save this file's directory (included files are relative)
  strcpy_s(dir, grammar);
  if ((end = strrchr(dir, '/')) != NULL)
    *(end + 1) = '\0';
  else
    *dir = '\0';
  
  // go through file line by line
  while ((start = clean_line(text, 200, in, 500)) != NULL)
    if (strncmp(start, "#include", 8) == 0)
    {
      // load another file first
      if ((start = strchr(start, '\"')) != NULL)
        if ((end = strchr(start + 1, '\"')) != NULL)
        {
          *end = '\0';
          sprintf_s(extra, "%s%s", dir, start + 1);
          parse_load(extra);
          *rname = '\0';
        }
    }
    else if (*start == '=')
    {
      // look for label in beginning of new rule paragraph
      if ((start = strpbrk(start + 1, "[<")) != NULL)
        if ((end = strpbrk(start + 1, "]>")) != NULL)
        {
          if (_strnicmp(start + 1, "xxx", 3) == 0)
          {

            // ignore sections starting with XXX
            *rname = '\0';
            continue;
          }
          *end = '\0';
          strcpy_s(rname, start + 1);
          nonterm_chk(rname, grammar);
        }
    }
    else if ((*rname != '\0') && (*start != '\0'))
      split_optional(rname, start);

  // close file
  fclose(in);
  return 1;
}


//= Strip off comment portion and newline character at end.
// also remove leading and trailing white space and eliminate tabs

char *jhcGramExec::clean_line (char *ans, int len, FILE *in, int ssz)
{
  char raw[500];
  char *start, *end;

  // get line from file (unless at end)
  if (fgets(raw, len, in) == NULL)
    return NULL;

  // copy minus leading whitespace
  start = raw;
  while (strchr(" \t", *start) != NULL)
    start++;
  strcpy_s(ans, ssz, start);

  // remove final newline and anything after semicolon
  if ((end = strchr(ans, '\n')) != NULL)
    *end = '\0';
  if ((end = strchr(ans, ';')) != NULL)
    *end = '\0';

  // remove double slashes and following comment
  end = ans;
  while ((end = strchr(end, '/')) != NULL)
    if (*(end + 1) == '/')
    {
      *end = '\0';
      break;
    }
    else
      end++;
  return ans;
}


//= Split an expansion with optional parts into many base expansions.

int jhcGramExec::split_optional (const char *rname, const char *line)
{
  char base[500];
  char *start;

  // no fancy components
  if (strpbrk(line, "(+*?") == NULL)
    return build_phrase(rname, line);
  strcpy_s(base, line);

  // constructs with a lot of editing
  if ((start = strchr(base, '(')) != NULL)
    return split_paren(rname, base, start);
  if ((start = strchr(base, '+')) != NULL)
    return split_dict(rname, base, start);

  // optional multi-word dictation (generally fewer is better)
  if ((start = strchr(base, '*')) != NULL)
  {
    *start = ' ';
    split_optional(rname, base);              // dropped
    *start = '+';
    return split_dict(rname, base, start);    // required multi
  }

  // optional single dictation (generally fewer is better)
  if ((start = strchr(base, '?')) != NULL)
  {
    *start = ' ';
    split_optional(rname, base);              // dropped
    *start = '#';
    return split_optional(rname, base);       // required single
  }
  return 1;
}


//= Handle optional parenthesized group by making two copies.

int jhcGramExec::split_paren (const char *rname, char *base, char *start)
{
  char alt[500];
  char *end;
  int lvl = 1;

  // find balancing closed parenthesis
  end = start + 1;
  while (*end != '\0')
  {
    if (*end == '(')
      lvl++;
    else if (*end == ')')
      if (--lvl <= 0)
        break;
    end++;
  }

  // generate one version with group missing
  strncpy_s(alt, base, start - base);                // always adds terminator
  if (*end != '\0')
    strcat_s(alt, end + 1);

  // generate another version with group present
  *start = ' ';
  if (*end != '\0')
    *end = ' ';

  // generally prefer more specific version
  split_optional(rname, base);  
  return split_optional(rname, alt);
}


//= Handle multi-word dictation by requiring various numbers of words.

int jhcGramExec::split_dict (const char *rname, char *base, char *start)
{
  char alt[500];
  int i, j;

  // require various numbers of copies
  for (i = 1; i <= dict_n; i++)
  {
    // keep start of phrase
    strncpy_s(alt, base, start - base);        // always adds terminator

    // insert single word required dictations before rest
    for (j = 1; j <= i; j++)
      strcat_s(alt, "# ");
    strcat_s(alt, start + 1);                  // skip wildcard character

    // process this new version
    split_optional(rname, alt);
  }
  return 1;
}


//= Assemble one path of a rule or an optional conjunct.

int jhcGramExec::build_phrase (const char *rname, const char *line)
{
  const char *start = line;
  jhcGramRule *t, *r = gram;
  jhcGramStep *s, *s0 = NULL;
  int n;

  // ignore any null expansions, add attention words to special list
  if ((*rname == '\0') || (*line == '\0'))
    return 0;
  if ((strcmp(rname, "ATTN") == 0) && (nn < 10))
    strcpy_s(alert[nn++], line);

  // make a new rule for given non-terminal
  if ((t = new jhcGramRule) == NULL)
    return -1;
  strcpy_s(t->head, rname);

  // break expansion into words
  while (1)
  {
    // make a new expansion step and link it onto the end of the rule
    if ((s = build_step(n, start)) == NULL)
      break;
    if (s0 != NULL)
      s0->tail = s;
    else
      t->tail = s;

    // look for next step of expansion
    s0 = s;
    start += n;
  }

  // check for duplication (clean up if necessary)
  while (r != NULL)
  {
    if (t->SameRule(r) > 0)
    {
      delete t;
      return 0;
    }
    r = r->next;
  }

  // add to END of rule list
  if (last == NULL)
    gram = t;
  else
    last->next = t;
  last = t;
  return 1;
}


//= Create a new rule expansion step for token at beginning of line.
// returns step and how much to advance string (inc)

jhcGramStep *jhcGramExec::build_step (int &inc, const char *line)
{
  jhcGramStep *s;
  const char *end, *start = line;
  int n;

  // skip leading white space 
  while ((*start == ' ') || (*start == '\t'))
    start++;
  if (*start == '\0')
    return NULL;
  inc = (int)(start - line);

  // find end of current token
  if ((end = strpbrk(start, " \t")) != NULL)
    n = (int)(end - start);
  else
    n = (int) strlen(start);
  inc += n;

  // make a new expansion step and link it onto the end of the rule
  if ((s = new jhcGramStep) == NULL)
    return NULL;

  // see if step is a non-terminal (enclosed by angle braces)
  if ((strchr("[<", start[0]) != NULL) && (strchr(">]", start[n - 1]) != NULL))
  {
    start++;
    n -= 2;
    s->non = 1;
  }

  // bind basic name (terminal or non-terminal)
  strncpy_s(s->symbol, start, n);                    // always adds terminator
  return s;
}


//= Check that there are no common mistakes in the name of non-terminals.

void jhcGramExec::nonterm_chk (const char *rname, const char *gram) const
{
  const char *r = rname;
  char c;
  int cap = 0, low = 0;

  // make sure parser will not get confused
  if (strpbrk(rname, "?#*+") != NULL)
  {
    jprintf(">>> Special character in =[%s] from %s !\n", rname, gram);
    return;
  }

  // count number of uppercase versus lower case characters
  while ((c = *r++) != '\0')
    if (isalpha(c) != 0)
    {
      if (isupper(c) != 0)
        cap++;
      else
        low++;
    }

  // check that a report category is all caps
  if ((low > 0) && (cap > low))
    jprintf(">>> Partial uppercase in =[%s] from %s !\n", rname, gram);
}


///////////////////////////////////////////////////////////////////////////
//                   Run-Time Parsing Modifications                      //
///////////////////////////////////////////////////////////////////////////

//= Remove all grammars that may have been loaded.
// also erases "gfile" member variable

void jhcGramExec::parse_clear ()
{
  jhcGramRule *r2, *r = gram;

  // get rid of all rules in grammar
  while (r != NULL)
  {
    r2 = r->next;
    delete r;
    r = r2;
  }

  // clear pointer and stored name
  gram = NULL;
  last = NULL;
  *gfile = '\0';             // can be problematic

  // no names for self yet (from ATTN in grammar)
  nn = 0;
}


//= Enable some top-level (i.e. sentence) rule within the grammar.
// returns 1 if successful, 0 if not found, negative for some error

int jhcGramExec::parse_enable (const char *rule)
{
  jhcGramRule *r = gram;
  int any = 0;

  while (r != NULL)
  {
    if ((rule == NULL) || (_stricmp(r->head, rule) == 0))
    {
      r->status = 2;
      any = 1;
    }
    r = r->next;
  }
  return any;
}


//= Disable some top-level (i.e. sentence) rule within the grammar.
// a NULL rule name serves to disable ALL top level rules
// returns 1 if successful, 0 if not found, negative for some error

int jhcGramExec::parse_disable (const char *rule)
{
  jhcGramRule *r = gram;
  int any = 0;

  while (r != NULL)
  {
    if ((rule == NULL) || (_stricmp(r->head, rule) == 0))
    {
      r->status = 1;
      any = 1;
    }
    r = r->next;
  }
  return any;
}


//= Add a new expansion to some existing rule in the grammar.
// alters internal graph and attempts to change original grammar file also
// returns 2 if ok, 1 if only run-time changed, 0 or negative for error

int jhcGramExec::parse_extend (const char *rule, const char *option)
{
  split_optional(rule, option);
  return 1;                            // never changes file
}


///////////////////////////////////////////////////////////////////////////
//                             Core Parser                               //
///////////////////////////////////////////////////////////////////////////

//= Accept an utterance for parsing by currently active grammar(s).
// can optionally take list of confidences (0-100) for each word
// automatically sets focus to top of first parse tree (if found)
// returns number of interpretations, 0 if no valid parse, negative if error

int jhcGramExec::parse_analyze (const char *text, const char *conf)
{
  char token[80], peek[80];
  const jhcGramRule *s, *r = gram;
  int more = 1;

  // clear interpretation then get first word of sentence
  rem_chart();
  SetSource(text);
  if (ReadWord(token, 0) <= 0)
    return 0;

  // initialize chart with top level rules
  while (r != NULL)
  {
    if (r->status >= 2)
      if (add_chart(r, 0, NULL, 1, token) <= 0)
        return -1;
    r = r->next;
  }

  // break sentence into a number of words then process them
  while (more > 0)
  {
    // token = current word, peek = next word
    more = ReadWord(peek, 0);
    if (scan(token, word++, peek) <= 0)
      return -1;
    strcpy_s(token, peek);
  }

  // see how many toplevel completions
  s = chart;
  while (s != NULL)
  {
    if ((s->status >= 2) && (s->dot == NULL) && 
        (s->w0 == 0) && (s->wn == word))
      nt++;
    s = s->next;
  }
  return nt;
}


//= Remove all elements of previous parsing chart (if any).

void jhcGramExec::rem_chart ()
{
  jhcGramRule *s2, *s = chart;

  // remove all states in queue
  while (s != NULL)
  {
    s2 = s->next;
    delete s;
    s = s2;
  }

  // clear parsing state
  chart = NULL;
  snum = 0;
  word = 0;

  // clear result interpretation
  nt = 0;
  tree = 0;
  focus = 0;
  stack[focus] = NULL;

  // clear normalized input
  *norm = '\0';
}


//= Advance dots in rule expansions based on newly read word N.
// returns 1 if okay, 0 if error

int jhcGramExec::scan (const char *token, int n, const char *peek)
{
  jhcGramRule *s = chart;

  while (s != NULL)
  {
    if (s->wn == n)                                        // good up thru word N
      if ((s->dot != NULL) && ((s->dot)->non <= 0))        // awaiting a terminal
        if ((_stricmp((s->dot)->symbol, token) == 0) ||    // terminal matches
            (strcmp((s->dot)->symbol, "#") == 0))          //   or wildcard
          if (add_chart(s, n + 1, NULL, 0, peek) <= 0)
            return 0;
    s = s->next;
  }
  return 1;
}


//= Look for states in the current chart which are waiting on this result.
// returns 1 if okay, 0 if problem

int jhcGramExec::complete (jhcGramRule *s0, int n, const char *peek)
{
  jhcGramRule *s = chart;

  while (s != NULL)
  {
    if (s->wn == s0->w0)                                   // good up thru this start
      if ((s->dot != NULL) && ((s->dot)->non > 0))         // awaiting a non-terminal
        if (_stricmp((s->dot)->symbol, s0->head) == 0)     // non-terminal matches
          if (add_chart(s, s0->wn, s0, 0, peek) <= 0)
            return 0;
    s = s->next;
  }
  return 1;
}


//= Expand given non-terminal starting at word N.
// returns 1 if okay, 0 if problem

int jhcGramExec::predict (const char *cat, int n, const char *peek)
{
  const jhcGramRule *s = chart, *r = gram;

  // see if non-terminal already expanded from current position
  while (s != NULL)
  {
    if ((s->w0 == n) && (_stricmp(s->head, cat) == 0))
      return 1;
    s = s->next;
  }

  // add all rules with the given non-terminal
  while (r != NULL)
  {
    if (_stricmp(r->head, cat) == 0)
      if (add_chart(r, n, NULL, 1, peek) <= 0)
        return 0;
    r = r->next;
  }
  return 1;
}


//= Add a copy of a rule or state to the chart.
// "chart" variable always points to this new item
// returns 1 if successful, 0 if fails somehow

int jhcGramExec::add_chart (const jhcGramRule *r, int end, jhcGramRule *s0, int init, const char *peek)
{
  jhcGramRule *s;
  const jhcGramStep *first = r->tail;

  // skip if next part of rule is a non-matching terminal
  if ((init > 0) && (peek != NULL) && (first != NULL))
    if ((first->non <= 0) && (*(first->symbol) != '#'))
      if (_stricmp(first->symbol, peek) != 0)
        return 1;

  // make an exact copy of rule or old state
  if ((s = new jhcGramRule) == NULL)
    return 0;
  if (s->CopyState(r) <= 0)
  {
    delete s;
    return 0;
  }

  // give it a unique number and add it to the chart list
  s->id = snum++;
  s->next = chart;
  chart = s;

  if (init > 0)
  {
    // initialize rule to become state
    s->w0 = end;
    s->wn = end;
    s->dot = s->tail;
  }
  else
  {
    // add back pointer and shift dot over 
    s->wn = end;
    (s->dot)->back = s0;
    s->dot = (s->dot)->tail;
  }

  // check if now finished or needs non-terminal expansion
  if (s->dot == NULL)
    return complete(s, end, peek);
  if ((s->dot)->non > 0)
    return predict((s->dot)->symbol, end, peek);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Parsing Results                              //
///////////////////////////////////////////////////////////////////////////

//= Returns the name or string associated with the current focus node.
// returns 1 if successful, 0 or negative for error

int jhcGramExec::parse_focus (char *token, int ssz) 
{
  const jhcGramRule *s;
  const jhcGramStep *t;

  // set default and check for problems
  *token = '\0';
  if ((focus < 0) || (focus >= 50))
    return 0;
  if ((s = stack[focus]) == NULL)
    return 0;

  // "mark" zero is head class
  if (s->mark <= 0)
  {
    strcpy_s(token, ssz, s->head);
    return 1;
  }

  // find proper non-terminal element of expansion
  if ((t = find_non(s, s->mark)) == NULL)
    return 0;
  strcpy_s(token, ssz, t->symbol);
  return 1;
}


//= Returns the range of surface words covered by the current focus node.
// word 0 is the initial word in the utterance
// returns total number of words in utterance, 0 or negative for error

int jhcGramExec::parse_span (int *first, int *last) 
{
  const jhcGramRule *s;
  const jhcGramStep *t;

  // set default values 
  if (first != NULL)
    *first = 0;
  if (last != NULL)
    *last = 0;

  // check for problems
  if ((focus < 0) || (focus >= 50))
    return 0;
  if ((s = stack[focus]) == NULL)
    return 0;

  // if focus is not head then advance through expansion
  if (s->mark > 0)
  {
    if ((t = find_non(s, s->mark)) == NULL)
      return 0;
    if ((s = t->back) == NULL)
      return 0;
  }

  // bind first and last word positions and calculate length
  if (first != NULL)
    *first = s->w0;
  if (last != NULL)
    *last = s->wn - 1;
  return(s->wn - s->w0);
}


//= Reset the focus to the top most node of the parse tree.
// can select particular interpretation if more than one
// returns 1 if successful, 0 or negative for error

int jhcGramExec::parse_top (int n)
{
  // check for valid request
  if ((n < 0) || (n >= nt))
    return -1;

  // find proper completed state
  focus = 0;
  if ((stack[0] = nth_full(n)) == NULL)
    return 0;

  // mark head (not expansion) as initial focus
  (stack[0])->mark = 0;                
  return 1;
}


//= Find nth complete state in chart (spans full input sentence).

jhcGramRule *jhcGramExec::nth_full (int n) const
{
  jhcGramRule *s = chart;
  int i = 0;

  while (s != NULL)
  {
    if ((s->status >= 2) && (s->dot == NULL) && 
        (s->w0 == 0) && (s->wn == word))
      if (i++ == n)
        return s;
    s = s->next;
  }
  return NULL;
}


//= Move focus to next non-terminal to the right in the current expansion.
// returns 1 if successful, 0 if focus unchanged, negative for error

int jhcGramExec::parse_next ()
{
  jhcGramRule *s;

  // check for problems and top of tree
  if ((focus < 0) || (focus >= 50))
    return -1;
  if ((s = stack[focus]) == NULL)
    return -1;

  // find current non-terminal based on "mark" field (top node special)
  if (s->mark <= 0)
    return 0;
  if (find_non(s, s->mark + 1) == NULL)
    return 0;
  s->mark += 1;
  return 1;
}


//= Move focus down one level (i.e. expand a non-terminal node).
// automatically sets focus to leftmost non-terminal of parse tree
// returns 1 if successful, 0 if focus unchanged, negative for error

int jhcGramExec::parse_down ()
{
  jhcGramRule *s;
  const jhcGramStep *st;
  int i = 1;

  // check for problems
  if ((focus < 0) || (focus >= 49))
    return -1;
  if ((s = stack[focus]) == NULL) 
    return -1;

  // if focussed on head (top of tree) then shift to expansion
  if (s->mark <= 0)
  {
    if (find_non(s, 1) == NULL)
      return 0;
    s->mark = 1;       
    return 1;
  }

  // find next non-terminal in expansion step based on "mark" field
  st = s->tail;
  while (st != NULL)
  {
    if (st->non > 0)
      if (i++ == s->mark)
      {
        // make sure next level exists and has some non-terminal
        if (find_non(st->back, 1) == NULL)
          return 0;

        // push non-terminal on stack, start with leftmost non-terminal
        focus++;
        stack[focus] = st->back;
        (stack[focus])->mark = 1;      
        return 1;
      }
    st = st->tail;
  }
  return -1;
}


//= Move focus up one level (i.e. restore it to location before call to down). 
// returns 1 if successful, 0 if focus unchanged, negative for error

int jhcGramExec::parse_up ()
{
  jhcGramRule *s;

  // generally pop stack, node's "mark" field will tell current element
  if (focus > 0)
  {
    focus--;
    return 1;
  }

  // check arguments
  if ((focus < 0) || (focus >= 50))
    return -1;
  if ((s = stack[focus]) == NULL)
    return -1;

  // special for top of tree (mark = 0)
  if (s->mark <= 0)
    return 0;
  s->mark = 0;
  return 1;
}


//= Find the Nth non-terminal in state expansion (first = 1).
// returns pointer to proper rule step, NULL if not found

jhcGramStep *jhcGramExec::find_non (const jhcGramRule *s, int n) const
{
  jhcGramStep *t;
  int i = 1;

  // check for reasonable arguments
  if ((s == NULL) || (n <= 0))
    return NULL;
  t = s->tail;

  // search expansion part of state
  while (t != NULL)
  {
    // increment i on each non-terminal found
    if (t->non > 0)
      if (i++ == n)
        return t;
    t = t->tail;
  }
  return NULL;
}

