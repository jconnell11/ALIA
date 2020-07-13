// jhcFaceNorm.cpp : generates canonical face image for recognition
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2015 IBM Corporation
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

#include "Face/jhcFaceNorm.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcFaceNorm::jhcFaceNorm ()
{
  Defaults();
}


//= Default destructor does necessary cleanup.

jhcFaceNorm::~jhcFaceNorm ()
{
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcFaceNorm::Defaults (const char *fname)
{
  int ok = 1;

  ok &= norm_params(fname);
  ok &= eye_params(fname);
  ok &= icon_params(fname);
  SetSizes();
  return ok;
}


//= Write current processing variable values to a file.

int jhcFaceNorm::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= nps.SaveVals(fname);
  ok &= eps.SaveVals(fname);
  ok &= ips.SaveVals(fname);
  return ok;
}


//= Parameters used for building canoncial images for recognition.
// should call SetSizes if values changed

int jhcFaceNorm::norm_params (const char *fname)
{
  jhcParam *ps = &nps;
  int ok;

  ps->SetTag("face_norm", 0);
  ps->NextSpec4( &gw,      100,   "Intermediate face width (pel)");  
  ps->NextSpecF( &wexp,      2.0, "Full width wrt face width");       
  ps->NextSpecF( &hexp,      3.0, "Full height wrt face width");  
  ps->NextSpecF( &enh,       4.0, "Max contrast stretch");   
  ps->NextSpec4( &mode,      2,   "Monochrome (1=A,I,G,R,B)"); 
  ps->NextSpec4( &fix_pos,   0,   "Fix position with eyes");    // was 1

  ps->NextSpec4( &fix_sc,    0,   "Fix scale with eyes");       // was 1
  ps->NextSpec4( &fix_ang,   0,   "Fix rotation with eyes");    // was 1
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set defaults for parameters related to matched filter for eyes.
// should call SetSizes if values changed

int jhcFaceNorm::eye_params (const char *fname)
{
  jhcParam *ps = &eps;
  int ok;

  ps->SetTag("face_eye", 0);
  ps->NextSpecF( &ewf,   0.7,  "Search width wrt face width");       
  ps->NextSpecF( &ehf,   0.4,  "Search height wrt face width");   
  ps->NextSpecF( &eup,   0.65, "Eyeline from bottom of face");
  ps->NextSpec2( &bc,    1,    "Bar vertical center (pel)");   
  ps->NextSpec2( &bs,   33,    "Bar vertical surround (pel)");   
  ps->NextSpec2( &bw,   17,    "Bar horizontal width (pel)");    // was 7

  ps->NextSpecF( &bg,    2.5,  "Horizontal gain");              
  ps->NextSpecF( &th2,   0.3,  "Secondary threshold");           // was 0.5
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set defaults for output image size and layout.
// should call SetSizes if values changed

int jhcFaceNorm::icon_params (const char *fname)
{
  jhcParam *ps = &ips;
  int ok;

  ps->SetTag("face_icon", 0);
  ps->NextSpecF( &eln,   0.55, "Eyeline height in icon");    
  ps->NextSpecF( &efrac, 0.7,  "Eye band width in icon");    
  ps->NextSpecF( &esep,  0.5,  "Eye separation in icon");
  ps->Skip();
  ps->NextSpec2( &back,  100,  "Neutral boundary color");
  ps->NextSpec4( &mode2,   2,  "Monochrome (1=A,I,G,R,B)"); 

  ps->NextSpec2( &iw,     60,  "Final icon width (pel)");         // was 50  
  ps->NextSpec2( &ih,     90,  "Final icon height (pel)");        // was 72  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Setup size of intermediate images.

void jhcFaceNorm::SetSizes ()
{
  int gh = ROUND(gw * ehf / ewf);

  // eye analysis images
  eyes.SetSize(gw, gh, 3);
  mono.SetSize(eyes, 1);
  hbar.SetSize(mono);
  ebin.SetSize(mono);

  // full face expanded area and small color face icon
  face.SetSize(ROUND(wexp * gw), ROUND(hexp * gw), 3);
  icol.SetSize(iw, ih, 3);

  // portion of face corresponding to eyes and eye search region
  band.CenterRoi(face.RoiMidX(), ROUND(face.RoiAvgY() + (eup - 0.5) * gh), gw, gh);
  efind.CenterRoi(ROUND(0.5 * gw), ROUND(0.5 * gh), ROUND(gw * ewf), ROUND(gw * ehf));
}


///////////////////////////////////////////////////////////////////////////
//                        Normalization Functions                        //
///////////////////////////////////////////////////////////////////////////

//= Convert input region of interest into normalized grayscale face image.
// gray is pointer to output image buffer of size freco_wid x freco_ht
// img is bottom-up BGR image with lines padded to 4 byte boundaries
// w is image width, h is image height, f is number of fields (3 = color)
// rx and ry define lower left corner of patch with size rw x rh
// if raw > 0 then bases result on initial face finder patch only
// returns 1 if successful, 0 or negative if error

int jhcFaceNorm::freco_norm (unsigned char *gray, 
                             const unsigned char *img, int w, int h, int f, 
                             int rx, int ry, int rw, int rh, int raw)
{
  jhcImg src, dest;
  jhcRoi fdet;

  // repackage inputs (overrides const but respects it)
  src.Wrap((UC8 *) img, w, h, f);
  dest.Wrap(gray, iw, ih, 1);
  fdet.SetRoi(rx, ry, rw, rh);

  // get intermediate image then recenter, scale, and rotate based on eyes
  face_area(face, src, fdet);
  find_eyes(roll, pod, face);
  build_icon(dest, face, roll, pod, raw);

  // see if transform was reasonable
  eok = chk_eyes(lex, ley, rex, rey);
  return eok;
}


//= Get nice small image around face detection location.

void jhcFaceNorm::face_area (jhcImg& dest, const jhcImg& src, const jhcRoi& fdet) 
{
  jhcImg clip;
  jhcRoi mid;

  // get portion of original image around face 
  halo.CopyRoi(fdet);
  halo.ResizeRoi(wexp, hexp);
  clip.SetSize(halo.RoiW(), halo.RoiH(), 3);
  Extract(clip, src, halo.RoiX(), halo.RoiY());

  // contrast enhance face area and resize to intermediate image
  mid.CenterRoi(clip.RoiMidX(), clip.RoiMidY(), fdet.RoiW(), fdet.RoiH());
  if (enh > 1.0)
    Enhance(clip, clip, enh, &mid);
  Bicubic(dest, clip);
}


//= Find horizontal dark bar region corresponding to eyes.
// returns angle in "ang" and bounding box of eye bar in "box"

void jhcFaceNorm::find_eyes (double& ang, jhcRoi& ebox, const jhcImg& src)
{
  double ecc;
  int top; 

  // extract just portion corresponding to eyes (plus some margin)
  Extract(eyes, src, band.RoiX(), band.RoiY());
  ForceMono(mono, eyes, mode);

  // emphasize eyes using matched filter (save unblotted response)
  ClipCS(hbar, mono, bw, bc, bw, bs, -bg); 
  top = MaxVal(hbar, efind);
  Threshold(ebin, hbar, ROUND(th2 * top));
  Matte(ebin, efind);

  // guess size and angle
  RegionNZ(ebox, ebin);
  Shape(&xc, &yc, &ecc, &ang, ebin, efind);
  ang = -ang;
  if (ang < -90.0)
    ang += 180.0;
  else if (ang > 90.0)
    ang -= 180.0;
}


//= Use eye parameters to build nice normalized face patch.
// also internally binds eye positions in original image

void jhcFaceNorm::build_icon (jhcImg& dest, const jhcImg& src, double ang, const jhcRoi& ebox, int raw)
{
  double sc = gw / (double) iw;

  // save default in plane rotation and patch scaling factor
  xmid = band.RoiAvgX();
  ymid = band.RoiAvgY();
  tip = 0.0;
  mag = (efrac * band.RoiW()) / ebox.RoiW();

  // setup for larger patch around canonical face based on eye positions
  if (raw <= 0)
  {
    if (fix_pos > 0)
    {
      xmid = ebox.RoiAvgX() + band.RoiX();
      ymid = ebox.RoiAvgY() + band.RoiY();
    }
    if (fix_sc > 0)
      sc = ebox.RoiW() / (efrac * iw);
    if (fix_ang > 0)
      tip = ang;
    mag = (efrac * band.RoiW()) / pod.RoiW();   // largely for display
  }

  // build rotated and scaled version of intermediate image (color and mono)
  RigidMixRGB(icol, src, -tip, 0.5 * iw, eln * ih, xmid, ymid, back, back, back, sc, sc);
  ForceMono(dest, icol, mode2);
}


//= Generate presumed eye positions and check for reasonable-ness.
// needs build_icon called first to set transform parameters

int jhcFaceNorm::chk_eyes (double& lx, double& ly, double& rx, double& ry) const
{
  double tmax = 20.0, zmax = 1.5;
  double isc, x0, y0, half, rads, dx, dy;

  // find face center in big image
  isc = halo.RoiW() / (double) face.XDim();
  x0 = halo.RoiX() + isc * xmid;
  y0 = halo.RoiY() + isc * ymid;

  // generate offsets for eyes 
  half = 0.5 * esep * iw * isc / mag;
  rads = -D2R * tip;
  dx = half * cos(rads);
  dy = half * sin(rads);

  // generate actual eye positions
  lx = x0 - dx;
  rx = x0 + dx;
  ly = y0 + dy;
  ry = y0 - dy; 

  // test transform parameters
  if ((fabs(tip) > tmax) || (mag > zmax))
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Debugging Graphics                           //
///////////////////////////////////////////////////////////////////////////

//= Mark eye bar angle and horizontal limits on binary mask (for debugging).

int jhcFaceNorm::EyeBounds (jhcImg& dest) const
{
  double len = 70, rads = D2R * tip, dx = len * cos(rads), dy = len * sin(rads);

  if (!dest.SameSize(ebin))
    return Fatal("Bad images to jhcFaceNorm::EyeBounds");
  DrawLine(dest, xc - dx, yc - dy, xc + dx, yc + dy, 1, 128);
  DrawLine(dest, pod.RoiX(), 0, pod.RoiX(), dest.YDim(), 1, 128);
  DrawLine(dest, pod.RoiLimX(), 0, pod.RoiLimX(), dest.YDim(), 1, 128);
  return 1;
}


