// jhcKin2VSrc.cpp : gets color and depth images from Kinect v2 sensor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2019 IBM Corporation
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

#include <string.h>

#include "Video/jhc_kin2.h"
#include "Video/jhcKin2VSrc.h"


///////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                       //
///////////////////////////////////////////////////////////////////////////

#ifdef JHC_GVID
  #include "Video/jhcVidReg.h"
  JREG_CAM(jhcKin2VSrc, "kin2 kin2h kin2r kin2hr");   
#endif


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcKin2VSrc::~jhcKin2VSrc ()
{
  kin2_close(unit);
}


//= Default constructor initializes certain values.

jhcKin2VSrc::jhcKin2VSrc (const char *filename, int index)
{
  // save class name and image source
  strcpy_s(kind, "jhcKin2VSrc");
  ParseName(filename);
  unit = atoi(BaseName) % 10;
  big = (((IsFlavor("kin2h") > 0) || (IsFlavor("kin2hr") > 0)) ? 1 : 0);
  rot = (((IsFlavor("kin2r") > 0) || (IsFlavor("kin2hr") > 0)) ? 1 : 0);

  // set up sizes and overall frame rate for depth
  // usable about 65 x 50 degs 
  w2 = 960;
  h2 = 540;
  d2 = 2;
  freq2 = 30.0;
  flen2 = 540.685;  // was 535
  dsc2  = 1.0;
  aspect2 = 1.0;

  // set up sizes and overall frame rate for color
  // 83 degs horizontal, 53 degs vertical
  w = w2;
  h = h2;
  d = 3;
  freq = freq2; 
  flen = flen2;     
  dsc  = 1.0;
  aspect = 1.0;
  if (big > 0)
  {
    // hi-res size 
    w *= 2;
    h *= 2;
    flen *= 2.0;
  }

  // invoke main DLL function
  ok = kin2_open(unit);
}


///////////////////////////////////////////////////////////////////////////
//                              Core Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get just the color or depth image from the sensor.
// depth is always 960 x 540 x 2, color is 1920 x 1080 x 3 for *.kin2h source
// ignores "block" flag

int jhcKin2VSrc::iGet (jhcImg& dest, int *advance, int src, int block)
{
  int ans;

  // just depth (always available)
  if (src > 0)
    return kin2_rcv(dest.PxlDest(), NULL, unit, 0, rot);             

  // just color (only sometimes available)
  while ((ans = kin2_rcv(NULL, dest.PxlDest(), unit, big, rot)) < 2)
    if (ans <= 0)
      return ans;
  return 1;
}


//= Get the color image (dest) and the depth image (dest2) from the sensor.
// returns 4x depth (z offset, not ray length) with depth and color pixels aligned
// raw depth = 440-10000mm (17.3"-32.8ft) ==> values 1760-40000, invalid = 65535
// libfreenect2 limits depth max to 4.5m (14.7') ==> max value 18000
// jhc_kin2 version 1.10+ extends to 8.0m (26.2') ==> max 32000 
// use jhcLUT::Night8 for convenient viewing
// returns 1 if just depth, 2 if depth and new color, 0 or negative for problem

int jhcKin2VSrc::iDual (jhcImg& dest, jhcImg& dest2)
{
  return kin2_rcv(dest2.PxlDest(), dest.PxlDest(), unit, big, rot);
}


