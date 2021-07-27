// jhcBodyData.h : datastructure for tracked person and pointing directions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2020 IBM Corporation
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

#ifndef _JHCBODYDATA_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBODYDATA_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Geometry/jhcKalVec.h"
#include "People/jhcBodyData.h"


//= Datastructure for tracked person and hand pointing directions.
// base member holds coordinates of center of head
// overall id: -1 = invalid, 0 = speculative, 1+ = tracking number
// for hands hok[i]: -1 = invalid, 0 = speculative, 1 = solidly tracked
// hands (left = 0, right = 1) are offsets relative to head (for stability)
// designations "left" and "right" are constant but may not be accurate
// absorbed old jhcBodyParts class (simpler class just for detection)

class jhcBodyData : public jhcKalVec
{
// PRIVATE MEMBER VARIABLES
private:
  // for averaging gaze estimates
  jhcMatrix gest;
  int gn;


// PUBLIC MEMBER VARIABLES
public:
  // other head information (including label)
  jhcKalVec vel; 
  char tag[80];
  void *node;                // no longer used in ALIA
  int id, vis, state; 

  // raw arm blob components
  int bnum, alt;

  // track validation and removal counts
  int hit0, miss0, hit, miss, hit2, miss2;
  double dt;

  // hand and pointing data
  jhcKalVec hoff[2], hdir[2], hvel[2];
  int hok[2], stable[2], busy[2], rpt[2];

  // table and object relations 
  double sep[2], sep0[2], tx[2], ty[2];
  int tpt[2], targ[2], sx[2], sy[2];

  // optional eye gaze information
  jhcKalVec gaze;
  int gok;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcBodyData ();
  void SetTrack (int h0 =1, int m0 =1, int h =1, int m =1, int h2 =1, int m2 =1, double secs =0.033)
    {hit0 = h0; miss0 = m0; hit = h; miss = m; hit2 = h2; miss2 = m2; dt = secs;}
  void SetMix (double pmix0 =0.2, double pmix =0.2, double dmix =0.5);

  // read only functions
  int TrackID () const {return id;}
  bool HandOK (int side =0) const {return((id > 0) && (hok[sn(side)] > 0));}
  int HandPos (jhcMatrix& full, int side =0) const;
  int RayHit (jhcMatrix& full, int side =0, double zlev =0.0) const;
  int RayHitY (jhcMatrix& full, int side =0, double yoff =0.0) const;
  int RayHitX (jhcMatrix& full, int side =0, double xoff =0.0) const;
  int RayBack (jhcMatrix& full, int side =0, double dist =7.0) const;
  int EyesHit (jhcMatrix& full, double zlev =0.0) const;

  // main functions
  int InitAll (const jhcBodyData& d, int suggest);
  int UpdateHead (const jhcBodyData& d, int suggest);
  void UpdateHand (int side, const jhcBodyData& d, int dside, double mth =1.0, double ath =1.0);
  void GazeEst (const jhcMatrix& dir);
  void UpdateGaze (int trk =1);
  void PenalizeAll ();
  void PenalizeHand (int side);
  void PenalizeGaze ();


// PRIVATE MEMBER FUNCTIONS
private:
  int sn (int i) const {return __max(0, __min(i, 1));}
  void clr_hand (int i);


};


#endif  // once




