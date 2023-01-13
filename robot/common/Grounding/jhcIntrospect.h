// jhcIntrospect.h : examines action tree in ALIA system to supply reasons
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2022-2023 Etaoin Systems
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

#ifndef _JHCINTROSPECT_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCINTROSPECT_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Reasoning/jhcActionTree.h"   // common audio

#include "Action/jhcAliaDir.h"         // common robot

#include "Action/jhcStdKern.h"         


//= Examines action tree in ALIA system to supply reasons.

class jhcIntrospect : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  // action tree
  jhcActionTree *atree;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcIntrospect ();
  jhcIntrospect ();
 
  // failure determination
  JCMD_DEF(why_try);
  const jhcAliaDir *failed_dir (const jhcAliaChain *start) const;
  bool multi_act (const jhcAliaChain *start, const jhcAliaChain *now, int some) const;

  // failure messages 
  int cuz_err (jhcAliaDesc *fail, const jhcGraphlet *sit);
  int cuz_find (jhcAliaDesc *fail, const jhcAliaDir *dir);
  int cuz_do (jhcAliaDesc *fail, const jhcAliaDir *dir);


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  void local_reset (jhcAliaNote *top) 
    {atree = dynamic_cast<jhcActionTree *>(top);}
  int local_start (const jhcAliaDesc *desc, int i);
  int local_status (const jhcAliaDesc *desc, int i);


};


#endif  // once




