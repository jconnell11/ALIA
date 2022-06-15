// jhcTable.cpp : find supporting surfaces in full height depth map
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
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

#include "Interface/jhcMessage.h"      // common video

#include "Environ/jhcTable.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcTable::~jhcTable ()
{
}


//= Default constructor initializes certain values.

jhcTable::jhcTable ()
{
  hhist.SetSize(256);
  wlob.SetSize(20);
  Defaults();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters controlling surface height finding.

int jhcTable::height_params (const char *fname)
{
  jhcParam *ps = &hps;
  int ok;

  ps->SetTag("tab_ht", 0);
  ps->NextSpecF( &margin,  24.0, "Range dev wrt preferred (in)");
  ps->NextSpecF( &over,     2.0, "Max above preferred ht (in)");
  ps->NextSpecF( &flip,    12.0, "Max below preferred ht (in)");  
  ps->NextSpecF( &under,    2.0, "Prefer ht below arm lift (in)");
  ps->NextSpec4( &hsm,      4,   "Histogram smoothing");           // was 8    
  ps->NextSpec4( &ppel,   400,   "Min peak in person map (pel)");  // was 200

  ps->NextSpecF( &dp,       5.0, "Centralizing for pan (deg)"); 
  ps->NextSpecF( &dt,      10.0, "Adjustment for edge tilt (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters controlling finding and tracking of surface candidates.

int jhcTable::cand_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("tab_cand", 0);
  ps->NextSpecF( &ztol,  2.0,  "Band around table ht (in)");
  ps->NextSpec4( &wsc,    9,   "Table mask smoothing (pel)");
  ps->NextSpec4( &wth,   80,   "Smooth region threshold");
  ps->NextSpec4( &wmin, 500,   "Min table area (pel)");
  ps->Skip();
  ps->NextSpecF( &pmix,   0.2, "Smooth estimate blending");

  ps->NextSpecF( &pn,     1.0, "Lateral estimate noise (in)");
  ps->NextSpecF( &hn,     0.5, "Height estimate noise (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcTable::Defaults (const char *fname)
{
  int ok = 1;

  ok &= height_params(fname);
  ok &= cand_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcTable::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= hps.SaveVals(fname);
  ok &= cps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Set up local images for proper wide angle depth projection size.

void jhcTable::SetSize (int x, int y)
{
  wmap.SetSize(x, y, 1);
  wbin.SetSize(wmap);
  wsm.SetSize(wmap);
  wcc.SetSize(wmap, 2);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcTable::Reset ()
{  
  // track smoothing 
  tmid.SetKal(pmix, pn, pn, hn);

  // preferences
  dpref = 0.0;               // any distance
  xpref = 0.0;               // robot origin
  ypref = 0.0;
  hpref = 28.0;              // table height

  // results
  tsel = -1; 
  tmid.Clear(0.0);
  ztab = hpref;              

  // number of frames
  fcnt = 0;
}


//= Choose reasonable table level based on wide range height map.
// assumes x0 = middle and y0 = 0 always facing forward with res pixels per inch
// pixel heights range from z0 (=1) to z1 (=254)
// returns best supporting surface height above floor in inches

double jhcTable::PickPlane (const jhcImg& hts, double res, double z0, double z1)
{
  jhcArr hhist0(256);
  int pk;

  if (!hts.SameFormat(wmap))
    return Fatal("Bad images to jhcTable::PickPlane");

  // cache inputs for later (typically from person finder jhcStare3D)
  wmap.CopyArr(hts);
  wipp = res;
  zbot = z0;
  zrng = z1 - z0;

  // get smoothed histogram of heights at some distance range 
  if ((dpref <= 0.0) || (dpref >= 240.0))
    hist_range(hhist0, wmap, 0.0, 240.0);
  else
    hist_range(hhist0, wmap, dpref - margin, dpref + margin);
  hhist.Boxcar(hhist0, hsm);

  // try to pick plane closest BELOW preferred height (with some padding)
  if (flip > 0.0)
  {
    pk = hhist.NearMassPeak(i2z(hpref + over), ppel, 0);
    zest = z2i(pk);
    if ((hhist.ARef(pk) >= ppel) && ((hpref - zest) < flip))
    {
      if (fabs(ztab - zest) > 1.0)
        ztab = zest;
      else
        ztab += pmix * (zest - ztab);
      return ztab;
    }
  }
     
  // otherwise pick closest plane below or ABOVE given height (exact)
  pk = hhist.NearMassPeak(i2z(hpref), ppel, 1);
  zest = z2i(pk);
  if (hhist.ARef(pk) < ppel)
    zest = hpref;
  if (fabs(ztab - zest) > 1.0)
    ztab = zest;
  else
    ztab += pmix * (zest - ztab);
  return ztab;
}


//= Get height histogram focussing on certain distance from robot.
// histograms heights in range from close to far inches from center
// assumes head will be turned to get proper side (if any)
// helps find correct height despite larger distractors

void jhcTable::hist_range (jhcArr& hist, const jhcImg& hts, double close, double far)
{
  double cpos = __max(0.0, close), fpos = __max(0.0, far);
  int c = ROUND(cpos / wipp), f = ROUND(fpos / wipp), c2 = c * c, f2 = f * f;
  int w = hts.XDim(), mx = w / 2, h = hts.YDim(), sk = hts.Skip();
  int x, y, y2, cmy2, fmy2, dx, dx2;
  const UC8 *z = hts.PxlSrc();

  // check if whole image should be histogrammed
  hist.Fill(0);
  if ((c2 <= 0) && ((mx * mx + h * h) <= f2))
  {
    for (y = 0; y < h; y++, z += sk)
      for (x = 0; x < w; x++, z++)
        if (*z != 0)
          hist.AInc(*z, 1);
    return;
  }

  // limit histogram to near, middle, or far ring
  for (y = 0; y < h; y++, z += sk)
  {
    y2 = y * y;
    cmy2 = c2 - y2;                    // robot at (mx 0)
    fmy2 = f2 - y2;
    for (x = 0; x < w; x++, z++)
      if (*z != 0)                     // ignores zeroes
      {
        dx = x - mx;
        dx2 = dx * dx;
        if ((cmy2 <= dx2) && (dx2 <= fmy2))
          hist.AInc(*z, 1);
       }
  }
}


//= Find location of most salient support surface and viewing parameters for head.
// assumes heights (wmap) and interpretation parameters already loaded via PickPlane
// finds centroid "tmid" of closest surface and edge "offset" from centroid 
// returns 1 if some table value cached, 0 if nothing suitable found

int jhcTable::FindSurf (const jhcMatrix& head, double ht)
{
  double mx = wmap.RoiAvgX();
  int iz = i2z(ztab), dev = zdev(ztol), t = -1;

  // get pixels in height range then find large components 
  Between(wbin, wmap, __max(1, iz - dev), iz + dev);
  BoxAvg(wsm, wbin, wsc, wsc);
  CComps4(wcc, wsm, wmin, wth);
  wlob.FindParams(wcc);

  // pick closest to preferred spot (reset to robot origin and arm height)
  head.DumpVec3(hx, hy, hz);
  if (++fcnt > 2)                      
    t = wlob.Nearest(mx + xpref / wipp, ypref / wipp);
  xpref = 0.0;
  ypref = 0.0;
  hpref = ht - under;
  return update_surf(t);
}


//= Update smoothed position and offset of currently selected surface.
// returns 1 if some track selected, 0 for nothing

int jhcTable::update_surf (int t)
{
  double mx = wmap.RoiAvgX();
  double cx, cy, step, dx, dy, dist;

  // default preferences (signal if major shift?)
//  dpref = 0.0;
//  hpref = shelf - under;

  // reset parameters if nothing chosen
  if (t < 0)
  {
    tsel = -1;
    tmid.Clear(0.0);
    return 0;
  }

  // update tracked center and set preferences for next cycle
  tsel = t;
  wlob.BlobCentroid(&cx, &cy, tsel);
  tmid.Update(wipp * (cx - mx), wipp * cy, zest);          // raw z estimate
//  dpref = tmid.PlaneVec3();
//  hpref = ztab;

  // find directional ray intersection with front edge (pixels)
  ex = mx;
  ey = 0.0;
  step = tmid.X() / tmid.Y();
  while (ey <= cy)
  {
    if (wcc.ARef16(ROUND(ex), ROUND(ey)) == tsel)
      break;
    ex += step;
    ey += 1.0;  
  }

  // blend new edge offset (rough size) into old estimate
  dx = tmid.X() - wipp * (ex - mx);
  dy = tmid.Y() - wipp * ey;
  dist = sqrt(dx * dx + dy * dy);
  if (tmid.First() > 0)
    offset = dist;
  else    
    offset += pmix * (dist - offset);

//jprintf("  raw detection = [%3.1f %3.1f %3.1f] off = %3.1f\n", wipp * (cx - mx), wipp * cy, zest, dist);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Alternative Targets                          //
///////////////////////////////////////////////////////////////////////////

//= Select surface blob closest to given planar position as the one to track.
// returns distance of blob centroid from desired position, negative if none

double jhcTable::BestSurf (double x, double y)
{
  double dx, dy;
  int t;

  tsel = -1;
  tmid.Clear(0.0);
  t = wlob.Nearest(wmap.RoiAvgX() + x / wipp, y / wipp);
  if (update_surf(t) <= 0)
    return -1.0;
  dx = tmid.X() - x;
  dy = tmid.Y() - y;
  return sqrt(dx * dx + dy * dy);
}


//= Select position and offset for next farthest of detected surfaces.
// can call InitSurf immediately before to restart enumeration
// returns distance if something found, negative if nothing suitable

double jhcTable::NextSurf ()
{
  double cx, cy, dx, out2, best2, mx = wmap.RoiAvgX(), ref2 = 0.0;
  int i, t0 = tsel, n = wlob.Active();

  // find distance of current surface from origin
  if (tsel >= 0)
  {
    wlob.BlobCentroid(&cx, &cy, tsel);
    dx = cx - mx;
    ref2 = dx * dx + cy * cy;
    tsel = -1;
  }

  // look for some other blob just beyond reference distance
  for (i = 0; i < n; i++)
    if (wlob.GetStatus(i) > 0)
    {
      wlob.BlobCentroid(&cx, &cy, i);
      dx = cx - mx;
      out2 = dx * dx + cy * cy;
      if ((out2 > ref2) && ((tsel < 0) || (out2 < best2)))
      {
        tsel = i;
        best2 = out2;
      }
    }

  // update tracking if successful
  if (tsel == t0)
    return -1.0;
  if (update_surf(tsel) <= 0)
    return -1.0;
  return(wipp * sqrt(best2));
}


///////////////////////////////////////////////////////////////////////////
//                          Target Information                           //
///////////////////////////////////////////////////////////////////////////

//= Return coordinates for closest edge point of table relative to robot.

void jhcTable::SurfEdge (jhcMatrix& edge, const jhcMatrix& mid, double off) const
{
  double rads = D2R * mid.PanVec3();

//jprintf("SurfEdge: mid (%3.1f %3.1f %3.1f) off %3.1f\n", mid.X(), mid.Y(), mid.Z(), offset);
  edge.SetX(mid.X() - off * cos(rads));
  edge.SetY(mid.Y() - off * sin(rads));
  edge.SetZ(mid.Z());
}

/*
//= Determine good gaze direction to observe objects given closest edge of surface.
// assumes head is in same pose as passed to FindSurf (height cached)
// similar to jhcNeck::AimFor but biases neck angles closer to straight ahead

void jhcTable::SurfGaze (double& pan, double& tilt, const jhcMatrix& edge) const
{
  double dr, dx = edge.X() - hx, dy = edge.Y() - hy, dz = edge.Z() - hz;

  // compute pan for patch edge point then adjust
  pan = R2D * atan2(dy, dx) - 90.0;
  if (pan > dp)
    pan -= dp;
  else if (pan < -dp)
    pan += dp;
  else
    pan = 0.0;

  // compute tilt for closest edge then adjust
  dr = sqrt(dx * dx + dy * dy);
  tilt = R2D * atan2(dz, dr);
  if (tilt < -dt)  
    tilt += dt;
  else
    tilt = 0.0;
}


//= Determine good gaze direction to observe objects on current surface.
// assumes head is in same pose as passed to FindSurf (cached)
// returns 1 if valid, 0 if no surface found (values unchanged)

int jhcTable::SurfGaze (double& pan, double& tilt) const
{
  jhcMatrix ej(4); 

  if (tsel < 0) 
    return 0; 
  SurfEdge(ej, tmid); 
  SurfGaze(pan, tilt, ej);
  return 1;
}
*/

//= Find out how far robot can travel straight toward current surface.
// "hw" is half the robot width in inches, ymin in image height to start search
// checks for at least one completely non-surface line to be found in range
// returns distance from origin (inches) not accounting for robot prow

double jhcTable::SurfMove (double hw, int ymin) const
{
  int iw = wcc.XDim(), pel = ROUND(2.0 * hw / wipp), rw = __min(pel, iw);
  int ih = wcc.YDim(), x0 = ROUND(0.5 * (iw - rw)), y0 = __max(0, ymin);
  int x, y, sk2 = wcc.RoiSkip(rw) >> 1;
  const US16 *c = (const US16 *) wcc.RoiSrc(x0, y0);

  if (tsel < 0) 
    return 0.0;
  for (y = y0; y < ih; y++, c += sk2)
    for (x = 0; x < rw; x++, c++)
      if (*c == tsel)
        return((y <= y0) ? 0.0 : (y - 1) * wipp);
  return((y <= y0) ? 0.0 : (y - 1) * wipp);
}
