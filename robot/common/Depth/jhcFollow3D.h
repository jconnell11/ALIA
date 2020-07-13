// jhcFollow3D.h : analyzes depth data to find person's waist
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2013 IBM Corporation
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

#ifndef _JHCFOLLOW3D_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFOLLOW3D_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"          // common video
#include "Data/jhcParam.h"
#include "Data/jhcBlob.h"
#include "Processing/jhcTools.h"

#include "Depth/jhcSurface3D.h"  // common robot


//= Analyzes depth data to find person's waist.

class jhcFollow3D 
{
friend class CThursdayDoc;  // for debugging

// PRIVATE MEMBER VARIABLES
private:
  // image processing
  jhcTools t;
  jhcSurface3D *sf;
  jhcImg tmp;

  // person finding data
  jhcBlob dudes;
  jhcImg wproj, wtmp, wcc;
  double tx, ty, azm, dist, look;
  int targ;


// PUBLIC MEMBER VARIABLES
public:
  // person map parameters
  jhcParam mps;
  int wsm, wth;
  double wht, wsz, wfront, wside, wpp;

  // person finding parameters
  jhcParam wps;
  int wdrop;
  double wmin, wmax, wfat, wthin, wnear;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcFollow3D ();
  ~jhcFollow3D ();
  void Bind (jhcSurface3D *surf) {sf = surf;}
  void Reset ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // read-only variables
  int LeaderWid () const     {return wproj.XDim();}
  int LeaderHt () const      {return wproj.YDim();}
  int LeaderMode () const    {return targ;}
  double LeaderDist () const {return dist;}
  double LeaderAzm () const  {return azm;}
  double LeaderX () const    {return tx;}
  double LeaderY () const    {return ty;}

  // main functions
  void ClrLeader ();
  int FindLeader (double gaze =0.0, double dinit =48.0);
  void AdjLeader (double dx, double dy, double da);

  // debugging functions
  int ProjLeader (jhcImg& dest, double foff =30.0, double dinit =48.0);
  int TagLeader (jhcImg& dest, const jhcImg& src);


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int map_params (const char *fname);
  int waist_params (const char *fname);

  // main functions
  void leader_blobs ();
  int pick_leader (double dinit);

};


#endif  // once




