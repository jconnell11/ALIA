// jhcBgSub.cpp : background subtraction and updating
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2001-2013 IBM Corporation
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

#include "System/jhcBgSub.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcBgSub::~jhcBgSub ()
{
  // array of de-wobbling values
  if (vdx != NULL)
    delete [] vdx;
}


//= Default constructor creates arrays and sets up defaults.

jhcBgSub::jhcBgSub ()
{
  vdx = NULL;

  objs.SetSize(100);
  regs.SetSize(100);
  ihist.SetSize(256);

  ante = &now2;
  last = &now3;
  pmask = &msk2;
  mask = &msk3;
  mot = &msrc2;
  pmot = &msrc3;
  gmot = &prv2;
  prev = &prv3;
  nej = &e2;
  pej = &e3;
  col = &c2;
  pcol = &c3;

  Defaults();
  Reset(1);
}


///////////////////////////////////////////////////////////////////////////
//                          Processing Parameters                        //
///////////////////////////////////////////////////////////////////////////

//= Set up default values for processing parameters (possibly read from file).

int jhcBgSub::Defaults (const char *fname)
{
  int ok = 1;

  ok &= DefaultsAGC(fname);
  ok &= fix_params(fname);
  ok &= fix2_params(fname);
  ok &= back_params(fname);
  ok &= sal_params(fname);
  ok &= mask_params(fname);

  k = 0;
  steps = 0;
  n = 0;
  src = NULL;
  thmap = 100;
  bad = 0;
  st_bad = 0;

  ct0.DefLims(20, 0, 150);
  ct1.DefLims(240, 50, 255); 
  ct0.FracMove(0.05);
  ct1.FracMove(0.05);
  return ok;
}


//= Parameters for preprocessing to fix up input images.

int jhcBgSub::fix_params (const char *fname)
{
  jhcParam *ps = &fps;
  int ok;

  ps->SetTag("bgs_fix", 0);
  ps->NextSpec2( &boost,  0,   "Contrast stretching");       // was 0 (added 4/05)
  ps->NextSpec2( &wind,   1,   "Camera de-jittering");       // use 2 for fine version
  ps->NextSpec2( &wob,    0,   "Sync wobble fix-up");
  ps->NextSpec2( &ntsc,   1,   "Color artifact removal");   
  ps->NextSpec2( &ksm,    1,   "Temporal smoothing");    
  ps->NextSpec2( &agc,    1,   "Enable AGC/AWB fix");        

  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  tsm = 4;
  return ok;
}


//= More detailed parameters for image preprocessing.

int jhcBgSub::fix2_params (const char *fname)
{
  jhcParam *ps = &dps;
  int ok;

  ps->SetTag("bgs_fix2", 0);
  ps->NextSpec2( &hdes,  100,   "Min img height");           // was 200
  ps->NextSpec2( &maxdup, 30,   "Max duplicated frames");
  ps->NextSpec2( &xrng,    4,   "Max X motion correction");  // sometimes need 12
  ps->NextSpec2( &yrng,    2,   "Max Y motion correction");    
  ps->NextSpec2( &wx,      2,   "Max X sync de-wobble"); 
  ps->Skip();

  ps->NextSpecF( &smax,    2.0, "Max contrast boost");       // sometimes need 8
  ps->NextSpecF( &maxfg,   0.5, "Maximum foreground");       // was 0.8
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  rpt = 5;
  return ok;
}


//= Parameters for using and maintaining background image.

int jhcBgSub::back_params (const char *fname)
{
  jhcParam *ps = &bps;
  int ok;

  ps->SetTag("bgs_back", 0);
  ps->NextSpec2( &bgmv,     10,   "Motion threshold");         // was 20 then 30 then 10
  ps->NextSpec2( &fat,      17,   "Motion expansion");         // was 33
  ps->NextSpec2( &bcnt,     30,   "Noise estimation time");        
  ps->NextSpec2( &sparkle, 192,   "Persistent motion block");  // was 192
  ps->NextSpec2( &still,    30,   "Quiescience count");        // was 25 then 15 then 30
  ps->NextSpec2( &stable,  150,   "Remove after stable");      // was 45

  ps->NextSpec2( &wait,      3,   "Update interval");          // was 30 then 10
  ps->NextSpecF( &bmix,      0.1, "Update mixing");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for evaluating pixel salience.

int jhcBgSub::sal_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("bgs_sal", 0);
  ps->NextSpecF( &rng,      5.0, "Salience range");  
  ps->NextSpecF( &mf,       0.3, "Motion weight");        // was 0.5
  ps->NextSpecF( &ef,       0.3, "Edge change wt");       // was 0.5 then 0.4
  ps->NextSpecF( &cf,       1.0, "Color change wt");
  ps->Skip(2);
  
  ps->NextSpecF( &bf,       0.5, "Max highlight cut");    // was 0.8 then 0.9 then 0.5
  ps->NextSpecF( &df,       2.0, "Max shadow boost");     // was 2.0 for indoors
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for cleaning the foreground mask.

int jhcBgSub::mask_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("bgs_mask", 0);
  ps->NextSpec2( &pass, 150,    "Salience threshold");     // was 100
  ps->NextSpec2( &bd,     3,    "Image border removal");  
  ps->NextSpec2( &mfill,  3,    "Mask gap closure");       // was 5 for hdes = 240
  ps->NextSpec2( &mtrim,  5,    "Mask shrinking");         // was 7
  ps->NextSpecF( &amin,   0.02, "Area min as 1D frac");    // was 0.07
  ps->NextSpec2( &cvx,    0,    "Convexify gap width");
  
  ps->NextSpecF( &hfrac,  0.2,  "Fillable hole fraction");  // was 0.1
  ps->NextSpecF( &afrac,  0.0,  "Smallest obj to biggest"); // was 0.1 then 0.01
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Write all parameter values to asome file so they can be reloaded.

int jhcBgSub::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= SaveValsAGC(fname);
  ok &= fps.SaveVals(fname);
  ok &= dps.SaveVals(fname);
  ok &= bps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= mps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                             Initialization                            //
///////////////////////////////////////////////////////////////////////////

//= Set sizes of internal images based on supplied image.
// can force a color image to monochrome if chroma is too noisy

void jhcBgSub::SetSize (jhcImg& ref, int force_mono)
{
  SetSize(ref.XDim(), ref.YDim(), ref.Fields(), force_mono);
}


//= Set sizes of internal images.

void jhcBgSub::SetSize (int w, int h, int f, int force_mono)
{
  int dw = w, dh = h;

  // pass to internal object
  jhcAGC::SetSizeAGC(w, h, f);

  // for output conversions
  big.SetSize(w, h, 1);

  // forced three field monochrome input
  mono = force_mono;
  bw3.SetSize(w, h, 3);
  nice.SetSize(w, h, 3);

  // for detecting repeated frames
  ante->SetSize(w, h, f);
  last->SetSize(w, h, f);
  diff.SetSize(w, h, 1);

  // for stabilization
  fg2.SetSize(w, h, 3);
  gnow.SetSize(w, h, 1);
  glast.SetSize(w, h, 1);
  shmsk.SetSize(w, h, 1);

  // see if image should be subsampled
  samp = ROUND(h / hdes);
  if (samp > 1)
  {
    dw /= samp;
    dh /= samp;
  }

  // color images (possibly monochrome)
  bg.SetSize(dw, dh, f, hdes);
  st_roi.CopyRoi(bg);

  fixed.SetSize(bg);
  nomov.SetSize(bg);
  ebg.SetSize(bg);
  nomov2.SetSize(w, h, f);
  kal.SetSize(fixed);

  former.SetSize(bg);
  s0.SetSize(bg);
  ctmp.SetSize(bg);
  ctmp2.SetSize(bg);
  sfix.SetSize(bg);
  rfix.SetSize(bg);

  // single field images
  tmp.SetSize(bg, 1);

  heal.SetSize(tmp);

  tmp2.SetSize(tmp);
  tmp3.SetSize(tmp);
  mot->SetSize(tmp);
  rmot.SetSize(tmp);
  gmot->SetSize(tmp);
  prev->SetSize(tmp);
  sal.SetSize(tmp);
  mask->SetSize(tmp);
  pmask->SetSize(tmp);
  pmask2.SetSize(tmp);
  quiet.SetSize(tmp);
  q2.SetSize(tmp);
  qfg.SetSize(tmp);
  csnow.SetSize(tmp);
  csref.SetSize(tmp);
  any.SetSize(tmp);
  map.SetSize(tmp);
  pmot->SetSize(tmp);
  nej->SetSize(tmp);
  pej->SetSize(tmp);
  col->SetSize(tmp);
  pcol->SetSize(tmp);
  label.SetSize(tmp, 2);

  avm.SetSize(tmp);
  rtex.SetSize(tmp);

  refm.SetSize(tmp);

  // for display
  mv.SetSize(tmp);
  ej.SetSize(tmp);
  cdif.SetSize(tmp);
  nosh.SetSize(ctmp);

  // for healing type assessment
  probe.SetSize(tmp);
  probe2.SetSize(tmp);
  probe3.SetSize(tmp);

  // for NTSC fix
  m1.SetSize(tmp);
  mag.SetSize(tmp);
  dir.SetSize(tmp);
  m3.SetSize(tmp, 3);

  // double field blob labels
  comp.SetSize(bg, 2);

  // internal object
  SetSizeAGC(bg);

  // array of sync de-wobbling values
  if (vdx != NULL)
    delete [] vdx;
  vdx = new double[h];
}


//= Reset state variables.
// Call at start of video sequence. Resets segmentation threshold, 
// reverts background to stored image, noise estimates to their defaults.
// Can optionally invalidate any stored background image.

void jhcBgSub::Reset (int bgclr)
{
  if (bgclr > 0)
  {
    bg.FillArr(0);         // worry whether validly sized yet?
    map.FillArr(0);
    bgok = 0;
  }
  else if (bg.Valid())
    SetGainRef(bg);
  ResetAGC();
  kal.Reset();
  quiet.FillArr(0);
  q2.FillArr(0);
  qfg.FillArr(0);
  mask->FillArr(0);
  pmask->FillArr(0);
  pmask2.FillArr(0);
  ante->FillArr(0);
  last->FillArr(0);
  heal.FillArr(0);
  any.FillArr(0);
  n = 0;
  first = 1;
  pmot->FillArr(0);
  pej->FillArr(0);
  pcol->FillArr(0);
  knock = 0;
  pushed = 0;
  dup = 0;
  bad = 0;
  st_bad = -50;
  fdx = 0.0;
  fdy = 0.0;
  off = 0;
  sc = 1.0;
  w = 1;           // for compatibility with 4 part
  st_cnt = -1;
  nuke = 0;
  hcnt = 0;
  avm.FillArr(0);
  ct0.Reset(1);
  ct1.Reset(1);
  nobjs = 0;
}


//= Tells status of BGS system and whether results are valid.
// returns 1 = ok, 0 = estimating noise, -1 = no bg

int jhcBgSub::Status ()
{
  if ((bgok <= 0) || !bg.Valid())
    return -1;
  if (n < bcnt)
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Background Functions                        //
///////////////////////////////////////////////////////////////////////////

//= Save the given image as the current background and stores for reset.
// can save some noise estimates for reset (if they are non-zero)

void jhcBgSub::SetBG (jhcImg& ref, int rest, int gest, int best)
{
  SetSize(ref);
  Sample(bg, ref);
  map.FillArr(255);
  bgok = 1;
  Reset(0);
  NoiseDefaults(rest, gest, best);
}


//= Loads background images and noise estimates if they were saved.
// returns 1 if ok, 0 if could not open, -1 if format problem

int jhcBgSub::LoadBG (const char *fname)
{
  int ans, w, h, f;
  jhcImg raw;
  UC8 nv[3] = {0, 0, 0};

  // try to read image header to find sizes
  ans = fio.Specs(&w, &h, &f, fname, 1);
  if (ans < 0)
    return -1;
  if (ans == 0)
    return 0;
  raw.SetSize(w, h, f);
  SetSize(w, h, f);

  // load images and maybe noise estimates
  fio.Load(raw, fname, 1, 3, nv);
  SetBG(raw, nv[2], nv[1], nv[0]);
  return 1;
}


//= Saves current background image and noise estimates.
// returns 1 if ok, 0 if could not open, -1 if no bg, -2 if other error

int jhcBgSub::SaveBG (const char *fname)
{
  int ans;
  UC8 nv[3];

  // make sure there is something to save
  if (!bg.Valid())
    return Complain("jhcBgSub - No background image exists yet!");
  
  // save image and noise estimates
  nv[0] = (UC8)(iNoiseB());
  nv[1] = (UC8)(iNoiseG());
  nv[2] = (UC8)(iNoiseR());
  ans = fio.Save(fname, bg, 1, 3, nv);
  if (ans < 0)
    return -2;
  return ans;
}


//= Average new image into background assuming no foreground objects or AGC.
// returns status 0 = more images needed, 1 = enough for noise estimate

int jhcBgSub::MergeBG (jhcImg& now)
{
  // see if first image else check for correct size
  if ((bgok <= 0) || !bg.Valid())
  {
    SetSize(now);
    Sample(bg, now);
    bgok = 1;
    SetGainRef(bg);
    return Status();
  }
  if (Sample(sfix, now) < 1)
    return Fatal("Bad images to jhcBgSub::MergeBG");

  // update noise values and slowly change background 
  EstNoise(sfix, &bg, NULL);
  MixToward(bg, sfix, bg, bmix, 1);
  n++;
  return Status();
}


//= Shrink foreground to mask given by copying "now" into "bg".
// fgmsk is 255 where you want to "push" bad FG to BG
// useful for removing stationary objects

int jhcBgSub::ForceBG (jhcImg& fgmsk, jhcImg& now)
{
  if (!bg.SameFormat(now) || !bg.SameSize(fgmsk, 1))
    return Fatal("Bad images to jhcBgSub::ForceBG");

  OverGate(tmp, fgmsk, *mask, 128);
  SubstOver(bg, now, tmp, 128);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Foreground Finding                          //
///////////////////////////////////////////////////////////////////////////

//= BASIC CALL: take a new image, find foreground, and update stats.
// will automatically reduce or expand images to correct size
// can return a copy of the foreground mask (possibly resized)
// otherwise can read "mask" member variable or call "FullMask"
// if the cc argument is positive, returns component labelled image 
// returns -2 if BG reset, -1 if BG patched, 0 if repeat frame, 1 if all OK

int jhcBgSub::FindFG (jhcImg& now, jhcImg *fgmsk, int cc)
{
  jhcImg *src = &bw3;
  int ans = 1;

  if (!now.SameSize0(fgmsk, 1))
    return Fatal("Bad images to jhcBgSub::FindFG");

  // check for repeated frames
  if (check_repeat(now) <= 0)
    dup = 0;
  else
  {
    if (fgmsk != NULL)
      FullMask(*fgmsk, cc);
    if (dup++ < maxdup)
    {
      ans = 0;               // to make more similar to 4 part version
      return 0;
    }
    else
    {
      knock = 1;
      return -2;
    }
  }

  // for compatibility with 4 part
  if (steps > 0)
  {
    emap.Clone(map);
    emask.Clone(*mask);
  }
  ebg.CopyArr(bg);  

  // finish background healing and keep history
  update_bg_1();
  swap_imgs(&ante, &last);
  last->CopyArr(now);
//  ebg.CopyArr(bg);  

  // possibly force to monochrome
  if (mono > 0)
    Mono3(bw3, now, mono);
  else
    src = &now;

  // preprocess image and check for camera catastrophe
  if (fix_input(*src) <= 0)
  {
    Reset(1);
    if (fgmsk != NULL)
      fgmsk->FillArr(0);
    return -2;
  }

  // do basic work
  if (salience(sal, sfix, rfix) > 0)
  {
    swap_imgs(&pmask, &mask);
    clean_mask(*mask, sal, pass);
    update_bg_0(*mask);           // use downsampled image created by MaskFG
  }
  if (fgmsk != NULL)
    FullMask(*fgmsk, cc);         // get mask after healing

  // see if camera knocked
  if (knock > 0)
    return -2;
  if (pushed > 0)
    return -1;
  return ans;
}


//= Check for repeated frames.

int jhcBgSub::check_repeat (jhcImg& now)
{
/*
  if (now.Fields() == 3)
    WtdSAD_RGB(diff, now, *last);
  else
    AbsDiff(diff, now, *last);
  if (FracOver(diff, 2) < 0.01)
    return 1;
*/
  double dth = 0.01;
  int diff, cnt = ROUND(dth * now.RoiArea()), th = 2;  // 2 * 3 = 6
  int x, y, rw = now.RoiW(), rh = now.RoiH(), rsk = now.RoiSkip();
  const UC8 *a = now.RoiSrc(), *b = last->RoiSrc(now);

  // new versions use early termination
  if (now.Fields() == 3)
  {
    th *= 3;
    for (y = rh; y > 0; y--)
    {
      for (x = rw; x > 0; x--, a += 3, b += 3)
      {
        // see if difference above threshold
        diff =  abs(a[0] - b[0]);
        diff += abs(a[1] - b[1]);
        diff += abs(a[2] - b[2]);
        if (diff <= th)
          continue;
        
        // if all allowed pixels used up then report non-duplicate
        if (--cnt <= 0)
          return 0;
      } 
      a += rsk;
      b += rsk;
    }
    return 1;
  }

  // monochrome version
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--, a++, b++)
      if (abs(*a - *b) <= th)
        continue;
      else if (--cnt <= 0)
        return 0;
    a += rsk;
    b += rsk;
  }
  return 1;
}


//= Override healings where given mask is non-zero.
// allows system to heal only part of a region it proposed
// okay for mask pixels to be outside of proposed healing regions
  
int jhcBgSub::VetoAreas (jhcImg& keep)
{
  // mix prohibitions with proposals to get error mask
  if (Sample(tmp, keep) <= 0)
     return 0;
  OverGate(tmp, heal, tmp, 0);
  Threshold(tmp, tmp, 0);

  // zero permanence in heal regions  
  UnderGate(qfg, qfg, tmp);    
  UnderGate(q2, q2, tmp);      
  MaxFcn(pmask2, pmask2, tmp);  // make red components be white
  return 1;
}


//= Override healing of one or more of the regions proposed.
// if reg_num is zero then all are vetoed and remain unhealed

void jhcBgSub::VetoHeal (int reg_num)
{
  // find rejected healing components
  if (reg_num > pushed)
    return;
  else if (reg_num <= 0)
    Threshold(tmp, heal, 0);
  else
    MatchKey(tmp, heal, reg_num);

  // zero permanence in heal regions  
  UnderGate(qfg, qfg, tmp);    
  UnderGate(q2, q2, tmp);      
  MaxFcn(pmask2, pmask2, tmp);  // make red components be white
}


//= Determine it proposed removal region is an addition or subtration.
// returns -1 if object removed, 1 if object added, 0 if unclear

int jhcBgSub::HealType (int reg_num)
{
  int before, after, must; 
  int ring = 5, edge = 100, cnt = 10, esm = ring;  // ring was 7 
  double frac = 0.07, excess = 1.5;

  if ((reg_num <= 0) || (reg_num > pushed))
    return 0;
  
  // build a ring mask around component
  MatchKey(tmp, heal, reg_num);
  BoxAvg(tmp, tmp, ring);
  InRange(tmp, tmp, 16, 240);

  // determine if contour present now for each section of ring
  Threshold(tmp2, csnow, edge);
  OverGate(tmp2, tmp2, tmp, 128);
  BoxThresh(tmp2, tmp2, esm, 20);
OverGate(probe, tmp2, tmp, 128, 85);

  // determine if contour present before for each section of ring
  Threshold(tmp3, csref, edge);
  OverGate(tmp3, tmp3, tmp, 128);
  BoxThresh(tmp3, tmp3, esm, 20);
OverGate(probe2, tmp3, tmp, 128, 85);

  // figure out average edge intensity before and after healing
  LiftDiff(tmp2, tmp2, tmp3);
  OverGate(tmp2, tmp2, tmp, 128, 128);
probe3.CopyArr(tmp2);
  after = CountOver(tmp2, 200);              // added contour
  before = CountUnder(tmp2, 100);            // removed contour
  must = ROUND(frac * CountOver(tmp, 128));  // total possible contour

  // judgement of deposit/withdrawl based on contour change
  if (must < cnt)
    return 0;
  if ((before > must) && (before > ROUND(excess * after)))
    return -1;
  if ((after > must) && (after > ROUND(excess * before)))
    return 1;
  return 0; 
}


///////////////////////////////////////////////////////////////////////////
//                         Image Preprocessing                           //
///////////////////////////////////////////////////////////////////////////

//= Do all sorts of image preprocessing to make input images sfix and rfix.
// will automatically reduce or expand images to correct size
// produces internal shadow-free source and motion difference images
// useful if some other process wants to edit foreground first
// uses ebg not bg to be more compatible with 4 part version

int jhcBgSub::fix_input (jhcImg& now)
{
  // possibly stretch contrast of incoming video
  mfix = 1;
  src = &now;
  if (boost > 0)
  {
    stretch(nice, *src);
    src = &nice;
  }

  if ((wind > 0) || (wob > 0))
  {
    // possibly fix up camera jitters and resize 
    mfix = stabilize(*src, wind, wob);
    Sample(fg2, ebg);                    // for pretty borders use background
    Sample(shmsk, map);                     
    SubstUnder(fg2, *src, shmsk, 128);  // or current image if no background
    if (wob > 0)
      LineShift(fg2, *src, vdx, fdy);
    else
      FracShift(fg2, *src, fdx, fdy);
    fg2.MaxRoi();                       // fix for next stabilization loop
    Smooth(nomov, fg2);                 // works better than LineSamp (or Sample)
    src = &nomov;
  }
  else if (!src->SameFormat(bg))
  {
    // downsample new image (only if needed)
    Smooth(s0, *src);
    src = &s0;
  }
  
  // possibly clean up colors 
  if (ntsc > 0)
    fix_ntsc(*src, *src);

  // possibly smooth image 
  if (ksm > 0) 
  {
    if (mfix <= 0)
      kal.Reset();
    kal.Flywheel(*src);
    src = &(kal.est);
  }

  // perform basic image corrections for camera parameters
  if (agc > 0)
  {
    // do updates only where no foreground and where bg is valid
    // uses clean foreground mask from previous step
    Threshold(tmp, map, -thmap);
    MaxFcn(tmp, *mask, tmp);

    // changed UpdateAGC to use ebg for 4 part compatibility
    UpdateAGC(*src, &ebg, &tmp, 1, 0, mfix);  

    // correct foreground and newer background 
    FixAGC(sfix, *src);
    LimitAGC(rfix, bg);  

    // estimate noise using newer background for 4 part compatibility
    EstNoise0(sfix, rfix, &tmp, 0);
  }
  else
  {
    sfix.CopyArr(*src);
    rfix.CopyArr(bg);
  }

  // save beautified input image 
  // was *src (before AGC) vs. sfix, compromise = undo AGC/AWB
  AdjustRGB(former, sfix, 1.0 / GainR(), 1.0 / GainG(), 1.0 / GainB());
  if (first > 0)
    Sample(bg, former);                  // for beautiful display
  return mfix;
}


//= Take intensity distribution and use more of the available range.
// does a linear offset and scaling on all color channels
// Note that the color of objects might change if ldes > 0 or lpct > 0

void jhcBgSub::stretch (jhcImg& dest, jhcImg& src)
{
  int ilo = 20, ihi = 240;
  double lpct = 0.05, hpct = 0.95;

  // build histogram of original image
  HistAll(ihist, src);
  ihist.ASet(0, 0);
  ihist.ASet(255, 0);
  ihist.Smooth(4);
  ct0.Update(ihist.Percentile(lpct));
  ct1.Update(ihist.Percentile(hpct));

  // apply stretching to image
  sc = (ihi - ilo) / (ct1.val - ct0.val);
  sc = __min(sc, smax);
  off = 0;
  if (sc > 1.0)
  {
    off = ROUND(ilo / sc - ct0.val);
    Offset(dest, src, off);
    ClipScale(dest, dest, sc);
  }
  else
    dest.CopyArr(src);
}


//= Look for sharp vertical edges and suppress nearby color information.

void jhcBgSub::fix_ntsc (jhcImg& dest, jhcImg& src)
{
  // monochrome version of images
  AvgAll(m1, src);

  // restrict to mostly vertical edges 
  DirSel(mag, m1, 45.0, 135.0, 1, 10.0);   // was 8

  // spread regions a little
  BoxAvg(tmp, mag, 5, 3, 3.0);  // 3 1 or 5 3, sc = 3

  // mix down color in that area
  CopyMono(m3, m1);
  Composite(dest, m3, src, tmp);
}


//= Compensate for small amounts of pan and tilt (e.g. camera jiggle).
// saves own copy of monochrome full-sized image and valid regions
// if fine is positive, then does very careful alignment (but slow)
// end result is to set the fdx and fdy image shift variables
// returns 1 if OK, 0 if not enough unmasked or shift out of bounds

int jhcBgSub::stabilize (jhcImg& src, int mot, int sync)
{
  int x0, y0, i, h = src.YDim(), samp = 4;     // xrng was 4 then 12
 
  // default values in case some problem
  fdx = 0.0;
  fdy = 0.0;
  for (i = 0; i < h; i++)
    vdx[i] = 0.0;
  st_roi.FullRoi();

  // see if first frame ever
  if (st_cnt++ < 0)
  {
//    Sample(bg, src);         // for beautiful images
    refm.CopyArr(*mask);
    ForceMono(glast, src);
    st_cnt = 0;
    st_num = 0;
    return 1;                // was 0
  }

  // get black & white image then figure out where comparison will be valid
  ForceMono(gnow, src);      // was style 0
  MaxFcn(tmp, *mask, refm);
  if (FracOver(tmp) > 0.5)
    return 0;
  Sample(shmsk, tmp);
//  BoxThresh(shmsk, shmsk, 17, 20);

  // find best alignment between input and background (and possibly refine)
  if (mot > 0)
    st.AlignCross(&fdx, &fdy, gnow, glast, &shmsk, -xrng, xrng, -yrng, yrng, samp);

  // possibly refine estimate around best coarse guess
  if (mot > 1)
  {
    x0 = -ROUND(fdx);
    y0 = -ROUND(fdy);
    st.AlignFull(&fdx, &fdy, gnow, glast, &shmsk, x0 - 1, x0 + 1, y0 - 1, y0 + 1, 1, 1);
  }

  // possibly undo "tearing" of image due to bad sync also
  if (sync > 0)
    st.EstWobble(vdx, gnow, glast, shmsk, -fdx, -fdy, wx, 1, 3);

  // >> improved version from jhcFixBGS
  // check for reasonable overall shift value
  if ((fabs(fdx) > (xrng - 0.5)) || (fabs(fdy) > (yrng - 0.5)))
  {
    fdx = 0.0;
    fdy = 0.0;
    for (i = 0; i < h; i++)
      vdx[i] = 0.0;
    st_bad = __max(0, st_bad);
    st_bad++;
  }
  else
  {
    st_bad = __min(0, st_bad);
    st_bad--;
  }

  // update local background model occasionally
  // also if too many uncorrectable images until camera settles down
  if ((st_bad >= 0) ||
      ((st_bad > -5) && ((fabs(fdx) >= 2.0) || (fabs(fdy) >= 2.0))) ||
      ((st_cnt > 100) && (fabs(fdx) < 0.1) && (fabs(fdy) < 0.1)))
  {
    glast.CopyArr(gnow);
    refm.CopyArr(*mask);
    st_cnt = 0;
  }
  if (st_bad > -5)  // was 0
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Salience Copmutation                          //
///////////////////////////////////////////////////////////////////////////

//= Combines various cues to give overall evidence of foreground objects.
// as a side-effect creates a shadow-free foreground and a motion image
// average error of a normal distribution is std / sqrt(pi) = 0.564 std
// returns 1 if okay, 0 if repeat image, -1 for error

int jhcBgSub::salience (jhcImg& dest, jhcImg& src, jhcImg& ref)
{
  // check image sizes
  if (!src.SameFormat(ref) || !src.SameSize(dest, 1))
    return Fatal("Bad images to jhcBgSub::Salience");

  // merge basic information sources
  if (fg_motion(dest, src) > 0)
    return 0;
  fg_texture(dest, src, ref);
  fg_color(dest, src, ref);  
  Border(dest, 1, 0);
  return 1;
}


//= Determine motion and use to initialize salience.
// also check if repeated frame
// note computed grayscale image is used later outside

int jhcBgSub::fg_motion (jhcImg& dest, jhcImg& src)
{
  double r = QuietR(), g = QuietG(), b = QuietB();
  double norm = 1.0 / (r + g + b);
  double rn = norm * r, gn = norm * g, bn = norm * b;
  double nmono = rn / (r * sqrt((double)(src.Fields()))), sc = 255.0 / rng; 
  double nmot = nmono * sqrt(2.0), motf = mf * sc / nmot;

  // create monochrome version and difference from last frame
  // skip update (and rest of salience) if a repeat frame 
  WtdSumRGB(*gmot, src, rn, gn, bn);
  if (first <= 0)
  {
    AbsDiff(*mot, *gmot, *prev, 1.0);
    if (mot->YDim() > 200)            // some noise reduction
      BoxAvg(rmot, *mot, 5);
    else 
      BoxAvg(rmot, *mot, 3);
//    if (!AnyNZ(*mot) && (k++ < rpt))  // turns off at beginning?
//      return 1;
    k = 0;
  }
  swap_imgs(&prev, &gmot);
  first = 0;

  // combine two differences (i.e. motion over 3 frames)
  MinFcn(*pmot, *mot, *pmot);   // so background gets written
  swap_imgs(&pmot, &mot);       // pmot = diff of 2, mot = diff of 3

  // scale motion difference as part of salience sum
  if (mf > 0.0)
    Square(dest, *mot, motf);  
  else
    dest.FillArr(0);
  OverGate(dest, dest, map, thmap);  // no motion energy in known noisy areas
  if (steps > 0)
    mv.Clone(dest);
  return 0;
}


//= Look for texture changes and add to salience.
// uses grayscale image computed by fg_motion

void jhcBgSub::fg_texture (jhcImg& dest, jhcImg& src, jhcImg& ref)
{
  double r = QuietR(), g = QuietG(), b = QuietB();
  double norm = 1.0 / (r + g + b);
  double rn = norm * r, gn = norm * g, bn = norm * b;
  double nmono = rn / (r * sqrt((double)(src.Fields()))), sc = 255.0 / rng; 
  double nsej = nmono * sqrt(6.0) / 4.0, ncs = nmono * sqrt(9.0 / 8.0);  
  double sejf = ef * sc / nsej, csf = ef * sc / ncs;

  if (ef <= 0.0)
    return;

  // prev has monochrome version of current image from fg_motion
  // create monochrome background and find several edge types 
  TripleEdge(ctmp, *prev);
  SobelEdge(csnow, *prev, 3.0);       // for deletion type
  WtdSumRGB(tmp, ref, rn, gn, bn); 
  TripleEdge(ctmp2, tmp);
  SobelEdge(csref, tmp, 3.0);         // for deletion type

  // then difference raw edge components 
  WtdSSD_RGB(rtex, ctmp, ctmp2, sejf, sejf, csf);
  Threshold(tmp2, map, thmap);
  BoxThresh(tmp2, tmp2, 5, 254);      // expand invalid bg map for edge support
  OverGate(*nej, rtex, tmp2, 128);
  OverGate(csref, csref, tmp2, 128);  // for deletion type

  Threshold(tmp2, *pej, pass);        // shrink map by 1 pixel
  BoxAvg(tmp2, tmp2, 3);
  OverGate(tmp, *pej, tmp2, 192);
  swap_imgs(&pej, &nej);

  ClipSum(dest, dest, tmp, 1.0);
  if (steps > 0)
    ej.Clone(tmp);
}


//= Look for changes in pixel color or brightness.

void jhcBgSub::fg_color (jhcImg& dest, jhcImg& src, jhcImg& ref)
{
  double r = QuietR(), g = QuietG(), b = QuietB();
  double sc = 255.0 / rng; 
  double rsc = cf * sc * r, gsc = cf * sc * g, bsc = cf * sc * b;

  if (cf <= 0.0)
    return;

  // get differences at basic pixel level after removing shadows
  fix_shadows(ctmp, src, ref);                
  WtdSSD_RGB(tmp, ctmp, ref, rsc, gsc, bsc);
  OverGate(tmp, tmp, map, thmap);
  BoxAvg(tmp2, tmp, 3);               // extra noise suppression
  MinFcn(*col, tmp, tmp2);

  // add to salience and possible save intermediate images
  ClipSum(dest, dest, *pcol, 1.0); 
  if (steps > 0)
  {
    cdif.Clone(*pcol);
    nosh.Clone(ctmp);                             // inside FixShadows?
  }
  swap_imgs(&pcol, &col);
}


//= Multiplies channel values so overall pixel intensity matches reference.
// assumes any AGC/AWB adjustment and limiting has already been done

int jhcBgSub::fix_shadows (jhcImg& sfix, jhcImg& src, jhcImg& ref)
{
  double rsc = QuietR(), gsc = QuietG(), bsc = QuietB();
  double sum = rsc + gsc + bsc;

  // check sizes and handle simple cases
  if (!sfix.SameFormat(src) || !sfix.SameFormat(ref))
    return Fatal("Bad images to jhcBgSub::FixShadows");
  if ((df == 1.0) && (bf == 1.0))
    return sfix.CopyArr(src);

  // compute ratios again and throw out any pixel with any estimate beyond bounds
  // tmp2 holds valid pixels
  double mid = 0.5, mval = mid * 256.0;
  int unity = ROUND(mval), brite = BOUND(ROUND(df * mval)), dark = ROUND(bf * mval);

  // get ratio estimate and keep places where all are within bounds
  NormBy(ctmp, ref, src, mid);
  AllWithin(tmp2, ctmp, dark, brite);

  // combine channel estimates into single gain and gate where valid
  WtdSumRGB(tmp, ctmp, rsc / sum, gsc / sum, bsc / sum);  
  OverGate(tmp, tmp, tmp2, 128, unity); 

  // fix image by applying gains
  MultRGB(sfix, src, tmp);
  return 1;
}


//= Cleans up foreground mask using morphology and components.
// takes raw salience map and thresholds it first

int jhcBgSub::clean_mask (jhcImg& dest, jhcImg &src, int th)
{
  int big, acnt = ROUND(amin * src.XDim() * amin * src.YDim());
  if (!dest.SameFormat(src))
    return Fatal("Bad images to jhcBgSub::CleanMask");

  // threshold, fill small gaps, shrink to compensate for fat edges
  Threshold(dest, src, th);
  Border(dest, bd, 0); 
  BoxThresh(dest, dest, mfill, 55);   // close holes
  BoxThresh(dest, dest, mtrim, 192);

  // remove small components then files holes up to hfrac of largest blob
  Border(dest, 1, 0); 
  big = RemSmall(dest, dest, afrac, acnt, 128);
  if (cvx > 0)
    Convexify(dest, dest, cvx);
  FillHoles(dest, dest, (int)(hfrac * big), 128);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Background Internals                          //
///////////////////////////////////////////////////////////////////////////

//= Starts process of background updating.
// checks for knocks, incrementally adds regions, and proposes healing
// uses internal shadow free source and motion regions images to compute

int jhcBgSub::update_bg_0 (jhcImg& fgmsk)
{
  if (bg_flush(fgmsk) > 0)
    return 0;
  bg_build(fgmsk);
  bg_heal();
  return 1;
}


//= Finishes process of background updating by performing authorized heals.
// update all internal state based on foreground mask (possibly edited)

void jhcBgSub::update_bg_1 ()
{
  bg_push();
  bg_smooth();
}


//= If camera angle changed or obstructed, re-intialize background model.

int jhcBgSub::bg_flush (jhcImg& fgmsk)
{
  // reset bg if too much fg for too int
  if ((FracOver(fgmsk, 128) > maxfg) || (st_bad > 0))
  {
    if (++bad > still)
    {
      Reset(1);
      knock = 1;
    }
    return 1;
  }

  // if fg is currently ok, either proint reset or reduce agitation level 
  // waits for "still" bad frames before barfing 
  // then waits another "still" frames in Reset mode before rebuilding 
  if (bad > still)
    bad = -still;
  else if (bad > 0)
    bad--;
  if (bad < 0)
  {
    bad++;
    Reset(1);
    knock = 1;
    return 1;
  }
  knock = 0;
  return 0;
}


//= Progressively build up background model where no motion.

void jhcBgSub::bg_build (jhcImg& fgmsk)
{
  int dec = 1 + (sparkle / 256), nth = 4 * dec, sp = sparkle / dec;
/*
  // update where not foreground and bg is valid
  Threshold(tmp, map, thmap);
  MaxComp2(tmp, fgmsk, tmp);
  UpdateAGC(&tmp);  
*/

  // expand foreground region and zero there, increment elsewhere
  Threshold(tmp, *mot, bgmv);
  BoxThresh(tmp, tmp, 3, 128);

  MaxFcn(tmp, tmp, fgmsk);
  BoxAvg(tmp, tmp, fat, fat, 4.0);
  Threshold(tmp, tmp, 0);
  Offset(quiet, quiet, 1);
  UnderGate(quiet, quiet, tmp, 128, 0); 

  // keep big map of where background is valid (holds stable count)
  Threshold(tmp, quiet, still);
  MinComp2(tmp2, tmp, map);           // new regions
  SubstOver(bg, former, tmp2, 128);   // initialize right away 
  MaxFcn(map, map, tmp);              // was tmp2

  // nuke background where high average motion
  Threshold(rtex, rtex, 128);
  BoxAvg(rtex, rtex, 9);
  if (nuke++ >= nth)                  // used to be 4 always
  {
    Offset(avm, avm, -1);
    IncOver(avm, rtex, 3, 32);
    nuke = 0;
  }
  if (sparkle > 0)                    // set to zero to turn off
    UnderGate(map, map, avm, sp, 0);  // used to compare to sparkle
}


//= Eventually absorb blobs with no internal motion.

void jhcBgSub::bg_heal ()
{
  int dec = 1 + (stable / 256), st = stable / dec;

  // keep map of regions where any motion
  Threshold(tmp, rmot, bgmv);               // was mot
  BoxThresh(tmp, tmp, 3, 80);  // was 128
  BoxAvg(tmp, tmp, fat, fat, 4.0);
  Threshold(tmp, tmp, 0);

  // increment quiet count where no recent motion
  if (++hcnt >= dec)
  {
    Offset(q2, q2, 1);                      // used to update on every frame
    hcnt = 0;
  }
  UnderGate(q2, q2, tmp, 128, 0); 

  // mark each component with lowest value it contains
  CComps4(comp, *mask, 0, 128);
  regs.MinEach(comp, q2);
  regs.MapParam(qfg, comp, 12, 255);
  UnderGate(*mask, *mask, qfg, st, 200);   // mark non-fidgetting as red (was "stable")

  // find proposed healing regions
  MatchKey(heal, *mask, 200, 1);           // find red deleted parts 
  regs.MaxEach(comp, pmask2);              // see if foreground in last frame
  regs.MapParam(tmp, comp, 12, 255);
  OverGate(heal, heal, tmp, 254);          // must be white turned to red

  // count and label components
  pushed = CComps4(label, heal);
  heal.CopyField(label, 0);
  pmask2.CopyArr(*mask);
}


//= Do erasing and update stillness too.
// also heals transitory regions not reported as objects

void jhcBgSub::bg_push ()
{
  int dec = 1 + (stable / 256), st = stable / dec;
 
  Threshold(tmp, qfg, st - 1);          // was "stable - 1"
  BoxAvg(tmp, tmp, fat, fat, 4.0);
  UnderGate(quiet, quiet, tmp, 1, st);  // was "stable" 
  SubstOver(bg, former, tmp, st - 1);   // was "stable - 1"
}


//= Mix in new image except in foreground.

void jhcBgSub::bg_smooth ()
{
  if (--w <= 0)
  {
    w = wait;
    MixToward(ctmp, former, bg, 0.1, 1);  
    SubstOver(bg, ctmp, quiet, still);
  }
}


///////////////////////////////////////////////////////////////////////////
//                          Size Changing Functions                      //
///////////////////////////////////////////////////////////////////////////

//= Returns binary mask for foreground regions, smooths if upsampling.
// if cc is positive, returns connected component image (unsmoothed)
// if cc is negative returns optionally smoothed version 

int jhcBgSub::FullMask (jhcImg& fgmsk, int cc)
{
  int f;

  if (!fgmsk.Valid(1))
    return Fatal("Bad images to jhcBgSub::FullMask");
  
  // run connected components and bounding boxes on mask
  if (cc > 0)
  {
    ParseFG();                    // builds valid comp image
    tmp.CopyField(comp, 0, 0);
    Sample(fgmsk, tmp);
    return 1;
  }

  // get non-red mask plus proposed healing areas
  Threshold(tmp, *mask, 254);
  UnderGate(tmp, tmp, heal, 1, 255);

  // expand and possibly smooth outlines
  Sample(fgmsk, tmp);
  f = fgmsk.YDim() / mask->YDim(); 
  if ((cc < 0) && (f > 1))
    BoxThresh(fgmsk, fgmsk, 2 * (f - 1) + 1, 128);
  return 1;
}


//= Shows the standard "windows through blue" version of foreground
// can optionally smooth foreground mask first

int jhcBgSub::FullFG (jhcImg& dest, int sm, int rdef, int gdef, int bdef)
{
  if (sm > 0)
    FullMask(big, -1);
  else
    FullMask(big, 0);
  return OverGateRGB(dest, *ante, big, 128, rdef, gdef, bdef);
}


//= Shows internal background image at some other scale.
// can optionally smooth output and window by validity mask

int jhcBgSub::FullBG (jhcImg& bgcopy, int sm, int val)
{
  jhcImg *src = &bg;
  int f;

  if (bgcopy.Fields() != bg.Fields())
    return Fatal("Bad image to jhcBgSub::FullBG");

  // get a big version of either full background or windowed portion
  if (val > 0)
  {
    OverGateRGB(ctmp, bg, map, 128, 0, 0, 0);
    src = &ctmp;
  }  
  Sample(bgcopy, *src);

  // possibly smooth result
  f = bgcopy.YDim() / bg.YDim(); 
  if ((sm > 0) && (f > 1))
    BoxAvgRGB(bgcopy, bgcopy, 2 * (f - 1) + 1);
  return 1;
}


//= Shows internal combined salience image at some other scale.
// can optionally smooth output

int jhcBgSub::FullSal (jhcImg& scores, int sm)
{
  jhcImg *src = &scores;
  int f;

  if (!scores.Valid(1, 3))
    return Fatal("Bad image to jhcBgSub::FullSal");

  // create a big version either directly in the output or in a temp
  if (scores.Valid(1))
    Sample(scores, sal);
  else
  {
    Sample(big, sal);
    src = &big;
  }

  // see if smoothing requested
  f = scores.YDim() / sal.YDim(); 
  if ((sm > 0) && (f > 1))
    BoxAvg(*src, *src, 2 * (f - 1) + 1);

  // for color output convert values to RGB
  if (scores.Valid(3))
    FalseColor(scores, *src);
  return 1;
}


//= Shows components forced into the background.
// returns the number of individual items 

int jhcBgSub::FullHeal (jhcImg& erased)
{
  Sample(erased, heal);
  return pushed;
}


//= Return input frame delayed by one cycle (to match FG mask).

int jhcBgSub::FullDelay (jhcImg& image)
{
  return Sample(image, *ante);
}


///////////////////////////////////////////////////////////////////////////
//                          Component Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Analyze foreground mask into connect components.
// returns number of distinct blobs found
// will do complete foreground finding if "now" image supplied

int jhcBgSub::ParseFG (jhcImg *now)
{
  if (now != NULL)
    FindFG(*now);

  // get true mask  and analyze (ignore special healing marking)
  Threshold(tmp, *mask, 254);          
  UnderGate(tmp, tmp, heal, 1, 255);
  CComps4(comp, tmp, 0, 128);
  objs.FindBBox(comp);
  nobjs = ObjectCnt();
  return nobjs;
}


//= Returns number of distinct blobs found.

int jhcBgSub::ObjectCnt ()
{
  return(objs.CountOver());
}


//= Returns the bounding box for some component in full-size image coords.

int jhcBgSub::ObjectBox (jhcRoi& spec, int n)
{
  int i = objs.IndexOver(n);

  if (i < 0)
    return 0;
  spec.CopyRoi(*(objs.GetRoi(i)));
  spec.ScaleRoi((double) samp);
  return 1;
}


//= Like other ObjectBox but returns 4 separate numbers.

int jhcBgSub::ObjectBox (int *cx, int *cy, int *w, int *h, int n)
{
  jhcRoi ans;

  if (ObjectBox(ans, n) <= 0)
    return 0;
  if (cx != NULL)
    *cx = ans.RoiX();
  if (cy != NULL)
    *cy = ans.RoiY();
  if (w != NULL)
    *w  = ans.RoiW();
  if (h != NULL)
    *h  = ans.RoiH();
  return 1;
}


//= Returns the full-size mask for the specified blob.
// assumes carve is big enough for component and sets image ROI
// only contains pixels from a single blob (if bboxes overlap)

int jhcBgSub::ObjectMask (jhcImg& carve, int n)
{
  int i = objs.IndexOver(n);

  // check for good image and object number
  if (!carve.Valid(1))
    return Fatal("Bad image to jhcBgSub::ObjectMask");
  if (i < 0)
    return 0;

  // fing pixels beinting to object and copy region around them
  MatchKey(tmp, comp, i);
  if (samp == 1)
    CopyPart(carve, tmp, *(objs.GetRoi(i)));
  else
  {
    CopyPart(tmp2, tmp, *(objs.GetRoi(i)));
    Sample(carve, tmp2);
  }
  return 1;
}

