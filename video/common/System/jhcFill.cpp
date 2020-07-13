// jhcFill.cpp : makes binary mask from closed contour
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 IBM Corporation
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

#include "System/jhcFill.h"


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Generate a binary mask from a closed set of edges (black on white);
// NOTE: contour must be drawn with clipping, e.g. jhcDraw::DrawClip(1)

int jhcFill::RegionFill (jhcImg& mask, const jhcImg& contour)
{
  if (!mask.Valid(1) || !mask.SameFormat(contour))
    return Fatal("Bad input to jhcFill::RegionFill");
  SetSize(mask);

  // discard connected components touching border
  CComps4(cc, contour);
  box.FindBBox(cc);
  box.RemBorder(contour, 1);

  // draw retained components and add in edges
  box.ThreshValid(mask, cc);
  MaxComp2(mask, mask, contour);
  return 1;
}


//= Create a binary mask from a list of closed polygon vertices.

int jhcFill::PolyFill (jhcImg& mask, const int *rx, const int *ry, int n)
{
  if (!mask.Valid(1) || (n < 3))
    return Fatal("Bad input to jhcFill::PolyFill");
  SetSize(mask);

  ej.FillArr(255);
  DrawPoly(ej, rx, ry, n, 1, 0);
  return RegionFill(mask, ej);
}


//= Create binary mask from a list of points representing a closed spline.

int jhcFill::SplineFill (jhcImg& mask, const int *rx, const int *ry, int n)
{
  if (!mask.Valid(1) || (n < 3))
    return Fatal("Bad input to jhcFill::SplineFill");
  SetSize(mask);

  ej.FillArr(255);
  MultiSpline(ej, rx, ry, n, 1, 0);
  return RegionFill(mask, ej);
}


//= Create a binary mask as the region between two open splines.
// connects closest endpoints with stright lines

int jhcFill::RibbonFill (jhcImg& mask, const int *rx, const int *ry, int n, 
                         const int *rx2, const int *ry2, int n2)
{
  if (!mask.Valid(1) || ((n < 2) && (n2 < 2)))
    return Fatal("Bad input to jhcFill::RibbonFill");
  SetSize(mask);

  ej.FillArr(255);
  Ribbon(ej, rx, ry, n, rx2, ry2, n2, 1, 0);
  return RegionFill(mask, ej);
}