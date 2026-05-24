// jhcGenGrok.h : generic interface to high level actuator commands
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

#pragma once

#include "jhcGlobal.h"

#include "Body/jhcGenArm.h"            // common robot
#include "Body/jhcGenBase.h"
#include "Body/jhcGenBody.h"
#include "Body/jhcGenLift.h"
#include "Body/jhcGenNeck.h"
#include "Peripheral/jhcGenMic.h"


//= Generic interface to high level actuator commands.
// holds pointers to body components - available to grounding kernels

class jhcGenGrok 
{
// PUBLIC MEMBER VARIABLES
public:
  // whether in sensor cycle
  int accept;

  // robot and subcomponents
  jhcGenBody *body;
  jhcGenNeck *neck;
  jhcGenArm  *arm;
  jhcGenLift *lift;
  jhcGenBase *base;

  // directional microphone
  jhcGenMic *mic;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  virtual ~jhcGenGrok () {};
  jhcGenGrok () 
    {body = NULL; neck = NULL; arm = NULL; lift = NULL; base = NULL; mic = NULL;}
  int CommOK () const
    {return(((NeckOK() <= 0) || (ArmOK() <= 0) || (LiftOK() <= 0) || (BaseOK() <= 0) || (MicOK() <= 0)) ? 0 : 1);}
  int NeckOK () const  {return(((neck != NULL) && (neck->CommOK() <= 0)) ? 0 : 1);}
  int ArmOK () const   {return(((arm  != NULL) && (arm->CommOK()  <= 0)) ? 0 : 1);}
  int LiftOK () const  {return(((lift != NULL) && (lift->CommOK() <= 0)) ? 0 : 1);}
  int BaseOK () const  {return(((base != NULL) && (base->CommOK() <= 0)) ? 0 : 1);}
  int MicOK () const   {return(((mic  != NULL) && (mic->CommOK()  <= 0)) ? 0 : 1);}
  void Broken () const 
    {jprintf("Broken:%s%s%s%s%s\n", 
             ((NeckOK() <= 0) ? " neck" : ""), ((ArmOK() <= 0) ? " arm" : ""), ((LiftOK() <= 0) ? " lift" : ""), 
             ((BaseOK() <= 0) ? " base" : ""), ((MicOK() <= 0) ? " mic" : ""));}

  // runtime status
  virtual bool Ghost () const {return false;}
  virtual bool Accepting () const {return(accept > 0);}

};
