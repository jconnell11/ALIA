// jhcStackSeg.h : object and freespace locator for Manus robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _JHCSTACKSEG_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSTACKSEG_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcBlob.h"              // common video
#include "Data/jhcImg.h" 
#include "Data/jhcParam.h"
#include "Processing/jhcALU.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcColor.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcEdge.h"
#include "Processing/jhcFilter.h"
#include "Processing/jhcGray.h"
#include "Processing/jhcGroup.h"
#include "Processing/jhcHist.h"
#include "Processing/jhcLUT.h"
#include "Processing/jhcRuns.h"
#include "Processing/jhcStats.h"
#include "Processing/jhcThresh.h"


//= Object and freespace locator for Manus robot.

class jhcStackSeg : private jhcALU,    private jhcArea,   private jhcColor, private jhcDraw, 
                    private jhcEdge,   private jhcFilter, private jhcGray,  private jhcGroup, 
                    private jhcHist,   private jhcLUT,    private jhcRuns,  private jhcStats, 
                    private jhcThresh
{
friend class CMensEtDoc;     // for debugging

// PRIVATE MEMBER VARIABLES
private:
  // preprocessing
  jhcImg enh, boost, rg, yb, wk;
  int iw, ih;

  // floor finding
  jhcImg wk3, rg3, yb3;
  jhcImg vote, vsm, bulk, floor;
  jhcArr fhist[3];
  jhcRoi p1, p2;
  int flims[6];

  // object detection
  jhcImg hcc, bcc;
  jhcImg holes, floor2, cirq, bays;
  jhcBlob hblob, bblob;
  jhcRoi glf, grt, grip;

  // debugging graphics
  jhcImg bin, line;

jhcImg objs, tmp, part, occ;
jhcBlob oblob;

jhcImg cvx, mask, seed;
  jhcArr ohist[3];
  int olims[6];


// PUBLIC MEMBER VARIABLES
public:
  // color segmentation parameters
  jhcParam cps;
  double rise, irise, drop, idrop;
  int sm, dev, blur, pick;

  // floor sampling parameters
  jhcParam fps;
  int rw, rh, rdx, rdy, rw2, rh2, rdx2, rdy2;

  // object seed parameters
  jhcParam sps;
  double asp;
  int keep, fill, fsm, omin, bmax;

  // object mask parameters
  jhcParam mps;
  int gap, mfill, msm;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcStackSeg ();
  jhcStackSeg ();
  int XDim () const {return iw;}
  int YDim () const {return ih;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // configuration
  void SetSize (const jhcImg& ref) {SetSize(ref.XDim(), ref.YDim());}
  void SetSize (int x, int y);

  // main functions
  void Reset ();
  int Analyze (const jhcImg& src);
  const jhcImg *Clean () const {return &est;}
  const jhcImg *Mono () const  {return &wk;}

  // region selection
  int CloseAbove (int x, int y) const;
  int PadMask (jhcImg& dest, int n, int clr =0) const;

  // object properties
  int Bottom (int i) const;
  int WidthX (int i) const;
  int HeightY (int i) const;
  int AreaPels (int i) const;
  double BotScale (int i) const;

  // debugging graphics
  int Contour (jhcImg& dest, jhcImg& src, int n, int t =5, int r =255, int g =0, int b =255);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int floor_params (const char *fname);
  int color_params (const char *fname);
  int seed_params (const char *fname);
  int mask_params (const char *fname);

  // main functions
  void object_detect ();
  int ok_regions (jhcImg& dest, jhcBlob& data, const jhcImg& cc) const;
  void bottom_fill (jhcImg& dest, const jhcImg& src, int h);
  void floor_area ();

  // color predicate
  void color_desc (int *lims, jhcArr *cols) const;
  void same_color (jhcImg& sim, int *lims, const jhcRoi *area);

  void chunkify (jhcBlob& b, jhcImg& cc, const jhcImg& seeds);



};


#endif  // once




