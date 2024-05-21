// jhcSwapArm.h : control interface for external robot arm
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Etaoin Systems
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

#include <math.h>

#include "Data/jhcParam.h"             // common video

#include "Geometry/jhcMatrix.h"        // common robot

#include "Body/jhcGenArm.h"            


//= Control interface for external robot arm.
// no actual control code -- merely exchanges variable values

class jhcSwapArm : public jhcGenArm
{
// PRIVATE MEMBER VARIABLES
private:
  // hand sensor data
  double w0, sqz;            // finger separation & force

  // pose sensor data
  double terr;               // offset from tucked pose

  // arm speed estimate
  UL32 now;                  // last update time
  double iarm;               // smoothed hand motion speed
  int parked;                // whether hand is not moving
  
  // hand command info
  double wstop;              // desired finger width (or force)
  double wrate;              // desired finger motion speed
  int wlock;                 // current command importance

  // tuck command info
  double trate;              // desired angular stowing speed
  int tlock;                 // current command importance

  // xyz command info
  jhcMatrix pdes, ddes;      // desired position & orientation
  double prate, drate;       // desired arm motion speeds
  int pmode, dmode;          // goal coordinate importance
  int plock, dlock;          // current command importance


// PUBLIC MEMBER VARIABLES
public:
  // arm stowing position
  jhcParam sps;              
  double rgap, rets, rete;

  // max finger opening
  double wmax;               

  // hardware status
  int aok;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSwapArm ();
  jhcSwapArm ();
  int CommOK () {return aok;}
  double MaxWidth () const {return wmax;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL) {return stow_params(fname);}
  int SaveVals (const char *fname) const {return sps.SaveVals(fname);}

  // configuration
  int Reset (int rpt =0);

  // core interaction
  int Status (float x, float y, float z, float p, float t, float r, float w, float f, float e);
  int PosCmd (float& x, float& y, float& z, float& vel, int& mode, int& bid);
  int DirCmd (float& pan, float& tilt, float& roll, float& vel, int& mode, int& bid);
  int AuxCmd (float& wf, float& wvel, float& svel, int& wbid, int& sbid);              

  // ---------------------- HAND MAIN ----------------------------

  // current hand information
  double Width () const   {return w0;}
  double Squeeze () const {return sqz;}
  double SqueezeGoal () const {return __max(0.0, -wstop);}

  // hand goal specification commands
  int WidthTarget (double sep, double rate =1.0, int bid =10)
    {return HandTarget(sep, rate, bid);}
  int SqueezeTarget (double force, int bid =10)
    {return HandTarget(-force, 1.0, bid);}
  int HandTarget (double wf, double rate =1.0, int bid =10);

  // hand motion progress
  double WidthErr (double sep) const {return fabs(sep - w0);}
  double SqueezeErr (double f) const {return(sqz - f);}

  // ---------------------- ARM MAIN ----------------------------

  // current arm information
  double ReachRate () const {return prate;}
  int Static () const {return parked;}

  // arm Cartesian goal specification commands
  int ArmTarget (const jhcMatrix& pos, const jhcMatrix& dir, double p_rate =1.0, double d_rate =0.0, int bid =10);
  int PosTarget (const jhcMatrix& pos, double rate =1.0, int bid =10, int mode =0x0);
  int PosTarget (double ax, double ay, double az, double rate =1.0, int bid =10, int mode =0x0);
  int PosTarget3D (const jhcMatrix& pos, double ht, double rate =1.0, int bid =10, int mode =0x0)
    {return PosTarget(pos.X(), pos.Y(), pos.Z() - ht, rate, bid, mode);}
  int DirTarget (const jhcMatrix& dir, double rate =1.0, int bid =10, int mode =0x0);
  int DirTarget (double hp, double ht, double hr, double rate =1.0, int bid =10, int mode =0x0);
  int Tuck (double rate =1.0, int bid =10);

  // arm motion progress
  double PosErr3D (jhcMatrix& perr, const jhcMatrix& pos, double ht, int abs =1) const;
  double PosOffset3D (const jhcMatrix& pos, double ht) const;
  double ErrZ (const jhcMatrix& pos) const {return fabs(loc.Z() - pos.Z());}
  double DirErr (jhcMatrix& derr, const jhcMatrix& dir, int abs =1) const;
  double TuckErr () const {return terr;}


// PROTECTED MEMBER FUNCTIONS
protected:
  // configuration
  int def_cmd ();


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameter manipulation
  int stow_params (const char *fname);

};
