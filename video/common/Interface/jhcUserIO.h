// jhcUserIO.h : simple interactive text console 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
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


//= Simple interactive text console.

class jhcUserIO
{
// PRIVATE MEMBER VARIABLES
private:
  char prior[200], input[200];
  int quit, fill, seq;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcUserIO ();
  jhcUserIO ();
  int Done () const {return quit;}

  // main functions
  void Start ();
  const char *Get ();
  void Post (const char *msg, int user=0);
  void Stop ();


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  int multikey (int ch);
  bool special (int ch);

};