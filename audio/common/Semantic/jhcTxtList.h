// jhcTxtList.h : singly linked list of strings
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2020 IBM Corporation
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

#ifndef _JHCTXTLIST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTXTLIST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <string.h>


//= Helper class for singly linked list of strings.
// this class should never be exposed outside jhcTxtAssoc
// weight (saved) can be used as probability for choosing
// mark can be used for enumeration or non-return inhibition
// Note: rest of list must be deallocated explicitly

class jhcTxtList
{
// PRIVATE MEMBER VARIABLES
private:
  char txt[200];       /** Name of list element. */
  double wt;           /** Degree of membership. */
  int vlen;            /** Length of string val. */
  jhcTxtList *next;    /** Remainder of list.    */


// PUBLIC MEMBER VARIABLES
public:
  int mark;            /** Run-time bookkeeping. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTxtList () {*txt = '\0'; vlen = 0; wt = 1.0; next = NULL; mark = 0;}

  // read only access
  const char *ValTxt () const {return txt;}
  int ValLen () const {return vlen;}
  double ValWt () const {return wt;}
  const jhcTxtList *NextVal () const {return next;}

  // modification
  void SetTxt (const char *vtxt) 
    {strncpy_s(txt, vtxt, 200); txt[199] = '\0'; vlen = (int) strlen(txt);}
  void SetWt (double amt) {wt = amt;}
  void IncWt (double amt =1.0) {wt += amt;}
  void SetNext (jhcTxtList *v) {next = v;}
  jhcTxtList *GetNext () {return next;}


};


#endif  // once




