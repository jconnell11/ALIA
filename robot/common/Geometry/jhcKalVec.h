// jhcKalVec.h : performs smooth updating of 3D point or vector
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2017 IBM Corporation
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

#ifndef _JHCKALVEC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCKALVEC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Geometry/jhcMatrix.h"


//= Performs smooth updating of 3D point or vector.
// also maintains a count of consecutive hits and misses
// exposes standard vector utilities like X(), Y(), and Z()

class jhcKalVec : public jhcMatrix
{
// PRIVATE MEMBER VARIABLES
private:
  double var[3], noise[3];
  double mix;
  int cnt;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcKalVec ();
  void SetKal (double m, double x, double y, double z)    /** Kalman filter parameters.   */
    {mix = m; noise[0] = x; noise[1] = y; noise[2] = z;}
  int Blank () const {return((cnt == 0) ? 1 : 0);}        /** Never been updated yet.     */
  int Last () const  {return(1 + __max(0, -cnt));}        /** Frames since last update.   */

  // main functions
  void Clear () {cnt = 0;}                                /** Get ready for first update. */
  int Update (const jhcMatrix& raw, jhcMatrix *diff =NULL, double dt =0.0);
  int Skip ();


};


#endif  // once




