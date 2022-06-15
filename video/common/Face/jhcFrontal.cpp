// jhcFrontal.cpp : finds faces in head regions and checks if frontal
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2021-2022 Etaoin Systems
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

#include "Interface/jhcMessage.h"

#include "Face/jhcFrontal.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcFrontal::~jhcFrontal ()
{
}


//= Default constructor initializes certain values.

jhcFrontal::jhcFrontal ()
{
  SetFront(0.3, 0.5, 0.5, 0.2, 0.1);
  Defaults();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for determining if faces are directly aimed at camera.

int jhcFrontal::front_params (const char *fname)
{
  jhcParam *ps = &dps;
  int ok;

  ps->SetTag("face_front", 0);
  ps->NextSpecF( &fsz,  "Min face wrt search");    // was 0.42
  ps->NextSpecF( &xoff, "X center wrt search");  
  ps->NextSpecF( &yoff, "Y center wrt search");
  ps->NextSpecF( &xsh,  "Max X shift wrt face");  
  ps->NextSpecF( &ysh,  "Max Y shift wrt face");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcFrontal::Defaults (const char *fname)
{
  int ok = 1;

  ok &= ff.Defaults(fname);
  ok &= front_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcFrontal::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= ff.SaveVals(fname);
  ok &= dps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcFrontal::Reset ()
{
  int p, c;

  // initialize face finder
  ff.Reset();

  // clear cached data
  for (p = 0; p < pmax; p++)
    for (c = 0; c < cmax; c++)
    {
      tried[p][c] = 0;  
      fcnt[p][c] = -1;
      seen[p][c] = 0;
    }
}


//= Look for a face in a particular area of the source image at a particular orientation.
// records results (if any) under person "p" and camera "cam" 
// tried: 1 = during in-progress cycle, -1 = during finished cycle, 0 = never
// return -1 for no face, 0 for non-frontal, else frontal count

int jhcFrontal::FaceChk (int p, const jhcImg& src, const jhcRoi& area, double ang, int cam)
{
  jhcRoi mid;
  jhcRoi *det;
  jhcImg *clip;
  double midx = area.RoiAvgX(), midy = area.RoiAvgY();
  double x0 = xoff * area.RoiW(), y0 = yoff * area.RoiH();
  int n;

  // check input values
  if ((p < 0) || (p >= pmax) || (cam > cmax) || !src.Valid(1, 3))
    return Fatal("Bad input to jhcFrontal::ChkFace");

  // set status and get current frontal count
  cx[p][cam] = midx;
  cy[p][cam] = midy;
  rot[p][cam] = ang;
  tried[p][cam] = 1;
  n = __max(0, fcnt[p][cam]);
  fcnt[p][cam] = -1;

  // carve out rotated image patch 
  clip = &(crop[p][cam]);
  clip->vsz = 1;
  clip->SetSize(area.RoiW(), area.RoiH(), src.Fields());
  ExtRotateRGB(*clip, src, midx, midy, ang);

  // stretch contrast based on middle portion
  mid.CenterWithin(0.5, 0.5, 0.5, 0.5, *clip);         // 0.3 also OK
  Enhance3(*clip, *clip, 4.0, &mid);

  // get bounding box from face finder
  det = &(face[p][cam]);
  if (ff.FindWithin(*det, *clip, *clip, fsz) > 0)
  {
    // check position of detection (wrt full image)
    fdx[p][cam] = (det->RoiAvgX() - x0) / det->RoiW();
    fdy[p][cam] = (det->RoiAvgY() - y0) / det->RoiH();
    fcnt[p][cam] = 0;
    if ((fabs(fdx[p][cam]) <= xsh) && (fabs(fdy[p][cam]) <= ysh))
    {
      fcnt[p][cam] = n + 1;
      seen[p][cam] += 1;
    }
  }
  return fcnt[p][cam];
}


//= All camera views for a particular time instant have been entered.
// updates validity and counts, especially for heads not found in certain cameras
// tried: 1 = during in-progress cycle, -1 = during finished cycle, 0 = never
// returns number of faces found (frontal or not) on this cycle

int jhcFrontal::DoneChk ()
{
  int p, c, n = 0;

  // go over all entries (even ones not updated)
  for (p = 0; p < pmax; p++)
    for (c = 0; c < cmax; c++)
    {
      // mark non-detection for entries not explicitly updated this cycle
      if (tried[p][c] <= 0)
      {
        tried[p][c] = 0;
        fcnt[p][c] = -1;
        seen[p][c] = 0;                      // track not valid
        continue;
      }

      // count how many faces actually found (not number of people)
      tried[p][c] = -1;                      // reset for next cycle
      if (fcnt[p][c] >= 0)
        n++;
    }
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                             Result Browsing                           //
///////////////////////////////////////////////////////////////////////////

//= See if system searched for person's face in some camera view.

bool jhcFrontal::Checked (int p, int cam) const
{
  return(ok_idx(p, cam) && (tried[p][cam] != 0));
}


//= See if there is a frontal face with count equal to at least the threshold.
// use fmin to accept non-frontal face detections

bool jhcFrontal::Frontal (int p, int cam, int fmin) const
{
  return(ok_idx(p, cam) && (fcnt[p][cam] >= fmin));
}


//= See if indices are valid.

bool jhcFrontal::ok_idx (int p, int cam) const
{
  return((p >= 0) && (p < pmax) && (cam >= 0) && (cam < cmax));
}


//= Tells how many consecutive frontal faces have been found for a person.
// if cam < 0 then checks across all cameras instead of a specific one
// returns -1 if no face found, 0 if recent face but not frontal

int jhcFrontal::FrontCnt (int p, int cam) const
{
  int c, best = -1;

  if ((p < 0) || (p >= pmax) || (cam > cmax))
    return Fatal("Bad input to jhcFrontal::FrontCnt");

  if (cam >= 0)
    return fcnt[p][cam];
  for (c = 0; c < cmax; c++)
    best = __max(best, fcnt[p][c]);
  return best;
}


//= Find the person with the most recently found face found in some camera.
// must have been detected for at least than fmin frames
// if cam < 0 then checks across all cameras instead of a specific one
// returns person ID (-1 if none), get actual detection count using FrontCnt

int jhcFrontal::FrontNew (int cam, int fmin) const
{
  int p, n, best, win = -1;

  for (p = 0; p < pmax; p++)
    if ((n = FrontCnt(p, cam)) >= fmin)
      if ((win < 0) || (n < best))
      {
        best = n;
        win = p;
      }
  return win;
}


//= Tells the highest frontal count for any person from any camera.
// useful for setting speech attention gate
// returns -1 if no faces, 0 if no frontal faces, else highest count

int jhcFrontal::FrontMax () const
{
  int p, c, best = -1;

  for (p = 0; p < pmax; p++)
    for (c = 0; c < cmax; c++)
      best = __max(best, fcnt[p][c]);
  return best;
}


//= Return the current face bounding box for the biggest frontal detection.
// ensures that frontal count is at least fmin (use zero for non-frontal)
// returns -1 for none, else winning camera number and ROI

int jhcFrontal::FrontBest (jhcRoi& area, int p, int fmin) const
{
  int a, c, win = -1, best = 0;

  if ((p < 0) || (p >= pmax))
    return Fatal("Bad input to jhcFrontal::FrontBest");

  // find all stable detections for this person
  for (c = 0; c < cmax; c++)
    if (fcnt[p][c] >= fmin)
    {
      // keep the one with the biggest bounding box
      a = (face[p][c]).RoiArea();
      if (a > best)
      {
        best = a;
        win = c;
      }
    }

  // see if anything found
  if (win >= 0)
    area.CopyRoi(face[p][win]);
  return win;
}


//= Total number of times a face has been seen associated with this person.
// if cam < 0 then checks across all cameras instead of a specific one
// Note: if face detected simultaneously in several cameras count is artifically high

int jhcFrontal::FaceCnt (int p, int cam) const
{
  int c, sum = 0;

  if ((p < 0) || (p >= pmax) || (cam > cmax))
    return Fatal("Bad input to jhcFrontal::FaceCnt");
  if (cam >= 0)
    return seen[p][cam];
  for (c = 0; c < cmax; c++)
    sum += seen[p][c];
  return sum;
}


//= Tell image coordinates for center of detected face.
// can set sc = 0.5 if depth (960 wide) not big color image (1920 wide)
// returns 1 if okay, 0 if no face

int jhcFrontal::FaceMid (double& fx, double& fy, int p, int cam, double sc) const
{
  double dx, dy, rads, c, s;

  if (!ok_idx(p, cam))
   return Fatal("Bad input to jhcFrontal::FaceMid");
  if (fcnt[p][cam] < 0)
    return 0;

  // get center of face detection wrt cropped probe
  dx = face[p][cam].RoiAvgX() - crop[p][cam].RoiAvgX();
  dy = face[p][cam].RoiAvgY() - crop[p][cam].RoiAvgY();

  // figure out rotation for cropped probe
  rads = -D2R * rot[p][cam];
  c = cos(rads);
  s = sin(rads);

  // get location and size of face detection wrt output image
  fx = sc * (cx[p][cam] + c * dx - s * dy);
  fy = sc * (cy[p][cam] + s * dx + c * dy);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Debugging Graphics                         //
///////////////////////////////////////////////////////////////////////////

//= Mark boxes around all faces found in some camera view.
// assumes destination image is same size as inputs to "ScanRGB"
// negative scale just draws thin white box (no frontal check)
// returns number of boxes drawn

int jhcFrontal::FacesCam (jhcImg& dest, int cam, int rev, double sc) const
{
  double fx2, fy2, wid, ht, ang, xlim = dest.XLim(), sc2 = fabs(sc);
  int p, f, n = 0;

  if ((cam < 0) || (cam >= cmax) || !dest.Valid(1, 3))
    return Fatal("Bad input to jhcFrontal::FacesCam");

  for (p = 0; p < pmax; p++)
    if ((f = fcnt[p][cam]) >= 0)
    {
      // get location of face detection wrt output image
      FaceMid(fx2, fy2, p, cam);
      fx2 *= sc2;
      fy2 *= sc2;

      // get size and rotation of face box
      wid = sc2 * face[p][cam].RoiW();
      ht  = sc2 * face[p][cam].RoiH(); 
      ang = rot[p][cam];

      // possibly swap left-to-right
      if (rev > 0)
      {
        fx2 = xlim - fx2;
        ang = -ang;
      }

      // draw colored box
      if ((sc >= 0.0) && (f > 0))
        RectCent(dest, fx2, fy2, wid, ht, ang, 3, -6);
      else
        RectCent(dest, fx2, fy2, wid, ht, ang, 1);
      n++;
    }
  return n;
}


//= Generate a clipped out portion of main image with face detection marked.
// changes size of destination image to fit extracted region
// returns 2 if frontal, 1 if some face, else 0 for no face

int jhcFrontal::FaceProbe (jhcImg& dest, const jhcImg& src, int p, int cam, int rev) const
{
  jhcRoi box;
  int f;

  if (!ok_idx(p, cam))
    return Fatal("Bad input to jhcFrontal::FaceProbe");

  // get cropped probe image
  box.CopyRoi(face[p][cam]);
  if (rev <= 0)
    dest.Clone(crop[p][cam]);
  else
  {
    // left-to-right reversed
    dest.SetSize(crop[p][cam]);
    FlipH(dest, crop[p][cam]);
    box.MirrorRoi(dest.XDim());
  }

  // show face box (if any) 
  f = fcnt[p][cam];
  if (f >= 0)
    RectEmpty(dest, box, ((f > 0) ? 3 : 1), ((f > 0) ? -6 : -7));
  return __min(2, f + 1);
}


//= Tell location and orientation of probe image clipped out.
// returns 1 if clipped, 0 if not clip region (pose values unchanged)

int jhcFrontal::ProbePose (double& x, double& y, double& degs, int p, int cam, double sc) const
{
  if (!Checked(p, cam))
    return 0;
  x = sc * cx[p][cam];
  y = sc * cy[p][cam];
  degs = rot[p][cam];
  return 1;
}