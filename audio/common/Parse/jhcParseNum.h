// jhcParseNum.h : converts speech string into floating point number
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

#ifndef _jhcParseNum_
/* CPPDOC_BEGIN_EXCLUDE */
#define _jhcParseNum_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcTxtSrc.h"


//= Converts speech string into floating point number.

class jhcParseNum : private jhcTxtSrc
{
// PRIVATE MEMBER VARIABLES
private:
  char zero[5][40], digit[20][40], tens[8][40];


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcParseNum ();
 
  // main functions
  int ConvertNum (double& res, const char *txt);

  // debugging
  void Test ();
  

// PRIVATE MEMBER FUNCTIONS
private:
  int txt_cvt (double& res, const char *txt) const; 
  int build_int (char *token);
  double build_frac (char *token);
  int get_lo (int& more, int& v, const char *token) const;
  int get_hi (int& more, int& v, const char *token) const;
  int get_x10 (int& more, int& v, const char *token) const;
  int get_100 (int& more, int& v, const char *token) const;
  int get_place (int& more, int& v, const char *token) const;
  double get_mult (const char *token) const;
 
};


#endif  // once




