// jhcManipulate.h : interface to ELI object manipulation kernel for ALIA system 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021-2023 Etaoin Systems
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

#ifndef _JHCMANIPULATE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMANIPULATE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "API/jhcAliaDesc.h"           // common audio
#include "API/jhcAliaNote.h"     

#include "Data/jhcParam.h"             // common video
#include "Processing/jhcArea.h"
#include "Processing/jhcDraw.h"
#include "Processing/jhcResize.h"
#include "Processing/jhcRuns.h"
#include "Processing/jhcStats.h"
#include "Processing/jhcThresh.h"

#include "Geometry/jhcMatrix.h"        // common robot
#include "Objects/jhcSurfObjs.h"       
#include "RWI/jhcEliGrok.h"            

#include "Kernel/jhcStdKern.h"      


//= Interface to ELI object manipulation kernel for ALIA system.
// most motion phases have a target goal but also fallback acceptance criteria
// semantic operations are implemented by repeated calls to a fcn like "man_move"
// this sequences a number of linear trajectory segments such as "goto_grasp"
// jhcEliArm::Issue then generates an intermediate point with jhcMotRamp::RampNext
// this passes to an iterative inverse kinematics solver jhcEliArm::pick_angles 
// then short-term angle goals and speeds are sent using jhcDynamixel::MultiPosVel 
// finally the AX-12 servos attempt to achieve the motion using "slope" and "punch"

class jhcManipulate : public jhcStdKern, private jhcArea,  private jhcDraw,  private jhcResize, 
                      private jhcRuns,   private jhcStats, private jhcThresh
{
friend class CBanzaiDoc;  // for debugging

// PRIVATE MEMBER VARIABLES
private:
  // terminology for spatial relations
  static const char * const rel[];
  enum RNUM {TWIXT, LEFT, RIGHT, FRONT, BEHIND, NEARBY, NEXTTO, ON, DOWN, RMAX};
  //           0     1      2      3      4       5       6     7    8     9

  // instance control variables
  jhcMatrix *cpos;                 /** Position goal for action.     */
  jhcMatrix *cend;                 /** Final position for action.    */     
  jhcMatrix *cdir;                 /** Direction goal for action.    */
  double *caux2;                   /** More extra act information.   */
  int *citem;                      /** Track num of target object.   */
  int *cref;                       /** Track number of reference.    */
  int *cref2;                      /** Track for second reference.   */
  int *cflag;                      /** Exceptions in progress.       */
  int *ccnt2;                      /** Secondary counter for action. */

  // link to hardware and processing
  jhcEliGrok *rwi;

  // useful subparts of platform
  jhcSurfObjs *sobj;
  jhcEliLift *lift;
  jhcEliArm *arm;
  const jhcMatrix *pos, *dir;

  // reported events
  jhcAliaNote *rpt;

  // relation of gripper to object and surface (can only be one)
  jhcAliaDesc *held;
  double nose, left, hang, skew, wt;
  int fcnt, htrk, drop;

  // ground path of trajectory
  jhcImg path, mtns;
  double gap;

  // empty space finding
  jhcImg space, align, shrink;
  double xdest, ydest;
  int xpick, ypick;

  // currently selected action parameters (for convenience)
  jhcMatrix end, aim;
  double wid, sp, tim;
  int inst, pmode, dmode, worksp, target, keep;

  // error measurement and reporting (for convenience)
  jhcMatrix perr, derr;
  char txt[40], txt2[40];
  int msg;


// PRIVATE MEMBER PARAMETERS
private:
  // grab geometric parameters
  double knob, mesa, choke, ecc0, down, gulp, loose, tip; 

  // deposit spot parameters
  double swell, fuzz, iwid, bias, sdev, tween, buddy, hood;

  // trajectory control parameters
  double ttol, hold, wmin, wtim, edge, over, graze;
  int park;

  // done tolerance parameters
  double ptol, atol, wtol, ftol, cont, ztol, dtol;
  int detwang;

  // workspace limit parameters
  double wx1, wx0, wy1, wy0, wz1, wz0, fwd, wcy;

  // workspace movement parameters
  double zup, zdn, ztra, ybd, prow, ytra, xbd, xtra;


// PUBLIC MEMBER VARIABLES
public:
  int dbg;                   // control of diagnostic messages
  jhcParam hps, sps, cps, dps, wps, ips;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcManipulate ();
  jhcManipulate ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // debugging utilities
  void Workspace (jhcImg& dest, int r =255, int g =255, int b =255) const;
  void ForceItem (int t);
  void ForceDest (double wx, double wy, double wz);
  int Move (); 
  int State () const   {return cst[0];}
  int LastErr () const {return msg;}


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int hold_params (const char *fname);
  int ctrl_params (const char *fname);
  int done_params (const char *fname);
  int spot_params (const char *fname);
  int work_params (const char *fname);
  int into_params (const char *fname);

  // overridden virtuals
  void local_platform (void *soma);
  void local_reset (jhcAliaNote& top);
  void local_volunteer ();
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

  // recurring functions
  void set_size (const jhcImg& ref);
  int update_held ();

  // motion sequences 
  JCMD_DEF(man_wrap);
  JCMD_DEF(man_lift);
  JCMD_DEF(man_trans);
  JCMD_DEF(man_tuck);
  JCMD_DEF(man_point);

  // take phases
  int assess_obj ();
  int goto_via ();
  int goto_grasp ();
  int close_fingers ();
  int lift_off ();

  // move phases
  int assess_spot ();
  int xfer_over ();
  int place_on ();
  int release_obj ();
  int stow_arm ();
  int tuck_elbow (int rc);

  // sequence helpers
  void init_vals (int i);
  int chk_hand (jhcAliaDesc *expect);
  int chk_stuck (double err);
  int final_pose (int xyz);
  int fail_clean ();

  // coordinated motion
  int command_bot (int rc);
  int chk_outside (int& old, double gx, double gy, double gz);
  int adj_workspace (int fix, double gx, double gy, double gz);
  double surf_gap () const;
  double diff_workspace (int fix) const;

  // object acquisition
  double src_full (jhcMatrix& full, int up);
  void src_rel (jhcMatrix& rel, const jhcMatrix& full) const;
  bool update_src (int& cnt) const;
  int compute_src (double& grip, jhcMatrix& rel, jhcMatrix& dir);
  int pick_grasp (double& w, double& ang, jhcMatrix& grab, int t) const;
  double corner_ang (double tx, double ty) const;
  double easy_grip (double pan, double ecc, double grip) const;
  int clear_grip ();
  void record_grip (int t);

  // trajectory utilities
  double obj_peaks (double wx, double wy, double fsep, int carry);
  void traj_path (double wx, double wy, double fsep, int carry);

  // destination determination
  void anchor_loc (jhcMatrix& loc) const;
  void dest_full (jhcMatrix& full, int up);
  void dest_rel (jhcMatrix& rel, const jhcMatrix& full) const;
  bool update_dest (int& cnt) const;
  int compute_dest (jhcMatrix& rel, double& pan, int& eflag, int adj);
  void adjust_dest (jhcMatrix& full, double& pan, int any);
  int dest_bottom (jhcMatrix& loc, double& pan);

  // open space finding
  void free_space (int exc);
  void dest_ref (double& ix, double& iy, int t, int rn, int a, int a2);
  double dest_ang (double ix, double iy, int t, int rn, int a, int a2) const;
  int pick_spot (double& cx, double& cy, double ix, double iy, double pan, int t, int rn, int a, int a2);

  // destination parsing
  int txt2rnum (const char *txt) const;
  const char *rnum2txt (int rn) const;
  int ref_tracks (int& a, int& a2, int rn, const jhcAliaDesc *place);

  // semantic messages
  int err_arm ();
  int err_size (int rc);
  int err_spot ();
  int err_gone (jhcAliaDesc *obj);
  int err_reach (jhcAliaDesc *obj);
  int err_lack (jhcAliaDesc *obj);
  int err_drop (jhcAliaDesc *obj);
  void msg_hold (jhcAliaDesc *obj, int neg);


};


#endif  // once




