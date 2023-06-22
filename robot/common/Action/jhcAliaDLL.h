// jhcAliaDLL.h : add procedural grounding functions from a DLL to ALIA
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#ifndef _JHCALIADLL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIADLL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "API/jhcAliaKernel.h"   


//= Add procedural grounding functions from a DLL to ALIA.
// DLL should contain these 6 functions:
// <pre>
//   const char *gnd_name ()
//     returns tag associated with KB0 operator, rule, and word files
//   void gnd_platform (void *soma)
//     connects functions to a real-world interface for a body
//   void gnd_reset (jhcAliaNote& attn)
//     clears any state for start of a new run
//   void gnd_volunteer ()
//     monitor conditions and spontaneously generate events
//   int gnd_start (const jhcAliaDesc& desc, int bid)
//     start described function using given importance bid
//   int gnd_status (const jhcAliaDesc& desc, int inst)
//     check whether described function instance has completed yet
//   int gnd_stop (const jhcAliaDesc& desc, int inst)
//     stop described function instance (or all if instance negative)
// </pre>
// see Grounding/alia_gnd.x for example starter shell

class jhcAliaDLL : public jhcAliaKernel
{
// PRIVATE MEMBER VARIABLES
private:
  // basic DLL handle
  void *lib;

  // included configuration functions
  const char * (*local_name)();
  void (*local_platform)(void *soma);
  void (*local_reset)(jhcAliaNote& attn);

  // included operational functions
  void (*local_volunteer)();
  int (*local_start)(const jhcAliaDesc& desc, int bid);
  int (*local_status)(const jhcAliaDesc& desc, int bid);
  int (*local_stop)(const jhcAliaDesc& desc, int bid);


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaDLL ();
  jhcAliaDLL (const char *file =NULL);
  int Load (const char *file);
  void AddFcns (jhcAliaKernel& pool);
  void Platform (void *soma);
  void Reset (jhcAliaNote& attn);                    

  // main functions
  void Volunteer ();
  int Start (const jhcAliaDesc& desc, int bid);
  int Status (const jhcAliaDesc& desc, int inst);
  int Stop (const jhcAliaDesc& desc, int inst);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  int close ();


};


#endif  // once




