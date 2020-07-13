// jhcVisObj.h : data about a visual object and its parts
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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

#ifndef _JHCVISOBJ_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCVISOBJ_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcBlob.h"
#include "Data/jhcRoi.h"
#include "Data/jhcImg.h"

#include "Objects/jhcVisPart.h"


//= Encapsulates data about a visual object and its parts.

class jhcVisObj
{
// PRIVATE MEMBER VARIABLES
private:
  int cnum;
  jhcVisObj *next;                      // next item in list of objects


// PUBLIC MEMBER VARIABLES
public:
  // selection status
  int valid;

  // object shape and pose
  double dir, asp;

  // how to grab item
  double gx, gy, gwid, gdir;

  // list of parts, first is "bulk"
  jhcVisPart part;                      


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcVisObj ();
 
  // list traversal
  jhcVisObj *NextObj () {return next;}
  void AddObj (jhcVisObj *obj) {if (next == NULL) next = obj;}

  // properties
  void BulkProps (double x, double y, double ang, double ecc); 
  void BulkMask (const jhcImg& bin, const jhcRoi& area); 
  char *BulkName () {return part.name;}
  jhcVisPart *GetPart (const char *id, int add =0);
  double OverlapBB (jhcVisObj& ref);

  // analysis
  void Clear ();
  void Copy (jhcVisObj& src);
  void Ingest (const jhcImg& src, const jhcImg& comp, const jhcBlob& blob, int i, const int *clim);


// PRIVATE MEMBER FUNCTIONS
private:
  int get_patches (const jhcImg& src, const jhcImg& comp, const jhcBlob& blob, int i);


};


#endif  // once




