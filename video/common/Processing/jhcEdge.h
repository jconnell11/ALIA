// jhcEdge.h : find edges in images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2000-2013 IBM Corporation
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

#ifndef _JHCEDGE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCEDGE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Standard edge finders and some others.

class jhcEdge
{
// PRIVATE MEMBER VARIABLES
private:
  static int instances;
  static US16 (*arct)[256];
  static US16 (*root)[256];
  static UC8 *third;

  jhcImg tmp, tmp2;


// PUBLIC MEMBER FUNCTIONS
public:
  jhcEdge ();
  ~jhcEdge ();

  // simplest edge finder
  int RobEdge (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int RobDir (jhcImg& dest, const jhcImg& src) const;
  int RawRob (jhcImg& xm, jhcImg& ym, const jhcImg& src) const;

  // standard small kernel
  int SobelEdge (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int SobelDir (jhcImg& dest, const jhcImg& src, int nz =0) const;
  int SobelAng (jhcImg& dest, const jhcImg& src, int mth =40, int mod180 =0) const;
  int SobelFull (jhcImg& mag, jhcImg& dir, const jhcImg& src, double sc =1.0, int nz =0) const;

  // color versions
  int SobelRGB (jhcImg& dest, const jhcImg& src, double sc =1.0);
  int SobelMagRGB (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int SobelMagRGB2 (jhcImg& dest, const jhcImg& src, double sc =1.0) const;
  int SobelAngRGB (jhcImg& dest, const jhcImg& src, int mth =40) const;
  int SobelAngRGB2 (jhcImg& dest, const jhcImg& src, int mth =40) const;
  int SobelFullRGB (jhcImg& mag, jhcImg& dir, const jhcImg& src, double sc =1.0, int nz =0) const;
  int SobelFullRGB2 (jhcImg& mag, jhcImg& dir, const jhcImg& src, double sc =1.0, int nz =0) const;

  // categorized directions
  int SobelHV (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi =15.0, double lo =10.0) const;
  int SobelHVD (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi =15.0, double lo =10.0) const;
  int Sobel22 (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi =15.0, double lo =10.0) const;
  int SobelQuad (jhcImg& mag, jhcImg& hv, jhcImg& d12, const jhcImg& src, double hi =15.0, double lo =10.0) const;

  // lower level results
  int RawSobel (jhcImg& xm, jhcImg& ym, const jhcImg& src) const;
  int RawSobel4 (jhcImg& xm, jhcImg& ym, 
                 jhcImg& d1m, jhcImg& d2m, const jhcImg& src) const;
  int DirSel (jhcImg& dest, const jhcImg& src, double alo, double ahi, 
              int mod180 =0, double sc =1.0) const;

  // primitive texture
  int AbsSobel4 (jhcImg& xm, jhcImg& ym, jhcImg& d1m, jhcImg& d2m, 
                 const jhcImg& src, double sc =1.0) const;
  int DomDir (jhcImg& dest, const jhcImg& dx, const jhcImg& dy, 
              const jhcImg& d1, const jhcImg& d2, int th =0) const;
  int DirMix (jhcImg& dx, jhcImg& dy, jhcImg& d1, jhcImg& d2, 
              jhcImg& nej, int th =0, int vnej =255) const;
              
  // edge variants                 
  int TripleEdge (jhcImg& dest, const jhcImg& src) const;
  int EdgeVect (jhcImg& unitx, jhcImg& unity, const jhcImg& src, int th =40) const;
  int DirMask (jhcImg& dir, jhcImg& mask, const jhcImg& src, int th =40) const;

  // underlying edge functions
  int RMS (jhcImg &dest, const jhcImg& dx, const jhcImg& dy, double sc =1.0) const;
  int Angle (jhcImg &dest, const jhcImg& dx, const jhcImg& dy) const;

  // bar finder
  int SobelBar (jhcImg& mag, jhcImg& dir, const jhcImg& src, double sc =1.0) const; 


// PRIVATE MEMBER FUNCTIONS
private:
  int SobelHV_RGB (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi =15.0, double lo =10.0) const;
  int SobelHVD_RGB (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi =15.0, double lo =10.0) const;
  int Sobel22_RGB (jhcImg& mag, jhcImg& dir, const jhcImg& src, double hi =15.0, double lo =10.0) const;
  int SobelQuad_RGB (jhcImg& mag, jhcImg& hv, jhcImg& d12, const jhcImg& src, double hi =15.0, double lo =10.0) const;


};


#endif




