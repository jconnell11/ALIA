// jhcBarcodeX.h : wrapper around core barcode reading class
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2007 IBM Corporation
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

#ifndef _JHCBARCODEX_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBARCODEX_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Data/jhcParam.h"

#include "jhcBarcode.h"


//= Wrapper around core barcode reading class to allow default files, etc.

class jhcBarcodeX : public jhcBarcode
{
// PUBLIC MEMBER VARIABLES
public:
  // slice and bar parameters
  jhcParam sps, eps, bps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcBarcodeX ();
  void SizeFor (jhcImg& ref);

  // processing parameter manipulation 
  void Defaults (const char *fname =NULL);
  void SaveVals (const char *fname);


// PRIVATE MEMBER FUNCTIONS
private:
  void slice_params (const char *fname);
  void edge_params (const char *fname);
  void bar_params (const char *fname);

};


#endif  // once




