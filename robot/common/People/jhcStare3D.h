// jhcStare3D.h : finds and tracks people using a multiple fixed sensors
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2020 IBM Corporation
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

#ifndef _JHCSTARE3D_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSTARE3D_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"          // common video
#include "Data/jhcParam.h"

#include "Depth/jhcOverhead3D.h"   // common robot
#include "People/jhcTrack3D.h"


//= Finds and tracks people using a multiple fixed sensors.
// handles fusion of sensors into a unified overhead view
// links sensors to detection and tracking part
// <pre>
// class tree and parameters:
//
//   Stare3D         
//     Track3D       tps tps2
//       Parse3D     bps hps sps aps gps eps
//     Overhead3D    cps[] rps[] mps
//       Surface3D
//         PlaneEst
//
// </pre>

class jhcStare3D : public jhcTrack3D, public jhcOverhead3D
{
// PRIVATE MEMBER VARIABLES
private:
  jhcMatrix w2b;


// PUBLIC MEMBER VARIABLES
public:
  // depth map interpolation for background thread
  int sm_bg, pmin_bg;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcStare3D ();
  ~jhcStare3D ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname, int geom =0) const;

  // main functions
  void Reset (double ftime =0.033);
  int Analyze (int sm =7, int pmin =10);
  int CntValid (int trk =1) const;
  int PersonLim (int trk =1) const;
  bool PersonOK (int i, int trk =1) const;
  int PersonID (int i, int trk =1) const;
  bool Named (int i, int trk =1) const;
  const jhcBodyData *GetPerson (int i, int trk =1) const;
  jhcBodyData *RefPerson (int i, int trk =1);
  const jhcBodyData *GetID (int id, int trk =1) const;
  jhcBodyData *RefID (int id, int trk =1);
  int TrackIndex (int id, int trk =1) const;
  int PersonTouch (double wx, double wy, int trk =1);

  // useful analysis data
  int Closest (int trk =1) const;
  int Head (jhcMatrix& full, int i, int trk =1) const;
  double Height (int i, int trk =1) const;
  int Hand (jhcMatrix& full, int i, int rt =1, int trk =1) const;
  double HandOver (int i, int rt =1, int trk =1) const;
  int Target (jhcMatrix& full, int i, int rt =1, int trk =1, double zlev =0.0) const;
  int TargetY (jhcMatrix& full, int i, int rt =1, int trk =1, double yoff =0.0) const;
  int TargetX (jhcMatrix& full, int i, int rt =1, int trk =1, double xoff =0.0) const;
  int HeadBoxCam (jhcRoi& box, int i, int cam =0, int trk =1, double sc =1.0);
  const char *GetName (int id, int trk =1) const;
  int SetName (int id, const char *name, int trk =1);
  const void *GetNode (int id, int trk =1) const;
  int SetNode (void *n, int id, int trk =1);  
  int NodeID (const void *node, int trk =1) const;

  // debugging graphics
  int AllHeads (jhcImg& dest, int trk =1, int invert =0, double sz =8.0, int style =2)
    {return((trk > 0) ? TrackedMark(dest, invert, sz, style) : RawMark(dest, invert, sz));}
  int AllHands (jhcImg& dest, int trk =1, int invert =0) const
    {return((trk > 0) ? TrackedHands(dest, invert) : RawHands(dest, invert));}
  int AllRays (jhcImg& dest, int trk =1, int invert =0, double zlev =0.0, int pt =3) const
    {return((trk > 0) ? TrackedRays(dest, invert, zlev, pt) : RawRays(dest, invert, zlev));}
  int AllRaysY (jhcImg& dest, int trk =1, int invert =0, double yoff =0.0, int pt =3) const
    {return((trk > 0) ? TrackedRaysY(dest, invert, yoff, pt) : RawRaysY(dest, invert, yoff));}
  int AllRaysX (jhcImg& dest, int trk =1, int invert =0, double xoff =0.0, int pt =3) const
    {return((trk > 0) ? TrackedRaysX(dest, invert, xoff, pt) : RawRaysX(dest, invert, xoff));}
  int ShowID (jhcImg& dest, int id, int trk =1, int invert =0, int col =6, double sz =8.0, int style =2);
  int ShowIDCam (jhcImg& dest, int id, int cam =0, int trk =1, int rev =0, int col =6, double sz =8.0, int style =2);
  int HeadsCam (jhcImg& dest, int cam =0, int trk =1, int rev =0, double sz =8.0, int style =2);  
  int PersonCam (jhcImg& dest, int i, int cam =0, int trk =1, int rev =0, int col =5, double sz =8.0, int style =2);
  int HandsCam (jhcImg& dest, int cam =0, int trk =1, int rev =0, double sz =4.0);  
  int RaysCam (jhcImg& dest, int cam =0, int trk =1, int rev =0, double zlev =0.0);  
  int RaysCamY (jhcImg& dest, int cam =0, int trk =1, int rev =0, double yoff =0.0);  
  int RaysCamX (jhcImg& dest, int cam =0, int trk =1, int rev =0, double xoff =0.0);  


// PRIVATE MEMBER FUNCTIONS
private:


};


#endif  // once




