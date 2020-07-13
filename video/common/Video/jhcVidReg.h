// jhcVidReg.h : remembers file extensions and classes for videos
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2005-2015 IBM Corporation
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

#ifndef _JHCVIDREG_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCVIDREG_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Video/jhcVideoSrc.h"
#include "Video/jhcVideoSink.h"


/////////////////////////////////////////////////////////////////////////////
//                  Crazy Macros for Registering Classes                   //
/////////////////////////////////////////////////////////////////////////////

//= Register a jhcVideoSrc derived class for a particular file extensions.
#define JREG_VSRC(src, exts)                                                              \
  jhcVideoSrc * new_##src (const char *f, int i){return((jhcVideoSrc *) new src(f, i));}; \
  int jvreg_##src = jvreg.RegReader(new_##src, exts, 0); 


//= Register a jhcVideoSrc derived class for a web URL with particular file extensions.
#define JREG_VURL(src, exts)                                                              \
  jhcVideoSrc * new_##src (const char *f, int i){return((jhcVideoSrc *) new src(f, i));}; \
  int jvreg_##src = jvreg.RegReader(new_##src, exts, 1); 


//= Register a jhcVideoSrc derived framegrabber class for given tags.
#define JREG_CAM(cam, exts)                                                            \
  jhcVideoSrc * new_##cam (const char *f, int i){return((jhcVideoSrc *) new cam(f));}; \
  int jvreg_##cam = jvreg.RegReader(new_##cam, exts, 2); 


//= Register a jhcVideoSink derived class for a particular file extensions.
#define JREG_VSINK(sink, exts)                                                      \
  jhcVideoSink * new_##sink (const char *f){return((jhcVideoSink *) new sink(f));}; \
  int jvreg_##sink = jvreg.RegWriter(new_##sink, exts); 


/////////////////////////////////////////////////////////////////////////////
/* CPPDOC_BEGIN_EXCLUDE */

// maximum number of extensions that can be registered

const int JVREG_MAX = 100;


/* CPPDOC_END_EXCLUDE */


//= Maps from extension or filename to appropriate video source classes.
// strips any final "+" character off extension to determine type
// full name including "*" and "+" passed to underlying class constructor
// derived video classes should provide a constructor that takes a filename
// non-framegrabber source constructors should also take an index request argument
// NOTE: class never explicity initialized, but assumes nrd and nwr start as zero

class jhcVidReg
{
private:
  int nrd, nwr, reg;                                      // total number of kinds
  int mode[JVREG_MAX];                                    // file reader, URL, or camera
  char rdtag[JVREG_MAX][10];                              // the extensions (no ".")
  jhcVideoSrc *(*read[JVREG_MAX])(const char *f, int i);  // array of ptrs to fcns
  char wrtag[JVREG_MAX][10];                              // the extensions (no ".")
  jhcVideoSink *(*write[JVREG_MAX])(const char *f);       // array of ptrs to fcns
  char filter[80 + 7 * JVREG_MAX];                        // dialog box filter string

public:
  int Kinds (int wr =0){return((wr > 0) ? nwr : nrd);};  /** How many extensions known. **/
  int Known (const char *fname, int wr =0);
  int Camera (const char *fname);
  const char *FilterTxt (int wr =0, int *len =NULL);
  const char *ListAll (int wr =0);

  int RegReader (jhcVideoSrc *fcn (const char *f, int i), const char *exts, int m =0);
  int RegWriter (jhcVideoSink *fcn (const char *f), const char *exts);
  jhcVideoSrc *Reader (const char *fname, int index =0, const char *hint =NULL);
  jhcVideoSink *Writer (const char *fname, const char *hint =NULL);

private:
  int find_index (const char *fname, int wr);
  void chk_lists ();

};


//= Global variable embodying unique registry for video classes.

extern jhcVidReg jvreg;



#endif




