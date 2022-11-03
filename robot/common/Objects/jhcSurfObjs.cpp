// jhcSurfObjs.cpp : find objects on surfaces using single mobile depth scanner
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020-2022 Etaoin Systems
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
  alt_cc = &gcc;
  wkhist.SetSize(256);
  pos.axes = 0;                                  // camera not stationary

  // maximum number of tracks
  ntrk = 0;
  SetCnt(n);

  // processing parameters for base and components
  SetFit(0.75, 2000, 0.5, 4.0, 4.0, 3.0, 100);
  pp.SetFind(3, 180, 35, 25, 245, 100, 50);
  pp.SetHue(250, 30, 49, 130, 175, 220);

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
  ps->NextSpecF( &sfar,  96.0, "Max intersect dist (in)");  
  ps->NextSpecF( &wexp,   1.0, "Map width expansion factor");        // was 1.2 then 0.8
  ps->NextSpec4( &pth,   40,   "Surface shape threshold");
  ps->Skip();
  ps->NextSpec4( &cup,  150,   "Occlusion fill width (pel)");        // was 100
  ps->NextSpec4( &bej,    5,   "FOV edge shrinkage (pel)");  

  ps->NextSpec4( &rmode,  2,   "Detection (depth, alt, both)");
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
  ps->NextSpec4( &idev,   30,    "Color boundary ramp");       // was 20
  ps->NextSpec4( &csm,     9,    "Region smoothing (pel)");    // was 5
  ps->NextSpec4( &cth,    50,    "Region threshold");          // was 60
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
  high.SetSize(map);

  // initialize view adjustment
  xcomp = 0.0;
  ycomp = 0.0;
  pcomp = 0.0;

  // set source of raw objects
  kdrop = fabs(kdrop);
  phase = -1;                // default to both
  if (rmode <= 0)
    kdrop = -kdrop;          // depth only
  if (rmode == 1)
    phase = 0;               // alternate (for speed)

  // color analysis
  pat.FillArr(100);          // in case no color analysis
  pp.Reset();
}


//= Alter tracked object parameters to compensate for robot base motion.
// dx0 is the shift to the right in the old frame and dy0 is the shift forward
// origin is at center of wheel base, dr is rotation around this (degs)
// NOTE: odometry only provides coarse adjustment, true tracking is more accurate

void jhcSurfObjs::AdjBase (double dx0, double dy0, double dr)
{
  double ang, wx, wy, wx2, wy2, rads = D2R * dr, c = cos(rads), s = sin(rads);
  int i, n = pos.Limit();

  for (i = 0; i < n; i++)
    if (pos.Valid(i) >= 0)
    {
      // same transform as jhcEliBase::AdjustXY
      ang = World(wx, wy, i);
      wx -= dx0;
      wy -= dy0;
      wx2 =  wx * c + wy * s;
      wy2 = -wx * s + wy * c;
      ForcePose(i, wx2, wy2, pos.TZ(i), ang - dr);
    }
}


//= Alter tracked object parameters to match new sensor pose.
// "loc" holds position of camera and "dir" hold its (pan tilt roll) orientation
// location of camera always (0 0) with pan = 90 for map generation
// need to call this before FindObjects

void jhcSurfObjs::AdjNeck (const jhcMatrix& loc, const jhcMatrix& dir)
{
  double sx = loc.X(), sy = loc.Y(), pan = dir.P(), dp = pan - pcomp;
  double r0 = D2R * (pcomp - 90.0), c0 = cos(r0), s0 = sin(r0);
  double r1 = -D2R * (pan - 90.0), c1 = cos(r1), s1 = sin(r1);
  double x0, y0, wx, wy, x1, tx, ty, y1, ang;
  int i, n = pos.Limit();

  // look for all valid and probationary tracks
  for (i = 0; i < n; i++)
    if (pos.Valid(i) >= 0)
    {
      // convert to invariant world position (cf. "FullXY") 
      x0 = pos.TX(i);
      y0 = pos.TY(i);
      wx = (x0 * c0 - y0 * s0) + xcomp;      
      wy = (x0 * s0 + y0 * c0) + ycomp;
 
      // convert back using new projection (cf. "ViewXY")
      tx = wx - sx;
      ty = wy - sy;
      x1 = tx * c1 - ty * s1;          
      y1 = tx * s1 + ty * c1;
      pos.ForceXYZ(i, x1, y1, pos.TZ(i));        // no z alteration needed

      // change ellipse orientation for current pan angle
      ang = shp[i][5] - dp;
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
// can optionally ignore all raw detections with non-zero masked pixels
// returns number of raw objects found (not number being tracked)

int jhcSurfObjs::FindObjects (const jhcImg& col, const jhcImg& d16, const jhcImg *mask)
{
  double dz, yhit, rhit, sz = cz[0], tilt = t0[0];         // from SetCam
  int nr;

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

  // possibly remove detections in invalid regions
  kill = mask;
  nr = Analyze();
  kill = NULL;
  return nr;
}


//= Find best top position and size in world coordinates for object with some index.
// thresholds current fitted depth map at "slice" down (inches) from highest point of object
// returns elongation direction of top part, negative for problem (e.g. occlusion)

double jhcSurfObjs::FullTop (double& wx, double& wy, double& wid, double& len, int i, double slice)
{
  double mx, my, mwid, mlen, mdir, cut = SizeZ(i) - slice;
  int lab = Component(i);

  // check for valid object then determine height threshold
  if ((lab < 0) || (slice <= 0.0))
    return -1.0;

  // get binary mask of highest portions of object
  if (Flat(i) > 0)
    high.CopyRoi(*(glob.ReadRoi(lab)));
  else
    high.CopyRoi(*(blob.ReadRoi(lab)));
  high.GrowRoi(1, 1);
  obj_slice(high, lab, __max(0.0, cut));

  // smooth regions then get statistics of biggest
  BoxAvg(high, high, sc, sc);
  if (Biggest(high, high, sth) <= 0)
    return -1.0;
  mdir = Ellipse(&mx, &my, &mwid, &mlen, high, high);

  // convert to robot relative world coordinates
  PelsXY(wx, wy, mx, my);
  wid = P2I(mwid);
  len = P2I(mlen);
  return FullOrient(mdir);
}


//= Create binary mask "up" inches above table for object with some detection label.
// relative to local planar fit for better accuracy (cf. jhcOverhead3D::z_err)
// assumes last plane fit results (CoefX, CoefY, and Offset) still valid

void jhcSurfObjs::obj_slice (jhcImg& dest, int lab, double up) const
{
  double ipz = (zhi - zlo) / 252.0, sc = 4096.0 / ipz, ht = Offset() + up;
  int x0 = dest.RoiX(), y0 = dest.RoiY(), rw = dest.RoiW(), rh = dest.RoiH();
  int dx = ROUND(sc * ipp * CoefX()), dy = ROUND(sc * ipp * CoefY()); 
  int sum, sum0 = ROUND(sc * ht) + x0 * dx + y0 * dy + 2048;
  int x, y, ssk = map.RoiSkip(dest), ssk2 = cc.RoiSkip(dest) >> 1;
  const US16 *c = (const US16 *) cc.RoiSrc(dest);
  const UC8 *m = map.RoiSrc(dest);
  UC8 *d = dest.RoiDest();

  for (y = rh; y > 0; y--, d += ssk, c += ssk2, m += ssk, sum0 += dy)
    for (sum = sum0, x = rw; x > 0; x--, d++, c++, m++, sum += dx)
      if ((*c == lab) && (*m >= (sum >> 12)))
        *d = 255;
      else
        *d = 0;
}


//= Find real-world table point closest to given object.
// object does not have to be in table image (e.g. held offscreen)
// returns 1 if okay, 0 if no table found, -1 if bad object

int jhcSurfObjs::NearTable (jhcMatrix& tpt, int i) const
{
  double wx, wy, tx, ty;
  int ix, iy, nx, ny;

  // find equivalent image location of tracked object
  if (World(wx, wy, i) < 0.0)
    return -1;
  ViewPels(ix, iy, wx, wy);

  // convert nearest table point into real-world coordinate vector
  if (NearPt(nx, ny, top, ix, iy, 50) < 0.0)
    return 0;
  PelsXY(tx, ty, nx, ny);
  tpt.SetVec3(tx, ty, ztab);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Overrides                                //
///////////////////////////////////////////////////////////////////////////

//= Find candidate object pixels in overhead map assuming movable camera.
// uses image "map" to yield info in "cc" image and "blob" analyzer
// also computes "top" image representing table each time
// tall_objs and flat_objs could be in separate threads if needed
// NOTE: overrides "raw_objs" function from base class jhcBumps

void jhcSurfObjs::raw_objs (int trk)
{
jtimer(13, "raw_objs (tall + flat)");
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
jtimer_x(13);
}


//= Find potential objects based on depth (results in "cc" and "blob").
// generates flattened overhead map "det" for fine separation of small objects
// always returns 1 to shift phase to flat_objs

int jhcSurfObjs::tall_objs ()
{
  int dev = ROUND(50.0 * htol / hobj);

jtimer(11, "tall_objs (bg2)");
  // find deviations from best plane fit (uses srng)
  PlaneDev(det, map, 2.0 * hobj);               

  // group protrusions (table = 128, hobj = 128 + 50)
  RampOver(obj, det, 178 - dev, 178 + dev);
  BoxAvg(obj, obj, sc, sc);
  CComps4(cc, obj, amin, sth);
  blob.FindParams(cc);

  // clean up basic planar surface
//  InRange(top, det, 78, 178, dev);
  InRange(top, det, 28, 228, dev);
  BoxThresh(top, top, sc, pth);
  ConvexUp(top, top, cup, 90);         // other shadow directions?
  BeamEmpty(top, ztab, 2 * bej, 25);   

  // suppress components extending beyond table or depth cone
  // also ignore detections from other invalid regions
  blob.PoisonOver(cc, top, -50);
  if (kill != NULL)
    blob.PoisonOver(cc, *kill);
jtimer_x(11);
  return 1;
}


//= Find potential objects based on color difference (results in "cc" and "glob").
// needs tall_objs to run first to get "det" image of surface deviations
// always returns 0 to shift phase to tall_objs

int jhcSurfObjs::flat_objs ()
{
  jhcArr hist(256);
  int pk, shrink = 4 * (csm - 1) + 1, hsm = 13;

jtimer(12, "flat_objs (bg2)");
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
  blob.MarkOver(bgnd, cc, 0, 0, 0);              // suppress tall object shadows
  Border(bgnd, -1, 0);

  // winnow detections
  BoxAvg(rim, bgnd, shrink);  
  glob.PoisonOver(gcc, rim, -bgth);     
  glob.RemBorder(gcc, 1);
  glob.ElongThreshBB(line, 0, 0, 1);             // was AspectThresh

  // ignore detection from invalid regions
  if (kill != NULL)
    glob.PoisonOver(gcc, *kill);
jtimer_x(12);
  return 0;
}


//= Find maximum value inside some component given its bounding box.
// relative to local planar fit for better accuracy (cf. jhcOverhead3D::z_err)
// assumes last plane fit results (CoefX, CoefY, and Offset) still valid
// returns converted height value in inches (relative to table)
// NOTE: overrides "find_hmax" function from base class jhcBumps

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
  double wx, wy, ang;

  if ((loc.Vector(3) <= 0) || (pos.Valid(i) < 0))  // probationary okay
    return -1.0;
  ang = World(wx, wy, i);
  loc.SetVec3(wx, wy, pos.TZ(i), 1.0);             // no z alteration needed
  return ang;
}


//= Get planar world coordinates in XY form for object with some index.
// returns planar orientation of object
// NOTE: better than jhcBumps::PosX and PosY since accounts for head pan

double jhcSurfObjs::World (double& wx, double& wy, int i) const
{
  if (pos.Valid(i) < 0)                // probationary okay
    return -1.0;
  FullXY(wx, wy, pos.TX(i), pos.TY(i));
  return FullOrient(Angle(i, 1));
}


//= Force the position and  planar angle of some object to be the specified value.
// mostly used when hand reorients while holding occluded object

void jhcSurfObjs::ForcePose (int i, double wx, double wy, double wz, double ang)
{
  double mx, my;

  if ((i < 0) || (i >= pos.Limit()))   // probationary okay
    return;
  ViewXY(mx, my, wx, wy);
  pos.ForceXYZ(i, mx, my, wz);         // no z alteration needed
  shp[i][5] = ViewOrient(ang);
}


//= Returns the distance in the XY plane to some particular object track index.
// returns inches from robot center, negative for problem

double jhcSurfObjs::DistXY (int i) const
{
  double wx, wy;

  if (pos.Valid(i) <= 0)
    return -1.0;
  FullXY(wx, wy, pos.TX(i), pos.TY(i));
  return sqrt(wx * wx + wy * wy);
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
    return -3;
  if ((lab = Component(i)) < 0)
    return -2;

  // set initial front projection ROI to be tracked box plus a little bit
  if (clr > 0)
    cmsk.FillMax(0);
  wlh = shp[i];
  if (ImgPrism(cmsk, pos.TX(i) + MDX(), pos.TY(i) + MY0(), pos.TZ(i), wlh[5], wlh[3], wlh[4], wlh[2]) <= 0)
    return -1;
  cmsk.PadRoi(side, bot, side, top);

  // make up pixel mask for object, ringed by black, and set tight ROI
  if (FrontMask(cmsk, d16, ztab - 2.0 * hobj, zmax, ((Flat(i) > 0) ? gcc : cc), lab) <= 0)
    return 0;
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
//                        Coordinate Transforms                          //
///////////////////////////////////////////////////////////////////////////

//= Adjust object detection coordinates (inches) for current sensor pose.
// similar to old fcn World (double& wx, double& wy, double tx, double ty, double tdir)
// NOTE: mx and my are in inches, not pixels (cf. PelsXY)

void jhcSurfObjs::FullXY (double& wx, double& wy, double mx, double my) const
{
  double rads = D2R * (pcomp - 90.0), c = cos(rads), s = sin(rads);

  wx = (mx * c - my * s) + xcomp;          
  wy = (mx * s + my * c) + ycomp;
}


//= Convert some overhead map image location (pels) into planar real-world coordinates (inches).
// useful for determining a real-world object deposit point on a surface with wz = ztab
// used to be called ClickWorld

void jhcSurfObjs::PelsXY (double& wx, double& wy, double ix, double iy) const
{
  FullXY(wx, wy, M2X(ix), M2Y(iy));
}


//= Adjust object angle (degrees) for current sensor pose.

double jhcSurfObjs::FullAngle (double mdir) const
{
  double p90 = pcomp - 90.0, wdir = mdir + p90;

  if (wdir > 180.0)
    wdir -= 360.0;
  else if (wdir <= -180.0)
    wdir += 360.0;
  return wdir;
}


//= Adjust object orientation (degrees) for current sensor pose.
// assumes orientation in (0 180) and returns equivalent in (0 180)

double jhcSurfObjs::FullOrient (double mdir) const
{
  double wdir = FullAngle(mdir);

  if (wdir < 0.0)
    wdir += 180.0;
  return wdir;
}


//= Adjust real-world position to give location in current map view (inches).
// NOTE: mx and my are in inches, not pixels (cf. ViewPels)

void jhcSurfObjs::ViewXY (double& mx, double& my, double wx, double wy) const
{
  double rads = -D2R * (pcomp - 90.0), c = cos(rads), s = sin(rads);
  double tx = wx - xcomp, ty = wy - ycomp;

  mx = tx * c - ty * s;          
  my = tx * s + ty * c;
}


//= Convert some real-world location (inches) to a pixel location in the current object map.
// takes into account automatic setting for projection, useful for showing where arm is
// used to be called MapCoords

void jhcSurfObjs::ViewPels (double& ix, double& iy, double wx, double wy) const
{
  double mx, my;

  ViewXY(mx, my, wx, wy);
  ix = W2X(mx);
  iy = W2Y(my);
}


//= Convert real-world location (inches) to integer pixel location in map.
// more convenient for functions like jhcDraw::DrawPoly
// used to be called MapPels

void jhcSurfObjs::ViewPels (int& ix, int& iy, double wx, double wy) const
{
  double fx, fy;

  ViewPels(fx, fy, wx, wy);
  ix = ROUND(fx);
  iy = ROUND(fy);
}


//= Adjust real-world angle to give angle in current map view (degrees).

double jhcSurfObjs::ViewAngle (double wdir) const
{
  double p90 = pcomp - 90.0, mdir = wdir - p90;

  if (mdir > 180.0)
    mdir -= 360.0;
  else if (mdir < 0.0)
    mdir += 360.0;
  return mdir;
}


//= Adjust real-world orientation to give direction in current map view.
// assumes orientation in (0 180) and returns result also in (0 180)

double jhcSurfObjs::ViewOrient (double wdir) const
{
  double mdir = ViewAngle(wdir);

  if (mdir < 0.0)
    mdir += 180.0;
  return mdir;
}


///////////////////////////////////////////////////////////////////////////
//                          Debugging Graphics                           //
///////////////////////////////////////////////////////////////////////////

//= Show objects that have interaction with reasoner in some color.
// green (pick) outlines around recent grounding args or volunteered statement
// yellow (known) outlines around every object with an assigned node
// thin magenta (all) outlines around detected object
// objects labelled by node nicknames (in "tag"), nothing if just enumerated
// assumes destination image is frontal camera 0 view at some scale

int jhcSurfObjs::AttnCam (jhcImg& dest, int pick, int known, int all)
{
  int i, n = pos.Limit();

  // set up projection from main camera
  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcSurfObjs::AttnCam");
  AdjGeometry(0);

  // show all detected objects (tall and flat)
  if (all >= 0)
    for (i = 0; i < n; i++)
      if (pos.Valid(i) > 0)
        attn_obj(dest, i, 1, all);

  // look for all tracked non-focal objects with semantic net links
  if (known >= 0)
    for (i = 0; i < n; i++)
      if ((pos.Valid(i) > 0) && (pos.state[i] <= 0) && (pos.tag[i][0] != '\0'))
        attn_obj(dest, i, 3, known);

  // look for all tracked focus objects (draw last so cleanest)
  if (pick >= 0)
    for (i = 0; i < n; i++)
      if ((pos.Valid(i) > 0) && (pos.state[i] > 0))
        attn_obj(dest, i, 3, pick);
  return 1;
}


//= Draw labelled box of some color around tracked object.
// assumes AdjGeometry(0) has already been called to set up camera transform

void jhcSurfObjs::attn_obj (jhcImg& dest, int i, int t, int col) 
{
  jhcRoi box;
  double *wlh = shp[i];

  ImgPrism(box, pos.TX(i) + x0 - 0.5 * mw, pos.TY(i) + y0, pos.TZ(i), 
           wlh[5], wlh[3], wlh[4], wlh[2], ISC(dest)); 
  RectEmpty(dest, box, t, -col);
  if (pos.tag[i][0] != '\0') 
    LabelBox(dest, box, pos.tag[i], -16, -col);
}


//= Mark the camera image approximately where a 3D point would be (inches).

int jhcSurfObjs::MarkCam (jhcImg& dest, const jhcMatrix& wpt, int col)
{
  double mx, my, ix, iy;

  // set up projection from main camera
  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcSurfObjs::MarkCam");
  AdjGeometry(0);

  // find projection of point from robot-centric coordinates
  ViewXY(mx, my, wpt.X(), wpt.Y());
  ImgPt(ix, iy, mx + x0 - 0.5 * mw, my + y0, wpt.Z(), ISC(dest));
  XMark(dest, ix, iy, 17, 3, -col);
  return 1;
}
