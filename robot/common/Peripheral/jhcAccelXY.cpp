// jhcAccelXY.cpp : interprets body accelometer data from onboard PIC
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 IBM Corporation
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

#include "jhcAccelXY.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcAccelXY::~jhcAccelXY ()
{
}


//= Default constructor initializes certain values.

jhcAccelXY::jhcAccelXY ()
{
  // no serial port for PIC yet
  aok = -1;
  dyn = NULL;

  // set up processing and interpretation parametets
  LoadCfg();
  Defaults();
  clr_vals();
}


//= Clear all cached results.
// returns 0 for convenience

int jhcAccelXY::clr_vals ()
{
  tilt = 0.0;
  roll = 0.0;
  tip  = 0.0;
  mag  = 0.0;
  ang  = 0.0;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for interpreting accelerometer data.

int jhcAccelXY::acc_params (const char *fname)
{
  jhcParam *ps = &aps;
  int ok;

  ps->SetTag("acc_cal", 0);
  ps->NextSpec4( &x0,  0,    "Flat surface X value");  
  ps->NextSpec4( &y0,  0,    "Flat surface Y value");  
  ps->NextSpecF( &mgx, 4.03, "X scale factor (mG/bit)");
  ps->NextSpecF( &mgy, 4.03, "Y scale factor (mG/bit)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Associate arm with some (possibly shared) Dyamixel interface.

void jhcAccelXY::Bind (jhcDynamixel& ctrl)
{
  dyn = &ctrl;
  aok = 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcAccelXY::Defaults (const char *fname)
{
  int ok = 1;

  return ok;
}


//= Read just body specific values from a file.

int jhcAccelXY::LoadCfg (const char *fname)
{
  return acc_params(fname);
}


//= Write current processing variable values to a file.

int jhcAccelXY::SaveVals (const char *fname) const
{
  int ok = 1;

  return ok;
}


//= Write current body specific values to a file.

int jhcAccelXY::SaveCfg (const char *fname) const
{
  return aps.SaveVals(fname);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Interpret accelerometer data (possibly reading it first).
// set chk > 0 if MegaUpdate not called, caches results for later reading
// tip is overall offset of base from vertical, magnitude is in mG's 
// ang is with respect to direction of travel: 0 = straight, +90 = left
// returns 1 if successful, 0 for error

int jhcAccelXY::Update (int chk)
{
  double gx, gy, sx, sy;
  int xpk, ypk, xav4, yav4;

  // get values from recent Dynamixel mega command
  if ((dyn == NULL) || (aok <= 0))
    return clr_vals();
  if (chk > 0)
    if (dyn->MegaUpdate(1, 0, 1) <= 0)
      return clr_vals();
  if (dyn->RawAccel(xpk, ypk, xav4, yav4) <= 0)
    return clr_vals();

  // possibly set zero level if needed
  if (x0 <= 0)
    x0 = xav4;
  if (y0 <= 0)
    y0 = yav4;

  // determine angles from XY averages
  gx = mgx * (xav4 - x0);
  gy = mgy * (yav4 - y0);
  tilt = R2D * asin(-0.001 * gx);
  roll = R2D * asin(-0.001 * gy);
  tip  = R2D * asin(sqrt(gx * gx + gy * gy));
jprintf("ACC avgs:  %d, %d [%d, %d] -> tilt = %3.1f, roll = %3.1f degs\n", xav4, yav4, x0, y0, tilt, roll);

  // determine angle and magnitude from XY shock peaks
  sx = mgx * (4.0 * (xpk - 96.0) - xav4); 
  sy = mgy * (4.0 * (ypk - 96.0) - yav4); 
  mag = sqrt(sx * sx + sy * sy);
  ang = R2D * atan2(sy, sx);
jprintf("ACC shock: %d, %d -> x = %1.0f, y = %1.0f mG -> %1.0f @ %3.1f degs\n", xpk, ypk, sx, sy, mag, ang);
  return 1;
}