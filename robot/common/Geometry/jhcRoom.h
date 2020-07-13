// jhcRoom.h : contour bounding local environment
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 IBM Corporation
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

#ifndef _JHCROOM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCROOM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"            // common video
#include "Processing/jhcDraw.h"


//= Stores segments for contour bounding local environment.
// each segment is a start point and an end point in xy inches
// can use multiple instance for different colors and thicknesses
// e.g. walls are drawn white x3 while tables are magenta x1 thick

class jhcRoom : private jhcDraw
{
// PRIVATE MEMBER VARIABLES
private:
  char wfile[80];
  double *segs;
  double x0, x1, y0, y1;
  int nlim, nseg;


// PUBLIC MEMBER VARIABLES
public:
  // drawing parameters
  jhcParam dps;
  double ipp;
  int bdx, bdy;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcRoom (int n =50);
  ~jhcRoom ();
  void Clear ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // read-only access 
  int NumEjs () const {return nseg;}
  int XDim () const {return(ROUND((x1 - x0) / ipp) + 2 * bdx);}   // width of image needed 
  int YDim () const {return(ROUND((y1 - y0) / ipp) + 2 * bdy);}   // height of image needed
  int XOff () const {return(ROUND(-x0 / ipp) + bdx);}             // pixel for x = 0.0 inches
  int YOff () const {return(ROUND(-y0 / ipp) + bdy);}             // pixel for y = 0.0 inches
  const char *Source () const {return wfile;}

  // main functions
  int Load (const char *fname, int clr =1);
  int DrawRoom (jhcImg& dest, int t =3, int r =255, int g =255, int b =255) const;


// PRIVATE MEMBER FUNCTIONS
private:
  void alloc (int n);

  // configuration
  int draw_params (const char *fname =NULL);

};


#endif  // once



