// jhcFrontal.h : finds faces in head regions and checks if frontal
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#ifndef _JHCFRONTAL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFRONTAL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"           // common video
#include "Data/jhcParam.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcHist.h"
#include "Processing/jhcResize.h"
#include "Processing/jhcStats.h"


// define FFIND_DLL to use DLL instead of OpenCV 2.4.5 sources

#ifdef FFIND_DLL
  #include "Face/jhcFFindDLL.h"
#else
  #include "Face/jhcFFindOCV.h"
#endif


//= Finds faces in head regions and checks if frontal.
// designed to be used in conjunction with jhcStare3D people finder
// person numbers are indices into jhcStare3D array, not unique IDs
// useful for determining eye contact and vetting face reco images
// should be usable with other face finders besides OpenCV

class jhcFrontal : protected jhcDraw, private jhcHist, private jhcResize, protected jhcStats
{
// PROTECTED MEMBER VARIABLES
protected:
  static const int pmax = 50;    /** Maximum number of people entries. */
  static const int cmax = 8;     /** Maximum number of camera entries. */


// PRIVATE MEMBER VARIABLES
private:
  double cx[pmax][cmax];         /** X center of search patch.          */
  double cy[pmax][cmax];         /** Y center of search patch.          */
  double rot[pmax][cmax];        /** Image rotation of search patch.    */
  jhcImg crop[pmax][cmax];       /** Rotated patch searched for face.   */
  jhcRoi face[pmax][cmax];       /** Face detection wrt rotated patch.  */
  int tried[pmax][cmax];         /** Whether any image areas checked.   */ 
  int fcnt[pmax][cmax];          /** Consecutive frontal face count.    */


// PUBLIC MEMBER VARIABLES
public:
  // face finder processing module
#ifdef FFIND_DLL
  jhcFFindDLL ff;
#else
  jhcFFindOCV ff;                
#endif

  // debugging info
  double fdx[pmax][cmax];        /** Signed fractional X offset of face. */
  double fdy[pmax][cmax];        /** Signed fractional Y offset of face. */

  // parameters for frontal geometry
  jhcParam dps;
  double fsz, xoff, yoff, xsh, ysh;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcFrontal ();
  jhcFrontal ();

  // parameter utilities
  void SetFront (double sz, double xc, double yc, double dx, double dy)
    {fsz = sz; xoff = xc; yoff = yc; xsh = dx; ysh = dy;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  void Reset ();
  int FaceChk (int p, const jhcImg& src, const jhcRoi& area, double ang =0.0, int cam =0);
  int DoneChk ();

  // result browsing
  bool Checked (int p, int cam =0) const;
  bool Found (int p, int cam =0) const 
   {return Frontal(p, cam, 0);}
  bool Frontal (int p, int cam =0, int fmin =1) const;
  int FrontCnt (int p, int cam =-1) const;
  int FrontMax () const;
  int FrontNew (int cam =0, int fmin =1) const;
  int FrontBest (jhcRoi& area, int p, int fmin =1) const;
  int FaceMid (double& fx, double& fy, int p, int cam =0, double sc =1.0) const;

  // read-only access
  const jhcImg *GetCrop (int p, int cam =0) const
    {return((!Found(p, cam)) ? NULL : &(crop[p][cam]));}
  const jhcRoi *GetFace (int p, int cam =0) const 
    {return((!Found(p, cam)) ? NULL : &(face[p][cam]));}
  int GetSize (int p, int cam =0) const
    {return((!Found(p, cam)) ? 0 : (face[p][cam]).RoiW());}
  double GetAngle (int p, int cam =0) const
    {return((!Found(p, cam)) ? 0.0 : rot[p][cam]);}
  double ShiftX (int p, int cam =0) const
    {return((!Found(p, cam)) ? 0.0 : fdx[p][cam]);}
  double ShiftY (int p, int cam =0) const
    {return((!Found(p, cam)) ? 0.0 : fdy[p][cam]);}
  int PctX (int p, int cam =0) const 
    {return ROUND(100.0 * ShiftX(p, cam));}
  int PctY (int p, int cam =0) const 
    {return ROUND(100.0 * ShiftY(p, cam));}

  // debugging graphics
  int FacesCam (jhcImg& dest, int cam =0, int rev =0, double sc =1.0) const;
  int FaceProbe (jhcImg& dest, const jhcImg& src, int p, int cam =0, int rev =0) const;
  int ProbePose (double& x, double& y, double& degs, int p, int cam =0, double sc =1.0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int front_params (const char *fname);

  // result browsing
  bool ok_idx (int p, int cam) const;


};


#endif  // once




