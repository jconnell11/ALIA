// jhcColorSeg.cpp : find objects against a homogeneous background
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

#include "Interface/jhcMessage.h"

#include "Objects/jhcColorSeg.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcColorSeg::jhcColorSeg ()
{
  // standard histograms and object list
  hist.SetSize(256);
  rghist.SetSize(256);
  ybhist.SetSize(256);
  wkhist.SetSize(256);
  blob.SetSize(500);

  // set processing parameters 
  SetSize(320, 240);
  SetRegion(320, 0, 210, 60, 319, 1, 239, 60);
  SetParse(13, 5, 3, 180, 100, 0.05, 0.2, 0.1);
  Defaults();
}


//= Set sizes of internal images based on a reference image.

void jhcColorSeg::SetSize (const jhcImg& ref)
{
  SetSize(ref.XDim(), ref.YDim());
}


//= Set sizes of internal images directly.
// always tries to make internal images roughly 320x240

void jhcColorSeg::SetSize (int x, int y)
{
  // save input image size 
  iw = x;
  ih = y;

  // set size of internal color images
  boost.SetSize(x, y, 3);

  // set size of connected components
  comps.SetSize(x, y, 2);

  // set size of monochrome image
  patch.SetSize(x, y, 1);
  retain.SetSize(patch);
  rg.SetSize(patch);
  yb.SetSize(patch);
  wk.SetSize(patch);
  rg3.SetSize(patch);
  yb3.SetSize(patch);
  wk3.SetSize(patch);
  vote.SetSize(patch);
  vsm.SetSize(patch);
  bulk.SetSize(patch);

  // set size of graphics helper images
  mask.SetSize(x, y, 1);
  targs.SetSize(mask);
}


///////////////////////////////////////////////////////////////////////////
//                        Processing Parameters                          //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcColorSeg::Defaults (const char *fname)
{
  int ok = 1;

  ok &= region_params(fname);
  ok &= color_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcColorSeg::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= rps.SaveVals(fname);
  ok &= cps.SaveVals(fname);
  return ok;
}


//= Parameters used for defining image regions.

int jhcColorSeg::region_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("cseg_reg", 0);
  ps->NextSpec4( &px1, "Right side of patch");  
  ps->NextSpec4( &px0, "Left side of patch");  
  ps->NextSpec4( &py1, "Top of patch");         // was 180
  ps->NextSpec4( &py0, "Bottom of patch");  
  ps->NextSpec4( &rx1, "Right side of region");  
  ps->NextSpec4( &rx0, "Left side of region");  

  ps->NextSpec4( &ry1, "Top of region");  
  ps->NextSpec4( &ry0, "Bottom of region");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set region location parameters in same order as configuration file.

void jhcColorSeg::SetRegion (int pr, int pl, int pt, int pb, int rr, int rl, int rt, int rb)
{
  px1 = pr;
  px0 = pl;
  py1 = pt;
  py0 = pb;
  rx1 = rr;
  rx0 = rl;
  ry1 = rt;
  ry0 = rb;
}


//= Parameters used for picking background color.

int jhcColorSeg::color_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("cseg_col", 0);
  ps->NextSpec4( &sm,    "Color histogram smoothing");  // was 9
  ps->NextSpec4( &dev,   "Color boundary ramp");  
  ps->NextSpec4( &blur,  "Evidence smoothing");         // was 5
  ps->NextSpec4( &pick,  "Evidence threshold");         // was 200
  ps->NextSpec4( &amin,  "Minimum object area");        // was 250
  ps->NextSpecF( &rise,  "Max histogram rise");         

  ps->NextSpecF( &drop,  "Color histogram drop");       // was 0.3 then 0.1
  ps->NextSpecF( &idrop, "Intensity histogram drop");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set segmentation parameters in same order as configuration file.

void jhcColorSeg::SetParse (int hs, int tol, int es, int th, int a, double r, double cd, double id)
{
  sm    = hs;
  dev   = tol;
  blur  = es;
  pick  = th;
  amin  = a;
  rise  = r;
  drop  = cd;
  idrop = id;
} 


//= Copy shape segmentation parameters from another instance.

void jhcColorSeg::CopyParse (const jhcColorSeg& ref)
{
  sm    = ref.sm;
  dev   = ref.dev;
  blur  = ref.blur;
  pick  = ref.pick;
  amin  = ref.amin;
  rise  = ref.rise;
  drop  = ref.drop;
  idrop = ref.idrop;
}


///////////////////////////////////////////////////////////////////////////
//                     Non-Background Object Spotting                    //
///////////////////////////////////////////////////////////////////////////

//= Find likely objects on a constant color background given table area.
// uses optional mask "clr" to guess color, kills objects extending outside optional "area"
// combines with simple rectangular regions for guessing color and restricting objects
// use HoleCount to get the number of objects found
// returns 1 if proper surface found, 0 if no surface, negative for error
// 6.0 ms for SIF, 28.3 ms for VGA on 3.2 GHz Pentium

int jhcColorSeg::FindHoles (const jhcImg& src, const jhcImg *clr, const jhcImg *area)
{
  int ok;

  ColorBG(src, clr);
  ok = MaskBG(src, 0);
  ParseFG(area);
  return ok;
}


//= Find and cache likely color of background region.
// can constrain to some mask area "clr" in addition to patch rectangle
// result can be examined with functions LimRG(), etc.

int jhcColorSeg::ColorBG (const jhcImg& src, const jhcImg *clr)
{
  jhcRoi box;
  double rise2 = -1.0;
  int pk;

  if (!src.SameFormat(iw, ih, 3) || ((clr != NULL) && !src.SameSize(*clr, 1)))
    return Fatal("Bad images to jhcColorSeg::ColorBG");

  // combine likely background map with rectangular ROI
  if (clr != NULL)
    patch.CopyArr(*clr);
  else
    patch.FillArr(255);
  box.SetRoi(px0, py0, px1 - px0, py1 - py0);
  Matte(patch, box, 0);

  // compute nice color representation
  Intensity(wk, src);
  MaxColor(boost, src, 5.0);
  ColorDiffs(rg, yb, boost);

  // histogram likely background in white-black
  // and find values corresponding to strongest peak
  HistOver(wkhist, wk, patch, 128);
  wkhist.ASet(0, 0);
  wkhist.ASet(255, 0);
  wkhist.Scale(wkhist, 100.0);
  hist.Boxcar(wkhist, sm);
  wkhist.Boxcar(hist, sm);
  pk  = wkhist.MaxBin();
  wk0 = wkhist.PeakLeft( pk, idrop, -1, 0.0, rise2) - 1;
  wk1 = wkhist.PeakRight(pk, idrop, -1, 0.0, rise2) + 1;

  // histogram likely background area in red-green
  // and find values corresponding to strongest peak
  HistOver(rghist, rg, patch, 128);
  rghist.Scale(rghist, 100.0);
  hist.Boxcar(rghist, sm);
  rghist.Boxcar(hist, sm);
  pk  = rghist.MaxBin();
  rg0 = rghist.PeakLeft( pk, drop, -1, 0.0, rise) - 1;
  rg1 = rghist.PeakRight(pk, drop, -1, 0.0, rise) + 1;

  // histogram likely background area in yellow-blue
  // and find values corresponding to strongest peak
  HistOver(ybhist, yb, patch, 128);
  ybhist.Scale(ybhist, 100.0);
  hist.Boxcar(ybhist, sm);
  ybhist.Boxcar(hist, sm);
  pk  = ybhist.MaxBin();
  yb0 = ybhist.PeakLeft( pk, drop, -1, 0.0, rise) - 1;
  yb1 = ybhist.PeakRight(pk, drop, -1, 0.0, rise) + 1;
  return 1;
}


//= Copy background color segmentation thresholds from another instance.

void jhcColorSeg::CopyColor (const jhcColorSeg& ref)
{
  rg0 = ref.rg0;
  rg1 = ref.rg1;
  yb0 = ref.yb0;
  yb1 = ref.yb1;
  wk0 = ref.wk0;
  wk1 = ref.wk1;
}


//= Generate mask of background region using a new input image.
// assumes ColorBG() called at some earlier point to set thresholds
// if get_col <= 0 then cached color representation of image will be used
// final result can be seen using function Background()

int jhcColorSeg::MaskBG (const jhcImg& src, int get_col)
{
  int bgmin = 1000;

  if (!src.SameFormat(iw, ih, 3))
    return Fatal("Bad images to jhcColorSeg::MaskBG");

  // see if color representation needs to be recomputed
  if (get_col > 0)
  {
    Intensity(wk, src);
    MaxColor(boost, src, 5.0);
    ColorDiffs(rg, yb, boost);
  }

  // find bland regions relative to thresholds and combine into bulk area
  InRange(wk3, wk, wk0 - dev, wk1 + dev, dev);     
  InRange(rg3, rg, rg0 - dev, rg1 + dev, dev);
  InRange(yb3, yb, yb0 - dev, yb1 + dev, dev);     
  AvgFcn(vote, rg3, yb3); 
  Blend(vote, vote, wk3, 0.6667);              
  BoxAvg(vsm, vote, blur);

  // pick biggest connected region and check area
  if (Biggest(bulk, vsm, pick) < bgmin)
    return 0;
  return 1;
}


//= Find objects against precomputed background region.
// can constrain objects to some "area" mask in addition to patch rectangle
// use HoleCnt() to see the number of objects found

int jhcColorSeg::ParseFG (const jhcImg *area)
{
  jhcRoi box;

  if ((area != NULL) && !area->SameFormat(iw, ih, 1))
    return Fatal("Bad images to jhcColorSeg::ParseFG");

  // find holes in background
  CComps4(comps, bulk, amin, -128);  

  // combine object validity map with rectangular ROI (but not borders)
  if (area != NULL)
    retain.CopyArr(*area);
  else
    retain.FillArr(255);
  box.SetRoi(rx0, ry0, rx1 - rx0, ry1 - ry0);
  Matte(retain, box, 0);
  Border(retain, 1, 0);

  // remove items extending beyond valid region
  blob.FindParams(comps);
  blob.MinEach(comps, retain);
  blob.ValueThresh(1);
  blob.RemBorder(comps, 3);                  
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Foreground Properties                         //
///////////////////////////////////////////////////////////////////////////

//= Return the pixel count of object N.

int jhcColorSeg::HoleArea (int n) const
{
  return blob.BlobArea(blob.Nth(n));
}


//= Return the centroid location of object N.

int jhcColorSeg::HoleCentroid (double& x, double &y, int n) const
{
  return blob.BlobCentroid(&x, &y, blob.Nth(n));
}


//= Return the bounding box covering object N.

int jhcColorSeg::HoleBBox (jhcRoi& box, int n) const
{
  return blob.GetRoi(box, blob.Nth(n));
}


//= Returns the length, width, and orientation for an ellipse approximation. 

int jhcColorSeg::HoleEllipse (double& len, double& wid, double& axis, int n) const
{
  int i = blob.Nth(n);

  if (i < 0)
    return 0;
  len = blob.BlobLength(i);
  wid = blob.BlobLength(i);
  axis = 180.0 - blob.BlobAngle(i);
  return 1;
}


//= Return a full-sized binary image of where object N is.

int jhcColorSeg::HoleMask (jhcImg& mask, int n) const
{
  return blob.MarkBlob(mask, comps, blob.Nth(n), 255, 1);
}


//= Find the nearest blob to a mouse click point on image.
// returns -1 if no objects or nothing reasonably close

int jhcColorSeg::NearestBox (int x, int y) const
{
  jhcRoi box;
  double dist, best, grow = 0.5;
  int i, j, win = -1, n = HoleCount();

  for (i = 0; i < n; i++)
  {
    j = blob.Nth(i);
    blob.GetRoi(box, j);
    box.ResizeRoi(1.0 + grow);
    if (box.RoiContains(x, y) > 0)
    {
      dist = box.CenterDist(x, y);
      if ((win < 0) || (dist < best))
      {
        win = i;
        best = dist;
      }
    }
  }
  return win;
}


///////////////////////////////////////////////////////////////////////////
//                           Debugging Graphics                          //
///////////////////////////////////////////////////////////////////////////

//= Generate a color coded image of objects in scene.
// will take either a monochrome or color image (better for saving)

int jhcColorSeg::PrettyHoles (jhcImg& dest)
{
  jhcImg *bin = (dest.Valid(1) ? &dest : &targs);

  if (!dest.SameSize(boost) || !dest.Valid(1, 3))
    return Fatal("Bad images to jhcColorSeg::PrettyHoles");

  blob.ThreshValid(mask, comps);
  Scramble(*bin, comps);
  OverGate(*bin, *bin, mask, 128);
  if (dest.Valid(3))
    FalseColor(dest, *bin);
  return 1;
}


//= Put axis parallel bounding box around some particular object.

int jhcColorSeg::DrawBBox (jhcImg& dest, int n, int t, int r, int g, int b)
{
  jhcRoi box;
  double f = dest.YDim() / (double) boost.YDim();
  int i = blob.Nth(n);

  if (!dest.Valid(1, 3))
    return Fatal("Bad images to jhcColorSeg::DrawBBox");

  if (i < 0)
    return 0;
  blob.GetRoi(box, i);
  box.ScaleRoi(f);
  return RectEmpty(dest, box, t, r, g, b);
}

