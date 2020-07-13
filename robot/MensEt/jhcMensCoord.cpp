// jhcMensCoord.cpp : language processing and perception for Manus robot
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

#include "jhcMensCoord.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcMensCoord::~jhcMensCoord ()
{
}


//= Default constructor initializes certain values.

jhcMensCoord::jhcMensCoord ()
{
  // current software version
  ver = 3.80;

  // connect processing to basic robot I/O
  rwi.BindBody(&body);

  // bridge from operators to robot motion
  act.Platform(&rwi);
  kern.AddFcns(&act);

  // bridge from operators to motion sequences
//  seq.Platform(&rwi);
//  (core.kern).AddFcns(&seq);

  // bridge from operators to visual detection
  vis.Platform(&rwi);
  kern.AddFcns(&vis);

  // bridge from operators to sound effects
  kern.AddFcns(&snd);

  // default processing parameters and state
  noisy = 1;
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcMensCoord::Defaults (const char *fname)
{
  int ok = 1;

  ok &= time_params(fname);
  ok &= rwi.Defaults(fname);
  ok &= act.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcMensCoord::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= tps.SaveVals(fname);
  ok &= rwi.SaveVals(fname);
  ok &= act.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// can also optionally run system without physical robot (id = 0)
// returns 2 if robot ready, 1 if ready but no robot, 0 or negative for error

int jhcMensCoord::Reset (int id)
{
  int rc;

  // connect to robot and start processing
  rc = body.Reset(noisy, "config", id);
  rwi.Reset();

  // initialize timing and speech components
  if (jhcAliaSpeech::Reset() <= 0)
    return 0;
  return((rc <= 0) ? 1 : 2);
}


//= Generate actions in response to update sensory information.
// returns 1 if happy, 0 to end interaction 

int jhcMensCoord::Respond ()
{
  if (UpdateSpeech() <= 0)
    return 0;
  rwi.Update(NextSense());
  if (jhcAliaSpeech::Respond() <= 0)
    return 0;
  rwi.Issue();
  DayDream(); 
  return 1;
}


//= Call at end of run to put robot in stable state and possibly save knowledge.

void jhcMensCoord::Done ()
{
  body.Stop();
  rwi.Stop();
  jhcAliaSpeech::Done();
}


