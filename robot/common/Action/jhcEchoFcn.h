// jhcEchoFcn.h : default handler for non-grounded ALIA functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
// Copyright 2021-2023 Etaoin Systems
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

#ifndef _JHCECHOFCN_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCECHOFCN_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "API/jhcAliaKernel.h"        


// Default handler for non-grounded ALIA functions.
// just echoes name of function and asks if done yet
// differs from jhcTimedFcns in that other pools called first
// this accepts all function names and is the last to be tried

class jhcEchoFcn : public jhcAliaKernel
{
// PUBLIC MEMBER VARIABLES
public:
  int noisy;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcEchoFcn ();
  jhcEchoFcn ();
  void AddFcns (class jhcAliaKernel& pool);
  void Platform (void *soma);
  void Reset (jhcAliaNote& attn);                    

  // main functions
  void Volunteer ();
  int Start (const jhcAliaDesc& desc, int bid);
  int Status (const jhcAliaDesc& desc, int inst);
  int Stop (const jhcAliaDesc& desc, int inst);


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  void fcn_args (const jhcAliaDesc& desc);


};


#endif  // once




