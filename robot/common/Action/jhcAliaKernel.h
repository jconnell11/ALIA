// jhcAliaKernel.h : generic interface to a pool of grounding functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#ifndef _JHCALIAKERNEL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIAKERNEL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Reasoning/jhcAliaNote.h"      // common audio
#include "Semantic/jhcAliaDesc.h"      


//= Generic interface to a pool of grounding functions.
// each kernel pool handles one or more named functions thru Start/Status/Stop

class jhcAliaKernel
{
// PROTECTED MEMBER VARIABLES
protected:
  double ver;                      /** Current version of functions.  */
  char tag[80];                    /** Base name of associated files. */
  jhcAliaKernel *next;             /** Other pools of functions.      */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  jhcAliaKernel () {ver = 1.00; *tag = '\0'; next = NULL;}
  double Version () {return ver;}
  const char *BaseTag () const {return tag;}
  const jhcAliaKernel *NextPool () const {return next;}
  virtual ~jhcAliaKernel () {};
  virtual void AddFcns (jhcAliaKernel *pool) =0;
  virtual void Reset (jhcAliaNote *attn) =0;                    

  // main functions
  virtual void Volunteer () =0;
  virtual int Start (const jhcAliaDesc *desc, int bid) =0;
  virtual int Status (const jhcAliaDesc *desc, int inst) =0;
  virtual int Stop (const jhcAliaDesc *desc, int inst) =0;
  
};


#endif  // once




