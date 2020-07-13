// jtimer.h : poor man's profiler for single-threaded non-recursive code
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2019 IBM Corporation
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

#ifndef _JTIMER_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JTIMER_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdlib.h>    // needed for definition of NULL !


void jtimer_clr ();
void jtimer (int n, const char *fcn =NULL);
void jtimer_x (int n =-1);
int jtimer_rpt (int tree =1, const char *fname =NULL, int full =0);


#endif  // once
