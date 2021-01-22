// jhcArea.h : computes averages, etc. over blocks of pixels
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#ifndef _JHCAREA_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCAREA_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include <stdlib.h>

#include "Data/jhcImg.h"


//= Computes averages, etc. over blocks of pixels.
// Note: many functions migrated to jhcRuns and jhcDist
// NOTE: keeps private internal state so copies must be made for OpenMP

class jhcArea
{
// PRIVATE MEMBER VARIABLES
private:
  US16 v0[256], vals[256];
  jhcImg a1, b1, a4, b4;


// PUBLIC MEMBER FUNCTIONS
public:
  // simple dispatch forms
  int BoxAvgX (jhcImg& dest, const jhcImg& src, int w1, int h2 =0, 
               double sc =1.0, int diag =0);
  int BoxAvgStdX (jhcImg& avg, jhcImg& std, const jhcImg& src, 
                  int w1, int h2 =0, double dsc =1.0, int diag =0);
  int BoxAvgInvX (jhcImg& avg, jhcImg& isd, const jhcImg& src, 
                  int w1, int h2 =0, double dsc =1.0, int diag =0);
  int BoxAvg16X (jhcImg& dest, const jhcImg& src, int w1, int h2 =0, 
                 double sc =1.0, int diag =0);

  // local area averages
  int BoxAvg (jhcImg &dest, const jhcImg& src, int wid, int ht =0, 
              double sc =1.0, jhcImg *vic =NULL);
  int BoxAvg (jhcImg& dest, const jhcImg& src, double wf, double hf =0.0, 
              double sc =1.0, jhcImg *vic =NULL);
  int BoxAvgRGB (jhcImg &dest, const jhcImg& src, int wid, int ht =0, double sc =1.0);
  int BoxThresh (jhcImg &dest, const jhcImg& src, int sc, 
                 int th =128, int over =255, int under =0);
  int BoxAvg3 (jhcImg& dest, const jhcImg& src, double sc =1.0);
  int BoxThresh3 (jhcImg& dest, const jhcImg& src, 
                  int th =128, int over =255, int under =0);

  // gated area averages
  int MaskBoxAvg (jhcImg &dest, const jhcImg& src, const jhcImg &gate, int th, 
                  int wid, int ht =0, jhcImg *vic =NULL, jhcImg *vic2 =NULL);
  int MaskBoxAvg (jhcImg &dest, const jhcImg& src, const jhcImg &gate, int th, 
                  double wf, double hf =0.0, jhcImg *vic =NULL, jhcImg *vic2 =NULL);
  int NotBoxAvg (jhcImg &dest, const jhcImg& src, int wid, int ht =0, 
                 int bg =128, int def =0, int samps =1);
  int NZBoxAvg (jhcImg &dest, const jhcImg& src, int wid, int ht =0, int samps =1);
  int NZBoxAvg (jhcImg &dest, const jhcImg& src, double wf, double hf =0.0, int samps =1);
  int NZBoxMax (jhcImg &dest, const jhcImg& src, int wid, int ht =0, int samps =1);

  // center surround operations
  int ClipCS (jhcImg& dest, const jhcImg& src, 
              int cw, int ch, int sw, int sh, double sc =1.0);
  int LiftCS (jhcImg& dest, const jhcImg& src, 
              int cw, int ch, int sw, int sh, double sc =1.0);
  int LocalAGC (jhcImg& dest, const jhcImg& src, int wid =17, double sc =3.0, int i0 =20); 

  // statistical operations
  int BoxStd (jhcImg &dest, const jhcImg& src, int wid, int ht =0, double sc =1.0);
  int BoxAvgStd (jhcImg &avg, jhcImg& std, const jhcImg& src, 
                 int wid, int ht =0, double dsc =1.0);
  int BoxAvgInv (jhcImg &dest, jhcImg& isd, const jhcImg& src, 
                 int wid, int ht =0, double dsc =1.0);

  // diagonal area averages
  int DBoxAvg (jhcImg &dest, const jhcImg& src, int w1, int h2 =0, double sc =1.0);
  int DBoxAvgStd (jhcImg &avg, jhcImg& std, const jhcImg& src, 
                  int w1, int h2 =0, double dsc =1.0);
  int DBoxAvgInv (jhcImg &avg, jhcImg& isd, const jhcImg& src, 
                  int w1, int h2 =0, double dsc =1.0);

  // two byte value versions
  int BoxAvg16 (jhcImg &dest, const jhcImg& src, int wid, int ht =0, double sc =1.0);
  int DBoxAvg16 (jhcImg &dest, const jhcImg& src, int w1, int h2 =0, double sc =1.0);

  // max and min sweeps
  int BoxMax (jhcImg& dest, const jhcImg& src, int wid, int ht =0);
  int BoxMin (jhcImg& dest, const jhcImg& src, int wid, int ht =0);
  int BoxMin16 (jhcImg& dest, const jhcImg& src, int wid, int ht =0);

  // rank order filtering
  int BoxFracOver (jhcImg& dest, const jhcImg& src, int wid, int ht =0, double frac =0.5);
  int BoxFracUnder (jhcImg& dest, const jhcImg& src, int wid, int ht =0, double frac =0.5);
  int BoxRankLin (jhcImg& dest, const jhcImg& src, int wid, int ht =0, double frac =0.5);
  int BoxMedian (jhcImg& dest, const jhcImg& src, int sc);

  // tracking
  int NearestComp (int& wx, int& wy, const jhcRoi& area, const jhcImg& comps) const;
  double ExtremePt (int& ex, int& ey, int x0, int y0, 
                    const jhcImg& comps, int mark, const jhcRoi& area) const;
  double ExtremePt (int& ex, int& ey, int x0, int y0, const jhcImg& comps, int mark) const
    {return ExtremePt(ex, ey, x0, y0, comps, mark, comps);}


// PRIVATE MEMBER FUNCTIONS
private:
  int box_avg0 (jhcImg &dest, const jhcImg& src, int dx, int dy, double sc, jhcImg *vic);
  void cdiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const;
  void ldiff (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, double sc) const;
  void thresh (jhcImg& dest, const jhcImg& src, int th, int over, int under) const;

  void rem_pel (int v, US16 *hist, int& under, int cut, int wt) const;
  void add_pel (int v, US16 *hist, int& under, int cut, int wt) const;
  void mid_cut_up (int& cut, int& under, const US16 *hist, int th) const;
  void mid_cut_dn (int& cut, int& under, const US16 *hist, int th) const;

};


#endif




