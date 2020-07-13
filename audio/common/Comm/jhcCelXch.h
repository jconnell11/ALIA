// jhcCelXch.h : read and write messages in CEL tagged JSON format
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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

#ifndef _JHCCELXCH_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCCELXCH_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Comm/jhcJSON.h"       // common audio

#include "Data/jhcParam.h"      // common video


//= Read and write messages in CEL tagged JSON format.

class jhcCelXch
{
// PRIVATE MEMBER VARIABLES
private:
  char buf[4000];
  void *ctx, *sub, *out;
  int xok;


// PUBLIC MEMBER VARIABLES
public:
  // parameters
  jhcParam xps;
  char host[80], filter[80], sid[80];
  int iport, oport, echo;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcCelXch ();
  ~jhcCelXch ();
  int Open (int ch =3);
  void Close ();
  int PrintCfg () const;
  int Status () const {return xok;} 

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  int Get (jhcJSON *msg, int noisy =0);
  int Push (const char *tag, const jhcJSON *msg, int noisy =0);

  // transcript functions
  int Input (const char *txt, const char *user =NULL, const char *alist =NULL, int noisy =0);


// PRIVATE MEMBER FUNCTIONS
private:
  int xch_params (const char *fname);

};


#endif  // once




