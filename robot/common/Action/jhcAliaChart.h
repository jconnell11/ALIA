// jhcAliaChart.h : display of statistics and mood for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
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

#ifndef _JHCALIACHART_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIACHART_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Interface/jhcDisplay.h"      // common video

#include "Action/jhcAliaStats.h"       // common robot


//= Display of statistics and mood for ALIA system.

class jhcAliaChart
{
// PRIVATE MEMBER VARIABLES
private:
  char msg[80];
  jhcAliaStats *stat;
  int w0, h0;                        


// PUBLIC MEMBER VARIABLES
public:
  double hz;                           // samples per second
  int ht;                              // desired graph display height


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaChart ();
  jhcAliaChart ();
  void Bind (jhcAliaStats& s) {stat = &s;}
 
  // graphical display
  void Memory (jhcDisplay& d);
  void Audio (jhcDisplay& d);
  void Physical (jhcDisplay& d);
  void Wheels (jhcDisplay& d);
  void Neck (jhcDisplay& d);

  // text display
  void Mood (jhcDisplay& d) const;
  const char *MoodTxt ();


// PRIVATE MEMBER FUNCTIONS
private:
  // graphical display
  void resize (jhcDisplay& d);
  void restore (jhcDisplay& d) const;


};


#endif  // once




