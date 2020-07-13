// jhcKinVSrc.h : gets color and depth images from Microsoft Kinect sensor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
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

#ifndef _JHCKINVSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCKINVSRC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Video/jhcVideoSrc.h"


//= Gets color and depth images from Microsoft Kinect sensor.
// in GUI application do "File : Explict/URL" and type "0.kin"
// color focal length = 525 pixels (62.7 degs horizontal)
// requires OpenNI2.dll at run-time (version 2.2.0.33)
// needs Microsoft Kinect SDK to find sensor (version 1.8)
// NOTE: may have to disable USB 3.0 mode in BIOS for Windows 7

class jhcKinVSrc : public jhcVideoSrc
{
// PRIVATE MEMBER VARIABLES
private:
  void *dev0, *depth0, *color0;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcKinVSrc (const char *filename, int index =0);
  ~jhcKinVSrc ();


// PRIVATE MEMBER FUNCTIONS
private:
  int iGet (jhcImg& dest, int *advance, int src, int block);
  int iDual (jhcImg& dest, jhcImg& dest2);

  int fill_color (jhcImg& dest);
  int fill_depth (jhcImg& dest);

};


#endif  // once




