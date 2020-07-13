// jhc_kin2.h : basic Kinect 2 functions in DLL
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2018 IBM Corporation
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

// DLL compiled with Visual Studio 2015 but can be used with VS2008 code
// requires VS2015 x64 versions of component DLLs:
//   freenect2.dll  lib-usb1.0.dll  turbojpeg.dll  glfw3.dll

/////////////////////////////////////////////////////////////////////////////

#ifndef _KIN2_DLL_
#define _KIN2_DLL_


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef KIN2_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


// link to library stub

#ifndef KIN2_EXPORTS
  #pragma comment(lib, "jhc_kin2.lib")
#endif


///////////////////////////////////////////////////////////////////////////
//                           Main Functions                              //
///////////////////////////////////////////////////////////////////////////

//= String with version number of DLL and possibly other information.

extern "C" DEXP const char *kin2_version ();


//= Connect to a particular physical Kinect 2 sensor.
// returns positive if successful, 0 or negative for failure

extern "C" DEXP int kin2_open (int unit =0);


//= Gets next color and range image from sensor (blocks).
// always waits for new depth image, sometimes color image ready also
// Note: color camera is about 10 fps in the dark (30 fps if bright)
// can pass NULL pointer if some image not needed
// assumes fixed size receiving buffers: rng = 960 x 540 x 2, flen = 540.685 
// col = 960 x 540 x 3 (big <= 0) or 1920 x 1080 x 3 (big >= 1)
// images are bottom-up, left to right, BGR or 16 bit depth (mm x 4)
// can optionally rotate both images by 180 for upside-down sensors
// returns 2 if color and depth, 1 if just depth, 0 or negative for problem

extern "C" DEXP int kin2_rcv (unsigned char *rng, unsigned char *col =NULL, int unit =0, int big =0, int rot =0);


//= Releases a particular Kinect2 sensor (call at end of run).

extern "C" DEXP void kin2_close (int unit =0);


///////////////////////////////////////////////////////////////////////////

#endif  // once