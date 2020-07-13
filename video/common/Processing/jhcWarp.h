// jhcWarp.h : perform arbitrary remapping of image pixels
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2019 IBM Corporation
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

#ifndef _JHCWARP_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCWARP_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"


//= Perform arbitrary remapping of image pixels.
// allows input sample location for each output pixel to be specified
// typically would loop through all output pixels to define complete map
// saved map is stored in form to allow rapid video frame remapping

class jhcWarp
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg off, mix;
  int dw, dh, sw, sh, sln, nf;


// PUBLIC MEMBER VARIABLES
public:


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcWarp ();
  void DestSize (const jhcImg& ref);
  void DestSize (int x, int y);
  void SrcSize (const jhcImg& ref);
  void SrcSize (int x, int y, int f =1);
  void InitSize (int x, int y, int f);

  // read-only variables
  int XDim () const {return dw;}
  int YDim () const {return dh;}

  // main functions
  void ClrMap ();
  int SetWarp (int xd, int yd, double xs, double ys);
  int Warp (jhcImg& dest, const jhcImg& src, int r0 =0, int g0 =0, int b0 =0) const;

  // standard variants
  void Identity ();
  void LogZoom (double xc, double yc, double hfov =60.0);
  void Rotate (double degs);
  void Flatten (double r2f, double r4f, double mag =1.0);
  void Rectify (double r2f, double r4f, double mag =1.0, double degs =0.0);


// PRIVATE MEMBER FUNCTIONS
private:
  void map_mono (jhcImg& dest, const jhcImg& src, int v0) const;
  void map_color (jhcImg& dest, const jhcImg& src, int r0, int g0, int b0) const;

};


#endif  // once




