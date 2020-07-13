// jhcFFind.h : base class for generic face finders
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017 IBM Corporation
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

#ifndef _JHCFFIND_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFFIND_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"
#include "Data/jhcParam.h"


//= Base class for generic face finders.
// derived classes generally re-implement virtual functions
// see ffind.h for meaning of low-level functions
// can use with a DLL by just having member fcn call ::fcn
// <pre>
// class tree and parameters:
//
//   Frontal         dps
//     +FFindOCV     fps
//       FFind
//
// </pre>

class jhcFFind
{
// PROTECTED MEMBER VARIABLES
protected:
  char spec[200];


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcFFind ();
  jhcFFind ();
  const char *Version () {return ffind_version(spec);}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL) {ffind_done(); return ffind_setup(fname);}
  int SaveVals (const char *fname) const {return 0;}

  // main functions
  int Reset () {return ffind_start(0, NULL);}
  int FindWithin (jhcRoi& det, const jhcImg& src, const jhcRoi& area, 
                  double fmin =0.3, double fmax =1.0, double sc =0.0);
  int FindBest (jhcRoi& det, const jhcImg& src, int wmin =20, int wmax =0, double sc =0.0);
  int FindAll (const jhcImg& src, int wmin =20, int wmax =0, double sc =0.0);
  double FaceDet (jhcRoi& det, int i) const;


// PUBLIC MEMBER FUNCTIONS - LOW LEVEL
public:
  // configuration (override these)
  virtual const char *ffind_version (char *spec, int ssz) const {*spec = '\0'; return spec;}
  virtual int ffind_setup (const char *fname) {return 0;}
  virtual int ffind_start (int level =0, const char *log_file =NULL) {return 1;}
  virtual void ffind_done () {}
  virtual void ffind_cleanup () {}

  // configuration - convenience
  template <size_t ssz>
    const char *ffind_version (char (&spec)[ssz]) const
      {return ffind_version(spec, ssz);}

  // convenient shorthand
  int ffind_run (const unsigned char *img, int w, int h, int f, int wmin, int wmax, double sc)
    {return ffind_roi(img, w, h, f, 0, 0, w, h, wmin, wmax, sc);}

  // main functions (override these)
  virtual int ffind_roi (const unsigned char *img, int w, int h, int f, 
                 int rx, int ry, int rw, int rh,
                 int wmin, int wmax, double sc) {return 0;} 
  virtual double ffind_box (int& x, int& y, int& w, int &h, int i) const {return 0.0;}
  virtual int ffind_cnt () const {return 0;}


};


#endif  // once




