// jhcBodyTrack.cpp : datastructure for tracked person and pointing direction
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

#include "People/jhcBodyTrack.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcBodyTrack::jhcBodyTrack ()
{
  id = -1;
  vis = 1;
  SetTrack();
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Start a new track using information from given raw detection.
// never updates velocity since only one position sample seen
// returns incremented suggestion if used to assign new ID

int jhcBodyTrack::InitAll (const jhcBodyParts& d, int suggest)
{
  int snext = suggest;

  // clear counts for all components 
  Clear();
  hv.Clear();
  clr_lf();
  clr_rt();

  // set up person as probationary status (not confirmed yet so id = 0)
  id = 0;
  if (Update(d) >= hit0)
    id = snext++;

  // add left arm as offset from head (not confirmed yet so ltrk = 0)
  if (d.lok > 0)
  {
    if (lf.Update(d.left) >= hit)
      ltrk = 1;
    sin.Update(d.lray);
  }

  // add right arm as offset from head (not confirmed yet so rtrk = 0)
  if (d.rok > 0)
  {
    if (rt.Update(d.right) >= hit)
      rtrk = 1;
    dex.Update(d.rray);
  }
  return snext;
}


//= Set up to start tracking left hand and ray.

void jhcBodyTrack::clr_lf ()
{
  sin.Clear();
  lf.Clear();
  lv.Clear();
  ltrk = 0;
}


//= Set up to start tracking right hand and ray.

void jhcBodyTrack::clr_rt ()
{
  dex.Clear();
  rt.Clear();
  rv.Clear();
  rtrk = 0;
}


//= Update tracking of all components based on matched detection.
// needs time since last update call in order to calculate velocities
// returns incremented suggestion if used to assign new ID

int jhcBodyTrack::UpdateAll (const jhcBodyParts& d, int suggest)
{
  jhcMatrix diff(4);
  int n, snext = suggest;

  // smooth head position, assign id if newly sure, and adjust velocity
  if ((n = Update(d, &diff)) >= hit0)
    if (id <= 0)
      id = snext++;
  hv.Update(diff, NULL, dt);                 // divide diff by time lag   
    
  // if left hand found then
  if (d.lok > 0)
  {
    // update positions and adjust velocity (if 2 consecutive)
    if (ltrk < 0)
      clr_lf();
    sin.Update(d.lray);
    if ((n = lf.Update(d.left, &diff)) >= hit)
      ltrk = 1;
    lv.Update(diff, NULL, dt);               // first gives bias to zero
  }
  else if (lf.Skip() >= miss)
    ltrk = -1;

  // see if right hand found
  if (d.rok > 0)
  {
    // update positions and adjust velocity (if 2 consecutive)
    if (rtrk < 0)
      clr_rt();
    dex.Update(d.rray);
    if ((n = rt.Update(d.right, &diff)) >= hit)
      rtrk = 1;
    rv.Update(diff, NULL, dt);               // first gives bias to zero
  }
  else if (rt.Skip() >= miss)
    rtrk = -1;
  return snext;
}


//= Consider erasing track since no matching detection was found.
// returns current id of track (negative if no longer valid)

int jhcBodyTrack::PenalizeAll ()
{
  // if expected to be found then boost consecutive miss count for head 
  //   default is for position and velocity estimate to remain frozen
  if (vis > 0)
    if (Skip() >= miss0)
      id = -1;
 
  // always boost consecutive miss count for hands 
  if (lf.Skip() >= miss)
    ltrk = -1;
  if (rt.Skip() >= miss)
    rtrk = -1;
  return id;
}
