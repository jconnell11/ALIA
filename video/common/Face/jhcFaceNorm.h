// jhcFaceNorm.h : generates canonical face image for recognition
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2015 IBM Corporation
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

#ifndef _JHCFACENORM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFACENORM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Data/jhcParam.h"
#include "Data/jhcRoi.h"
#include "Processing/jhcArea.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcGray.h"
#include "Processing/jhcHist.h"
#include "Processing/jhcResize.h"
#include "Processing/jhcStats.h"
#include "Processing/jhcThresh.h"


//= Generates canonical face image for recognition.
// implements normalization functions found in "freco.h"
// eye finding adapted from Visual Push To Talk (VPTT)

class jhcFaceNorm : protected jhcResize, protected jhcDraw, 
                    private jhcArea, private jhcGray, private jhcHist, 
                    private jhcStats, private jhcThresh 
{
friend class CCeilingDoc;              // for debugging
friend class CWaldoDoc;              


// PRIVATE MEMBER VARIABLES
private:
  jhcImg face, eyes, mono, hbar, ebin, icol;
  jhcRoi band, efind, halo, pod;
  double xc, yc, xmid, ymid, tip, roll, mag;
  double lex, ley, rex, rey;
  int eok;


// PUBLIC MEMBER VARIABLES
public:
  // grayscale intermediate parameters
  jhcParam nps;
  int gw, mode, fix_pos, fix_sc, fix_ang;
  double wexp, hexp, enh;

  // eye filter parameters
  jhcParam eps;                         
  int bc, bs, bw;
  double ewf, ehf, eup, bg, th2;

  // output icon parameters
  jhcParam ips;
  int back, mode2, iw, ih;
  double eln, efrac, esep;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcFaceNorm ();
  ~jhcFaceNorm ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  void SetSizes ();

  // normalization functions
  int freco_wid () const {return iw;}
  int freco_ht () const  {return ih;}
  int freco_norm (unsigned char *gray, 
                  const unsigned char *img, int w, int h, int f, 
                  int rx, int ry, int rw, int rh, int raw =0);

  // eye location functions
  int freco_eye_lf (double& x, double& y) const {x = lex; y = ley; return eok;};
  int freco_eye_rt (double& x, double& y) const {x = rex; y = rey; return eok;};

  // debugging graphics
  int EyeBounds (jhcImg& dest) const;


// PRIVATE MEMBER FUNCTIONS
private:
  int norm_params (const char *fname);
  int eye_params (const char *fname);
  int icon_params (const char *fname);

  // normalization functions
  void face_area (jhcImg& dest, const jhcImg& src, const jhcRoi& fdet);
  void find_eyes (double& ang, jhcRoi& ebox, const jhcImg& src);
  void build_icon (jhcImg& dest, const jhcImg& src, double ang, const jhcRoi& ebox, int raw);
  int chk_eyes (double& lx, double& ly, double& rx, double& ry) const;


};


#endif  // once




