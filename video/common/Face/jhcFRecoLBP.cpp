// jhcFRecoLBP.cpp : face recognition based on uniform local binary patterns
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

#include <stdio.h>
#include <math.h>

#include "Interface/jhcMessage.h"

#include "jhcFRecoLBP.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcFRecoLBP::jhcFRecoLBP ()
{
  // current code version
  ver = 1.00;

  // set processing parameters
  Defaults();
}


//= Default destructor does necessary cleanup.

jhcFRecoLBP::~jhcFRecoLBP ()
{
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcFRecoLBP::Defaults (const char *fname)
{
  int ok = 1;

  ok &= lbp_params(fname);
  ok &= jhcFaceNorm::Defaults(fname);
  SetSizes();
  return ok;
}


//= Write current processing variable values to a file.

int jhcFRecoLBP::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= lps.SaveVals(fname);
  ok &= jhcFaceNorm::SaveVals(fname);
  return ok;
}


//= Parameters used for something.

int jhcFRecoLBP::lbp_params (const char *fname)
{
  jhcParam *ps = &lps;
  int ok;

  ps->SetTag("face_lbp", 0);
  ps->NextSpec4( &radius,  1, "LBP radius (pels)");  
  ps->NextSpec4( &pts,     8, "LBP sampling pts");  
  ps->NextSpec4( &uni,     0, "Drop uniform patterns");  
  ps->Skip();
  ps->NextSpec4( &xgrid,   5, "Face X grid divisions");  
  ps->NextSpec4( &ygrid,   9, "Face Y grid divisions");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Determine how many LBP patterns and number of histogram bins.
// this should be called any time basic parameters change

void jhcFRecoLBP::SetSizes ()
{
  nlbp = (int) pow(2.0, pts);
  hsz = xgrid * ygrid * nlbp;
  jhcFaceNorm::SetSizes();
}


///////////////////////////////////////////////////////////////////////////
//                          Signature Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of processing code.
// returns pointer to input string for convenience

const char *jhcFRecoLBP::freco_version (char *spec, int ssz) const
{
  sprintf_s(spec, ssz, "LBP face recognition %4.2f", ver);
  return spec;
}


//= Computes a signature vector given a cropped grayscale face image.
// image is 8 bit gray scanned left-to-right, bottom up
// returns negative for error, else vector size

int jhcFRecoLBP::freco_vect (float *hist, const unsigned char *img) const
{

  return hsz;
}


//= Computes a distance between two signature vectors (small is good).
// assumes signatures are both of correct length

double jhcFRecoLBP::freco_dist (const float *probe, const float *gallery) const
{

  return 0.0;
}