// jhcPyramid.cpp : routines for composite octave pyramids
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2004-2011 IBM Corporation
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

#include "jhcPyramid.h"


///////////////////////////////////////////////////////////////////////////

/* CPPDOC_BEGIN_EXCLUDE */

// minimum height of smallest level of the pyramid

const int JPYR_HMIN = 30;

/* CPPDOC_END_EXCLUDE */


///////////////////////////////////////////////////////////////////////////

// pyramid levels are all stacked together in the same image
//
//  +---------------------------------+
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  |               L0                |
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  +----------------+----------------+
//  |                |                |
//  |                |                |
//  |                +--------+       |
//  |      L1        |        |       |
//  |                |   L2   +----+  |
//  |                |        | L3 |  |
//  +----------------+--------+----+--+


///////////////////////////////////////////////////////////////////////////
//                              Utilities                           //
///////////////////////////////////////////////////////////////////////////

//= Resize an image to accept a pyramid based on some source.

jhcImg *jhcPyramid::PyrSize (jhcImg& dest, const jhcImg& src) const 
{
  if ((src.YDim() / 2) < JPYR_HMIN)
    return dest.SetSize(src);
  return dest.SetSize(src.XDim(), (3 * src.YDim()) / 2, src.Fields()); 
}


//= Determines if the pyramid image is the correct size for the given source.

int jhcPyramid::PyrOK (const jhcImg& pyr, const jhcImg& src) const 
{
  if ((pyr.Fields() != src.Fields()) || (pyr.XDim() != src.XDim()) ||
      (pyr.YDim() != ((3 * src.YDim()) / 2)))
    return 0;
  return 1;
}


//= Report how many levels there are in a composite pyramid.
// level number arguments run from zero to one less than this value

int jhcPyramid::PyrDepth (const jhcImg& pyr) const 
{
  int n = 1, h = (2 * pyr.YDim()) / 3;

  while (h > JPYR_HMIN)
  {
    n++;
    h /= 2;
  }
  return n;
}


//= Tells the width of a level without actually changing the ROI.

int jhcPyramid::PyrWid (const jhcImg& pyr, int level) const 
{
  int n = 0, w = pyr.XDim();

  if ((level < 0) || (level >= PyrDepth(pyr)))
    return 0;
  while (n++ < level)
    w /= 2;
  return w;
}


//= Tells the height of a level without actually changing the ROI.

int jhcPyramid::PyrHt (const jhcImg& pyr, int level) const 
{
  int n = 0, h = (2 * pyr.YDim()) / 3;

  if ((level < 0) || (level >= PyrDepth(pyr)))
    return 0;
  while (n++ < level)
    h /= 2;
  return h;
}


///////////////////////////////////////////////////////////////////////////
//                           Pyramid Formation                           //
///////////////////////////////////////////////////////////////////////////

//= Create a pyramid by averaging together four pixels at each level.

int jhcPyramid::PyrAvg (jhcImg& pyr, const jhcImg& src) const 
{
  return 1;
}


//= Create a pyramid by sampling the lower left corner pixel at each level.

int jhcPyramid::PyrSamp (jhcImg& pyr, const jhcImg& src) const 
{
  return 1;
}


//= Create a pyramid by taking the max value of four pixels at each level.

int jhcPyramid::PyrMax (jhcImg& pyr, const jhcImg& src) const 
{

  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Level Extraction                            //
///////////////////////////////////////////////////////////////////////////

//= Set the image ROI to be some particular level of the pyramid.
// returns 1 if level reached, else returns 0 for nearest level

int jhcPyramid::PyrRoi (jhcImg& pyr, int level) const 
{
  int x = 0, y = 0, w = pyr.XDim(), h = pyr.YDim();
  int n = 0, stop = 2 * JPYR_HMIN;

  if (h >= stop)
  {
    // set up parameters for level 0 image in non-degenerate pyramid
    h = (2 * h) / 3;
    y = h / 2;

    if (level > 0)
    {
      // the level 1 image is directly below the level 0 image
      y = 0;
      w /= 2;
      h /= 2;
      n++;

      // each successive image is a shift aint the baseline
      while ((n < level) && (h >= stop))
      {
        x += w;
        w /= 2;
        h /= 2;
        n++; 
      }
    }
  }

  // configure ROI with values and see if exact
  pyr.SetRoi(x, y, w, h);
  if (n == level)
    return 1;
  return 0;
}
    

//= Expand a level of the pyramid to fit into the given image.
// leaves the pyramid ROI set to the selected level

int jhcPyramid::PyrGet (jhcImg& dest, jhcImg& pyr, int level) const 
{
  int lv = PyrRoi(pyr, level), pw = pyr.RoiW(), ph = pyr.RoiH();
  int f = dest.Fields(), dup = dest.YDim() / ph;

  // check for correct size destination
  if ((lv <= 0) || (pyr.Fields() != f) || 
      (dup <= 0) || (dest.XDim() < (dup * pw)))
    return Fatal("Bad arguments to jhcPyramid::PyrGet");

  // duplicate samples to create bigger image
  const UC8 *s = pyr.RoiSrc();
  UC8 *d = dest.PxlDest();
  int psk = pyr.RoiSkip(), dsk = dest.Line() - dup * pw * f;
  int x, y, i, j, k;

  for (y = ph; y > 0; y--)
  {
    for (j = dup; j > 0; j--)
    {
      for (x = pw; x > 0; x--, s += f)
        for (i = dup; i > 0; i--, d += f)
          for (k = 0; k < f; k++)
            d[k] = s[k];
      d += dsk;
    }
    s += psk;
  }
  return 1;
}

