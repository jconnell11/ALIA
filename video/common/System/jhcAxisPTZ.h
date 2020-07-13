// jhcAxisPTZ.h : controls Axis pan, tilt, zoom network cameras 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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

#ifndef _jhcAxisPTZ_
/* CPPDOC_BEGIN_EXCLUDE */
#define _jhcAxisPTZ_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"


//= Controls Axis pan, tilt, zoom network cameras.
// operates by doing a system call to the "curl" executable.
// needs cURL system from http://curl.haxx.se/download.html

class jhcAxisPTZ
{
// PUBLIC MEMBER VARIABLES
public:
  char name[80], ip[80], upwd[80];

  // starting pose
  jhcParam hps;
  int z0, brite, back, iris, focus;
  double p0, t0;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcAxisPTZ ();
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // home position and mode
  int Reset () const;
  int SetMode (int brite =60, int back =1, int iris =1, int focus =1) const;
 
  // movement functions
  int SetPTZ (double pan, double tilt, int zoom =1) const;
  int Shift (double dp, double dt) const;
  int Zoom (double sc) const;
  int Center (int x, int y, double sc =1.0) const;

  // status functions
  int GetPTZ (double& pan, double& tilt, int& zoom) const;


// PRIVATE MEMBER FUNCTIONS
private:
  int home_params (const char *fname);

};


#endif  // once




