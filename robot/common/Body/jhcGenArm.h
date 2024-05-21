// jhcGenArm.h : control interface for generic robot arm
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

#include "Geometry/jhcMatrix.h"        // common robot


//= Control interface for generic robot arm.
// these are the functions available to grounding kernels

class jhcGenArm
{
// PROTECTED MEMBER VARIABLES
protected:
  jhcMatrix loc;                       // Current gripper position.    
  jhcMatrix aim;                       // Current gripper orientation. 


// PUBLIC MEMBER VARIABLES
public:
  // arm stowed position
  double retx, rety, retz, rdir, rtip;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  virtual ~jhcGenArm () {}
  jhcGenArm () {loc.SetSize(4); aim.SetSize(4);}
  virtual int CommOK () {return 1;}
  virtual double AngTol () const  {return 3.0;}
  virtual double MaxWidth () const =0;

  // ---------------------- HAND MAIN ----------------------------

  // current hand information
  virtual double Width () const =0;
  virtual double Squeeze () const =0;
  virtual double SqueezeGoal () const =0;

  // hand goal specification commands
  virtual int WidthTarget (double sep, double rate =1.0, int bid =10) =0;
  virtual int SqueezeTarget (double force, int bid =10) =0;
  virtual int HandTarget (double sep, double rate =1.0, int bid =10) =0;

  // hand motion progress
  virtual double WidthErr (double sep) const =0;
  virtual double SqueezeErr (double f) const =0;

  // ---------------------- ARM MAIN ----------------------------

  // current arm information
  const jhcMatrix *Position () const    {return &loc;}     // values continually updated
  const jhcMatrix *Direction () const   {return &aim;}     // values continually updated
  void Position (jhcMatrix& pos) const  {pos.Copy(loc);}
  void Direction (jhcMatrix& dir) const {dir.Copy(aim);}
  virtual double ObjectWt (double grav =4.0, double fsc =0.57) const {return 0.0;}
  virtual double ReachRate () const =0;
  virtual int Static () const =0;

  // arm Cartesian goal specification commands
  virtual int PosTarget (double ax, double ay, double az, double rate =1.0, int bid =10, int mode =0x0) =0;
  virtual int PosTarget3D (const jhcMatrix& pos, double ht, double rate =1.0, int bid =10, int mode =0x0) =0;
  virtual int DirTarget (const jhcMatrix& dir, double rate =1.0, int bid =10, int mode =0x0) =0;
  virtual int ArmTarget (const jhcMatrix& pos, const jhcMatrix& dir, 
                         double p_rate =1.0, double d_rate =0.0, int bid =10) =0;
  virtual int Tuck (double rate =1.0, int bid =10) =0;

  // arm motion progress
  virtual double PosErr3D (jhcMatrix& perr, const jhcMatrix& pos, double ht, int abs =1) const =0;
  virtual double PosOffset3D (const jhcMatrix& pos, double ht) const =0;
  virtual double ErrZ (const jhcMatrix& pos) const =0;
  virtual double DirErr (jhcMatrix& derr, const jhcMatrix& dir, int abs =1) const =0;
  virtual double TuckErr () const =0;

};
