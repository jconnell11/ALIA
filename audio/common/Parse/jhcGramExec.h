// jhcGramExec.h : Earley chart parser controller for CFG grammars
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

#ifndef _JHCGRAMEXEC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGRAMEXEC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcGenParse.h"
#include "Parse/jhcGramRule.h"
#include "Parse/jhcTxtSrc.h"


//= Earley chart parser controller for CFG grammars.
// ignores nullable rule expansions (e.g. <foo> <-- *)
// does one symbol lookahead for better efficiency
// can largely replace jhcSpeechX for text inputs
// <pre>
//
// STANDARD COMMAND AND CONTROL GRAMMAR FORMAT:
//
//     ; first comment line
//     =[rule0]
//       word1 word2 (opt_word2) word3 <RULE1> word4 <rule2>
//       (word1) <rule2> *
//
//     // another comment
//     =[RULE1]
//       word1 ?                ; trailing comment
//       word2 word3, word4
//
//   Left hand sides are preceeded by "=" and are in square or angle brackets. 
//   Succeeding lines are possible disjunctive right hand side expansions. 
// 
//   Nonterminals are enclosed in square or angle brackets. Terminals are unquoted words 
//   or numbers. Optional terminals and non-terminal elements are enclosed in parenthesis. 
//
//   A dictation request is signalled with special characters. 
//     # = exactly 1 word 
//     ? = 0 or 1 word (same as "(#)")
//     + = at least 1 word but more allowed
//     * = 0 or more words (same as "(+)")
//   These should not be used anywhere else (in terminals or non-terminals).
// 
//   Comments can be added either with "//" or with ";" to disregard the 
//   remainder of the line. 
//
//   Other grammar files can be embedded using #include "alt_gram.sgm" lines.
//   Rules can span multiple files (i.e. each disjunctive expansion independent).
//
//
// SLOT VALUE PAIRS:
//
//   Capitalized non-terminals are slots which receive the first non-terminal of their 
//   expansion as their value. If the first character is "^" or there are no non-terminals, 
//   then the value is the set of words spanned by the non-terminal.
//
//   Non-terminals that start with "!" (actions) or "$" (arguments) or "%" (properties) 
//   are emitted as fragment markers only, and still allow retrieval of slot value pairs 
//   beneath them in the tree.
//
//   Example:
//
//     =[top]
//       <!ingest>
//       <!yack>
//
//     =[!ingest]
//       eat <^FOOD>
//       drink <BEV>
//
//     =[BEV]
//       (some) <soda>
//       a glass of milk
//
//     =[soda]
//       soda
//       pop
//       Coke
//
//     =[^FOOD]
//       (<quant>) +
//
//     =[quant]
//       a lot of
//       a piece of 
//
//   "drink some Coke"          --> !ingest BEV=soda
//   "drink a glass of milk"    --> !ingest BEV=a glass of milk
//   "eat a piece of apple pie" --> !ingest FOOD=a piece of apple pie    
//
//   All entries in the association list are separated by tabs.
//
// </pre>

class jhcGramExec : public jhcTxtSrc, public jhcGenParse
{
// PRIVATE MEMBER VARIABLES
private:
  // grammar
  double ver;                /** Version number of code.         */
  jhcGramRule *gram;         /** Grammar rules used in parsing.  */
  jhcGramRule *last;         /** Most recent grammar rule added. */

  // parsing
  jhcGramRule *chart;        /** State of parsing operation.     */
  int snum;                  /** Next chart state to assign.     */
  int word;                  /** How many words in input string. */

  // result inspection
  int nt;                    /** How many interpretations found.   */
  int tree;                  /** Which interpretation to examine.  */
  int focus;                 /** Current parse element to examine. */
  jhcGramRule *stack[50];    /** List of previous parse elements.  */
  char frag[80];             /** Temporary string results.         */

  // cleaned up source
  jhcTxtSrc txt2;            /** Nice version of input word list.  */       
  char norm[200];            /** Input with proper capitalization. */ 

  // robot names
  char alert[10][40];        /** Phrases referring to robot.  */
  int nn;                    /** Number of valid alert terms. */


// PROTECTED MEMBER VARIABLES
protected:
  char gfile[200];           /** Grammar file loaded or to load.   */
  int dict_n;                /** Max words for "+" or "*" pattern. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcGramExec ();
  ~jhcGramExec ();
  virtual int PrintCfg ();                 // allow to be overriden
  int NumStates () const {return snum;}

  // grammar set up (let some fcns be overridden later)
  void ClearGrammar (int keep =1);
  virtual void SetGrammar (const char *fname, ...);
  virtual int LoadGrammar (const char *fname, ...);
  const char *GrammarFile () const {return gfile;}
  int DumpRules (const char *fname, ...);
  void ListRules () const;
  int NumRules () const;
  virtual int MarkRule (const char *name =NULL, int val =1);
  virtual int ExtendRule (const char *name, const char *phrase);

  // main functions
  int Parse (const char *sent);
  int NumTrees () const {return nt;}
  int Selected () const {return tree;}
  int WildCards (int n) const;
  int DictItems (int n) const;
  int Nodes (int n) const;
  int PickTree (int n);
  void ClrTree ();

  // parse results
  virtual const char *Input () const {return Raw();}
  virtual const char *Clean () const {return norm;}
  virtual const char *Root ();
  const char *TopCat ();
  char *AssocList (char *alist, int close, int ssz);
  int NameSaid (const char *sent, int amode) const;

  // parse results - convenience
  template <size_t ssz> 
    char *AssocList (char (&alist)[ssz], int close =0) 
      {return AssocList(alist, close, ssz);}

  // debugging
  void PrintTree (int top =1);

  // parsing configuration (low level sp_parse)
  const char *parse_version (char *spec, int ssz) const;
  int parse_setup (const char *cfg_file =NULL);
  int parse_start (int level =0, const char *log_file =NULL);
  void parse_cleanup ();

  // parsing configuration - convenience
  template <size_t ssz> 
    const char *parse_version (char (&spec)[ssz]) const
      {return parse_version(spec, ssz);}

  // run time parsing modifications (low level sp_parse)
  int parse_load (const char *grammar);
  void parse_clear ();
  int parse_enable (const char *rule);
  int parse_disable (const char *rule =NULL);
  int parse_extend (const char *rule, const char *option);

  // parsing results (low level sp_parse)
  int parse_analyze (const char *text, const char *conf =NULL);
  int parse_focus (char *token, int ssz);
  int parse_focus () {return parse_focus(NULL, 0);}
  int parse_span (int *first =NULL, int *last =NULL);
  int parse_top (int n =0);
  int parse_next ();
  int parse_down ();
  int parse_up ();

  // parsing results - convenience
  template <size_t ssz> 
    int parse_focus (char (&token)[ssz])
      {return parse_focus(token, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  int normalize (int n0, const jhcGramRule *r);
  int wild_cnt (const jhcGramRule *s) const;
  int dict_cnt (const jhcGramRule *s) const;
  int node_cnt (const jhcGramRule *s) const;

  // association list
  int tree_major (char *ans, int ssz);
  void tree_slots (char *alist, int close, int ssz);
  int all_caps (const char *name) const;
  void append (char *dest, const char *extra, int ssz) const;


  // debugging
  void print_focus (int indent, int start, int end);

  // grammar construction
  char *clean_line (char *ans, int len, FILE *in, int ssz);
  int split_optional (const char *rname, const char *line);
  int split_paren (const char *rname, char *base, char *start);
  int split_dict (const char *rname, char *base, char *start);
  int build_phrase (const char *rname, const char *line);
  jhcGramStep *build_step (int &inc, const char *line);
  void nonterm_chk (const char *rname, const char *gram) const;

  // core parser
  void rem_chart ();
  int scan (const char *token, int n, const char *peek);
  int complete (jhcGramRule *s0, int n, const char *peek);
  int predict (const char *cat, int n, const char *peek);
  int add_chart (const jhcGramRule *r, int end, jhcGramRule *s0, int init, const char *peek);

  // parsing results
  jhcGramRule *nth_full (int n) const;
  jhcGramStep *find_non (const jhcGramRule *s, int n) const;


};


#endif  // once




