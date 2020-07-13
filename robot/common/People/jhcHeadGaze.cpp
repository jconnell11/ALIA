// jhcHeadGaze.cpp : computes direction using offset of face from head center
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#include "People/jhcHeadGaze.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcHeadGaze::~jhcHeadGaze ()
{
}


//= Default constructor initializes certain values.
// s3 must be bound for class to work

jhcHeadGaze::jhcHeadGaze ()
{
  s3 = NULL;
  SetGaze(0.0, 0.0, 14.0, 6.0, 20.0, 10.0);
  SetAttn(0.0, 64.0, 96.0);
  Defaults();
  Reset();
}


//= Bind a shared person finder and tracker.
// NOTE: needed before class will work, but beware dangling pointers

void jhcHeadGaze::Bind (jhcStare3D *stare)
{
  s3 = stare;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for computing gaze direction from face position.

int jhcHeadGaze::gaze_params (const char *fname)
{
  jhcParam *ps = &vps;
  int ok;

  ps->SetTag("gaze_vals", 0);
  ps->NextSpecF( &hadj, "Eye height adjust (in)");    
  ps->NextSpecF( &dadj, "Head depth adjust (in)");       // suggest 2
  ps->NextSpecF( &diam, "Face search diameter (in)");  
  ps->NextSpecF( &fwid, "Min face width (in)");          // was 5 
  ps->Skip(2);

  ps->NextSpecF( &ptol, "Attn pan tolerance (deg)");
  ps->NextSpecF( &ttol, "Attn tilt tolerance (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used to define attention point for talking to.

int jhcHeadGaze::attn_params (const char *fname)
{
  jhcParam *ps = &zps;
  int ok;

  ps->SetTag("gaze_attn", 0);
  ps->NextSpecF( &xme, "Attention point X (in)");  
  ps->NextSpecF( &yme, "Attention point Y (in)");  
  ps->NextSpecF( &zme, "Attention point Z (in)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcHeadGaze::Defaults (const char *fname)
{
  int ok = 1;

  ok &= jhcFrontal::Defaults(fname);
  ok &= gaze_params(fname);
  ok &= attn_params(fname);
  return ok;
}


//= Read just deployment specific values from a file.

int jhcHeadGaze::LoadCfg (const char *fname)
{
  int ok = 1;

  ok &= attn_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcHeadGaze::SaveVals (const char *fname, int geom) const
{
  int ok = 1;

  ok &= jhcFrontal::SaveVals(fname);
  ok &= vps.SaveVals(fname);
  ok &= zps.SaveVals(fname);
  return ok;
}


//= Write current deployment specific values to a file.

int jhcHeadGaze::SaveCfg (const char *fname) const
{
  int ok = 1;

  ok &= zps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// NOTE: should call Bind with a person finder first 

void jhcHeadGaze::Reset ()
{
  int i;

  // no one is looking at attention spot yet
  for (i = 0; i < pmax; i++)
    gcnt[i] = 0;

  // reset face finder
  jhcFrontal::Reset();
}


//= Look for tracked people in a roll-corrected color input image.
// assumes all depth images have already been loaded with "Ingest"
// assumes composite floor map has been processed with "Analyze"
// needs corrected depthmap in order to extract face distance

void jhcHeadGaze::ScanRGB (const jhcImg& src, const jhcImg& d16, int cam, int trk)
{
  jhcRoi probe;
  jhcMatrix mid(4), rel(4), fc(4), dir(4);
  jhcBodyData *guy;
  double rot, fx, fy, sc = ((src.YDim() > 640) ? 2.0 : 1.0);
  int p, n;

  if (s3 == NULL)
    Fatal("Unbound person detector in jhcHeadGaze::ScanRGB");
  if ((cam < 0) || (cam >= cmax) || !src.Valid(1, 3))
    Fatal("Bad input to jhcHeadGaze::ScanRGB");
    
  // consider all potential people as viewed from this camera
  s3->AdjGeometry(cam);
  n = s3->PersonLim(trk);
  for (p = 0; p < n; p++)
    if (s3->PersonOK(p, trk))
    {
      // set search area around rotational midpoint of head
      guy = s3->RefPerson(p, trk);
      head_mid(mid, *guy, cam);
      if (search_area(probe, rot, mid, src) <= 0)
        continue;

      // look for face in search area
      if (FaceChk(p, src, probe, rot, cam) >= 0)
      {
        // get realworld face center location
        FaceMid(fx, fy, p, cam, sc);                 // was 0.5 * sc ?
        if (face_pt(fc, fx, fy, d16, sc) > 0)
        {
          // accumulate vector sum of estimates
          dir.DirVec3(fc, mid);
          guy->GazeEst(dir);
        }
      }
    }
}


//= Adjust nominal head position for more accurate results.
// "hadj" shifts expected eye position relative to head center
// "dadj" moves center back from front shell (mostly for single camera) 

void jhcHeadGaze::head_mid (jhcMatrix& mid, const jhcMatrix& head, int cam) const
{
  jhcMatrix kin(4);
  double d;

  // move head outward by dadj along viewing direction
  s3->DumpLoc(kin, cam);
  mid.DiffVec3(head, kin);
  d = mid.LenVec3();
  mid.ScaleVec3((d + dadj) / d);
  mid.IncVec3(kin);
  
  // shift eye position upwards
  mid.IncZ(hadj);
}


//= Set up image area to search for face based on head midpoint position.
// assumes "s3" already has its geometry adjusted for the current camera
// returns 1 if successful, 0 if no good area in this view

int jhcHeadGaze::search_area (jhcRoi& probe, double& rot, const jhcMatrix& mid, const jhcImg& src) const
{
  jhcMatrix rel(4), dir(4);
  double sz, ix, iy, ix2, iy2, sc = ((src.YDim() > 640) ? 2.0 : 1.0), up = 12.0;

  // see how many pixels head should be in image
  s3->BeamCoords(rel, mid);
  sz = diam * s3->ImgScale(rel, sc);
  if ((sz < 20.0) || (sz > 500.0))
    return 0;

  // setup to search image in a square around this point
  s3->ImgPt(ix, iy, rel, sc);
  probe.SetCenter(ix, iy, sz);
  if (src.RoiOverlap(probe) < (0.75 * probe.RoiArea()))
    return 0;

  // find likely rotation of head by looking at a point above it
  dir.RelVec3(mid, 0.0, 0.0, up);
  s3->BeamCoords(rel, dir); 
  s3->ImgPt(ix2, iy2, rel, sc);
  rot = R2D * atan2(iy2 - iy, ix2 - ix) - 90.0;
  return 1;
}


//= Gets realworld face position given center coordinates in color image.
// assumes "s3" already has its geometry adjusted for the current camera
// return 1 if computed, 0 if no good estimate

int jhcHeadGaze::face_pt (jhcMatrix& fc, double fx, double fy, const jhcImg& d16, double sc) const
{
  jhcRoi samp;
  jhcMatrix rel(4);
  double fz;

  // determine depth near face center
  samp.SetCenter(fx / sc, fy / sc, 5.0);
  if (AnyOver_16(d16, samp, 40000) != 0)
    return 0;
  fz = AvgVal(d16, samp, 1);

  // undo camera geometric transforms
  s3->WorldPt(rel, fx, fy, fz, sc);
  s3->InvBeamCoords(fc, rel);
  return 1;
}


//= Call when all input RGB images have been checked for faces.
// blends in average of estimates for each person

void jhcHeadGaze::DoneRGB (int trk)
{
  jhcBodyData *guy;
  int i;

  if (s3 == NULL)
    return;

  DoneChk();
  for (i = 0; i < pmax; i++)
    if ((guy = s3->RefPerson(i, trk)) != NULL)
      guy->UpdateGaze(trk);
  attn_hits(trk);
}


//= Update amount of time each person has looked at attention point.

void jhcHeadGaze::attn_hits (int trk)
{
  jhcMatrix me(4), rel(4);
  const jhcBodyData *guy;
  int i, g0;

  // scan through all people
  me.SetVec3(xme, yme, zme);
  for (i = 0; i < pmax; i++)
  {
    // assume this person is not looking in correct spot
    g0 = gcnt[i];
    gcnt[i] = 0;

    // check if gaze valid yet
    if ((guy = s3->GetPerson(i, trk)) != NULL)
      if ((guy->id > 0) && (guy->gok > 0))
      {
        // compare gaze against vector from head to attention spot
        rel.DiffVec3(me, *guy);
        if ((fabs(rel.PanDiff3( guy->gaze)) <= ptol) && 
            (fabs(rel.TiltDiff3(guy->gaze)) <= ttol))
          gcnt[i] = g0 + 1;
      }
  }
}


//= Check longest that anyone has been looking at attention spot.

int jhcHeadGaze::GazeMax () const
{
  int i, top = 0;

  for (i = 0; i < pmax; i++)
    top = __max(top, gcnt[i]);
  return top;
}


//= Tell how long person with particular ID has been looking at spot.
// returns number of frames, negative if bad ID

int jhcHeadGaze::GazeID (int id, int trk) const
{
  int i;

  if (id < 0)
    return -1;
  for (i = 0; i < pmax; i++)
    if (s3->PersonID(i, trk) == id)
      return gcnt[i];
  return -1;
}


//= Find the index of the person who most recently started to look at spot.
// returns -1 if no one is looking at spot

int jhcHeadGaze::GazeNew (int trk, int gmin) const
{
  int i, best, win = -1;

  for (i = 0; i < pmax; i++)
    if (s3->PersonOK(i, trk) && (gcnt[i] >= gmin))
      if ((win < 0) || (gcnt[i] < best))
      {
        win = i;
        best = gcnt[i];
      }
  return win;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Show gaze rays radiating from center of head in overhead map view.

int jhcHeadGaze::AllGaze (jhcImg& dest, int trk) const
{
  jhcMatrix tip(4), head(4), tail(4);
  const jhcBodyData *guy;
  double len;
  int i, col, n;

  if (s3 == NULL)
    return Fatal("Unbound person detector in jhcHeadGaze::AllGaze");
  if (!dest.SameFormat(s3->ParseWid(), s3->ParseHt(), 1))
    return Fatal("Bad input to jhcHeadGaze::AllGaze");

  // check all people
  len = s3->I2P(240.0);               // 20 feet to end
  n = s3->PersonLim(trk);
  for (i = 0; i < n; i++)
  {
    // skip if gaze is not valid yet
    guy = s3->GetPerson(i, trk);
    if ((guy->id <= 0) || (guy->gok <= 0))
      continue;

    // find end of gaze vector then convert to map coords
    tip.ScaleVec3(guy->gaze, len);
    tip.IncVec3(*guy);
    tail.MatVec(s3->ToMap(), tip);
    head.MatVec(s3->ToMap(), *guy);

    // draw a heavy line for pan 
    col = ((gcnt[i] > 0) ? -3 : -2);
    DrawLine(dest, head.X(), head.Y(), tail.X(), tail.Y(), 3, col);
  }
  return 1;
} 


//= Show a face (presumably gazer) associated with index on some camera view.

int jhcHeadGaze::GazeCam (jhcImg& dest, int i, int cam, int trk) const
{
  const jhcRoi *box;
  double fx2, fy2;

  // check arguments
  if ((cam < 0) || (cam >= cmax) || !dest.Valid(1, 3))
    return Fatal("Bad input to jhcHeadGaze::GazeCam");

  // get location of face detection wrt output image
  if ((i < 0) || !Found(i, cam))
    return 0;
  FaceMid(fx2, fy2, i, cam);
  box = GetFace(i, cam);

  // draw colored box
  RectCent(dest, fx2, fy2, box->RoiW(), box->RoiH(), GetAngle(i, cam), 3, -3);
  return 1;
}
