// jhcKinectIO.h : access to Kinect LED, motor, and accelerometer
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

#ifndef _JHCKINECTIO_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCKINECTIO_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Provides access to Kinect LED, motor, and accelerometer.
// uses call from OpenNI library and thus requires OpenNI.dll at run-time
// Note: works with older 1414 model, but NOT newer 1473 version

class jhcKinectIO
{
// PRIVATE MEMBER VARIABLES
private:
  void *dev;
  int kok;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcKinectIO ();
  ~jhcKinectIO ();
  int Open ();
  int Close ();
  int Status () const {return kok;}
 
  // main functions
  double Tilt ();
  int SetTilt (double degs);
  int SetLED (int col, int blink =0);


// PRIVATE MEMBER FUNCTIONS
private:


};


#endif  // once




