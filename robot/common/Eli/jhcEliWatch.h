// jhcEliWatch.h : innate gaze attention behaviors for ELI robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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

#ifndef _JHCELIWATCH_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCELIWATCH_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"             // common video

#include "Geometry/jhcMatrix.h"        // common robot


//= Innate gaze attention behaviors for ELI robot.
// used to be part of jhcEliGrok and could be a derived class but 
// separate components more easily accomodate sets of unrelated behaviors 

class jhcEliWatch 
{
// PRIVATE MEMBER VARIABLES
private:
  char wtarg[9][20];
  jhcMatrix src;
  int seek, up, mid;


// PUBLIC MEMBER VARIABLES
public:
  // watching behaviors bids
  jhcParam wps;
  int freeze, speak, close, sound, stare, face, rise, align;

  // self-orientation parameters
  jhcParam ops;
  int fmin;
  double bored, edge, hnear, pdist, hdec, aimed, rtime;
  

// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcEliWatch ();
  jhcEliWatch ();
  const char *Behavior (int gwin, int dash =1) const;

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname);

  // main functions
  void Reset ();
  void React (class jhcEliGrok *g);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int watch_params (const char *fname);
  int orient_params (const char *fname);

  // innate behaviors
  void cmd_freeze (class jhcEliGrok *g);
  void watch_talker (class jhcEliGrok *g);
  void watch_closest (class jhcEliGrok *g);
  void gaze_sound (class jhcEliGrok *g);
  void gaze_stare (class jhcEliGrok *g);
  void gaze_face (class jhcEliGrok *g);
  void head_rise (class jhcEliGrok *g);
  void head_center (class jhcEliGrok *g);

};


#endif  // once




