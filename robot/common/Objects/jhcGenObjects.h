// jhcGenObjects.h : generic manipulable object detector interface
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

#include "Data/jhcImg.h"               // common video
#include "Data/jhcRoi.h"

#include "Geometry/jhcMatrix.h"        // common robot


//= Generic manipulable object detector interface.

class jhcGenObjects
{
// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  virtual ~jhcGenObjects ();
  jhcGenObjects ();
 
  // main functions
  virtual int CntValid (int trk =1) const =0;

  // object properties 
  virtual int ObjLimit (int trk =1) const =0;
  virtual int ObjID (int i, int trk =1) const =0;
  virtual bool ObjOK (int i, int trk =1) const =0;
  virtual int ObjTrack (int id) const =0;
  virtual bool OkayID (int id) const {return ObjOK(ObjTrack(id));}
  virtual double PosX (int i, int trk =1) const =0; 
  virtual double PosY (int i, int trk =1) const =0; 
  virtual double SizeZ (int i, int trk =1) const =0;
  virtual double Major (int i, int trk =1) const =0;
  virtual double Minor (int i, int trk =1) const =0;
  virtual double OverZ (int i, int trk =1) const =0;

//  virtual void Retain (int i) =0;
//  virtual void CamBox (jhcRoi& box, int i, int ydim =480) const =0;

  // object properties
  virtual int Closest () const =0;
  virtual double DistXY (int i) const =0;
  virtual double World (jhcMatrix& loc, int i) const =0;
  virtual double World (double& wx, double& wy, int i) const =0;

  virtual int Spectralize (const jhcImg& col, const jhcImg& d16, int i, int clr =0) =0;
  virtual int DegColor (int i, int cnum) const =0;
  virtual double AmtColor (int i, int cnum) const =0;

  // display helpers
  virtual void SetTag (int i, const char *txt) =0;
  virtual const char *GetTag (int i) const =0;
  virtual int SetState (int i, int val) =0;
  virtual int GetState (int i) const =0;

  // debugging graphics
//  virtual void AdjGeom (int cam =0) =0;


};
