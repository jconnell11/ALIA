// jhcKin2VSrc.h : gets color and depth images from Kinect v2 sensor
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

#ifndef _JHCKIN2VSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCKIN2VSRC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Video/jhcVideoSrc.h"


//= Gets color and depth images from Kinect v2 sensor.
// requires DLLs: jhc_kin2, freenect2, lib-usb1.0, turbojpeg, and glfw3
// NOTE: jhc_kin2.dll compiled for VS2008, others for VS2015

class jhcKin2VSrc : public jhcVideoSrc
{
// PRIVATE MEMBER VARIABLES
private:
  int unit, big, rot;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcKin2VSrc ();
  jhcKin2VSrc (const char *filename, int index =0);


// PRIVATE MEMBER FUNCTIONS
private:
  int iGet (jhcImg& dest, int *advance, int src, int block);
  int iDual (jhcImg& dest, jhcImg& dest2);

};


#endif  // once




