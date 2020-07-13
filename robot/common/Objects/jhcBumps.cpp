// jhcBumps.cpp : finds and tracks objects on table
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2016-2019 IBM Corporation
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

#include "Objects/jhcBumps.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcBumps::~jhcBumps ()
{
  dealloc();
}


//= Default constructor initializes certain values.

jhcBumps::jhcBumps (int n)
{
  double noise = P2I(4.0);           // 4 pixels 

  // size detection and tracking
  strcpy_s(name, "bump");
  total = 0;
  SetCnt(n);

  // surface height histogram and peak histogram
  hts.SetSize(256);
  pks.SetSize(256);

  // set default tracking values
  pos.SetName("bump");
  pos.SetTrack(6.0, 6.0, 6.0, 0.25, 0.0, 0.0, 2.0);            // was 8", 2", 1", then 12" (12Hz) 
  pos.SetFilter(noise, noise, noise, 0.8, 0.8, 0.2, 10, 30);   // was all 0.1 then 0.2

  // background thread parameter
  trk_bg = 1;

  // load processing parameters and clear state
  Defaults();
  Reset();
}


//= Configure for the maximum number of objects to track.

void jhcBumps::SetCnt (int n)
{
  int i;

  // if change needed then erase previous arrays   
  if (n <= total)
    return;
  if (total > 0)
    dealloc();

  // set up for given number of tracks and a lot of blobs 
  total = n;
  pos.SetSize(total);
  blob.SetSize(2 * total);

  // make up shape smoothing arrays for tracks
  shp = new double * [total];
  for (i = 0; i < total; i++)
    shp[i] = new double [6];      // w, l, h, maj, min, ang

  // allocate a middling number of raw detections
  rlim = ROUND(1.5 * total);
  raw = new double * [rlim];
  for (i = 0; i < rlim; i++)
    raw[i] = new double [11];     // x, y, z, w, l, h, maj, min, ang, ex, ey 

  // auxiliary object-person array
  touch = new int [total];
}


//= Get rid of all locally allocated storage.

void jhcBumps::dealloc ()
{
  int i;

  // auxiliary object-person array
  delete [] touch;

  // frame-by-frame detections
  if (raw != NULL)
  {
    for (i = 0; i < rlim; i++)
      delete [] raw[i];
    delete [] raw;
  }

  // shape parameters for tracked objects
  if (shp != NULL)
  {
    for (i = 0; i < total; i++)
      delete [] shp[i];
    delete [] shp;
  }
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for initial detection of table objects.

int jhcBumps::detect_params (const char *fname)
{
  char tag[40];
  jhcParam *ps = &dps;
  int ok;

  sprintf_s(tag, "%s_det", name);
  ps->SetTag(tag, 0);
  ps->NextSpec4( &sm,     5,    "Map interpolation (pel)");    // was 9 then 11 then 7
  ps->NextSpec4( &pmin,  10,    "Min averaging (pel)");        // was 3, 0 then 22
  ps->NextSpecF( &hobj,   0.75, "Object ht threshold (in)");   // was 0.5 then 1.0
  ps->NextSpecF( &htol,   0.25, "Object ht tolerance (in)");   
  ps->NextSpec4( &sc,     5,    "Evidence smoothing (pel)");   // was 7
  ps->NextSpec4( &sth,   80,    "Shape binary threshold");

  ps->NextSpec4( &amin,  50,    "Min blob area (pel)");        // was 50, 20, 200, then 100
  ps->NextSpecF( &hmix,   1.0,  "Height estimate mixing");     // was 0.05
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for correcting and updating shapes of objects.

int jhcBumps::shape_params (const char *fname)
{
  char tag[40];
  jhcParam *ps = &sps;
  int ok;

  sprintf_s(tag, "%s_size", name);
  ps->SetTag(tag, 0);
  ps->NextSpecF( &xyf,   0.85, "Shrink lateral sizes");
  ps->NextSpecF( &zf,    0.9,  "Shrink height");
  ps->Skip();
  ps->NextSpecF( &xymix, 0.1,  "Lateral update rate");
  ps->NextSpecF( &zmix,  0.1,  "Height update rate");
  ps->NextSpecF( &amix,  0.1,  "Angle update rate");

  ps->NextSpec4( &pcnt,   20,  "Points in height peak");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for detecting special class of objects.

int jhcBumps::target_params (const char *fname)
{
  char tag[40];
  jhcParam *ps = &tps;
  int ok;

  sprintf_s(tag, "%s_targ", name);
  ps->SetTag(tag, 0);
  ps->NextSpecF( &tlen1, 4.0, "Max target length (in)");
  ps->NextSpecF( &tlen0, 0.8, "Min target length (in)");
  ps->NextSpecF( &twid1, 2.7, "Max target width (in)");
  ps->NextSpecF( &twid0, 0.8, "Min target width (in)");
  ps->NextSpecF( &tht1,  2.5, "Max target height (in)");
  ps->NextSpecF( &tht0,  0.7, "Min target height (in)");

  ps->NextSpec4( &tcnt,    1, "Max number to detect");
  ps->NextSpec4( &hold,    0, "Track while holding");    
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcBumps::Defaults (const char *fname)
{
  int ok = 1;

  ok &= jhcOverhead3D::Defaults(fname);
  ok &= detect_params(fname);
  ok &= shape_params(fname);
  ok &= pos.Defaults(fname);
  ok &= target_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcBumps::SaveVals (const char *fname, int geom) const
{
  int ok = 1;

  ok &= jhcOverhead3D::SaveVals(fname, geom);
  ok &= dps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= pos.SaveVals(fname);
  ok &= tps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// can optionally erase old table area map (needed to detect arms)

void jhcBumps::Reset (int notop)
{
  // clear projection module
  jhcOverhead3D::Reset();

  // make sure local images are correct size
  det.SetSize(map);
  prev.SetSize(map);
  obj.SetSize(map);
  hand.SetSize(map);
  cc.SetSize(map, 2);
  hcc.SetSize(cc);
 
  // see if valid surface image is loaded
  surf = 0;
  if (!top.SameFormat(map))
    top.InitSize(map);
  else if (notop > 0)
    top.FillArr(0);
  else if (RegionNZ(troi, top, 128) > 1000)
    surf = 1;

  // initialize detector smoothing
  prev.FillArr(0);
  nr = 0;
  nr2 = 0;

  // clear tracking
  pos.Reset();
}


//= Find mask for surface of table.
// assumes all sensors have already been ingested
// can directly save and load "top" image

void jhcBumps::Surface (int trk)
{
  double mix = 0.1, stol = 0.75, side = 18.0;   // stol was 0.5
  jhcImg *src = &det;
  int big = ROUND(I2P(side) * I2P(side));

  // make sure surface image is correct size
  if (!top.SameFormat(map))
  {
    top.SetSize(map);
    top.FillArr(0);
  }

  // determine likely table area
  Interpolate(sm, pmin);
  Between(det, map2, DI2Z(-stol), DI2Z(stol));

  // smooth in time then space
  if (trk > 0)
  {
    AvgFcn(obj, prev, det);
    prev.CopyArr(det);
    src = &obj;
  }
  BoxAvg(obj, *src, sc);

  // carve into solid components
  FillHoles(obj, obj, big);          // was 10000 !!!
  CComps4(cc, obj, amin, sth);
  blob.FindBBox(cc);

  // keep only the biggest blob and fill all holes
  blob.MarkBlob(det, cc, blob.Biggest());
  if (trk > 0)
    MixToward(top, det, top, mix, 1);
  else
    top.CopyArr(det);
}


//= Tells if any object is currently being touched.
// use Contact() to figure out which one

int jhcBumps::AnyTouch () const
{
  int i, n = ObjLimit();

  for (i = 0; i < n; i++)
    if (Contact(i))
      return 1;
  return 0;
}


//= Find and track all objects on the table.
// assumes all sensors have already been ingested
// returns number of raw object detections (may not be tracked yet)

int jhcBumps::Analyze (int trk)
{
  // detect clearly separated objects
  raw_objs(trk);
  obj_boxes();

  // match detections to tracks then salvage grabbed objects
  pos.MatchAll(raw, nr, 0, shp);
  occluded();
  pos.Prune();

  // update shape parameters for tracked objects
  adj_shapes();
  if (tcnt > 0)
    mark_targets("target", trk);

  // recalibrate table height based on map
  if (hmix > 0.0)
    TableHt();
  return nr;
}


//= Find candidate objects on table for later tracking.
// information is in "cc" image and "blob" analyzer
// returns number found

void jhcBumps::raw_objs (int trk)
{
  jhcImg *src = &det;

  // find objects above table in overhead map
  Interpolate(sm, pmin);
  RampOver(det, map2, DI2Z(hobj - htol), DI2Z(hobj + htol));

  // smooth evidence in time then in space
  if (trk > 0)
  {
    AvgFcn(obj, prev, det);
    prev.CopyArr(det);
    src = &obj;
  }
  BoxThresh(obj, *src, sc, sth);

  // thin out then get rid of components extending beyond table
  BoxAvg(obj, obj, sc);
  CComps4(cc, obj, amin, 180);
  blob.FindParams(cc);             // also centroid, major, minor
  if (surf > 0)
    blob.PoisonOver(cc, top, -128);
}


//= Get real coordinates of bounding boxes for all detections.
// detections stored in "raw" (xyz[11]) array (x, y, z, w, l, h, maj, min, ang, ex, ey)

void jhcBumps::obj_boxes ()
{
  jhcRoi area;
  double *xyz;
  double ht;
  int i, n = blob.Active();

  // collect bounding boxes centers and sizes
  nr = 0;
  for (i = 0; i < n; i++)
    if (blob.GetStatus(i) > 0)
    {
      // determine true object height (apply shrinking)
      blob.GetRoi(area, i);
      ht = zf * find_max(map2, cc, i, area);    
      if (ht <= 0.0)
      {
        // punt if just noise
        blob.SetStatus(i, 0);
        continue;
      }

      // make new entry and record position (bounding box center, not centroid)
      xyz = raw[nr];
      xyz[0] = P2I(blob.BoxAvgX(i)) - x0;
      xyz[1] = P2I(blob.BoxAvgY(i)) - y0;
      xyz[2] = 0.5 * ht + ztab;

      // record dimensions (compensate for bloat)
      xyz[3] = xyf * P2I(blob.BoxW(i));
      xyz[4] = xyf * P2I(blob.BoxH(i));
      xyz[5] = ht;

      // record equivalent ellipse (compensate for bloat)
      xyz[6] = xyf * P2I(blob.BlobLength(i));
      xyz[7] = xyf * P2I(blob.BlobWidth(i));
      xyz[8] = blob.BlobAngle(i, 1);

      // assume no fingertip point needed
      xyz[9]  = -1;
      xyz[10] = -1;
      if (++nr >= rlim)
        break;
    }
}


//= Find maximum value inside some component given its bounding box.
// used to find single max, now uses histogram for noise robustness
// returns converted height value in inches (relative to table)

double jhcBumps::find_max (const jhcImg& val, const jhcImg& comp, int i, const jhcRoi& area) 
{
  int vsk = val.RoiSkip(area), csk = comp.RoiSkip(area) >> 1;
  int x, y, rw = area.RoiW(), rh = area.RoiH();
  const US16 *c = (const US16 *) comp.RoiSrc(area);
  const UC8 *v = val.RoiSrc(area);

  pks.Fill(0);
  for (y = rh; y > 0; y--, v += vsk, c += csk)
    for (x = rw; x > 0; x--, v++, c++)
      if (*c == i)
        pks.AInc(*v, 1);
  return DZ2I(pks.MaxBinN(pcnt));
}


//= Update shape statistics for tracked objects.
// array "shp" (wlh[6]) has smooth track information (w, l, h, maj, min, ang)
// array "raw" (xyz[11]) has detection info (x, y, z, w, l, h, maj, min, ang, ex, ey)

void jhcBumps::adj_shapes ()
{
  double da, ang;
  double *wlh, *xyz;
  int i, j, n = pos.Limit();

  for (i = 0; i < n; i++)
    if ((j = pos.DetectFor(i)) >= 0)
    {
      // get source and destination for bounding box
      xyz = raw[j];
      wlh = shp[i];
      if (pos.Valid(i) <= 0)
      {
        // just copy box dimensions for a new track
        wlh[0] = xyz[3];
        wlh[1] = xyz[4];
        wlh[2] = xyz[5];

        // copy equivalent ellipse and angle
        wlh[3] = xyz[6];
        wlh[4] = xyz[7];
        wlh[5] = xyz[8];
      }
      else
      {
        // apply IIR filter to box dimensions
        wlh[0] += xymix * (xyz[3] - wlh[0]); 
        wlh[1] += xymix * (xyz[4] - wlh[1]);
        wlh[2] +=  zmix * (xyz[5] - wlh[2]);

        // apply IIR filter to ellipse shape
        wlh[3] += xymix * (xyz[6] - wlh[3]);
        wlh[4] += xymix * (xyz[7] - wlh[4]);

        // determine actual orientation shift
        da = xyz[8] - wlh[5];
        if (da >= 90.0)
          da -= 180.0;
        else if (da < -90.0)
          da += 180.0;

        // constrain result to be from 0 to 180 degrees
        ang = wlh[5] + amix * da;
        if (ang >= 180.0)
          ang -= 180.0;
        else if (ang < 0.0)
          ang += 180.0;
        wlh[5] = ang;
      }
    }
}


///////////////////////////////////////////////////////////////////////////
//                          Occlusion Handling                           //
///////////////////////////////////////////////////////////////////////////

//= Generate fake detections for tracked objects under presumed arms.

void jhcBumps::occluded ()
{
  double xc, yc, wid, len;
  int i, mark, ex, ey, dx, dy, tip = 0, n = pos.Limit();

  // find hand-like regions (i.e. over table but also extended beyond)
  blob.MarkOnly(hand, cc, 0);
  if (surf > 0)
    OverGate(hand, hand, top, 128);     
  CComps4(hcc, hand);                      // in case 2 arms visible

  // attempt to imagine detections for unpaired tracks
  nr2 = nr;
  for (i = 0; i < n; i++)
    if ((pos.Valid(i) > 0) && (pos.DetectFor(i) < 0))
    {
      // find associated hand component number and fingertip position
      img_box(xc, yc, wid, len, i);
      if ((mark = arm_end(ex, ey, xc, yc)) == 0)
        continue;
      dx = incl_x(xc, wid, ex);
      dy = incl_y(yc, len, ey);

      // check if fingertip is close enough to act as anchor
      if ((abs(dx) <= I2P(pos.close[0])) && (abs(dy) <= I2P(pos.close[1])))
      {
        xc += dx;
        yc += dy;
        tip = 1;
      }

      // jockey bounding box around to be consistent with hand and fingertip
      xc += drag_x(xc, yc, wid, len, hcc, mark);
      if (tip > 0)
        xc += incl_x(xc, wid, ex);
      yc += drag_y(xc, yc, wid, len, hcc, mark);
      if (tip > 0)
        yc += incl_y(yc, len, ey);

      // make sure object is still over table
      if (surf > 0)
        if (top.ARef(ROUND(xc), ROUND(yc)) < 128)
          continue;

      // create new detection and force match to track
      make_det(nr2, M2X(xc), M2Y(yc), pos.TZ(i), shp[i], ex, ey);
      pos.PairUp(i, raw, nr2);
      if (++nr2 >= rlim)
        break;
    }
}


//= Get last position and size of tracked object wrt image.
// converts inches in global coordinates into map pixels

void jhcBumps::img_box (double& xc, double& yc, double& wid, double& len, int i) const
{
  double *wlh = shp[i];

  xc  = W2X(pos.TX(i));
  yc  = W2Y(pos.TY(i)); 
  wid = I2P(wlh[0]);
  len = I2P(wlh[1]);
}


//= Find component number for arm and likely fingertip location in image.
// takes center of tracked component (in pixels not inches)

int jhcBumps::arm_end (int& ex, int& ey, double xc, double yc) const
{
  jhcRoi area;
  double bx, by, xrng, yrng, jump = I2P(8.0);
  int mark, hx, hy, dude; 

  // search near track center for arm blob (and person blob)
  xrng = I2P(2.0 * pos.close[0]);
  yrng = I2P(2.0 * pos.close[1]);
  area.SetCenter(xc, yc, xrng, yrng);
  if ((mark = NearestComp(hx, hy, area, hcc)) == 0)
    return 0;
  dude = cc.ARef(hx, hy);

  // set up to search within part of component roughly over table
  blob.GetRoi(area, dude);
  if (surf > 0)
    area.MergeRoi(troi);
  blob.BlobCentroid(&bx, &by, dude);
  ExtremePt(ex, ey, ROUND(bx), ROUND(by), hcc, mark, area); 
  return mark;
}


//= Calculate shift box so that it contains extremal X coordinate.
// no shift if extremal point is invalid (negative)

int jhcBumps::incl_x (double xc, double wid, int ex) const 
{
  int x0 = ROUND(xc - 0.5 * wid), x2 = ROUND(xc + 0.5 * wid);

  if (ex < 0)
    return 0;
  if (ex < x0)
    return(ex - x0);
  if (ex > x2)
    return(ex - x2);
  return 0;
}


//= Calculate shift box so that it contains extremal Y coordinate.
// no shift if extremal point is invalid (negative)

int jhcBumps::incl_y (double yc, double len, int ey) const 
{
  int y0 = ROUND(yc - 0.5 * len), y2 = ROUND(yc + 0.5 * len);

  if (ey < 0)
    return 0;
  if (ey < y0)
    return(ey - y0);
  if (ey > y2)
    return(ey - y2);
  return 0;
}


//= Pick amount to move bounding box in X to better cover hand region.
// stretches box in x directions and looks for flanking blank columns
// centers box if both flanks found, else sucks BB edge to single flank
// positions, sizes, and return adjustment all in pixels, not inches

int jhcBumps::drag_x (double xc, double yc, double wid, double ht, const jhcImg& src, int mark) const
{
  jhcRoi area;
  double mid = 0.0, mv2 = I2P(2.0 * pos.close[0]);
  int x, y, rw, rh, any = 0, lo = -1, hi = -1, ln = src.Line() >> 1;
  const US16 *s, *s0;

  // make sure the search area is completely within the image
  area.SetCenter(xc, yc, wid + mv2, ht);
  area.RoiClip(src);
  s0 = (const US16 *) src.RoiSrc(area);
  rw = area.RoiW();
  rh = area.RoiH();

  // scan the image column by column 
  for (x = 0; x < rw; x++, s0++)
  {
    // check if next column is completely empty
    s = s0;
    for (y = rh; y > 0; y--, s += ln)
      if (*s == mark)
        break;

    // if so, shrink bounds on occupied area
    if (y > 0)
      any = 1;
    else if (any <= 0)
      lo = x;             
    else if (hi < 0)
      hi = x;
  }

  // center if possible, else suck to nearest edge (if any)
  if ((hi < 0) && (lo < 0))
    return 0;
  if (hi >= 0)
    mid += ((hi - 1) - 0.5 * wid);
  if (lo >= 0)
    mid += ((lo + 1) + 0.5 * wid);
  if ((hi >= 0) && (lo >= 0))
    mid *= 0.5;

  // convert ROI-based adjustment to original center
  mid += area.RoiX();
  return ROUND(mid - xc);
}


//= Pick amount to move bounding box in Y to better cover hand region.
// stretches box in y directions and looks for flanking blank lines
// centers box if both flanks found, else sucks BB edge to single flank
// positions, sizes, and return adjustment all in pixels, not inches

int jhcBumps::drag_y (double xc, double yc, double wid, double ht, const jhcImg& src, int mark) const 
{
  jhcRoi area;
  double mid = 0.0, mv2 = I2P(2.0 * pos.close[1]);
  int x, y, rw, rh, any = 0, lo = -1, hi = -1, ln = src.Line() >> 1;
  const US16 *s, *s0;

  // make sure the search area is completely within the image
  area.SetCenter(xc, yc, wid, ht + mv2);
  area.RoiClip(src);
  s0 = (const US16 *) src.RoiSrc(area);
  rw = area.RoiW();
  rh = area.RoiH();

  // scan the image line by line
  for (y = 0; y < rh; y++, s0 += ln)
  {
    // check if next line is completely empty
    s = s0;
    for (x = rw; x > 0; x--, s++)
      if (*s == mark)
        break;

    // if so, shrink bounds on occupied area
    if (x > 0)
      any = 1;
    else if (any <= 0)
      lo = y;             
    else if (hi < 0)
      hi = y;
  }

  // center if possible, else suck to nearest edge (if any)
  if ((hi < 0) && (lo < 0))
    return 0;
  if (hi >= 0)
    mid += ((hi - 1) - 0.5 * ht);
  if (lo >= 0)
    mid += ((lo + 1) + 0.5 * ht);
  if ((hi >= 0) && (lo >= 0))
    mid *= 0.5;

  // convert ROI-based adjustment to original center
  mid += area.RoiY();
  return ROUND(mid - yc);
}


//= Create new pseudo-detection for occluded item.
// assumes value are already in inches and global coordinates
// ex and ey are arm extremal point (for debugging)

void jhcBumps::make_det (int i, double xc, double yc, double zc, double *wlh, int ex, int ey)
{
  double *xyz = raw[i];

  // position
  xyz[0] = xc;
  xyz[1] = yc;
  xyz[2] = zc;

  // dimensions
  xyz[3] = wlh[0];
  xyz[4] = wlh[1];
  xyz[5] = wlh[2];

  // equivalent ellipse
  xyz[6] = wlh[3];
  xyz[7] = wlh[4];
  xyz[8] = wlh[5];

  // fingertip position
  xyz[9]  = ex;        
  xyz[10] = ey;
}


///////////////////////////////////////////////////////////////////////////
//                           Target Finding                              //
/////////////////////////////////////////////////////////////////////////// 

//= Find potential objects of special target class and mark them.
// returns number of target items found

int jhcBumps::mark_targets (const char *name, int trk)
{
  double il = 2.0 / (tlen1 + tlen0), iw = 2.0 / (twid1 + twid0);
  double len, wid, ht, diff, frac, best, ih = 2.0 / (tht1 + tht0);
  int i, win, n = ((trk <= 0) ? nr : pos.Limit()), nt = 0;

  // count number of currently marked target items 
  if (trk <= 0)
    for (i = 0; i < n; i++)       // clear all tags
      (pos.tag[i])[0] = '\0';
  else
    for (i = 0; i < n; i++)
      if (pos.Valid(i) > 0)       // only stable tracks
        if (strcmp(pos.tag[i], name) == 0)
        {
          // get object dimensions
          len = Major(i, trk);
          wid = Minor(i, trk);
          ht  = SizeZ(i, trk);

          // invalidate if no longer satisfies size ranges
          // possibly invalidate if being touched by user
          if ((len < tlen0) || (wid < twid0) || (ht < tht0) ||
              (len > tlen1) || (wid > twid1) || (ht > tht1) ||
              ((hold <= 0) && (touch[i] >= 0)))
            pos.tag[i][0] = '\0';
          else
            nt++;
        }

  // possibly find more that match specs
  while (nt < tcnt)
  {
    // find most target-like object which is not yet marked
    // possibly ignore if currently being touched
    win = -1;
    for (i = 0; i < n; i++)
      if ((trk <= 0) || (pos.Valid(i) > 0))
        if ((strcmp(pos.tag[i], name) != 0) &&
            ((hold > 0) || (touch[i] < 0)))
        {
          // get object dimensions
          len = Major(i, trk);
          wid = Minor(i, trk);
          ht  = SizeZ(i, trk);

          // punt if does not satisfies size ranges
          if ((len < tlen0) || (wid < twid0) || (ht < tht0) ||
              (len > tlen1) || (wid > twid1) || (ht > tht1))
            continue;

          // compute L2 distance wrt fractions of average dimensions
          diff = len * il - 1.0;
          frac = diff * diff;
          diff = wid * iw - 1.0; 
          frac += diff * diff;
          diff = ht * ih - 1.0;
          frac += diff * diff;
          if ((win < 0) || (frac < best))
          {
            // keep if better than previous
            win = i;
            best = frac;
          }
        }
    
     // label winning object as a target
     if (win < 0)
       break;
     strcpy_s(pos.tag[win], 80, name);
     nt++;     
  }
  return nt;
}


///////////////////////////////////////////////////////////////////////////
//                        Read-Only Properties                           //
///////////////////////////////////////////////////////////////////////////

//= Number of objects in list to enumerate over.

int jhcBumps::ObjLimit (int trk) const      
{
  return((trk > 0) ? pos.Limit() : nr2);
}


//= Whether a particular object is considered legitimate.

bool jhcBumps::ObjOK (int i, int trk) const
{
  if (trk > 0)
    return(pos.Valid(i) > 0);
  return((i >= 0) && (i < nr2));
}


//= What identification has been assigned to a particular object.

int jhcBumps::ObjID (int i, int trk) const 
{
  if (trk > 0)
    return pos.Valid(i);
  return(((i >= 0) && (i < nr2)) ? i : -1);
}


//= Text tag associated with object.

const char *jhcBumps::ObjDesc (int i, int trk) const
{
  return((ok_idx(i, trk)) ? pos.tag[i] : NULL);
}


//= Bounding box center X of object.

double jhcBumps::PosX (int i, int trk) const
{
  if (trk > 0)
    return pos.TX(i);
  return(((i >= 0) && (i < nr2)) ? raw[i][0] : 0.0);
}


//= Bounding box center Y of object.

double jhcBumps::PosY (int i, int trk) const
{
  if (trk > 0)
    return pos.TY(i);
  return(((i >= 0) && (i < nr2)) ? raw[i][1] : 0.0);
}


//= Bounding box center Z of object.

double jhcBumps::PosZ (int i, int trk) const  
{
  if (trk > 0)
    return pos.TZ(i);
  return(((i >= 0) && (i < nr2)) ? raw[i][2] : 0.0);
}


//= Bounding box X size of object.

double jhcBumps::SizeX (int i, int trk) const 
{
  if (!ok_idx(i, trk))
    return 0.0;
  return((trk > 0) ? shp[i][0] : raw[i][3]);
}


//= Bounding box Y size of object.

double jhcBumps::SizeY (int i, int trk) const 
{
  if (!ok_idx(i, trk))
    return 0.0;
  return((trk > 0) ? shp[i][1] : raw[i][4]);
}


//= Bounding box Z size of object.

double jhcBumps::SizeZ (int i, int trk) const 
{
  if (!ok_idx(i, trk))
    return 0.0;
  return((trk > 0) ? shp[i][2] : raw[i][5]);
}


//= Longest dimension of equivalent ellipse dimension for object.

double jhcBumps::Major (int i, int trk) const 
{
  if (!ok_idx(i, trk))
    return 0.0;
  return((trk > 0) ? shp[i][3] : raw[i][6]);
}


//= Shortest dimension of equivalent ellipse dimension for object.

double jhcBumps::Minor (int i, int trk) const 
{
  if (!ok_idx(i, trk))
    return 0.0;
  return((trk > 0) ? shp[i][4] : raw[i][7]);
}


//= Orientation of major axis of equivalent ellipse for object.

double jhcBumps::Angle (int i, int trk) const 
{
  if (!ok_idx(i, trk))
    return 0.0;
  return((trk > 0) ? shp[i][5] : raw[i][8]);
}


//= Ratio of longest XY ellipse dimension to shortest.

double jhcBumps::Elongation (int i, int trk) const
{
{
  if (!ok_idx(i, trk))
    return 0.0;
  return((trk > 0) ? shp[i][3] / shp[i][4] : raw[i][6] / raw[i][7]);
}

}

//= Whether supplied index makes sense for an actual object.

bool jhcBumps::ok_idx (int i, int trk) const
{
  if (trk > 0)
    return((i >= 0) && (i < pos.Limit())); 
  return((i >= 0) && (i < nr2));
}


//= Whether the object is currently touched and hence hallucinated.

bool jhcBumps::Contact (int i, int trk) const 
{
  if (trk > 0)
    return(pos.DetectFor(i) >= nr);
  return((i >= nr) && (i < nr2));
}


///////////////////////////////////////////////////////////////////////////
//                   Auxiliary Object-Person Array                       //
///////////////////////////////////////////////////////////////////////////

//= Tell if tracked object is being touched, and possibly by whom.
// returns person ID if known, 0 for touched but unknown, -1 for not touched
// NOTE: not set by this class, use Contact() instead

int jhcBumps::TouchID (int i, int trk) const
{
  return((ok_idx(i, trk)) ? touch[i] : -1); 
}


//= Set the source of touching (typically a person ID) for a tracked object.
// returns 1 if accepted, 0 if rejected

int jhcBumps::SetTouch (int i, int src, int trk)
{
  if (!ok_idx(i, trk))
    return 0;
  touch[i] = src;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Proximity Detection                            //
///////////////////////////////////////////////////////////////////////////

//= Check if point is near any of the current objects.
// grows oriented bounding box by "bloat" on each side
// uses circle as reference instead if box elongation less than "ecc"

bool jhcBumps::ObjNear (double hx, double hy, int trk, double bloat, double ecc) const
{
  double dx, dy, hlen, hwid, r, rads, c, s, lc, ls, wc, ws;
  int i, n = ObjLimit(trk);

  // possibly disable check
  if (bloat < 0.0)
    return false;

  // scan through all current table objects
  for (i = 0; i < n; i++)
    if (ObjOK(i, trk))
    {
      // get oriented bounding box after growing a bit
      dx = hx - PosX(i, trk);
      dy = hy - PosY(i, trk);
      hlen = 0.5 * Major(i, trk) + bloat;
      hwid = 0.5 * Minor(i, trk) + bloat;

      // if round-ish then check distance to center
      if (Elongation(i, trk) < ecc)
      {
        r = 0.5 * (hlen + hwid);
        if ((dx * dx + dy * dy) < (r * r))
          return true;
        continue;
      }

      // get offsets of side and corners for oriented box
      rads = D2R * Angle(i, trk);
      c = cos(rads);
      s = sin(rads);
      lc = hlen * c;
      ls = hlen * s;
      wc = hwid * c;
      ws = hwid * s;

      // check if point outside any edge segment
      if (( c * (dx - lc) + s * (dy - ls)) > 0.0)   // east  @ ( lc  ls), out = ( c  s)
        continue;
      if ((-s * (dx + ws) + c * (dy - wc)) > 0.0)   // north @ (-ws  wc), out = (-s  c)
        continue;
      if ((-c * (dx + lc) - s * (dy + ls)) > 0.0)   // west  @ (-lc -ls), out = (-c -s)
        continue;
      if (( s * (dx - ws) - c * (dy + wc)) > 0.0)   // south @ ( ws -wc), out = ( s -c)
        continue;
      return true;                                  // inside all
    }
  return false;
}


//= Check whether point is close to something projecting past the table edge.

bool jhcBumps::AgtNear (double hx, double hy, double dist) const
{
  jhcRoi box;
  const US16 *s;
  int x, y, w, h, skip;

  // possibly disable check
  if (dist < 0.0)
    return true;

  // get search box around given point (at least 1 pixel)
  box.SetCenter(W2X(hx), W2Y(hy), 2 * ROUND(I2P(dist)) + 1);
  box.RoiClip(cc); 
  w = box.RoiW();
  h = box.RoiH();
  s = (const US16 *) cc.RoiSrc(box);
  skip = cc.RoiSkip(box) >> 1;

  // find blob associated with each pixel and see if valid
  for (y = h; y > 0; y--, s += skip)
    for (x = w; x > 0; x--, s++)
      if ((*s > 0) && (blob.GetStatus(*s) == 0))
        return true;
  return false;
}


///////////////////////////////////////////////////////////////////////////
//                       Environment Calibration                         //
///////////////////////////////////////////////////////////////////////////

//= Attempts to find current height of table based on surface map.
// returns most common value in table map (to mark hts array)

int jhcBumps::TableHt (int update)
{
  double frac, drop = 0.2, cover = 0.5;
  int sum, mid, up, dn, hpk = I2Z(ztab), cnt = 10000, thin = 15;

  // histogram heights on table area
  HistOver(hts, map, top, 128);
  hts.ASet(0, 0);
  if ((sum = hts.SumAll()) > cnt)
  {
    // make sure there is a tight high mass peak
    mid = hts.MaxBin();
    up = hts.PeakRise(mid, drop);
    dn = hts.PeakFall(mid, drop);
    frac = hts.SumRegion(up, dn) / (double) sum;
    if (((mid - up) < thin) && ((dn - mid) < thin) && (frac > cover))
      hpk = mid + 1;                       // plus one is a fudge?
    else
      jprintf(">>> Bad peak %d %+d frac %4.2f in jhcBumps::TableHt !\n", up - mid, dn - mid, frac); 
  }
  
  // possibly slowly adjust working height
  if (update > 0)
    ztab += hmix * (Z2I(hpk) - ztab);
  return hpk;
}


//= Tell if some world point is over known table mask.

bool jhcBumps::OverTable (double wx, double wy) const
{
  int ix = ROUND(W2X(wx)), iy = ROUND(W2Y(wy));

  if (!top.InBounds(ix, iy))
    return false;
  return(top.ARef(ix, iy) > 128);
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Show all items on a map-like image.
// only shows primary detections, not imagined occlusions
// can work with inverted (upside down) image

int jhcBumps::ShowAll (jhcImg& dest, int trk, int invert, int style)
{
  int i, n = pos.Limit();

  if (!dest.Valid(1, 3))
    return Fatal("Bad input to jhcBumps::ShowAll");

  if (trk > 0)
    for (i = 0; i < n; i++)
      TrackBox(dest, i, 1, invert, style);
  else
    for (i = 0; i < nr; i++)
      RawBox(dest, i, 1, invert);
  return 1;
}


//= Show objects where the head of text description matches string.
// shows all objects with non-empty descriptions if desc = NULL
// can work with inverted (upside down) image

int jhcBumps::Targets (jhcImg& dest, const char *desc, int trk, int invert)
{
  int i, head = 0, n = pos.Limit();

  if (!dest.Valid(1, 3))
    return Fatal("Bad input to jhcBumps::Targets");
  if (desc != NULL)
    head = (int) strlen(desc);

  if (trk > 0)
  {
    for (i = 0; i < n; i++)
      if (((pos.tag[i])[0] != '\0') &&
          ((desc == NULL) || (strncmp(pos.tag[i], desc, head) == 0)))
        TrackBox(dest, i, -1, invert);
  }
  else
    for (i = 0; i < nr; i++)
      if (((pos.tag[i])[0] != '\0') &&
          ((desc == NULL) || (strncmp(pos.tag[i], desc, head) == 0)))
        RawBox(dest, i, -1, invert);
  return 1;
}


//= Show presumed position of occluded items.
// these pseudo-detections do not show up with AllBoxes

int jhcBumps::Occlusions (jhcImg& dest)
{
  int i;

  if (!dest.Valid(1, 3))
    return Fatal("Bad input to jhcBumps::Occlusions");

  for (i = nr; i < nr2; i++)
    RawBox(dest, i, -1);
  return 1;
}


//= Show mark over middle of items currently in contact with presumed hands.

int jhcBumps::Touches (jhcImg& dest) const
{
  double sc = MSC(dest);
  int i, n = pos.Limit();

  if (!dest.Valid(1, 3))
    return Fatal("Bad input to jhcBumps::Touches");

  for (i = 0; i < n; i++)
    if (Contact(i))
      Cross(dest, sc * W2X(pos.TX(i)), sc * W2Y(pos.TY(i)), 25, 25, 1);
  return 1;
}


//= Show presumed ends of arms used to help track occluded objects.

int jhcBumps::ArmEnds (jhcImg& dest) const
{
  double ex, ey, sc = MSC(dest);
  int i;

  if (!dest.Valid(1, 3))
    return Fatal("Bad input to jhcBumps::ArmEnds");

  for (i = nr; i < nr2; i++)
  {
    ex = raw[i][9];
    ey = raw[i][10];
    if ((ex >= 0.0) && (ey >= 0.0))
      XMark(dest, sc * ex, sc * ey, 17, 3, -5);
  }
  return 1;
}


//= Show tracked objects as oriented rectangles with emphasis if touched.
// if object has low eccentricity then shown as an ellipse instead
// can optionally scale XY dimensions to match report or provide margin

int jhcBumps::Ellipses (jhcImg& dest, double rect, int trk, int style)
{
  jhcRoi box;
  double *xyz, *wlh;
  double xc, yc, wid, len, ang, c, s, xdim, ydim, sc = MSC(dest);
  int i, id, col = 7, n = ((trk <= 0) ? nr2 : pos.Limit());

  if (!dest.Valid(1, 3))
    return Fatal("Bad input to jhcBumps::Ellipses");

  for (i = 0; i < n; i++)
    if ((trk <= 0) || ((id = pos.Valid(i)) > 0))
    {
      // get image position, size, and orientation of item 
      if (trk <= 0)
      {
        id = i;
        xyz = raw[i];
        xc = W2X(xyz[0]);
        yc = W2Y(xyz[1]);
        len = I2P(xyz[6]);
        wid = I2P(xyz[7]);
        ang = xyz[8];
      }
      else
      {
        xc = W2X(pos.TX(i));
        yc = W2Y(pos.TY(i));
        wlh = shp[i];
        len = I2P(wlh[3]); 
        wid = I2P(wlh[4]);
        ang = wlh[5];
      }

      // adjust for destination image size
      xc *= sc;
      yc *= sc;
      len *= sc;
      wid *= sc;

      // figure out rough region occupied by ellipse
      c = fabs(cos(D2R * ang));
      s = sin(D2R * ang);
      xdim = len * c + wid * s;
      ydim = len * s + wid * c;
      box.SetCenter(xc, yc, xdim, ydim);

      // draw oriented rectangle with number nearby, color depends on ID 
      col = ((id % 6) + 1);
      if ((rect <= 0.0) || (len < (rect * wid)))
        EllipseEmpty(dest, xc, yc, len, wid, ang, 3, -col);
      else
        RectCent(dest, xc, yc, len, wid, ang, 3, -col);
      LabelBox(dest, box, id, -16, -col);
    }
  return 1;
}


//= Show position of some tracked object on a map-like image.
// color is based on ID unless num < 0 in which case it is white
// can optionally show ID number near the box (num > 0)
// can work with inverted (upside down) image

int jhcBumps::TrackBox (jhcImg& dest, int i, int num, int invert, int style) 
{
  jhcRoi box;
  double *wlh;
  int id, col = 7;
 
  if (!dest.Valid(1, 3) || (i < 0))
    return Fatal("Bad input to jhcBumps::TrackBox");
  if ((i >= pos.Limit()) || ((id = pos.Valid(i)) <= 0))
    return 0;
  
  // get image position of item and pick color based on ID
  wlh = shp[i];
  box.SetCenter(W2X(pos.TX(i)), W2Y(pos.TY(i)), I2P(wlh[0]), I2P(wlh[1]));
  box.ScaleRoi(MSC(dest));
  if (invert > 0)
    box.InvertRoi(dest.XDim(), dest.YDim());
  if (num >= 0)
    col = ((id % 6) + 1);

  // draw bounding box and possibly ID number
  RectEmpty(dest, box, 3, -col);
  if (num > 0)
    LabelBox(dest, box, label(i, style), -16, -col);
  return 1;
}


//= Show position of some detected object on a map-like image.
// color is based on ID unless num < 0 in which case it is white
// can optionally show ID number near the box (num > 0)
// can work with inverted (upside down) image

int jhcBumps::RawBox (jhcImg& dest, int i, int num, int invert)
{
  jhcRoi box;
  double *xyz;
  int col = 7;

  if (!dest.Valid(1, 3) || (i < 0))
    return Fatal("Bad input to jhcBumps::RawBox");
  if (i >= nr2)
    return 0;

  // get image position of item and pick color based on ID
  xyz = raw[i];
  box.SetCenter(W2X(xyz[0]), W2Y(xyz[1]), I2P(xyz[3]), I2P(xyz[4]));
  box.ScaleRoi(MSC(dest));
  if (invert > 0)
    box.InvertRoi(dest.XDim(), dest.YDim());
  if (num >= 0)
    col = ((i % 6) + 1);

  // draw bounding box and possibly ID number
  RectEmpty(dest, box, 3, -col);
  if (num > 0)
    LabelBox(dest, box, i, -16, -col);
  return 1;
}


//= Show current object locations and numbers on color or depth input image.
// needs to know which sensor input to plot relative to
// can optionally show raw or tracked version 

int jhcBumps::ObjsCam (jhcImg& dest, int cam, int trk, int rev, int style)
{
  jhcRoi box;
  double *xyz, *wlh;
  double dx = x0 - 0.5 * mw, sc = ISC(dest);
  int i, id, col, w = dest.XDim(), n = pos.Limit();

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcBumps::ObjsCam");

  // set up projection from a particular camera
  AdjGeometry(cam);

  // find projection of oriented flat rectangle for detection
  // then draw box on image, red if touched
  if (trk <= 0)
    for (i = 0; i < nr2; i++)
    {
      xyz = raw[i];
      ImgPrism(box, xyz[0] + dx, xyz[1] + y0, xyz[2], xyz[8], xyz[6], xyz[7], xyz[5], sc); 
      box.ScaleRoi(sc);
      if (rev > 0)
        box.MirrorRoi(w);
      col = ((i > nr) ? 1 : 2); 
      RectEmpty(dest, box, 3, -col);
      LabelBox(dest, box, i, -16, -col);
    }
  else
    for (i = 0; i < n; i++)
      if ((id = pos.Valid(i)) > 0)
      {
        wlh = shp[i];
        ImgPrism(box, pos.TX(i) + dx, pos.TY(i) + y0, pos.TZ(i), wlh[5], wlh[3], wlh[4], wlh[2], sc); 
        box.ScaleRoi(sc);
        if (rev > 0)
          box.MirrorRoi(w);
        col = ((Contact(i)) ? 1 : 2);
        RectEmpty(dest, box, 3, -col);
        LabelBox(dest, box, label(i, style), -16, -col);
      }
  return 1;
}


//= Generate text label for object in given style.
// style: 0 = number, 1 = object-N, 2 = box, 3 = red box

const char *jhcBumps::label (int i, int style)
{
  const char *txt = pos.tag[i];
  int id = pos.Valid(i);

  if ((style <= 0) || ((style == 2) && (*txt == '\0')))
    _itoa_s(id, tmp, 10);       
  else if ((style == 1) || ((style >= 3) && (*txt == '\0')))
    sprintf_s(tmp, "object-%d", id);
  else if (style >= 2)
    strcpy_s(tmp, txt);
  return tmp;
}


