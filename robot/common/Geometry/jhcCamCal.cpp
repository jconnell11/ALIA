// jhcCamCal.cpp : camera calibration using a known floor rectangle
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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
#include <math.h>

#include "Interface/jhcMessage.h"  // common video

#include "Geometry/jhcCamCal.h"    // common robot


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcCamCal::jhcCamCal (int cam)
{
  // default to camera 0 and set sizes of conversion matrices
  i2f.SetSize(4, 4);
  f2i.SetSize(4, 4);
  i2w.SetSize(4, 4);
  w2i.SetSize(4, 4);

  // set default mappings
  i2f.Identity();
  f2i.Identity();
  i2w.Identity();
  w2i.Identity();

  // calibration tile and image corner defaults
  // change to arm coords: relative to wheel centers with origin 2" over table
//  SetGeom(    3.25,  -1.75,  25.25,   0.0,   10.0,   0.0,   11.0,   8.5  ); 
  SetGeom(    0.5,   9.0,  26.7,  -3.3,  16.9,  -2.0,  11.0,   8.5  ); 
  ImagePts( 164.0, 288.0, 359.0, 282.0, 364.0, 136.0, 134.0, 142.0  );
  world_rect();

  // load standard processing values and refresh matrices
  SetNum(cam);
  SetSize(640, 480);
  LoadCfg();
}


//= Set dimension of images typically used.
// should call Calibrate after this

void jhcCamCal::SetSize (int w, int h)
{
  // save sizes
  iw = w;
  ih = h;

  // calculate middle of image
  mx = 0.5 * (iw - 1);
  my = 0.5 * (ih - 1);
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcCamCal::LoadCfg (const char *fname)
{
  int ok = 1;

  ok &= geom_params(fname);
  ok &= world_params(fname);
  ok &= image_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcCamCal::SaveCfg (const char *fname) const
{
  int ok = 1;

  ok &= gps.SaveVals(fname);
  ok &= wps.SaveVals(fname);
  ok &= ips.SaveVals(fname);
  return ok;
}


//= Parameters used to describe calibration geometry.
// describes camera location and Z plane of target (tz)
// also placement and dimensions of standard rectangle target

int jhcCamCal::geom_params (const char *fname)
{
  jhcParam *ps = &gps;
  char tag[20];
  int ok;

  ps->SetTitle("Camera %d geometry", cnum);
  sprintf_s(tag, "cam%d_geom", cnum);
  ps->SetTag(tag, 0);
  ps->NextSpecF( &cx0,  "Camera X");  
  ps->NextSpecF( &cy0,  "Camera Y");  
  ps->NextSpecF( &cz0,  "Camera Z");  
  ps->NextSpecF( &rcx,  "Rectangle mid X");  
  ps->NextSpecF( &rcy,  "Rectangle mid Y");  
  ps->NextSpecF( &tz,   "Target Z plane");  

  ps->NextSpecF( &rwid, "Rectangle width");  
  ps->NextSpecF( &rht,  "Rectangle height");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set target parameters in same order as configuration file.

void jhcCamCal::SetGeom (double xcam, double ycam, double zcam, 
                         double xtarg, double ytarg, double ztarg, double wtarg, double htarg)
{
  SetCam(xcam, xcam, zcam);

  rcx = xtarg;
  rcy = ytarg;
  tz  = ztarg;

  rwid = wtarg;
  rht  = htarg;
}


//= World locations of calibration points (all on one Z plane).
// generally should not adjust these by hand, , order must match image_params
// should call Calibrate after loading or any changes

int jhcCamCal::world_params (const char *fname)
{
  jhcParam *ps = &wps;
  char tag[20];
  int ok;

  ps->SetTitle("Camera %d world pts", cnum);
  sprintf_s(tag, "cam%d_wpts", cnum);
  ps->SetTag(tag, 0);
  ps->NextSpecF( wx,     "X of NW corner");  
  ps->NextSpecF( wy,     "Y of NW corner");  
  ps->NextSpecF( wx + 1, "X of NE corner"); 
  ps->NextSpecF( wy + 1, "Y of NE corner"); 
  ps->NextSpecF( wx + 2, "X of SE corner"); 
  ps->NextSpecF( wy + 2, "Y of SE corner"); 

  ps->NextSpecF( wx + 3, "X of SW corner");  
  ps->NextSpecF( wy + 3, "Y of SW corner");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set world coordinates (rectangle corners) in same order as configuration file.

void jhcCamCal::WorldPts (double x0, double y0, double x1, double y1,
                          double x2, double y2, double x3, double y3)
{
  wx[0] = x0;
  wy[0] = y0;
  wx[1] = x1;
  wy[1] = y1;
  wx[2] = x2;
  wy[2] = y2;
 
  wx[3] = x3;
  wy[3] = y3;
}


//= Pixel locations of calibration points (e.g. rectangle corners).
// generally should not adjust these by hand, order must match world_params
// should call Calibrate after loading or any changes

int jhcCamCal::image_params (const char *fname)
{
  jhcParam *ps = &ips;
  char tag[20];
  int ok;

  ps->SetTitle("Camera %d image pts", cnum);
  sprintf_s(tag, "cam%d_ipts", cnum);
  ps->SetTag(tag, 0);
  ps->NextSpecF( cx,     "X of NW corner");  
  ps->NextSpecF( cy,     "Y of NW corner");  
  ps->NextSpecF( cx + 1, "X of NE corner"); 
  ps->NextSpecF( cy + 1, "Y of NE corner"); 
  ps->NextSpecF( cx + 2, "X of SE corner"); 
  ps->NextSpecF( cy + 2, "Y of SE corner"); 

  ps->NextSpecF( cx + 3, "X of SW corner");  
  ps->NextSpecF( cy + 3, "Y of SW corner");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set image pixel coordinates (rectangle corners) in same order as configuration file.

void jhcCamCal::ImagePts (double x0, double y0, double x1, double y1,
                          double x2, double y2, double x3, double y3)
{
  cx[0] = x0;
  cy[0] = y0;
  cx[1] = x1;
  cy[1] = y1;
  cx[2] = x2;
  cy[2] = y2;
 
  cx[3] = x3;
  cy[3] = y3;
}


///////////////////////////////////////////////////////////////////////////
//                           Core Calibration                            //
///////////////////////////////////////////////////////////////////////////

//= Save set of corners for rectangle found in image.
// can also take associate list of real-world coordinates
// re-orders points so they are clockwise from upper left.
// useful if user clicks on corners in random order
// should call Calibrate after this to get transform matrices
// <pre>
//       NW (x0 y0)     (x1 y1) NE
//              *----------*
//              |          |
//              *----------*
//       SW (x3 y3)     (x2 y2) SE
// </pre>

void jhcCamCal::ImageRect (const double *rx, const double *ry, double *ax, double *ay)
{
  double lo;
  int i, pt0, pt1, pt2, pt3;

  // make sure corresponding world coordinates are loaded
  if ((rx == NULL) || (ry == NULL))
    Fatal("Bad input to jhcCamCal::ImageRect");
  world_rect();

  // lowest x must be on left (pt0 or pt3)
  pt0 = 0;
  lo = rx[0];
  for (i = 1; i < 4; i++)
    if (rx[i] < lo)
    {
      pt0 = i;
      lo = rx[i];
    }

  // second lowest x is also on left (pt0 or pt3)
  pt3 = -1;
  for (i = 0; i < 4; i++)
    if ((i != pt0) && ((pt3 < 0) || (rx[i] < lo)))
    {
      pt3 = i;
      lo = rx[i];
    }
  
  // sort two lowest x's so pt0 has higher y
  if (ry[pt3] >= ry[pt0])
  {
    i = pt0;
    pt0 = pt3;
    pt3 = i;
  }

  // remaining point with lowest y must be pt2
  pt2 = -1;
  for (i = 0; i < 4; i++)
    if ((i != pt0) && (i != pt3) && ((pt2 < 0) || (ry[i] < lo)))
    {
      pt2 = i;
      lo = ry[i];
    }

  // single remaining point must be pt1
  for (i = 0; i < 4; i++)
    if ((i != pt0) && (i != pt2) && (i != pt3))
      pt1 = i;

  // unscramble points and save as image coordinates
  cx[0] = rx[pt0];
  cx[1] = rx[pt1];
  cx[2] = rx[pt2];
  cx[3] = rx[pt3];
  cy[0] = ry[pt0];
  cy[1] = ry[pt1];
  cy[2] = ry[pt2];
  cy[3] = ry[pt3];

  // do the same with ground truth (if any)
  if ((ax != NULL) && (ay != NULL))
  {
    wx[0] = ax[pt0];
    wx[1] = ax[pt1];
    wx[2] = ax[pt2];
    wx[3] = ax[pt3];
    wy[0] = ay[pt0];
    wy[1] = ay[pt1];
    wy[2] = ay[pt2];
    wy[3] = ay[pt3];
  }
}


//= Set world coordinates of calibration points to match standard rectangle target.

void jhcCamCal::world_rect ()
{
  double hw = 0.5 * rwid, hh = 0.5 * rht;

  wx[0] = rcx - hw;
  wx[1] = rcx + hw;
  wx[2] = rcx + hw;
  wx[3] = rcx - hw;
  wy[0] = rcy + hh;
  wy[1] = rcy + hh;
  wy[2] = rcy - hh;
  wy[3] = rcy - hh;
}


//= Determines basic camera parameters from a set of stored correspondences.
// must enter points first using SetCorners, needs SetSize for physics transforms
// can optionally move x and y of image points by up to jitter for better fit
// sets global intermediate results mfx, mfy, pan, tilt, roll, aspect, and focal
// returns RMS error of image points using physics based transform

double jhcCamCal::Calibrate (double jitter)
{
  double ix[4], iy[4], winx[4], winy[4];
  double e, best = -1.0;
  int i, var;

  // possibly change world points if using a rectangular target
  if ((rwid > 0.0) && (rht > 0.0))
    world_rect();

  // possibly just do one step
  if (jitter <= 0.0)
  {
    cal_core(cx, cy);
    return mark_error(cx, cy);
  }

  // move image points around a little (6561 = (3x3)^4)
  for (var = 0; var < 6561; var++)
  {
    // check projection error on variation of ponts
    alt_marks(ix, iy, jitter, var);
    cal_core(ix, iy);
    e = mark_error(ix, iy);
    if ((best < 0.0) || (e < best))
    {
      // record best alternate version of points
      best = e;
      alt_marks(winx, winy, jitter, var);
    }
  }

  // keep best fit to points
  for (i = 0; i < 4; i++)
  {
    cx[i] = winx[i];
    cy[i] = winy[i];
  }

  // build final matrices and return fit 
  cal_core(cx, cy);
  return best;
}


//= Alter image points by a certain amount to give new set.
// test -1, 0, +1 for x and y for each point for 9^4 = 6561 variations

void jhcCamCal::alt_marks (double *ix, double *iy, double jitter, int var) const
{
  int i, v = var;

  // pick one of 3 variations for each x coordinate
  for (i = 0; i < 4; i++)
  {
    ix[i] = cx[i] + (v % 3) * jitter - jitter;
    v /= 3;
  }

  // pick one of 3 variations for each y coordinate
  for (i = 0; i < 4; i++)
  {
    iy[i] = cy[i] + (v % 3) * jitter - jitter;
    v /= 3;
  }
}


//= Find the root mean square error of backprojected landmarks in image.

double jhcCamCal::mark_error (const double *ix, const double *iy) const
{
  double px, py, dx, dy, e2sum = 0.0;
  int i;

  for (i = 0; i < 4; i++)
  {
    FromWorld(px, py, wx[i], wy[i], 0.0);
    dx = px - ix[i];
    dy = py - iy[i];
    e2sum += dx * dx + dy * dy;
  }
  return sqrt(0.25 * e2sum);
}
    
   
///////////////////////////////////////////////////////////////////////////
//                        Physics Based Transform                        //
///////////////////////////////////////////////////////////////////////////

//= Determine homography and physics-based transform for image points.

void jhcCamCal::cal_core (const double *ix, const double *iy)
{
  // use calibration rectangle floor coordinates and image corners to
  // build floor homographies and find camera extrinsic parameters 
  homography(i2f, wx, wy, ix, iy);
  homography(f2i, ix, iy, wx, wy);  
  ToFloor(mfx, mfy, mx, my);
  est_angles(pan, tilt, roll, mfx, mfy);

  // determine camera orientation then build initial world to image transform
  // physics based transform from world to image allows z to vary
  w2i.Translation(-cx0, -cy0, -cz0);  // shift to camera origin
  w2i.RotateZ(-pan);                  // swivel around post
  w2i.RotateX(-(90.0 + tilt) );       // tip backwards (was staring down)
  w2i.RotateZ(roll);                  // spin around viewing direction
  w2i.Project(-1.0);                  // project using pinhole model

  // find best focal length to minimize corner errors then fix mapping
  aspect = adj_scale(focal, ix, iy, wx, wy);
  w2i.Project(-focal);
  w2i.Magnify(aspect, 1.0, 1.0);

  // build inverse image to world matrix for NORMALIZED image coordinates
  i2w.RotationZ(-roll);
  i2w.RotateX(90.0 + tilt);
  i2w.RotateZ(pan);
  i2w.Translate(cx0, cy0, cz0);
}


//= Get camera orientation based on known scene geometry and established homographies.
// needs global floor intersection point (fx0 fy0) as an intermediate result

void jhcCamCal::est_angles (double& p, double& t, double& r, double fx0, double fy0) const 
{
  double dx, dy, dz, tfx, tfy, tx, ty, d, span = 1.0;
  int i;

  // estimate pan and tilt from optical axis intersection with table 
  dx = fx0 - cx0;
  dy = fy0 - cy0; 
  dz =  tz - cz0;
  p = R2D * atan2(-dx, dy);
  t = R2D * atan2(dz, (double) sqrt(dx * dx + dy * dy));

  // pick some reasonable length for an orthogonal line
  for (i = 0; i < 4; i++)
  {
    dx = wx[i] - fx0;
    dy = wy[i] - fy0;
    d = (double) sqrt(dx * dx + dy * dy);
    span = __max(span, d);
  }
  span *= 0.5;

  // floor line along camera orthogonal to pan direction should be horizontal in image
  tfx = span * cos(D2R * p) + fx0;
  tfy = span * sin(D2R * p) + fy0;
  FromFloor(tx, ty, tfx, tfy);
  r = R2D * atan2(ty - my, tx - mx);
}


//= Pick best focal length to minimize corner matching errors using w2i guess.
// minimizes RMS image error, e^2 = sum_i[ (f * px_i - cx_i)^2 ] 
// returns x/y pixel aspect ratio 

double jhcCamCal::adj_scale (double &foc, const double *ix, const double *iy, 
                             const double *fx, const double *fy) const
{
  double cx, cy, px, py, xf, xnum = 0.0, xden = 0.0, ynum = 0.0, yden = 0.0;
  int i;

  for (i = 0; i < 4; i++)
  {
    // get true image corner distance relative to image center
    cx = ix[i] - mx;
    cy = iy[i] - my;

    // project landmark corner into image and get distance from center
    FromWorld(px, py, fx[i], fy[i], 0.0);
    px -= mx;
    py -= my;

    // update terms for focal length estimation
    xnum += cx * px; 
    xden += px * px;
    ynum += cy * py; 
    yden += py * py;
  }

  // set derivative to zero to find minimum
  xf  = xnum / xden;
  foc = ynum / yden;
  return(xf / foc);
}


///////////////////////////////////////////////////////////////////////////
//                            Basic Homography                           //
///////////////////////////////////////////////////////////////////////////

//= Compute 4x4 homography matrix that takes (fx fy) to (px py)
// needs at least 4 points in correspondence (ignores any extra)
// when using set fz = 1 and normalize ix' = ix/iz, iy' = iy/iz
// this is the Hartley normalized 8 point algorithm with h33 = 1

void jhcCamCal::homography (jhcMatrix& h, const double *ix, const double *iy, 
                            const double *fx, const double *fy) const
{
  jhcMatrix A(8, 8), Ainv(8, 8);
  jhcMatrix pre(4, 4), post(4, 4);
  jhcMatrix b(8), coef(8);
  double ixn[4], iyn[4], fxn[4], fyn[4];
  double idx, idy, isc, fdx, fdy, fsc;
  int i, j, n = 0;

  // pre-condition input points and get correction matrix
  homo_norm(fdx, fdy, fsc, fxn, fyn, fx, fy);
  pre.Identity();
  pre.MSet(3, 0, -fdx * fsc);
  pre.MSet(3, 1, -fdy * fsc);
  pre.MSet(0, 0, fsc);
  pre.MSet(1, 1, fsc);

  // pre-condition output points and get correction matrix
  homo_norm(idx, idy, isc, ixn, iyn, ix, iy);
  post.Identity();
  post.MSet(2, 0, idx);              // since normalized by z
  post.MSet(2, 1, idy);
  post.MSet(0, 0, 1.0 / isc);
  post.MSet(1, 1, 1.0 / isc);

  // fill in basic A matrix then get inverse
  A.Zero(0.0);
  for (i = 0; i < 4; i++)
  {
    j = i + i;
    A.MSet(0, j, fxn[i]);            // first row
    A.MSet(1, j, fyn[i]);
    A.MSet(2, j, 1.0);
    A.MSet(6, j, -fxn[i] * ixn[i]);
    A.MSet(7, j, -fyn[i] * ixn[i]);
    j++;
    A.MSet(3, j, fxn[i]);            // second row
    A.MSet(4, j, fyn[i]);
    A.MSet(5, j, 1.0);
    A.MSet(6, j, -fxn[i] * iyn[i]);
    A.MSet(7, j, -fyn[i] * iyn[i]);
  }
  Ainv.Invert(A);

  // multiply by column vector b of output points 
  for (i = 0; i < 4; i++)
  {
    j = i + i;
    b.VSet(j,     ixn[i]);
    b.VSet(j + 1, iyn[i]);
  }
  coef.MatVec(Ainv, b);

  // reformat h column vector into matrix form (h33 = 1)
  h.Identity();                   
  for (j = 0; j < 3; j++)
    for (i = 0; i < 3; i++, n++)
      if (n < 8)
        h.MSet(i, j, coef.VRef(n));

  // apply normalization corrections
  h.MatMat(h, pre);
  h.MatMat(post, h);
}


//= Performs Hartley's pre-conditioning on a list of 4 points.
// adjusts so centroid is at 0 and average point distance is sqrt(2)
// returns position shift and scaling factor used

void jhcCamCal::homo_norm (double& dx, double& dy, double& sc, double *xn, double *yn, 
                           const double *x, const double *y) const
{
  double xsum = 0.0, ysum = 0.0, dist = 0.0;
  int i;

  // compute centroid shift
  for (i = 0; i < 4; i++)
  {
    xsum += x[i];
    ysum += y[i];
  }
  dx = 0.25 * xsum;
  dy = 0.25 * ysum;

  // apply shift and compute distance scaling
  for (i = 0; i < 4; i++)
  {
    xn[i] = x[i] - dx;
    yn[i] = y[i] - dy;
    dist += (double) sqrt(xn[i] * xn[i] + yn[i] * yn[i]);
  }
  sc = 4.0 * sqrt(2.0) / dist;

  // apply scaling
  for (i = 0; i < 4; i++)
  {
    xn[i] *= sc;
    yn[i] *= sc;
  }
}
                          

///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Apply floor to image homography to some point on the floor.

void jhcCamCal::FromFloor (double& ix, double& iy, double fx, double fy) const
{
  jhcMatrix img(4), floor(4);

  floor.SetVec3(fx, fy, 1.0);
  img.MatVec(f2i, floor);
  ix = img.X() / img.Z();
  iy = img.Y() / img.Z();
}


//= Apply image to floor homography to some point from the image.

void jhcCamCal::ToFloor (double& fx, double& fy, double ix, double iy) const
{
  jhcMatrix img(4), floor(4);

  img.SetVec3(ix, iy, 1.0);
  floor.MatVec(i2f, img);
  fx = floor.X() / floor.Z();
  fy = floor.Y() / floor.Z();
}


//= Apply world to image camera model to find point on image.

void jhcCamCal::FromWorld (double& ix, double& iy, double wx, double wy, double wz) const
{
  jhcMatrix img(4), world(4);

  world.SetVec3(wx, wy, wz);
  img.MatVec(w2i, world);
  ix = img.X() / img.Homo() + mx; 
  iy = img.Y() / img.Homo() + my;
}


//= Get world coordinates of image point given known height above ground plane.
// must supply a presumed world z coordinate to pick point on ray

void jhcCamCal::ToWorld (double& wx, double& wy, double wz, double ix, double iy) const
{
  jhcMatrix norm(4), zrow(4), world(4);
  double iz;

  // use normalized image coordinates assuming iz = 1 to solve for actual iz
  //   wz = (m02 * nx + m12 * ny + m22 * nz) / iz + m32
  //   iz = (m02 * nx + m12 * ny + m22 * nz) / (wz - m32)
  norm.SetVec3((ix - mx) / aspect, (iy - my), -focal, 0.0); 
  zrow.GetRow(i2w, 2);
  iz = zrow.DotVec3(norm) / (wz - i2w.MRef(3, 2));

  // put derived iz back into normalized image vector then apply standard transform
  norm.ScaleVec3(1.0 / iz, 1.0);
  world.MatVec(i2w, norm);
  wx = world.X();
  wy = world.Y();
}


//= Determine world z coordinate for image point with known world Y coordinate.
// supplying wx also could add an unsatisfiable constraint (make an X version instead?)

double jhcCamCal::WorldHt (double wy, double ix, double iy) const
{
  jhcMatrix norm(4), yrow(4), world(4);
  double iz;

  // use normalized image coordinates assuming iz = 1 to solve for actual iz
  //   wy = (m01 * nx + m11 * ny + m21 * nz) / iz + m31
  //   iz = (m01 * nx + m11 * ny + m21 * nz) / (wy - m31)
  norm.SetVec3((ix - mx) / aspect, (iy - my), -focal, 0.0); 
  yrow.GetRow(i2w, 1);
  iz = yrow.DotVec3(norm) / (wy - i2w.MRef(3, 1));

  // put derived iz back into normalized image vector then apply standard transform
  norm.ScaleVec3(1.0 / iz, 1.0);
  world.MatVec(i2w, norm);
  return world.Z();
}