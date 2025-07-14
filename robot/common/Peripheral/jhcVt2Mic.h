// jhcVt2Mic.h : sound direction from Acoustic Magic VT-2 array microphone
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2020 IBM Corporation
// Copyright 2021-2024 Etaoin Systems
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

#include "Data/jhcArr.h"               // common video
#include "Data/jhcParam.h"     
#include "Interface/jhcSerial.h"  

#include "Peripheral/jhcGenMic.h"


//= Reads sound direction from Acoustic Magic VT-2 array microphone.

class jhcVt2Mic : public jhcGenMic
{
// PRIVATE MEMBER VARIABLES
private:
  // serial port (robot needs plain jhcSerial)
  jhcSerial mcom;

  // basic direction filtering
  jhcArr ssm;
  double b0, b1, b2;

  // Gaussian mix speech direction
  double bavg, favg, bvar, fvar, bwt, fwt;
  int skip, fgnd;

  // sound direction voting
  jhcArr raw, snd;
  int pk, pk2;

  // Gaussian mixture parameters
  double msc, mix, zone, blend, istd, dlim; 
  int box, gcnt;


// PUBLIC MEMBER VARIABLES
public:
  jhcParam mps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcVt2Mic ();
  ~jhcVt2Mic ();

  // parameter utilities
  void CopyVals (const jhcGenMic& ref);

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL, int geom =1);
  int SaveVals (const char *fname, int geom =1) const;

  // main functions
  int Reset (int rpt =0);
  int Update (int voice =0);

  // read only variables
  double BlurtDir () const   {return favg;}
  bool Blurt () const        {return(fgnd > 0);}


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int gmix_params (const char *fname);

  // main functions
  void init_mix ();
  void update_mix (double val);

};

