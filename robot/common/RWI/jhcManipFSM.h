// jhcManipFSM.h : state sequencer for complex arm motions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2016 IBM Corporation
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

#ifndef _JHCMANIPFSM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMANIPFSM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"             // common video

#include "Geometry/jhcMatrix.h"        // common robot

#include "Body/jhcEliArm.h"


//= State sequencer for complex arm motions.

class jhcManipFSM
{
// PRIVATE MEMBER VARIABLES
private:
  jhcEliArm *arm;
  jhcMatrix tpos0, tdir0, tpos, tdir;
  int phase, fcnt, gcnt, dcnt, retry;
  double zgrab, tupd, slow, psp, dsp, wsp, grip;
  double tx, ty, vx, vy, vdir;


// PUBLIC MEMBER VARIABLES
public:
  int noisy, mark;

  // parameters for motions
  jhcParam tps;
  double tz, tilt, roll, up, wid, wtol, fhi, flo;

  // parameters for home location
  jhcParam hps;
  double hx, hy, hz, hp, ht, hr, hw;

  // parameters for middle deployed location
  jhcParam mps;
  double mx, my, mz, mp, mt, mr;

  // parameters for passing objects
  jhcParam pps;
  int pbox;
  double px, py, pz, pp, pt, pr, pwait;
  

// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  jhcManipFSM ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // configuration
  void Bind (jhcEliArm *a);
  void SetRate (double freq) {tupd = 1.0 / freq;};
  void Reset ();
  void SetTarget (double x2, double y2, double x, double y, double dir);
  int Complete (int total);
  int NearHome (double pmax =1.0, double dmax =10.0);

  // motion primitives
  int CloseHand (int step0 =1);
  int OpenHand (int step0 =1);
  int ExtendHand (double amt =1.0, int step0 =1);
  int SlideHand (double amt =1.0, int step0 =1);
  int RaiseHand (double amt =1.0, int step0 =1);
  int PanHand (double amt =1.0, int step0 =1);
  int TiltHand (double amt =1.0, int step0 =1);
  int RollHand (double amt =1.0, int step0 =1);

  // locations
  int GotoVia (double zdes =2.0, double wdes =0.0, int step0 =1);
  int GotoHome (int step0 =1);
  int GotoMiddle (int step0 =1);
  int GotoXfer (int step0 =1);

  // full motions
  int TablePoint (double wdes =0.0, int step0 =1);
  int TableLift (int step0 =1);
  int TableDeposit (int step0 =1);
  int Handoff (int click =0, int step0 =1);
  int Replace (int click =0, int step0 =1);

  // motion cycles
  int GrabCycle (double dwell =-1.0, int step0 =1);
  int GrabReset (double dwell =0.0, int step0 =1);
  int PointCycle (int hold =0, int step0 =1);
  int GiveCycle (int click =0, int step0 =1);
               

// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int traj_params (const char *fname);
  int home_params (const char *fname);
  int mid_params (const char *fname);
  int pass_params (const char *fname);

  // arm interface
  void get_current (double& x, double& y, double& z, 
                    double& p, double& t, double& r, double& w,
                    int update =1);
  void get_grasp (double& z, double& w, int update =1);
  double get_width (int update =1);
  void seek_target (double x, double y, double z, 
                    double p, double t, double r, double w,
                    double sp =1.0);
  void pick_speeds (double sp);
  int await_target (int mv_code =0, double inxy =0.25, double inz =0.25, double degs =5.0, int nofail =1);


};


#endif  // once




