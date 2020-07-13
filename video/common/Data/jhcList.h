// jhcList.h : mix-in to allow list of items to be displayed
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2007 IBM Corporation
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

#ifndef _JHCLIST_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCLIST_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include <stdlib.h>     // for definition of NULL !


//= Mix-in to allow list of items to be displayed.
// maintains state element telling current place in list
// the first item can always be requested by giving index 0
// to use with jhcPickList you must override the virtual functions 
// make sure to bind "reported" to value to be returned from jhcPickList

class jhcList
{
protected:
  int reported;      /** Value associated with last string retrieved. */

public:
  int recent;        /** Sticky default selection -- most recently edited. */

public:
  jhcList () {reported = -1; recent = -1;}             /** Default constructor */
  int LastVal () {return reported;}                    /** Value associated with last item requested.  */
  virtual const char *NextItem (int n) {return NULL;}  /** Return descriptor for certain item in list. */
  virtual int AddItem (const char *desc) {return -1;}  /** Add an item and return its index.           */

};


#endif




