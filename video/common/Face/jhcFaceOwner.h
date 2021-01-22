// jhcFaceOwner.h : stores all face information about a particular person
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 IBM Corporation
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

#ifndef _JHCFACEOWNER_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFACEOWNER_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Face/jhcFaceVect.h"


//= Stores all face information about a particular person.

class jhcFaceOwner
{
// PRIVATE MEMBER VARIABLES
private:
  // configuration
  char name[80];
  int vsz;

  // recognition data 
  jhcFaceVect *vect;
  int nv;


// PUBLIC MEMBER VARIABLES
public:
  // list structure and image numbering
  jhcFaceOwner *next;
  int ibig;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcFaceOwner ();
  jhcFaceOwner (const char *who, int sz =256);
  const char *Who () const {return name;}
  int NumVec () const {return nv;} 

  // main functions
  int AddVect (jhcFaceVect *v, int vcnt =0);
  jhcFaceVect *NextVect (const jhcFaceVect *v =NULL) const
    {return((v == NULL) ? vect : v->next);}

  // file functions
  int Load (const char *dir);
  int Save (const char *dir) const;


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr_vect ();

  // main functions
  void rem_weakest ();


};


#endif  // once




