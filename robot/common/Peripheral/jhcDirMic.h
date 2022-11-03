// jhcDirMic.h : sound direction from Acoustic Magic VT-2 array microphone
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2020 IBM Corporation
// Copyright 2021-2022 Etaoin Systems
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

#ifndef _JHCDIRMIC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCDIRMIC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"               // common video
#include "Data/jhcParam.h"     
#include "Interface/jhcSerial.h"  

#include "Geometry/jhcMatrix.h"        // common robot


//= Reads sound direction from Acoustic Magic VT-2 array microphone.

class jhcDirMic
{
// PRIVATE MEMBER VARIABLES
private:
  // basic direction filtering
  jhcArr ssm;
  double b0, b1, b2, beam, slow;
  int mok;

  // Gaussian mix speech direction
  double bavg, favg, bvar, fvar, bwt, fwt, talk;
  int skip, fgnd, spcnt;


// PUBLIC MEMBER VARIABLES
public:
  // serial port (robot needs plain jhcSerial)
  jhcSerial mcom;
  int unit;

  // pose as vectors
  jhcMatrix loc, axis;

  // sound direction voting
  jhcArr raw, snd;
  int cnt, pk, pk2;

  // interpretation parameters
  jhcParam aps;
  double mix, msc, oth, ath, dth;
  int box;

  // Gaussian mixture parameters
  jhcParam mps;
  double zone, blend, istd, dlim; 
  int gcnt;

  // geometric calibration
  jhcParam gps;
  double mcal, x0, y0, z0, pan, tilt;
  int mport, light;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcDirMic ();
  ~jhcDirMic ();
  int CommOK (int bad =0) const {return mok;}
  int Reset (int rpt =0);

  // parameter utilities
  void SetGeom (double x, double y, double z, double p =0.0, double t =0.0, int n =8, int i =0);
  void CopyVals (const jhcDirMic& ref);

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL, int geom =1);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname, int geom =1) const;
  int SaveCfg (const char *fname) const;

  // read only variables
  double BeamDir () const    {return beam;}
  double SmoothDir () const  {return slow;}
  double BlurtDir () const   {return favg;}
  double VoiceDir () const   {return talk;}
  double Dir (int src) const {return((src <= 0) ? beam : ((src == 1) ? slow : talk));}
  int VoiceStale () const    {return((spcnt < 0) ? -spcnt : __max(0, spcnt));}
  bool NewVoice () const     {return(spcnt == 0);}
  bool Blurt () const        {return(fgnd > 0);}

  // main functions
  int Update (int voice =0);
  double ClosestPt (jhcMatrix *pt, const jhcMatrix& ref, int src =0, int chk =1) const;
  double OffsetAng (const jhcMatrix& ref, double aim =0.0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int ang_params (const char *fname);
  int mix_params (const char *fname);
  int geom_params (const char *fname);

  // main functions
  void init_mix ();
  void update_mix (double val);


};


#endif  // once




