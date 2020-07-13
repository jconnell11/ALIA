// jhcTessOCR.h : uses Open Tesseract OCR to find and read diagram call-outs
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013 IBM Corporation
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

#ifndef _JHCTESSOCR_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTESSOCR_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Data/jhcImgIO.h"
#include "Data/jhcTxtBox.h"
#include "Processing/jhcResize.h"      // needed for magnification


//= Uses Open Tesseract OCR to find and read diagram call-outs.
// runs command line version then parses HOCR output file to find strings

class jhcTessOCR : public jhcTxtBox, private jhcResize
{
// PRIVATE MEMBER VARIABLES
private:
  jhcImgIO jio;
  jhcImg big;
  char saved;


// PUBLIC MEMBER VARIABLES
public:
  // configuration
  char prog[200];              /** Where Tesseract is installed.       */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTessOCR ();
  ~jhcTessOCR ();

  // main functions
  int Analyze (const jhcImg& src, double mag =2.9, int psm =6, int norm =1);


// PRIVATE MEMBER FUNCTIONS
private:
  int parse_html (const char *fname, int hmax, double mag, int norm);
  int get_frag (char *txt, FILE *in, int term);


};


#endif  // once




