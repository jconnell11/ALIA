// jhcSpFix.h : text substitution utilities for speech reco and TTS
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Etaoin Systems
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

#pragma once

#include <stdio.h>

#include "jhcGlobal.h"


//= Text substitution utilities for speech reco and TTS.
// adds replacement of erroneous strings using "misheard.map" table
// adds phonetic re-spelling of TTS words using "pronounce.map" table

class jhcSpFix
{
// PRIVATE MEMBER VARIABLES
private:
  static const int fmax = 200;         // max misheard
  static const int pmax = 200;         // max pronunciation

  // speech transcription repairs
  char reco[fmax][40], subst[fmax][40];
  int rlen[fmax];
  int nfix;

  // TTS pronunciation tweaks     
  char word[pmax][40], pron[pmax][40];
  int wlen[pmax];
  int nsp;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  jhcSpFix () {nfix = 0; nsp = 0;}
  int LoadFix (const char *fname =NULL, int path =1);
  int LoadPron (const char *fname =NULL, int path =1);

  // main functions
  int FixUp (char *fix, int ssz, const char *spin) const
    {return replace(fix, ssz, spin, reco, rlen, subst, nfix);}
  int ReSpell (char *tts, int ssz, const char *msg) const
    {return replace(tts, ssz, msg, word, wlen, pron, nsp);}

  // convenience functions
  template <size_t ssz>
    int FixUp (char (&fix)[ssz], const char *spin) const
      {return FixUp(fix, ssz, spin);}
  template <size_t ssz>
    int ReSpell (char (&tts)[ssz], const char *msg) const
      {return ReSpell(tts, ssz, msg);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  FILE *map_file (const char *fname, int path, const char *def) const;
  int trim_wh (char *nice, int ssz, const char *raw) const;

  // main functions
  int replace (char *out, int ssz, const char *in,
               const char k[][40], const int cnt[],
               const char s[][40], int n) const;

};