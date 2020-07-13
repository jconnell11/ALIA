// jhcAccelXY.h : interprets body accelometer data from onboard PIC
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 IBM Corporation
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

#ifndef _JHCACCELXY_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCACCELXY_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"
#include "Peripheral/jhcDynamixel.h"


//= Interprets body accelometer data from onboard PIC.
// works with code version dyna_104.asm, uses X and Y axes only

class jhcAccelXY
{
// PRIVATE MEMBER VARIABLES
private:
  jhcDynamixel *dyn;                   /** Dynamixel serial controller. */
  int aok;                             /** Communications status.       */
  double tilt, roll, tip, mag, ang;    // cached instantaneous results


// PUBLIC MEMBER VARIABLES
public:
  // parameters
  jhcParam aps;
  int x0, y0;
  double mgx, mgy;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAccelXY ();
  jhcAccelXY ();

  // configuration
  void Bind (jhcDynamixel *ctrl);
  int CommOK () const {return aok;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int LoadCfg (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SaveCfg (const char *fname) const;

  // read only variables
  double BaseTilt () const  {return tilt;}
  double BaseRoll () const  {return roll;}
  double BaseTip () const   {return tip;}
  double BaseShock () const {return mag;}
  double BaseDir () const   {return ang;}

  // main functions
  int Update (int chk =0);


// PRIVATE MEMBER FUNCTIONS
private:
  int clr_vals ();

  // processing parameters
  int acc_params (const char *fname);

};


#endif  // once




