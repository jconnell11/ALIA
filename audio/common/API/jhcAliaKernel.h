// jhcAliaKernel.h : generic interface to a pool of grounding functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
// Copyright 2023-2024 Etaoin Systems
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

#include "API/jhcAliaNote.h"      
#include "API/jhcAliaDesc.h"      


//= Generic interface to a pool of grounding functions.
// each kernel pool handles one or more named functions thru Start/Status/Stop

class jhcAliaKernel
{
// PROTECTED MEMBER VARIABLES
protected:
  char tag[80];                    /** Base name of associated files. */
  jhcAliaKernel *next;             /** Other pools of functions.      */
  int alloc;                       /** Whether to delete self at end. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  virtual ~jhcAliaKernel () {};
  jhcAliaKernel () {*tag = '\0'; next = NULL; alloc = 0;}
  const char *BaseTag () const {return tag;}
  jhcAliaKernel *NextPool () const {return next;}
  int CleanUp () const {return alloc;}
  virtual void AddFcns (jhcAliaKernel& pool) =0;

  // main functions
  virtual void Platform (void *soma) =0;         // be careful of casts
  virtual void Reset (jhcAliaNote& attn) =0;                    
  virtual void Volunteer () =0;
  virtual int Start (const jhcAliaDesc& desc, int bid) =0;
  virtual int Status (const jhcAliaDesc& desc, int inst) =0;
  virtual int Stop (const jhcAliaDesc& desc, int inst) =0;
  
};
