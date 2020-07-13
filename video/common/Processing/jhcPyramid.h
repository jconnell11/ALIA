// jhcPyramid.h : routines for manipulating composite pyramid images
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2004-2011 IBM Corporation
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

#ifndef _JHCPYRAMID_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCPYRAMID_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include "Data/jhcImg.h"


//= Routines for manipulating composite pyramid images.
// pyramid levels are all stacked together in the same image
// <pre>
//
//  +---------------------------------+
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  |               L0                |
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  |                                 |
//  +----------------+----------------+
//  |                |                |
//  |                |                |
//  |                +--------+       |
//  |      L1        |        |       |
//  |                |   L2   +----+  |
//  |                |        | L3 |  |
//  +----------------+--------+----+--+
//
// </pre>

class jhcPyramid
{
// PUBLIC MEMBER FUNCTIONS
public:
  // utilities
  jhcImg *PyrSize (jhcImg& dest, const jhcImg& src) const;
  int PyrOK (const jhcImg& pyr, const jhcImg& src) const;
  int PyrDepth (const jhcImg& pyr) const; 
  int PyrWid (const jhcImg& pyr, int level =0) const;
  int PyrHt (const jhcImg& pyr, int level =0) const;

  // pyramid formation 
  int PyrAvg (jhcImg& pyr, const jhcImg& src) const;
  int PyrSamp (jhcImg& pyr, const jhcImg& src) const;
  int PyrMax (jhcImg& pyr, const jhcImg& src) const;

  // level extraction
  int PyrRoi (jhcImg& pyr, int level =0) const;
  int PyrGet (jhcImg& dest, jhcImg& pyr, int level =0) const;

};


#endif




