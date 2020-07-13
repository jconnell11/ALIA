// jhcFollow3D.cpp : analyzes depth data to find person's waist
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

#include "Depth/jhcFollow3D.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcFollow3D::jhcFollow3D ()
{
  // connected component analysis
  dudes.SetSize(200);
  sf = NULL;

  // load processing parameters
  Defaults();
  Reset();
}


//= Default destructor does necessary cleanup.

jhcFollow3D::~jhcFollow3D ()
{
}


//= Reset state for the beginning of a sequence.

void jhcFollow3D::Reset ()
{
  // images for person finding
  wproj.SetSize(ROUND(2.0 * wside / wpp), ROUND(wfront / wpp));
  wtmp.SetSize(wproj);
  wcc.SetSize(wproj, 2);

  // temporary image
  if (sf != NULL)
    tmp.SetSize(sf->XDim2(), sf->YDim2(), 1);  

  // not tracking any person yet
  ClrLeader();
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcFollow3D::Defaults (const char *fname)
{
  int ok;

  ok &= map_params(fname);
  ok &= waist_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcFollow3D::SaveVals (const char *fname) const
{
  int ok;

  ok &= mps.SaveVals(fname);
  ok &= wps.SaveVals(fname);
  return ok;
}


//= Parameters used for generating person map.

int jhcFollow3D::map_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("3d_wmap", 0);
  ps->NextSpecF( &wht,    39.0, "Slice height (in)");  
  ps->NextSpecF( &wsz,     6.0, "Slice span (in)");  
  ps->NextSpecF( &wfront, 96.0, "Projection front distance (in)");  
  ps->NextSpecF( &wside,  36.0, "Projection side distance (in)");  
  ps->NextSpecF( &wpp,     0.3, "Projection resolution (ipp)");  
  ps->Skip();

  ps->NextSpec4( &wsm,     3,   "Projection smoothing (pels)");
  ps->NextSpec4( &wth,    80,   "Smooth thresh");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for finding people.

int jhcFollow3D::waist_params (const char *fname)
{
  jhcParam *ps = &wps;
  int ok;

  ps->SetTag("3d_waist", 0);
  ps->NextSpecF( &wmin,    8.0, "Min width (in)");          // was 10     
  ps->NextSpecF( &wmax,   36.0, "Max width (in)");  
  ps->NextSpecF( &wfat,    1.5, "Min elongation");  
  ps->NextSpecF( &wthin,   4.5, "Max elongation");  
  ps->Skip(2);

  ps->NextSpecF( &wnear,  12.0, "Track proximity (in)");
  ps->NextSpec4( &wdrop,  45,   "Track dropout (frames)");  // 1.5 sec
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get rid of any current target.

void jhcFollow3D::ClrLeader () 
{
  targ = 0;
  tx   = 0.0;
  ty   = 0.0;
  azm  = 0.0;
  dist = 0.0;
  look = 0.0;
}


//= Look for likely people in the scene based on detecting waist regions.
// targ: 0 = uninitialized, +N = some blob, -M = failed to find M times
// use AngleLeader and DistLeader to extract target direction
// returns 2 if candidate found, 1 if flywheeling, 0 if values not altered

int jhcFollow3D::FindLeader (double gaze, double dinit)
{
  double x;
  int win;

  if (sf == NULL)
    return Fatal("Bad initialization jhcFollow3D::FindLeader");

  // find best candidate using current local waist-level map
  look = gaze;
  leader_blobs();
  win = pick_leader(dinit);

  // remember new position, give up, or flywheel through dropout
  if (win > 0)
  {
    targ = win;
    dudes.BlobCentroid(&tx, &ty, win);
  }
  else if ((targ == 0) || (targ < -wdrop))
  {
    targ = 0;
    return 0;             // does not change azm or dist
  }
  else if (targ > 0)
    targ = -1;
  else 
    targ--;

  // get current distance and bearing to target
  x = tx - 0.5 * wproj.XDim();
  azm = R2D * atan2(-x, ty);
  dist = wpp * sqrt(x * x + ty * ty);
//jprintf(" img (%3.1f %3.1f), x = %3.1f --> dist = %3.1f\n", tx, ty, x, dist);
  if (win <= 0)
    return 1;
  return 2;
}


//= Generate candidate blobs for nearby people.
// builds floor projection "wproj", components "wcc", and blob list "dudes"
// blob mark: 0 = bad size, 1 = bad shape, 2 = possible
// assumes orthogonal world coordinates already found (CacheXYZ)

void jhcFollow3D::leader_blobs ()
{
  double xm, ym, len, wid, sz, ecc;
  int i, n;

  // take new depth slice near waist and find biggish things
  sf->Slice(wproj, wht - 0.5 * wsz, wht + 0.5 * wsz, wpp, 0.0, 255);
  t.BoxThresh(wproj, wproj, wsm, wth);
  t.CComps4(wcc, wproj, ROUND(wmin / wpp));
  n = dudes.FindParams(wcc);
  dudes.RemBorder(wcc, 1);

  // eliminate non-person blobs based on size and shape 
  for (i = 1; i <= n; i++)
    if (dudes.GetStatus(i) > 0)                   // all = 1
    {
      dudes.ABox(xm, ym, len, wid, wcc, i);
      sz  = len * wpp;
      ecc = len / wid;
      if ((sz < wmin) || (sz > wmax)) 
        dudes.SetStatus(i, 0);                    // 0 = bad size
      else if ((ecc >= wfat) && (ecc <= wthin))
        dudes.SetStatus(i, 2);                    // 2 = good shape
    }
}


//= Pick best person shaped blob based on tracking or initial search.
// disregards blobs beyond distance "dinit" for initial search
// assumes blob list "dudes" has been set up for current components "wcc"
// return 0 if failed, else number of winning blob

int jhcFollow3D::pick_leader (double dinit) 
{
  double fx, dx, dy, away = wnear / wpp, slope = -tan(D2R * look);
  double dmax = dinit / wpp, dstep = sqrt(slope * slope + 1.0);
  int x, y, win, w = wcc.XDim(), h = wcc.YDim();

  // see if any candidate blobs
  if (dudes.CountOver(2) <= 0)
    return 0;

  // pick blob closest to previous sighting (within reason)
  if (targ != 0)
  {
    win = dudes.Nearest(tx, ty);
    dudes.BlobCentroid(&dx, &dy, win);
    dx -= tx; 
    dy -= ty;
    if ((dx * dx + dy * dy) <= (away * away))
      return win;
    return 0;
  }

  // scan view line outwards for first good blob (only +/- 45 degs)
  fx = 0.5 * w;
  for (y = 0; y < h; y++, fx += slope, dmax -= dstep)
  {
    x = ROUND(fx);
    if ((x < 0) || (x >= w) || (dmax < 0))
      break;
    win = wcc.ARef16(x, y);
    if ((win > 0) && (dudes.GetStatus(win) >= 2))
      return win;
  }
  return 0;
}


//= Use odometry to adjust likely position of leader for tracking.
//   dx = move orthogonal to previous direction (in prev coords)
//   dy = move in previous direction of travel (in prev coords)
//   da = change in heading (body) direction (wrt previous)
// dx and dy are relative to robot wheelbase midpoint (not camera)

void jhcFollow3D::AdjLeader (double dx, double dy, double da)
{
  double r = D2R * da, c = cos(r), s = sin(r);
  double hwid = 0.5 * wproj.XDim(), x = tx - hwid - dx, y = ty - dy;

  if (targ == 0)
    return;
  tx =  x * c + y * s + hwid;
  ty = -x * s + y * c;
}


///////////////////////////////////////////////////////////////////////////
//                          Debugging Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Take full-sized input and mark tracked person in half-sized output.
// input is color, output is monochrome except for person's waist
// assumes orthogonal world coordinates already found (sf->CacheXYZ)

int jhcFollow3D::TagLeader (jhcImg& dest, const jhcImg& src)
{
  int dsk = dest.Skip(), msk = tmp.Skip(), sln2 = src.Line() << 1;
  int x, y, i, hw = dest.XDim(), hh = dest.YDim(); 
  const UC8 *s, *s0 = src.PxlSrc(), *m = tmp.PxlSrc();
  UC8 *d = dest.PxlDest();

  if (sf == NULL)
    return Fatal("Bad initialization jhcFollow3D::TagLeader");
  if (!dest.SameFormat(sf->XDim2(), sf->YDim2(), 3) || 
      !src.SameFormat(sf->XDim(), sf->YDim(), 3))
    return Fatal("Bad images to jhcFollow3D::TagLeader");

  // generate image of where target is
  if (targ <= 0)
    tmp.FillArr(0);
  else
  {
    dudes.MarkBlob(wtmp, wcc, targ, 255, 1);
    sf->MapBack(tmp, wtmp, wht - 0.5 * wsz, wht + 0.5 * wsz, wpp, 0.0);
  }

  // make red patches on small monochrome version
  for (y = 0; y < hh; y++, d += dsk, m += msk, s0 += sln2)
  {
    s = s0;
    for (x = 0; x < hw; x++, d += 3, m++, s += 6)
    {
      i = (s[0] + s[1] + s[2]) >> 2;
      d[0] = i;
      d[1] = i;
      if (*m > 128)
        d[2] = 255;
      else
        d[2] = i;
    }
  }
  return 1;
}


//= Make a nice overhead view of target and other obstacles.
// needs an image LeaderWid() x LeaderHt() x 1 as input
// leader = red (230), shape = yellow (230), size = green (128), other = blue (70) 
// assumes orthogonal world coordinates already found (CacheXYZ)
// does not do motion filtering step

int jhcFollow3D::ProjLeader (jhcImg& dest, double foff, double dinit)
{
  double rads = D2R * look, dmax = dinit / wpp;
  int mid = wproj.XDim() >> 1, h = wproj.YDim(), goal = ROUND(foff / wpp);
  int box = ROUND(6.0 / wpp), hbox = box >> 1, cr = 17;

  if (!dest.SameFormat(wproj))
    return Fatal("Bad images to jhcFollow3D::ProjLeader");

  // show various stage of filtering
  t.Threshold(dest, wproj, 0, 70);            // blue
  dudes.MarkOver(dest, wcc, 0, 128, 0);       // green
  dudes.MarkOver(dest, wcc, 1, 230, 0);       // yellow
  if (targ > 0)
    dudes.MarkBlob(dest, wcc, targ, 200, 0);  // red

  // show target location or central search ray
  if (targ == 0)
    t.DrawLine(dest, mid, 0.0, mid - dmax * sin(rads), dmax * cos(rads), 1, 255);
  else
  {
    t.RectEmpty(dest, mid - hbox, goal - hbox, box, box, 1, 215);
    t.Cross(dest, tx, ty, cr, cr, 1, 255);
  }
  return 1;
}

