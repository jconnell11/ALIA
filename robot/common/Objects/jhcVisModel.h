// jhcVisModel.h : data describing a visual object for matching
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011 IBM Corporation
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

#ifndef _JHCVISMODEL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCVISMODEL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"


//= Data describing a visual object for matching.

class jhcVisModel
{
// PUBLIC MEMBER VARIABLES
public:
  char name[80];
  jhcArr data;
  jhcVisModel *next;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcVisModel (int sz =12)
  {
    data.SetSize(sz); 
    *name = '\0'; 
    next = NULL;
  }


};


#endif  // once




