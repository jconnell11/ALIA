// jhcTools.h : combination of all image processing libraries
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2006 IBM Corporation
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

#ifndef _JHCTOOLS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTOOLS_
/* CPPDOC_END_EXCLUDE */

#include "Processing/jhcALU.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcColor.h"
#include "Processing/jhcDist.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcEdge.h"
#include "Processing/jhcGray.h"
#include "Processing/jhcGroup.h"
#include "Processing/jhcHist.h"
#include "Processing/jhcLUT.h"
#include "Processing/jhcResize.h"
#include "Processing/jhcRuns.h"
#include "Processing/jhcStats.h"
#include "Processing/jhcThresh.h"
#include "Processing/jhcVect.h"


//= Combination of all image processing libraries.
// this pseudo-class includes all the functions available
// special processing packages can be derived from this class
//   when doing so, it is good to make this base class as "private"
//   relevant pieces can be extracted later when prototyping is done

class jhcTools : 
  public jhcALU,   public jhcArea,   public jhcColor,  public jhcDist, 
  public jhcDraw,  public jhcEdge,   public jhcGray,   public jhcGroup, 
  public jhcHist,  public jhcLUT,    public jhcResize, public jhcRuns,  
  public jhcStats, public jhcThresh, public jhcVect
{
  void swap_imgs (jhcImg **a, jhcImg **b)   /** Interchange two pointers. **/
    {jhcImg *swap = *a; *a = *b; *b = swap;};
};


#endif 







