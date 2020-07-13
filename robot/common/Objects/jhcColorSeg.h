// jhcColorSeg.h : find objects against a homogeneous background
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

#ifndef _JHCCOLORSEG_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCCOLORSEG_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcBlob.h"
#include "Data/jhcImg.h"
#include "Data/jhcParam.h"
#include "Data/jhcRoi.h"
#include "Processing/jhcALU.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcColor.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcGray.h"
#include "Processing/jhcGroup.h"
#include "Processing/jhcHist.h"
#include "Processing/jhcLUT.h"
#include "Processing/jhcResize.h"
#include "Processing/jhcThresh.h"


//= Find objects against a homogeneous background.

class jhcColorSeg : private jhcALU,    private jhcArea,   private jhcColor, private jhcDraw, 
                    private jhcGray,   private jhcGroup,  private jhcHist,  private jhcLUT,  
                    private jhcResize, private jhcThresh
{
// PRIVATE MEMBER VARIABLES
private:
  jhcArr hist, rghist, ybhist, wkhist;
  jhcImg boost, patch, retain, mask, targs;
  jhcImg rg, yb, wk, rg3, yb3, wk3, vote, vsm, bulk;
  int iw, ih, rg0, rg1, yb0, yb1, wk0, wk1;
  double f;


// PUBLIC MEMBER VARIABLES
public:
  jhcImg comps;
  jhcBlob blob;

  // region parameters
  jhcParam rps;
  int px0, px1, py0, py1, rx0, rx1, ry0, ry1; 

  // color parameters
  jhcParam cps;
  int sm, dev, blur, pick, amin;
  double rise, drop, idrop;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcColorSeg ();
  void SetSize (const jhcImg& ref);
  void SetSize (int x, int y);

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  void SetRegion (int pr, int pl, int pt, int pb, int rr, int rl, int rt, int rb);
  void SetParse (int hs, int tol, int es, int th, int a, double r, double cd, double id);
  void CopyParse (const jhcColorSeg& ref);

  // non-background object spotting
  int FindHoles (const jhcImg& src, const jhcImg *clr =NULL, const jhcImg *area =NULL);
  int ColorBG (const jhcImg& src, const jhcImg *clr =NULL);
  void CopyColor (const jhcColorSeg& ref);
  int MaskBG (const jhcImg& src, int get_col =1);
  int ParseFG (const jhcImg *area =NULL);

  // foreground properties
  int HoleCount () const {return blob.CountValid();};
  int HoleArea (int n =1) const;
  int HoleCentroid (double& x, double& y, int n =1) const;
  int HoleBBox (jhcRoi& box, int n =1) const;
  int HoleEllipse (double& len, double& wid, double& axis, int n =1) const;
  int HoleMask (jhcImg& mask, int n =1) const;
  int NearestBox (int x, int y) const;

  // read-only access
  const jhcImg& Colorful () const   {return boost;}
  const jhcImg& Background () const {return bulk;}
  const jhcImg& Voting (int sm =0) const        {return((sm > 0) ? vsm : vote);}
  const jhcImg& RedGreen (int bland =0) const   {return((bland > 0) ? rg3 : rg);}
  const jhcImg& YellowBlue (int bland =0) const {return((bland > 0) ? yb3 : yb);}
  const jhcImg& WhiteBlack (int bland =0) const {return((bland > 0) ? wk3 : wk);}
  const jhcArr& HistRG () const {return rghist;}
  const jhcArr& HistYB () const {return ybhist;}
  const jhcArr& HistWK () const {return wkhist;}
  int LimRG (int hi) const {return((hi > 0) ? rg1 : rg0);}
  int LimYB (int hi) const {return((hi > 0) ? yb1 : yb0);}
  int LimWK (int hi) const {return((hi > 0) ? wk1 : wk0);}
  void Patch (jhcRoi& area) const {area.SetRoi(px0, py0, px1 - px0, py1 - py0);}
  void Valid (jhcRoi& area) const {area.SetRoi(rx0, ry0, rx1 - rx0, ry1 - ry0);}

  // debugging graphics
  int PrettyHoles (jhcImg& dest);
  int DrawBBox (jhcImg& dest, int n =1, int t =1, int r =255, int g =0, int b =255);


// PRIVATE MEMBER FUNCTIONS
private:
  int region_params (const char *fname);
  int color_params (const char *fname);

};


#endif  // once




