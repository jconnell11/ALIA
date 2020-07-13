// jhcScan3D.h : finds and tracks people using a single scanning sensor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2015 IBM Corporation
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

#ifndef _JHCSCAN3D_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSCAN3D_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"           // common video
#include "Data/jhcParam.h"

#include "Depth/jhcSurface3D.h"    // common robot
#include "People/jhcTrack3D.h"


//= Finds and tracks people using a single scanning sensor.

class jhcScan3D : public jhcTrack3D
{
// PRIVATE MEMBER VARIABLES
private:


// PUBLIC MEMBER VARIABLES
public:
  jhcSurface3D sf;

  // parameters
  jhcParam eps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcScan3D ();
  ~jhcScan3D ();
//  void Reset ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions

  // debugging graphics
  int ShowHeads (jhcImg& dest, int raw =0, double sz =8.0);  


// PRIVATE MEMBER FUNCTIONS
private:
  int empty_params (const char *fname);

};


#endif  // once




