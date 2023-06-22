// jhcManusRWI.h : collection of real world interfaces for Manus forklift robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#ifndef _JHCMANUSRWI_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMANUSRWI_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Body/jhcManusX.h"

#include "RWI/jhcBackgRWI.h"


//= Collection of real world interfaces for Manus forklift robot.
// holds basic body control and sensors as well as follow-on processing modules
// generally processing belongs here while links to FCNs are in a kernel class
// allows attachment of different versions of body but assumes not shared

class jhcManusRWI : public jhcBackgRWI
{
// PRIVATE MEMBER VARIABLES
private:
  int seen;


// PUBLIC MEMBER VARIABLES
public:
  jhcManusX *body;                     // allow physical or simulator
  class jhcStackSeg *seg;
  class jhcPatchProps *ext;
  class jhcInteractFSM *fsm;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcManusRWI ();
  jhcManusRWI ();
  void BindBody (jhcManusX *b);
  void SetSize (int x, int y);

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;

  // main functions
  void Reset ();
  void Stop ();


// PRIVATE MEMBER FUNCTIONS
private:
  // interaction overrrides
  void body_update ();
  void body_issue ();


};


#endif  // once




