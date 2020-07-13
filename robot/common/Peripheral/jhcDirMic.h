// jhcDirMic.h : sound direction from Acoustic Magic VT-2 array microphone
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2020 IBM Corporation
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
  jhcArr ssm;
  int mok, spcnt;
  double beam, slow, talk;


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
  jhcParam mps;
  double mix, oth, ath, dth, msc;
  int box, spej;

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

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL, int geom =1);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname, int geom =1) const;
  int SaveCfg (const char *fname) const;
  void CopyVals (const jhcDirMic& ref);

  // read only variables
  double BeamDir () const    {return beam;}
  double SmoothDir () const  {return slow;}
  double VoiceDir () const   {return talk;}
  double Dir (int src) const {return((src <= 0) ? beam : ((src == 1) ? slow : talk));}
  int VoiceStale () const    {return((spcnt < 0) ? -spcnt : __max(0, spcnt - spej));}
  bool NewVoice () const     {return(spcnt == spej);}

  // main functions
  int Update (int voice =0);
  double ClosestPt (jhcMatrix *pt, const jhcMatrix& ref, int src =0, int chk =1) const;
  double OffsetAng (const jhcMatrix& ref, int src =0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  int geom_params (const char *fname);
  int mic_params (const char *fname);

};


#endif  // once




