// jhcSceneVis.h : interface to ELI visual behavior kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 Etaoin Systems
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

#ifndef _JHCSCENEVIS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSCENEVIS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Action/jhcTimedFcns.h"       // common robot   
#include "Eli/jhcEliGrok.h"


//= Interface to ELI visual behavior kernel for ALIA system.

class jhcSceneVis : public jhcTimedFcns
{
// PRIVATE MEMBER VARIABLES
private:
  // link to hardware 
  jhcEliGrok *rwi;           // likely shared
  jhcSurfObjs *seg;
  jhcPatchProps *ext;
  jhcEliBody *body;

  // status variables
  jhcAliaNote *rpt;          // where to inject NOTEs

  // analysis mask
  jhcImg bin;

int ocnt, rcnt, ccnt;


// PUBLIC MEMBER VARIABLES
public:
  int dbg;                   // controls diagnostic messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcSceneVis ();
  jhcSceneVis ();
  void Platform (jhcEliGrok *io);

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  void local_reset (jhcAliaNote *top);
  void local_volunteer ();
  int local_start (const jhcAliaDesc *desc, int i);
  int local_status (const jhcAliaDesc *desc, int i);

  // object finding
  JCMD_DEF(vis_enum);

  // color analysis
//  JCMD_DEF(vis_color);
  JCMD_DEF(vis_col_ok);
  JCMD_DEF(vis_shp_ok);
//  int add_colors (jhcAliaDesc *obj, int only);

  // size analysis
//  JCMD_DEF(vis_size);
//  JCMD_DEF(vis_sz_ok);
//  int add_size (jhcAliaDesc *obj, int only);

  // width analysis
//  JCMD_DEF(vis_width);
//  JCMD_DEF(vis_wd_ok);
//  int add_width (jhcAliaDesc *obj, int only);

  // height analysis
//  JCMD_DEF(vis_height);
//  JCMD_DEF(vis_ht_ok);
//  int add_height (jhcAliaDesc *obj, int only);

  // spatial arrangement
  JCMD_DEF(vis_layout);


/*
  // event functions
  void alert_close ();
  jhcAliaDesc *obj_seen () const;

  // color analysis
  JCMD_DEF(class_color);
  int add_colors (jhcAliaDesc *obj, int id, int only);

  // size analysis
  JCMD_DEF(class_size);
  int add_size (jhcAliaDesc *obj, int id, int only);

  // width analysis
  JCMD_DEF(class_width);
  int add_width (jhcAliaDesc *obj, int id, int only);

  // texture detection
  JCMD_DEF(det_texture);
  int add_striped (jhcAliaDesc *obj, int id, int only);
*/
};


#endif  // once




