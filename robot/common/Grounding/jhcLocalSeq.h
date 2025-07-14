// jhcLocalSeq.h : interface to Manus sensor sequence kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include "Body/jhcManusX.h"            // common robot

#include "Kernel/jhcStdKern.h"       


//= Interface to Manus sensor sequence kernel for ALIA system.

class jhcLocalSeq : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  // link to hardware
  jhcManusGrok *rwi;


// PUBLIC MEMBER VARIABLES
public:
  int dbg;                   // controls diagnostic messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcLocalSeq ();
  jhcLocalSeq ();


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  void local_platform (void *soma, const char *kind);
  void local_reset (jhcAliaNote& top);
  void local_volunteer ();
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

};

