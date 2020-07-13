// jhcObstacle3D.h : analyzes depth data for local obstacle map
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2014 IBM Corporation
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

#ifndef _JHCOBSTACLE3D_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCOBSTACLE3D_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"          // common video
#include "Data/jhcParam.h"
#include "Processing/jhcTools.h"

#include "Depth/jhcSurface3D.h"  // common robot


//= Analyzes depth data for local obstacle map.
// white is traversable, black is obstacle, gray is unknown

class jhcObstacle3D 
{
friend class CThursdayDoc;  // for debugging

// PRIVATE MEMBER VARIABLES
private:
  // image processing
  jhcTools t;
  jhcSurface3D *sf;
  jhcImg tmp;

  // local occupancy
  jhcRoi bot;
  jhcArr dirs;
  jhcImg floor, favg, fobst, fsp, fprev, fbin, fdist, fmv, fcol;
  double fcx, fcy;
  int phase;


// PUBLIC MEMBER VARIABLES
public:
  // occupancy map parameters
  jhcParam ops;
  int fclr, finc, fdec;
  double ffront, fback, fside, fpp, fz; 

  // integrated freespace map parameters
  jhcParam bps;
  double rarm, rfront, rmid, rback, rwide, hdrm, flank;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcObstacle3D ();
  ~jhcObstacle3D ();
  void Bind (jhcSurface3D *surf) {sf = surf;}
  void Reset ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  void BuildFree (double dx =0.0, double dy =0.0, double da =0.0, double ht =48.0);
  int OverlayBot (jhcImg& dest, const jhcImg& src, int body =1);
  int MarkFree (jhcImg& dest, const jhcImg& src, int dok =8, int dbad =8);
  int MarkDrive (jhcImg& dest, const jhcImg& src);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int bot_params (const char *fname);
  int occ_params (const char *fname);

  // main functions
  void mark_obst (jhcImg& dest, const jhcImg& src) const;
  void update_map (jhcImg& dest, const jhcImg& src) const;

};


#endif  // once




