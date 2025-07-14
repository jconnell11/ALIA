// jhcGenMic.h : read speech direction from some audio sensor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2025 Etaoin Systems
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

#pragma once

#include "jhcGlobal.h"

#include "Data/jhcParam.h"             // common video

#include "Geometry/jhcMatrix.h"        // common robot


//= Reads speech direction from some audio sensor.

class jhcGenMic
{
// PRIVATE MEMBER VARIABLES
private:
  // pose as vectors
  jhcMatrix loc, axis;

  // geometric calibration
  double x0, y0, z0, pan, tilt;
  int mport, light;

  // beam matching
  double oth, ath, dth;


// PROTECTED MEMBER VARIABLES
protected:
  double beam;               // most recent sound dir (-180 to +180)
  double slow;               // smoothed estimate (-180 to +180)
  double talk;               // angle for last recognition (-180 to +180)
  int audio;                 // cycle of sound or silence (negative)
  int spcnt;                 // speech recognition count (cycles)
  int mok;                   // overall status


// PUBLIC MEMBER VARIABLES
public:
  jhcParam ops, gps;
  int unit;                  // id if multiple mics used


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcGenMic ();
  ~jhcGenMic ();
  void UsePort (int n) {mport = n;}
  int Port () const    {return mport;}
  int CommOK () const  {return mok;}
  int LED () const     {return light;}
  double Pan () const  {return pan;}
  double Tilt () const {return tilt;}
  const jhcMatrix& Location () const {return loc;}

  // parameter utilities
  void SetGeom (double x, double y, double z, double p =0.0, double t =0.0, int n =8, int i =0);
  virtual void CopyVals (const jhcGenMic& ref);

  // processing parameter manipulation 
  virtual int Defaults (const char *fname =NULL, int geom =1);
  virtual int LoadCfg (const char *fname =NULL);
  virtual int SaveVals (const char *fname, int geom =1) const;
  virtual int SaveCfg (const char *fname) const;

  // main functions
  virtual int Reset ();
  virtual int Update (int voice =0);

  // processed directions
  int Sound () const {return audio;}
  double BeamDir () const    {return beam;}
  double SmoothDir () const  {return slow;}
  double VoiceDir () const   {return talk;}
  double Dir (int src) const {return((src <= 0) ? beam : ((src == 1) ? slow : talk));}
  bool NewVoice () const  {return(spcnt == 1);}
  int VoiceStale () const {return((spcnt < 0) ? -spcnt : __max(0, spcnt));}

  // head matching
  double ClosestPt (jhcMatrix *pt, const jhcMatrix& ref, int src =0, int chk =1) const;
  double OffsetAng (const jhcMatrix& ref, double aim =0.0) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int off_params (const char *fname);
  int geom_params (const char *fname);

};

