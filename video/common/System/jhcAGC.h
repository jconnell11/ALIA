// jhcAGC.h : corrects for drifting camera parameters
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

#ifndef _JHCAGC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCAGC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"
#include "Data/jhcArr.h"
#include "Data/jhcKnob.h"
#include "Data/jhcParam.h"

#include "Processing/jhcALU.h"
#include "Processing/jhcHist.h"
#include "Processing/jhcLUT.h"
#include "Processing/jhcResize.h"
#include "Processing/jhcStats.h"
#include "Processing/jhcThresh.h"


//= Corrects for drifting camera parameters.
// cycle of FixAGC, LimitAGC, EstGains on 320x240x3 runs at 112fps on P3-650
// noise estimation included because it can also run at reduced resolution

class jhcAGC 
{
// essentially base classes, but this lets jhcAGC be used with jhcTools
private:
  jhcALU al;
  jhcHist jh;
  jhcLUT lu;
  jhcResize sz;
  jhcStats st;
  jhcThresh th;

// internal temporary variables and persistent state
protected:
  jhcImg tmp, ctmp, gref, gate;
  jhcImg cmid, rats;
  jhcArr rh, gh, bh;
  int iclip, cclip, adj, ref_mode, gcnt, gbad;
  jhcKnob rn, gn, bn;
  double g[4];
  char nmsg[40], gmsg[40];

// settable control parameters
public:
  int gwait;     /** How many pegged AGC/AWB frames until reset.        */
  int gsamp;     /** How often to mix new image into AGC/AWB reference. */
  double bmix;   /** Mixing rate of new images into AGC/AWB reference.  */

  //= Gain adjustment parameter set. 
  // includes agc0, agc1, awb1, gmix, gfra, ilo, ihi, and hagc
  jhcParam gps;  
  double agc0;   /** Max intensity cut.             */
  double agc1;   /** Max intensity boost.           */
  double awb1;   /** Max channel boost/cut.         */
  double gmix;   /** Gain update move fraction.     */
  double gfrac;  /** Min histogram fraction.        */
  int ilo;       /** Min valid intensity.           */
  int ihi;       /** Max valid intensity.           */
  int hagc;      /** Desired internal image height. */

  //= Noise estimation parameter set.
  // includes ndrop, nfrac, and nsm
  jhcParam nps;   
  double ndrop;  /** Histogram drop fraction.        */
  double nfrac;  /** Minimum histogram fraction.     */
  int nsm;       /** Histogram smoothing.            */


// readable internal state values
public:
  int XDimAGC () {return cmid.XDim();}     /** Internal image width.  */
  int YDimAGC () {return cmid.YDim();}     /** Internal image height. */

  double DGainR () {return(g[3]);}         /** Differential gain for red.      */
  double DGainG () {return(g[2]);}         /** Differential gain for green.    */
  double DGainB () {return(g[1]);}         /** Differential gain for blue.     */
  double GainI ()  {return(g[0]);}         /** Overall intensity gain.         */
  double GainR ()  {return(g[3] * g[0]);}  /** Overall gain for red channel.   */
  double GainG ()  {return(g[2] * g[0]);}  /** Overall gain for green channel. */
  double GainB ()  {return(g[1] * g[0]);}  /** Overall gain for blue channel.  */

  int iNoiseR ()   {return(rn.Ival());}    /** Red channel integer noise.    */
  int iNoiseG ()   {return(gn.Ival());}    /** Green channel integer noise.  */
  int iNoiseB ()   {return(bn.Ival());}    /** Blue channel integer noise.   */
  double NoiseR () {return rn.val;}        /** Red channel noise estimate.   */
  double NoiseG () {return gn.val;}        /** Green channel noise estimate. */
  double NoiseB () {return bn.val;}        /** Blue channel noise estimate.  */
  double QuietR () {return(rn.Recip());}   /** Reciprocal of red noise.      */
  double QuietG () {return(gn.Recip());}   /** Reciprocal of green noise.    */
  double QuietB () {return(bn.Recip());}   /** Reciprocal of blue noise.     */

  const char *SizeTxtAGC () {return cmid.SizeTxt();}  /** Size of internal images. */
  const char *GainTxt (int dec3 =0);
  const char *NoiseTxt ();


// main operations
public:
  jhcAGC ();

  int DefaultsAGC (const char *fname =NULL);
  int SaveValsAGC (const char *fname) const;

  void SetSizeAGC (jhcImg& ref);
  void SetSizeAGC (int w, int h, int f);
  void SetGainRef (jhcImg& truth);
  void UpdateRef (jhcImg& current);
  void SetGainSize (jhcImg& ref);

  void ResetAGC ();
  void ResetNoise (int r =0, int g =0, int b =0);
  void NoiseDefaults (int rest, int gest, int best); 
  void ChannelWts (double *rf, double *gf, double *bf);
  void ResetGains ();
  void DecayGains ();
  int GainStatus ();

  int FixAGC (jhcImg& dest, jhcImg& src);
  int LimitAGC (jhcImg& dest, jhcImg& src);
  int EstGains (jhcImg& now, jhcImg *ref =NULL, jhcImg *mask =NULL, int mok =1);
  int EstNoise (jhcImg& now, jhcImg *ref =NULL, jhcImg *mask =NULL, int mok =1);
  int UpdateAGC (jhcImg& now, jhcImg *ref =NULL, jhcImg *mask =NULL, 
                 int nskip =0, int gskip =0, int mok =1);
  void ForceColor (jhcImg& src, jhcRoi& area, int r, int g, int b);

protected:
  int gain_params (const char *fname =NULL);
  int noise_params (const char *fname =NULL);

  void NoiseCopy ();

  int gains_ok ();
  int EstGains0 (jhcImg& f, jhcImg& b, jhcImg *m);
  void SetGainsRGB (double estr, double estg, double estb);
  void SetGainMono (double gest);
  void ClipGain (double val);

  int EstNoise0 (jhcImg& f, jhcImg& b, jhcImg *m, int fix =1);

};


#endif




