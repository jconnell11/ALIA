// jhcBeacon.h : generates entry for Jeff's process management table
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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

#ifndef _JHCBEACON_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBEACON_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Comm/jhcJSON.h"


//= Generates entry for Jeff's process management table.
// figures out process ID, working directory, current time, etc.

class jhcBeacon
{
// PRIVATE MEMBER VARIABLES
private:
  jhcJSON pod;
  __int64 last, slast;


// PUBLIC MEMBER VARIABLES
public:
  char hdr[40], dns[80], snd[40];
  double gap, sgap;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcBeacon ();

  // main functions
  void Init (const char *name);
  const jhcJSON *Update ();
  int Sound (int any);


};


#endif  // once




