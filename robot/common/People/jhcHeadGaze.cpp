// jhcHeadGaze.cpp : computes direction using offset of face from head center
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2024-2025 Etaoin Systems
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
  p2s.SetSize(4, 4);
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
// Note: does not load s3 parameters!

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
//                            Frontal Camera                             //
///////////////////////////////////////////////////////////////////////////

//= Set the frontal view camera pose relative to the input camera pose.
// needed since projection is normalized with cx = 0, cy = 0, and cpan = 90
// NOTE: cpos and cdir not saved - used only to get relative pose of color camera

void jhcHeadGaze::SetFront (int view, const jhcMatrix& vpos, const jhcMatrix& vdir,
                            const jhcMatrix& cpos, const jhcMatrix& cdir)
{
  SetAttn(vpos);             // robot eyes = color camera
  s3->SetAlt(view, vpos.X() - cpos.X(), vpos.Y() - cpos.Y(), vpos.Z(), 
                   vdir.P() - cdir.P() + 90.0, vdir.T(), vdir.R());
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

void jhcHeadGaze::ScanRGB (const jhcImg& src, const jhcImg& d16, int view, int trk)
{
  jhcRoi probe;
  jhcMatrix mid(4), rel(4), fc(4), diff(4);
  jhcBodyData *guy;
  double rot, fx, fy, sc = ((src.YDim() > 640) ? 2.0 : 1.0);
  int p, n;

  if (s3 == NULL)
    Fatal("Unbound person detector in jhcHeadGaze::ScanRGB");
  if ((view < 0) || (view >= cmax) || !src.Valid(1, 3))
    Fatal("Bad input to jhcHeadGaze::ScanRGB");
 
  // consider all potential people as viewed from this camera
  s3->SetColorGeom(view);
  p2s.Invert(s3->S2P());                         // member var
  n = s3->PersonLim(trk);
  for (p = 0; p < n; p++)
    if (s3->PersonOK(p, trk))
    {
      // set 3D search area around midpoint of head (and guess in-plane rotation)
      guy = s3->RefPerson(p, trk);
      head_mid(mid, *guy, 0);                    // wrt main depth sensor
      if (search_area(probe, rot, mid, src) <= 0)
        continue;

      // look for face in search area with given orientation
      if (FaceChk(p, src, probe, rot, view) >= 0)
      {
        // get realworld 3D face center location given color detection at (fx fy)
        FaceMid(fx, fy, p, view, sc);  
        if (face_pt(fc, mid, d16, fx, fy) > 0)
        {
          // accumulate vector sum of head->face estimates
          diff.DiffVec3(fc, mid);
          guy->GazeEst(diff);
        }
      }
    }
}


//= Adjust nominal head position for more accurate results.
// "hadj" shifts expected eye position relative to head center
// "dadj" moves center back from front shell (mostly for single sensor) 

void jhcHeadGaze::head_mid (jhcMatrix& mid, const jhcMatrix& head, int cam) const
{
  jhcMatrix kin(4);
  double d;

  // move head outward by dadj along viewing direction (rotates around spine not center)
  s3->DumpLoc(kin, cam);
  mid.DiffVec3(head, kin);
  d = mid.LenVec3();
  mid.ScaleVec3((d + dadj) / d);
  mid.IncVec3(kin);
  
  // shift eye position upwards
  mid.IncZ(hadj);
}


//= Set up color image area to search for face based on head midpoint 3D position.
// also binds "rot" which is the in-plane rotation of the face (around view direction)
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

  // setup to search color image in a square around this point
  s3->ImgPt(ix, iy, rel, sc);
  probe.SetCenter(ix, iy, sz);
  if (src.RoiOverlap(probe) < (0.5 * probe.RoiArea()))     // was 0.75
    return 0;

  // find likely rotation of head by looking at a point straight up from center
  dir.RelVec3(mid, 0.0, 0.0, up);
  s3->BeamCoords(rel, dir); 
  s3->ImgPt(ix2, iy2, rel, sc);
  rot = R2D * atan2(iy2 - iy, ix2 - ix) - 90.0;
  return 1;
}


//= Gets realworld face position given equivalent center coordinates in range image.
// assumes "s3" already has its geometry adjusted for the current camera
// return 1 if computed, 0 if no good estimate

int jhcHeadGaze::face_pt (jhcMatrix& fc, const jhcMatrix& mid, const jhcImg& d16, double fx, double fy) const
{
  jhcRoi samp;
  jhcMatrix rel(4);
  double hz, ftol = 2.0;
  int hx, hy;

  // determine depth in small patch near face center
  if (equiv_head(hx, hy, mid, d16, fx, fy) > ftol)
    return 0;
  samp.SetCenter(hx, hy, 5.0);
  if ((hz = AvgVal_16(d16, samp, -0xFFFF)) < 0.0)
    return 0;

  // undo camera geometric transforms 
  s3->WorldPt(rel, hx, hy, hz);             
  s3->InvBeamCoords(fc, rel);
  return 1;
}


//= Find head pixel in depth image that is closest to face center from color image.
// assumes depth image and color image are oriented similarly wrt pixel errors 
// returns pixel error of projected pixel wrt color image face center

double jhcHeadGaze::equiv_head (int &hx, int &hy, const jhcMatrix& mid, const jhcImg& d16, double fx, double fy) const
{
  jhcMatrix hrel(4), hproj(4), hrng(4);
  double hfmix = 0.7;  
  double rxf, ryf, dx, dy, d2, mx, my, mz, cxf, cyf, czf, best = 10000.0;
  int i, rx, ry, rz, w = s3->XDim(), h = s3->YDim(), rx0 = 0, ry0 = 0;

  // start at head center point (rz = depth in 4 x mm, p2s = member var)
  s3->BeamCoords(hrel, mid);
  hproj.SetVec3(32768.0 + 50.0 * hrel.X(), 32768.0 + 50.0 * hrel.Y(), 32768.0 + 50.0 * hrel.Z());
  hrng.MatVec(p2s, hproj);
  rxf = (s3->XDim2() - 1) + (2.0 * hrng.X() / hrng.Z());
  ryf = (s3->YDim2() - 1) + (2.0 * hrng.Y() / hrng.Z());
  rz  = ROUND(hrng.Z());                      

  for (i = 0; i < 10; i++)
  {
    // get depth reading at newly chosen depth image pixel
    rx = ROUND(rxf);
    ry = ROUND(ryf);
    if ((i > 0) && (rx == rx0) && (ry == ry0))
      break;
    if ((rx < 0) || (ry < 0) || (rx >= w) || (ry >= h))
      break;
    if ((rz = d16.ARef16(rx, ry)) >= 40000)
      break;

    // convert depth point to color image point and compare to face center
    s3->ToCache(mx, my, mz, 0.5 * rxf, 0.5 * ryf, rz);
    s3->FromCache(cxf, cyf, czf, mx, my, mz);
    dx = fx - 2.0 * cxf;
    dy = fy - 2.0 * cyf;    
    d2 = dx * dx + dy * dy;  

    // save if best guess so far
    if ((i == 0) || (d2 < best))
    {
      hx = rx;
      hy = ry;
      best = d2;
    }

    // move depth pixel to make equivalent face pixel closer
    rxf += hfmix * dx;
    ryf += hfmix * dy;
    rx0 = rx;
    ry0 = ry;
  }
  return sqrt(best);
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
  int i, best = 0, win = -1;

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

int jhcHeadGaze::AllGaze (jhcImg& dest, int trk) 
{
  jhcMatrix tip(4), head(4), tail(4);
  char degs[40];
  const jhcBodyData *guy;
  double tilt, len = 240.0;            // 20 feet to end 
  int i, col, n;

  if (s3 == NULL)
    return Fatal("Unbound person detector in jhcHeadGaze::AllGaze");
  if (!dest.SameFormat(s3->ParseWid(), s3->ParseHt(), 1))
    return Fatal("Bad input to jhcHeadGaze::AllGaze");

  // check all people
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

    // draw a heavy line for pan (yellow when staring)
    tilt = tip.TiltVec3();
    col = ((gcnt[i] > 0) ? -3 : ((tilt > 0.0) ? -6 : -2));
    sprintf_s(degs, "%d", ROUND(fabs(tilt)));
    DrawLine(dest, head.X(), head.Y(), tail.X(), tail.Y(), 3, col);
    LabelOver(dest, head.X(), head.Y(), degs);
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
