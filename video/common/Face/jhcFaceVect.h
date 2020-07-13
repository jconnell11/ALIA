// jhcFaceVect.h : stores condensed representation for one face instance
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 IBM Corporation
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

#ifndef _JHCFACEVECT_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFACEVECT_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"


//= Stores condensed representation for one face instance.

class jhcFaceVect
{
// PRIVATE MEMBER VARIABLES
private:
  int sz;


// PUBLIC MEMBER VARIABLES
public:
  // basic vector list
  double *data;
  jhcFaceVect *next;

  // example image
  jhcImg thumb;
  int inum;

  // match status
  double dist;
  int util, rank;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcFaceVect ();
  jhcFaceVect (int vsz =256);
  int Copy (const jhcFaceVect *ref); 

  // file functions
  int Load (FILE *in);
  void Save (FILE *out) const;


// PRIVATE MEMBER FUNCTIONS
private:


};


#endif  // once




