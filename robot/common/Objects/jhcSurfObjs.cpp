// jhcSurfObjs.cpp : find objects on surfaces using single mobile depth scanner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 Etaoin Systems
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

#include "Objects/jhcSurfObjs.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSurfObjs::~jhcSurfObjs ()
{

}


//= Default constructor initializes certain values.

jhcSurfObjs::jhcSurfObjs ()
{
  pos.axes = 0;              // camera not stationary
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for finding supporting surface.

int jhcSurfObjs::surf_params (const char *fname)
{
  jhcParam *ps = &zps;
  int ok;

  ps->SetTag("sobj_surf", 0);
  ps->NextSpecF( &lrel,  -2.0, "Default wrt lift height (in)");  
  ps->NextSpec4( &ppel, 200,   "Min peak in person map (pel)");
  ps->NextSpecF( &sfar,  96.0, "Max intersect dist (in)");  
  ps->NextSpecF( &wexp,   1.0, "Map width expansion factor");  
  ps->Skip();
  ps->NextSpec4( &spel, 500,   "Min surface patch (pel)");  

  ps->NextSpec4( &cup,  100,   "Occlusion fill width (pel)");  
  ps->NextSpec4( &bej,    5,   "FOV edge shrinkage (pel)");  

  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcSurfObjs::Defaults (const char *fname)
{
  int ok = 1;

  ok &= surf_params(fname);
  ok &= jhcBumps::Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcSurfObjs::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= zps.SaveVals(fname);
  ok &= jhcBumps::SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcSurfObjs::Reset ()
{
  xcomp = 0.0;
  ycomp = 0.0;
  pcomp = 0.0;
  jhcBumps::Reset(1);
}


//= Find objects by analyzing supporting surface.
// "pos" holds  position of camera and "dir" hold its (pan tilt roll) orientation
// assumes "ztab" already holds expected height (e.g. jhcOverhead3D::PickPlane)
// returns number of raw objects found (not number being tracked)

int jhcSurfObjs::FindObjects (const jhcImg& d16, const jhcMatrix& pos, const jhcMatrix& dir)
{
  double dz, yhit, rhit;

  // make camera always aimed upwards in map (compensate externally)
  SetCam(0, 0.0, 0.0, pos.Z(), 90.0, dir.T(), dir.R()); 
  adj_tracks(pos.X(), pos.Y(), dir.P());

  // find radial distance and width of beam at table
  dz = ztab - pos.Z();
  yhit = dz / tan(D2R * dir.T());
  yhit = __max(0.0, __min(yhit, sfar));
  rhit = sqrt(dz * dz + yhit * yhit);

  // adjust resolution and map offset
  mw = wexp * rhit * d16.XDim() / kf;
  x0 = 0.5 * mw;
  ipp = mw / map.XDim();
  mh = ipp * map.YDim();
  y0 = 0.5 * mh - yhit;

  // get heights relative to ztab
  Reproject(map, d16); 
  return Analyze();
}


//= Alter tracked object parameters to match new sensor pose.
// location of camera always (0 0) with pan = 90 for map generation
// (sx sy) and pan are in world coodinates straight from neck
// ignores bounding box since sets jhcSmTrack::axes = 0

void jhcSurfObjs::adj_tracks (double sx, double sy, double pan)
{
  double trads = D2R * (pcomp - 90.0), c0 = cos(trads), s0 = sin(trads);
  double srads = -D2R * (pan - 90.0), c2 = cos(srads), s2 = sin(srads);
  double tx, ty, x, y, ang, dp = pan - pcomp;
  int i, n = pos.Limit();

  // look for all valid and probationary tracks
  for (i = 0; i < n; i++)
    if (pos.Valid(i) >= 0)
    {
      // get full world location of object from jhcSmTrack
      tx = pos.TX(i);
      ty = pos.TY(i);
      x = tx * c0 - ty * s0 + xcomp;          
      y = tx * s0 + ty * c0 + ycomp;

      // update jhcSmTrack so relative to current sensor pose
      x -= sx;
      y -= sy;
      pos.ForceTX(i, x * c2 - y * s2);
      pos.ForceTY(i, x * s2 + y * c2);

      // change ellipse orientation for current pan angle
      ang = shp[i][5] + dp;
      if (ang > 180.0)
        ang -= 180.0;
      else if (ang < 0.0)
        ang += 180.0;
      shp[i][5] = ang;
    }

  // remember current sensor pose used for map generation
  xcomp = sx;
  ycomp = sy;
  pcomp = pan;
}


//= Get full world coordinates of object with some index (not ID).
// returns planar orientation direction if successful, negative for problem

double jhcSurfObjs::World (jhcMatrix& loc, int i) const
{
  double tx, ty, ang, p90 = pcomp - 90.0, rads = D2R * p90, c = cos(rads), s = sin(rads);

  if ((loc.Vector(3) <= 0) || (pos.Valid(i) <= 0))
    return -1.0;
  if (loc.Vector(4) > 0)
    loc.SetH(1.0);

  // compute world coordinates from map values given current sensor pose
  tx = pos.TX(i);
  ty = pos.TY(i);
  loc.SetX(tx * c - ty * s + xcomp);          
  loc.SetY(tx * s + ty * c + ycomp);
  loc.SetZ(pos.TZ(i));

  // figure out world orientation for map value given current sensor pan
  ang = Angle(i, 1) + p90;
  if (ang > 180.0)
    ang -= 180.0;
  else if (ang < 0.0)
    ang += 180.0;
  return ang;
}


///////////////////////////////////////////////////////////////////////////
//                               Overrides                               //
///////////////////////////////////////////////////////////////////////////

//= Find candidate object pixels in overhead map assuming movable camera.
// uses image "map" to yield info in "cc" image and "blob" analyzer
// also computes "top" image representing table each time
// overrides "raw_objs" function from base class jhcBumps

void jhcSurfObjs::raw_objs (int trk)
{
jtimer(50, "raw_objs");
  int dev = ROUND(50.0 * htol / hobj);

jtimer(51, "evidence");
  // find deviations from best plane fit (table = 128, hobj = 128 + 50)
  Interpolate(sm, pmin);
  PlaneDev(det, map2, 2.0 * hobj);               // uses srng
  RampOver(obj, det, 178 - dev, 178 + dev);
jtimer_x(51);

jtimer(52, "object form");
  // group protrusions into objects
  BoxAvg(obj, obj, sc, sc);
  CComps4(cc, obj, amin, sth);
  blob.FindParams(cc);    
jtimer_x(52);      

jtimer(53, "table");
  // get more comprehensive surface mask
  InRange(top, det, 78, 178, dev);
  BoxAvg(top, top, sc);
  RemSmall(top, top, 0.0, spel, sth);
jtimer_x(53);
jtimer(54, "table form");
  ConvexUp(top, top, cup, 90);
  BeamEmpty(top, ztab, 2 * bej, 25);
jtimer_x(54);

  // suppress components extending beyond table
jtimer(55, "PoisonOver");
  blob.PoisonOver(cc, top, -50);
jtimer_x(55);
jtimer_x(50);
}
