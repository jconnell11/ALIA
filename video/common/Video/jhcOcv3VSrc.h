// jhcOcv3VSrc.h : reads videos using OpenCV 3.4.5 functions in a DLL
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#ifndef _JHCOCV3VSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCOCV3VSRC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Video/jhcVideoSrc.h"


//= Reads videos using OpenCV 3.4.5 functions in a DLL.
// very similar to jhcOcvVSrc (which uses OpenCV 2.4.5 instead)

class jhcOcv3VSrc : public jhcVideoSrc
{
// PRIVATE MEMBER VARIABLES
private:
  // max frames in buffer
  static const int bsz = 15;

  // OpenCV 
  jhcImg buf[bsz];            // video frame buffers
  UL32 tdec[bsz];             // when frame was decoded
  int fill, lag;              // where to write and max latency

  // threading
  void *bg;                   // background grabbing thread
  void *lock, *done;          // thread control items
  int run;                    // terminator for thread


// PUBLIC MEMBER VARIABLES
public:


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcOcv3VSrc ();
  jhcOcv3VSrc (const char *name, int index =0);
 
  // core working functions 
  void Prefetch (int doit =1); 


// PROTECTED MEMBER FUNCTIONS
protected:
  // required functions for jhcVideoSrc base class
  int iGet (jhcImg &dest, int *advance, int src, int block); 


// PRIVATE MEMBER FUNCTIONS
private:
  // continuous frame grabber
  int grab_loop();

  // background thread 
  static unsigned int __stdcall grab_backg (void *inst)
    {jhcOcv3VSrc *me = (jhcOcv3VSrc *) inst; return me->grab_loop();}


};


/////////////////////////////////////////////////////////////////////////////

// part of mechanism for automatically associating class with file extensions

extern int jvreg_jhcOcv3VSrc;


#endif  // once




