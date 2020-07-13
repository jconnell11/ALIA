// jhcDist.h : spreading activation like space claiming 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2016 IBM Corporation
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

#ifndef _JHCDIST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCDIST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include <stdlib.h>

#include "Data/jhcImg.h"


//= Spreading activation like space claiming. 

class jhcDist
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg a1, b1, a2, b2, a4;


// PUBLIC MEMBER FUNCTIONS
public:
  // Manhattan distance
  int Nearest (jhcImg& label, const jhcImg& seed, int bg =0, jhcImg *rng =NULL);
  int Nearest8 (jhcImg& label, const jhcImg& seed, int bg =0, jhcImg *rng =NULL);
  int Expand (jhcImg& label, const jhcImg& seed, int dmax, int bg =0); 

  // Euclidean distance
  int Voronoi (jhcImg& label, const jhcImg& seed, int bg =0, jhcImg *rng =NULL, 
               jhcImg *xrng =NULL, jhcImg *yrng =NULL, jhcImg *rng2 =NULL);
  int Voronoi8 (jhcImg& label, const jhcImg& seed, int bg =0, jhcImg *rng =NULL, 
                jhcImg *xrng =NULL, jhcImg *yrng =NULL, jhcImg *rng2 =NULL);

};


#endif




