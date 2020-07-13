// freco.h : generic functions provided by face recognition package
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014 IBM Corporation
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

// Simple programming example of recognition:
// Note: freco does NOT hold database (allows more flexibility)
//
//    // local variables
//    double *p
//    double **g;
//    double dist, best;
//    int sz, i, win = -1;
//  
//    // configure and start system
//    freco_setup(cfg_file);
//    freco_load(metric_file);
//    freco_start();
//  
//    // create signature vectors including database
//    sz = freco_size();
//    p = new double[sz];
//    g = new (double *)[3];
//    for (i = 0; i < 3; i++)
//      g[i] = new double [sz];
//
//    // load gallery face images into database
//    freco_vect(g[1], gallery1);
//    freco_vect(g[2], gallery2);
//    freco_vect(g[3], gallery3);
//  
//    // load probe face image and compare to gallery database
//    freco_vect(p, probe);
//    for (i = 0; i < 3; i++)
//    {
//      dist = freco_dist(p, g[i]);
//      if ((win < 0) || (dist < best))
//      {
//        // save index of gallery image with smallest distance
//        win = i;
//        best = dist;
//      }
//    }
//  
//    // tell results
//    printf("Probe face most similar to gallery %d (dist = %4.2f)\n", win, best);
//
//    // cleanup (including database)
//    freco_done();
//    freco_cleanup();
//    for (i = 0; i < 3; i++)
//      delete [] g[i];
//    delete [] g;
//    delete [] p;
//

/////////////////////////////////////////////////////////////////////////////

#ifndef _FRECO_
#define _FRECO_

#include <stdlib.h>            // needed for NULL


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef FRECO_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


// link to library stub

#ifndef FRECO_EXPORTS
  #ifdef _DEBUG
    #pragma comment(lib, "freco_d.lib")
  #else
    #pragma comment(lib, "freco.lib")
  #endif
#endif


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of processing code.
// returns pointer to input string for convenience

extern "C" DEXP const char *freco_version (char *spec) const;


//= Loads all configuration and calibration data from a file.
// if this function is not called, default values will be used for all parameters
// returns positive if successful, 0 or negative for failure

extern "C" DEXP int freco_setup (const char *fname);


//= Loads comparison metric derived from training samples.
// returns 1 if successful, 0 or negative for error 

extern "C" DEXP int freco_metric (const char *fname);


//= Start the face recognition system for training or identification.
// takes a debugging level specification and log file designation
// use this to initially fire up the system 
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int freco_start (int level =0, const char *log_file =NULL);


//= Call at end of run to close log file or before loading a new configuration.

extern "C" DEXP void freco_done ();


//= Releases any allocated resources (call at very end of program before exit).

extern "C" DEXP void freco_cleanup ();


///////////////////////////////////////////////////////////////////////////
//                       Normalization Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Returns expected width of input grayscale face image.

extern "C" DEXP int freco_wid () const;


//= Returns expected width of input grayscale face image.

extern "C" DEXP int freco_ht () const;


//= Convert input region of interest into normalized grayscale face image.
// gray is pointer to output imag buffer of size freco_wid x freco_ht
// img is bottom-up BGR image with lines padded to 4 byte boundaries
// w is image width, h is image height, f is number of fields (3 = color)
// rx and ry define lower left corner of patch with size rw x rh
// returns 1 if successful, 0 or negative if error

extern "C" DEXP int ffreco_norm (unsigned char *gray, 
                                 const unsigned char *img, int w, int h, int f, 
                                 int rx, int ry, int rw, int rh);


//= Reports coordinates of left eye (wrt person) in original input image.
// returns 1 if found, 0 if missing or not computed for normalization

extern "C" DEXP int freco_eye_lf (double& x, double& y) const;


//= Reports coordinates of right eye (wrt person) in original input image.
// returns 1 if found, 0 if missing or not computed for normalization

extern "C" DEXP int freco_eye_rt (double& x, double& y) const;


///////////////////////////////////////////////////////////////////////////
//                         Signature Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Returns the number of elements in an example signature vector. 
// returns negative for error

extern "C" DEXP int freco_size () const;


//= Computes a signature vector given a cropped grayscale face image.
// image is 8 bit gray scanned left-to-right, bottom up
// returns negative for error, else vector size

extern "C" DEXP int freco_vect (const unsigned char *gray) const;


//= Computes a distance between two signature vectors (small is good).
// assumes signatures are both of correct length

extern "C" DEXP double freco_dist (const float *probe, const float *gallery) const;



///////////////////////////////////////////////////////////////////////////

#endif  // once