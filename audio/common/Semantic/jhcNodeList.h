// jhcNodeList.h : sequential list of semantic network nodes
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2021-2023 Etaoin Systems
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

#ifndef _JHCNODELIST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCNODELIST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Semantic/jhcNetNode.h"       // common audio


//= Sequential list of semantic network nodes.
// abstract class that others are derived from
// anything that can be cast to this class is enumerable

class jhcNodeList
{
// PUBLIC MEMBER FUNCTIONS
public:
  // main functions (override in derived classes)
  virtual jhcNetNode *NextNode (const jhcNetNode *prev =NULL, int bin =-1) const =0;
  virtual int Length () const =0;
  virtual bool InList (const jhcNetNode *n) const =0;
  virtual bool Prohibited (const jhcNetNode *n) const {return(n == NULL);}
  virtual int NumBins () const {return 1;}
  virtual int SameBin (const jhcNetNode& focus, const jhcBindings *b) const {return 1;}
  virtual int NumBands () const {return 1;}
  virtual bool InBand (const jhcNetNode *n, int part) const {return true;}


};


#endif  // once




