// jhcFFindDLL.h : maps virtual functions in jhcFFind to DLL calls
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#ifndef _JHCFFINDDLL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCFFINDDLL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Face/ffind_ocv.h"
#include "Face/jhcFFind.h"


//= Maps virtual functions in jhcFFind to DLL calls.
// this can help eliminate OpenCV dependencies

class jhcFFindDLL : public jhcFFind
{
// PUBLIC MEMBER FUNCTIONS - LOW LEVEL
public:
  // configuration 
  const char *ffind_version (char *spec, int ssz) const 
    {return ::ffind_version(spec, ssz);}
  int ffind_setup (const char *fname) 
    {return ::ffind_setup(fname);}
  int ffind_start (int level =0, const char *log_file =NULL) 
    {return ::ffind_start(level, log_file);}
  void ffind_done () 
    {::ffind_done();}
  void ffind_cleanup () 
    {::ffind_cleanup();}

  // main functions
  int ffind_roi (const unsigned char *img, int w, int h, int f, 
                 int rx, int ry, int rw, int rh,
                 int wmin, int wmax, double sc) 
    {return ::ffind_roi(img, w, h, f, rx, ry, rw, rh, wmin, wmax, sc);} 
  double ffind_box (int& x, int& y, int& w, int &h, int i) const 
    {return ::ffind_box(x, y, w, h, i);}
  int ffind_cnt () const 
    {return ::ffind_cnt();}

};


#endif  // once




