// jhcManusRWI.cpp : collection of real world interfaces for Manus forklift robot
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

#include "Manus/jhcInteractFSM.h"
#include "Objects/jhcStackSeg.h"
#include "Objects/jhcPatchProps.h"

#include "Manus/jhcManusRWI.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcManusRWI::~jhcManusRWI ()
{
  // deallocate components
  delete fsm;
  delete ext;
  delete seg;
}


//= Default constructor initializes certain values.
// creates member instances here so header file has fewer dependencies

jhcManusRWI::jhcManusRWI ()
{
  // allocate components
  body = NULL;
  seg = new jhcStackSeg;
  ext = new jhcPatchProps;
  fsm = new jhcInteractFSM;
}


//= Attach extra processing to physical or simulated body.

void jhcManusRWI::BindBody (jhcManusX *b)
{
  body = b;
  fsm->BindBody(b);
}


//= Set image sizes even if no body.

void jhcManusRWI::SetSize (int x, int y)
{
  seg->SetSize(x, y);
  ext->SetSize(x, y);
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcManusRWI::Defaults (const char *fname)
{
  int ok = 1;

  ok &= LoadCfg(fname);
  ok &= seg->Defaults(fname);
  ok &= ext->Defaults(fname);
  return ok;
}


//= Read just deployment specific values from a file.

int jhcManusRWI::LoadCfg (const char *fname)
{
  int ok = 1;

  if (body != NULL)
    ok &= body->Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcManusRWI::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= SaveCfg(fname);
  ok &= seg->SaveVals(fname);
  ok &= ext->SaveVals(fname);
  return ok;
}


//= Write current deployment specific values to a file.

int jhcManusRWI::SaveCfg (const char *fname) const
{
  int ok = 1;

  if (body != NULL)
    ok &= body->SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Restart background processing loop.
// NOTE: body should be reset outside of this

void jhcManusRWI::Reset ()
{
  const jhcImg *src;

  // reset components
  seg->Reset();

  if (body != NULL)
  {
    // configure visual analysis for camera images
    src = body->View();
    seg->SetSize(*src);
    ext->SetSize(*src);

    // after body reset, sensor info will be waiting and need to be read
    body->Update(1);
  }

  // restart background loop, which first generates a body Issue call
  jhcBackgRWI::Reset();
}


//= Call at end of main loop to stop background processing.

void jhcManusRWI::Stop ()
{
  jhcBackgRWI::Stop();
}


///////////////////////////////////////////////////////////////////////////
//                          Interaction Overrides                        //
///////////////////////////////////////////////////////////////////////////

//= Run local behaviors then send arbitrated commands to body.

void jhcManusRWI::body_issue ()
{
  if (body != NULL)
    body->Issue();
  seen = 0;
  if (body != NULL)
    seen = body->UpdateImg(0); 
}


//= Get sensor inputs and fully process images.

void jhcManusRWI::body_update ()
{
  // do bulk of image processing
  if (seen > 0)
  {  
    body->Rectify();
    seg->Analyze(*(body->View()));
  }

  // wait (if needed) for body sensor data to be received
  if (body != NULL)
    body->Update(0);
}

