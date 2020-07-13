// jms_x.h : common millisecond time functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#ifndef _JMS_X_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JMS_X_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


// elapsed time
void jms_sleep (int ms);
UL32 jms_now ();
int jms_diff (UL32 now, UL32 before);
double jms_secs (UL32 now, UL32 before);
UL32 jms_wait (UL32 tref, int delay);
int jms_resume (UL32 cont);
char *jms_elapsed (char *dest, UL32 tref, int ssz);

// absolute time
char *jms_date (char *dest, int res, int ssz);
char *jms_time (char *dest, int res, int ssz);
bool jms_expired (int mon, int yr, int smon =0, int syr =0);

// convenience
template <size_t ssz> 
  char *jms_elapsed (char (&dest)[ssz], UL32 tref)
    {return jms_elapsed(dest, tref, ssz);}
template <size_t ssz> 
  char *jms_date (char (&dest)[ssz], int res =0) 
    {return jms_date(dest, res, ssz);}
template <size_t ssz> 
  char *jms_time (char (&dest)[ssz], int res =0) 
    {return jms_time(dest, res, ssz);}


#endif  // once




