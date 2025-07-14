// jhcTofVSrc.h : interface to MaixSense A010 Time-of-Flight sensor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Etaoin Systems
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

#pragma once

#include "jhcGlobal.h"
#include "jhc_pthread.h"

#include "Data/jhcImg.h"
#include "Interface/jhcSerial.h"

#include "Video/jhcVideoSrc.h"


//= Interface to MaixSense A010 Time-of-Flight sensor.

class jhcTofVSrc : public jhcVideoSrc
{
// PRIVATE MEMBER VARIABLES
private:
  // camera connection
  jhcSerial s;

  // background receiver
  pthread_t hoover;
  pthread_mutex_t data;

  // image data
  UC8 raw0[10018], raw1[10018];
  UC8 *raw;
  int run, fresh;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcTofVSrc ();
  jhcTofVSrc (const char *spec);
 

// PRIVATE MEMBER FUNCTIONS
private:
  // core functionality
  int iGet (jhcImg& dest, int *advance, int src, int block);

  // background thread functions
  static pthread_ret absorb (void *tof);
  int sync ();
  int fill_buf ();

};
