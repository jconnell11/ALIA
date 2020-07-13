// ffind.h : generic functions provided by face finder package
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

// Simple programming example:
//
//    // local variables
//    double sc;
//    int i, n, x, y, w, h;
//
//    // configure and start system
//    ffind_setup(cfg_file)
//    ffind_start();
//
//    // find faces in some image
//    n = ffind_run(img, 640, 480, 3);
//
//    // report results
//    printf("%d faces found\n", n);
//    for (i = 0; i < n; i++)
//    {
//      sc = ffind_box(x, y, w, h, i);
//      printf("  face %d: lower left (%d %d), size (%d %d), score %4.2f\n", i, x, y, w, h, sc);
//    }
//
//    // cleanup
//    ffind_done();
//    ffind_cleanup();


/////////////////////////////////////////////////////////////////////////////

#ifndef _FFIND_
#define _FFIND_


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef FFIND_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


// link to library stub (for DLL or static library)

#ifndef FFIND_EXPORTS
  #ifdef _DEBUG
    #pragma comment(lib, "ffind_d.lib")
  #else
    #pragma comment(lib, "ffind.lib")
  #endif
#endif


///////////////////////////////////////////////////////////////////////////
//                            Configuration                              //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of processing code.
// returns pointer to input string for convenience

extern "C" DEXP const char *ffind_version (char *spec, int ssz =80);


//= Loads all configuration and calibration data from a file.
// if this function is not called, default values will be used for all parameters
// returns positive if successful, 0 or negative for failure

extern "C" DEXP int ffind_setup (const char *fname);


//= Start the face finder system running and await input.
// takes a debugging level specification and log file designation
// use this to initially fire up the system (REQUIRED)
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int ffind_start (int level =0, const char *log_file =NULL);


//= Call at end of run to close log file or before loading new configuration.

extern "C" DEXP void ffind_done ();


//= Releases any allocated resources (call at very end of program before exit).

extern "C" DEXP void ffind_cleanup ();


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

extern "C" DEXP int ffind_run (const unsigned char *img, int w, int h, int f, 
                               int wmin =20, int wmax =0, double sc =0.0);


//= Do face finding in a region of interest with lower left corner (rx ry).
// img is bottom-up BGR image with lines padded to 4 byte boundaries
// w is image width, h is image height, f is number of fields (3 = color)
// rx and ry define lower left corner of patch with size rw x rh
// wmin is the minimum face width to look for (in pixels) 
// wmax is biggest to look for (0 = whole image is okay)
// sc is the minimum face detection score to accept
// returns number of face detections for later examination (i = 0 to n-1)
// Note: Can pass a subimage based on other properties like skintone or depth

extern "C" DEXP int ffind_roi (const unsigned char *img, int w, int h, int f, 
                               int rx, int ry, int rw, int rh,
                               int wmin =20, int wmax =0, double sc =0.0); 


//= Extract bounding box lower left corner and dimensions for some face.
// returns negative if bad index, else score for this face

extern "C" DEXP double ffind_box (int& x, int& y, int& w, int &h, int i);


//= Tell number of faces found by last analysis.

extern "C" DEXP int ffind_cnt ();


///////////////////////////////////////////////////////////////////////////
//                         Convenience Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Fills string of inferred size with version number of processing code.

template <size_t ssz>
  const char *ffind_version (char (&spec)[ssz])
    {return ffind_version(spec, ssz);}


///////////////////////////////////////////////////////////////////////////

#endif  // once