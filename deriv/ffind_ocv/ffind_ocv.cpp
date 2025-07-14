// ffind_ocv.cpp : functions included in OpenCV face finder DLL
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 IBM Corporation
// Copyright 2024 Etaoin Systems
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

#ifndef __linux__
  #include <windows.h>
#endif

#include "Data/jhcImg.h"               // common video

#include "Face/jhcFFindOCV.h"          // core computational class

#include "Face/ffind_ocv.h"


///////////////////////////////////////////////////////////////////////////
//                          Global Variables                             //
///////////////////////////////////////////////////////////////////////////

//= An instance of the main computational class.

static jhcFFindOCV core;


///////////////////////////////////////////////////////////////////////////
//                      Initialization and Locking                       //
///////////////////////////////////////////////////////////////////////////

//= Entry point for DLL setup.

#ifndef __linux__
  BOOL APIENTRY DllMain (HANDLE hModule,
                         DWORD ul_reason_for_call, 
                         LPVOID lpReserved)
  {
    // clean up on exit
    if (ul_reason_for_call == DLL_PROCESS_DETACH)
      return TRUE;
    if (ul_reason_for_call != DLL_PROCESS_ATTACH)
      return TRUE;

    // system initialization
    core.hmod = (void *) hModule;
    return TRUE;
  }
#endif


///////////////////////////////////////////////////////////////////////////
//                            Configuration                              //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of processing code.
// returns pointer to input string for convenience

extern "C" DEXP_F const char *ffind_version (char *spec, int ssz)
{
  return core.ffind_version(spec, ssz);
}


//= Loads all configuration and calibration data from a file.
// if this function is not called, default values will be used for all parameters
// returns positive if successful, 0 or negative for failure

extern "C" DEXP_F int ffind_setup (const char *fname)
{
  return core.ffind_setup(fname);
}


//= Start the face finder system running and await input.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP_F int ffind_start (int level, const char *log_file)
{
  return core.ffind_start(level, log_file);
}


//= Call at end of run to close log file or before loading new configuration.

extern "C" DEXP_F void ffind_done ()
{
  core.ffind_done();
}


//= Releases any allocated resources (call at very end of program before exit).

extern "C" DEXP_F void ffind_cleanup ()
{
  core.ffind_cleanup();
}


///////////////////////////////////////////////////////////////////////////
//                           Main Functions                              //
///////////////////////////////////////////////////////////////////////////

//= Perform face finding on given image to generate several detections.
// img is bottom-up BGR image with lines padded to 4 byte boundaries
// w is image width, h is image height, f is number of fields (3 = color)
// wmin is the minimum face width to look for (in pixels) 
// wmax is biggest to look for (0 = whole image is okay)
// sc is the minimum face detection score to accept
// returns number of face detections for later examination (i = 0 to n-1)

extern "C" DEXP_F int ffind_run (const unsigned char *img, int w, int h, int f, 
                               int wmin, int wmax, double sc)
{
  return core.ffind_run(img, w, h, f, wmin, wmax, sc);
}


//= Do face finding in a region of interest with lower left corner (rx ry).
// img is bottom-up BGR image with lines padded to 4 byte boundaries
// w is image width, h is image height, f is number of fields (3 = color)
// rx and ry define lower left corner of patch with size rw x rh
// wmin is the minimum face width to look for (in pixels) 
// wmax is biggest to look for (0 = whole image is okay)
// sc is the minimum face detection score to accept
// returns number of face detections for later examination (i = 0 to n-1)
// Note: Can pass a subimage based on other properties like skintone or depth

extern "C" DEXP_F int ffind_roi (const unsigned char *img, int w, int h, int f, 
                               int rx, int ry, int rw, int rh,
                               int wmin, int wmax, double sc)
{
  return core.ffind_roi(img, w, h, f, rx, ry, rw, rh, wmin, wmax, sc);
}


//= Extract bounding box lower left corner and dimensions for some face.
// returns negative if bad index, else score for this face

extern "C" DEXP_F double ffind_box (int& x, int& y, int& w, int &h, int i)
{
  return core.ffind_box(x, y, w, h, i);
}


//= Tell number of faces found by last analysis.

extern "C" DEXP_F int ffind_cnt ()
{
  return core.ffind_cnt();
}


