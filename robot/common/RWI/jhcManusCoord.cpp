// jhcManusCoord.cpp : language processing and perception for Manus robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "RWI/jhcManusCoord.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcManusCoord::~jhcManusCoord ()
{
}


//= Default constructor initializes certain values.

jhcManusCoord::jhcManusCoord ()
{
  // connect display utilities to data
  disp.Bind(stat);

  // connect processing to basic robot I/O
  rwi.BindBody(&body);

  // add hardware dependent kernels
  kern.AddFcns(snd);
  kern.AddFcns(act);
  kern.AddFcns(vis);
  kern.Platform(&rwi);

  // default processing parameters and state
  noisy = 1;
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcManusCoord::Defaults (const char *fname)
{
  int ok = 1;

  ok &= time_params(fname);
  ok &= jhcAliaCore::Defaults(fname);
  ok &= rwi.Defaults(fname);
  ok &= vis.Defaults(fname);
  ok &= act.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcManusCoord::SaveVals (const char *fname) 
{
  int ok = 1;

  ok &= tps.SaveVals(fname);
  ok &= jhcAliaCore::SaveVals(fname);
  ok &= rwi.SaveVals(fname);
  ok &= vis.SaveVals(fname);
  ok &= act.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Connect a possibly canned video input to robot (or disconnect all). 

int jhcManusCoord::BindVideo (jhcVideoSrc *v, int vnum)
{
  body.BindVideo(v);
  return 1;
}


//= Reset state for the beginning of a sequence.
// can also optionally run system without physical robot (id = 0)
// returns 2 if robot ready, 1 if ready but no robot, 0 or negative for error

int jhcManusCoord::Reset (int id)
{
  int rc;

  // connect to robot and start processing
  rc = body.Reset(noisy, "config", id);
  rwi.Reset();

  // initialize timing and speech components
  if (jhcAliaSAPI::Reset(body.rname) <= 0)
    return 0;
  return((rc <= 0) ? 1 : 2);
}


//= Generate actions in response to update sensory information.
// returns 1 if happy, 0 to end interaction 

int jhcManusCoord::Respond ()
{
  if (UpdateSpeech() <= 0)
    return 0;
  rwi.Update(NextSense());
  if (jhcAliaSAPI::Respond() <= 0)
    return 0;
  rwi.Issue();
  DayDream(); 
  return 1;
}


//= Get some possibly annotated image to display on GUI.

const jhcImg *jhcManusCoord::View (int num) 
{
  if (!body.NewFrame())
    return NULL;
  return body.View();
}


//= Call at end of run to put robot in stable state and possibly save knowledge.

int jhcManusCoord::Done (int save)
{
  int status = 1;

  // stop real time interaction
  body.Stop();
  rwi.Stop();

  // save learned items
  DumpSession();
  jhcAliaSAPI::Done(1);
  return 1;
}


