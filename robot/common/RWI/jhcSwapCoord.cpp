// jhcSwapCoord.cpp : parsing, learning, and control for external robot
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

#include "Interface/jms_x.h"           // common video
#include "Interface/jtimer.h"

#include "RWI/jhcSwapCoord.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSwapCoord::~jhcSwapCoord ()
{
  // for debugging - only happens when program closes
  DumpAll();
}


//= Default constructor initializes certain values.

jhcSwapCoord::jhcSwapCoord ()
{
  // bind sub-mechanisms into overall body 
  rwi.neck = &neck0;
  rwi.arm  = &arm0;
  rwi.lift = &lift0;
  rwi.base = &base0;

  // attach grounding kernels
  kern.AddFcns(ball); 
  kern.Platform(&rwi);

  // default processing parameters and state
  noisy = 1;
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for overall control of timing.
// this should be called in Defaults and tps used in SaveVals

int jhcSwapCoord::kern_params (const char *fname)
{
  jhcParam *ps = &kps;
  int ok;

  ps->SetTag("kern_dbg", 0);
/*
  ps->NextSpec4( &(svis.dbg),    2, "SceneVis objects (std = 2)");
  ps->NextSpec4( &(sup.dbg),     2, "Support surfaces (std = 2)");
  ps->NextSpec4( &(soc.dbg),     2, "Social agents (std = 2)");
  ps->Skip();
  ps->NextSpec4( &(ball.dbg),    1, "Ballistic body (std = 1)");
  ps->NextSpec4( &(man.dbg),     1, "Manipulation arm (std = 1)");
*/
  ps->NextSpec4( &(dmem.enc),    0, "LTM encoding (dbg = 3)");
  ps->NextSpec4( &(dmem.detail), 0, "LTM retrieval for node");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcSwapCoord::Defaults (const char *fname)
{
  int ok = 1;

  // local parameters
  ok &= time_params(fname);            // jhcAliaSpeech
  ok &= kern_params(fname);

  // component parameters
  ok &= arm0.Defaults(fname);

  // kernel parameters
  ok &= ball.Defaults(fname);

  // core parameters
  ok &= jhcAliaCore::Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcSwapCoord::SaveVals (const char *fname) 
{
  int ok = 1;

  // local parameters
  ok &= tps.SaveVals(fname);
  ok &= kps.SaveVals(fname);
  ok &= jhcAliaCore::SaveVals(fname);

  // component parameters
  ok &= arm0.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// dir = working directory for language, KBx, GND, config, calib, log, dump
// rname = particular robot, last name encodes body type (e.g. "Ivy Banzai")
// returns 2 if robot ready, 1 if ready but no robot, 0 or negative for error

int jhcSwapCoord::Reset (const char *dir, const char *rname, int prt)
{
  char ifile[200] = "config/robot_vals.ini";
  const char *prog;

  // load configuration parameters based on robot last name
  SetDir(dir);
  if (rname != NULL) 
    if ((prog = strchr(rname, ' ')) != NULL)
      sprintf_s(ifile, "config/%s_vals.ini", prog + 1);
  Defaults(wrt(ifile));

  // initialize hardware subsystems
  arm0.Reset();
  base0.Reset();

  // initialize timing and speech components
  jtimer_clr();
  if (jhcAliaSpeech::Reset(rname, prt) <= 0)
    return 0;
  return 1;
}


//= Call at end of run to put robot in stable state and possibly save knowledge.

int jhcSwapCoord::Done (int face)
{
  char fname[80], date[80];
  const char *me = Name();

  // stop hardware subsystems
 

  // save learned items
  DumpSession();                       // brand new rules and ops
  jhcAliaSpeech::Done(1);              // incl. accumulated knowledge

  // save call profiling
  sprintf_s(fname, "timing/%s_%s.txt", ((me != NULL) ? me : "timing"), jms_date(date));
  jtimer_rpt(1, wrt(fname), 1);
  return 1;
}


