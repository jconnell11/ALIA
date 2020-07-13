// jhcFFindOCV.h : wrapper for OpenCV face finding
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2018 IBM Corporation
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

#ifndef _JHCFFINDOCV_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFFINDOCV_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"
#include "Data/jhcRoi.h"

#include "Face/jhcFFind.h"


//= Wrapper for OpenCV face finding.
// implements functions found in "ffind.h"
// NOTE: needs to have file "lbpcascade_frontalface.xml" available
// From Resource View tab do Add Resource / Custom / type = RCDATA
//   then change the Property ID to IDR_FACE_XML and point to the proper file 
// Generally also have to manually add to project .rc file the line:
//   IDR_FACE_XML            RCDATA                  "res\\lbpcascade_frontalface.xml"
// Must also add file "lbpcascade_frontalface.xml" to Project (e.g. under Resource Files)

class jhcFFindOCV : public jhcFFind
{
// PRIVATE MEMBER VARIABLES
private:
  jhcRoi fbox[100];      /** Coordinates of faces found.  */
  void *cc;              /** OpenCV cascade classifier.   */
  double ver;            /** Current version of code.     */
  int ok;                /** Cascade properly configured. */
  int nface;             /** Number of faces found.       */


// PUBLIC MEMBER VARIABLES
public:
  // module handle for DLL or EXE (0)
  void *hmod;

  // name of XML cascade definition file
  char cname[200];

  // parameters for searching over scales
  jhcParam fps;
  int pals, wlim;
  double pyr;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcFFindOCV ();
  ~jhcFFindOCV ();
  int BindFind (const char *fname) const {return 1;} 
  int FindReady () const {return ok;}

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int SetCascade (const char *fname =NULL);
  int SetCascade (int res_id);

  // configuration functions (override base class virtuals)
  const char *ffind_version (char *spec, int ssz) const;
  int ffind_setup (const char *fname);
  int ffind_start (int level =0, const char *log_file =NULL);

  // main functions (override base class virtuals)
  int ffind_roi (const unsigned char *img, int w, int h, int f, 
                 int rx, int ry, int rw, int rh,
                 int wmin, int wmax, double sc); 
  double ffind_box (int& x, int& y, int& w, int &h, int i) const;
  int ffind_cnt () const {return nface;}


// PRIVATE MEMBER FUNCTIONS
public:
  int find_params (const char *fname);
  int extract_xml (char *fname, int res_id, int ssz) const;


};


#endif  // once




