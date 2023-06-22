// jhcInteractFSM.cpp : sequential and sensor behaviors for Manus robot 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include "RWI/jhcInteractFSM.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcInteractFSM::~jhcInteractFSM ()
{
}


//= Default constructor initializes certain values.

jhcInteractFSM::jhcInteractFSM ()
{
  body = NULL;
  noisy = 2;
  Defaults();
  Reset();
}


//= Reset state for the beginning of a sequence.

void jhcInteractFSM::Reset ()
{
  phase = 1;
}


//= Test if some procedure has reached completion.
// returns 1 if trying, 2 finished, negative if failed

int jhcInteractFSM::Status (int total)
{
  if (phase < 0)
    return phase;
  if ((total == 0) || (phase > total))
    return 2;
  return 1;
}



///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for something.

int jhcInteractFSM::empty_params (const char *fname)
{
  jhcParam *ps = &eps;
  int ok;

  ps->SetTag("empty_vals", 0);
//  ps->NextSpec4( &n, 10, "Empty value");  

  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcInteractFSM::Defaults (const char *fname)
{
  int ok = 1;

  ok &= empty_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcInteractFSM::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= eps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                            Hand Behaviors                             //
///////////////////////////////////////////////////////////////////////////

//= Wait until hand is fully open.
// returns total number of steps (never fails)

int jhcInteractFSM::FullOpen (int step0)
{
  int n = 0;

  // send open event to robot
  if ((phase - step0) == n++)
  {
    jprintf(1, noisy, "  open ...\n");
    body->Grab(-1);
    phase++;
  }

  // wait until some motion
  if ((phase - step0) == n++)
  {
//jprintf("  open start: %d vs %d (%4.2f)\n", body->wcnt, body->wstop, body->Width());
//    body->Grab(-1);
    if (body->Changing())
      phase++;
  }  

  // wait until no more motion
  if ((phase - step0) == n++)
  {
//jprintf("  open finish: %d vs %d (%4.2f)\n", body->wcnt, body->wstop, body->Width());
//    body->Grab(-1);
    if (body->Stable())
      phase++;
  }
  return n; 
}


//= Wait until hand has good grasp.
// returns total number of steps (phase < 0 if failure)

int jhcInteractFSM::GoodGrip (int step0)
{
  int n = 0;

  // send close event to robot
  if ((phase - step0) == n++)
  {
    jprintf(1, noisy, "  grip ...\n");
    body->Grab(1);
    phase++;
  }

  // wait until some motion
  if ((phase - step0) == n++)
  {
//jprintf("  close start: %d vs %d (%4.2f)\n", body->wcnt, body->wstop, body->Width());
//    body->Grab(1);
    if (body->Changing())
      phase++;
  }  

  // wait until no more motion
  if ((phase - step0) == n++)
  {
    jprintf(2, noisy, "    w = %4.2f\n", body->Width()); 
//jprintf("  close finish: %d vs %d (%4.2f)\n", body->wcnt, body->wstop, body->Width());
//    body->Grab(1);
    if (body->Stable())
    {
      // totally closed hand counts as failure
      if (body->Empty())
        phase = -1;
      else
        phase++;
    }
  }
  return n; 
}


///////////////////////////////////////////////////////////////////////////
//                            Drive Behaviors                            //
///////////////////////////////////////////////////////////////////////////

//= Blindly move forward (or backward) by some amount.
// returns total number of steps (never fails)

int jhcInteractFSM::Advance (double amt, int step0)
{
  double msp = 4.0;
  int n = 0;

  // determine stopping point
  if ((phase - step0) == n++)
  {
    jprintf("  advance %3.1f ...\n", amt);
    mlim = body->Travel() + amt;
    phase++;
  }

  // wait until proper distance recorded
  if ((phase - step0) == n++)
  {
    if (((amt >= 0.0) && (body->Travel() >= mlim)) ||
        ((amt <= 0.0) && (body->Travel() <= mlim)))
      phase++;
    else
      body->MoveVel((amt >= 0) ? msp : -msp);
  }
  return n;
}


//= Get object to some distance relative to inner edge of gripper.
// can fail if object not getting closer
// returns total number of steps (phase < 0 if failure)

int jhcInteractFSM::Standoff (double stop, int step0)
{
  double dprog = 0.1, dtol = 0.3, msp = 2.0;
  double d, err;
  int init = 0, n = 0;

  // initialize progress indicator 
  if ((phase - step0) == n++)
  {
    jprintf(1, noisy, "  standoff %3.1f ...\n", stop);
    init = 1;
    dok = 0;
    phase++;
  }

  // keep moving until reasonably close to goal 
  if ((phase - step0) == n++)
  {
    d = body->Distance();
    err = fabs(d - stop);
    if (dcnt > 0)
      jprintf(2, noisy, "    d = %3.1f stuck %d\n", d, dcnt);
    else
      jprintf(2, noisy, "    d = %3.1f\n", d);

    if (err <= dtol) 
    {
      // make sure distance is stable
      dcnt = 0;
      if (++dok >= 10)
        phase++;
    }
    else
    { 
      // pick speed based on distance from goal
      dok = 0;
      body->MoveVel((d > stop) ? msp : -msp);

      // barf if insufficient progress made for a while
      if ((init > 0) || ((eprev - err) >= dprog))
      {
        eprev = err;
        dcnt = 0;
      }
      else if (++dcnt >= 20)
      {
        jprintf(1, noisy, "    FAIL - no improvement\n");
        phase = -1;
      }
    }
  }
  return n; 
}


///////////////////////////////////////////////////////////////////////////
//                              Lift Behaviors                           //
///////////////////////////////////////////////////////////////////////////

//= Set gripper to standard height above floor and wait for it to get there.
// returns total number of steps (never fails)

int jhcInteractFSM::Altitude (double val, int step0)
{
  double htol = 0.1, hnear = 0.5, hfar = 2.0, fsp = 0.5, fsp2 = 2.0;
  double dh, err, v, vsc = (fsp2 - fsp) / (hfar - hnear);
  int n = 0;

  // possibly announce entry
  if ((phase - step0) == n++)
  {
    jprintf(1, noisy, "  altitude %3.1f ...\n", val);
    phase++;
  }

  // stop when within tolerance or at limits
  if ((phase - step0) == n++)
  {
    jprintf(2, noisy, "    h = %3.1f\n", body->Height());
    dh = body->Height() - val;
    err = fabs(dh);
    if (err <= htol) 
      phase++;
    else
    {
      // pick speed base on current error
      v = vsc * (err - hnear) + fsp;
      v = __max(fsp, __min(v, fsp2));
      body->LiftVel((dh > 0.0) ? -v : v);
    }
  }
  return n; 
}


//= Rise or lower a bit relative to starting height.
// returns total number of steps (never fails)

int jhcInteractFSM::RiseBy (double amt, int step0)
{
  int n = 0;

  // set up goal height
  if ((phase - step0) == n++)
  {
    jprintf(1, noisy, "  rise by %3.1f ...\n", amt);
    hlim = body->Height() + amt;
    phase++;
  }
  n += Altitude(hlim, step0 + n);
  return n;
}


//= Raise the gripper until the distance sensor becomes longer.
// remembers height achieved, fails if no jump seen
// returns total number of steps (phase < 0 if failure)

int jhcInteractFSM::RiseTop (int step0)
{
  double d, jump = 1.0, fsp = 0.5, ddone = 0.2;
  int n = 0;

  // set up trigger distance
  if ((phase - step0) == n++)
  {
    dlast = body->Distance();
    dlim = body->Distance() + jump;
    jprintf("  rise top ...\n");
    phase++;
  }

  // wait until distance jumps then save height
  if ((phase - step0) == n++)
  {
    d = body->Distance();
    jprintf(2, noisy, "    d = %3.1f\n", d);
    if (d >= dlim)
    {
      if (fabs(d - dlast) <= ddone)  // wait to stabilize
      {
        hobj = body->Height();
        phase++;
      }
      else
        dlast = d;
    }
    else if (body->AtTop())
      phase = -1;
    else
      body->LiftVel(fsp);
  }
  return n; 
}


///////////////////////////////////////////////////////////////////////////
//                            Combined Behaviors                         //
///////////////////////////////////////////////////////////////////////////

//= Do standard gripping of the top of something and remember height.
// advances toward object if needed
// returns total number of steps (phase < 0 if failure)

int jhcInteractFSM::Acquire (int thin, int step0)
{
  double gap = 0.5, stop = ((thin > 0) ? -0.5 : 0.5);
  int n = 0;

  // possibly announce entry
  if ((phase - step0) == n++)
  {
    jprintf("  acquire ...\n");
    phase++;
  }

  // position fingers around object
  n += Standoff(stop, step0 + n);
  n += RiseTop(step0 + n);

  // grab and lift slightly
  n += GoodGrip(step0 + n);
  n += RiseBy(gap, step0 + n);
  return n; 
}


//= Put thing currently gripped on floor and back away.
// returns total number of steps (phase < 0 if failure)

int jhcInteractFSM::Deposit (int step0)
{
  double cozy = 2.5, drop = 0.1;
  int n = 0;

  // possibly announce entry
  if ((phase - step0) == n++)
  {
    jprintf("  deposit ...\n");
    phase++;
  }

  // lower to near floor, release, then back away
  n += Altitude(hobj + drop);
  n += FullOpen(step0 + n);
  n += Standoff(cozy, step0 + n);
  return n; 
}


//= Place thing currently gripped on top of tower directly ahead then back away.
// returns total number of steps (phase < 0 if failure)

int jhcInteractFSM::AddTop (int step0)
{
  double gap = 0.3, drop = 0.1, cozy = 2.5, hc = 0.3;
  int n = 0;

  // possibly announce entry
  if ((phase - step0) == n++)
  {
    jprintf("  add top ...\n");
    phase++;
  }

  // get close to tower and measure it
  n += Standoff(cozy, step0 + n);
  n += RiseTop(step0 + n);

  // position block over top of tower
  n += RiseBy(hobj + gap, step0 + n);
  n += Advance(cozy, step0 + n);
  n += RiseBy(drop - gap);

  // leave block, back off, and descend
  n += FullOpen(step0 + n);
  n += Advance(-cozy, step0 + n);
  n += Altitude(hc, step0 + n);
  return n; 
}


//= Pluck thing off top of tower directly ahead then back away.
// returns total number of steps (phase < 0 if failure)

int jhcInteractFSM::RemTop (int step0)
{
  int n = 0;

  // possibly announce entry
  if ((phase - step0) == n++)
  {
    jprintf("  add top ...\n");
    phase++;
  }

  return n; 
}

