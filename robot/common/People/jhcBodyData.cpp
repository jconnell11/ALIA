// jhcBodyData.cpp : datastructure for tracked person and pointing direction
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

#include "People/jhcBodyData.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcBodyData::jhcBodyData ()
{
  int side;

  // collecting gaze estimates
  gest.SetSize(4);
  gest.Zero();
  gn = 0;

  // basic head info
  *tag = '\0';
  node = NULL;
  vis = 1;
  id = -1;

  // component numbers
  bnum = -1;
  alt = -1;

  for (side = 0; side <= 1; side++)
  {
    // hand validity and pointing stability
    hok[side] = -1;
    stable[side] = 0;
    busy[side] = 0;
    rpt[side] = 0;

    // table separation and intersection point
    sep[side]  = -1.0;
    sep0[side] = -1.0;
    tpt[side]  = 0;
    targ[side] = -1;
    sx[side] = 0;
    sy[side] = 0;
  }

  // gaze validity
  gok = -1;

  // tracking and updating
  SetTrack();
  SetMix();
}


//= Set smoothing update fractions for positions and directions.

void jhcBodyData::SetMix (double pmix0, double pmix, double dmix)
{
  double dist = 1.0, hang = 0.175, gang = 0.087;
 
  SetKal(        pmix0, dist, dist, dist);     // head (1 in)
  hoff[0].SetKal(pmix,  dist, dist, dist);     // hands (1 in)
  hoff[1].SetKal(pmix,  dist, dist, dist);
  hdir[0].SetKal(dmix,  hang, hang, hang);     // pointing (10 degs)
  hdir[1].SetKal(dmix,  hang, hang, hang);
  gaze.SetKal(   dmix,  gang, gang, gang);     // looking (5 degs)
}


///////////////////////////////////////////////////////////////////////////
//                         Read Only Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Compute full position of one hand or the other.
// returns 1 if okay, 0 if invalid

int jhcBodyData::HandPos (jhcMatrix& full, int side) const
{
  if (!HandOK(side))
    return 0;
  full.Copy(*this);
  full.IncVec3(hoff[sn(side)]);
  return 1;
}


//= Determine intersection point of ray with horizontal surface at some level.
// returns 2 if okay, 1 if no intersection, 0 if invalid ray

int jhcBodyData::RayHit (jhcMatrix& full, int side, double zlev) const
{
  jhcMatrix hand(4);
  double dz, dist, r = 240.0;                        // default ray length (20 feet)
  int i = sn(side), hit = 1;

  // ray starts at hand (itself and offset from the body)
  if (HandPos(hand, side) <= 0)
    return 0;

  // find positive distance to surface intersection
  if ((dz = hdir[i].Z()) != 0.0)                     // not exactly horizontal
    if ((dist = (zlev - hand.Z()) / dz) > 0.0)       // forward intersection
    {
      r = dist;
      hit = 2;
    }

  // convert to end point 
  full.ScaleVec3(hdir[i], r);
  full.IncVec3(hand);
  return hit;
}


//= Determine intersection point of ray with vertical surface having a fixed y.
// returns 2 if okay, 1 if no intersection, 0 if invalid ray

int jhcBodyData::RayHitY (jhcMatrix& full, int side, double yoff) const
{
  jhcMatrix hand(4);
  double dy, dist, r = 240.0;                        // default ray length (20 feet)
  int i = sn(side), hit = 1;

  // ray starts at hand (itself and offset from the body)
  if (HandPos(hand, side) <= 0)
    return 0;

  // find positive distance to surface intersection
  if ((dy = hdir[i].Y()) != 0.0)                     // not exactly parallel
    if ((dist = (yoff - hand.Y()) / dy) > 0.0)       // forward intersection
    {
      r = dist;
      hit = 2;
    }

  // convert to end point 
  full.ScaleVec3(hdir[i], r);
  full.IncVec3(hand);
  return hit;
}


//= Determine intersection point of ray with vertical surface having a fixed x.
// returns 2 if okay, 1 if no intersection, 0 if invalid ray

int jhcBodyData::RayHitX (jhcMatrix& full, int side, double xoff) const
{
  jhcMatrix hand(4);
  double dx, dist, r = 240.0;                        // default ray length (20 feet)
  int i = sn(side), hit = 1;

  // ray starts at hand (itself and offset from the body)
  if (HandPos(hand, side) <= 0)
    return 0;

  // find positive distance to surface intersection
  if ((dx = hdir[i].X()) != 0.0)                     // not exactly parallel
    if ((dist = (xoff - hand.X()) / dx) > 0.0)       // forward intersection
    {
      r = dist;
      hit = 2;
    }

  // convert to end point 
  full.ScaleVec3(hdir[i], r);
  full.IncVec3(hand);
  return hit;
}


//= Determine rough elbow point along a pointing ray.
// returns 1 if okay, 0 if invalid ray

int jhcBodyData::RayBack (jhcMatrix& full, int side, double dist) const
{
  jhcMatrix hand(4);

  if (HandPos(hand, side) <= 0)
    return 0;
  full.ScaleVec3(hdir[sn(side)], -dist);
  full.IncVec3(hand);
  return 1;
}


//= Determine intersection point of gaze with surface (if any).
// returns 2 if okay, 1 if no intersection, 0 if invalid ray

int jhcBodyData::EyesHit (jhcMatrix& full, double zlev) const
{
  double dz, dist, r = 240.0;                        // default ray length (20 feet)
  int hit = 1;

  if (id <= 0)
    return 0;

  // find positive distance to surface intersection
  if ((dz = gaze.Z()) != 0.0)                        // not exactly horizontal
    if ((dist = (zlev - Z()) / dz) > 0.0)            // forward intersection
    {
      r = dist;
      hit = 2;
    }

  // convert to end point 
  full.ScaleVec3(gaze, r);
  full.IncVec3(*this);
  return hit;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Start a new track using information from given raw detection.
// never updates velocity since only one position sample seen
// returns incremented suggestion if used to assign new ID

int jhcBodyData::InitAll (const jhcBodyData& d, int suggest)
{
  int snext;

  // clear all position, directions, velocities and counts
  Clear();
  vel.Clear();
  clr_hand(0);
  clr_hand(1);
  *tag = '\0';
  node = NULL;
  id = 0;                         // speculative track

  // gaze data
  gest.Zero();
  gn = 0;

  // set up new track to match detection
  snext = UpdateHead(d, suggest);
  UpdateHand(0, d, 0);
  UpdateHand(1, d, 1);
  return snext;
}


//= Update tracking of head position and velocity based on a detection.
// needs time since last update call in order to calculate velocities
// returns incremented suggestion if used to assign new ID

int jhcBodyData::UpdateHead (const jhcBodyData& d, int suggest)
{
  jhcMatrix diff(4);
  int snext = suggest;

  // copy over raw arm component numbers 
  bnum = d.bnum;
  alt  = d.alt;
  vis  = 1;                  // mark as visible (since found)

  // smooth head position, assign id (and label string) if newly sure
  if (Update(d, &diff) >= hit0)
    if (id <= 0)
      id = snext++;

  // divide diff by time lag to get velocity
  vel.Update(diff, NULL, dt);                 
  return snext;
}


//= Update tracking of hand position, direction, and velocity based on a detection.
// assumes best matching of tracked hand to detected hand done externally
// will punt if head is not being tracked or detected hand is invalid

void jhcBodyData::UpdateHand (int side, const jhcBodyData& d, int dside, double mth, double ath)
{
  jhcMatrix diff(4);
  double mv, ang;
  int i = sn(side), j = sn(dside);

  // basic book-keeping
  if ((id <= 0) || (d.hok[j] <= 0))             // sanity check
    return;                                     
  hok[i] = __max(0, hok[i]);                    // at least speculative

  // mix in new hand position (wrt head)
  if (hoff[i].Update(d.hoff[j], &diff) >= hit)  // newly solid track
    hok[i] = 1;
  hvel[i].Update(diff, NULL, dt);               // initially biased to zero
  mv = diff.LenVec3();

  // mix in new pointing direction estimate (wrt hand)
  diff.Copy(hdir[i]);
  hdir[i].Update(d.hdir[j]);
  hdir[i].UnitVec3();
  ang = diff.DirDiff3(hdir[i]);

  // check for valid direction (negative means not enough extension)
  if (stable[i] >= 0)
  {
    // count number of cycles that direction does not change much
    if ((mv > mth) || (ang > ath))
      stable[i] = 0;
    else
      stable[i] += 1;
  }

  // assume not touching anything
  busy[i] = 0;
}


//= Incrementally build an average of gaze estimates from cameras.
// need to call UpdateGaze after all estimates have been entered

void jhcBodyData::GazeEst (const jhcMatrix& dir)
{
  jhcMatrix unit(4);

  unit.UnitVec3(dir);
  gest.IncVec3(unit);
  gn++;
}


//= Update gaze direction based on average of estimates.
// must call GazeEst for each camera in which face was seen
// generally done as a second step (not part of jhcParse3D output)
// will punt if head is not being tracked
// <pre>
// de-jitter gaze detection:
//
//  raw      ____|___|_|___|________|________
//
//  miss     56780123010123012345678012345678
//
//  miss<4   ____|'''|'|'''|''''|___|''''|___
//
//  hit      00001111223333444440000111110000
//
//  hit>=3   __________|''''''''|____________
//
// checks that 3 hits within 4 of each other
// </pre>

void jhcBodyData::UpdateGaze (int trk)
{
  // must be valid person
  if (id <= 0)
    return;
 
  // see if no estimates this cycle
  if (gn <= 0)
  {
    if (trk <= 0)
      gok = -1;
    else
      PenalizeGaze();
    return;
  }

  // take average of all estimates
  gest.UnitVec3();
  if (trk <= 0)
  {
    // single frame analysis
    gok = 1;
    gaze.Copy(gest);
  }
  else
  {
    // blend with history and possibly make valid
    gok = __max(0, gok);
    if (gaze.Update(gest) >= hit2)
      gok = 1;
  }

  // set up for next cycle
  gest.Zero();
  gn = 0;
}


//= Consider erasing person track since no matching detection was found.
// if expected to be found then boost consecutive miss count for head 
// default is for position and velocity estimate to remain frozen

void jhcBodyData::PenalizeAll ()
{
  // no associated raw blobs
  bnum = -1;
  alt = -1;

  // check if solidly tracked then possibly invalidate
  if ((vis <= 0) || (id < 0))
    return;
  if (Skip() >= miss0)
  {
    id = -1;
    *tag = '\0';
  }

  // no hands or gaze can be found if head is missing
  PenalizeHand(0);
  PenalizeHand(1);
  PenalizeGaze();
}


//= Decrement hand tracking count if no match found on this cycle.
// head is still valid so needs to clear hand data awaiting new detection

void jhcBodyData::PenalizeHand (int side)
{
  int i = sn(side);

  if (hok[i] >= 0)
    if (hoff[i].Skip() >= miss)
      clr_hand(i);
}


//= Clear hand tracking data in preparation for new track.
// hand cannot become valid until head is solidly tracked

void jhcBodyData::clr_hand (int i)
{
  hoff[i].Clear();
  hdir[i].Clear();
  hvel[i].Clear();
  hok[i] = -1;        // not yet even speculative         
  stable[i] = 0;      // not firm pointing
  busy[i] = 0;        // not touching anything
  rpt[i] = 0;
  sep[i]  = -1.0;     // not over table
  sep0[i] = -1.0;
}


//= Decrement gaze smoothing count if no estimate made on this cycle.

void jhcBodyData::PenalizeGaze ()
{
  if (gok >= 0)
    if (gaze.Skip() >= miss2)
    {
      gaze.Clear();
      gok = -1;
    }
}

