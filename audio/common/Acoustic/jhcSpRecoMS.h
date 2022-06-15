// jhcSpRecoMS.h : speech recognition and synthesis using Microsoft SAPI
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
// Copyright 2022 Etaoin Systems
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

#ifndef _JHCSPRECOMS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSPRECOMS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>


//= Speech recognition and synthesis using Microsoft SAPI.
// Typically uses eng->GetPropertyNum with:
//   Response speed = 150 ms            (unambiguous grammar path)
//   Complex response speed = 500 ms    (ambiguous grammar path)
//   AdaptationOn = 1                   (adapt voice model)
//   PersistedBackgroundAdaptation = 1  (adapt noise level)
// implements functions found in "sp_reco.h" and "sp_parse.h"
// <pre>
//
// Standard command and control grammar format:
//
//   ; first comment line
//   =[rule0]
//   word1 word2 (opt_word2) word3 <RULE1> word4 <rule2>
//   (word1) <rule2> *
//
//   // another comment
//   =[RULE1]
//   word1 ?
//   word2 word3, word4
//
// Left hand sides are preceeded by "=" and are in square or angle brackets. 
// Succeeding lines are possible disjunctive right hand side expansions. 
// 
// Terminals are unquoted words or numbers and can be broken into separate 
// parts using commas (to tolerate pauses better). 
//
// Nonterminals are enclosed in square or angle brackets. Non-terminals can 
// be declared "important" by putting their names all in caps. 
//
// Optional terminals and non-terminal elements are enclosed in parenthesis. 
//
// A dictation request is signalled with special characters. 
//   # = exactly 1 word 
//   ? = 0 or 1 word (same as "(#)")
//   + = at least 1 word but more allowed
//   * = 0 or more words (same as "(+)")
// These should not be used anywhere else (in terminals or non-terminals).
//
// Comments can be added either with "//" or with ";" to disregard the 
// remainder of the line. 
//
// Other grammar files can be embedded using #include "alt_gram.sgm" lines.
// Rules can span multiple files (i.e. each disjunctive expansion independent).
//
// </pre>

class jhcSpRecoMS 
{
// PRIVATE MEMBER VARIABLES (speech recognition)
private:
  void *e;	            /** Recognition engine instance.        */
  void *c;              /** Recognition context.                */
  void *a;              /** Stream for file input.              */
  void *f;              /** Mutex for engine start/stop.        */
  void *m;              /** Voice model to install in engine.   */
  char sfile[200];      /** Name of audio file (if any).        */
  char partial[500];    /** Cached partial recognition result.  */
  char result[500];     /** Cached last recognition result.     */
  char phonetic[1500];  /** Cached last phonetic result (3x).   */
  int attn;             /** Whether system currently listening. */
  int ready;            /** Whether text result ready yet.      */
  int noisy;            /** Allow to show partial recognition.  */


// PRIVATE MEMBER VARIABLES (parsing)
private:
  void *g;                        /** Grammar object.                        */
  void *d;                        /** Parse tree of last recognition.        */
  const void *stack[50];          /** Aid for backtracking during browsing.  */
  char gfile[200];                /** Name of grammar file loaded.           */
  int focus;                      /** Current focus position in stack.       */
  int match;                      /** Number of non-dictation words covered. */


// PROTECTED MEMBER VARIABLES (speech recognition)
protected:
  int r_ok;                       /** Status of recognizer.               */
  char rfile[200];                /** Recognizer DLL name (not used).     */
  char mic[80];                   /** Name of live audio source (if any). */
  char tag[20];                   /** Prefix for debugging messages.      */
  double dict_wt;                 /** Relative weight for dictation arcs. */


// PROTECTED MEMBER VARIABLES (parsing)
protected:
  int p_ok;                       /** Status of parser.           */
  char pfile[200];                /** Parser DLL name (not used). */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcSpRecoMS ();
  ~jhcSpRecoMS ();
  int BindReco (const char *fname, const char *cfg, int start =1);
  int BindParse (const char *fname, const char *cfg, int start =1);

  // speech recognition configuration (low level sp_reco)
  const char *reco_version (char *spec, int ssz) const;
  int reco_setup (const char *cfg_file =NULL);
  const char *reco_input (char *spec, int ssz) const;
  const char *reco_engine (char *spec, int ssz) const;
  int reco_start (int level =0, const char *log_file =NULL);
  void reco_cleanup ();

  // recognition configuration - convenience
  template <size_t ssz>
    const char *reco_version (char (&spec)[ssz]) const
      {return reco_version(spec, ssz);}
  template <size_t ssz>
    const char *reco_input (char (&spec)[ssz]) const
      {return reco_input(spec, ssz);}
  template <size_t ssz>
    const char *reco_engine (char (&spec)[ssz]) const
      {return reco_engine(spec, ssz);}

  // run time recognition modifications (low level sp_reco)
  int reco_set_src (const char *name, int file =0);
  void reco_loc_user (int azim, int elev =0, int dist =36);
  int reco_add_user (const char *name, int force =2);
  void reco_del_user (const char *name);
  void reco_clr_users ();
  int reco_list_users (char *list, int ssz) const;
  int reco_add_model (const char *topic);
  void reco_del_model (const char *topic);
  void reco_clr_models ();
  int reco_list_models (char *list, int ssz) const;

  // run time reco mods - convenience
  template <size_t ssz>
    int reco_list_users (char (&list)[ssz]) const
      {return reco_list_users(list, ssz);}
  template <size_t ssz>
    int reco_list_models (char (&list)[ssz]) const
      {return reco_list_models(list, ssz);}

  // recognition results (low level sp_parse)
  void reco_listen (int doit =1, int block =1);
  int reco_status ();
  int reco_partial (char *text, int ssz);
  int reco_heard (char *text, char *conf, int choice, int tsz, int csz);
  void reco_phonetic (char *pseq, int choice, int ssz) const;
  int reco_speaker (char *name, int ssz) const;

  // recognition results - convenience
  template <size_t ssz>
    int reco_partial (char (&text)[ssz])
      {return reco_partial(text, ssz);}
  template <size_t tsz, size_t csz>
    int reco_heard (char (&text)[tsz], char (&conf)[csz], int choice =0)
      {return reco_heard(text, conf, choice, tsz, csz);}
  template <size_t ssz>
    void reco_phonetic (char (&pseq)[ssz], int choice =0) const
      {return reco_phonetic(pseq, choice, ssz);} 
  template <size_t ssz>
    int reco_speaker (char (&name)[ssz]) const
      {return reco_speaker(name, ssz);}


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
  int parse_extend (const char *rule, const char *option, int file =0);

  // parsing results (low level sp_parse)
  int parse_analyze (const char *text, const char *conf =NULL);
  int parse_focus (char *token, int ssz) const;
  int parse_focus () {return parse_focus(NULL, 0);}
  int parse_span (int *first =NULL, int *last =NULL) const;
  int parse_top (int n =0);
  int parse_next ();
  int parse_down ();
  int parse_up ();

  // parsing results - convenience
  template <size_t ssz>
    int parse_focus (char (&token)[ssz]) const
      {return parse_focus(token, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  // engine control
  static void eng_start (void *me);
  static void eng_stop (void *me);

  // speech recognition configuration
  int connect_mic (const char *name);
  int connect_file (const char *fname);
  int walk_tree (char *ctxt, const void *n);

  // run time parsing modifications
  int add_option (const char *fname, const char *rule, const char *option);

  // JHC grammar construction
  int load_jhc (const char *fname, int flush =1);
  char *clean_line (char *ans, int len, FILE *in, char ignore);
  char *build_phrase (char *line, void *t, int opt);
  char *trim_wh (char *string);
  void nonterm_chk (const char *rname, const char *gram) const;

  // BNF gramar construction
  int load_bnf (const char *fname, int flush =1);
  void bnf_expansions (const char *cat, const char *tail);
  const char *bnf_token (char *tag, const char *line) const;

  // grammar components
  void add_rule (void *r, const char *tag);
  int all_caps (const char *name);
  void add_nonterm (void *n, const char *tag);
  void add_term (void *n, const char *tag);
  void add_dict (void *n, int cnt, int opt);
  void add_ignore (void *n);
  void add_jump (void *n0, void *n1);
  void end_phrase (void *n);


};


#endif  // once




