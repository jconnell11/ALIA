// vid_ocv3.h : simple interface for reading videos using OpenCV 3.4.5
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _VID_OCV3_DLL_
#define _VID_OCV3_DLL_

// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef VID_OCV3_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


// link to library stub

#ifndef VID_OCV3_EXPORTS
  #pragma comment(lib, "vid_ocv3.lib")
#endif


///////////////////////////////////////////////////////////////////////////
//                             Information                               //
///////////////////////////////////////////////////////////////////////////

//= String with version number of DLL and possibly other information.

extern "C" DEXP const char *vid_ocv3_version ();


//= Returns image width in pixels of currently bound video source.

extern "C" DEXP int vid_ocv3_w ();


//= Returns image height in pixels of currently bound video source.

extern "C" DEXP int vid_ocv3_h ();


//= Returns image number of fields (bytes per pixel) of currently bound video source.

extern "C" DEXP int vid_ocv3_nf ();


//= Returns the nomimal framerate of the source (better for files than live).

extern "C" DEXP double vid_ocv3_fps ();


///////////////////////////////////////////////////////////////////////////
//                           Main Functions                              //
///////////////////////////////////////////////////////////////////////////

//= Tries to open a video source (file or stream) and grabs a test frame.
// only a single source can be active at a time with this DLL
// returns positive if successful, 0 or negative for failure
// NOTE: needs opencv_world345.dll and opencv_ffmpeg345_64.dll to function

extern "C" DEXP int vid_ocv3_open (const char *fname);


//= Tries to open a local camera for input and grabs a test frame.
// returns positive if successful, 0 or negative for failure
// NOTE: needs opencv_world345.dll and opencv_ffmpeg345_64.dll to function

extern "C" DEXP int vid_ocv3_cam (int unit);


//= Get next frame into supplied buffer (assumed to be big enough).
// images are left-t-right, bottom-up, with BGR color order
// returns 1 if successful, 0 for problem
// NOTE: initiaties framegrab and blocks until fully decoded

extern "C" DEXP int vid_ocv3_get (unsigned char *buf);


//= Disconnect from current video source (automatically called on exit).

extern "C" DEXP void vid_ocv3_close ();


///////////////////////////////////////////////////////////////////////////

#endif  // once
