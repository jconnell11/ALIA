// jhcKalVec.cpp : performs smooth updating of 3D point or vector
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2017 IBM Corporation
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

#include "Geometry/jhcKalVec.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcKalVec::jhcKalVec ()
{
  SetSize(4);
  Zero();
  SetKal(0.1, 1.0, 1.0, 1.0);
  Clear();
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Update smoothed value to bring it closer to observation.
//
//   measurement:  M  = P + Vm          where Vm = variance in measurement
//       process:  P' = d * P + c       where c is expected jumpiness 
//                                        and d is a time decay constant 
// Arguments for each of XYZ axes:
//   pos   = smoothed prediction (item at start)
//   var   = variance of prediction (noise^2 at start)
//   raw   = new observation of raw value 
//   noise = expected noise in raw value
//   mix   = temporal mixing for measurement variance
//
// loads diff vector with change in position (related to velocity)
// can optionally scale raw vector by 1 / time-lag (e.g. for velocity)
// returns consecutive hit count

int jhcKalVec::Update (const jhcMatrix& raw, jhcMatrix *diff, double dt)
{
  double inc[3];
  double d, vm, k, norm = dt * Last();
  int i;

  if (!raw.Vector(3) || ((diff != NULL) && !diff->Vector(3)))
    return cnt;

  // handle case where values are uninitialized
  if (cnt == 0)
  {
    Copy(raw);
    for (i = 0; i < 3; i++)
      var[i] = noise[i] * noise[i]; 
    if (diff != NULL)
      diff->Zero();
    cnt = 1;
    return cnt;
  }

  // update consecutive hit count
  if (cnt < 0)
    cnt = 0;
  cnt++;

  // figure amount to blend in new observation
  for (i = 0; i < 3; i++)
  {
    // estimate observation variance
    d = raw.VRef(i);
    if (dt > 0.0)
      d /= norm; 
    d -= VRef(i);
    vm = mix * d * d + (1.0 - mix) * var[i];

    // figure out Kalman gain
    k = vm / (vm + noise[i] * noise[i]);
    var[i] = (1.0 - k) * vm;
    inc[i] = k * d;
  }

  // update smoothed coordinates
  IncVec3(inc[0], inc[1], inc[2]);
  if (diff != NULL)
    diff->SetVec3(inc[0], inc[1], inc[2]);
  return cnt;
}


//= Function to call when no update is made on some cycle.
// returns consecutive miss count (never negative)

int jhcKalVec::Skip ()
{
  if (cnt > 0)
    cnt = 0;
  cnt--;
  return -cnt;
}

