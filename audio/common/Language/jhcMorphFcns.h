// jhcMorphFcns.h : converts words from base form plus tag to surface form
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#ifndef _JHCMORPHFCNS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMORPHFCNS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcSlotVal.h"


//= Converts words from base form plus tag to surface form.
// Guesses proper form using some standard rules for English
// but allows exceptions and overrides (in case the rules are wrong).
// Some more mainstream stemmer package like Snowball might also be used.

class jhcMorphFcns : private jhcSlotVal
{
// PRIVATE MEMBER VARIABLES
private:
  // number of exceptions for each category
  static const int nmax = 100; 
  static const int vmax = 100; 
  static const int amax = 100; 

  // lookup tables for irregular forms
  char nsing[nmax][40], npl[nmax][40]; 
  char vimp[vmax][40], vpres[vmax][40], vprog[vmax][40], vpast[vmax][40];
  char adj[amax][40], comp[amax][40], sup[amax][40];
  int nn, nv, na;

  // temporary conversion result
  char btemp[80], stemp[80];


// PUBLIC MEMBER VARIABLES
public:


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcMorphFcns ();
  jhcMorphFcns ();

  // configuration
  void ClrExcept ();
  int LoadExcept (const char *fname, int append =1);
  int SaveExcept (const char *fname) const;
  
  // main functions
  int AddVocab (class jhcGenParse *p, const char *fname);
  int AddVocab (class jhcSpeechX *p, const char *fname);
 
  // derived forms
  const char *SurfWord (const char *base, UL32 tags);

  // normalization
  const char *BaseWord (const char *surf, UL32 tags);
  
  // graphizer functions
  const char *NounLex (UL32& tags, char *pair) const;
  const char *VerbLex (UL32& tags, char *pair) const;
  const char *AdjLex (UL32& tags, char *pair) const;

  // debugging tools
  int LexDeriv (const char *gram, int chk =1, const char *deriv =NULL);
  int LexBase (const char *deriv, int chk =1, const char *morph =NULL);


// PRIVATE MEMBER FUNCTIONS
private:
  // configuration
  int add_morph (int &n, char key[][40], char val[][40], int wmax, const char *base, const char *surf);
  char *clean_line (char *ans, int ssz, FILE *in) const;
  UL32 parse_line (char *base, char *surf, const char *line) const;
  int trim_tail (char *dest, const char *start, const char *end, int ssz) const;

  // derived forms
  const char *lookup_surf (const char *surf, UL32 tags);
  char *noun_morph (char *val, UL32 tags) const;
  char *verb_morph (char *val, UL32 tags) const;
  char *adj_morph (char *val, UL32 tags) const;
  char *add_s (char *val, int ssz) const;
  char *add_vowel (char *val, const char *suffix, int ssz) const;

  // normalization
  const char *lookup_base (const char *surf, UL32 tags) const;
  char *noun_stem (char *val, UL32 tags) const;
  char *verb_stem (char *val, UL32 tags) const;
  char *adj_stem (char *val, UL32 tags) const;
  char *rem_s (char *val) const;
  char *rem_vowel (char *val, int strip) const;

  // shared functions
  const char *scan_for (const char *probe, const char key[][40], const char val[][40], int n) const;
  bool vowel (char c) const;

  // graphizer versions
  UL32 gram_tag (const char *pair) const;

  // debugging tools
  int base2surf (FILE *out, FILE *in, UL32 tags, int chk);
  bool base_sec (const char *line, UL32 tags) const;
  void cat_hdr (FILE *out, UL32 tags) const;
  int surf2base (FILE *out, FILE *in, UL32 tags, int chk);
  bool surf_sec (const char *line, UL32 tags) const;
  void deriv_hdr (FILE *out, UL32 tags) const;
  const char *cat_txt (UL32 tags) const;


};


#endif  // once




