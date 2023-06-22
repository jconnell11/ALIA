// jhcInteractFSM.h : sequential and sensor behaviors for Manus robot 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _JHCINTERACTFSM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCINTERACTFSM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"             // common video

#include "Body/jhcManusX.h"            // common robot


//= Sequential and sensor behaviors for Manus robot.

class jhcInteractFSM
{
// PRIVATE MEMBER VARIABLES
private:
  double dlim, hlim, mlim, eprev, hobj, dlast, wprev;
  int phase, wcnt, dcnt, dok;


// PUBLIC MEMBER VARIABLES
public:
  int noisy;
  jhcParam eps;
  jhcManusX *body;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcInteractFSM ();
  jhcInteractFSM ();
  void Reset ();
  int Status (int total);
  void BindBody (jhcManusX *b) {body = b;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // hand behaviors
  int FullOpen (int step0 =1);
  int GoodGrip (int step0 =1);

  // drive behaviors
  int Advance (double amt, int step0 =1);
  int Standoff (double stop, int step0 =1);

  // lift behaviors
  int Altitude (double val, int step0 =1);
  int RiseBy (double amt, int step0 =1);
  int RiseTop (int step0 =1);

  // combined behaviors
  int Acquire (int thin, int step0 =1);
  int Deposit (int step0 =1);
  int AddTop (int step0 =1);
  int RemTop (int step0 =1);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int empty_params (const char *fname);

};


#endif  // once




