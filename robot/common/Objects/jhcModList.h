// jhcModList.h : list of visual object models
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

#ifndef _JHCMODLIST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMODLIST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcArr.h"
#include "Data/jhcParam.h"

#include "Objects/jhcVisModel.h"


//= List of visual object models.

class jhcModList
{
// PRIVATE MEMBER VARIABLES
private:
  jhcVisModel *db;


// PUBLIC MEMBER VARIABLES
public:
  // parameters
  jhcParam mps;
  double af, wf, ef, cf, match, close;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcModList ();
  ~jhcModList ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // file operations
  int LoadModels (const char *fname, int append =0);
  int SaveModels (const char *fname);

  // main functions
  int AnyModels (const char *kind);
  int IsKind (const char *kind, jhcArr& vec);
  double DistKind (const char *kind, jhcArr& vec);
  int FindKind (char *kind, jhcArr& vec, int ssz);
  int AddModel (const char *kind, jhcArr& vec, int force =0);
  void RemModel ();

  // main functions - convenience
  template <size_t ssz>
    int FindKind (char (&kind)[ssz], jhcArr& vec)
      {return FindKind(kind, vec, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  void rem_all ();
  int match_params (const char *fname);

  double dist (jhcArr& vec1, jhcArr& vec2);

};


#endif  // once




