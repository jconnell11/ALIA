// jhcSupport.h : interface to ELI surface finder kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
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

#include "Environ/jhcTable.h"          // common robot   
#include "Geometry/jhcMatrix.h"
#include "RWI/jhcEliRWI.h"            

#include "Kernel/jhcStdKern.h"       


//= Interface to ELI surface finder kernel for ALIA system.
// maintains set of "saved" surface locations associated with semantic nodes

class jhcSupport : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  static const int smax = 10;          /** Max remembered patches. */

  // instance control variables
  jhcMatrix *cpos;             

  // link to hardware and components
  jhcEliRWI *rwi;                      // likely shared
  jhcTable *tab;
  jhcGenNeck *neck;
  jhcGenLift *lift;

  // semantic network input
  jhcAliaNote *rpt;                    // where to inject NOTEs

  // event state
  int tok, any, prox;

  // saved patches
  jhcMatrix saved[smax];
  double soff[smax];
  int sid[smax];
  int last_id;   


// PRIVATE MEMBER PARAMETERS
private:
  // table detection event parameters
  double d1, d0, dhys, dnear, h1, h0;
  int tnew;

  // surface motion parameters
  double ptol, ttol, atol, acc, app;

  // height parameters
  double hmax, havg, hmth, mavg, mlth, lavg, flr;

  // location and azimuth parameters
  double dfar, dmid, band, dxy, hfov;

  // tracking and selection parameters
  double ztol, xytol, mix, inset, gtol, drop;


// PUBLIC MEMBER VARIABLES
public:
  jhcParam eps, mps, hps, lps, tps;
  int gok;                   // whether succeeds without body
  int dbg;                   // control of diagnostic messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSupport ();
  jhcSupport ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int event_params (const char *fname);
  int motion_params (const char *fname);
  int height_params (const char *fname);
  int location_params (const char *fname);
  int track_params (const char *fname);

  // overridden virtuals
  void local_platform (void *soma);
  void local_reset (jhcAliaNote& top);
  void local_volunteer ();
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

  // event functions
  void update_patches ();
  void table_seen ();
  void table_close ();
  int current_vis (jhcAliaDesc **obj);

  // surface finding 
  JCMD_DEF(surf_enum);
  double scan_suitable (int dqnt, int hqnt, double d0);
  JCMD_DEF(surf_on_ok);

  // surface interaction
  JCMD_DEF(surf_look);
  JCMD_DEF(surf_orient);
  JCMD_DEF(surf_goto);

  // utilities
  int save_patch ();
  int saved_index (const jhcAliaDesc *obj) const;
  int saved_detect ();
  int saved_gaze () const;
  int chk_neck (int i, double err);
  int chk_base (int i, double err);

  // quantized constraints
  int surf_azm (const jhcAliaDesc *obj) const;
  int surf_dist (const jhcAliaDesc *obj) const;
  int surf_ht (const jhcAliaDesc *obj) const;
  int surf_azm (const jhcMatrix& patch) const;
  int surf_dist (const jhcMatrix& patch) const;
  int surf_ht (const jhcMatrix& patch) const;

  // net assertions
  void std_props (jhcAliaDesc *obj, int born);
  void add_azm (jhcAliaDesc *obj, int aqnt) const;
  void add_dist (jhcAliaDesc *obj, int aqnt) const;
  void add_ht (jhcAliaDesc *obj, int aqnt) const;

  // semantic messages
  void msg_gone (jhcAliaDesc *surf);
  int err_hw (const char *sys);
  int err_vis (jhcAliaDesc *item);



};

