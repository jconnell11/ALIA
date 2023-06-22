// jhcTargetVis.h : interface to Manus visual behavior kernel for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _JHCTARGETVIS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTARGETVIS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Objects/jhcStackSeg.h"       // common robot
#include "Objects/jhcPatchProps.h"
#include "RWI/jhcManusRWI.h"         

#include "Kernel/jhcStdKern.h"       


//= Interface to Manus visual behavior kernel for ALIA system.

class jhcTargetVis : public jhcStdKern
{
// PRIVATE MEMBER VARIABLES
private:
  // link to hardware 
  jhcManusRWI *rwi;          // likely shared
  jhcStackSeg *seg;
  jhcPatchProps *ext;
  jhcManusX *body;

  // status variables
  jhcAliaNote *rpt;          // where to inject NOTEs
  int focus, close;

  // analysis mask
  jhcImg bin;


// PUBLIC MEMBER VARIABLES
public:
  int dbg;                   // controls diagnostic messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcTargetVis ();
  jhcTargetVis ();

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // overridden virtuals
  void local_platform (void *soma);
  void local_reset (jhcAliaNote& top);
  void local_volunteer ();
  int local_start (const jhcAliaDesc& desc, int i);
  int local_status (const jhcAliaDesc& desc, int i);

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

};


#endif  // once




