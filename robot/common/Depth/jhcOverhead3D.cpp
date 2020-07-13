// jhcOverhead3D.cpp : combine depth sensors into an overhead height map
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2016-2020 IBM Corporation
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

#include <stdio.h>
#include <string.h>

#include "Interface/jhcMessage.h"

#include "Depth/jhcOverhead3D.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcOverhead3D::~jhcOverhead3D ()
{
  dealloc();
}


//= Get rid of all current camera structures.

void jhcOverhead3D::dealloc ()
{
  // mark no arrays
  if (smax <= 0)
    return;
  smax = 0;

  // restriction regions
  delete [] ry;
  delete [] rx;
  delete [] rps;

  // interpretation elements
  delete [] used;

  // camera parameters
  delete [] dev;
  delete [] rmax;
  delete [] r0;
  delete [] t0;
  delete [] p0;
  delete [] cz;
  delete [] cy;
  delete [] cx;
  delete [] cps;
}


//= Default constructor initializes certain values.

jhcOverhead3D::jhcOverhead3D (int ncam)
{
  // determine how many cameras will be used
  smax = 0;
  AllocCams(ncam);
  strcpy_s(name, "ov3");

  // set up processing parameters (make sure some sensor is valid)
  SetMap(144.0, 144.0, 72.0, 72.0, 0.0, 8.0, 0.2, 42.0);
  SetFit(4.0, 10000, 2.0, 3.0, 4.0, 2.0);
  SrcSize();
  Defaults();
  Reset();
}


//= Make structures for however many input sensors will be used.

void jhcOverhead3D::AllocCams (int ncam)
{
  int i, s4, n = 0;

  // clear out old stuff then save new size
  if (ncam == smax) 
    return;
  dealloc();
  smax = __max(1, __min(ncam, 12));
  s4 = 4 * smax;

  // camera parameters
  cps  = new jhcParam[smax];
  cx   = new double [smax];
  cy   = new double [smax];
  cz   = new double [smax];
  p0   = new double [smax];
  t0   = new double [smax];
  r0   = new double [smax];
  rmax = new double [smax];
  dev  = new int [smax];

  // interpretation elements
  used = new int[smax];

  // fill in cameras with default values
  for (i = 0; i < smax; i++, n--)
    SetCam(i, -66.0, 0.0, 90.0, 0.0, -18.0, 180.0, 192.0, n);

  // calibration adjustment regions
  rps = new jhcParam[smax];
  rx = new int [s4];
  ry = new int [s4];

  // fill in as empty restrictions
  for (i = 0; i < s4; i++)
  {
    rx[i] = -1;
    ry[i] = -1;
  }
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for input camera analysis.

int jhcOverhead3D::cam_params (int n, const char *fname)
{
  char tag[40];
  jhcParam *ps = cps + n;
  int ok;

  // customize for some camera
  if ((n < 0) || (n >= smax))
    return 0;
  ps->SetTitle("Kinect %d Geometry", n);
  sprintf_s(tag, "%s_cam%d", name, n);

  // set up parameters
  ps->SetTag(tag, 0);
  ps->NextSpecF( cx + n,   "X position (in)");  
  ps->NextSpecF( cy + n,   "Y position (in)");  
  ps->NextSpecF( cz + n,   "Height above floor (in)");  
  ps->NextSpecF( p0 + n,   "Pan wrt X axis (deg)");  
  ps->NextSpecF( t0 + n,   "Tilt wrt ceiling (deg)");  
  ps->NextSpecF( r0 + n,   "Roll wrt floor (deg)");  

  ps->NextSpecF( rmax + n, "Max range to plot (in)");
  ps->NextSpec4( dev + n,  "Device number");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Image corners of input camera fine adjustment regions.

int jhcOverhead3D::flat_params (int n, const char *fname)
{
  char tag[40];
  jhcParam *ps = rps + n;
  int ok, n4 = 4 * n;
  int *x = rx + n4, *y = ry + n4;

  // customize for some camera
  if ((n < 0) || (n >= smax))
    return 0;
  ps->SetTitle("Kinect %d Area", n);
  sprintf_s(tag, "%s_flat%d", name, n);

  // set up parameters
  ps->SetTag(tag, 0);
  ps->NextSpec4( x,     "X0 corner (pel)");  
  ps->NextSpec4( y,     "Y0 corner (pel)"); 
  ps->NextSpec4( x + 1, "X1 corner (pel)");  
  ps->NextSpec4( y + 1, "Y1 corner (pel)"); 
  ps->NextSpec4( x + 2, "X2 corner (pel)");  
  ps->NextSpec4( y + 2, "Y2 corner (pel)"); 

  ps->NextSpec4( x + 3, "X3 corner (pel)");  
  ps->NextSpec4( y + 3, "Y3 corner (pel)"); 
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for specifying absolute overhead map.

int jhcOverhead3D::map_params (const char *fname)
{
  char tag[40];
  jhcParam *ps = &mps;
  int ok;

  sprintf_s(tag, "%s_map", name);
  ps->SetTag(tag, 0);
  ps->NextSpecF( &mw,    "Full map width (in)");  
  ps->NextSpecF( &mh,    "Full map height (in)");  
  ps->NextSpecF( &x0,    "X zero offset (in)");  
  ps->NextSpecF( &y0,    "Y zero offset (in)");  
  ps->NextSpecF( &zlo,   "Lowest Z wrt surface (in)");  
  ps->NextSpecF( &zhi,   "Highest Z wrt surface (in)");  

  ps->NextSpecF( &ipp,   "Map pixel resolution (in)");  
  ps->NextSpecF( &ztab0, "Expected surface ht (in)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  ztab = ztab0;
  return ok;
}


//= Parameters used for testing whether plane fit is valid.

int jhcOverhead3D::plane_params (const char *fname)
{
  char tag[40];
  jhcParam *ps = &pps;
  int ok;

  sprintf_s(tag, "%s_plane", name);
  ps->SetTag(tag, 0);
  ps->NextSpecF( &srng,  "Surface search range (in)");
  ps->NextSpec4( &npts,  "Min points in estimate");  
  ps->NextSpecF( &rough, "Max surface std dev (in)");  
  ps->NextSpecF( &dt,    "Max surface tilt (deg)");  
  ps->NextSpecF( &dr,    "Max surface roll (deg)");  
  ps->NextSpecF( &dh,    "Max surface offset (in)");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Utilities                         //
///////////////////////////////////////////////////////////////////////////

//= Set all map parameters in order that they appear in configuration file.

void jhcOverhead3D::SetMap (double w, double h, double x, double y, 
                            double lo, double hi, double pel, double ht)
{
  mw   = w;
  mh   = h;
  x0   = x;
  y0   = y;
  zlo  = lo;
  zhi  = hi;
  ipp  = pel;
  ztab = ht;
}


//= Set all parameters for plane fitting in order that they appear in configuration file.

void jhcOverhead3D::SetFit (double d, int n, double e, double t, double r, double h)
{
  srng = d;
  npts = n;
  rough = e;
  dt = t;
  dr = r;
  dh = h;
}


//= Set all parameters of a camera in order that they appear in configuration file.

void jhcOverhead3D::SetCam (int n, double x, double y, double z, double pan, double tilt, 
                            double roll, double rng, int dnum)
{
  if ((n < 0) || (n >= smax))
    return;
  cx[n]   = x;
  cy[n]   = y;
  cz[n]   = z;
  p0[n]   = pan;
  t0[n]   = tilt;
  r0[n]   = roll;
  rmax[n] = rng;
  dev[n]  = dnum;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcOverhead3D::Defaults (const char *fname)
{
  int ok = 1;

  ok &= LoadCfg(fname);
  ok &= plane_params(fname);
  return ok;
}


//= Read all geometric calibration values from a file.

int jhcOverhead3D::LoadCfg (const char *fname)
{
  int i, ok = 1;

  // clear camera devices and regions
  for (i = 0; i < smax; i++)
  {
    dev[i] = -1;
    rx[4 * i] = -1;
  }

  // get new values from file
  for (i = 0; i < smax; i++)
    cam_params(i, fname);            // may be fewer than smax
  for (i = 0; i < smax; i++)
    ok &= flat_params(i, fname);
  ok &= map_params(fname);
  return ok;
}


//= Write current processing variable values to a file.
// can optionally save camera calibration for non-negative devices

int jhcOverhead3D::SaveVals (const char *fname, int geom) const
{
  int ok = 1;

  ok &= SaveCfg(fname, geom);
  ok &= pps.SaveVals(fname);
  return ok;
}


//= Write current geometric calibration values to a file.
// can optionally save camera calibration for all devices

int jhcOverhead3D::SaveCfg (const char *fname, int geom) const
{
  int i, ok = 1;

  if (geom > 0)
  {
    // store valid devices but erase other lines
    for (i = 0; i < smax; i++)
      if (dev[i] >= 0)
        ok &= cps[i].SaveVals(fname);   
      else
        cps[i].RemVals(fname); 

    // store true restrictions but erase other lines
    for (i = 0; i < smax; i++)
      if ((dev[i] >= 0) && (Restricted(i) > 0))
        ok &= rps[i].SaveVals(fname);    
      else
        rps[i].RemVals(fname);
  }
  ok &= mps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Camera Utilities                            //
///////////////////////////////////////////////////////////////////////////

//= Copy camera geometry from some other instance (or similar class).

void jhcOverhead3D::CopyCams (const jhcOverhead3D& ref)
{
  int i, lim = __min(smax, ref.smax), lim4 = 4 * lim;

  // camera geometry
  for (i = 0; i < lim; i++)
  {
    cx[i]   = ref.cx[i];
    cy[i]   = ref.cy[i];
    cz[i]   = ref.cz[i];
    p0[i]   = ref.p0[i];
    t0[i]   = ref.t0[i];
    r0[i]   = ref.r0[i];
    rmax[i] = ref.rmax[i];
    dev[i]  = ref.dev[i];
  }

  // tilt estimation regions
  for (i = 0; i < lim4; i++)
  {
    rx[i] = ref.rx[i];
    ry[i] = ref.ry[i];
  }
}


//= Fill vector with {x y z} location of camera N.

int jhcOverhead3D::DumpLoc (jhcMatrix& loc, int cam) const
{
  if ((cam < 0) || (cam >= smax) || !loc.Vector(3))
    return Fatal("Bad input to jhcOverhead3D::DumpLoc");
  loc.SetVec3(cx[cam], cy[cam], cz[cam]);
  return 1;
}


//= Unpack vector of location {x y z} into camera N.

int jhcOverhead3D::LoadLoc (int cam, const jhcMatrix& loc) 
{
  if ((cam < 0) || (cam >= smax) || !loc.Vector(3))
    return Fatal("Bad input to jhcOverhead3D::LoadLoc");
  cx[cam] = loc.X();
  cy[cam] = loc.Y();
  cz[cam] = loc.Z();
  return 1;
}


//= Fill vector with {x y z pan tilt roll} of camera N.

int jhcOverhead3D::DumpPose (jhcMatrix& pose, int cam) const
{
  if ((cam < 0) || (cam >= smax) || !pose.Vector(6))
    return Fatal("Bad input to jhcOverhead3D::DumpPose");
  pose.SetX(cx[cam]);
  pose.SetY(cy[cam]);
  pose.SetZ(cz[cam]);
  pose.VSet(3, p0[cam]);
  pose.VSet(4, t0[cam]);
  pose.VSet(5, r0[cam]);
  return 1;
}


//= Unpack vector of {x y z pan tilt roll} into camera N.

int jhcOverhead3D::LoadPose (int cam, const jhcMatrix& pose) 
{
  if ((cam < 0) || (cam >= smax) || !pose.Vector(6))
    return Fatal("Bad input to jhcOverhead3D::LoadPose");
  cx[cam] = pose.X();
  cy[cam] = pose.Y();
  cz[cam] = pose.Z();
  p0[cam] = pose.VRef(3);
  t0[cam] = pose.VRef(4);
  r0[cam] = pose.VRef(5);
  return 1;
}


//= Tell the number of sensors actually in use.

int jhcOverhead3D::Active () const
{
  int i, n = 0;

  for (i = 0; i < smax; i++)
    if (dev[i] >= 0)
      n++;
  return n;
}


//= Tell the first active camera in list.

int jhcOverhead3D::FirstCam () const
{
  int i;

  for (i = 0; i < smax; i++)
    if (dev[i] >= 0)
      return i;
  return 0;
}


//= Tell the last active camera in list.

int jhcOverhead3D::LastCam () const
{
  int i, n = 0;

  for (i = 0; i < smax; i++)
    if (dev[i] >= 0)
      n = i;
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                           Image Normalization                         //
///////////////////////////////////////////////////////////////////////////

//= Sensor is operating in portrait rather than normal landscape mode.
// does not test for upside-down image of same size

bool jhcOverhead3D::Sideways (int cam) const
{
  return((cam >= 0) && (cam < smax) && 
         (fabs(r0[cam]) > 45.0) && (fabs(r0[cam]) <= 135.0));
}


//= Checks if image needs no correction.
// tests for downsizing and any rotation (sideways or upside-down)

bool jhcOverhead3D::AlreadyOK (const jhcImg& ref, int cam, int big) const
{
  return(((big > 0) || (ref.YDim() <= 600)) &&
         ((cam < 0) || (cam >= smax) || (fabs(r0[cam]) <= 45.0) || 
          ((dev[cam] >= 20) && (fabs(r0[cam]) > 135.0))));          // Kinect2
}


//= Configure destination image size properly if reference is rotated 90 degrees.
// big: 1 = possibly very large (1920x1080), 0 = same as depth (960x540), -1 = small (480x270)

void jhcOverhead3D::RollSize (jhcImg& dest, const jhcImg& ref, int cam, int fields, int big) const
{
  int w = ref.XDim(), h = ref.YDim();

  // possibly downsize hi-res Kinect image
  if ((big <= 0) && (h > 600))
  {
    w /= 2;
    h /= 2;
  }

  // possibly generate smaller sample sized image
  if (big < 0)
  {
    w /= 2;
    h /= 2;
  }

  // swap from landscape to portrait if needed
  if (Sideways(cam))
    dest.SetSize(h, w, ((fields > 0) ? fields : ref.Fields()));
  else
    dest.SetSize(w, h, ((fields > 0) ? fields : ref.Fields()));
}


//= Rotates an image to produce version that compensates for camera roll. 
// big: 1 = possibly very large (1920x1080), 0 = same as depth (960x540), -1 = small (480x270)
// dest is always holds corrected image and is resized as needed

jhcImg *jhcOverhead3D::Correct (jhcImg& dest, const jhcImg& src, int cam, int big)
{
  double roll = (((cam >= 0) && (cam < smax)) ? r0[cam] : 0.0);
  int kin = (((cam >= 0) && (cam < smax)) ? dev[cam] : 0);

  // simplest case of no rotation
  RollSize(dest, src, cam, 0, big);
  if (fabs(roll) <= 45.0)
  {
    SmoothN(dest, src);
    return &dest;
  }

  // just flipped over (Kinect2 driver might have handled this)
  if ((roll > 135.0) || (roll < -135.0))
  {
    SmoothN(dest, src);
    if (kin < 20)            
      UpsideDown(dest);
    return &dest;
  }

  // resize first then rotate for sideways
  ctmp.SetSize(dest.YDim(), dest.XDim(), dest.Fields());
  SmoothN(ctmp, src);
  if ((roll > 45.0) && (roll <= 135.0))
    RotateCW(dest, ctmp);  
  else 
    RotateCCW(dest, ctmp);  
  return &dest;
}


//= Normalize roll for upside down and sideways cameras.

double jhcOverhead3D::ImgRoll (int cam) const
{
  double roll;
  int n = __max(0, __min(cam, smax - 1));

  roll = r0[n];
  if (roll > 135.0)
    roll -= 180.0;
  else if (roll >= 45.0)
    roll -= 90.0;
  else if (roll < -135.0)
    roll += 180.0;
  else if (roll <= -45.0)
    roll += 90.0; 
  return roll;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Configure system to accept images of given size.
// also takes Kinect geometric parameters

void jhcOverhead3D::SrcSize (int w, int h, double f, double sc)
{
  double tmp;

  // set sizes and depth interpretation
  SetOptics(f, sc);

  // default rightways up values
  SetSize(w, h, 1);                
  hfov = 2.0 * R2D * atan2(0.5 * w, f);
  vfov = 2.0 * R2D * atan2(0.5 * h, f);

  // Kinect2 depth is narrower than color
  if (h > 500)
    hfov *= 0.78;

  // correct input image size and field-of-view (for graphics)
  if (Sideways())
  {
    SetSize(h, w, 1);                
    tmp = hfov;
    hfov = vfov;
    vfov = tmp;
  }
}


//= Reset state for the beginning of a sequence.
// adjusts map size, should call SrcSize before this

void jhcOverhead3D::Reset ()
{
  map.SetSize(PELS(mw), PELS(mh), 1);
  map2.SetSize(map);
  ztab = ztab0;
  rasa = 1;
}


//= Add a rightway-up depth sensor image to the accumulated map.
// can mark multiple cells for long ranges using zst > 0
// use Correct to flip image if needed first

int jhcOverhead3D::Ingest (const jhcImg& d16, double bot, double top, int cam, int zst, double zlim)
{
  double zcut = ((zlim > 0.0) ? zlim : 84.0);              // ignore above this height
  int i, n = __max(0, __min(cam, smax - 1));

  if (!d16.Valid(2))
    return Fatal("Bad input to jhcOverhead3D::Ingest");

  // set static camera parameters
  SetCamera(cx[n] + x0 - 0.5 * mw, cy[n] + y0, cz[n]);
  SetView(p0[n] - 90.0, t0[n], ImgRoll(n));

  // get projection from this camera 
  SetProject(ztab + bot, ztab + top, zcut, ipp, rmax[n]);  // zcut was ztab + top
  FloorMap2(map, d16, rasa, zst); 

  // remember which sensors have been used (for graphics)
  if (rasa > 0)
  {
    for (i = 0; i < smax; i++)
      used[i] = 0;
    rasa = 0;
  }
  used[n] = 1;
  return 1;  
}


//= Add a rightway-up depth sensor image to a new blank map.
// differs from Ingest in that camera projections are combined later
// can mark multiple cells for long ranges using zst > 0
// use Correct to flip image if needed first

int jhcOverhead3D::Reproject (jhcImg& dest, const jhcImg& d16, double bot, double top, int cam, int zst, double zlim, int clr)
{
  double zcut = ((zlim > 0.0) ? zlim : 84.0);              // ignore above this height
  int i, n = __max(0, __min(cam, smax - 1));

  if (!d16.Valid(2) || !map.SameFormat(dest))
    return Fatal("Bad input to jhcOverhead3D::Reproject");

  // set static camera parameters
  SetCamera(cx[n] + x0 - 0.5 * mw, cy[n] + y0, cz[n]);
  SetView(p0[n] - 90.0, t0[n], ImgRoll(n));

  // get projection from this camera 
  SetProject(ztab + bot, ztab + top, zcut, ipp, rmax[n]);  // zcut was ztab + top
  FloorMap2(dest, d16, clr, zst); 

  // remember which sensors have been used (for graphics)
  for (i = 0; i < smax; i++)
    used[i] = 0;
  used[n] = 1;
  rasa = 0;
  return 1;  
}


//= Fill in missing parts of surface for highly oblique views.
// call after Ingest done, image is stored in member variable "map2"

void jhcOverhead3D::Interpolate (int sc, int pmin)
{
  NZBoxMax(map2, map, sc, sc, pmin);
//  NZBoxAvg(map2, map, sc, sc, pmin);
//  MaxFcn(map2, map2, map);
}


///////////////////////////////////////////////////////////////////////////
//                             Plane Fitting                             //
///////////////////////////////////////////////////////////////////////////

//= Tell if any restriction area for this camera.
// a negative coordinate for any corner invalidates region

int jhcOverhead3D::Restricted (int cam) const
{
  int i, n = __max(0, __min(cam, smax - 1)), n4 = 4 * n;
  int *x = rx + n4, *y = ry + n4;

  for (i = 0; i < 4; i++)
    if ((x[i] < 0) || (y[i] < 0))
      return 0;
  return 1;
}


//= Guess orientation and height for a particular camera.
// returns plane fitting error (in inches), negative for problem

double jhcOverhead3D::EstPose (double& t, double& r, double& h, const jhcImg& d16, 
                               int cam, double ztol)
{
  int n = __max(0, __min(cam, smax - 1)), n4 = 4 * n;
  int *x = rx + n4, *y = ry + n4;
  const jhcImg *src = &d16;

  // keep only estimation region (if defined)
  if (Restricted(cam) > 0)
  {
    dmsk.SetSize(d16);
    mask.SetSize(d16, 1);
    fill.PolyFill(mask, x, y, 4); 
    OverGate(dmsk, d16, mask);
    src = &dmsk;
  }

  // process single depth image (map2) with different zlo and zhi
  rasa = 1;
  Ingest(*src, -ztol, ztol, cam);
  Interpolate();
  return CamCalib(t, r, h, map2, ztab, ztol, ztab - ztol, ztab + ztol, ipp);
}


//= Show plane fitting errors from last estimation.
// assumes projection in "map2" and that "zlo" and "zhi" were over-ridden

int jhcOverhead3D::EstDev (jhcImg& devs, double dmax, double ztol)
{
  if (!devs.SameFormat(map))
    return Fatal("Bad images to jhcOverhead3D::EstDev");  
  devs.FillArr(0); 
  return surf_err(devs, map2, dmax, ztab - ztol, ztab + ztol);
}


//= Fit plane to points with +/- search of surface then note pixel-by-pixel deviations.
// fit must have at least "npt" and adjustments must be less then "dt", "dr", and "dh"
// surface estimation points must have standard deviation less than "rough" inches
// returns 1 if successful, 0 if bad fit

int jhcOverhead3D::PlaneDev (jhcImg& devs, const jhcImg& hts, double dmax, double search)
{
  double std, t, r, h, sdev = ((search > 0.0) ? search : srng);

  if (!devs.SameFormat(map) || !hts.SameFormat(map))
    return Fatal("Bad images to jhcOverhead3D::PlaneDev");
  devs.FillArr(0);
  std = CamCalib(t, r, h, hts, ztab, sdev, zlo, zhi, ipp);
  if ((Pts() < npts) || (std > rough) || (fabs(t) > dt) || (fabs(r) > dr) || (fabs(h) > dh))
    return 0;
  return surf_err(devs, hts, dmax, zlo, zhi);
}


//= Use fitting coefficients to find plane height at every pixel and get absolute difference.
// ignores input heights of 0 (unknown) and writes 0 to output (otherwise 1-255 valid) 
// marks below plane by dmax = 28, on plane = 128 above plane by dmax = 228
// assumes hts map was created with zlo = lo and zhi = hi, returns 1 for convenience
// <pre>
// wz0 = a * wx + b * wy + c where coefficients come from jhcPlaneEst
//   wx, wy, and wz are in inches, where wx, wy are from lower left corner
//   wx = x * ipp, wy = y * ipp, wz = z * ipz  where ipz = (zhi - zlo) / 252
// dev = k * (wz - wz0) + 128  where k = 100 / dmax
//     = k * (wz - [a * wx + b * wy + c]) + 128
//     = k * (ipz * z - [a * ipp * x + b * ipp * y + c]) + 128
//     = (k * ipz) * z - [(k * ipp * a) * x + (k * ipp * b) * y + (k * c - 128)]
// </pre>

int jhcOverhead3D::surf_err (jhcImg& devs, const jhcImg& hts, double dmax, double lo, double hi) const
{ 
  double k = 100.0 / dmax, sc = 4096.0 * k * ipp;
  int sum, dx = ROUND(sc * CoefX()), dy = ROUND(sc * CoefY()); 
  int sum0 = ROUND(4096.0 * (k * Offset() - 128.0)) + 2048;        // for final rounding
  int dz, zsc = ROUND(4096.0 * k * (hi - lo) / 252.0);
  int x, y, rw = hts.XDim(), rh = hts.YDim(), sk = hts.Skip();
  const UC8 *m = hts.PxlSrc();
  UC8 *d = devs.PxlDest();

  for (y = rh; y > 0; y--, d += sk, m += sk, sum0 += dy)
   for (sum = sum0, x = rw; x > 0; x--, d++, m++, sum += dx)
     if (*m > 0)
     {
       dz = (zsc * (*m) - sum) >> 12;
       dz = __max(1, __min(dz, 255));
       *d = (UC8) dz;
     }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Debugging Graphics                           //
///////////////////////////////////////////////////////////////////////////

//= Set up geometric transform from a particular sensor.
// useful for higher-level graphics functions

void jhcOverhead3D::AdjGeometry (int cam)
{
  int n = __max(0, __min(cam, smax - 1));

  SetCamera(cx[n] + x0 - 0.5 * mw, cy[n] + y0, cz[n]);
  SetView(p0[n] - 90.0, t0[n], ImgRoll(n));
  BuildMatrices();
}


//= Show all active sensors as crosses on overhead map.

int jhcOverhead3D::ShowCams (jhcImg& dest, int t, int r, int g, int b) const
{
  int i;

  if (!dest.Valid(1, 3) || !map.SameSize(dest))
    return Fatal("Bad images to jhcOverhead3D::ShowCams");

  for (i = 0; i < smax; i++)
    if (used[i] > 0)
      CamLoc(dest, i, t, r, g, b);
  return 1;
}


//= Show the location of some particular camera on overhead map.

int jhcOverhead3D::CamLoc (jhcImg& dest, int cam, int t, int r, int g, int b) const
{
  int n = __max(0, __min(cam, smax - 1));
  double w = 11.0, d = 3.0;
  double ppi = 1.0 / ipp, x = ppi * (cx[n] + x0), y = ppi * (cy[n] + y0);
  double rads = D2R * p0[n], c = cos(rads), s = sin(rads), hw = ppi * 0.5 * w;

  if (!dest.Valid(1, 3) || !map.SameSize(dest))
    return Fatal("Bad images to jhcOverhead3D::CamLoc");

  DrawLine(dest, x, y, x - ppi * d * c, y - ppi * d * s, t, r, g, b);
  DrawLine(dest, x - hw * s, y + hw * c, x + hw * s, y - hw * c, t, r, g, b);
  return 1;
}


//= Show depth zones of all active sensors.

int jhcOverhead3D::ShowZones (jhcImg& dest, int t, int r, int g, int b) const
{
  int i;

  if (!dest.Valid(1, 3) || !map.SameSize(dest))
    return Fatal("Bad images to jhcOverhead3D::ShowZones");

  for (i = 0; i < smax; i++)
    if (used[i] > 0)
      CamZone(dest, i, t, r, g, b);
  return 1;
}


//= Show rough active depth zone of a particular sensor on an overhead map image.
// hfov angle (e.g. 55 degrees) from member variable

int jhcOverhead3D::CamZone (jhcImg& dest, int cam, int t, int r, int g, int b) const
{
  int n = __max(0, __min(cam, smax - 1));
  double da = 0.5 * hfov * D2R, a1 = D2R * p0[n] + da, a2 = D2R * p0[n] - da;
  double kx0 = (cx[n] + x0)/ ipp, ky0 = (cy[n] + y0) / ipp, hyp = rmax[n] / (ipp * cos(da));
  double kx1 = kx0 + hyp * cos(a1), ky1 = ky0 + hyp * sin(a1);
  double kx2 = kx0 + hyp * cos(a2), ky2 = ky0 + hyp * sin(a2);

  if (!dest.Valid(1, 3) || !map.SameSize(dest))
    return Fatal("Bad images to jhcOverhead3D::CamZone");
  DrawLine(dest, kx0, ky0, kx1, ky1, t, r, g, b); 
  DrawLine(dest, kx1, ky1, kx2, ky2, t, r, g, b);
  DrawLine(dest, kx2, ky2, kx0, ky0, t, r, g, b);
  return 1;
}


//= Show surface patches of all active sensors.

int jhcOverhead3D::ShowPads (jhcImg& dest, int t, int r, int g, int b) const
{
  int i;

  if (!dest.Valid(1, 3) || !map.SameSize(dest))
    return Fatal("Bad images to jhcOverhead3D::ShowPads");

  for (i = 0; i < smax; i++)
    if (used[i] > 0)
      Footprint(dest, i, t, r, g, b);
  return 1;
}


//= Show rough boundaries for a particular sensor of viewing area on the surface.
// FOV is derived from focal length in SrcSize() = 57x43 degrees for Kinect1

int jhcOverhead3D::Footprint (jhcImg& dest, int cam, int t, int r, int g, int b) const
{
  int n = __max(0, __min(cam, smax - 1));
  double cx0 = x0 + cx[n], cy0 = y0 + cy[n], p = D2R * p0[n];
  double c = cos(p), s = sin(p), f = tan(D2R * 0.5 * hfov);
  double dz = cz[0] - ztab, tmid = 90.0 + t0[n], dt = 0.5 * vfov, ppi = 1.0 / ipp;
  double bot = dz / cos(D2R * (tmid - dt));
  double hyp0 = ((bot < 0.0) ? rmax[n] : __min(bot, rmax[n])), hw0 = hyp0 * f;           
  double off0 = sqrt(hyp0 * hyp0 - dz * dz), bx0 = cx0 + off0 * c, by0 = cy0 + off0 * s;
  double kx0 = ppi * (bx0 - hw0 * s), ky0 = ppi * (by0 + hw0 * c);
  double kx1 = ppi * (bx0 + hw0 * s), ky1 = ppi * (by0 - hw0 * c); 
  double top = dz / cos(D2R * (tmid + dt));
  double hyp1 = ((top < 0.0) ? rmax[n] : __min(top, rmax[n])), hw1 = hyp1 * f;           
  double off1 = sqrt(hyp1 * hyp1 - dz * dz), bx1 = cx0 + off1 * c, by1 = cy0 + off1 * s;
  double kx2 = ppi * (bx1 - hw1 * s), ky2 = ppi * (by1 + hw1 * c); 
  double kx3 = ppi * (bx1 + hw1 * s), ky3 = ppi * (by1 - hw1 * c); 

  if (!dest.Valid(1, 3) || !map.SameSize(dest))
    return Fatal("Bad images to jhcOverhead3D::Footprint");

  DrawLine(dest, kx0, ky0, kx1, ky1, t, r, g, b); 
  DrawLine(dest, kx1, ky1, kx3, ky3, t, r, g, b);
  DrawLine(dest, kx3, ky3, kx2, ky2, t, r, g, b);
  DrawLine(dest, kx2, ky2, kx0, ky0, t, r, g, b);
  return 1;
}


//= Show polygon for restricting fine tilt estimation on input image.
// returns 1 if successful, 0 if no such region (nothing drawn) 

int jhcOverhead3D::AreaEst (jhcImg& dest, int cam, int t, int r, int g, int b) const
{
  int n = __max(0, __min(cam, smax - 1)), n4 = 4 * n;

  if (!dest.Valid(1, 3) || (dest.XDim() != InputW()) || (dest.YDim() != InputH()))
    return Fatal("Bad images to jhcOverhead3D::AreaEst");
  if (Restricted(cam) > 0)
    return DrawPoly(dest, rx + n4, ry + n4, 4, t, r, g, b);
  return 0;
}
