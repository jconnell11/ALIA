// jhcAliaStats.h : monitors internal processes of ALIA reasoner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
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

#ifndef _JHCALIASTATS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIASTATS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"               // common video

#include "Reasoning/jhcActionTree.h"   // common audio


//= Monitors internal processes of ALIA reasoner.

class jhcAliaStats
{
// PRIVATE MEMBER VARIABLES
private:
  jhcArr goal, wmem, hmem;             // core operations
  jhcArr mcmd, mips, rcmd, rdps;       // wheel servos
  jhcArr pcmd, pdeg, tcmd, tdeg;       // neck servos
  int sz, fill;

  // original graph size
  int w0, h0;


// PUBLIC MEMBER VARIABLES
public:
  // desired graph size
  int ht;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaStats ();
  jhcAliaStats (int n =600);
  void SetSize (int n =600);
  double Len () const {return sz;}
 
  // main functions
  void Reset ();
  void Thought (jhcActionTree& atree);
  void Drive (double m, double mest, double r, double rest);
  void Gaze (double p, double pest, double t, double test);
  void Shift () {fill++;}

  // graphical display
  void Memory (class jhcDisplay *d, int i =-1, int j =0, double hz =30.0);
  void Wheels (class jhcDisplay *d, int i =-1, int j =0);
  void Neck (class jhcDisplay *d, int i =-1, int j =0);


// PRIVATE MEMBER FUNCTIONS
private:
  // graphical display
  int corner (int& x, int& y, int i, int j, class jhcDisplay *d);
  void restore (jhcDisplay *d) const;

};


#endif  // once




