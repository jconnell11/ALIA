// jhcThresh.h : collection of functions for thresholding images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2020 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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

#ifndef _JHCTHRESH_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTHRESH_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Pass various parts of image or convert to standard values.

class jhcThresh_0
{
// PUBLIC MEMBER FUNCTIONS
public:
  // sharp thresholds
  int Threshold (jhcImg& dest, const jhcImg& src, int th, int mark =255) const;
  int Threshold (jhcImg& dest, const jhcImg& src, const jhcRoi& area, int th, int mark =255) const;
  int Between (jhcImg& dest, const jhcImg& src, int lo, int hi, int mark =255) const;
  int AbsOver (jhcImg& dest, const jhcImg& src, int th) const;
  int Trinary (jhcImg& dest, const jhcImg& src, int lo, int hi) const;
  int BothWithin (jhcImg& dest, const jhcImg& src1, const jhcImg& src2, int lo, int hi) const;
  int MatchKey (jhcImg& dest, const jhcImg& src, int key, int mark =255) const;
  int DeadBand (jhcImg& dest, const jhcImg& src, int delta) const;
  int Squelch (jhcImg& dest, const jhcImg& src, int level) const;
  int ZeroOver (jhcImg& dest, const jhcImg& src, int level) const;
  int OverBy (jhcImg& dest, const jhcImg& src, const jhcImg& ref, int delta, int mark =255) const;
  int UnderBy (jhcImg& dest, const jhcImg& src, const jhcImg& ref, int delta, int mark =255) const;

  // soft thresholds
  int RampOver (jhcImg& dest, const jhcImg& src, int lo, int hi) const;
  int RampOver (jhcImg& dest, const jhcImg& src, int mid, double slope) const;
  int RampUnder (jhcImg& dest, const jhcImg& src, int lo, int hi) const;
  int RampUnder (jhcImg& dest, const jhcImg& src, int mid, double slope) const;
  int SoftOver (jhcImg& dest, const jhcImg& src, int th, int dev) const;
  int SoftOver (jhcImg& dest, const jhcImg& src, int th, double frac =0.2) const;
  int SoftUnder (jhcImg& dest, const jhcImg& src, int th, int dev) const;
  int SoftUnder (jhcImg& dest, const jhcImg& src, int th, double frac =0.2) const;
  int InRange (jhcImg& dest, const jhcImg& src, int lo, int hi, int dev =0, int nz =0) const;

  // direct soft threshold combination
  int MinUnder (jhcImg& dest, const jhcImg& old, const jhcImg& src, int th, int soft =0) const;
  int MinOver (jhcImg& dest, const jhcImg& old, const jhcImg& src, int th, int soft =0) const;
  int MaxUnder (jhcImg& dest, const jhcImg& old, const jhcImg& src, int th, int soft =0) const;
  int MaxOver (jhcImg& dest, const jhcImg& old, const jhcImg& src, int th, int soft =0) const;
  int KeepOver (jhcImg& dest, const jhcImg& src, const jhcImg& bar, int alt =0) const;
  int KeepPeak (jhcImg& dest, const jhcImg& lo1, const jhcImg& src, const jhcImg& lo2) const;

  // gating
  int ThreshGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th =0, int def =0) const;
  int ThreshGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, 
                     int rdef =0, int gdef =0, int bdef =0) const;
  int OverGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th =0, int def =0) const;
  int OverGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, 
                   int rdef =0, int gdef =0, int bdef =0) const;
  int UnderGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th =255, int def =0) const;
  int UnderGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, 
                   int rdef =0, int gdef =0, int bdef =0) const;
  int BandGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, 
                int lo, int hi =255, int def =0) const;
  int BandGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, 
                   int lo, int hi =255, int rdef =0, int gdef =0, int bdef =0) const;
  int AlphaGate (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int def =0) const;
  int AlphaGateRGB (jhcImg& dest, const jhcImg& src, const jhcImg& gate, 
                    int rdef =0, int gdef =0, int bdef =0) const;
  int Composite (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, const jhcImg& awt) const;
  int CompositeRGB (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, const jhcImg& awt) const;

  // pixel switching
  int OverlayNZ (jhcImg& dest, const jhcImg& src, const jhcImg& marks) const;
  int OverlayNZ (jhcImg& dest, const jhcImg& marks) const 
    {return OverlayNZ(dest, dest, marks);}
  int SubstOver (jhcImg& dest, const jhcImg& alt, const jhcImg& gate, int th =0) const;
  int SubstUnder (jhcImg& dest, const jhcImg& alt, const jhcImg& gate, int th =255) const;
  int SubstKey (jhcImg& dest, const jhcImg& alt, const jhcImg& gate, int val =128) const;
  int MarkTween (jhcImg& dest, const jhcImg& val, int lo, int hi, int mark =255) const;

  // area restriction
  int RoiNZ (jhcRoi& region, const jhcImg& src) const;
  int RoiThresh (jhcRoi& region, const jhcImg& src, int th=255) const;


// PROTECTED MEMBER FUNCTIONS
protected:
  int Thresh16 (jhcImg& dest, const jhcImg& src, int th, int mark =255) const;
  int OverGateBW (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, int def) const;
  int OverGate16 (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, int def) const;
  int UnderGateBW (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int th, int def) const;
  int BandGateBW (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int lo, int hi, int def) const;
  int AlphaGateBW (jhcImg& dest, const jhcImg& src, const jhcImg& gate, int def) const;
  int CompositeBW (jhcImg& dest, const jhcImg& imga, const jhcImg& imgb, const jhcImg& awt) const;
  int OverlayNZ_RGB (jhcImg& dest, const jhcImg& src, const jhcImg& marks) const;
  int OverlayNZ_BW (jhcImg& dest, const jhcImg& src, const jhcImg& marks) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // ROI utilities
  void adj_lims (int v, int& lo, int& hi, int& n) const;

  
};


#endif


/////////////////////////////////////////////////////////////////////////////

//= Allows transparent insertion of MMX class over top of normal class.

#ifdef JHC_MMX
#include "MMX/jhcThresh_mmx.h"
#else
typedef jhcThresh_0 jhcThresh;
#endif




