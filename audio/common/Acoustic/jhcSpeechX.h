// jhcSpeechX.h : more advanced speech functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
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

#ifndef _JHCSPEECHX_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSPEECHX_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#ifdef JHC_SPDLL
  #include "Acoustic/jhcSpeechDLL.h"    // possibly IBM or Nuance version
#else
  #include "Acoustic/jhcSpRecoMS.h"     // core Microsoft SAPI version
  #include "Acoustic/jhcSpTextMS.h" 
#endif
  #include "Acoustic/jhcGenIO.h"
  #include "Parse/jhcTxtSrc.h"


//= More advanced speech functions based on primitives (from DLLs).
// no longer has base class jhcGenParse, so use jhcSpeechX2 instead
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
//   These should not be used anywhere else (in termninals or non-terminals).
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

#ifdef JHC_SPDLL
  class jhcSpeechX : private jhcSpeechDLL, public jhcGenIO
#else
  class jhcSpeechX : private jhcSpRecoMS, private jhcSpTextMS, public jhcGenIO
#endif
{
// PRIVATE MEMBER VARIABLES
private:
  static const int altp = 200;   /** Max number of alternate pronunciations. */

  // DLLs, configuration files, and grammar
  char ifile[200];
  char rfile[200], rcfg[200], model[80];
  char pfile[200], pcfg[200],  gram[200];
  char tfile[200], tcfg[200];

  // speech results and browsing
  char ph[1500], utt[500], utt0[500], conf[200], spkr[200], frag[80];
  int cf, nw;

  // acoustic status
  char qtext[500], atext[500];
  unsigned int now, last;
  int txtin, hear, talk, tlast, tlock;

  // keyboard interaction
  int suspend;

  // alternate TTS pronunciations
  jhcTxtSrc txt;
  char key[altp][40], subst[altp][40];
  int nalt;


// PUBLIC MEMBER VARIABLES
public:
  // printing options
  int phon;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and destruction
  jhcSpeechX ();
  ~jhcSpeechX ();
  int Init (int dbg =0, int noisy =0);
  int InitTTS (int noisy =0);
  void Reset ();
  int Ready () const {if (__min(__min(r_ok, p_ok), t_ok) <= 0) return 0; return 1;}
  bool Escape ();
  bool Paused () const {return(suspend > 0);}
  void Dictation (double wt =1.0) {dict_wt = wt;}

  // configuration
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;
  void SetTag (int n);
  const char *Tag () const {return tag;}
  int SetMic (const char *name);
  const char *MicName (char *name, int nick, int ssz) const;
  int SetUser (const char *name, int strict =0, int force =2);
  const char *UserName (int chk =1);
  int FirstName (char *name, int ssz);
  int PrintCfg ();

  // configuration - convenience
  template <size_t ssz> 
    const char *MicName (char (&name)[ssz], int nick =1) const 
      {return MicName(name, nick, ssz);}
  template <size_t ssz> 
    int FirstName (char (&name)[ssz])
      {return FirstName(name, ssz);}

  // top level convenience functions and status
  void Inject (const char *txt, int stop =0);
  virtual int Update (int reco =1, int prolong =0);    // allow to be overridden
  void Issue ();
  int Hearing () const {return hear;}
  int Talking () const {return talk;}
  double Silence () const;
  void ClrTimer () {last = 0;}
  const char *Partial () const {return utt0;}
  const char *Heard () const   {return utt;} 

  // speech processing
  void Listen (int reco =1, int block =1);
  void SuggestPos (double azim, double elev =0.0, double dist =36.0);
  int SuggestUser (const char *name, ...);
  int AwaitOrQuit (double secs =10.0);
  int AwaitPhrase (double secs =10.0);
  int AwaitQuiet (double secs =10.0);
  const char *SpeakerId ();
  
  // parsing 
  void SetGrammar (const char *fname, ...);
  void ClearGrammar (int keep =1);
  int LoadGrammar (const char *fname, ...);
  int MarkRule (const char *name =NULL, int val =1);
  int ExtendRule (const char *name, const char *phrase, int file =0);

  // results
  int Confidence () const        {return cf;}
  const char *Input () const     {return utt;} 
  const char *Phonemes () const  {return ph;}
  const char *WordConfs () const {return conf;}
  int WordMatch () const         {return nw;}
  const char *Root ();
  const char *TopCat ();
  char *SlotValuePairs (char *alist, int close, int ssz);

  // results - convenience
  template <size_t ssz> 
    char *SlotValuePairs (char (&alist)[ssz], int close =0)
      {return SlotValuePairs(alist, close, ssz);}

  // debugging
  void PrintTree (int top =1);

  // speech synthesis
  int Say (const char *msg, ...);
  int Say (int bid, const char *msg, ...);
  int Utter ();
  int Finish (double secs =10.0);
  int ChkOutput ();
  void ShutUp ();
  const char *Said () const {return qtext;}
  int SetVoice (const char *spec, int pct =0);
  const char *VoiceName (char *name, int nick, int ssz) const;

  // speech - convenience
  template <size_t ssz> 
    const char *VoiceName (char (&name)[ssz], int nick =1) const
      {return VoiceName(name, nick, ssz);}

  // alternate pronunciations
  int LoadAlt (const char *fname, int clr =1);
  int Fixes () const {return nalt;}


// PRIVATE MEMBER FUNCTIONS
private:
  // configuration
  int update_model ();

  // association list
  int tree_major (char *ans, int ssz);
  void tree_slots (char *alist, int fake, int close, int ssz);
  int all_caps (const char *name) const;
  char *get_words (char *dest, int first, int last, int ssz) const;
  char *fake_words (char *dest, int first, int last, int ssz) const;
  char *next_token (char **src, const char *delim) const;
  void append (char *dest, const char *extra, int ssz) const;

  // debugging
  void print_focus (int indent, int start, int end);

  // alternate pronunciations
  const char *alt_pron (const char *src);


};


#endif  // once




