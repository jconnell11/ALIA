// jhcGenBody.h : interface class for adding legacy jhcEliBody to jhcVisGrok
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2025 Etaoin Systems
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

#include "Geometry/jhcMatrix.h"        // common robot


//= Interface class primarily for adding legacy jhcEliBody to jhcVisGrok.

class jhcGenBody 
{
// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  virtual ~jhcGenBody () {}
  jhcGenBody () {}

  // sensor info
  virtual int RngReady () const  {return 1;}
  virtual int RngStatic () const {return 30;}
  virtual int ColStatic () const {return RngStatic();}
  virtual int AuxStatic () const {return ColStatic();}
  virtual void RngPose (jhcMatrix& pos, jhcMatrix& dir) const =0;
  virtual void ColPose (jhcMatrix& pos, jhcMatrix& dir) const {RngPose(pos, dir);}
  virtual void AuxPose (jhcMatrix& pos, jhcMatrix& dir) const {ColPose(pos, dir);}

  // basic actions
  virtual int Limp () {return 0;}

  // main functions
  virtual int UpdateImgs () {return 0;}
  virtual int Update (int voice =0, int imgs =1) =0;
  virtual int Issue (double lead =3.0) =0;

};

