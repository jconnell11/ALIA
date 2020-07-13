// jhcGenVSrc_reg.cpp : links file extensions to video classes in libraries
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2005-2013 IBM Corporation
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

// Defines links between file extensions and video/image classes.
// Not needed for normal applications, only required for libraries.
// For this file to have any effect, define the global macro _LIB.

#ifdef _LIB

#include "Video/jhcGenVSrc.h"


// Depending on the application you may want to comment out some of these
// classes or add additional new video or image classes.

#include "Video/jhcAsxVSrc.h"
#include "Video/jhcAviVSrc.h"
#include "Video/jhcBwVSrc.h"
#include "Video/jhcDxVSrc.h"
#include "Video/jhcListVSrc.h"
#include "Video/jhcLiveVSrc.h"
#include "Video/jhcMpegVSrc.h"
#include "Video/jhcWmVSrc.h"
//#include "Video/jhcKinVSrc.h"

#include "Video/jhcAviVSink.h"


//= Method called by jhcGenVSrc constructor when compiled for a static library.
// Forces global variables to be evaluated and hence allows types to register.
// The macros JREG_X from jhcVidReg.h create variables like "jvreg_CLASS".

int jhcGenVSrc::RegisterAll ()
{
  return(
    // source types
    jvreg_jhcAsxVSrc   +
    jvreg_jhcAviVSrc   +
    jvreg_jhcBwVSrc    +
    jvreg_jhcDxVSrc    +
    jvreg_jhcListVSrc  +
    jvreg_jhcLiveVSrc  +
    jvreg_jhcMpegVSrc  +
    jvreg_jhcWmVSrc    +
//    jvreg_jhcKinVSrc   +

    // sink types
    jvreg_jhcAviVSink  +
    0);
}


#endif  // _LIB