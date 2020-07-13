// jhcVisPart.cpp : data about a portion of a visual object
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

#include <string.h>

#include "Objects/jhcVisPart.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcVisPart::jhcVisPart ()
{
  int i;

  // set hue histogram size
  hhist.SetSize(256);

  // clear semantic name and status
  *name = '\0';
  status = -1;

  // clear all numerical data
  for (i = 0; i < 9; i++)
    cols[i] = 0;
  cx = 0.0;
  cy = 0.0;

  // clear iconic data
  rx = 0;
  ry = 0;
  IconSize(0, 0);

  // nothing else in list yet
  next = NULL;
}


//= Default destructor does necessary cleanup.

jhcVisPart::~jhcVisPart ()
{
  if (next != NULL)
    delete next;
}


//= Set size of internal images.

void jhcVisPart::IconSize (int x, int y)
{
  // record for later use
  rw = x;
  rh = y;

  // RGB images
  crop.SetSize(rw, rh, 3);

  // monochrome images
  mask.SetSize(rw, rh, 1);
  shrink.SetSize(mask);
  hue.SetSize(mask);
  hmsk.SetSize(mask);
  wht.SetSize(mask);
  blk.SetSize(mask);
}


// Copy all information from some other part.

void jhcVisPart::Copy (jhcVisPart& src)
{
  int i;

  // copy selection status and semantic name
  status = src.status;
  if (status < 0)
    return;
  strcpy_s(name, src.name);

  // copy overall position and size
  cx = src.cx;
  cy = src.cy;
  area  = src.area;
  area2 = src.area2;

  // copy color properties
  hhist.Copy(src.hhist);
  for (i = 0; i < 9; i++)
  {
    cols[i]  = (src.cols)[i];
    cvect[i] = (src.cvect)[i];
  }

  // copy patch location and size
  rx = src.rx;
  ry = src.ry;
  IconSize(src.rw, src.rh);

  // copy processed patch images
  crop.CopyArr(  src.crop);
  mask.CopyArr(  src.mask);
  shrink.CopyArr(src.shrink);
  hue.CopyArr(   src.hue);
  hmsk.CopyArr(  src.hmsk);
  wht.CopyArr(   src.wht);
  blk.CopyArr(   src.blk);
}


///////////////////////////////////////////////////////////////////////////
//                          Part Characteristics                         //
///////////////////////////////////////////////////////////////////////////

//= Return semantic color name for dominant color of object.
// returns answer in either capitalized or lowercase format 

const char *jhcVisPart::Color (int n)
{
  char cname[9][20] = {"red", "orange", "yellow", "green", "blue", "purple", 
                       "black", "gray", "white"};
  int i, cnt = 0;

  for (i = 0; i < 9; i++)
    if (cvect[i] == 2)
      if (cnt++ == n)
      {
        strcpy_s(main, cname[i]);
        return main;
      }
  return NULL;
}


//= Return semantic color name for other colors in object.

const char *jhcVisPart::AltColor (int n) 
{
  char cname[9][20] = {"red", "orange", "yellow", "green", "blue", "purple", 
                       "black", "gray", "white"};
  int i, cnt = 0;

  for (i = 0; i < 9; i++)
    if (cvect[i] == 1)
      if (cnt++ == n)
      {
        strcpy_s(mix, cname[i]);
        return mix;
      }
  return NULL;
}


///////////////////////////////////////////////////////////////////////////
//                            Part Analysis                              //
///////////////////////////////////////////////////////////////////////////

//= Build color histograms after source and mask image are bound.

int jhcVisPart::Analyze (const int *clim)
{
  BoxThresh(shrink, mask, 9, 200);  // borders are unreliable
  color_bins(crop, shrink, clim);
  qual_col();
  return 1;
}


//= Process image fragment to yield hue histogram and coarse hue distribution.
// adapted from jhcScoFeat::get_col and jhcScoFeat::frac_col 
// Note: color bins copied in jhcObjList::clim

void jhcVisPart::color_bins (const jhcImg& src, const jhcImg& gate, const int *clim) 
{
//  int clim[6] = {13, 26, 47, 120, 170, 234};  // boundaries of ROYGBV-R hues (YG was 50 then 45, RO was 12)
//  int clim[6] = {7, 26, 47, 120, 170, 234};  // boundaries of ROYGBV-R hues (RO was 13)
  double bdec = 0.1;
  int smin = 50, imin = 80;                   // smin was 30
  int white = 200, dark = 90, brite = 240;    // white 180-240, imin 50
  int i, wcnt, hcnt, bcnt; 
 
  // find regions with valid hue
  HueMask(hue, hmsk, src, smin, 0);
  MaxAll(wht, src);                   // similar to MaxBoost (wht = very colorful)
  OverGate(hmsk, hmsk, wht, imin);
  ForceMono(wht, src, 2);             // wht = perceptual white
  UnderGate(hmsk, hmsk, wht, brite);  // very bright = white not colored
//  BoxThresh(hmsk, hmsk, 3, 128);      

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
  area2 = CountOver(shrink, 128);
  cols[6] = bcnt;                        // black
  cols[7] = area2 - hcnt - bcnt - wcnt;  // gray
  cols[8] = wcnt;                        // white
}


//= Interprets color histogram as a number of discrete color bands.
// Primary color must be above cprime fraction (e.g. 10%) and must
// be at least cdom times greater (e.g. 1.5x) than any other color.
// If no single primary color found, then all colors above cdom of 
// biggest are marked. Adapted from jhcScoFeat::bin_col

void jhcVisPart::qual_col ()
{
  double cmin = 0.05, cprime = 0.2, cdom = 2.0, csec = 0.3;
  int i, cp, cm, th, hi, chi, sum = 0, most = -1;

  // clear all colors and find total pixel count
  for (i = 0; i < 9; i++)
  {
    sum += cols[i];
    cvect[i] = 0;
  }
  cm = ROUND(cmin * sum);
  cp = ROUND(cprime * sum);

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
    cvect[most] = 2;
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

