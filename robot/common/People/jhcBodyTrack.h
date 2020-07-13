// jhcBodyTrack.h : datastructure for tracked person and pointing direction
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 IBM Corporation
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

#ifndef _JHCBODYTRACK_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBODYTRACK_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Geometry/jhcKalVec.h"
#include "People/jhcBodyParts.h"


//= Datastructure for tracked person and pointing direction.
// base member holds coordinates of center of head
// hands (left, right) are offsets relative to head (for stability)
// designations "left" and "right" are constant but may not be accurate

class jhcBodyTrack : public jhcKalVec
{
// PRIVATE MEMBER VARIABLES
private:
  // state variables
  int id, ltrk, rtrk;


// PUBLIC MEMBER VARIABLES
public:
  // tracked positions and velocities
  jhcKalVec lf, rt, sin, dex, hv, lv, rv;
  int vis; 

  // track validation and removal counts
  int hit0, miss0, hit, miss;
  double dt;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcBodyTrack ();
  void SetTrack (int h0 =1, int m0 =1, int h =1, int m =1, double secs =0.033)
    {hit0 = h0; miss0 = m0; hit = h; miss = m; dt = secs;}

  // read only functions
  int TrackID () const {return id;}
  int LeftOK () const  {return(((id > 0) && (ltrk > 0)) ? 1 : 0);}
  int RightOK () const {return(((id > 0) && (rtrk > 0)) ? 1 : 0);}

  // main functions
  int InitAll (const jhcBodyParts& d, int suggest);
  int UpdateAll (const jhcBodyParts& d, int suggest);
  int PenalizeAll ();


// PRIVATE MEMBER FUNCTIONS
private:
  void clr_lf ();
  void clr_rt ();


};


#endif  // once




