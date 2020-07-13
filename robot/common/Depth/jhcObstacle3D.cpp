// jhcObstacle3D.cpp : analyzes depth data for local obstacle map
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2014 IBM Corporation
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

#include "jhcObstacle3D.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcObstacle3D::jhcObstacle3D ()
{
  sf = NULL;
  Defaults();
  Reset();
}


//= Default destructor does necessary cleanup.

jhcObstacle3D::~jhcObstacle3D ()
{
}


//= Reset state for the beginning of a sequence.
// need to call this if any of the mapping parameters change

void jhcObstacle3D::Reset ()
{
  int rhw = ROUND(0.5 * rwide / fpp);   // for laterally symmetric footprint

  // images for obstacle avoidance
  floor.SetSize(ROUND(2.0 * fside / fpp), ROUND((ffront + fback) / fpp));
  favg.SetSize(floor);
  fobst.SetSize(floor);
  fsp.SetSize(floor);
  fprev.SetSize(floor);
  fbin.SetSize(floor);
  fdist.SetSize(floor);
  fmv.SetSize(floor);
  fcol.SetSize(floor, 3);

  // polar projection around robot (0 +/- 120 degs)
  dirs.SetSize(241);

  // location of camera and robot in occupancy map
  fcx = 0.5 * floor.XDim();
  fcy = fback / fpp;
  bot.RoiClip(floor);
  bot.SetRoi(ROUND(fcx) - rhw, ROUND((fback - rback) / fpp),
             2 * rhw, ROUND((rarm + rback) / fpp));

  // temporary image
  if (sf != NULL)
    tmp.SetSize(sf->XDim2(), sf->YDim2(), 1);  

  // clear accumulated map
  fsp.FillArr(128);
  phase = 0;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcObstacle3D::Defaults (const char *fname)
{
  int ok;

  ok &= bot_params(fname);
  ok &= occ_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcObstacle3D::SaveVals (const char *fname) const
{
  int ok;

  ok &= bps.SaveVals(fname);
  ok &= ops.SaveVals(fname);
  return ok;
}


//= Parameters describing robot geometry.

int jhcObstacle3D::bot_params (const char *fname)
{
  jhcParam *ps = &bps;
  int ok;

  ps->SetTag("3d_bot", 0);
  ps->NextSpecF( &rarm,    8.0,  "Arm wrt camera (in)");
  ps->NextSpecF( &rfront,  4.0,  "Front wrt camera (in)");
  ps->NextSpecF( &rmid,    7.75, "Wheels wrt camera (in)");
  ps->NextSpecF( &rback,  21.5,  "Back wrt camera (in)");  
  ps->Skip();
  ps->NextSpecF( &rwide,  14.5,  "Max width (in)");  

  ps->NextSpecF( &hdrm,    6.0,  "Overhead clearance (in)");  
  ps->NextSpecF( &flank,   1.0,  "Side clearance (in)");      // was 3
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters describing local occupancy map.

int jhcObstacle3D::occ_params (const char *fname)
{
  jhcParam *ps = &ops;
  int ok;

  ps->SetTag("3d_occ", 0);
  ps->NextSpecF( &ffront, 120.0, "Ahead wrt camera (in)");  
  ps->NextSpecF( &fback,   60.0, "Behind wrt camera (in)");  
  ps->NextSpecF( &fside,   60.0, "Lateral wrt camera (in)");  
  ps->NextSpecF( &fpp,      0.5, "Map resolution (ipp)");  
  ps->NextSpecF( &fz,       4.0, "Floor deviation (in)");    // was 2
  ps->NextSpec4( &fclr,    28,   "Traversable threshold");  

  ps->NextSpec4( &finc,    20,   "Update step");             // 2 samples to fclr
  ps->NextSpec4( &fdec,     4,   "Decay interval");          // 1 = 4.2 sec decay   
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Look for sudden height changes on floor to find obstacles.
// accumulates local map around robot of where obstacles are
// map is built with camera as center (not wheelbase midpoint)
// needs to know robot travel since last update 
//   dx = move orthogonal to previous direction (in prev coords)
//   dy = move in previous direction of travel (in prev coords)
//   da = change in heading (body) direction (wrt previous)
//   ht = height of camera relative to floor
// dx and dy are relative to robot wheelbase midpoint (not camera)
// assumes orthogonal world coordinates already found (CacheXYZ)
// 0 = obstacle, 255 = floor, 128 = unknown

void jhcObstacle3D::BuildFree (double dx, double dy, double da, double ht)
{
  double rads = D2R * da;
  double cdx = (dx - sin(rads) * rmid) / fpp;
  double cdy = (dy - (1.0 - cos(rads)) * rmid) / fpp;
  int sm = ROUND(3.0 / fpp) | 0x01;                      // was 1.5"

  if (sf == NULL)
    Fatal("Bad initialization jhcObstacle3D::BuildFree");

  // alter previous map for new position and orientation
  t.Rigid(fprev, fsp, -da, fcx, fcy, fcx + cdx, fcy + cdy, 128);  
  fsp.CopyArr(fprev);

  // get current top-view image and find obstructions
  sf->Plane(floor, fpp, fback, 0.0, fz, ht + hdrm);
  t.RectFill(floor, bot, 128);
  t.NotBoxAvg(favg, floor, sm, sm, 0, 0);
  t.EdgeDup(favg, 1);
  mark_obst(fobst, favg);

  // combine old and new maps (and clear robot footprint)
  if (++phase >= fdec)
    phase = 0;
  update_map(fsp, fobst); 
  t.RectFill(fsp, bot, 255);

  // threshold and shrink by robot size
  t.Threshold(fbin, fsp, 128 + fclr);
//  t.Nearest8(fmv, fbin, 0, &fdist); 
  t.Voronoi8(fmv, fbin, 255, &fdist); 
  t.Threshold(fmv, fdist, ROUND((0.5 * rwide + flank) / fpp));
t.ClipScale(fdist, fdist, 128.0 * fpp / (0.5 * rwide + flank));

  // reproject in polar coordinates
//  polar_proj(dirs, fmv);
}


//= Convert deviations from floor (128) to obstacle markings.
// 0 = obstacle, 255 = floor, 128 = unknown

void jhcObstacle3D::mark_obst (jhcImg& dest, const jhcImg& src) const
{
  int x, y, w = dest.XDim(), h = dest.YDim(), sk = dest.Skip();
  const UC8 *s = src.PxlSrc();
  UC8 *d = dest.PxlDest();

  for (y = 0; y < h; y++, d += sk, s += sk)
    for (x = 0; x < w; x++, d++, s++)
      if (*s == 0)
        *d = 128;
      else
        *d = 255 - (abs(*s - 128) << 1); 
}


//= Decay toward 128 if no information else move toward current value.

void jhcObstacle3D::update_map (jhcImg& dest, const jhcImg& src) const
{
  int x, y, mv, w = dest.XDim(), h = dest.YDim(), sk = dest.Skip();
  const UC8 *s = src.PxlSrc();
  UC8 *d = dest.PxlDest();

  for (y = 0; y < h; y++, d += sk, s += sk)
    for (x = 0; x < w; x++, d++, s++)
      if (*s != 128)
      {
        // move closer to observation
        mv = abs(*s - *d);
        mv = __min(mv, finc);
        if (*d > *s)
          *d -= mv;
        else
          *d += mv;
      }
      else if (phase == 0)
      {
        // decay old parts of map
        if (*d > 128)
          *d -= 1;
        else
          *d += 1;
      }
}


///////////////////////////////////////////////////////////////////////////
//                          Debugging Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Draws a rough outline of robot in magenta on some grayscale image.
// automatically adjusts positions if output image is magnified

int jhcObstacle3D::OverlayBot (jhcImg& dest, const jhcImg& src, int body) 
{
  double sc = dest.XDim() / (double) floor.XDim();
  double scx = sc * fcx, scy = sc * fcy, spp = sc / fpp;
  int cr = ROUND(spp * 4.0);

  if (!src.Valid(1) || !dest.Valid(3) || 
      (sc <= 0.0) || (dest.YDim() != ROUND(sc * floor.YDim())))
    return Fatal("Bad images to jhcObstacle3D::OverlayBot");

  // make a big color version of input
  t.CopyMono(fcol, src);
  t.SampleN(dest, fcol);

  // mark rough location of Kinect sensor
  t.Cross(dest, scx, scy, cr, cr, 1, 255, 0, 255);

  // draw rectangle including arm and wheel base
  if (body > 0)
  {
    t.RectEmpty(dest, 
                ROUND(scx - 0.5 * spp * rwide), ROUND(spp * (fback - rback)),
                ROUND(spp * rwide), ROUND(spp * (rfront + rback)), 
                1, 255, 0, 255);  
    t.Cross(dest, scx, scy - spp * rmid, ROUND(spp * rwide), cr, 1, 255, 0, 255);
  }
  return 1;
}


//= Convert input color image to grayscale and show tranversable areas.
// meant for color camera associated with Kinect depth sensor
// 0 = obstacle, 255 = floor, 128 = unknown

int jhcObstacle3D::MarkFree (jhcImg& dest, const jhcImg& src, int dok, int dbad) 
{
  int gth = 128 + dok, rth = 128 - dbad;
  int x, y, i, w = dest.XDim(), h = dest.YDim(), sk = dest.Skip();
  const UC8 *s = src.PxlSrc(), *v = tmp.PxlSrc();
  UC8 *d = dest.PxlDest();

  if (sf == NULL)
    return Fatal("Bad initialization jhcObstacle3D::MarkFree");
  if (!dest.SameFormat(sf->XDim2(), sf->YDim2(), 3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcObstacle3D::MarkFree");

  // undo floor projection
  sf->MapBack(tmp, fobst, -fz, fz, fpp, fback, 128);

  // shade areas according to classification
  for (y = 0; y < h; y++, d += sk, s += sk, v += sk)
    for (x = 0; x < w; x++, d += 3, s += 3, v++)  
    {
      // convert input to dim monochrome
      i = (s[0] + s[1] + s[2]) >> 2;
      d[0] = i;
      d[1] = i;
      d[2] = i;

      // boost green if free, red if obstacle
      if (*v > gth)
        d[1] = BOUND(i + 100);
      else if (*v < rth)
        d[2] = BOUND(i + 100);
    }
  return 1;
}


//= Convert input color image to grayscale and show allowed robot centers.
// meant for color camera associated with Kinect depth sensor

int jhcObstacle3D::MarkDrive (jhcImg& dest, const jhcImg& src)
{
  int x, y, i, w = dest.XDim(), h = dest.YDim(), sk = dest.Skip();
  const UC8 *s = src.PxlSrc(), *v = tmp.PxlSrc();
  UC8 *d = dest.PxlDest();

  if (sf == NULL)
    return Fatal("Bad initialization jhcObstacle3D::MarkDrive");
  if (!dest.SameFormat(sf->XDim2(), sf->YDim2(), 3) || !dest.SameFormat(src))
    return Fatal("Bad images to jhcObstacle3D::MarkDrive");

  // undo floor projection
  sf->MapBack(tmp, fmv, -fz, fz, fpp, fback, 0);

  // shade areas according to classification
  for (y = 0; y < h; y++, d += sk, s += sk, v += sk)
    for (x = 0; x < w; x++, d += 3, s += 3, v++)  
    {
      // convert input to dim monochrome
      i = (s[0] + s[1] + s[2]) >> 2;
      d[0] = i;
      d[1] = i;
      d[2] = i;

      // boost blue if robot will fit at location
      if (*v > 128)
        d[0] = BOUND(i + 100);
    }
  return 1;
}


