// jhcTalkFcn.h : string and semantic net language output for ALIA system
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

#ifndef _JHCTALKFCN_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTALKFCN_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Language/jhcDegrapher.h"     // common audio

#include "Action/jhcTimedFcns.h"       // common robot


//= String and semantic net language output for ALIA system.
// simplest form just outputs literal string in "-str-" of main "pat" node 
// fancier form has embedded "?0" ... "?9" corresponding to "arg0" ... "arg9"
// system attempts to generate descriptive string for these nodes
// NOTE: does not wait for output to be fully actualized (e.g. spoken)
// requests basically generate quick events which may be overwritten

class jhcTalkFcn : public jhcTimedFcns
{
  static const int smax = 10;        /** Maximum pending things to say. */

// PRIVATE MEMBER VARIABLES
private:
  // other parts of system
  jhcAliaNote *attn;

  // string generation
  jhcDegrapher dg;
  char full[smax][500];

  // output arbitration
  const char *winner;
  int imp;


// PUBLIC MEMBER VARIABLES
public:
  int noisy;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcTalkFcn ();
  jhcTalkFcn ();
  int Output (char *out, int ssz);
  template <size_t ssz>
    int *Output (char (&outt)[ssz])              // for convenience
      {return Output(out, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  void local_reset (jhcAliaNote *top) {attn = top;}
  int local_start (const jhcAliaDesc *desc, int i);
  int local_status (const jhcAliaDesc *desc, int i);

  // user literal output
  JCMD_DEF(echo_wds);
  int build_string (const jhcAliaDesc *desc, int inst);


};


#endif  // once




