// jhcFFind.cpp : base class for generic face finders
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017 IBM Corporation
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

#include "Interface/jhcMessage.h"

#include "Face/jhcFFind.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcFFind::~jhcFFind ()
{
  ffind_done();
  ffind_cleanup();
}


//= Default constructor initializes certain values.

jhcFFind::jhcFFind ()
{
  Defaults();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Find biggest face within search box given (score > sc).
// width of face has to be between fmin and fmax fraction of search area width
// should work with other ffind.h compliant detectors also
// returns 1 if face found, 0 if nothing

int jhcFFind::FindWithin (jhcRoi& det, const jhcImg& src, const jhcRoi& area, 
                          double fmin, double fmax, double sc)
{
  int i, n, x, y, w, h, a, win, best = 0;

  // call basic low-level function
  n = ffind_roi(src.PxlSrc(), src.XDim(), src.YDim(), src.Fields(), 
                area.RoiX(), area.RoiY(), area.RoiW(), area.RoiH(),
                ROUND(fmin * area.RoiW()), ROUND(fmax * area.RoiW()), sc);  
  if (n <= 0)
    return 0;

  // pick biggest detection (ignore score)
  for (i = 0; i < n; i++)
  {
    ffind_box(x, y, w, h, i);
    a = w * h;
    if (a > best)
    {
      win = i;
      best = a;
    }
  }

  // load ROI with winning detection
  ffind_box(x, y, w, h, i);
  det.SetRoi(x, y, w, h);
  return 1;
}


//= Find the biggest face in the whole image (score > sc).
// width of face must be between wmin and wmax pixels (wmax = 0 for whole image)
// should work with other ffind.h compliant detectors also
// returns 1 if face found, 0 if nothing

int jhcFFind::FindBest (jhcRoi& det, const jhcImg& src, int wmin, int wmax, double sc)
{
  int i, n, x, y, w, h, a, win, best = 0;

  // call basic low-level function
  n = ffind_run(src.PxlSrc(), src.XDim(), src.YDim(), src.Fields(), wmin, wmax, sc);  
  if (n <= 0)
    return 0;

  // pick biggest detection (ignore score)
  for (i = 0; i < n; i++)
  {
    ffind_box(x, y, w, h, i);
    a = w * h;
    if (a > best)
    {
      win = i;
      best = a;
    }
  }

  // load ROI with winning detection
  ffind_box(x, y, w, h, i);
  det.SetRoi(x, y, w, h);
  return 1;
}


//= Find all faces in the whole image with score > sc.
// width of face must be between wmin and wmax pixels (wmax = 0 for whole image)
// should work with other ffind.h compliant detectors also
// returns number of faces found, 0 if nothing

int jhcFFind::FindAll (const jhcImg& src, int wmin, int wmax, double sc)
{
  int n;

  n = ffind_run(src.PxlSrc(), src.XDim(), src.YDim(), src.Fields(), wmin, wmax, sc);  
  if (n <= 0)
    return 0;
  return n;
}


//= Get the image bounding box for a particular face detection.
// should work with other ffind.h compliant detectors also
// returns score if successful, negative if bad index

double jhcFFind::FaceDet (jhcRoi& det, int i) const
{
  int x, y, w, h;
  double sc;

  if (i >= ffind_cnt())
    return -1.0;
  sc = ffind_box(x, y, w, h, i);
  det.SetRoi(x, y, w, h);
  return sc;
}
