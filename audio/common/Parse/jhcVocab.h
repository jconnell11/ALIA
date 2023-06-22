// jhcVocab.h : collection of known words and various input fixes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
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

#ifndef _JHCVOCAB_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCVOCAB_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcGramRule.h"


//= Collection of known words and various input fixes.

class jhcVocab
{
// PRIVATE MEMBER VARIABLES
private:
  static const int nbins = 12;         // number of length bins
  static const int nchar = 40;         // maximum length of any word

  // words of various lengths
  char **wlen[nbins];
  int nwds[nbins], wmax[nbins];

  // temporary outputs
  char clean[200], mark[200];
  char cat[20], unk[nchar], oov[nchar], dup[nchar];
  int worst;

  // category inference
  char sep[6][10], item[6][nchar];
  int fcn[6];


// PUBLIC MEMBER VARIABLES
public:
  int dbg;                             // dbugging messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcVocab ();
  jhcVocab (int nb =100);

  // word list
  void Clear ();
  int GetWords (const jhcGramRule *gram);
  int Add (const char *word);
  int Remove (const char *word);

  // typing correction
  const char *FixTypos (const char *txt);
  const char *Fixed () const {return clean;}

  // category inference
  void InitGuess ();
  const char *NextGuess (const char *txt);
  const char *Mystery () const  {return unk;}
  const char *Category () const {return cat;}
  const char *Confused () const {return oov;}
  const char *Marked () const   {return mark;}

  // utilities
  int ListAll () const;
  int WeedGram (const jhcGramRule *gram, int nt =500) const;
  const char *MarkBad (const char *txt);


// PRIVATE MEMBER FUNCTIONS
private:
  // word list
  bool known (const char *word) const;
  const char *lookup (const char *word) const;  
  int bin (const char *word) const; 
  void extend (int idx, int add);    

  // typing correction
  int try_fadd (char *d, const char *prev, char *word) const;
  int try_frem (char *d, const char *prev, char *word) const;
  int try_badd (char *word, char *next, const char **after) const;
  int try_brem (char *word, char *next, const char **after) const;
  int try_split (char *word) const;
  int try_swap (char *word) const;
  int try_ins (char *word) const;
  int try_sub (char *word) const;

  // category inference
  int gram_fcn (char *word) const;
  int guess_word ();
  int name_ctx (const char *word, int npl);
  int noun_ctx (const char *word);
  int adj_ctx (const char *word);
  int verb_ctx (const char *word);
  bool poss_end (const char *word); 
  bool verb_end (const char *word); 
  bool adv_end (const char *word);

  // string elements
  const char *copy_gap (char *dest, const char *txt) const;
  const char *next_word (char *word, const char *txt) const;
  bool word_part (char c) const;


};


#endif  // once




