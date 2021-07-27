// jhcSurfObjs.cpp : find objects on surfaces using single mobile depth scanner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2021 Etaoin Systems
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
  dealloc();
}


//= Default constructor initializes certain values.

jhcSurfObjs::jhcSurfObjs (int n)
{
  // set standard sizes
  glob.SetSize(100);
  alt_blob = &glob;                              // for jhcBumps::obj_boxes
  wkhist.SetSize(256);
  pos.axes = 0;                                  // camera not stationary

  // maximum number of tracks
  ntrk = 0;
  SetCnt(n);

  // processing parameters for base and components
  SetFit(0.75, 2000, 0.5, 4.0, 4.0, 3.0, 100);
  pp.SetFind(3, 50, 45, 25, 245, 100, 50); 
  pp.SetHue(2, 10, 30, 120, 180, 240); 

  // own parameters
  Defaults();
}


//= Change the maximum number of objects that can be tracked.

void jhcSurfObjs::SetCnt (int n)
{
  int i, cmax = pp.NumCols();

  // if change needed then erase previous arrays    
  jhcBumps::SetCnt(n);
  if (n <= ntrk)
    return;
  if (ntrk > 0)
    dealloc();

  // set up for given number of tracks with color info
  ntrk = n;
  cfrac = new double * [ntrk];
  cvec = new int * [ntrk];
  for (i = 0; i < ntrk; i++)
  {
    cfrac[i] = new double [cmax];      // ROYGBP-KXW
    cvec[i] = new int [cmax];
  }
}


//= Get rid of all locally allocated storage.

void jhcSurfObjs::dealloc ()
{
  int i;

  for (i = ntrk - 1; i >= 0; i--)
  {
    delete [] cvec[i];
    delete [] cfrac[i];
  }
  delete [] cvec;
  delete [] cfrac;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for depth-based segmentation.
// "lrel" and "ppel" used by jhcEliGrok::body_update to guess surface

int jhcSurfObjs::tall_params (const char *fname)
{
  jhcParam *ps = &zps;
  int ok;

  ps->SetTag("sobj_tall", 0);
  ps->NextSpecF( &lrel,  -2.0, "Default wrt lift height (in)");  
  ps->NextSpec4( &ppel, 400,   "Min peak in person map (pel)");      // was 200
  ps->NextSpecF( &flip,  12.0, "Max below wrt lift height (in)");  
  ps->NextSpecF( &sfar,  96.0, "Max intersect dist (in)");  
  ps->NextSpecF( &wexp,   1.0, "Map width expansion factor");        // was 1.2 then 0.8
  ps->NextSpec4( &pth,   40,   "Surface shape threshold");

  ps->NextSpec4( &cup,  150,   "Occlusion fill width (pel)");        // was 100
  ps->NextSpec4( &bej,    5,   "FOV edge shrinkage (pel)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for color-based segmentation.
// set "kdrop" negative to suppress color-based routines

int jhcSurfObjs::flat_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("sobj_flat", 0);
  ps->NextSpecF( &kdrop,   0.35, "Black cutoff wrt peak");     // was 0.5, 0.7, 0.9, then 0.4
  ps->NextSpecF( &wdrop,   0.0,  "White cutoff wrt peak");     // was 0.1 then 0.5
  ps->NextSpec4( &idev,   20,    "Color boundary ramp");   
  ps->NextSpec4( &csm,     5,    "Region smoothing (pel)");   
  ps->NextSpec4( &cth,    60,    "Region threshold");   
  ps->NextSpec4( &hole,  450,    "Background fill (pel)");

  ps->NextSpec4( &bgth,  180,    "BG shrink threshold");   
  ps->NextSpecF( &line,    7.0,  "Line aspect ratio");   
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

  ok &= tall_params(fname);
  ok &= flat_params(fname);
  ok &= jhcBumps::Defaults(fname);
  ok &= pp.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcSurfObjs::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= zps.SaveVals(fname);
  ok &= gps.SaveVals(fname);
  ok &= jhcBumps::SaveVals(fname);
  ok &= pp.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcSurfObjs::Reset ()
{
  // initialize object framework
  tcnt = 0;                                      // no "target" labels
  jhcBumps::Reset(1);

  // set image sizes
  gray.SetSize(map);
  cdet.SetSize(gray);
  bgnd.SetSize(gray);
  rim.SetSize(gray);
  gcc.SetSize(gray, 2);
  pat.SetSize(gray, 3);

  // initialize view adjustment
  xcomp = 0.0;
  ycomp = 0.0;
  pcomp = 0.0;

  // alternation of tall_objs and flat_objs
  phase = 0;                                     // negative runs both
  
  // color analysis
  pp.Reset();
}


//= Alter tracked object parameters to match new sensor pose.
// "loc" holds  position of camera and "dir" hold its (pan tilt roll) orientation
// location of camera always (0 0) with pan = 90 for map generation
// need to call this before FindObjects

void jhcSurfObjs::AdjTracks (const jhcMatrix& loc, const jhcMatrix& dir)
{
  double sx = loc.X(), sy = loc.Y(), pan = dir.P();
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

  // make camera always aimed upwards in map (compensate externally)
  // remember current sensor pose used for map generation
  SetCam(0, 0.0, 0.0, loc.Z(), 90.0, dir.T(), dir.R()); 
  xcomp = sx;
  ycomp = sy;
  pcomp = pan;
}


//= Find objects by analyzing supporting surface.
// need to call AdjTracks first (okay even during saccade)
// assumes "ztab" already holds expected height (e.g. jhcOverhead3D::PickPlane)
// ignores bounding box since sets jhcSmTrack::axes = 0
// returns number of raw objects found (not number being tracked)

int jhcSurfObjs::FindObjects (const jhcImg& col, const jhcImg& d16)
{
  double dz, yhit, rhit, sz = cz[0], tilt = t0[0];         // from SetCam

  // set up for later color analysis
  pp.SetSize(col);
  cmsk.SetSize(col, 1);                
  cmsk.FillArr(0);

  // find radial distance and width of beam at table
  dz = ztab - sz;
  yhit = dz / tan(D2R * tilt);
  yhit = __max(0.0, __min(yhit, sfar));
  rhit = sqrt(dz * dz + yhit * yhit);

  // adjust resolution and map offset
  mw = rhit * d16.XDim() / (kf * wexp);
  x0 = 0.5 * mw;
  ipp = mw / map.XDim();
  mh = ipp * map.YDim();
  y0 = 0.5 * mh - yhit;

  // get heights (map) relative to ztab and surface markings (pat)
  if (kdrop >= 0.0)
    Reproject2(pat, map, col, d16);
  else
    Reproject(map, d16);
  return Analyze();
}


///////////////////////////////////////////////////////////////////////////
//                              Segmentation                             //
///////////////////////////////////////////////////////////////////////////

//= Find potential objects based on depth (results in "cc" and "blob").
// always returns 1 to shift phase to flat_objs

int jhcSurfObjs::tall_objs ()
{
  int dev = ROUND(50.0 * htol / hobj);

jtimer(10, "tall_objs (bg2)");
  // find deviations from best plane fit 
  PlaneDev(det, map, 2.0 * hobj);                // uses srng

  // group protrusions (table = 128, hobj = 128 + 50)
  RampOver(obj, det, 178 - dev, 178 + dev);
  BoxAvg(obj, obj, sc, sc);
  CComps4(cc, obj, amin, sth);
  blob.FindParams(cc);

  // clean up basic planar surface
  InRange(top, det, 78, 178, dev);
  BoxThresh(top, top, sc, pth);
  ConvexUp(top, top, cup, 90);
  BeamEmpty(top, ztab, 2 * bej, 25);   

  // suppress components extending beyond table or depth cone
  blob.PoisonOver(cc, top, -50);
jtimer_x(10);
  return 1;
}


//= Find potential objects based on color difference (results in "cc" and "glob").
// needs tall_objs to run first to get "det" image of surface deviations
// always returns 0 to shift phase to tall_objs

int jhcSurfObjs::flat_objs ()
{
  jhcArr hist(256);
  int pk, shrink = 4 * (csm - 1) + 1, hsm = 13;

jtimer(11, "flat_objs (bg2)");
  // get grayscale pattern on surface
  Intensity(gray, pat);
  BandGate(gray, gray, det, 78, 178);

  // find intensity range of background    
  HistOver(wkhist, gray, gray, 0);
  wkhist.ASet(0, 0);
  hist.Boxcar(wkhist, hsm);
  wkhist.Boxcar(hist, hsm);
  pk  = wkhist.MaxBin();
  wk0 = wkhist.PeakRise(pk, kdrop) - 1;
  wk1 = wkhist.PeakFall(pk, wdrop) + 1;             

  // find anomalous regions
  InRange(cdet, gray, wk1 + idev, wk0 - idev, idev, 1);  
  BoxAvg(bgnd, cdet, csm);                             
  CComps4(gcc, bgnd, amin, cth);
  glob.FindParams(gcc);         

  // find solid background mask
  Threshold(bgnd, gray, 0);
  BoxThresh(bgnd, bgnd, csm, cth);
  FillHoles(bgnd, bgnd, hole);    

  // winnow detections
  BoxAvg(rim, bgnd, shrink);           
  glob.PoisonOver(gcc, rim, -bgth);     
  glob.RemBorder(gcc, 1);
  glob.AspectThresh(line, 0, 0, 1);     
jtimer_x(11);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Overrides                                //
///////////////////////////////////////////////////////////////////////////

//= Find candidate object pixels in overhead map assuming movable camera.
// uses image "map" to yield info in "cc" image and "blob" analyzer
// also computes "top" image representing table each time
// overrides "raw_objs" function from base class jhcBumps
// NOTE: tall_objs and flat_objs could be in separate threads if needed

void jhcSurfObjs::raw_objs (int trk)
{
jtimer(12, "raw_objs");
  if (kdrop < 0.0)
    tall_objs();                       // depth only
  else if (phase < 0)
  {
    tall_objs();                       // always do both
    flat_objs();
  }
  else if (phase < 1)                  // alternate (1:1 could be 2:1 or 3:1)
    phase += tall_objs();               
  else
    phase = flat_objs();               // zeroes phase after running once
jtimer_x(12);
}


//= Find maximum value inside some component given its bounding box.
// relative to local planar fit for better accuracy (cf. jhcOverhead3D::z_err)
// assumes last plane fit results (CoefX, CoefY, and Offset) still valid
// returns converted height value in inches (relative to table)

double jhcSurfObjs::find_hmax (int i, const jhcRoi *area) 
{
  double ipz = (zhi - zlo) / 252.0, sc = 4096.0 / ipz;
  int x0 = area->RoiX(), y0 = area->RoiY(), rw = area->RoiW(), rh = area->RoiH();
  int dx = ROUND(sc * ipp * CoefX()), dy = ROUND(sc * ipp * CoefY()); 
  int sum, sum0 = ROUND(sc * Offset()) + x0 * dx + y0 * dy + 2048;      
  int x, y, msk = map.RoiSkip(*area), csk = cc.RoiSkip(*area) >> 1;
  const US16 *c = (const US16 *) cc.RoiSrc(*area);
  const UC8 *m = map.RoiSrc(*area);

  pks.Fill(0);
  for (y = rh; y > 0; y--, c += csk, m += msk, sum0 += dy)
    for (sum = sum0, x = rw; x > 0; x--, c++, m++, sum += dx)
      if ((*c == i) && (*m > 1))                                     // ht 1 is invalid
        pks.AIncChk(*m - (sum >> 12), 1);
  return(ipz * pks.MaxBinN(pcnt));
}


///////////////////////////////////////////////////////////////////////////
//                          Object Properties                            //
///////////////////////////////////////////////////////////////////////////

//= Find tracked object closest to robot center in XY plane.
// returns index (not id) if successful, negative if none suitable

int jhcSurfObjs::Closest () const
{
  double dist, best;
  int i, n = pos.Limit(), win = -1;

  for (i = 0; i < n; i++)
    if (pos.Valid(i) > 0)
    {
      dist = DistXY(i);
      if ((win < 0) || (dist < best))
      {
        win = i;
        best = dist;
      }
    }
  return win;
}


//= Get full world coordinates of object with some index (not ID).
// returns planar orientation direction if successful, negative for problem

double jhcSurfObjs::World (jhcMatrix& loc, int i) const
{
  double tx, ty, ang, p90 = pcomp - 90.0, rads = D2R * p90, c = cos(rads), s = sin(rads);

  if ((loc.Vector(3) <= 0) || (pos.Valid(i) <= 0) || (pos.Valid(i) < 0))
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


//= Returns the distance in the XY plane to some particular object track index.
// returns inches from robot center, negative for problem

double jhcSurfObjs::DistXY (int i) const
{
  jhcMatrix loc(4);

  if (World(loc, i) < 0.0)
    return -1.0;
  return loc.PlaneVec3();  
}


//= Looks at some particular track index more closely to find current colors.
// afterward pp.ColorN, pp.AltColorN, and pp.QuantColor will be customized to this object
// needs original color image, set clr > 0 for nice looking mask image
// this processing is not automatic, instead must be called for each object as needed
// results stay with object track so okay unless never run before or lighting changes
// returns number of main colors found (zero or negative for error)

int jhcSurfObjs::Spectralize (const jhcImg& col, const jhcImg& d16, int i, int clr)
{
  double *wlh, *f;
  int *v;
  int side = 5, bot = -5, top = 15;
  int lab, cnum, cmax = pp.NumCols(), n = 0; 

  // sanity check then find connected component number
  if (!cmsk.SameSize(col, 3) || !cmsk.SameSize(d16, 2))
    return Fatal("Bad input to jhcSurfObjs::Spectralize");
  if (pos.Valid(i) <= 0)
    return -1;
  if ((lab = Component(i)) < 0)
    return 0;

  // set initial front projection ROI to be tracked box plus a little bit
  if (clr > 0)
    cmsk.FillMax(0);
  wlh = shp[i];
  ImgPrism(cmsk, pos.TX(i) + MDX(), pos.TY(i) + MY0(), pos.TZ(i), wlh[5], wlh[3], wlh[4], wlh[2]); 
  cmsk.PadRoi(side, bot, side, top);

  // make up pixel mask for object, ringed by black, and set tight ROI
  if (Flat(i) > 0)
    FrontMask(cmsk, d16, ztab - 2.0 * hobj, ztab + 2.0 * hobj, gcc, lab); 
  else
    FrontMask(cmsk, d16, ztab + hobj - htol, zmax, cc, lab);   
  cmsk.GrowRoi(1, 1);
  Border(cmsk, 1, 0);

  // analyze color for this object and cache results
  pp.FindColors(cmsk, col);
  f = cfrac[i];
  v = cvec[i];
  for (cnum = 0; cnum < cmax; cnum++)
  {
    f[cnum] = pp.AmtColor(cnum);
    if ((v[cnum] = pp.DegColor(cnum)) >= 2)
      n++;
  }
  return n;        // number of main colors
}


//= Retrieve cached qualitative color for track index.
// assumes Spectralize called on this track already
// returns 3 = primary, 2 = main, 1 = extras, 0 = none

int jhcSurfObjs::DegColor (int i, int cnum) const
{
  if ((cnum >= 0) && (cnum < pp.NumCols()))
    if (ObjOK(i))
      return (cvec[i])[cnum];
  return 0;
}


//= Retrieve cached fractional color for track index.
// assumes Spectralize called on this track already

double jhcSurfObjs::AmtColor (int i, int cnum) const
{
  if ((cnum >= 0) && (cnum < pp.NumCols()))
    if (ObjOK(i))
      return (cfrac[i])[cnum];
  return 0.0;
}


///////////////////////////////////////////////////////////////////////////
//                          Debugging Graphics                           //
///////////////////////////////////////////////////////////////////////////

//= Show objects that have interaction with reasoner in some color.
// green outlines around recent grounding args or volunteered statement
// magenta oultines around every object with an assigned node
// objects labelled by node nicknames (in "tag"), nothing if just enumerated
// assumes destination image is frontal camera 0 view at some scale

int jhcSurfObjs::AttnCam (jhcImg& dest, int most, int pick)
{
  int i, n = pos.Limit();

  // set up projection from main camera
  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcSurfObjs::AttnCam");
  AdjGeometry(0);

  // look for all tracked non-focal objects with semantic net links
  if (most >= 0)
    for (i = 0; i < n; i++)
      if ((pos.Valid(i) > 0) && (pos.state[i] <= 0) && (pos.tag[i][0] != '\0'))
        attn_obj(dest, i, most);

  // look for all tracked focus objects (draw last so cleanest)
  if (pick >= 0)
    for (i = 0; i < n; i++)
      if ((pos.Valid(i) > 0) && (pos.state[i] > 0))
        attn_obj(dest, i, pick);
  return 1;
}


//= Draw labelled box of some color around tracked object.
// assumes AdjGeometry(0) has already been called to set up camera transform

void jhcSurfObjs::attn_obj (jhcImg& dest, int i, int col) 
{
  jhcRoi box;
  double *wlh = shp[i];

  ImgPrism(box, pos.TX(i) + x0 - 0.5 * mw, pos.TY(i) + y0, pos.TZ(i), 
           wlh[5], wlh[3], wlh[4], wlh[2], ISC(dest)); 
  RectEmpty(dest, box, 3, -col);
  if (pos.tag[i][0] != '\0') 
    LabelBox(dest, box, pos.tag[i], -16, -col);
}

