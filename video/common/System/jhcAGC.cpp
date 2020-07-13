// jhcAGC.cpp : estimates and nulls camera AGC and AWB
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

#include <stdio.h>
#include "Interface/jhcMessage.h"

#include "System/jhcAGC.h"

// Note: altered to allow system to build and maintain its own background

///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor creates histograms and sets defaults.

jhcAGC::jhcAGC ()
{ 
  rh.SetSize(256);
  gh.SetSize(256);
  bh.SetSize(256);
  DefaultsAGC();
  ResetAGC();
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Tell gains currently in use.

const char *jhcAGC::GainTxt (int dec3)
{
  if (dec3 > 0)
    sprintf_s(gmsg, "%5.3f (%5.3f %5.3f %5.3f)", g[0], g[3], g[2], g[1]); 
  else
    sprintf_s(gmsg, "%4.2f (%4.2f %4.2f %4.2f)", g[0], g[3], g[2], g[1]); 
  return ((const char *) gmsg);
}


//= Tell current noise estimates.

const char *jhcAGC::NoiseTxt ()
{
  sprintf_s(nmsg, "(%4.2f %4.2f %4.2f)", rn.val, gn.val, bn.val);
  return ((const char *) nmsg);
}


//= Load all values, possible from a file.

int jhcAGC::DefaultsAGC (const char *fname)
{
  int ok = 1;

  ok &= gain_params(fname);
  ok &= noise_params(fname);

  // other parameters
  gwait = 5;       
  gsamp = 300;     
  bmix = 0.1;  
  return ok;
}


//= Load default operating parameter values.
// given the brightness restrictions, agc1 should be under sqrt(ihi / iho)

int jhcAGC::gain_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("agc_gain", 0);
  ps->NextSpecF( &agc1,   2.0, "Max intensity boost");  
  ps->NextSpecF( &agc0,   0.5, "Max intensity cut");
  ps->NextSpecF( &awb1,   1.5, "Max channel boost/cut");
  ps->NextSpec2( &ihi,  240,   "Max valid intensity");
  ps->NextSpec2( &ilo,   50,   "Min valid intensity");
  ps->NextSpec2( &hagc,  90,   "Internal image height");

  ps->NextSpecF( &gfrac,  0.1, "Min histogram frac");
  ps->NextSpecF( &gmix,   0.5, "Gain update move");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Load default operating parameter values for noise estimation.

int jhcAGC::noise_params (const char *fname)
{
  jhcParam *ps = &nps;
  int ok;

  ps->SetTag("agc_noise", 0);
  ps->NextSpec2( &(bn.vmax), 80,    "Max blue noise");
  ps->NextSpec2( &(rn.vmax), 30,    "Max red and green");
  ps->NextSpec2( &(rn.vmin),  5,    "Min val for all");  
  ps->NextSpec2( &(rn.vdef), 10,    "Default noise");  
  ps->NextSpec2( &nsm,        8,    "Histogram smoothing");
  ps->NextSpecF( &ndrop,      0.1,  "Histogram drop");

  ps->NextSpecF( &nfrac,      0.1,  "Min histogram frac");
  ps->NextSpecF( &(rn.frac),  0.05, "Noise update move");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  NoiseCopy();
  return ok;
}


//= Save parameter values to a file for later reloading.

int jhcAGC::SaveValsAGC (const char *fname) const
{
  int ok = 1;

  ok &= gps.SaveVals(fname);
  ok &= nps.SaveVals(fname);
  return ok;
}


//= Copy most properties of red noise into other colors.

void jhcAGC::NoiseCopy ()
{
  rn.move = 0;

  gn.move = 0;
  gn.frac = rn.frac;
  gn.vdef = rn.vdef;
  gn.vmin = rn.vmin;
  gn.vmax = rn.vmax;
  
  bn.move = 0;
  bn.frac = rn.frac;
  bn.vdef = rn.vdef;
  bn.vmin = rn.vmin;
}


//= Set size of internal images based on a sample input image.

void jhcAGC::SetSizeAGC (jhcImg& ref)
{
  SetSizeAGC(ref.XDim(), ref.YDim(), ref.Fields());
}


//= Pre-size images to prevent allocation during critical runtime.
// internal size of image is often less than original

void jhcAGC::SetSizeAGC (int w, int h, int f)
{
  cmid.AdjSize(w, h, f, __min(hagc, h));
  ctmp.SetSize(cmid);
  rats.SetSize(cmid);
  gref.SetSize(cmid);
  gate.SetSize(cmid, 1);
  tmp.SetSize(gate);
}


//= Set just the size of the AGC/AWB images, not noise estimator.
// this is usually not useful by itself (use SetSizeAGC)

void jhcAGC::SetGainSize (jhcImg& ref)
{
  SetSizeAGC(ref);
}


//= Store a background image for comparison with input.
// better than just accessing member variable because sets size

void jhcAGC::SetGainRef (jhcImg& truth)
{
  SetSizeAGC(truth);
  sz.Sample(gref, truth);
  ResetAGC();
  ref_mode = 2;   // user supplied image
}


//= Blend in current image with pre-existing background.

void jhcAGC::UpdateRef (jhcImg& current)
{
  if (ref_mode <= 0)
  {
    sz.Sample(gref, current);
    ref_mode = 1;
  }
  else
    al.MixToward(gref, current, gref, bmix, 1);   // blend in current image
}


///////////////////////////////////////////////////////////////////////////
//                           Simple Operations                           //
///////////////////////////////////////////////////////////////////////////

//= Resets both gains and noise estimates.

void jhcAGC::ResetAGC ()
{
  NoiseCopy();
  ResetNoise();
  ResetGains();
}
  

//= Resets just the noise estimate to given values (if any).

void jhcAGC::ResetNoise (int r, int g, int b)
{
  if (r > 0.0)
    rn.Force((double) r);
  else
    rn.Reset(1);
  if (g > 0.0)
    gn.Force((double) g);
  else
    gn.Reset(1);
  if (b > 0.0)
    bn.Force((double) b);
  else
    bn.Reset(1);
}


//= Set just the defaults for the noise (if given non-zero value).

void jhcAGC::NoiseDefaults (int rest, int gest, int best)
{
  if (rest > 0)
    rn.vdef = rest;
  if (gest > 0)
    gn.vdef = gest;
  if (best > 0)
    bn.vdef = best;
}


//= Relative weights for each color channel to make noise comparable.
// typically used to generate an intensity image (sum of wts = 1)

void jhcAGC::ChannelWts (double *rf, double *gf, double *bf)
{
  double r = rn.Recip(), g = gn.Recip(), b = bn.Recip();
  double norm = 1.0 / (r + g + b);

  if (rf != NULL)
    *rf = norm * r;
  if (gf != NULL)
    *gf = norm * g;
  if (bf != NULL)
    *bf = norm * b;
}


//= Reset all channel gains to one.

void jhcAGC::ResetGains ()
{
  // gains
  g[0] = 1.0;
  g[1] = 1.0;
  g[2] = 1.0;
  g[3] = 1.0;

  // processing status
  iclip = 0;
  cclip = 0;
  adj   = 1;

  // reference status
  ref_mode = 0;   // no reference yet
  gbad = 0;
  gcnt = 0;
}


//= Move gains closer to unity values (e.g. if bad foreground).
// time to reach one depends on gmix parameter

void jhcAGC::DecayGains ()
{
  if (cmid.Fields() == 1)
    SetGainMono(1.0);
  else
    SetGainsRGB(1.0, 1.0, 1.0);
}


//= Tell if any clipping has occurred.
// -1 = not estimated last cycle, 0 = clipped, 1 = fine

int jhcAGC::GainStatus ()
{
  if (adj <= 0)
    return -1;
  if ((iclip > 0) || (cclip > 0))
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Correct channel gains using most recent parameters.
// returns 1 if well adjusted, 0 if only partial, -1 if guess

int jhcAGC::FixAGC (jhcImg& dest, jhcImg& src)
{
  if (!dest.Valid(1, 3) || !dest.SameSize(src))
    return Fatal("Bad images to jhcAGC::FixAGC");

  SetSizeAGC(src);
  if (dest.Fields() == 1)
    lu.ClipScale(dest, src, GainI());
  else
    lu.AdjustRGB(dest, src, GainR(), GainG(), GainB());
  return GainStatus();
}


//= Limit channels in some other image if gains less than one.
// source defaults to stored background image

int jhcAGC::LimitAGC (jhcImg& dest, jhcImg& src)
{
  if (!dest.Valid(1, 3) || !dest.SameSize(src))
    return Fatal("Bad images to jhcAGC::LimitAGC");

  SetSizeAGC(src);
  if (dest.Fields() == 1)
    return lu.LimitMax(dest, src, ROUND(255.0 * GainI()));
  return lu.LimitRGB(dest, src, ROUND(255.0 * GainR()), 
                                ROUND(255.0 * GainG()),
                                ROUND(255.0 * GainB()));
}


//= Just do gain part.

int jhcAGC::EstGains (jhcImg& now, jhcImg *ref, jhcImg *mask, int mok)
{
  return UpdateAGC(now, ref, mask, 1, 0, mok);
}


//= Just do noise part.

int jhcAGC::EstNoise (jhcImg& now, jhcImg *ref, jhcImg *mask, int mok)
{
  return UpdateAGC(now, ref, mask, 0, 1, mok);
}


//= Does noise estimation first, then gain estimation on reduced images.
// can skip one or both steps (encapsulates common defaults and sampling)
// returns 1 if all updates requested are OK, 0 if some are not made

int jhcAGC::UpdateAGC (jhcImg& now, jhcImg *ref, jhcImg *mask, 
                       int nskip, int gskip, int mok)
{
  jhcImg *m = &gate;
  int ans = 1;

  // check image sizes and whether desired update can be performed
  if (!now.Valid(1, 3) || !now.SameFormat0(ref) || !now.SameSize0(mask, 1))
    return Fatal("Bad images to jhcAGC::UpdateAGC");
  if ((nskip <= 0) && (gskip > 0) && (GainStatus() < 1))
    return 0;

  // if known bad motion correction then reference is likely bad too
  if (mok <= 0)
  {
    ResetAGC();
    return 0;
  } 

  // determine what reference image to use
  SetSizeAGC(now);
  if (ref != NULL)
  {
    sz.Sample(gref, *ref);
    ref_mode = 2;         // user-supplied reference
  }
  else if (ref_mode <= 0)
  {
    sz.Sample(gref, now);
    ref_mode = 1;         // beginning frame is reference
  }

  // subsample mask
  if (mask == NULL) 
    m = NULL;
  else if (!st.AnyNZ(*mask))
    m = NULL;
  else if (gref.SameSize(*mask, 1))  
    m = mask;
  else
    sz.SampleN(gate, *mask); 

  // call requested internal functions (originally noise first)
  sz.SampleN(ctmp, now);
  if (gskip <= 0)
    ans &= EstGains0(ctmp, gref, m);
  if ((nskip <= 0) && (GainStatus() >= 1))
    ans = EstNoise0(ctmp, gref, m, 1);

  // possibly update background model
  if (ref_mode == 1)
  {
    if ((mok <= 0) || (gains_ok() <= 0))        // something disastrous happened
      gref.CopyArr(ctmp);
    else if ((++gcnt % gsamp) == 0)
    {
      sz.Sample(ctmp, now);
      al.MixToward(gref, ctmp, gref, bmix, 1);  // blend in current image
    }
  }
  return ans;
}


//= Checks that gains are in reasonable range.
// only barfs if multiple bad frames

int jhcAGC::gains_ok ()
{
  if (GainStatus() > 0)
  {
    gbad = 0;
    return 1;
  }
  if ((GainStatus() == 0) && (++gbad > gwait))
    return 0;
  return 1;
}


//= Simpler update rule just tries to set average RGB of a patch.

void jhcAGC::ForceColor (jhcImg& src, jhcRoi& area, int r, int g, int b)
{
  double rav, gav, bav;

  st.AvgRGB(&rav, &gav, &bav, src, area);
//  char msg[80];
//Pause("averages %f %f %f in %s of %s", rav, gav, bav, area.RoiText(msg), src.SizeTxt());
  SetGainsRGB(r / rav, g / gav, b / bav);
}


///////////////////////////////////////////////////////////////////////////
//                          Gain Estimation                              //
///////////////////////////////////////////////////////////////////////////

//= Computes gains for foreground to match background.
// restricted to where mask (if provided) is zero
// assumes size checking, defaults, and subsampling already done
// returns 1 if update made, 0 if not made for some reason

int jhcAGC::EstGains0 (jhcImg& f, jhcImg& b, jhcImg *m)
{
  int amin;
  double hdrm = 1.0, awb0 = 1.0 / awb1;
  double gmax = agc1 * awb1 * hdrm, gmin = agc0 * awb0 / hdrm;
  double sc = gmax / 256.0, hlo = gmin / gmax;  // 255 is worse!!!

  // initialize internal values 
  rh.SetLims(hlo, 1.0);
  gh.SetLims(hlo, 1.0);
  bh.SetLims(hlo, 1.0);
  amin = ROUND(gfrac * f.XDim() * f.YDim());
  adj = 0;

  // find valid intensity regions 
  th.BothWithin(cmid, f, b, ilo, ihi);
  if ((m != NULL) && st.AnyNZ(*m))
    th.UnderGate(cmid, cmid, *m, 128);
  if (!st.AnyNZ(cmid))
    return 0;

  // compute ratios where valid
  al.NormBy(rats, b, f, 1.0 / gmax);
  th.OverGate(rats, rats, cmid, 128);

  // handle monochrome estimate case
  if (f.Fields() == 1)
  {
    jh.HistAll(rh, rats);
    rh.ASet(0, 0);
    if (rh.SumAll() < amin)
      return 0;
    rh.Smooth(rh, 4);
    SetGainMono(sc * rh.MaxBin()); 
    adj = 1;
    return 1;
  }

  // compute RGB channel estimates
  jh.HistRGB(rh, gh, bh, rats);
  bh.ASet(0, 0);
  gh.ASet(0, 0);
  rh.ASet(0, 0);
  if ((rh.SumAll() < amin) || (gh.SumAll() < amin) || (bh.SumAll() < amin))
    return 0;
  bh.Smooth(bh, 4);
  gh.Smooth(gh, 4);
  rh.Smooth(rh, 4);
  SetGainsRGB(sc * rh.MaxBin(), sc * gh.MaxBin(), sc * bh.MaxBin());
  adj = 1;
  return 1;
}


//= Mixes RGB gain estimates with old values  and applies limits.
// negative estimates leave old value unchanged
// sets iclip and cclip nonzero if raw intensity or hue are out of range

void jhcAGC::SetGainsRGB (double estr, double estg, double estb)
{
  int i;
  double avg, awb0 = 1.0 / awb1;

  // mix new and old values
  for (i = 1; i <= 3; i++)
    g[i] *= g[0];
  if (estb > 0.0)
    g[1] = g[1] + gmix * (estb - g[1]);
  if (estg > 0.0)
    g[2] = g[2] + gmix * (estg - g[2]);
  if (estr > 0.0)
    g[3] = g[3] + gmix * (estr - g[3]);

  // separate into AGC and AWB then apply limits
  avg = (g[1] + g[2] + g[3]) / 3.0;
  ClipGain(avg);
  cclip = 3;
  for (i = 1; i <= 3; i++)
  {
    g[i] /= avg;
    if (g[i] >= awb1)
      g[i] = awb1;
    else if (g[i] <= awb0)
      g[i] = awb0;
    else
      cclip--;
  }
}


//= Mixes gain estimate with old value and applies limits.
// sets iclip nonzero if raw estimate is out of range

void jhcAGC::SetGainMono (double gest)
{
  ClipGain(g[0] + gmix * (gest - g[0]));
}


//= Limits intensity gain to valid range and updates iclip value.

void jhcAGC::ClipGain (double val)
{
  g[0] = val;
  iclip = 1;
  if (g[0] >= agc1)
    g[0] = agc1;
  else if (g[0] <= agc0)
    g[0] = agc0;
  else
    iclip = 0;
}


///////////////////////////////////////////////////////////////////////////
//                         Noise Estimation                              //
///////////////////////////////////////////////////////////////////////////

//= Histograms differences between "now" and "ref" where mask.
// restricted to where mask (if provided) is zero
// assumes size checking, defaults, and subsampling already done
// returns 1 if update made, 0 if not made for some reason

int jhcAGC::EstNoise0 (jhcImg& f, jhcImg &b, jhcImg *m, int fix)
{
  double hdrm = 1.2;

  // see if enough data to analyze
  if ((m != NULL) && (st.FracUnder(*m, 128) < nfrac))
    return 0;
  if (m != NULL)
    sz.SampleN(tmp, *m);

  // maximize histogram analysis regions
  rh.SetLimits(0, ROUND(hdrm * rn.vmax));
  gh.SetLimits(0, ROUND(hdrm * gn.vmax));
  bh.SetLimits(0, ROUND(hdrm * bn.vmax));

  // find pixel-wise differences in all channels
  if (fix > 0)
  {
    sz.SampleN(ctmp, f);
    FixAGC(rats, ctmp);                    // redundant if stored?
    sz.SampleN(ctmp, b);
    LimitAGC(cmid, ctmp);                  // redundant if stored?
  }
  else
  {
    sz.SampleN(rats, f);
    sz.SampleN(cmid, b);
  }
  al.AbsDiff(rats, rats, cmid);

  // handle monochrome case
  if (f.Fields() == 1)
  {
    if (m != NULL)
      jh.HistUnder(rh, rats, tmp, 128);
    else
      jh.HistAll(rh, rats);
    rh.Smooth(rh, nsm);
    rn.Update(rh.PeakFall(rh.MaxBin(), ndrop));
    return 1;
  }

  // histogram outside of foreground
  if (m != NULL)
    jh.HistUnderRGB(rh, gh, bh, rats, tmp, 128);
  else
    jh.HistRGB(rh, gh, bh, rats);

  // use upper edge of peak as new estimate
  rh.Smooth(rh, nsm);
  gh.Smooth(gh, nsm);
  bh.Smooth(bh, nsm);
  rn.Update(rh.PeakFall(rh.MaxBin(), ndrop));
  gn.Update(gh.PeakFall(gh.MaxBin(), ndrop));
  bn.Update(bh.PeakFall(bh.MaxBin(), ndrop));
  return 1;
}

