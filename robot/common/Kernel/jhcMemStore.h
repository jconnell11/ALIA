// jhcMemStore.h : interface for explicit LTM formation in ALIA system
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

#ifndef _JHCMEMSTORE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMEMSTORE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Reasoning/jhcActionTree.h"   // common audio
#include "Reasoning/jhcDeclMem.h"      

#include "Kernel/jhcStdKern.h"


//= Interface for explicit LTM formation in ALIA system.
// Note: could hang jhcDeclMem::DejaVu off local_volunteer

class jhcMemStore : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  jhcActionTree *atree;                
  jhcDeclMem *dmem;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcMemStore ();
  jhcMemStore ();
  void Bind (jhcDeclMem& ltm) {dmem = &ltm;}

  // main functions
  JCMD_DEF(mem_form);


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  void local_reset (jhcAliaNote& top) 
    {atree = dynamic_cast<jhcActionTree *>(&top);}
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

  // main function
  void note_that (jhcNetNode *focus, const jhcNetNode *root) const;


};


#endif  // once




