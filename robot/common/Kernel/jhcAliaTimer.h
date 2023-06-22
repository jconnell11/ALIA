// jhcAliaTimer.h : simple timer function grounding for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023 Etaoin Systems
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

#ifndef _JHCALIATIMER_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIATIMER_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Kernel/jhcStdKern.h"         


//= Simple timer function grounding for ALIA system.

class jhcAliaTimer : public jhcStdKern
{
// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaTimer ();
  jhcAliaTimer ();

  // main functions
  JCMD_DEF(time_delay);


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

  // main functions
  double find_secs (const jhcAliaDesc *amt) const;


};


#endif  // once




