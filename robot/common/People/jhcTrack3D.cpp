// jhcTrack3D.cpp : find and track heads, hands, and pointing directions
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

#include <math.h>

#include "Interface/jhcMessage.h"

#include "People/jhcTrack3D.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcTrack3D::jhcTrack3D ()
{
  dude = (jhcBodyData *) new jhcBodyData[tmax];
  Defaults();
  Reset();
}


//= Default destructor does necessary cleanup.

jhcTrack3D::~jhcTrack3D ()
{
  delete [] dude;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for tracking overall people. 

int jhcTrack3D::htrk_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("t3d_htrk", 0);
  ps->NextSpecF( &dmax0, 18.0, "Max match distance (in)");     // was 24 then 6 then 12
  ps->NextSpecF( &pmix0,  0.9, "Position update rate");        // was 0.2
  ps->NextSpec4( &hit0,   5,   "Hits to add person");          // was 20
  ps->NextSpec4( &miss0, 15,   "Misses to remove person");     // was 30
  ps->NextSpec4( &anchor, 1,   "No penalty if person blob");     
  ps->Skip();

  ps->NextSpec4( &hit2,   5, "Hits to add gaze");  
  ps->NextSpec4( &miss2,  5, "Misses to remove gaze");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for tracking the hands of a person.

int jhcTrack3D::atrk_params (const char *fname)
{
  jhcParam *ps = &tps2;
  int ok;

  ps->SetTag("t3d_atrk", 0);
  ps->NextSpecF( &dmax, 12.0, "Max match distance (in)");     // was 24 then 18 then 6
  ps->NextSpecF( &awt,  10.0, "Angle mismatch wt (deg/in)");
  ps->NextSpecF( &pmix,  0.9, "Position update rate");        // was 0.2
  ps->NextSpecF( &dmix,  0.9, "Direction update rate");       // was 0.5 then 0.7
  ps->NextSpec4( &hit,   5,   "Hits to add hand");            // was 2
  ps->NextSpec4( &miss,  5,   "Misses to remove hand");       // was 2

  ps->NextSpecF( &mth,   2.0, "Stable hand movement (in)");
  ps->NextSpecF( &ath,   2.0, "Stable angle change (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcTrack3D::Defaults (const char *fname)
{
  int ok = 1;

  ok &= jhcParse3D::Defaults(fname); 
  ok &= htrk_params(fname);
  ok &= atrk_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcTrack3D::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= jhcParse3D::SaveVals(fname);
  ok &= tps.SaveVals(fname);
  ok &= tps2.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// needs typically call interval to determine velocities

void jhcTrack3D::Reset (double dt)
{
  int i;

  // broadcast tracking parameters
  for (i = 0; i < tmax; i++)
  {
    dude[i].SetTrack(hit0, miss0, hit, miss, hit2, miss2, dt);  
    dude[i].SetMix(pmix0, pmix, dmix);
  } 

  // no tracks yet
  last_id = 0;
  nt = 0;
}


//= Find new people given overhead map and combine them with previous tracks.
// needs time since last tracking call in order to calculate velocities
// assumes map was made with jhcSurface3D::FloorFwd and pose input via SetView
// returns index limit for collection of heads (not number tracked)

int jhcTrack3D::TrackPeople (const jhcImg& map)
{
  int i, j, m, id;

  // find new people and get pairwise distances
  m = FindPeople(map);
//  world_coords();
  dist_matrix(dude, nt, raw, m);

  // match sure tracked items first then tentative ones
  while (best_match(i, j, 1) > 0)
  {
    last_id = dude[i].UpdateHead(raw[j], last_id);
    match_hands(dude[i], raw[j]);
  }
  while (best_match(i, j, 0) > 0)
  {
    last_id = dude[i].UpdateHead(raw[j], last_id);
    match_hands(dude[i], raw[j]);
  }

  // penalize unmatched tracks (unless solid on top of person blob)
  for (i = 0; i < nt; i++)
    if ((fwd[i] < 0) && ((id = dude[i].TrackID()) >= 0))
      if ((anchor <= 0) || (id == 0) || !PersonBlob(dude[i]))
        dude[i].PenalizeAll();

  // scavenge any dead tracks at end of array
  while (nt > 0)
    if (dude[nt - 1].TrackID() >= 0)
      break;
    else
      nt--;

  // add new tracks for remaining detections
  for (j = 0; j < m; j++)
    if (back[j] < 0)
    {
      if ((i = first_open()) < 0)
        break;
      last_id = dude[i].InitAll(raw[j], last_id);
    }
  return nt;
}


//= Find the distances of all new detections from all old tracks.
// leaves results in internal array "mate", clear "fwd" and "back"

void jhcTrack3D::dist_matrix (const jhcBodyData *t, int n, const jhcBodyData *d, int m)
{
  jhcMatrix diff(4);
  int i, j;

  // clear usage indicators
  for (i = 0; i < n; i++)
    fwd[i] = -1;
  for (j = 0; j < m; j++)
    back[j] = -1;

  // get pairwise distances
  for (i = 0; i < n; i++)
    if (t[i].TrackID() >= 0)
      for (j = 0; j < m; j++)
      {
        diff.DiffVec3(t[i], d[j]);
        mate[i][j] = diff.Len2Vec3();
      }
}


//= Find raw to track pairing smallest remaining distance.
// track must have id >= threshold (0 = unsure, 1 = sure)
// returns 1 if good pair indices bound, 0 if none

int jhcTrack3D::best_match (int& iwin, int& jwin, int th) 
{
  double best, d2 = dmax0 * dmax0;
  int i, j, m = NumRaw(), any = 0;

  // look for best overall match
  for (i = 0; i < nt; i++)
    if ((dude[i].TrackID() >= th) && (fwd[i] < 0))
      for (j = 0; j < m; j++)
        if ((back[j] < 0) && ((any <= 0) || (mate[i][j] < best)))
        {
          best = mate[i][j];
          iwin = i;
          jwin = j;
          any = 1;
        }

  // make sure some reasonable pairing found
  if ((any <= 0) || (best > d2))
    return 0;

  // invalidate pairing in next search
  back[jwin] = iwin;
  fwd[iwin] = jwin;
  return 1;
}


//= Find first unused entry in track array.
// updates max index of tracking array if needed

int jhcTrack3D::first_open () 
{
  int i;

  for (i = 0; i < nt; i++)
    if (dude[i].TrackID() < 0)
      return i;
  if (nt >= tmax)
    return -1;
  return nt++;             // value before increment
}


//= Determines the current number of valid heads being tracked.

int jhcTrack3D::CntTracked () const
{
  int i, n = 0;

  for (i = 0; i < nt; i++)
    if (dude[i].TrackID() > 0)
      n++;
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                             Hand Assignment                           //
///////////////////////////////////////////////////////////////////////////

//= Match detected hands to current hand tracks.

void jhcTrack3D::match_hands (jhcBodyData& trk, const jhcBodyData& det)
{
  int i, j;

  // find distance from all detections to all tracks
  if (trk.id <= 0)
    return;
  hand_dists(trk, det);

  // grab best matches for solid tracks, then for speculative tracks
  while (best_hand(i, j, trk, det, 1) > 0)
    trk.UpdateHand(i, det, j, mth, ath);
  while (best_hand(i, j, trk, det, 0) > 0)
    trk.UpdateHand(i, det, j, mth, ath);

  // penalize unmatched hands
  for (i = 0; i <= 1; i++)
    if (fh[i] < 0) 
      trk.PenalizeHand(i);

  // add new tracks for remaining detections
  for (j = 0; j <= 1; j++)
    if ((bh[j] < 0) && (det.hok[j] > 0))
    {
      // prefer to preserve hand designation
      if (trk.hok[j] < 0)
        trk.UpdateHand(j, det, j, mth, ath);
      else if (trk.hok[1 - j] < 0)
        trk.UpdateHand(1 - j, det, j, mth, ath);
    } 
}


//= Compute how well each detected hand matches each tracked hand.

void jhcTrack3D::hand_dists (const jhcBodyData& trk, const jhcBodyData& det)
{
  double err, f = 1.0 / awt;
  int i, j;

  // clear usage indicators
  fh[0] = -1;
  fh[1] = -1;
  bh[0] = -1;
  bh[1] = -1;

  // get pairwise distances for hands 
  for (i = 0; i <= 1; i++)
    if (trk.hok[i] >= 0)
      for (j = 0; j <= 1; j++)
        if (det.hok[j] > 0)
        {
          // position change of hand + position change of wrist
          err = (trk.hoff[i]).PosDiff3(det.hoff[j]);
          err += f * (trk.hdir[i]).DirUnit3(det.hdir[j]);
          dh[i][j] = err;
        }
}


//= Find smallest remaining pairing distance from hand track to detection.
// track must have status >= threshold (0 = unsure, 1 = sure)
// returns 1 if good pair indices bound, 0 if none

int jhcTrack3D::best_hand (int& iwin, int& jwin, const jhcBodyData& trk, const jhcBodyData& det, int th) 
{
  double best;
  int i, j, any = 0;

  // look for best overall match
  for (i = 0; i <= 1; i++)
    if ((trk.hok[i] >= th) && (fh[i] < 0))
      for (j = 0; j <= 1; j++)
        if ((bh[j] < 0) && (det.hok[j] > 0) && 
            ((any <= 0) || (dh[i][j] < best)))
        {
          // record indices of pair as well as distance
          best = dh[i][j];
          iwin = i;
          jwin = j;
          any = 1;
        }

  // make sure some reasonable pairing found
  if ((any <= 0) || (best > dmax))
    return 0;

  // invalidate pairing in next search
  bh[jwin] = iwin;
  fh[iwin] = jwin;
  return 1;
}
