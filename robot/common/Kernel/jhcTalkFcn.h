// jhcTalkFcn.h : string and semantic net language output for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "jhcGlobal.h"

#include "Language/jhcDegrapher.h"     // common audio

#include "Kernel/jhcStdKern.h"      


//= String and semantic net language output for ALIA system.
// simplest form just outputs literal string in "-str-" of main "pat" node 
// fancier form has embedded "?0" ... "?9" corresponding to "arg0" ... "arg9"
// system attempts to generate descriptive string for these nodes
// NOTE: does not wait for output to be fully actualized (e.g. spoken)
// requests basically generate quick events which may be overwritten

class jhcTalkFcn : public jhcStdKern
{
  static const int smax = 10;          /** Maximum pending things to say. */

// PRIVATE MEMBER VARIABLES
private:
  // status of speech input and TTS output 
  class jhcAliaCore *core;

  // string generation
  char full[smax][500];

  // output arbitration
  char winner[500];
  UL32 finish;
  int imp;


// PUBLIC MEMBER VARIABLES
public:
  // network to sentence
  jhcDegrapher dg;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcTalkFcn ();
  jhcTalkFcn ();
  void Bind (class jhcAliaCore *c);
  int Output (char *out, int ssz);
  int Busy () const {return((finish != 0) ? 1 : 0);}

  // covenience
  template <size_t ssz>
    int *Output (char (&out)[ssz])      
      {return Output(out, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  void local_reset (jhcAliaNote& top);
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

  // user literal output
  JCMD_DEF(talk_echo);
  JCMD_DEF(talk_wait);
  void q_mark (char *txt) const;
  int build_string (const jhcAliaDesc& desc, int inst);

  // string cleanup
  int fix_surface (char *txt);
  void fix_itis (char *txt);
  void fix_verb (char *txt);
  void fix_det (char *txt);
  void fix_abbrev (char *txt);
  void fix_num (char *txt);
  void convert_all (const char *pat, const char *rep, char *txt, int wd);


};

