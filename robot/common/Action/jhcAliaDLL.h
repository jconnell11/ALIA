// jhcAliaDLL.h : add procedural grounding functions from a DLL to ALIA
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

#ifndef _JHCALIADLL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIADLL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Action/jhcAliaKernel.h"   


//= Add procedural grounding functions from a DLL to ALIA.
// DLL should contain these 6 functions:
// <pre>
//   void pool_bind (void *body)
//   void pool_reset (jhcAliaNote *attn)
//   void pool_volunteer ()
//   int pool_start (const jhcAliaDesc *desc, int bid)
//     returns new instance number (>= 0) if success, -1 for problem, -2 for unknown
//   int pool_status (const jhcAliaDesc *desc, int i)
//     returns positive for done, 0 for still running, -1 for failure, -2 if unknown
//   int pool_stop (const jhcAliaDesc *desc, int i)
//     returns positive for convenience, -2 if unknown (courtesy call - should never wait)
// </pre>

class jhcAliaDLL : public jhcAliaKernel
{
// PRIVATE MEMBER VARIABLES
private:
  // basic DLL handle
  void *lib;

  // other pools of functions
  jhcAliaKernel *next;

  // included configuration functions
  void (*local_bind)(void *body);
  void (*local_reset)(jhcAliaNote *attn);

  // included operational functions
  void (*local_volunteer)();
  int (*local_start)(const jhcAliaDesc *desc, int bid);
  int (*local_status)(const jhcAliaDesc *desc, int bid);
  int (*local_stop)(const jhcAliaDesc *desc, int bid);


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaDLL ();
  jhcAliaDLL (const char *file =NULL);
  int Load (const char *file);
  void AddFcns (jhcAliaKernel *pool);
  void Bind (void *body);
  void Reset (jhcAliaNote *attn);                    

  // main functions
  void Volunteer ();
  int Start (const jhcAliaDesc *desc, int bid);
  int Status (const jhcAliaDesc *desc, int inst);
  int Stop (const jhcAliaDesc *desc, int inst);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  int close ();


};


#endif  // once




