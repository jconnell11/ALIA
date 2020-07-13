// jhcLocalNav.h : collection of local robot navigation routines
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2016 IBM Corporation
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

#ifndef _JHCLOCALNAV_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCLOCALNAV_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"            // common video
#include "Data/jhcParam.h"

#include "Body/jhcEliBody.h"        // common robot
#include "Depth/jhcFollow3D.h" 
#include "Depth/jhcObstacle3D.h" 
#include "Depth/jhcFindPlane.h" 
#include "Depth/jhcSurface3D.h" 


//= Collection of local robot navigation routines.

class jhcLocalNav
{
friend class CThursdayDoc;  // for debugging

// PRIVATE MEMBER VARIABLES
private:
  jhcFindPlane pf;          // floor finder
  jhcSurface3D sf;          // basic depth analysis
  jhcFollow3D fol;          // person following
  jhcObstacle3D obs;        // obstacle avoidance
  int comm;

  // following data
  int fmode;
  double fazm, fazm0, fdist, fdist0; 

  // sound data
  int nmode;
  double nazm, nazm0;

  // ballistic data
  double dazm;


// PUBLIC MEMBER VARIABLES
public:
  // robot mechanicals & sensors
  jhcEliBody *eb;            

  // following parameters
  jhcParam fps;
  double fdown, finit, foff, gtime, rtime, mtime, align, dead, skew;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcLocalNav ();
  ~jhcLocalNav ();
  void Bind (jhcEliBody *body);
  int Reset (int body =1);

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // person following
  void HipReset ();
  int HipFollow (const jhcImg& d16, int base =0);
  int HipExceed (double d) const {return(((fdist0 <= d) && (fdist > d)) ? 1 : 0);}

  // sound orientation
  void SndReset ();
  int SndTrack (int talk =0, int base =0);
  int SndExceed (double a) const {return(((fabs(nazm) < a) && (fabs(nazm0) > a)) ? 1 : 0);}

  // ballist turn
  void DirReset (double desire);
  int DirTurn ();


// PRIVATE MEMBER FUNCTIONS
private:
  int follow_params (const char *fname);

};


#endif  // once




