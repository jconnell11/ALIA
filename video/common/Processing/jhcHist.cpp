// jhcHist.cpp : generate various kinds of histograms from images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
// Copyright 2022 Etaoin Systems
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

#include "Processing/jhcHist.h"


///////////////////////////////////////////////////////////////////////////
//                            Basic Histograms                           //
///////////////////////////////////////////////////////////////////////////

//= Build histogram of all pixels in ROI.

int jhcHist::HistAll (jhcArr& h, const jhcImg& src, int vmin, int vmax, int squash)  
{
  if (!h.Valid() || !src.Valid())
    return Fatal("Bad inputs to jhcHist::HistAll");

  // general ROI case
  int i, x, y, f, nf = src.Fields(), rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();
  int *hist = h.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(h.Size(), vmin, vmax, squash);

  // use pixel value to figure out which bin in the histogram to increment
  h.Fill(0);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      for (f = nf; f > 0; f--, s++)
        if ((i = bin[*s]) >= 0)
          hist[i]++;
    s += rsk;
  }
  return 1;
}


//= Restricts histogram to some subregion of image.
// area specified by fractional widths and heights

int jhcHist::HistRegion (jhcArr& h, const jhcImg& src, 
                         double xc, double yc, double wid, double ht) 
{
  jhcRoi area;

  area.CenterRoi(xc, yc, wid, ht);
  return HistRegion(h, src, area);
}


//= Like other HistRegion but takes a Roi object instead of numbers.

int jhcHist::HistRegion (jhcArr& h, const jhcImg& src, const jhcRoi& area) 
{
  if (!h.Valid() || !src.Valid())
    return Fatal("Bad inputs to jhcHist::HistRegion");
  h.Fill(0);
  if (area.RoiArea() <= 0)
    return 1;

  // general ROI case
  int i, x, y, f, nf = src.Fields(), rw = area.RoiW(), rh = area.RoiH(), rsk = src.RoiSkip(area);
  const UC8 *s = src.RoiSrc(area);
  int *hist = h.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(h.Size(), 0, 255, 0);

  // use pixel value to figure out which bin in the histogram to increment
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      for (f = nf; f > 0; f--, s++)
        if ((i = bin[*s]) >= 0)
          hist[i]++;
    s += rsk;
  }
  return 1;
}


//= Figure out a map to take ALL pixel values to some bin.
// a bin value of -1 means out of bounds (for non-squashed)

void jhcHist::compute_bins (int n, int vmin, int vmax, int squash)
{
  int i, v, lim = n - 1;
  double rng = (double)(vmax - vmin + 1), sc = 1.0;

  if (rng != 0.0)
    sc = n / rng;
  for (v = 0; v <= 255; v++)
  {
    i = (int)(sc * (v - vmin));
    if (squash != 0)
      i = __max(0, __min(i, lim));
    else if ((v < vmin) || (vmax < v))
      i = -1;
    bin[v] = i;
  }
}


//= Histograms straight 8 bit values (no remapping) from some region of the image.
// bins are always pixel values so the histogram should have at least 256 bins

int jhcHist::HistRegion8 (jhcArr& h, const jhcImg& src, const jhcRoi& area, int clr) 
{
  if (!h.Valid() || !src.Valid(1, 3))
    return Fatal("Bad inputs to jhcHist::HistRegion8");
  if (clr > 0)
    h.Fill(0);
  if (area.RoiArea() <= 0)
    return 1;

  // general ROI case
  int rc = area.RoiW() * src.Fields(), rh = area.RoiH(), rsk = src.RoiSkip(area);
  int x, y, n = h.Size();
  const UC8 *s = src.RoiSrc(area);
  int *hist = h.Data();

  // use pixel value to figure out which bin in the histogram to increment
  for (y = rh; y > 0; y--, s += rsk)
    for (x = rc; x > 0; x--, s++)
      if (*s < n)
        hist[*s] += 1;
  return 1;
}


//= Histogram 16 bit values in whole image.
// bins are always pixel values so a 12 bit image needs a 4096 element histogram

int jhcHist::HistAll16 (jhcArr& h, const jhcImg& src) 
{
  return HistRegion16(h, src, src);
}


//= Histograms 16 bit values in region given by SW corner and size specs.
// bins are always pixel values so a 12 bit image needs a 4096 element histogram

int jhcHist::HistRegion16 (jhcArr& h, const jhcImg& src, int rx, int ry, int rw, int rh)
{
  jhcRoi area;

  area.SetRoi(rx, ry, rw, rh);
  return HistRegion16(h, src, area);
}


//= Histograms straight 16 bit values (no remapping) from some region of the image.
// bins are always pixel values so a 12 bit image needs a 4096 element histogram

int jhcHist::HistRegion16 (jhcArr& h, const jhcImg& src, const jhcRoi& area) 
{
  if (!h.Valid() || !src.Valid(2))
    return Fatal("Bad inputs to jhcHist::HistRegion16");
  h.Fill(0);
  if (area.RoiArea() <= 0)
    return 1;

  // general ROI case
  int x, y, n = h.Size(), rw = area.RoiW(), rh = area.RoiH(), rsk = src.RoiSkip(area) >> 1;
  const US16 *s = (US16 *) src.RoiSrc(area);
  int *hist = h.Data();

  // use pixel value to figure out which bin in the histogram to increment
  for (y = rh; y > 0; y--, s += rsk)
    for (x = rw; x > 0; x--, s++)
      if (*s < n)
        hist[*s] += 1;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                      Weighted Partial Histograms                      //
///////////////////////////////////////////////////////////////////////////

//= Build histogram only for pixels which are non-zero in mask.

int jhcHist::HistNZ (jhcArr& h, const jhcImg& src, const jhcImg& mask,
                     int vmin, int vmax, int squash) 
{
  if (!h.Valid() || !src.Valid(1) || !src.SameFormat(mask))
    return Fatal("Bad inputs to jhcHist::HistNZ");

  // general ROI case
  int i, x, y, rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  UL32 roff = src.RoiOff();
  const UC8 *s = src.PxlSrc() + roff, *g = mask.PxlSrc() + roff;
  int *hist = h.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(h.Size(), vmin, vmax, squash);

  // use pixel value to figure out which bin in the histogram to increment
  h.Fill(0);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*g++ == 0)
        s++;
      else if ((i = bin[*s++]) >= 0)
        hist[i]++;
    s += rsk;
    g += rsk;
  }
  return 1;
}


//= Build histogram only for pixels which are above threshold in mask.

int jhcHist::HistOver (jhcArr& h, const jhcImg& src, const jhcImg& gate, int th, 
                       int vmin, int vmax, int squash, int bump) 
{
  jhcRoi r;
  UC8 thv = BOUND(th);

  if (!h.Valid() || !src.Valid(1) || !src.SameFormat(gate))
    return Fatal("Bad inputs to jhcHist::HistOver");
  if (thv == 0)
    return HistNZ(h, src, gate);
  r.CopyRoi(gate);
  r.MergeRoi(src);

  // general ROI case
  int i, x, y, rw = r.RoiW(), rh = r.RoiH(), rsk = src.RoiSkip(r);
  const UC8 *s = src.RoiSrc(r), *g = gate.RoiSrc(r);
  int *hist = h.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(h.Size(), vmin, vmax, squash);

  // use pixel value to figure out which bin in the histogram to increment
  h.Fill(0);
  for (y = rh; y > 0; y--, s += rsk, g += rsk)
    for (x = rw; x > 0; x--, s++, g++)
      if (*g > thv)
        if ((i = bin[*s]) >= 0)
          hist[i] += bump;
  return 1;
}


//= Build 8 bit histogram only for pixels which are above threshold in mask.
// bins are always pixel values so the histogram should have at least 256 bins

int jhcHist::HistOver8 (jhcArr& h, const jhcImg& src, const jhcImg& gate, int th, int bump)
{
  jhcRoi r;
  UC8 thv = BOUND(th);

  if (!h.Valid() || !src.Valid(1) || !src.SameFormat(gate))
    return Fatal("Bad inputs to jhcHist::HistOver8");
  r.CopyRoi(gate);
  r.MergeRoi(src);

  // general ROI case
  int x, y, n = h.Size(), rw = r.RoiW(), rh = r.RoiH(), rsk = src.RoiSkip(r);
  const UC8 *s = src.RoiSrc(r), *g = gate.RoiSrc(r);
  int *hist = h.Data();

  // use pixel value to figure out which bin in the histogram to increment
  h.Fill(0);
  for (y = rh; y > 0; y--, s += rsk, g += rsk)
    for (x = rw; x > 0; x--, s++, g++)
      if ((*g > thv) && (*s < n))
        hist[*s] += bump;
  return 1;
}


//= Build histogram only for pixels which are under threshold in mask.

int jhcHist::HistUnder (jhcArr& h, const jhcImg& src, const jhcImg& gate, int th, 
                        int vmin, int vmax, int squash) 
{
  UC8 thv = BOUND(th);

  if (!h.Valid() || !src.Valid(1) || !src.SameFormat(gate))
    return Fatal("Bad inputs to jhcHist::HistUnder");

  // general ROI case
  int i, x, y, rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  UL32 roff = src.RoiOff();
  const UC8 *s = src.PxlSrc() + roff, *g = gate.PxlSrc() + roff;
  int *hist = h.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(h.Size(), vmin, vmax, squash);

  // use pixel value to figure out which bin in the histogram to increment
  h.Fill(0);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if (*g++ >= thv)
        s++;
      else if ((i = bin[*s++]) >= 0)
        hist[i]++;
    s += rsk;
    g += rsk;
  }
  return 1;
}


//= Build histogram with pixels weighted by factors in "wts" image.

int jhcHist::HistWts (jhcArr& h, const jhcImg& src, const jhcImg& wts,
                      int vmin, int vmax, int squash) 
{
  if (!h.Valid() || !src.Valid(1) || !src.SameFormat(wts))
    return Fatal("Bad inputs to jhcHist::HistWts");

  // general ROI case
  int i, x, y, rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  UL32 roff = src.RoiOff();
  const UC8 *s = src.PxlSrc() + roff, *w = wts.PxlSrc() + roff;
  int *hist = h.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(h.Size(), vmin, vmax, squash);

  // use pixel value to figure out which bin in the histogram to increment
  h.Fill(0);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
      if ((i = bin[*s++]) >= 0)
        hist[i] += *w++;
    s += rsk;
    w += rsk;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                      Color Histograms & Averages                      //
///////////////////////////////////////////////////////////////////////////

//= Like HistAll but generates separate histogram for each color channel.

int jhcHist::HistRGB (jhcArr& red, jhcArr& grn, jhcArr& blu, 
                      const jhcImg& src, int vmin, int vmax, int squash) 
{
  if (!red.Valid() || !red.SameSize(grn) || !red.SameSize(blu) || !src.Valid(3))
    return Fatal("Bad inputs to jhcHist::HistRGB");

  // general ROI case
  int i, x, y, rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();
  int *rhist = red.Data(), *ghist = grn.Data(), *bhist = blu.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(red.Size(), vmin, vmax, squash);

  // use pixel value to figure out which bin in the histogram to increment
  red.Fill(0);
  grn.Fill(0);
  blu.Fill(0);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if ((i = bin[s[0]]) >= 0)
        bhist[i]++;
      if ((i = bin[s[1]]) >= 0)
        ghist[i]++;
      if ((i = bin[s[2]]) >= 0)
        rhist[i]++;
      s += 3;
    }
    s += rsk;
  }
  return 1;
}


//= Like HistRGB but restricts analysis to a particular region.

int jhcHist::HistRGB (jhcArr& red, jhcArr& grn, jhcArr& blu, const jhcImg& src, const jhcRoi& area) 
{
  if (!red.Valid() || !red.SameSize(grn) || !red.SameSize(blu) || !src.Valid(3))
    return Fatal("Bad inputs to jhcHist::HistRGB");

  // general ROI case
  int i, x, y, rw = area.RoiW(), rh = area.RoiH(), rsk = src.RoiSkip(area);
  const UC8 *s = src.RoiSrc(area);
  int *rhist = red.Data(), *ghist = grn.Data(), *bhist = blu.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(red.Size(), 0, 255, 0);

  // use pixel value to figure out which bin in the histogram to increment
  red.Fill(0);
  grn.Fill(0);
  blu.Fill(0);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if ((i = bin[s[0]]) >= 0)
        bhist[i]++;
      if ((i = bin[s[1]]) >= 0)
        ghist[i]++;
      if ((i = bin[s[2]]) >= 0)
        rhist[i]++;
      s += 3;
    }
    s += rsk;
  }
  return 1;
}


//= Like HistRGB but builds only operates where gate image is valid.

int jhcHist::HistUnderRGB (jhcArr& red, jhcArr& grn, jhcArr& blu, 
                           const jhcImg& src, const jhcImg& gate, int th, 
                           int vmin, int vmax, int squash) 
{
  if (!red.Valid() || !red.SameSize(grn) || !red.SameSize(blu) ||
      !src.Valid(3) || !src.SameSize(gate, 1))
    return Fatal("Bad inputs to jhcHist::HistUnderRGB");

  // general ROI case
  int i, x, y, rw = gate.RoiW(), rh = src.RoiH();
  int gsk = gate.RoiSkip(), ssk = src.RoiSkip(gate);
  const UC8 *g = gate.RoiSrc(), *s = src.RoiSrc(gate);
  int *rhist = red.Data(), *ghist = grn.Data(), *bhist = blu.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(red.Size(), vmin, vmax, squash);

  // use pixel value to figure out which bin in the histogram to increment
  red.Fill(0);
  grn.Fill(0);
  blu.Fill(0);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      if (*g++ < th)
      {
        if ((i = bin[s[0]]) >= 0)
          bhist[i]++;
        if ((i = bin[s[1]]) >= 0)
          ghist[i]++;
        if ((i = bin[s[2]]) >= 0)
          rhist[i]++;
      }
      s += 3;
    }
    g += gsk;
    s += ssk;
  }
  return 1;
}
                    

//= Take averages of values in first image using bin values from second image.
// useful for determining noise models like dI for different values of I

int jhcHist::HistAvgs (jhcArr& avgs, const jhcImg& vals, const jhcImg& bins)  
{
  if ((avgs.Size() < 256) || !vals.Valid(1) || !vals.SameFormat(bins))
    return Fatal("Bad inputs to jhcHist::HistAvgs");
  
  // general ROI case
  jhcArr tmp(avgs);
  int i, x, y, rw = vals.RoiW(), rh = vals.RoiH(), rsk = vals.RoiSkip();
  UL32 roff = vals.RoiOff();
  const UC8 *v = vals.PxlSrc() + roff, *b = bins.PxlSrc() + roff;
  int *sums = avgs.Data(), *cnts = tmp.Data();

  // compute which bin each possible pixel value corresponds to
  compute_bins(avgs.Size(), 0, 255, 0);

  // use "bins" pixel value to figure out which bin in the histogram to use
  // increment selected bin in "sums" by pixel value found in "vals" image
  tmp.Fill(0);
  avgs.Fill(0);
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      i = bin[*b++];
      sums[i] += *v++;
      cnts[i] += 1;
    }
    v += rsk;
    b += rsk;
  }

  // normalize sums by counts to get averages
  for (i = 0; i < avgs.Size(); i++)
    if (cnts[i] > 0)
      sums[i] = (int)(sums[i] / (double) cnts[i] + 0.5); 
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Image Improvement                            //
///////////////////////////////////////////////////////////////////////////

//= Stretch contrast of image for better viewing.
// can optionally set some rectangular subarea for contrast calculation
// parameters for last enhancement available in member variables "sc" and "off"

int jhcHist::Enhance (jhcImg& dest, const jhcImg& src, double smax, const jhcRoi *area, int omax)  
{
  if (!src.Valid() || !src.SameFormat(dest))
    return Fatal("Bad inputs to jhcHist::Enhance");

  // local variables
  jhcArr ihist(256);
  UC8 scaled[256];
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // find correction for relevant part of intensity distribution
  if (area != NULL)
    HistRegion(ihist, src, *area);
  else
    HistAll(ihist, src);
  off = linear_fix(sc, scaled, ihist, smax, omax);

  // apply stretching to image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rw; x > 0; x--, d++, s++)
      *d = scaled[*s];
  return 1;
}


//= Stretch contrast of image using data from arbitrary shaped mask.
// parameters for last enhancement available in member variables "sc" and "off"

int jhcHist::Enhance (jhcImg& dest, const jhcImg& src, const jhcImg& mask, double smax, int omax)  
{
  if (!src.Valid() || !src.SameFormat(dest) || !src.SameFormat(mask))
    return Fatal("Bad inputs to jhcHist::Enhance");

  // local variables
  jhcArr ihist(256);
  UC8 scaled[256];
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // find correction for relevant part of intensity distribution
  HistOver(ihist, src, mask);
  off = linear_fix(sc, scaled, ihist, smax, omax);

  // apply stretching to image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rw; x > 0; x--, d++, s++)
      *d = scaled[*s];
  return 1;
}


//= Stretch contrast of each color channel for better definition and color cast removal.
// can optionally set some subarea for contrast calculation
// parameters for last enhancement "sc", "gsc", "bsc" and "off", "goff", "boff"

int jhcHist::Enhance3 (jhcImg& dest, const jhcImg& src, double smax, const jhcRoi *area, int omax)  
{
  if (!src.Valid() || !src.SameFormat(dest))
    return Fatal("Bad inputs to jhcHist::Enhance3");

  // local variables
  jhcArr rhist(256), ghist(256), bhist(256);
  UC8 rlut[256], glut[256], blut[256];
  int x, y, rw = dest.RoiW(), rh = dest.RoiH(), rsk = dest.RoiSkip();
  UC8 *d = dest.RoiDest();
  const UC8 *s = src.RoiSrc();

  // find original intensity distribution parameters
  if (area != NULL)
    HistRGB(rhist, ghist, bhist, src, *area);
  else
    HistRGB(rhist, ghist, bhist, src);

  // get channel correction tables
  off  = linear_fix( sc, rlut, rhist, smax, omax);
  goff = linear_fix(gsc, glut, ghist, smax, omax);
  boff = linear_fix(bsc, blut, bhist, smax, omax);

  // apply stretching to image
  for (y = rh; y > 0; y--, d += rsk, s += rsk)
    for (x = rw; x > 0; x--, d += 3, s += 3)
    {
      d[0] = blut[s[0]];
      d[1] = glut[s[1]];
      d[2] = rlut[s[2]];
    }
  return 1;
}


//= Compute linear offset and scaling for some intensity histogram.
// fills in lookup table for converting old values to new values
// returns offset used and scaling factor

int jhcHist::linear_fix (double& isc, UC8 scaled[], jhcArr& hist, double smax, int omax) const
{
  double sc2, lpct = 0.05, hpct = 0.95;
  int bot, top, ioff, f, sum, i, val, ilo = 20, ihi = 240, hsm = 4; 

  // get red channel histogram cut points
  hist.ASet(0, 0);
  hist.ASet(255, 0);
  hist.Smooth(hsm);
  bot = hist.Percentile(lpct);
  top = hist.Percentile(hpct);

  // compute stretching linear correction
  // if too much stretching, opt for darker image
  isc = (ihi - ilo) / (double)(top - bot);
  sc2 = (255 - ilo) / (double)(255 - bot);   // retain white
  isc = __max(isc, sc2);
  isc = __min(isc, smax);
  ioff = ROUND(ilo / isc - bot);
  if (omax > 0)
    ioff = __max(ioff, -omax);

  // compute answers for all possible values
  f = ROUND(256.0 * isc);
  sum = ROUND(f * ioff + 128);
  for (i = 0; i <= 255; i++)
  {
    val = (int)(sum >> 8);
    scaled[i] = BOUND(val);
    sum += f;
  }
  return ioff;
}


//= Condenses a 16 bit image to an 8 bit one using contrast stretching.
// needs to know maximum 16 bit valid value, can limit max stretching
// can optionally set some subarea for contrast calculation
// parameters for last enhancement available in member variables "sc" and "off"

int jhcHist::Enhance16 (jhcImg& dest, const jhcImg& src, int pmax, double smax, const jhcRoi *area, int omax)  
{
  if (!src.Valid(2) || !src.SameSize(dest, 1) || (pmax <= 0) || (pmax > 65536))
    return Fatal("Bad inputs to jhcHist::Enhance16");

  // local variables
  double lpct = 0.05, hpct = 0.95;
  int x, y, i, val, f, bot, top, ilo = 20, ihi = 240, hsm = 4; 
  int sum, rw = dest.RoiW(), rh = dest.RoiH(), dsk = dest.RoiSkip(), ssk = src.RoiSkip() >> 1;
  UC8 *d = dest.RoiDest();
  const US16 *s = (US16 *) src.RoiSrc();
  jhcArr ihist(pmax + 1);
  UC8 *scaled = new UC8[pmax + 1];

  // find original intensity distribution parameters
  if (area != NULL)
    HistRegion16(ihist, src, *area);
  else
    HistAll16(ihist, src);
  ihist.ASet(0, 0);
  ihist.ASet(pmax, 0);
  ihist.Smooth(hsm);
  bot = ihist.Percentile(lpct);
  top = ihist.Percentile(hpct);

  // compute stretching linear correction
  // if too much stretching, opt for darker image
  sc = (ihi - ilo) / (double)(top - bot);
  sc = __min(sc, smax);
  off = ROUND(ilo / sc - bot);
  if (omax > 0)
    off = __max(off, -omax);

  // compute answers for all possible values
  f = ROUND(256.0 * sc);
  sum = ROUND(f * off + 128);
  for (i = 0; i <= pmax; i++)
  {
    val = (int)(sum >> 8);
    scaled[i] = BOUND(val);
    sum += f;
  }

  // apply stretching to image
  for (y = rh; y > 0; y--, d += dsk, s += ssk)
    for (x = rw; x > 0; x--, d++, s++)
      *d = scaled[__min(*s, pmax)];

  // clean up
  delete [] scaled;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Directional Projections                        //
///////////////////////////////////////////////////////////////////////////

//= Vertical projection in ROI, histogram size must match image height.
// normalizes projection bins to be average gray value times scale
// if scale factor is negative, uses faster integer multiply scaling

int jhcHist::ProjectV (jhcArr& hist, const jhcImg& src, double sc) const 
{
  int x, y, i, ry = src.RoiY();
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();

  if (!src.Valid(1) || (hist.Size() != src.YDim()))
    return Fatal("Bad inputs to jhcHist::ProjectV");

  hist.Fill(0);
  for (y = 0; y < rh; y++, s += rsk)
  {
    i = ry + y;
    for (x = 0; x < rw; x++, s++)
      hist.AInc(i, *s);
  }
  if (sc == rw)
    return 1;
  if (sc < 0.0)
    return hist.ScaleFast(hist, -sc / rw);
  return hist.Scale(hist, sc / rw);
}


//= Like other ProjectV but limited to a smaller area.

int jhcHist::ProjectV (jhcArr& hist, const jhcImg& src, const jhcRoi& area, double sc) const 
{
  int x, y, i, ry = area.RoiY(), sz = hist.Size();
  int rw = area.RoiW(), rh = area.RoiH(), rsk = src.RoiSkip(area);
  const UC8 *s = src.RoiSrc(area);

  if (!src.Valid(1) || (hist.Size() != src.YDim()))
    return Fatal("Bad inputs to jhcHist::ProjectV");

  hist.Fill(0);
  for (y = 0; y < rh; y++, s += rsk)
  {
    i = ry + y;
    if ((i >= 0) && (i < sz))
      for (x = 0; x < rw; x++, s++)
        hist.AInc(i, *s);
    else
      s += rw;
  }
  if (sc == rw)
    return 1;
  if (sc < 0.0)
    return hist.ScaleFast(hist, -sc / rw);
  return hist.Scale(hist, sc / rw);
}


//= Horizontal projection in ROI, histogram size must match image width.
// normalizes projection bins to be average gray value times scale
// if scale factor is negative, uses faster integer multiply scaling

int jhcHist::ProjectH (jhcArr& hist, const jhcImg& src, double sc) const 
{
  int x, y, rx = src.RoiX();
  int rw = src.RoiW(), rh = src.RoiH(), rsk = src.RoiSkip();
  const UC8 *s = src.RoiSrc();

  if (!src.Valid(1) || (hist.Size() != src.XDim()))
    return Fatal("Bad inputs to jhcHist::ProjectH");

  hist.Fill(0);
  for (y = 0; y < rh; y++, s += rsk)
    for (x = 0; x < rw; x++, s++)
      hist.AInc(rx + x, *s);

  if (sc == rh)
    return 1;
  if (sc < 0.0)
    return hist.ScaleFast(hist, -sc / rh);
  return hist.Scale(hist, sc / rh);
}


//= Like other ProjectH but limited to a smaller area.

int jhcHist::ProjectH (jhcArr& hist, const jhcImg& src, const jhcRoi& area, double sc) const 
{
  int x, y, rx = area.RoiX();
  int rw = area.RoiW(), rh = area.RoiH(), rsk = src.RoiSkip(area);
  const UC8 *s = src.RoiSrc(area);

  if (!src.Valid(1) || (hist.Size() != src.XDim()))
    return Fatal("Bad inputs to jhcHist::ProjectH");

  hist.Fill(0);
  for (y = 0; y < rh; y++, s += rsk)
    for (x = 0; x < rw; x++, s++)
      hist.AIncChk(rx + x, *s);

  if (sc == rh)
    return 1;
  if (sc < 0.0)
    return hist.ScaleFast(hist, -sc / rh);
  return hist.Scale(hist, sc / rh);
}


//= Take a one pixel wide vertical slice through image at some offset.
// slice is taken from bottom up (as displayed on screen)

int jhcHist::SliceV (jhcArr& hist, const jhcImg& src, int x) const 
{
  int y, h = src.YDim(), ln = src.Line();
  const UC8 *s;

  if (!src.Valid(1) || (hist.Size() != h) || (x < 0) || (x >= src.XDim()))
    return Fatal("Bad inputs to jhcHist::SliceV");

  s = src.RoiSrc(x, 0);
  for (y = 0; y < h; y++, s += ln)
    hist.ASet(y, *s);
  return 1;
}


//= Take a one pixel wide horizontal slice through image at some offset.
// slice is taken from left to right

int jhcHist::SliceH (jhcArr& hist, const jhcImg& src, int y) const 
{
  int x, w = src.XDim(), h = src.YDim();
  const UC8 *s;

  if (!src.Valid(1) || (hist.Size() != w) || (y < 0) || (y >= h))
    return Fatal("Bad inputs to jhcHist::SliceH");

  s = src.RoiSrc(0, y);
  for (x = 0; x < w; x++, s++)
    hist.ASet(x, *s);
  return 1;
}


//= Vertical projection of minimum in ROI, histogram size must match image height.
// returns the darkest pixel on each row of image

int jhcHist::ProjMinV (jhcArr& hist, const jhcImg& src, const jhcRoi& area)
{
  int x, y, i, lo, ry = area.RoiY(), sz = hist.Size();
  int rw = area.RoiW(), rh = area.RoiH(), rsk = src.RoiSkip(area);
  const UC8 *s = src.RoiSrc(area);

  if (!src.Valid(1) || (hist.Size() != src.YDim()))
    return Fatal("Bad inputs to jhcHist::ProjMinV");

  hist.Fill(0);
  for (y = 0; y < rh; y++, s += rsk)
  {
    i = ry + y;
    if ((i >= 0) && (i < sz))
    {
      lo = 255;
      for (x = 0; x < rw; x++, s++)
        lo = __min(lo, *s);
      hist.ASet(i, lo);
    }
    else
      s += rw;
  }
  return 1;
}


//= Vertical projection of percentage in ROI, histogram size must match image height.
// for example frac = 0.1 gives value on each row that 10% of pixels are below 

int jhcHist::ProjPctV (jhcArr& hist, const jhcImg& src, const jhcRoi& area, double frac)
{
  jhcArr vals(256);
  int x, y, i, ry = area.RoiY(), sz = hist.Size();
  int rw = area.RoiW(), rh = area.RoiH(), rsk = src.RoiSkip(area);
  const UC8 *s = src.RoiSrc(area);

  if (!src.Valid(1) || (hist.Size() != src.YDim()))
    return Fatal("Bad inputs to jhcHist::ProjPctV");

  hist.Fill(0);
  for (y = 0; y < rh; y++, s += rsk)
  {
    i = ry + y;
    if ((i >= 0) && (i < sz))
    {
      vals.Fill(0);
      for (x = 0; x < rw; x++, s++)
        vals.AInc(*s, 1);
      hist.ASet(i, vals.Percentile(frac));
    }
    else
      s += rw;
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Scatter Plots                              //
///////////////////////////////////////////////////////////////////////////

//= Builds a 2D scatterplot of pixel values in xval and yval images.
// image scaled so 255 in x or y is heigth of output image, peak is 255 
// can adjust peak value and max radius using two scaling factors

int jhcHist::Hist2D (jhcImg& dest, const jhcImg& xval, const jhcImg& yval, double psc, double rsc) 
{
  if (!dest.Valid(1) || !xval.Valid(1) || !xval.SameFormat(yval))
    return Fatal("Bad inputs to jhcHist::Hist2D");
  temp.SetSize(dest, 4);

  // local variables
  int vsc, v, i, j, x, y, w = xval.XDim(), h = xval.YDim(), sk = xval.Skip();
  int dw = dest.XDim(), dh = dest.YDim(), dsk = dest.Skip(); 
  int sc = ROUND(65536.0 * rsc * dh / 255.0);
  int xoff = ROUND(0.5 * dw - 0.5 * rsc * dh), yoff = ROUND(0.5 * dh  - 0.5 * rsc * dh);
  const UC8 *xv = xval.PxlSrc(), *yv = yval.PxlSrc();
  UC8 *d = dest.PxlDest();
  UL32 *t = (UL32 *) temp.PxlDest();
  UL32 top = 0;

  if (!dest.Valid(1) || !xval.Valid(1) || !xval.SameFormat(yval))
    return Fatal("Bad inputs to jhcHist::Hist2D");

  // accumulate basic counts in temporary array
  temp.FillArr(0);
  for (y = h; y > 0; y--, xv += sk, yv += sk)
    for (x = w; x > 0; x--, xv++, yv++)
    {
      i = xoff + (((*xv) * sc) >> 16);
      if ((i < 0) || (i >= dw))
        continue;
      j = yoff + (((*yv) * sc) >> 16);
      if ((j < 0) || (j >= dh))
        continue;
      t[j * dw + i] += 1;
    }

  // determine how to rescale counts then copy to output image
  j = temp.PxlCnt();
  for (i = 0; i < j; i++)
    if (t[i] > top)
      top = t[i];
  vsc = ROUND(65536.0 * 255.0 * psc / top);
  for (y = dh; y > 0; y--, d += dsk)
    for (x = dw; x > 0; x--, t++, d++)
    {
      v = ((*t) * vsc) >> 16;
      *d = BOUND(v); 
    }
  return 1;
}

