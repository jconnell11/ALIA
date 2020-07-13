// jhcManusX.h : generic functions for physical or TAIS forklift robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#ifndef _JHCMANUSX_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMANUSX_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"               // common video


//= Generic functions for physical or TAIS forklift robot.
// either jhcManusBody or jhcManusTAIS can be cast to this

class jhcManusX
{
// PROTECTED MEMBER VARIABLES
protected:
  // state
  int mok;

  // commands and bids
  double move, turn, fork;
  int grip, mlock, tlock, flock, glock;

  // odometry and sensed values
  double trav, head, xpos, ypos;
  double ht, wid, dist;

  // gripper status
  double wprev, wmin, wmax;
  int wcnt;

  // image acquisition
  jhcImg frame;
  int got;


// PUBLIC MEMBER VARIABLES
public:
  // for gripper status
  double wsm, wtol, wprog;
  int wstop;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcManusX ();
  jhcManusX ();
  virtual void SetSize (int w, int h);

  // processing parameter bundles (override these)
  virtual int Defaults (const char *fname =NULL) {return 0;}
  virtual int SaveVals (const char *fname) const {return 0;}

  // main functions (override Reset and Stop)
  int CommOK () const {return mok;}
  virtual int Reset (int noisy =0) {return clr_state();}
  virtual void Stop () {};

  // image acquisition 
  bool NewFrame () const {return(got > 0);}
  const jhcImg *View () const {return &frame;}
  
  // rough odometry (override Zero)
  virtual void Zero ()    {clr_odom();}
  double Travel () const  {return trav;}
  double WindUp () const  {return head;}
  double Heading () const {return norm_ang(head);}
  double X () const       {return ypos;}
  double Y () const       {return xpos;}

  // fork and hand status
  double Distance () const {return dist;}
  double Height () const   {return ht;}
  bool AtTop () const      {return(ht >= 4.0);}
  double Width () const    {return wid;}
  double Narrow () const   {return(wid - wsm);}
  bool Stable () const     {return(wcnt >= wstop);}
  bool Changing () const   {return(wcnt <= 0);}
  bool Empty () const      {return(wid <= (wmin + wtol));}

  // motion commands
  int MoveVel (double ips, int bid =10);
  int TurnVel (double dps, int bid =10);
  int LiftVel (double ips, int bid =10);
  int Grab (int dir, int bid =10);

  // core interaction (override most of these)
  virtual int Update (int img =1) {return cmd_defs();}
  virtual int UpdateImg (int rect =1) {return 0;}
  virtual void Rectify () {};
  virtual int Issue () {return mok;}


// PROTECTED MEMBER FUNCTIONS
protected:
  // helper functions
  int clr_state ();
  int cmd_defs ();
  void clr_odom ();
  double norm_ang (double a) const;


};


#endif  // once




