// jhcCeliaBody.h : controls basic I/O of Celia smart room
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 IBM Corporation
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

#ifndef _JHCCELIABODY_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCCELIABODY_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <math.h>

#include "Data/jhcImg.h"                   // common video
#include "Data/jhcParam.h"
#include "Processing/jhcLUT.h"
#include "Processing/jhcResize.h"
#include "Video/jhcVideoSrc.h"

#include "Body/jhcCeliaNeck.h"             // common robot


//= Controls basic I/O of Celia smart room (Kinect, neck, laser).
// adds video images management to neck motions
// takes pointer to jhcVideoSrc to allow external main loop

class jhcCeliaBody : public jhcCeliaNeck, private jhcLUT, private jhcResize
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImg col, rng, col2;  // images from Kinect sensor
  jhcRoi dbox;
  DWORD tprev;
  int cw, ch, iw, ih, kin, tstep;


// PUBLIC MEMBER VARIABLES
public:
  // Kinect (or video)
  jhcVideoSrc *vid;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcCeliaBody ();
  ~jhcCeliaBody ();
  void BindVideo (jhcVideoSrc *v);
  int SetKinect (int noisy =1);
  int Reset (int motors =1, int noisy =0);
  int VideoOK () const {if ((vid == NULL) || !vid->Valid()) return 0; return 1;}

  // read-only access to camera parameters
  int ColW () const {return cw;}
  int ColH () const {return ch;}
  int XDim () const {return iw;}
  int YDim () const {return ih;}
  double MidX () const {return(0.5 * (iw - 1));}
  double MidY () const {return(0.5 * (ih - 1));}
  double ColMidX () const {return(0.5 * (cw - 1));}
  double ColMidY () const {return(0.5 * (ch - 1));}
  double ColScale () const {return(cw / (double) iw);}
  void BigSize (jhcImg& dest) const {dest.SetSize(cw, ch, 3);}
  void SmallSize (jhcImg& dest) const {dest.SetSize(iw, ih, 3);}
  void DepthSize (jhcImg& dest) const {dest.SetSize(iw, ih, 1);}
  const jhcRoi &DepthArea () const {return dbox;}
  int FrameMS () const {return tstep;}
  double FrameTime () const {return(0.001 * tstep);}

  // access to Kinect images
  const jhcImg& Color () const {return col;}  /** Returns native resolution RGB image. */
  const jhcImg& Range () const {return rng;}  /** Returns native (8 or 16) depth map.  */
  int ImgSmall (jhcImg& dest);
  int ImgBig (jhcImg& dest); 
  int Depth8 (jhcImg& dest) const; 
  int Depth16 (jhcImg& dest) const; 

  // main functions
  int UpdateImgs ();
  int UpdatePose () {return NeckUpdate();}
  int Update ();
  int Issue ();


// PRIVATE MEMBER FUNCTIONS
private:
  void chk_vid ();


};


#endif  // once




