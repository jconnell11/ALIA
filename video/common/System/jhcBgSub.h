// jhcBgSub.h : background subtraction routines
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2001-2013 IBM Corporation
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

#ifndef _JHCBGSUB_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBGSUB_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "System/jhcAGC.h"

#include "Data/jhcImgIO.h"
#include "Data/jhcBBox.h"
#include "Data/jhcBlob.h"

#include "Processing/jhcFilter.h"
#include "Processing/jhcShift.h"
#include "Processing/jhcTools.h"


//= Background subtraction routines for object detection.

class jhcBgSub : private jhcTools, public jhcAGC
{
friend class CCafeDoc;              // for debugging
friend class CCafe4Doc;

private:
  jhcImgIO fio;
  jhcBBox objs;
  jhcFilter kal;

jhcImg big;  // for output conversion

jhcImg mag2, mag3;

  jhcImg *src, *ante, *last;

  int dup, pushed, mono, hcnt;
  int bad, samp, n, k, first, bgok, w, thmap;      // internal state variables

  jhcImg former, s0, ctmp, ctmp2, sfix, rfix;
  jhcImg tmp, tmp2, tmp3, rmot, any, map, heal;
  jhcImg comp, bw3;

  jhcImg prv2, prv3;
  jhcImg *prev, *gmot;

  jhcImg msrc2, msrc3;
  jhcImg *mot, *pmot;

  jhcImg nice;                                     // for contrast stretching
  jhcKnob ct0, ct1;
  jhcArr ihist;

  int tsm;       
  jhcImg m1, m3, mag, dir;                         // for NTSC fix
  int nuke;

  int st_cnt, st_num, st_bad, mfix;                // for de-jiggle
  jhcRoi st_roi;
  double fdx, fdy;
  double *vdx;
  jhcImg fg2, nomov, nomov2, gnow, glast, shmsk, refm; 
  jhcShift st;

  jhcImg avm, rtex;

  jhcImg probe, probe2, probe3;
  jhcImg ebg, emap, emask;

  jhcImg e2, e3;
  jhcImg *pej, *nej;

  jhcImg c2, c3;
  jhcImg *pcol, *col;

  jhcImg pmask2, label, csnow, csref;
  int knock;

  int nobjs;                           // number of connect components

private:
  int steps;                           // set non-zero to see intermediates
  jhcImg mv, ej, cdif, nosh;
  jhcImg bg, sal, quiet;               // basic images used in computations

  jhcImg msk2, msk3;
  jhcImg *pmask, *mask;

  jhcImg now2, now3;
  jhcImg q2, qfg, fixed, diff;
  jhcBlob regs;

  int off;                             // contrast adjustment used
  double sc;

public:
  //= Preprocessing image fix-up parameter set.
  // includes boost, wind, wob, ntsc, ksm, and agc
  jhcParam fps;  
  int boost;     /** Perform contrast stretching.     */
  int wind;      /** Use camera de-jittering.         */
  int wob;       /** Fix up wavy image from bad sync. */
  int ntsc;      /** Use color artifact removal.      */
  int ksm;       /** Use temporal smoothing.          */
  int agc;       /** Fix color and intensity drift.   */

  //= More detailed preprocessing parameter set.
  // includes hdes, maxdup, xrng, yrng, wx, smax, and maxfg
  jhcParam dps;  
  int hdes;      /** Minimum internal image height.     */
  int maxdup;    /** Duplicated frames allowed.         */
  int xrng;      /** X search for motion correction.    */
  int yrng;      /** Y search for motion correction.    */
  int wx;        /** X search for "tearing" correction. */
  double smax;   /** Max amount to boost contrast.      */
  double maxfg;  /** Maximum foreground fraction.       */
  int rpt;

  //= Background model maintenance parameter set.
  // includes  bgmv, fat, bcnt, sparkle, still, stable, wait, and bmix
  jhcParam bps;  
  int bgmv;      /** Motion threshold.                   */
  int fat;       /** Motion expansion size.              */
  int bcnt;      /** Frames needed for noise estimation. */
  int sparkle;   /** Persistent motion block level.      */
  int still;     /** Background quiescience (frames).    */
  int stable;    /** Heal after stable (frames).         */
  int wait;      /** Update interval (frames).           */
  double bmix;   /** Update mixing coefficient.          */

  //= Salience computation parameter set.
  // includes rng, mf, ef, cf, bf, and df
  jhcParam sps;  
  double rng;    /** Salience range (in stdevs).     */
  double mf;     /** Motion weighting.               */
  double ef;     /** Edge change weighting.          */
  double cf;     /** Color change weighting.         */
  double bf;     /** Maximum highlight attenutation. */
  double df;     /** Maximum shadow boost.           */

  //= Foreground morphology parameter set.
  // includes pass, bd, mfill, mtrim, cvx, hfrac, afrac, and amin
  jhcParam mps;  
  int pass;      /** Salience threshold.            */
  int bd;        /** Image border removal (pixels). */
  int mfill;     /** Mask gap closure size.         */
  int mtrim;     /** Mask shrinking size.           */
  int cvx;       /** Convexify gap width.           */
  double hfrac;  /** Fillable hole fraction.        */
  double afrac;  /** Smallest object wrt biggest.   */
  double amin;   /** Minimum area as 1D fraction.   */


public:
  ~jhcBgSub ();
  jhcBgSub ();

  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  void SetSize (jhcImg& ref, int force_mono =0);
  void SetSize (int w, int h, int f =3, int force_mono =0);
  int XDim () {return bg.XDim();}    /** Internal image width.  */
  int YDim () {return bg.YDim();}    /** Internal image height. */
  int Fields () {return bg.YDim();}  /** Internal image depth.  */

  void Reset (int bgclr =0);
  int Status ();

  void SetBG (jhcImg& ref, int rest =0, int gest =0, int best =0);  
  int LoadBG (const char *fname);
  int SaveBG (const char *fname);
  int MergeBG (jhcImg& now);
  int ForceBG (jhcImg& fgmsk, jhcImg& now);

  int FindFG (jhcImg& now, jhcImg *fgmsk =NULL, int cc =0);
  int VetoAreas (jhcImg& keep);
  void VetoHeal (int reg_num =0);
  int HealType (int reg_num =1);

  int FullMask (jhcImg& fgmsk, int cc =0);
  int FullFG (jhcImg& dest, int sm =0, int rdef =0, int gdef =0, int bdef =255);
  int FullBG (jhcImg& bgcopy, int sm =0, int val =0);
  int FullSal (jhcImg& scores, int sm =0);
  int FullHeal (jhcImg& erased);
  int FullDelay (jhcImg& image);

  int ParseFG (jhcImg *now =NULL);
  int ObjectCnt ();
  int ObjectBox (jhcRoi& spec, int n =0);
  int ObjectBox (int *cx, int *cy, int *w, int *h, int n =0);
  int ObjectMask (jhcImg& carve, int n =0);

private:
  int fix_params (const char *fname =NULL);
  int fix2_params (const char *fname =NULL);
  int back_params (const char *fname =NULL);
  int sal_params (const char *fname =NULL);
  int mask_params (const char *fname =NULL);

  void swap_imgs (jhcImg **a, jhcImg **b)   /** Interchange two pointers. **/
    {jhcImg *swap = *a; *a = *b; *b = swap;};

  int check_repeat (jhcImg& now);
  void stretch (jhcImg& dest, jhcImg& src);
  void fix_ntsc (jhcImg& dest, jhcImg& src);
  int stabilize (jhcImg& src, int mot, int sync);

//  int mask_fg (jhcImg& now);
  int fix_input (jhcImg& now);
  int salience (jhcImg& dest, jhcImg& src, jhcImg& ref);
  int fg_motion (jhcImg& dest, jhcImg& src);
  void fg_texture (jhcImg& dest, jhcImg& src, jhcImg& ref);
  void fg_color (jhcImg& dest, jhcImg& src, jhcImg& ref);
  int fix_shadows (jhcImg& sfix, jhcImg& src, jhcImg& ref);
  int clean_mask (jhcImg& dest, jhcImg& src, int th =128);

  int update_bg_0 (jhcImg& fgmsk);
  void update_bg_1 ();
  int bg_flush (jhcImg& fgmsk);
  void bg_build (jhcImg& fgmsk);
  void bg_heal ();
  void bg_push ();
  void bg_smooth ();

};


#endif




