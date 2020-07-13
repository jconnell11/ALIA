// jhcFill.h : makes binary mask from closed contour
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

#ifndef _JHCFILL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFILL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcBBox.h"
#include "Data/jhcImg.h"
#include "Processing/jhcALU.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcGroup.h"


//= Makes binary mask from closed contour.
// NOTE: if sharing jhcDraw class, this version always saturates line against borders

class jhcFill : private jhcALU, virtual private jhcDraw, virtual private jhcGroup
{
// PRIVATE MEMBER VARIABLES
private:
  jhcBBox box;
  jhcImg ej, cc;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  jhcFill () {box.SetSize(100); DrawClip(1);}
  void SetSize (const jhcImg& ref) {SetSize(ref.XDim(), ref.YDim());}
  void SetSize (int x, int y) {ej.SetSize(x, y, 1); cc.SetSize(x, y, 2);}

  // main functions
  int RegionFill (jhcImg& mask, const jhcImg& contour);
  int PolyFill (jhcImg& mask, const int *rx, const int *ry, int n);
  int SplineFill (jhcImg& mask, const int *rx, const int *ry, int n);
  int RibbonFill (jhcImg& mask, const int *rx, const int *ry, int n, 
                  const int *rx2, const int *ry2, int n2);


};


#endif  // once




