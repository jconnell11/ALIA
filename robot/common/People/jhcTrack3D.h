// jhcTrack3D.h : find and track heads, hands, and pointing directions
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

#ifndef _JHCTRACK3D_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTRACK3D_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "People/jhcBodyData.h"
#include "People/jhcParse3D.h"


//= Find and track heads, hands, and pointing directions.
// Does assignment of newly detected heads to old tracks then,
// for a head, does assignment of detected hands to tracked hands.
// Same general algorithm for both parts:
//   find all track (verified or speculative) to detection distances
//   sequentially bind smallest verified track to detection pairs
//   sequentially bind smallest speculative track to detection pairs
//   penalize any tracks without binds on this cycle
//   start new tracks for any unbound detections

class jhcTrack3D : public jhcParse3D
{
// PRIVATE MEMBER VARIABLES
private:
  static const int tmax = 200;   /** Maximum number of tracked people. */

  // overall status
  int last_id, nt;

  // head matching
  double mate[tmax][rmax];
  int back[rmax];

  // hand matching
  double dh[2][2];
  int fh[2], bh[2];


// PUBLIC MEMBER VARIABLES
public:
  // actual tracked people
  jhcBodyData *dude;                   // tmax entries
  int fwd[tmax];

  // parameters for tracking overall people
  jhcParam tps;
  int hit0, miss0, hit2, miss2, anchor;
  double dmax0, pmix0;

  // parameters for tracking hands of a person
  jhcParam tps2;
  int hit, miss;
  double dmax, awt, pmix, dmix, mth, ath;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTrack3D ();
  ~jhcTrack3D ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  void Reset (double dt =0.033);
  int TrackPeople (const jhcImg& map);
  int CntTracked () const;
  int NumPotential () const {return nt;}

  // debugging graphics
  int TrackedMark (jhcImg& dest, int invert =0, double sz =8.0, int style =2)         
    {return MarkHeads(dest, dude, nt, invert, sz, style);}
  int TrackedHeads (jhcImg& dest, int invert =0, double sz =8.0) const   
    {return ShowHeads(dest, dude, nt, invert, sz);}
  int TrackedHands (jhcImg& dest, int invert =0) const   
    {return ShowHands(dest, dude, nt, invert);}
  int TrackedRays (jhcImg& dest, int invert =0, double zlev =0.0, int pt =3) const 
    {return ShowRays(dest, dude, nt, invert, zlev, pt);}
  int TrackedRaysY (jhcImg& dest, int invert =0, double yoff =0.0, int pt =3) const 
    {return ShowRaysY(dest, dude, nt, invert, yoff, pt);}
  int TrackedRaysX (jhcImg& dest, int invert =0, double xoff =0.0, int pt =3) const 
    {return ShowRaysX(dest, dude, nt, invert, xoff, pt);}


// PRIVATE MEMBER FUNCTIONS
private:
  int htrk_params (const char *fname);
  int atrk_params (const char *fname);

  // main functions
  void dist_matrix (const jhcBodyData *t, int n, const jhcBodyData *d, int m);
  int best_match (int& iwin, int& jwin, int th);
  int first_open ();

  // hand matching
  void match_hands (jhcBodyData& trk, const jhcBodyData& det);
  void hand_dists (const jhcBodyData& trk, const jhcBodyData& det);
  int best_hand (int& iwin, int& jwin, const jhcBodyData& trk, const jhcBodyData& det, int th);


};


#endif  // once




