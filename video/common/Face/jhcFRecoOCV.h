// jhcFRecoOCV.h : face recognizer based on OpenCV LBP histogram method
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

#ifndef _JHCFRECOOCV_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFRECOOCV_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"

#include "Face/jhcFaceNorm.h"


//= Face recognizer based on OpenCV LBP histogram method.
// implements most functions found in "freco.h"

class jhcFRecoOCV : public jhcFaceNorm
{
// PRIVATE MEMBER VARIABLES
private:
  double ver;
  int nlbp, hsz;


// PUBLIC MEMBER VARIABLES
public:
  // LBP computation parameters
  jhcParam lps;
  int radius, pts, uni, xgrid, ygrid;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcFRecoOCV ();
  ~jhcFRecoOCV ();
  int BindReco (const char *fname) {return 1;}

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  void SetSizes ();

  // main functions
  const char *freco_version (char *spec) const;
  int freco_setup (const char *fname)  {return Defaults(fname);}
  int freco_metric (const char *fname) {return 1;}
  int freco_start (int level =0, const char *log_file =NULL) {SetSizes(); return 1;}
  void freco_done () {};
  void freco_cleanup () {};

  // signature functions
  int freco_size () const {return hsz;}
  int freco_vect (float *hist, const unsigned char *img) const;
  double freco_dist (const float *probe, const float *gallery) const;


// PRIVATE MEMBER FUNCTIONS
private:
  int lbp_params (const char *fname);


};


#endif  // once




