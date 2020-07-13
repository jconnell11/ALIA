// jrand.h : interface to random number generator
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2015 IBM Corporation
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

#ifndef _JRAND_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JRAND_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


void jrand_seed ();
double jrand ();

int jrand_pick (int n);
int jrand_int (int lo, int hi);
double jrand_rng (double lo, double hi);
double jrand_cent (double mid, double dev);
double jrand_norm (double avg, double std);
double jrand_trim (double avg, double std, double lo, double hi);


#endif  // once




