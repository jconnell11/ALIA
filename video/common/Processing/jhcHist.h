// jhcHist.h : histogram filling routines, not a data container
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2019 IBM Corporation
// Copyright 2022 Etaoin Systems
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

#ifndef _JHCHIST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCHIST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcArr.h"
#include "Data/jhcImg.h"


//= Histogram filling routines, not a data container.
// histograms themselves are represented by jhcArr structures

class jhcHist
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg temp;
  int bin[256];

// PUBLIC MEMBER VARIABLES
public:
  double sc, gsc, bsc;                 // from call to Enhance
  int off, goff, boff;                 // from call to Enhance


// PUBLIC MEMBER FUNCTIONS
public:
  // basic histograms
  int HistAll (jhcArr& h, const jhcImg& src, 
               int vmin =0, int vmax =255, int squash =0);
  int HistRegion (jhcArr& h, const jhcImg& src, 
                  double xc, double yc, double wid, double ht);
  int HistRegion (jhcArr& h, const jhcImg& src, const jhcRoi& area);
  int HistRegion8 (jhcArr& h, const jhcImg& src, const jhcRoi& area, int clr =1);
  int HistAll16 (jhcArr& h, const jhcImg& src);
  int HistRegion16 (jhcArr& h, const jhcImg& src, int rx, int ry, int rw, int rh);
  int HistRegion16 (jhcArr& h, const jhcImg& src, const jhcRoi& area);

  // weighted partial histograms
  int HistNZ (jhcArr& h, const jhcImg& src, const jhcImg& mask,
              int vmin =0, int vmax =255, int squash =0);
  int HistOver (jhcArr& h, const jhcImg& src, const jhcImg& gate, int th =127, 
                int vmin =0, int vmax =255, int squash =0, int bump =1);
  int HistOver8 (jhcArr& h, const jhcImg& src, const jhcImg& gate, int th =127, int bump =1);
  int HistUnder (jhcArr& h, const jhcImg& src, const jhcImg& gate, int th =127, 
                 int vmin =0, int vmax =255, int squash =0);
  int HistWts (jhcArr& h, const jhcImg& src, const jhcImg& wts, 
               int vmin =0, int vmax =255, int squash =0);

  // color histograms & averages
  int HistRGB (jhcArr& red, jhcArr& grn, jhcArr& blu, const jhcImg& src, 
               int vmin =0, int vmax =255, int squash =0);
  int HistRGB (jhcArr& red, jhcArr& grn, jhcArr& blu, const jhcImg& src, const jhcRoi& area);
  int HistUnderRGB (jhcArr& red, jhcArr& grn, jhcArr& blu, 
                    const jhcImg& src, const jhcImg& gate, int th =127, 
                    int vmin =0, int vmax =255, int squash =0);
  int HistAvgs (jhcArr& avgs, const jhcImg& vals, const jhcImg& bins);

  // image enhancement
  int Enhance (jhcImg& dest, const jhcImg& src, double smax =8.0, const jhcRoi *area =NULL, int omax =0);
  int Enhance (jhcImg& dest, const jhcImg& src, const jhcImg& mask, double smax =8.0, int omax =0);  
  int Enhance3 (jhcImg& dest, const jhcImg& src, double smax =8.0, const jhcRoi *area =NULL, int omax =0);
  int Enhance16 (jhcImg& dest, const jhcImg& src, int pmax =4095, double smax =8.0, const jhcRoi *area =NULL, int omax =0);

  // directional projections
  int ProjectV (jhcArr& hist, const jhcImg& src, double sc =1.0) const;
  int ProjectV (jhcArr& hist, const jhcImg& src, const jhcRoi& area, double sc =1.0) const;
  int ProjectH (jhcArr& hist, const jhcImg& src, double sc =1.0) const;
  int ProjectH (jhcArr& hist, const jhcImg& src, const jhcRoi& area, double sc =1.0) const;
  int SliceV (jhcArr& hist, const jhcImg& src, int x) const;
  int SliceH (jhcArr& hist, const jhcImg& src, int y) const;
  int ProjMinV (jhcArr& hist, const jhcImg& src, const jhcRoi& area);
  int ProjPctV (jhcArr& hist, const jhcImg& src, const jhcRoi& area, double frac =0.5);

  // scatter plots
  int Hist2D (jhcImg& dest, const jhcImg& xval, const jhcImg& yval, double psc =1.0, double rsc =1.0);


// PRIVATE MEMBER FUNCTIONS
private:
  void compute_bins (int n, int vmin, int vmax, int squash);
  int linear_fix (double& isc, UC8 scaled[], jhcArr& hist, double smax, int omax) const;


};


#endif




