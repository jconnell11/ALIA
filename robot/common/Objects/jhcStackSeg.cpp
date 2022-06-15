// jhcStackSeg.cpp : object and freespace locator for Manus robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include "Objects/jhcStackSeg.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcStackSeg::~jhcStackSeg ()
{
}


//= Default constructor initializes certain values.

jhcStackSeg::jhcStackSeg ()
{
  // histogram sizes
  fhist[0].SetSize(256);
  fhist[1].SetSize(256);
  fhist[2].SetSize(256);
  ohist[0].SetSize(256);
  ohist[1].SetSize(256);
  ohist[2].SetSize(256);

  // number of potential objects
  hblob.SetSize(100);
  bblob.SetSize(100);

oblob.SetSize(100);

  // processing parameters
  SetSize(640, 360);
  Defaults();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used to define reference floor patches.

int jhcStackSeg::floor_params (const char *fname)
{
  jhcParam *ps = &fps;
  int ok;

  ps->SetTag("tvis_flr", 0);
  ps->NextSpec4( &rdx,     0, "Patch 1 center offset");
  ps->NextSpec4( &rdy,   100, "Patch 1 bottom offset");
  ps->NextSpec4( &rw,    500, "Patch 1 width");  
  ps->NextSpec4( &rh,    250, "Patch 1 height");          // was 200
  ps->NextSpec4( &rdx2,  130, "Patch 2 center offset");
  ps->NextSpec4( &rdy2,  100, "Patch 2 bottom offset");  

  ps->NextSpec4( &rw2,     0, "Patch 2 width");           // was 80
  ps->NextSpec4( &rh2,   120, "Patch 2 height");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for color-based segmentation.

int jhcStackSeg::color_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("tvis_col", 0);
  ps->NextSpec4( &sm,     13,    "Color histogram smoothing");    
  ps->NextSpecF( &rise,    0.2,  "Color histogram mode rise");           
  ps->NextSpecF( &irise,  -1.0,  "Intensity histogram mode rise");           
  ps->NextSpecF( &drop,    0.3,  "Color histogram edge");       // was 0.2 
  ps->NextSpecF( &idrop,   0.1,  "Black histogram edge");       // was 0.3 then 0.05 
  ps->NextSpec4( &dev,    20,    "Color boundary ramp");        // was 5

  ps->NextSpec4( &blur,    3,    "Evidence smoothing");      
  ps->NextSpec4( &pick,  200,    "Evidence threshold");     
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for finding object seeds.

int jhcStackSeg::seed_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("tvis_seed", 0);
  ps->NextSpec4( &keep, 1500,   "Min floor patch area");    // was 2000
  ps->NextSpec4( &fill,  500,   "Floor hole to erase");    
  ps->NextSpec4( &fsm,     5,   "Boundary smoothing");      // was 9
  ps->NextSpec4( &omin,  250,   "Minimum region area");   
  ps->NextSpecF( &asp,     6.0, "Max region elongation");   // was 4
  ps->NextSpec4( &bmax,  150,   "Max bay top width"); 

  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for cleaning up object masks.

int jhcStackSeg::mask_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("tvis_mask", 0);
  ps->NextSpec4( &gap,    20, "Mask gap filling"); 
  ps->NextSpec4( &mfill, 100, "Object hole file");
  ps->NextSpec4( &msm,     9, "Boundary smoothing");   
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcStackSeg::Defaults (const char *fname)
{
  int ok = 1;

  ok &= floor_params(fname);
  ok &= color_params(fname);
  ok &= seed_params(fname);
  ok &= mask_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcStackSeg::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= fps.SaveVals(fname);
  ok &= cps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= mps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Set sizes of internal images directly.

void jhcStackSeg::SetSize (int x, int y)
{
  // remember for later
  iw = x;
  ih = y;

  // pre-processing
  enh.SetSize(x, y, 3);
  boost.SetSize(enh);
  jhcFilter::SetSize(enh);
  wk.SetSize(x, y, 1);
  rg.SetSize(wk);
  yb.SetSize(wk);

  // floor finding
  wk3.SetSize(x, y, 1);
  rg3.SetSize(wk3);
  yb3.SetSize(wk3);
  vote.SetSize(wk3);
  vsm.SetSize(wk3);
  bulk.SetSize(wk3);
  floor.SetSize(wk3);

  // object detection
  hcc.SetSize(x, y, 2);
  bcc.SetSize(hcc);
  holes.SetSize(x, y, 1); 
  floor2.SetSize(holes);
  cirq.SetSize(holes);
  bays.SetSize(holes);

objs.SetSize(holes);
tmp.SetSize(holes);
part.SetSize(holes);
occ.SetSize(x, y, 2);

  cvx.SetSize(x, y, 1);
  mask.SetSize(cvx);
  seed.SetSize(x, y, 2);

  // debugging graphics
  bin.SetSize(x, y, 1);
  line.SetSize(bin);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcStackSeg::Reset ()
{
  int midx = iw / 2;

  jhcFilter::Reset();
  p1.CenterRoi(midx + rdx,  rdy,  rw,  rh); 
  p2.CenterRoi(midx + rdx2, rdy2, rw2, rh2); 
}


//= Perform bulk of processing on input image.

int jhcStackSeg::Analyze (const jhcImg& src)
{
  // image cleanup and color separation
  Enhance3(enh, src, 2.0);             // was x8
//enh.CopyArr(src);
  Flywheel(enh);
  MaxColor(boost, est, 5.0);
  ColorDiffs(rg, yb, boost);
  Intensity(wk, est);

  // find non-floor areas
  floor_area();
  object_detect();

  return 1;
}


//= Find image region likely to be floor based on color in patches.
// uses "wk", "rg", and "yb" color breakdown of source
// results in images "bulk" and less tattered "floor" mask

void jhcStackSeg::floor_area ()
{
  // get color statistics of main reference patch
  HistRegion8(fhist[0], rg, p1);
  HistRegion8(fhist[1], yb, p1);
  HistRegion8(fhist[2], wk, p1);

  // add in secondary reference patch (if any)
  HistRegion8(fhist[0], rg, p2, 0);
  HistRegion8(fhist[1], yb, p2, 0);
  HistRegion8(fhist[2], wk, p2, 0);

  // find floor colored regions 
  color_desc(flims, fhist);
  same_color(vsm, flims, NULL);

  // keep big, erase holes, smooth
  RemSmall(bulk, vsm, 0.0, keep, pick);
  FillHoles(floor, bulk, fill);
  BoxThresh(floor, floor, fsm, 80);
}


//= Looks for potential objects as convex exceptions to the background.

void jhcStackSeg::object_detect ()
{
  // find totally enclosed regions
  CComps4(hcc, floor, omin, -128);
  ok_regions(holes, hblob, hcc);

chunkify(oblob, occ, holes);


/*
  // look for incursions into remaining floor
  UnderGate(floor2, floor, holes, 128, 255);
  SmallGapH(cirq, floor2, bmax);
  CComps4(bcc, cirq, omin, 128);
  ok_regions(bays, bblob, bcc);
*/


/*
  jhcImg hcc(ss->wk, 2), holes(ss->wk);
  jhcBBox box(100);

// gripper region
int hsep = 80, jaw = 30, gwid = 50, ght = 100, mid = iw / 2;

glf.CenterRoi(mid - hsep, ght / 2, gwid, ght);
grt.CenterRoi(mid + hsep, ght / 2, gwid, ght);
grip.CenterRoi(mid, jaw / 2, 2 * hsep, jaw);


  // find incursions into floor region
  SmallGapH(mask, floor, 150);

  // break into proto-objects but ignore gripper
  CComps4(seed, mask, fill, 128);
  blob.FindParams(seed);
  blob.PoisonWithin(seed, glf);
  blob.PoisonWithin(seed, grt);
  blob.PoisonWithin(seed, grip);


//MinRun(seed, mask, 4.0);
*/
}


//= Reshape object blobs one at a time to give new components image and stats.

void jhcStackSeg::chunkify (jhcBlob& b, jhcImg& cc, const jhcImg& seeds) 
{
  jhcRoi area;
  int i, n;

part.MaxRoi();   
part.FillArr(128);

jhcImg cc2(cc);
jhcBBox b2;


  // find components in binary mask
  tmp.FillArr(0);
  CComps4(cc, seeds);
  n = b.FindBBox(cc);

  // scan through individual objects
  for (i = 1; i < n; i++)
    if (b.GetStatus(i) > 0)
    {
      // establish region to constrain processing to
      b.GetRoi(area, i);
      area.GrowRoi(1, 1);
      cc.CopyRoi(area);

      // get mask for single object and bridge small gaps
      b.MarkBlob(part, cc, i);
      Convexify(part, part, gap);

      // fill small holes (except around edges of bounding box)
      CComps4(cc2, part, 0, -128);
      b2.FindBBox(cc2);
      b2.RemTouch(area);
      b2.AreaThresh(-mfill);
      b2.MarkOver(part, cc2, 0, 255, 0);

      // smooth object outline some more
      BoxThresh(part, part, msm, 128);
      MaxFcn(tmp, tmp, part);
      tmp.MaxRoi();
    }

  // break final reshaped objects into components
  CComps4(cc, tmp);
  b.FindParams(cc);
}


//= Finds and keeps only reasonable regions in blob list.
// returns number of regions that passed

int jhcStackSeg::ok_regions (jhcImg& dest, jhcBlob& data, const jhcImg& cc) const
{
  data.FindParams(cc);
//  data.RemBorder(iw, ih, 1, 1, 1, 0);
  data.AspectThresh(-asp);
  data.ThreshValid(dest, cc);
  return data.CountOver();
}


///////////////////////////////////////////////////////////////////////////
//                            Color Predicate                            //
///////////////////////////////////////////////////////////////////////////

//= Figure out 6 component color passband valuess given 3 color histograms.
// largely copied from jhcColorSeg::ColorBG
// NOTE: modifies incoming histograms (mostly smoothing)

void jhcStackSeg::color_desc (int *lims, jhcArr *cols) const
{
  jhcArr hist(256);
  int pk;

  // histogram likely background area in red-green
  // and find values corresponding to strongest peak
  hist.Boxcar(cols[0], sm);
  cols[0].Boxcar(hist, sm);
  pk  = cols[0].MaxBin();
  lims[0] = cols[0].PeakLeft( pk, drop, -1, 0.0, rise) - 1;
  lims[1] = cols[0].PeakRight(pk, drop, -1, 0.0, rise) + 1;

  // histogram likely background area in yellow-blue
  // and find values corresponding to strongest peak
  hist.Boxcar(cols[1], sm);
  cols[1].Boxcar(hist, sm);
  pk  = cols[1].MaxBin();
  lims[2] = cols[1].PeakLeft( pk, drop, -1, 0.0, rise) - 1;
  lims[3] = cols[1].PeakRight(pk, drop, -1, 0.0, rise) + 1;

  // histogram likely background in white-black
  // and find values corresponding to strongest peak
  cols[2].ASet(0, 0);
  cols[2].ASet(255, 0);
  hist.Boxcar(cols[2], sm);
  cols[2].Boxcar(hist, sm);
  pk  = cols[2].MaxBin();
  lims[4] = cols[2].PeakLeft( pk, idrop, -1, 0.0, irise) - 1;
  lims[5] = cols[2].PeakRight(pk,  drop, -1, 0.0, irise) + 1;
}


//= Find similar colored region based on color limits.
// can restrict computation to certain area if ROI supplied
// output image is similarity measure, not a binary mask

void jhcStackSeg::same_color (jhcImg& sim, int *lims, const jhcRoi *area)
{
  // set up computation region
  if (area != NULL)
  {
    rg.CopyRoi(*area);
    yb.CopyRoi(*area);
    wk.CopyRoi(*area);
  }
  else
  {
    rg.MaxRoi();
    yb.MaxRoi();
    wk.MaxRoi();
  }

  // get support in various color bands
  InRange(rg3, rg, lims[0] - dev, lims[1] + dev, dev);
  InRange(yb3, yb, lims[2] - dev, lims[3] + dev, dev);     
  InRange(wk3, wk, lims[4] - dev, lims[5] + dev, dev);     

  // combine bands and smooth
  AvgFcn(vote, rg3, yb3); 
  Blend(vote, vote, wk3, 0.6667);              
  BoxAvg(sim, vote, blur);

  // make sure ROIs are restored
  if (area != NULL)
  {
    rg.MaxRoi();
    yb.MaxRoi();
    wk.MaxRoi();
  }
}


///////////////////////////////////////////////////////////////////////////
//                            Region Selection                           //
///////////////////////////////////////////////////////////////////////////

//= Find the object with centroid above the given y closest to the (x y) point.
// returns object index, 0 if none suitable

int jhcStackSeg::CloseAbove (int x, int y) const
{
  double dx, dy, d2, best;
  int i, n = oblob.Active(), focus = 0;

  // scan all active blobs
  for (i = 1; i < n; i++)
    if (oblob.GetStatus(i) > 0)
    {
       // make sure centroid high enough
       oblob.BlobCentroid(&dx, &dy, i);
       dx -= x; 
       dy -= y;
       if (dy < 0.0)
         continue;

       // keep one closest to point
       d2 = dx * dx + dy * dy;
       if ((focus <= 0) || (d2 < best))
       {
         best = d2;
         focus = i;
       }
    }
  return focus;
}


//= Get binary mask associated with some item.
// pads by one black pixel all around and sets destination ROI
// can optionally clear whole original image for prettier debugging

int jhcStackSeg::PadMask (jhcImg& dest, int n, int clr) const
{
  if (clr > 0)
    dest.FillMax(0);
  return oblob.TightMask(dest, occ, n, 1);
}


///////////////////////////////////////////////////////////////////////////
//                            Object Properties                         //
///////////////////////////////////////////////////////////////////////////

//= Report min y of selected object's bounding box.

int jhcStackSeg::Bottom (int i) const
{
  return ROUND(oblob.BoxBot(i));
}


//= Report x span of selected object's bounding box.

int jhcStackSeg::WidthX (int i) const
{
  return ROUND(oblob.BoxW(i));
}


//= Report y span of selected object's bounding box.

int jhcStackSeg::HeightY (int i) const
{
  return ROUND(oblob.BoxH(i));
}


//= Report actual pixel count of selected object's mask.

int jhcStackSeg::AreaPels (int i) const
{
  return oblob.PixelCnt(i);
}


//= Report inferred pixels per inch at bottom of selected object.
// uses camera tilt and FOV assuming object is resting on flat ground plane

double jhcStackSeg::BotScale (int i) const
{
  return 32.0;
}


///////////////////////////////////////////////////////////////////////////
//                            Debugging Graphics                         //
///////////////////////////////////////////////////////////////////////////

//= Draw outline around a particular object.

int jhcStackSeg::Contour (jhcImg& dest, jhcImg& src, int n, int t, int r, int g, int b)
{
  // create padded binary mask
  if (oblob.TightMask(bin, occ, n, (t / 2) + 1) <= 0)
    return 0;

  // get equivalent fat outline
  line.FillMax(0);
  Outline(line, bin);
  BoxThresh(line, line, t, 20);

  // overlay on source image
  line.MaxRoi();
//  if (src.Valid(3))
//    UnderGateRGB(dest, src, line, 128, r, g, b);
//  else
    UnderGate(dest, src, line, 128, r);
  return 1;
}

