// jhcHeadGaze.h : computes direction using offset of face from head center
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

#ifndef _JHCHEADGAZE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCHEADGAZE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"          // common video
#include "Data/jhcParam.h"
#include "Face/jhcFrontal.h"

#include "People/jhcStare3D.h"    // common robot


//= Computes direction using offset of face from head center.
// derived from jhcStare3D so can add gaze to list of jhcBodyData
// <pre>
// class tree and parameters:
//
//   HeadGaze          zps vps
//     Frontal         dps
//       +FFindOCV     fps
//         FFind
//     >Stare3D        ips
//       Track3D       tps tps2
//         Parse3D     bps hps sps aps gps eps
//       Overhead3D    cps[] rps[] mps
//         Surface3D
//           PlaneEst
//
// </pre>

class jhcHeadGaze : public jhcFrontal
{
// PRIVATE MEMBER VARIABLES
private:
  int gcnt[pmax];


// PROTECTED MEMBER VARIABLES
protected:
  // person finder
  jhcStare3D *s3; 


// PUBLIC MEMBER VARIABLES
public:
  // parameters for self position
  jhcParam zps;
  double xme, yme, zme;

  // parameters for gaze interpretation
  jhcParam vps;
  double hadj, dadj, diam, fwid, ptol, ttol;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcHeadGaze ();
  jhcHeadGaze ();
  void Bind (jhcStare3D *stare);

  // parameter utilities
  void SetGaze (double dz, double dr, double s, double fw, double pt, double tt)
    {hadj = dz; dadj = dr; diam = s; fwid = fw; ptol = pt; ttol = tt;}
  void SetAttn (double x, double y, double z)
    {xme = x; yme = y; zme = z;}
  void SetAttn (const jhcMatrix& pos) 
    {xme = pos.X(); yme = pos.Y(); zme = pos.Z();}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname, int geom =0) const;
  int SaveCfg (const char *fname) const;

  // main functions
  void Reset ();
  void ScanRGB (const jhcImg& src, const jhcImg& d16, int cam =0, int trk =1);
  void DoneRGB (int trk =1); 
  int GazeMax () const;
  int AnyGaze (int th =1) const {return((GazeMax() >= th) ? 1 : 0);}
  int GazeID (int id, int trk =1) const;
  int GazeNew (int trk =1, int gmin =1) const;

  // convenience
  int GazeNewID (int trk =1, int gmin =1) const 
    {return((s3 == NULL) ? 0 : s3->PersonID(GazeNew(trk, gmin)));}
  int FrontNewID (int cam =0, int fmin =1) const
    {return((s3 == NULL) ? 0 : s3->PersonID(FrontNew(cam, fmin)));}

  // debugging graphics
  int AllGaze (jhcImg& dest, int trk =1) const;
  int GazeCam (jhcImg& dest, int i, int cam =0, int trk =1) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int attn_params (const char *fname);
  int gaze_params (const char *fname);

  // main functions
  void head_mid (jhcMatrix& mid, const jhcMatrix& head, int cam) const;
  int search_area (jhcRoi& probe, double& rot, const jhcMatrix& mid, const jhcImg& src) const;
  int face_pt (jhcMatrix& fc, double fx, double fy, const jhcImg& d16, double sc) const;
  void attn_hits (int trk);


};


#endif  // once




