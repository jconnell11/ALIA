// jhcParse3D.cpp : find head and hands of people using overhead map
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2020 IBM Corporation
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

#include "jhcParse3D.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcParse3D::jhcParse3D ()
{
  int i;

  // make body data entries
  raw = (jhcBodyData *) new jhcBodyData[rmax];

  // global and local connected components
  box.SetSize(2 * rmax);
  blob.SetSize(50);
  blob2.SetSize(50);
  nr = 0;

  // coordinate transform matrices
  m2w.SetSize(4, 4);
  w2m.SetSize(4, 4);

  // radial histograms for arm finding
  star0.SetSize(360);
  for (i = 0; i < rmax; i++)
    star[i].SetSize(360);

  // head peak histogram, no head step recording
  hist.SetSize(256);
  dbg = 0;

  // initial parameter values
  SetChest(100.0, 38.0, 1.5, 180, 25.0, 700.0, 44.0, 74.0);
  SetHead(7.0, 10.0, 4.0, 5.0, 13.0, 6.5, 2.0, 20);
  SetShoulder(10.0, 40.0, 8.0, 8.0, 1.05, 10.0, 75.0);
  SetArm(30.0, 1.5, 180, 10.0, 0, 20.0, 50.0);
  SetHand(11, 0.1, 2.0, 0.9, 12.0, 16.0, 40.0, 0.0);
  SetAim(0.0, 1.0, 15.0, 4.0, 22.0);

  // processing parameters
  Defaults();
  SetScale();
  SetView();
}


//= Default destructor does necessary cleanup.

jhcParse3D::~jhcParse3D ()
{
  delete [] raw;
}


//= Set sizes of internal map images directly.

void jhcParse3D::MapSize (int x, int y)
{
  // remember dimensions
  mw = x;
  mh = y;

  // build versions of overhead map
  floor.SetSize(x, y, 1);
  chest.SetSize(floor);
  mid.SetSize(floor);
  arm.SetSize(floor);

  // build connected component images
  cc.SetSize(x, y, 2);
  cc0.SetSize(cc);
  cc2.SetSize(cc);

  // intermediate head steps
  step.SetSize(cc, 1);
}


//= Set parameters governing input overhead map of space.
// pel values: 1 = lo height in inches, 254 = hi in inches
// overall scale of x and y are sc inches per pixel 

void jhcParse3D::SetScale (double lo, double hi, double sc)
{
  z0 = lo; 
  z1 = hi; 
  ipp = sc;
}


//= Set up to re-space Kinect data by rotating map and shifting origin.
// rotated by ang around point (w/2 0) then this point is set to (xref yref) 
// ang is usually head pan - 90 (default beam plotted along y axis, not x)

void jhcParse3D::SetView (double ang, double xref, double yref)
{
  rot = ang;
  x0 = xref;
  y0 = yref;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for person separation.

int jhcParse3D::chest_params (const char *fname)
{
  jhcParam *ps = &bps;
  int ok;

  ps->SetTag("p3d_chest", 0);
  ps->NextSpecF( &wall, "Mask out above (in)");
  ps->NextSpecF( &ch,   "Torso height cutoff (in)");  // was 36 then 30
  ps->NextSpecF( &sm,   "Smoothing scale (in)");      // was 2
  ps->NextSpec4( &sth,  "Smooth fill threshold");     // was 80
  ps->NextSpecF( &amin, "Min person area (in^2)");  
  ps->NextSpecF( &amax, "Max person area (in^2)");    // was 250 then 300 then 500

  ps->NextSpecF( &h0,   "Min head height (in)");      // was 43
  ps->NextSpecF( &h1,   "Max head height (in)");      // was 78
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for head filtering.

int jhcParse3D::head_params (const char *fname)
{
  jhcParam *ps = &hps;
  int ok;

  ps->SetTag("p3d_head", 0);
  ps->NextSpecF( &chop,   "Head slice drop (in)");      // was 5 then 6
  ps->NextSpecF( &hmin,   "Min head area (in^2)");      // was 20
  ps->NextSpecF( &hecc,   "Max head elongation");       // was 2.5 then 3 
  ps->NextSpecF( &w0,     "Min head width (in)");       // was 5.5
  ps->NextSpecF( &w1,     "Max head width (in)");  
  ps->NextSpecF( &edn,    "Eyeline from top (in)");     // was 5 then 7

  ps->NextSpecF( &margin, "Min dist from edge (in)");   // was 8
  ps->NextSpec4( &pcnt,   "Points in height peak");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for shoulder validation.

int jhcParse3D::shoulder_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("p3d_shoulder", 0);
  ps->NextSpecF( &shdn, "Shoulder slice drop (in)");       // was 12
  ps->NextSpecF( &smin, "Min shoulder area (in^2)");       // was 80 then 50
  ps->NextSpecF( &secc, "Max shoulder elongation");   
  ps->NextSpecF( &sw0,  "Min shoulder width (in)");        // was 12
  ps->NextSpecF( &wrel, "Min shoulder width wrt head");    // was 1.5
  ps->NextSpecF( &arel, "Max area wrt head");              // was 6.0

  ps->NextSpecF( &ring, "Max distance from origin (in)");  // DS was 72.0
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}

  
//= Parameters for finding and reattaching arm blobs.

int jhcParse3D::arm_params (const char *fname)
{
  jhcParam *ps = &aps;
  int ok;

  ps->SetTag("p3d_arm", 0);
  ps->NextSpecF( &alev,  "Arm height cutoff (in)");        // was 20
  ps->NextSpecF( &sm2,   "Smoothing scale (in)");          // was 2
  ps->NextSpec4( &sth2,  "Smooth fill threshold");         // was 80
  ps->NextSpecF( &arm0,  "Min arm area (in^2)");  
  ps->Skip();
  ps->NextSpec4( &ret,   "Attempt to reattach arms");

  ps->NextSpecF( &agrab, "Arm claim radius (in)");
  ps->NextSpecF( &arm1,  "Max extra arm area (in^2)");     // was 2000 pel
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for finding hands.

int jhcParse3D::hand_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("p3d_hand", 0);
  ps->NextSpec4( &ssm,   "Radial smoothing (degs)");
  ps->NextSpecF( &afall, "Arm peak falloff");
  ps->NextSpecF( &fsz,   "Fingertip region (in)");     // was 3
  ps->NextSpecF( &fpct,  "Z histogram percentile");
  ps->NextSpecF( &foff,  "Min hand XY offset (in)");   // was 16
  ps->NextSpecF( &ext0,  "Min arm 3D length (in)");    // was 18

  ps->NextSpecF( &ext1,  "Max arm 3D length (in)");    // was 32 then 36
  ps->NextSpecF( &back,  "Max mid-back shift (in)");   // was 8
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for finding pointing direction.

int jhcParse3D::finger_params (const char *fname)
{
  jhcParam *ps = &eps;
  int ok;

  ps->SetTag("p3d_aim", 0);
  ps->NextSpecF( &flen, "Hand length (in)");             // was 12
  ps->NextSpecF( &fecc, "Min elongation");               // was 1.1
  ps->NextSpecF( &flat, "Max flatness");                 // was 6
  ps->Skip();
  ps->NextSpecF( &dip,  "Reduce Z angle (deg)");         // was 10
  ps->NextSpecF( &plen, "Min point extension (in)");     // whole arm
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcParse3D::Defaults (const char *fname)
{
  int ok = 1;

  ok &= chest_params(fname);
  ok &= head_params(fname);
  ok &= shoulder_params(fname);
  ok &= arm_params(fname);
  ok &= hand_params(fname);
  ok &= finger_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcParse3D::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= bps.SaveVals(fname);
  ok &= hps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= aps.SaveVals(fname);
  ok &= gps.SaveVals(fname);
  ok &= eps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Find heads, hands, and point directions given an overhead map.
// input from jhcSurface3D::FloorMap with known inches per pixel (ipp)
// also needs height for pel = 1 (z0) and height for pel = 254 (z1)
// generates world coordinates such that middle of bottom = (xmid, ybot)
// takes about 3.6ms on (624 576) x 0.5", 1.8ms on (446 411) x 0.7"
// returns number of people detected

int jhcParse3D::FindPeople (const jhcImg& map)
{
  // check arguments then 
  if (!map.SameFormat(mw, mh, 1) || (z1 <= z0) || (ipp <= 0.0))
    return Fatal("Bad input to jhcParse3D::FindPeople");

  // build coordinate transform matrix (just in XY plane)
  m2w.Translation(-0.5 * mw, 0.0, 0.0);
  m2w.RotateZ(rot);                       // rotation around map center
  m2w.Translate(0.5 * mw, 0.0, 0.0);
  m2w.Magnify(ipp, ipp, 1.0);
  m2w.Translate(-x0, -y0, 0.0);           // global reference frame

  // get inverse transform for graphics
  w2m.Invert(m2w);

  // remove very tall objects (walls) then parse overhead human forms
  ZeroOver(floor, map, ht2pel(wall));
  nr = find_heads(floor);
  find_arms(floor, nr);
  return nr;
}


//= Determine if a real world point (x y) is inside a viable person candidate blob.
// useful for preserving tracks when person scratching head, etc.

bool jhcParse3D::PersonBlob (const jhcMatrix& probe) const
{
  jhcMatrix pos(4);
  double h;
  int ix, iy, bnum = -1;

  // find pixel location in map
  if (!probe.Vector(4))
    return false;
  pos.MatVec(w2m, probe);
  ix = ROUND(pos.X());
  iy = ROUND(pos.Y());

  // test for valid blob with decent height
  if (cc.InBounds(ix, iy) > 0)
    bnum = cc.ARefChk(ix, iy);
  if ((bnum <= 0) || (box.GetStatus(bnum) <= 0))
    return false;
  h = box.GetAux(bnum);
  return((h >= h0) && (h <= h1));
}


//= Find the arm blob number at some particular real-world coordinates.
// useful for checking against "bnum" and "alt" fields of jhcBodyData
// returns blob number normally, 0 for no blob, -1 for outside region

int jhcParse3D::BlobAt (double wx, double wy) const
{
  jhcMatrix pos(4), probe(4);
  int ix, iy;

  probe.SetVec3(wx, wy, 0.0);
  pos.MatVec(w2m, probe);
  ix = ROUND(pos.X());
  iy = ROUND(pos.Y());
  if (cc2.InBounds(ix, iy) <= 0)
    return -1;
  return cc2.ARef16(ix, iy);
}


///////////////////////////////////////////////////////////////////////////
//                              Head Finding                             //
///////////////////////////////////////////////////////////////////////////

//= Finds head given overhead map with walls suppressed.
// global "cc" holds person blobs and "box" holds some analysis of them
// returns number of people detected, "raw" holds details

int jhcParse3D::find_heads (const jhcImg& ohd)
{
  jhcRoi area;
  double h;
  int zval = ht2pel(ch), ism = ROUND(sm / ipp) | 0x01;
  int i, j, k, nc, bv = 40, n = 0;

  // cut overhead map at chest height to separate people
  Threshold(chest, ohd, __max(0, zval));
  BoxAvg(chest, chest, ism);
  CComps4(cc, chest, ROUND(amin / (ipp * ipp)), sth);

  // throw out anything way too big to be a person
  // potential head blobs have status 1, all others are 0
  box.FindBBox(cc);
  box.PixelThresh(-ROUND(amax / (ipp * ipp)));
  if (dbg > 0)
    box.ThreshValid(step, cc, 1, bv);

  // find best head for each potential person component
  nc = box.Active();
  for (i = 1; i < nc; i++)
    if (box.GetStatus(i) > 0)
    {
      // get height of initial head candidate 
      box.GetRoi(area, i);
      h = find_max(ohd, cc, i, area);
      box.SetAux(i, h);                                      // save for tracker
      if ((h < h0) || (h > h1))
        continue;

      // test for proper head size and shape
      area.GrowRoi(ism, ism);
      area.RoiClip(ohd);                                     // added 6/17
      if ((j = chk_head(n, h, ohd, cc, i, area)) < 0)
        continue;

      // make sure not touching beam edges then check for shoulders underneath
      if (visible(raw[n], margin) <= 0)
        continue;
      if ((ring > 0.0) && (raw[n].PlaneVec3() > ring))       // Dataspace reflections
        continue;
      if ((k = chk_shoulder(n, blob.BlobLength(j), blob.BlobArea(j), ohd, cc, i, area)) < 0)
        continue;

      // determine star center: blob[j] = head ellipse, blob2[k] = shoulder ellipse
      mid_back(stx[n], sty[n], j, k);
      if (++n >= rmax)       
        break;                 // already recorded, but stop if end of list
    }
  return n;
}


//= Checks shape of potential head denoted by component of given label.
// binds head's center in world coordinates and first non-zero pixel in map 
// returns index of head in new blob array, -1 if fails some test

int jhcParse3D::chk_head (int n, double h, const jhcImg& view, 
                          const jhcImg& comp, int i, const jhcRoi& area) 
{
  jhcMatrix pos(4);
  jhcRoi area2;
  double xc, yc, h2;
  int j, hv = 128;

  // re-slice overhead map at presumed eye level to find heads
  thresh_within(mid, view, ht2pel(h - chop), comp, i, area);
  BoxAvg(mid, mid, ROUND(sm / ipp) | 0x01);
  if (dbg > 0)
  {
    UnderGate(step, step, mid, sth, hv);  
    step.MaxRoi();
  }
  if (CComps4(cc0, mid, ROUND(hmin / (ipp * ipp)), sth) <= 0)
    return -1;

  // keep only blobs with reasonable shape and size to be heads
  blob.FindParams(cc0);
  blob.AspectThresh(-hecc);
  blob.LengthThresh( w0 / ipp); 
  blob.LengthThresh(-w1 / ipp); 

  // find most likely head blob and get height again (if multiple)
  if ((j = blob.Nearest(area.RoiAvgX(), area.RoiAvgY())) <= 0)
    return -1;
  blob.GetRoi(area2, j);
  if ((h2 = find_max(view, cc0, j, area2)) < h0)
    return -1;

  // convert image coordinates to world coordinates and store
  blob.BlobCentroid(&xc, &yc, j);
  pos.SetVec3(xc, yc, h2 - edn);
  raw[n].MatVec(m2w, pos);
  raw[n].id = n + 1;

  // find good pixel for linking blobs then return chosen blob number
  first_nz(xlink[n], ylink[n], view, cc0, j, area2);
  return j;
}


//= Given a potential head make sure it is supported by something like shoulders.
// returns shoulder blob number if seems reasonable, -1 if fails some test

int jhcParse3D::chk_shoulder (int n, double w, double a, const jhcImg& view, 
                              const jhcImg& comp, int i, const jhcRoi& area) 
{
  int j, sv = 50, ccth = 45, bv = 40;

  // re-slice overhead map at presumed shoulder level
  thresh_within(mid, view, ht2pel(raw[n].Z() - shdn), comp, i, area);
  BoxThresh(mid, mid, ROUND(sm / ipp) | 0x01, sth, sv, bv);
  if (dbg > 0)
  {
    SubstKey(step, mid, step, bv);
    step.MaxRoi();
  }
  if (CComps4(cc0, mid, ROUND(smin / (ipp * ipp)), ccth) <= 0)
    return -1;

  // test component attached to head for reasonable shape and width
  blob2.FindParams(cc0);
  j = cc0.ARef(xlink[n], ylink[n]);
  if ((blob2.BlobAspect(j) > secc) || 
      (blob2.BlobLength(j) < (sw0 / ipp)) ||
      (blob2.BlobLength(j) < (wrel * w)) ||
      (blob2.BlobArea(j)   > (arel * a)))
    return -1; 
  return j;
}


//= Find maximum value inside some component given its bounding box.
// used to find single max, now uses histogram for noise robustness
// returns height above floor in inches (not pixel value)

double jhcParse3D::find_max (const jhcImg& val, const jhcImg& comp, int i, const jhcRoi& area) 
{
  int vsk = val.RoiSkip(area), csk = comp.RoiSkip(area) >> 1;
  int x, y, rw = area.RoiW(), rh = area.RoiH();
  const US16 *c = (const US16 *) comp.RoiSrc(area);
  const UC8 *v = val.RoiSrc(area);

  hist.Fill(0);
  for (y = rh; y > 0; y--, v += vsk, c += csk)
    for (x = rw; x > 0; x--, v++, c++)
      if (*c == i)
        hist.AInc(*v, 1);
  return pel2ht(hist.MaxBinN(pcnt));
}


//= Find values in area that are at or above value and also part of given component.
// sets ROI of result to match area (for subsequent processing)

void jhcParse3D::thresh_within (jhcImg& dest, const jhcImg& src, int th, 
                                const jhcImg& comp, int i, const jhcRoi& area) const
{
  int x, y, rw = area.RoiW(), rh = area.RoiH();
  int sk = src.RoiSkip(area), csk = comp.RoiSkip(area) >> 1;
  const US16 *c = (const US16 *) comp.RoiSrc(area);
  const UC8 *s = src.RoiSrc(area);
  UC8 *d = dest.RoiDest(area);

  dest.CopyRoi(area);
  for (y = rh; y > 0; y--, d += sk, s += sk, c += csk)
    for (x = rw; x > 0; x--, d++, s++, c++)
      if ((*c == i) && (*s >= th))
        *d = 255;
      else
        *d = 0;
}


//= Find first non-zero value in source that is part of component i.
// search zone restricted to given area

void jhcParse3D::first_nz (int& x, int& y, const jhcImg& src, 
                           const jhcImg& comp, int i, const jhcRoi& area) const
{
  int x0 = area.RoiX(), x1 = area.RoiX2(), y0 = area.RoiY(), y1 = area.RoiY2();
  int ssk = src.RoiSkip(area), csk = comp.RoiSkip(area) >> 1;
  const UC8 *s = src.RoiSrc(area);
  const US16 *c = (const US16 *) comp.RoiSrc(area);

  for (y = y0; y < y1; y++, s += ssk, c += csk)
    for (x = x0; x < x1; x++, s++, c++)
      if ((*s > 0) && (*c == i))
        return;
}


//= Convert a height to pixel value in map.
// similar to jhcOverhead3D::DI2Z 

int jhcParse3D::ht2pel (double ht) const
{
  return(1 + ROUND(252.0 * (ht - z0) / (z1 - z0)));
}


//= Convert a pixel value from map into a height.
// similar to jhcOverhead3D::DZ2I

double jhcParse3D::pel2ht (int pel) const
{
  return(z0 + (pel - 1) * (z1 - z0) / 252.0);
}


//= Determine middle of back for finding arms as radial extensions.
// blob[hd] should be head ellipse while blob2[sh] is shoulder ellipse
// old version used blob.BlobCentroid(&hx, &hy, hd) as center

void jhcParse3D::mid_back (int& cx, int& cy, int hd, int sh) const
{
  double hx, hy, ha, sx, sy, sa, mx, my, dx, dy, len, wlen, f;

  // get center of head blob and shoulder-plus-head blob
  blob.BlobCentroid(&hx, &hy, hd);
  blob2.BlobCentroid(&sx, &sy, sh);

  // adjust to get center of shoulder-only blob
  ha = (double) blob.BlobArea(hd);
  sa = (double) blob2.BlobArea(sh);
  mx = (sa * sx - ha * hx) / (sa - ha);
  my = (sa * sy - ha * hy) / (sa - ha);

  // find shift of head wrt shoulders-only
  dx = mx - hx;
  dy = my - hy;
  len = sqrt(dx * dx + dy * dy);
  wlen = ipp * len;

  // decide whether to move and by how much
  if (wlen > back)
  {
    // keep center of head as reference
    cx = ROUND(hx);
    cy = ROUND(hy);
  }
  else if (wlen < 1.0)
  {
    // head square on shoulders, no obvious direction to shift
    cx = ROUND(mx);
    cy = ROUND(my);
  }
  else
  {
    // shift by width (of combined) in opposite direction from head
    f = 0.5 * blob2.BlobWidth(sh) / len;
    cx = ROUND(mx + f * dx);
    cy = ROUND(my + f * dy);
  }
}


///////////////////////////////////////////////////////////////////////////
//                              Hand Finding                             //
///////////////////////////////////////////////////////////////////////////

//= Given valid head detections try to find ends of associated arms.
// reads global "raw" array to get head candidates, writes to store hand detections
// largely copied from jhcScreenPos class in Muriel project

void jhcParse3D::find_arms (const jhcImg& ohd, int nh) 
{
  jhcRoi body, tip;
  jhcMatrix pos(4);
  jhcBodyData *item;
  int zval = ht2pel(alev), ism2 = ROUND(sm2 / ipp) | 0x01;
  int i, side, bnum, hx, hy, ix, iy, iz, pk, alt;

  // chop person pillars lower than chest separation level
  Threshold(arm, ohd, __max(0, zval));
  BoxAvg(arm, arm, ism2);
  CComps4(cc2, arm, ROUND(arm0 / (ipp * ipp)), sth2);  // arm0 not needed below
  box.FindBBox(cc2);

  for (i = 0; i < nh; i++)
  {
    // get new blob number associated with head
    item = raw + i;
    pos.MatVec(w2m, *item);
    bnum = cc2.ARef16(ROUND(pos.X()), ROUND(pos.Y()));
    item->bnum = bnum;

    // assume no arms 
    item->hok[0] = 0;
    item->hok[1] = 0;

    // find candidates based on radial plot
    hx = stx[i];
    hy = sty[i];
    alt = arm_peaks(cc2, box, hx, hy, bnum, i);
    item->alt = alt;

    // get combined blob and alternate blob search area
    box.GetRoi(body, bnum);
    if (alt > 0)
      body.AbsorbRoi(*(box.ReadRoi(alt)));

    // look for left and right arms
    for (side = 0; side <= 1; side++)
    {
      // see if candidate arm detected
      pk = ((side > 0) ? rpk[i] : lpk[i]);
      if (pk < 0)
        continue;

      // figure out fingertip location and height
      finger_area(tip, hx, hy, star[i], pk);
      tip.MergeRoi(body);
      iz = finger_loc(ix, iy, hx, hy, ohd, cc2, bnum, alt, tip);

      // check for reasonable arm length then find pointing direction
      if (arm_coords(item, ix, iy, iz, hx, hy, side) <= 0.0)
        continue;
      if (est_ray(item, side, ohd, ix, iy, iz, cc2, bnum, alt, body) > 0)
        item->hok[side] = 1;
    }

    // fix order of arms if needed
    swap_arms(i);
  }
}


//= Locate potential hand peaks in radial histogram.
// decodes left and right hand indices into globals "lpk[i]" and "rpk[i]"
// writes smoothed radial histogram to global "star[i]"
// peaks are reversed and shifted by -90 degs for nice screen plots
// returns blob number of any reattached arm used

int jhcParse3D::arm_peaks (const jhcImg& comp, jhcBBox& b, int hx, int hy, int bnum, int i) 
{
  jhcArr *plot = star + i;
  int lo, hi, last = star0.Last(), alt = -1;

  // build radial histogram of person blob 
  star0.Fill(0);
  radial_plot(star0, hx, hy, comp, b, bnum);

  // possibly add in detached arm then smooth
  if (ret > 0)
    if ((alt = grab_arm(hx, hy, comp, b, bnum)) > 0)
    {
      b.SetStatus(alt, 2);                             // blob cannot be used twice
      radial_plot(star0, hx, hy, comp, b, alt);
    }
  plot->Boxcar(star0, ssm);

  // find biggest overall peak (assume this is the right hand)
  // then look for secondary peak outside slopes of main peak
  lpk[i] = -1;
  if ((rpk[i] = plot->TrueMax(0, last, 1)) >= 0)
    if (plot->CycBounds(lo, hi, rpk[i], afall) > 0)
      lpk[i] = plot->TrueMax(hi, lo);
  return alt;
}


//= Generate maximum distance at each angle for all pixels in a blob relative to head center.
// overwrites values in plot if new, farther point found (= distance * 100)

void jhcParse3D::radial_plot (jhcArr& plot, int hx, int hy, const jhcImg& comp, const jhcBBox& b, int i) const
{
  jhcRoi area;
  const US16 *s;
  double dist, ang, sc = plot.Size() / 360.0;
  int n, x, y, x0, x1, y0, y1, dx, dy, dy2, skip;

  // get search area for component
  b.GetRoi(area, i);
  x0 = area.RoiX();
  x1 = area.RoiX2();
  y0 = area.RoiY();
  y1 = area.RoiY2();
  s = (const US16 *) comp.RoiSrc(area);
  skip = comp.RoiSkip(area) >> 1;

  // scan for blob pixels
  for (y = y0; y < y1; y++, s += skip)
  {
    dy = y - hy;
    dy2 = dy * dy;
    for (x = x0; x < x1; x++, s++)
      if (*s == i)
      {
        // compute distance and angle from head center
        // measured CW from -90, screen is in middle
        dx = x - hx;
        dist = sqrt((double)(dx * dx + dy2));
        ang = -90.0 - R2D * atan2(dy, (double) dx);   // offset for screen
        if (ang < 0.0)
          ang += 360.0;

        // add point to plot 
        n = ROUND(sc * ang);
        if (n >= 360)
          n -= 360;
        plot.AMax(n, ROUND(100.0 * dist));
      }
  }
}


//= Append biggest unclaimed blob nearby (possibly separated arm).
// scans a fixed size box around head location
// returns component number or 0 if nothing suitable

int jhcParse3D::grab_arm (int hx, int hy, const jhcImg& comp, const jhcBBox& b, int i) const
{
  jhcRoi area;
  const US16 *s;
  double ppi2 = 1.0 / (ipp * ipp);
  int r = ROUND(agrab / ipp), a0 = ROUND(ppi2 * arm0);
  int a1 = ROUND(ppi2 * arm1), d = 2 * r + 1, r2 = r * r;
  int x0, x1, y0, y1, skip, x, y, dx, dy, dy2, pels, win = -1, best = 0; 

  // get search area around head
  area.CenterRoi(hx, hy, d, d);
  area.RoiClip(comp);
  x0 = area.RoiX();
  x1 = area.RoiX2();
  y0 = area.RoiY();
  y1 = area.RoiY2();
  s = (const US16 *) comp.RoiSrc(area);
  skip = comp.RoiSkip(area) >> 1;

  // scan for unallocated components
  for (y = y0; y < y1; y++, s += skip)
  {
    dy = y - hy;
    dy2 = dy * dy;    
    for (x = x0; x < x1; x++, s++)
      if ((*s != 0) && (*s != i) && 
          (*s != win) && (b.GetStatus(*s) < 2))    // unclaimed, not head (3) or arm (2)
      {
        // see if inside circle
        dx = x - hx;
        if ((dx * dx + dy2) > r2)
          continue;

        // see if reasonable size and bigger than previous winner (if any)
        pels = b.PixelCnt(*s);
        if ((pels <= best) || (pels < a0) || (pels > a1))
          continue;

        // save as best candidate
        win = *s;
        best = pels;
      }
  }
  return win;
}


//= Get region near end of hand based on peak in radial plot.
// peaks are reversed and shifted by -90 degrees for nice screen plots

void jhcParse3D::finger_area (jhcRoi& tip, int hx, int hy, const jhcArr& plot, int pk) const
{
  double rads = D2R * (-90.0 - pk), dist = 0.01 * plot.ARef(pk);
  int t = ROUND(fsz / ipp) | 0x01, tsz = __max(t, 3);

  tip.CenterRoi(ROUND(hx + dist * cos(rads)), ROUND(hy + dist * sin(rads)), tsz, tsz);
}


//= Find the blob point within the search region farthest from reference point (hx hy).
// takes base component number (j) plus any attached arm (alt)
// binds x and y of farthest point, histograms z of nearby points
// returns best guess of z in pixel values (not inches)

int jhcParse3D::finger_loc (int& ix, int& iy, int hx, int hy, const jhcImg& map, 
                            const jhcImg& comp, int bnum, int alt, jhcRoi& area) const
{
  jhcArr hist(256);
  int x0 = area.RoiX(), y0 = area.RoiY(), x2 = area.RoiX2(), y2 = area.RoiY2();
  int msk = map.RoiSkip(area), csk = comp.RoiSkip(area) >> 1;
  const US16 *c = (const US16 *) comp.RoiSrc(area);
  const UC8 *m = map.RoiSrc(area);
  int x, y, dx, dy, d2, best = 0;

  // scan all pixels in designated hand region
  hist.Fill(0);
  for (y = y0; y < y2; y++, m += msk, c += csk)
    for (x = x0; x < x2; x++, c++, m++)
      if ((*m > 0) && ((*c == bnum) || (*c == alt)))
      {
        // add z to histogram 
        hist.AInc(*m, 1);

        // find distance from reference
        dx = x - hx;
        dy = y - hy;
        d2 = dx * dx + dy * dy;
        if (d2 > best)
        {
          // save coordinates of best point
          best = d2;
          ix = x;
          iy = y;
        }
      }

  // if nothing found, try again with a bigger box
  if (best > 0)
    return hist.Percentile(fpct);
  area.GrowRoi(2, 2);
  area.RoiClip(map);
  return finger_loc(ix, iy, hx, hy, map, comp, bnum, alt, area);
}


//= Convert image to world coordinates then save if hand passes geometric checks.
// real world coordinate (in inches) stored in given item and valid flag set
// returns hand to shoulder XY offset distance squared, 0 if fails tests

double jhcParse3D::arm_coords (jhcBodyData *item, int ix, int iy, int iz, int mx, int my, int side) const
{
  jhcMatrix diff(4), fi(4), mid(4);
  jhcMatrix *hand = &(item->hoff[side]);
  double dist2, len2;

  // convert finger pixel location into physical coordinates 
  diff.SetVec3(ix, iy, pel2ht(iz));
  fi.MatVec(m2w, diff); 
  hand->DiffVec3(fi, *item);                         // store wrt head

  // convert mid-back point into physical coords
  diff.SetVec3(mx, my, item->Z() - shdn);
  mid.MatVec(m2w, diff); 

  // get 2D and 3D extension relative to mid-back (also in physical coordinates)
  diff.DiffVec3(fi, mid);
  len2 = diff.Len2Vec3();
  dist2 = len2 - (diff.Z() * diff.Z());

  // see if planar distance enough, then check for proper arm length
  if (dist2 < (foff * foff))
    return 0.0;
  if ((len2 < (ext0 * ext0)) || (len2 > (ext1 * ext1)))
    return 0.0;
  return dist2;
}


//= Swap the left and right hands if angle > 180 degrees.
// writes globals "rpk", "lpk", and "raw"
// returns 1 if swapped values 

int jhcParse3D::swap_arms (int i) 
{ 
  jhcMatrix tmp(4);
  jhcBodyData *item = raw + i;
  int diff, val, sz = star0.Size(), hsz = sz >> 1;

  // make sure two valid arms found
  if ((item->hok[0] <= 0) || (item->hok[1] <= 0))
    return 0;

  // check that arms are at an acute angle using peaks
  diff = rpk[i] - lpk[i];
  if (diff < -hsz)
    diff += sz;      
  else if (diff > hsz) 
    diff -= sz;
  if (diff >= 0)
    return 0;

  // swap peaks (for display)
  val = lpk[i];
  lpk[i] = rpk[i];
  rpk[i] = val;

  // swap hand positions
  tmp.Copy(item->hoff[0]);
  (item->hoff[0]).Copy(item->hoff[1]);
  (item->hoff[1]).Copy(tmp);

  // swap pointing directions
  tmp.Copy(item->hdir[0]);
  (item->hdir[0]).Copy(item->hdir[1]);
  (item->hdir[1]).Copy(tmp);
  return 1;
} 


///////////////////////////////////////////////////////////////////////////
//                           Pointing Direction                          //
///////////////////////////////////////////////////////////////////////////

//= Estimate user pointing direction by finding axis of hand out to wrist.
// if flen <= 0 just gives unit vector from head to hand (normalized hand offset)
// returns 1 if seems valid, 0 if likely not a real hand

int jhcParse3D::est_ray (jhcBodyData *item, int side, const jhcImg& map, int ix, int iy, int iz,
                         const jhcImg& comp, int bnum, int alt, const jhcRoi& body) 
{
  double s[9];
  jhcRoi end;
  jhcMatrix fix(4, 4);
  jhcMatrix *hand = &(item->hoff[side]), *dir = &(item->hdir[side]);
  double ax, ay, az, r, zang;
  int sz = 2 * ROUND(flen / ipp) + 1, pts = 20;

  // mark as unstable if extension (head to hand) is too little
  if (hand->PlaneVec3() < plen)
    item->stable[side] = -1;

  // check for simpler head-to-hand case
  if (flen <= 0.0)
  {
    dir->Copy(item->hoff[side]);
    dir->UnitVec3();
    return 1;
  }

  // collect statistics around presumed point finger
  end.CenterRoi(ix, iy, sz, sz);
  end.MergeRoi(body);
  if (area_stats(s, map, ix, iy, iz, comp, bnum, alt, end) < pts)
    return 0;

  // if valid finger, generate a pointing direction vector
  if (find_axis(ax, ay, az, s) <= 0)
    return 0;
  dir->SetVec3(ax, ay, az);

  // rotate for map then orient in same direction as hand wrt head
  fix.RotationZ(rot);
  dir->MatVec(fix, *dir);
  if (dir->DotVec3(*hand) < 0.0)
    dir->ScaleVec3(-1.0);

  // fudge pointing angle downward then normalize
  r = dir->PlaneVec3();
  zang = atan2(dir->Z(), r) - D2R * dip;
  dir->SetZ(r * tan(zang));
  dir->UnitVec3();
  return 1;
}


//= Collect statistics on all 3D points within certain distance of hand point.
// s[] generally in map pixels (e.g. 0.15" quanta, even for z)
// return number of points used, fills in array "s" with expectations

int jhcParse3D::area_stats (double s[], const jhcImg& map, int ix, int iy, int iz,
                            const jhcImg& comp, int bnum, int alt, const jhcRoi& area) 
{
  UL32 sum[9];
  double ipp2 = ipp * ipp, z2p = (z1 - z0) / (252.0 * ipp), den = 0.0;                
  int max2 = ROUND(flen * flen / ipp2), rw = area.RoiW(), rh = area.RoiH();
  int msk = map.RoiSkip(area), csk = comp.RoiSkip(area) >> 1;
  int fx = ix - area.RoiX(), fy = iy - area.RoiY(), fz = ROUND((iz - 1) * z2p);
  int i, x, y, z, dx, dy, dy2, dz, n = 0;
  const US16 *c = (const US16 *) comp.RoiSrc(area);
  const UC8 *m = map.RoiSrc(area);
  UC8 *st = step.RoiDest(area);

  // clear statistics
  for (i = 0; i < 9; i++)
    sum[i] = 0;

  // find all pixels in area belonging to blobs associated with the person
  for (y = 0; y < rh; y++, m += msk, c += csk, st += msk)
  {
    dy = y - fy;
    dy2 = dy * dy;
    for (x = 0; x < rw; x++, m++, c++, st++)
      if ((*m > 0) && ((*c == bnum) || (*c == alt)))
      {
        // test squared distance from fingertips (in pixels)
        z = ROUND((*m - 1) * z2p);
        dx = x - fx;
        dz = z - fz;
        if ((dx * dx + dy2 + dz * dz) > max2)
          continue;

        // possibly mark as yellow in debugging image
        if (dbg > 0)
          *st = 230;

        // add to statistical sums (in pixels)
        sum[0] += x;       // sx
        sum[1] += y;       // sy
        sum[2] += z;       // sz
        sum[3] += x * x;   // sxy
        sum[4] += x * y;   // sxy
        sum[5] += x * z;   // sxz
        sum[6] += y * y;   // syy
        sum[7] += y * z;   // syz
        sum[8] += z * z;   // szz
        n++;
      }
  }

  // find axis of least inertia based on expected values
  if (n > 0)
    den = 1.0 / n;
  for (i = 0; i < 9; i++)
    s[i] = sum[i] * den;
  return n;
}


//= Find first principle component axis from point cloud statistics.
// solved using trigonometric method (cf. Wikipedia "Eigenvalue algorithm")
// <pre>
//          0   1   2   3   4   5   6   7   8
//   s[] = {ex  ey  ez  exx exy exz eyy eyz ezz}
// </pre>
// binds (ax ay az) vector along axis, but not normalized or correct sign
// returns 1 if likely hand, 0 if bad shape

int jhcParse3D::find_axis (double& ax, double& ay, double& az, const double s[]) const
{
  jhcMatrix a(3, 3), b(3, 3), c(3, 3);
  double a11 = s[3] - s[0] * s[0], a12 = s[4] - s[0] * s[1], a13 = s[5] - s[0] * s[2];
  double a22 = s[6] - s[1] * s[1], a23 = s[7] - s[1] * s[2], a33 = s[8] - s[2] * s[2];  
  double p1, q, a11q, a22q, a33q, p2, p, r, phi, ev1, ev2, ev3;

  // load values into symmetric covariance matrix
  a.MSet(0, 0, a11);
  a.MSet(1, 0, a12);
  a.MSet(2, 0, a13);
  a.MSet(0, 1, a12);
  a.MSet(1, 1, a22);
  a.MSet(2, 1, a23);
  a.MSet(0, 2, a13);
  a.MSet(1, 2, a23);
  a.MSet(2, 2, a33);

  // use trigonometry to solve cubic characteristic equation
  p1 = a12 * a12 + a13 * a13 + a23 * a23;
  q = (a11 + a22 + a33) / 3.0;
  a11q = a11 - q;
  a22q = a22 - q;
  a33q = a33 - q;
  p2 = a11q * a11q + a22q * a22q + a33q * a33q + 2.0 * p1;
  p = sqrt(p2 / 6.0);

  // generate B = (1 / p) * (A - q * I)  
  c.Zero();                  // C = -q * I
  c.MSet(0, 0, -q); 
  c.MSet(1, 1, -q); 
  c.MSet(2, 2, -q); 
  b.Copy(a);                 // B = (A - C) / p
  b.Add(c);
  b.Scale(1.0 / p);
  r = 0.5 * b.Det();

  // compute phi being careful of underflow and overflow
  if (r <= -1.0) 
    phi = PI / 3.0;
  else if (r >= 1.0)
    phi = 0.0;
  else
    phi = acos(r) / 3.0;

  // get all eigenvectors (ev1 is guaranteed largest) 
  // then test for sutiable finger-like elongations
  ev1 = q + 2.0 * p * cos(phi);
  ev3 = q + 2.0 * p * cos(phi + (2.0 * PI) / 3.0);
  ev2 = 3.0 * q - ev1 - ev3;    
  if ((ev1 < (fecc * fecc * ev2)) || (ev1 > (flat * flat * ev3)))
    return 0;

  // C = (A - ev2 * I)(A - ev3 * I) for ordinary eigenspace
  b.Copy(a);                 // B = A - ev2 * I 
  b.MInc(0, 0, -ev2);
  b.MInc(1, 1, -ev2);
  b.MInc(2, 2, -ev2);
  a.MInc(0, 0, -ev3);        // A = A - ev3 * I
  a.MInc(1, 1, -ev3);
  a.MInc(2, 2, -ev3);
  c.MatMat(b, a);            // C = B A

  // extract eigenvector for ev1 (any column)
  ax = c.MRef(0, 0);
  ay = c.MRef(0, 1);
  az = c.MRef(0, 2);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Shows overhead map with very tall objects suppressed.

int jhcParse3D::NoWalls (jhcImg& dest) const
{
  return dest.CopyArr(floor);
}


//= Shows overhead map above chest height with very tall objects suppressed.

int jhcParse3D::ChestMap (jhcImg& dest) const
{
  return Squelch(dest, floor, __max(0, ht2pel(ch)));
}


//= Shows components at first level of person/head finding.

int jhcParse3D::ChestBlobs (jhcImg& dest) const
{
  return Scramble(dest, cc);
}


//= Shows candidate head regions (green) and shoulders (yellow) in blobs (purple).
// can be useful for adjusting slice and shape thresholds (must set dbg > 0 first)

int jhcParse3D::HeadLevels (jhcImg& dest) const
{
  if (dbg <= 0)
    return dest.FillArr(1);
  return dest.CopyArr(step);
}


//= Shows overhead map above arm height with very tall objects suppressed.

int jhcParse3D::ArmMap (jhcImg& dest) const
{
  return Squelch(dest, floor, ht2pel(alev));
}


//= Shows components at second level of arm finding.

int jhcParse3D::ArmBlobs (jhcImg& dest) const
{
  return Scramble(dest, cc2);
}


//= Show numbered heads in white and arm claim blob radius in green.

int jhcParse3D::ArmClaim (jhcImg& dest)
{
  jhcRoi box;
  jhcMatrix pos(4);
  int i, hx, hy, cr = 17;

  if (!dest.SameFormat(mw, mh, 1))
    return Fatal("Bad input to jhcParse3D::ArmClaim");

  for (i = 0; i < nr; i++)
  {
    // find head position in image
    pos.MatVec(w2m, raw[i]);
    hx = ROUND(pos.X());
    hy = ROUND(pos.Y());
    box.CenterRoi(hx, hy, cr, cr);

    // draw elements
    Cross(dest, hx, hy, cr, cr, 1);
    LabelBox(dest, box, i + 1, 16);
    CircleEmpty(dest, hx, hy, agrab / ipp, 1, -2);
  }
  return 1;
}


//= Show mid-back point as white cross and arm extension threshold in green.

int jhcParse3D::ArmExtend (jhcImg& dest)
{
  jhcRoi box;
  int i, cr = 17;

  if (!dest.SameFormat(mw, mh, 1))
    return Fatal("Bad input to jhcParse3D::ArmExtend");

  for (i = 0; i < nr; i++)
  {
    Cross(dest, stx[i], sty[i], cr, cr, 1);
    CircleEmpty(dest, stx[i], sty[i], foff / ipp, 1, -2);
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          List-based Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Overlays outlines of heads (white boxes) with numbers on map-sized image.
// can work with inverted (upside down) image
// size of boxes given by sz (in inches), negative picks color based on ID
// style: 0 = number, 1 = person-N, 2 = Jon, 3 = Jon Connell
// if col < 0 then draws with thinner line 

int jhcParse3D::MarkHeads (jhcImg& dest, jhcBodyData *items, int n, 
                           int invert, double sz, int style, int col)
{
  jhcRoi box;
  jhcMatrix pos(4);
  int hsz = ROUND(fabs(sz) / ipp), c = abs(col);
  int i, id, th = ((col >= 0) ? 3 : 1), fsz = ((col >= 0) ? -16 : 16);

  if (!dest.SameFormat(mw, mh, 1))
    return Fatal("Bad input to jhcParse3D::MarkHeads");

  for (i = 0; i < n; i++)
    if ((id = items[i].id) > 0)
    {
      // determine outline position
      pos.MatVec(w2m, items[i]);
      box.CenterRoi(ROUND(pos.X()), ROUND(pos.Y()), hsz, hsz);
      if (invert > 0)
        box.InvertRoi(dest.XDim(), dest.YDim());

      // get drawing color and label string
      if (sz < 0.0)
        c = (id % 6) + 1;
      RectEmpty(dest, box, th, -c);
      LabelBox(dest, box, label(items[i], style), fsz, -c);
    }
  return 1;
}


//= Generate text label for given head in some particular style.
// style: 0 = number, 1 = person-N, 2 = Jon, 3 = Jon Connell

const char *jhcParse3D::label (const jhcBodyData& guy, int style)
{
  char *end;

  if ((style <= 0) || ((style == 2) && (*(guy.tag) == '\0')))
    _itoa_s(guy.id, tmp, 10);
  else if ((style == 1) || ((style >= 3) && (*(guy.tag) == '\0')))
    sprintf_s(tmp, "person-%d", guy.id);
  else 
  {
    // get full tag, possibly cutting off last name
    strcpy_s(tmp, guy.tag);
    if (style == 2)
      if ((end = strchr(tmp, ' ')) != NULL)
        *end = '\0';
  }
  return tmp;
}


//= Overlays outlines of heads (white boxes) on map-sized image.
// size of boxes given by sz (in inches), negative picks color based on ID
// col picks color (B:G:R), except negative col draws with thinner line
// can work with inverted (upside down) image

int jhcParse3D::ShowHeads (jhcImg& dest, const jhcBodyData *items, int n, int invert, double sz, int col) const
{
  jhcRoi box;
  jhcMatrix pos(4);
  int i, id, c = abs(col), th = ((col >= 0) ? 3 : 1), hsz = ROUND(fabs(sz) / ipp);

  if (!dest.SameFormat(mw, mh, 1))
    return Fatal("Bad input to jhcParse3D::ShowHeads");

  for (i = 0; i < n; i++)
    if ((id = items[i].id) > 0)
    {
      pos.MatVec(w2m, items[i]);
      box.CenterRoi(ROUND(pos.X()), ROUND(pos.Y()), hsz, hsz);
      if (invert > 0)
        box.InvertRoi(dest.XDim(), dest.YDim());
      if (sz < 0.0)
        c = (id % 6) + 1;
      RectEmpty(dest, box, th, -c);
    }
  return 1;
}


//= Overlays outlines of hands (blue crosses) on map-size image.
// if col < 0 then draws with thinner line
// can work with inverted (upside down) image

int jhcParse3D::ShowHands (jhcImg& dest, const jhcBodyData *items, int n, int invert, int col) const
{
  jhcMatrix full(4), pos(4), lims(4);
  int i, c = abs(col), th = ((col >= 0) ? 3 : 1);

  if (!dest.SameFormat(mw, mh, 1))
    return Fatal("Bad input to jhcParse3D::ShowHands");
  lims.SetVec3(dest.XLim(), dest.YLim(), 0.0);

  for (i = 0; i < n; i++)
  {
    // left hand is X
    if (items[i].HandPos(full, 0) > 0)
    {
      pos.MatVec(w2m, full);
      if (invert > 0)
        pos.CompVec3(lims);
      XMark(dest, pos.X(), pos.Y(), 17, th, -c); 
    }

    // right hand is +
    if (items[i].HandPos(full, 1) > 0)
    {
      pos.MatVec(w2m, full);
      if (invert > 0)
        pos.CompVec3(lims);
      Cross(dest, pos.X(), pos.Y(), 23, 23, th, -c); 
    }
  }
  return 1;
}


//= Overlay pointing ray (yellow) from hand to table or floor.
// if no surface intersection, plot to rmax (inches)
// can work with inverted (upside down) image

int jhcParse3D::ShowRays (jhcImg& dest, const jhcBodyData *items, int n, int invert, double zlev, int pt) const
{
  jhcMatrix full(4), start(4), end(4), lims(4);
  int i, side, cnt, th, col;

  if (!dest.SameFormat(mw, mh, 1))
    return Fatal("Bad input to jhcParse3D::ShowRays");
  lims.SetVec3(dest.XLim(), dest.YLim(), 0.0);

  for (i = 0; i < n; i++)
    for (side = 0; side <= 1; side++)
      if (items[i].HandOK(side) && ((items[i]).stable[side] >= 0))
      {
        // get end of ray
        items[i].RayHit(full, side, zlev);
        end.MatVec(w2m, full);
        if (invert > 0)
          end.CompVec3(lims);

        // get start of ray
        items[i].RayBack(full, side, flen);    // use 0 instead of flen for hand itself
        start.MatVec(w2m, full);              
        if (invert > 0)
          start.CompVec3(lims);
 
        // draw ray with different color and thickness if stable
        cnt = (items[i]).stable[side];
        th  = ((cnt >= pt) ? 3 : 1);
        col = ((cnt >= pt) ? -3 : -2);
        DrawLine(dest, start.X(), start.Y(), end.X(), end.Y(), th, col); 
      }
  return 1;
}


//= Overlay pointing ray (yellow) from hand to wall having a fixed y.
// can work with inverted (upside down) image
// NOTE: only shows rays that intersect given plan

int jhcParse3D::ShowRaysY (jhcImg& dest, const jhcBodyData *items, int n, int invert, double yoff, int pt) const
{
  jhcMatrix full(4), start(4), end(4), lims(4);
  int i, side, cnt, th, col;

  if (!dest.SameFormat(mw, mh, 1))
    return Fatal("Bad input to jhcParse3D::ShowRaysY");
  lims.SetVec3(dest.XLim(), dest.YLim(), 0.0);

  for (i = 0; i < n; i++)
    for (side = 0; side <= 1; side++)
      if (items[i].HandOK(side)  && ((items[i]).stable[side] >= 0))
      {
        // get end of ray
        if (items[i].RayHitY(full, side, yoff) < 2)   // must intersect
          continue;
        end.MatVec(w2m, full);
        if (invert > 0)
          end.CompVec3(lims);

        // get start of ray
        items[i].RayBack(full, side, flen);    // use 0 instead of flen for hand itself
        start.MatVec(w2m, full);              
        if (invert > 0)
          start.CompVec3(lims);

        // draw ray with different color and thickness if stable
        cnt = (items[i]).stable[side];
        th  = ((cnt >= pt) ? 3 : 1);
        col = ((cnt >= pt) ? -3 : -2);
        DrawLine(dest, start.X(), start.Y(), end.X(), end.Y(), th, col); 
      }
  return 1;
}


//= Overlay pointing ray (yellow) from hand to wall having a fixed x.
// can work with inverted (upside down) image
// NOTE: only shows rays that intersect given plan

int jhcParse3D::ShowRaysX (jhcImg& dest, const jhcBodyData *items, int n, int invert, double xoff, int pt) const
{
  jhcMatrix full(4), start(4), end(4), lims(4);
  int i, side, cnt, th, col;

  if (!dest.SameFormat(mw, mh, 1))
    return Fatal("Bad input to jhcParse3D::ShowRaysX");
  lims.SetVec3(dest.XLim(), dest.YLim(), 0.0);

  for (i = 0; i < n; i++)
    for (side = 0; side <= 1; side++)
      if (items[i].HandOK(side) && ((items[i]).stable[side] >= 0))
      {
        // get end of ray
        if (items[i].RayHitX(full, side, xoff) < 2)   // must intersect
          continue;
        end.MatVec(w2m, full);
        if (invert > 0)
          end.CompVec3(lims);

        // get start of ray
        items[i].RayBack(full, side, flen);    // use 0 instead of flen for hand itself
        start.MatVec(w2m, full);              
        if (invert > 0)
          start.CompVec3(lims);

        // draw ray with different color and thickness if stable
        cnt = (items[i]).stable[side];
        th  = ((cnt >= pt) ? 3 : 1);
        col = ((cnt >= pt) ? -3 : -2);
        DrawLine(dest, start.X(), start.Y(), end.X(), end.Y(), th, col); 
      }
  return 1;
}
