// jhcPTZ.h : control for Sony EVI-D30 pan tilt zoom camera
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2002-2013 IBM Corporation
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

#ifndef _JHCPTZ_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCPTZ_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Interface/jhcSerial.h"


//= Control for Sony EVI-D30/D100 pan tilt zoom camera.

class jhcPTZ
{
// PRIVATE MEMBER VARIABLES
private:
  jhcSerial defp;
  jhcSerial *port;
  int PMAX, TMAX, PVMX, TVMX, ZMAX, ZVMN, ZVMX;
  double PRNG, TRNG, PVEL, TVEL, FMIN, FMAX, AMAX, AMIN, ZTIM, HSZ2;
  double fov_est;
  int mode, count, phase, ack_pend;


// PUBLIC MEMBER VARIABLES
public:
  int addr;   /** Camera ID number for serial.  */
  double p0;  /** Home pan angle (degrees).     */
  double t0;  /** Home tilt angle (degrees).    */
  double z0;  /** Home field of view (degrees). */


// PUBLIC MEMBER FUNCTIONS
public:
  ~jhcPTZ ();
  jhcPTZ ();
  jhcPTZ (int n);

  void Defs ();
  int SetPort (int n =1);
  int BindPort (jhcSerial *ser);

  void SetD30 ();
  void SetD100 ();
  double FOV (int zpos);

  int Init ();
  int Freeze ();
  int Home ();
  int Mark ();
  int Status ();

  int AngVel (double pdegs, double tdegs, double zdegs, 
              double secs =1.0, int force =0);
  int FracVel (double xfrac, double yfrac, double sc, 
               double secs =1.0, int force =0);
  int Where (double *pdeg, double *tdeg, double *fov =NULL);
  int Goto (double pdeg, double tdeg, double fov, int slew =0);
  int Shift (double xfrac, double yfrac, double sc, int slew =0);
  int AwaitDone (double max_secs =0.0);

  void AimVel (int pan, int tilt, int wait =0);
  void ZoomVel (int zoom, int wait =0);
  void Idle ();

  void RawZoomSpeed (int zvel);
  void RawZoomSet (int zpos);
  int RawZoomPos (int *zcnt);
  void RawAimSpeed (int pvel, int tvel);
  void RawAimSet (int ppos, int tpos, int pvel, int tvel);
  int RawAimPos (int *pcnt, int *tcnt);


// PRIVATE MEMBER FUNCTIONS
private:
  double focal (double view);
  double view (double focal);

  int stop_cmd ();

  void command (int c0, int c1 =-1, int c2 =-1, int c3 =-1);
  void send (int c0, int c1 =-1, int c2 =-1, int c3 =-1);
  int await_ack (int force =0);
  int await_msg (int tag =-1, int mask =0xFF);
  int packet_end ();
};


#endif




