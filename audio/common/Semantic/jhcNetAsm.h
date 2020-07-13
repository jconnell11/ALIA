// jhcNetAsm.h : builds semantic network from output of syntactic parser
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2016 IBM Corporation
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

#ifndef _JHCNETASM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNETASM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcSlotVal.h"
#include "Semantic/jhcTripleMem.h"


//= Builds semantic network from output of syntactic parser.
//
// constituents:
//   evt  = top matrix verb
//   obj  = direct object of transitive
//   dest = destination of ditransitive
//
// intermediate results:
//   pp  = prepositional phrase waiting to attach
//   np  = noun phrase being built
//   np0 = previous NP (for "of" phrase)
//
// flags:
//   need_obj  = expect NP for transitive verb
//   need_dest = expect PP for destination of ditransitive
//   need_pref = expect NP for generic prepositional phrase
//   need_dref = expect NP for destination like "onto"
//   need_base = expect NP for owner in "of" phrases
//   mass      = dual AKO links for "bottle of milk"
//
// dialog status:
//   attn = pay attention to speech or not
//   rc   = 1:finished, -1:complaint, 0:give-up
//   miss = code for problem with input (4 bits)
//   nag  = previous problem code (fix multiple things)

class jhcNetAsm : private jhcSlotVal
{
// PRIVATE MEMBER VARIABLES
private:
  char evt[80], obj[80], dest[80], np[80], pp[80], np0[80];
  char problem[80], evt_txt[80], obj_txt[80], dest_txt[80];
  int need_obj, need_dest, need_dref, need_pref, need_base, mass;
  int attn, rc, miss, nag;


// PUBLIC MEMBER VARIABLES
public:
  jhcTripleMem *mem;
  double edit, turn, flush;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcNetAsm ();
  ~jhcNetAsm ();
  void Reset (); 
  void Ack ();
  int Alert () {return attn;}
  int Status () {return rc;}

  // status of partial interpretation
  const char *MajorIssue ();
  const char *Action ();
  const char *Object ();
  const char *Dest ();

  // main functions
  int Build (const char *tags, double sil);


// PRIVATE MEMBER FUNCTIONS
private:
  // status functions
  char *np_base (char *base, const char *node, const char *def, int ssz) const;

  // main functions
  void connect_up ();
  int chk_complete (double sil);

  // tag handling
  void build_np (const char *alist);
  void build_dest (const char *alist);
  void finish_dest ();
  void build_pos (const char *alist);
  void finish_pos ();
  void build_part (const char *alist);
  void finish_part ();
  void build_evt (const char *alist);
  void finish_evt ();


};


#endif  // once




