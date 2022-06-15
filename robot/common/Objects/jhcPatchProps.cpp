// jhcPatchProps.cpp : extracts semantic properties for image regions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#include <string.h>
#include <math.h>

#include "Interface/jhcMessage.h"

#include "jhcPatchProps.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcPatchProps::~jhcPatchProps ()
{
}


//= Default constructor initializes certain values.

jhcPatchProps::jhcPatchProps ()
{
  // shared list of color names
  strcpy_s(cname[0], "red");
  strcpy_s(cname[1], "orange");
  strcpy_s(cname[2], "yellow");
  strcpy_s(cname[3], "green");
  strcpy_s(cname[4], "blue");
  strcpy_s(cname[5], "purple");
  strcpy_s(cname[6], "black");
  strcpy_s(cname[7], "gray");
  strcpy_s(cname[8], "white");

  // hue histogram
  hhist.SetSize(256);

  // processing parameters
  SetFind(3, 180, 35, 25, 245, 100, 50);
  SetHue(250, 30, 49, 130, 170, 220);
  SetPrimary(0.2, 2.0, 0.3, 0.05);

  // processing parameters
  Defaults();
  Reset();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for extracting color properties.

int jhcPatchProps::cfind_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("prop_cfind", 0);
  ps->NextSpec4( &csm,   "Mask shrinkage (pel)");       // was 3 then 9
  ps->NextSpec4( &cth,   "Shrink shape threshold");     // was 50 then 180
  ps->Skip(); 
  ps->NextSpec4( &smin,  "Min saturation for color");   // was 30, 50, then 35
  ps->NextSpec4( &imin,  "Min intensity for color");    // was 50, 80, then 120
  ps->NextSpec4( &imax,  "Max intensity for color");    // was 240

  ps->NextSpec4( &white, "White intensity threshold");  // was 180 then 150
  ps->NextSpec4( &dark,  "Black intensity threshold");  // was 90 then 100
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set color extraction parameters in same order as in configuration file line.

void jhcPatchProps::SetFind (int sm, int th, int s0, int i0, int i1, int wh, int bk)
{
  csm   = sm;
  cth   = th;
  smin  = s0;
  imin  = i0;
  imax  = i1;
  white = wh;
  dark  = bk;
}


//= Parameters used for qualitative color naming based on hue.

int jhcPatchProps::hue_params (const char *fname)
{
  jhcParam *ps = &hps;
  int ok;

  ps->SetTag("prop_hue", 0);
  ps->NextSpec4( clim,     "Red-orange boundary");     // was 13, 7, 2, 25, 10, 12, then 18
  ps->NextSpec4( clim + 1, "Orange-yellow boundary");  // was 26, 33, then 30
  ps->NextSpec4( clim + 2, "Yellow-green boundary");   // was 45, 47, then 55, then 50
  ps->NextSpec4( clim + 3, "Green-blue boundary");  
  ps->NextSpec4( clim + 4, "Blue-purple boundary");    // was 170 then 180
  ps->NextSpec4( clim + 5, "Purple-red boundary");     // was 234 then 240
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set hue boundaries for naming in same order as in configuration file line.

void jhcPatchProps::SetHue (int ro, int oy, int yg, int gb, int bp, int pr)
{
  clim[0] = ro;
  clim[1] = oy;
  clim[2] = yg;
  clim[3] = gb;
  clim[4] = bp;
  clim[5] = pr;
}


//= Parameters used for determining primary/secondary colors.

int jhcPatchProps::cname_params (const char *fname)
{
  jhcParam *ps = &nps;
  int ok;

  ps->SetTag("prop_cname", 0);
  ps->NextSpecF( &cprime, "Min primary fraction");  
  ps->NextSpecF( &cdom,   "Primary dominance");  
  ps->NextSpecF( &csec,   "Secondary wrt max");  
  ps->NextSpecF( &cmin,   "Min fraction for any");  
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Set color name parameters in same order as in configuration file line.

void jhcPatchProps::SetPrimary (double p, double d, double s, double f)
{
  cprime = p;
  cdom   = d;
  csec   = s;
  cmin   = f;
}


//= Parameters used for determining if something is striped.

int jhcPatchProps::stripe_params (const char *fname)
{
  jhcParam *ps = &sps;
  int ok;

  ps->SetTag("prop_stripe", 0);
  ps->NextSpec4( &ejth,  35,    "Edge threshold");  
  ps->NextSpec4( &elen,  30,    "Min edge length (pel)");  
  ps->NextSpec4( &nej,    5,    "Min number of edges");     // was 10
  ps->NextSpecF( &tfill,  0.03, "Min textured fraction");   // was 0.3
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for categorizing size and width.

int jhcPatchProps::size_params (const char *fname)
{
  jhcParam *ps = &zps;
  int ok;

  ps->SetTag("prop_size", 0);
  ps->NextSpecF( &big,    2.5, "Big size (in)");  
  ps->NextSpecF( &sm,     1.0, "Small size (in)");  
  ps->NextSpecF( &wth,    1.7, "Wide ratio");
  ps->NextSpecF( &nth,    0.7, "Narrow ratio");
  ps->NextSpecF( &tall,   5.0, "Tall size (in)");
  ps->NextSpecF( &petite, 1.0, "Short size (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcPatchProps::Defaults (const char *fname)
{
  int ok = 1;

  ok &= cfind_params(fname);
  ok &= hue_params(fname);
  ok &= cname_params(fname);
  ok &= stripe_params(fname);
  ok &= size_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcPatchProps::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= cps.SaveVals(fname);
  ok &= hps.SaveVals(fname);
  ok &= nps.SaveVals(fname);
  ok &= sps.SaveVals(fname);
  ok &= zps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Set sizes of internal images directly.

void jhcPatchProps::SetSize (int x, int y)
{
  // interior of patch
  shrink.SetSize(x, y, 1);

  // color functions
  clip.SetSize(x, y, 3);
  hmsk.SetSize(x, y, 1);
  hue.SetSize(hmsk);
  wht.SetSize(hmsk);
  blk.SetSize(hmsk);

  // texture functions
  thumb.SetSize(x, y, 1);
  ej.SetSize(thumb);
  ejv.SetSize(thumb);
  hcc.SetSize(x, y, 2);
  vcc.SetSize(hcc);
}


//= Clear some internal stuff in case it is displayed.

void jhcPatchProps::Reset ()
{
  int i;

  for (i = 0; i < cmax; i++)
  {
    cols[i]  = 0;
    cvect[i] = 0;
  }
}


///////////////////////////////////////////////////////////////////////////
//                             Color Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Determine primary and secondary colors for some region of the image.
// speeds things up if mask has proper bounding box set with black border
// use Color and AltColor to get text names

int jhcPatchProps::FindColors (const jhcImg& mask, const jhcImg& src)
{
  // duplicate source only in ROI to limit processing
  // then remove borders with unreliable color
  clip.CopyArr(src, mask);
  if (mask.RoiMinDim() < csm)
    shrink.CopyArr(mask);
  else
    BoxThresh(shrink, mask, csm, cth);   

  // do actual color analysis
  color_bins(clip, shrink);
  qual_col();
  return 1;
}


//= Process image fragment to yield hue histogram and coarse hue distribution.
// largely borrowed from jhcVisPart

void jhcPatchProps::color_bins (const jhcImg& src, const jhcImg& gate) 
{
  int i, wcnt, hcnt, bcnt, area2; 

  // find regions with valid hue
  HueMask(hue, hmsk, src, smin, 0);
  MaxAll(wht, src);                    // similar to MaxBoost (wht = very colorful)
  OverGate(hmsk, hmsk, wht, imin);
  ForceMono(wht, src, 2);              // wht = perceptual white
  UnderGate(hmsk, hmsk, wht, imax);    // very bright = white not colored

  // separate achromatic parts into white and black parts
  Threshold(blk, wht, -dark);         
  Threshold(wht, wht, white);         

  // clean up masks and count pixels
  MinComp2(wht, wht, hmsk);
  MinFcn(wht, wht, gate);
  wcnt = CountOver(wht, 128);
  MinComp2(blk, blk, hmsk);
  MinFcn(blk, blk, gate);
  bcnt = CountOver(blk, 128);

  // build hue histogram of colored foreground regions
  MinFcn(hmsk, hmsk, gate);
  HistOver(hhist, hue, hmsk, 128);
  hcnt = hhist.SumAll();

  // determine fractions of foreground in various color ranges (ROYGBV)
  cols[0] = hhist.SumRegion(clim[5] + 1, clim[0]);
  for (i = 1; i < 6; i++)
    cols[i] = hhist.SumRegion(clim[i - 1] + 1, clim[i]);

  // use foreground areas to compute white and black percentages
  area2 = CountOver(gate, 128);
  cols[6] = bcnt;                        // black
  cols[7] = area2 - hcnt - bcnt - wcnt;  // gray
  cols[8] = wcnt;                        // white
}


//= Interprets color histogram as a number of discrete color bands.
// Primary color must be above cprime fraction (e.g. 10%) and must
// be at least cdom times greater (e.g. 1.5x) than any other color.
// If no single primary color found, then all colors above cdom of 
// biggest are marked. Largely borrowed from jhcVisPart.

void jhcPatchProps::qual_col ()
{
  int i, cp, cm, th, hi, chi, most = -1;

  // clear all colors and find total pixel count
  pels = 0;
  for (i = 0; i < 9; i++)
  {
    pels += cols[i];
    cvect[i] = 0;
  }
  cm = ROUND(cmin * pels);
  cp = ROUND(cprime * pels);

  // find highest color bin percentage 
  hi = cm;
  for (i = 0; i < 9; i++)
    if (cols[i] > hi)
    {
      most = i;
      hi = cols[i];
    }
  chi = hi;

  // see if sufficiently higher than any other color
  th = ROUND(hi / cdom);
  if (most >= 0)
    for (i = 0; i < 9; i++)
      if ((i != most) && (cols[i] > th))
      {
        most = -1;
        break;
      }

  // if still no single color selected, pick all that are close
  if (most >= 0)
    cvect[most] = 3;
  else if (hi >= cp)
    for (i = 0; i < 9; i++)
      if (cols[i] > th)
        cvect[i] = 2;

  // set bit vector for all significant colors (e.g. > 10% primary)
  th = ROUND(csec * chi);
  th = __max(cm, th);
  for (i = 0; i < 9; i++)
    if ((cols[i] >= th) && (cvect[i] == 0))
      cvect[i] = 1;
}


//= Overwrite local cvec array to allow properties function to work on cached data.
// assumes v has at least cmax entries

void jhcPatchProps::ForceVect (int *v)
{
  int i;

  for (i = 0; i < cmax; i++)
    cvect[i] = v[i];
}


//= Return semantic color name for dominant color of object.
// must call FindColors first, start with n = 0 
// returns answer in lowercase, NULL if end of list

const char *jhcPatchProps::ColorN (int n) const
{
  int i, cnt = 0;

  for (i = 0; i < cmax; i++)
    if (cvect[i] >= 2)
      if (cnt++ == n)
        return cname[i];
  return NULL;
}


//= Return semantic color name for other colors in object.
// must call FindColors first, start with n = 0
// returns answer in lowercase, NULL if end of list

const char *jhcPatchProps::AltColorN (int n) const
{
  int i, cnt = 0;

  for (i = 0; i < cmax; i++)
    if (cvect[i] == 1)
      if (cnt++ == n)
        return cname[i];
  return NULL;
}


//= Lists all the colors that can be detected, one at a time.
// returns answer in lowercase, NULL if end of list

const char *jhcPatchProps::KnownColor (int n) const
{
  if ((n < 0) || (n >= cmax))
    return NULL;
  return cname[n];
}


//= Stretch quantified color histogram (9 bins) over larger histogram.
// makes nicer square bin histogram for debugging

int jhcPatchProps::QuantColor (jhcArr& dest) const
{
  int i, j, v, sz = dest.Size(), bin = sz / 9, n = 0;

  if (sz < 9)
    return 0;
  for (i = 0; i < 9; i++)
  {
    v = cols[i];
    for (j = 0; j < bin; j++)
      dest.ASet(n++, v);
  }
  while (n < sz)
    dest.ASet(n++, 0);
  return 1;
}


//= List all primary colors present separated by spaces.
// returns number of colors listed

int jhcPatchProps::MainColors (char *dest, int ssz) const
{
  const char *c;
  int i, cnt = 0;

  if (dest == NULL) 
    return 0;
  *dest = '\0';
  for (i = 0; i < cmax; i++)
  {
    if ((c = ColorN(i)) == NULL)
      break;
    if (cnt > 0)
      strcat_s(dest, ssz, " ");
    strcat_s(dest, ssz, c);
    cnt++;
  }
  return cnt;
}


//= List all secondary colors present separated by spaces.
// returns number of colors listed

int jhcPatchProps::AltColors (char *dest, int ssz) const
{
  const char *c;
  int i, cnt = 0;

  if (dest == NULL)
    return 0;
  *dest = '\0';
  for (i = 0; i < cmax; i++)
  {
    if ((c = AltColorN(i)) == NULL)
      break;
    if (cnt > 0)
      strcat_s(dest, ssz, " ");
    strcat_s(dest, ssz, c);
    cnt++;
  }
  return cnt;
}


//= For a color 0-8 (ROYGBP-KXW) tell if it is prime(3), main (2), alt (1), or none (0).

int jhcPatchProps::DegColor (int cnum) const
{
  if ((cnum >= 0) && (cnum <= 8))
    return cvect[cnum];
  return 0;
}


//= For a color 0-8 (ROYGBP-KXW) tell fraction of pixels that are that color.

double jhcPatchProps::AmtColor (int cnum) const
{
  if ((cnum >= 0) && (cnum <= 8))
    return(cols[cnum] / (double) pels);
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                            Texture Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Determine degree of stripedness for some region of the image.
// needs monochrome image as input
// speeds things up if mask has proper bounding box set

int jhcPatchProps::Striped (const jhcImg& mask, const jhcImg& mono)
{
  jhcImg gate;
  int atex = 0;

  // duplicate source only in ROI to limit processing then find edges
  thumb.CopyArr(mono, mask);
  RawSobel(ejv, ej, thumb);

  // find long horizontal edges in object
  OverGate(ej, ej, mask, 128, 128);
  Trinary(ej, ej, 128 - ejth, 128 + ejth);
  nh = GComps4(hcc, ej, elen, 50, 128);

  // find long vertical edges in object
  OverGate(ejv, ejv, mask, 128, 128);
  Trinary(ejv, ejv, 128 - ejth, 128 + ejth);
  nv = GComps4(vcc, ejv, elen, 50, 128);

  // add up number of edge pixels if enough valid edges
  if (nh >= nej)
    atex += CountOver(hcc);
  if (nv >= nej) 
    atex += CountOver(vcc);

  // determine what fraction of object is textured
  ftex = 0.0;
  if (atex > 0)
    ftex = 3.0 * atex / (double) CountOver(mask);
  return((ftex >= tfill) ? 1 : 0);
}


///////////////////////////////////////////////////////////////////////////
//                             Size and Shape                            //
///////////////////////////////////////////////////////////////////////////

//= Find size category based on object area and scale factor of pixels near base.
// returns: small (0), normal (1), or wide (2)

int jhcPatchProps::SizeClass (int area, double ppi) 
{
  dim = sqrt((double) area) / ppi;
  return SizeClass(dim);
}


//= Find size category based on given object max dimension (in inches).
// returns: small (0), normal (1), or wide (2)

int jhcPatchProps::SizeClass (double dmax) const
{
  if (dmax >= big)
    return 2;
  if (dmax > sm)
    return 1;
  return 0;
}


//= Find object width category based on oriented aspect ratio.
// can use width of bounding box versus height, or max planar dimension over Z height
// returns: narrow (0), normal (1), or wide (2)

int jhcPatchProps::WidthClass (double wx, double hy) 
{
  wrel = wx / hy;
  if (wrel >= wth)
    return 2;
  if (wrel > nth)
    return 1;
  return 0;
}


//= Find object height category based on absolute dimension (inches).
// returns: short (0), normal (1), or tall (2)

int jhcPatchProps::HeightClass (double zdim) const
{
  if (zdim >= tall)
    return 2;
  if (zdim > petite)
    return 1;
  return 0;
}






