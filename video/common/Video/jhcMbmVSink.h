// jhcMbmVSink.h : saves images as a simple concatenated bitmap video file
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2009 IBM Corporation
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

#ifndef _JHCMBMVSINK_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMBMVSINK_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>

#include "Video/jhcVideoSink.h"


//= Saves images as a simple concatenated bitmap video file.
// MBM = Motion BitMap (like motion JPEG)
// format:
//   MBM   = type marker (ASCII)
//   3     = three bytes per pixel (ASCII)
//   640   = width (unsigned short)
//   480   = height (unsigned short)
//   30000 = frames per second (x1000 = unsigned long)
//   nnnn  = total count of frames in file (unsigned long)
//   <D1>  = first frame data (e.g. 640 * 480 * 3 = 921600 bytes)
//   <D2>  = second frame data (same fixed size)
//   ...   = rest of frames
// all lines are padded to multiples of 4 bytes (e.g. 750 * 3 -> 2252)

class jhcMbmVSink : public jhcVideoSink
{
// PRIVATE MEMBER VARIABLES
private:
  FILE *out;
  int bsize, frames;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcMbmVSink (const char *fname =NULL);
  ~jhcMbmVSink ();
 

// PRIVATE MEMBER FUNCTIONS
private:
  void iClose ();             
  int iOpen ();               
  int iPut (jhcImg& src);     

  int write_hdr (FILE *dest);

};


#endif  // once




