// freco_nkr.h : get a 256 feature vector from a 100x100 RGB face
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018 IBM Corporation
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

#ifndef _FRECO_NKR_DLL_
#define _FRECO_NKR_DLL_


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifdef FRECO_NKR_EXPORTS
    #define DEXP __declspec(dllexport)
  #else
    #define DEXP __declspec(dllimport)
  #endif
#endif


// link to library stub

#ifndef FRECO_NKR_EXPORTS
  #pragma comment(lib, "freco_nkr.lib")
#endif


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= String with version number of DLL and possibly other information.

extern "C" DEXP const char *freco_version ();


//= Sets up DNN for processing a series of cropped mugshots.
// if file name is NULL then default values will be used
// can use either main CPU or GPU (if compatible)
// does the time-consuming loading of GPU once ahead of time
// returns 2 if running GPU mode, 1 for CPU mode, 0 or negative for failure

extern "C" DEXP int freco_setup (const char *fname =0, int gpu =1);


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


//= Releases any allocated resources (call at end of run).

extern "C" DEXP void freco_cleanup ();


///////////////////////////////////////////////////////////////////////////
//                          Mugshot Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Returns width of representative color face image.

extern "C" DEXP int freco_mug_w ();


//= Returns height of representative color face image.

extern "C" DEXP int freco_mug_h ();


//= Convert input region of interest into normalized color face image.
// mug is pointer to output image buffer of size freco_mug_w x freco_mug_h x 3
// src is bottom-up BGR image with lines padded to 4 byte boundaries
// iw is image width, ih is image height, assumes 3 fields (color)
// face bounding box has lf <= x <= rt and bot <= y <= top in pixels
// potentially rotates and enhances some portion of input image
// returns 1 if successful, 0 or negative if error

extern "C" DEXP int freco_mug (unsigned char *gray, 
                               const unsigned char *src, int iw, int ih, 
                               int lf, int rt, int bot, int top);


//= Reports coordinates of left eye (wrt person) in original input image.
// returns 1 if found, 0 if missing or not computed

extern "C" DEXP int freco_eye_lf (double& x, double& y);


//= Reports coordinates of right eye (wrt person) in original input image.
// returns 1 if found, 0 if missing or not computed

extern "C" DEXP int freco_eye_rt (double& x, double& y);


///////////////////////////////////////////////////////////////////////////
//                         Signature Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Returns the number of elements in an example signature vector. 

extern "C" DEXP int freco_vsize ();


//= Analyze "src" image to generate "feat" vector (blocks until complete).
// source image is bottom-up 8 bit BGR order with width = iw, height = ih
// face bounding box has lf <= x <= rt and bot <= y <= top in pixels
// passed feature array must have 256 elements for DNN output
// must call freco_setup before (even with default parameters)
// about 67 ms on 2.7 GHz CPU, 12.5 ms on Quadro M1000M
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int freco_vect (double *feat, const unsigned char *src, 
                                int iw, int ih, int lf, int rt, int bot, int top);


//= Request that a face in an image be analyzed in the background.
// source image is bottom-up 8 bit BGR order with width = iw, height = ih
// face bounding box has lf <= x <= rt and bot <= y <= top in pixels
// will cancel any other request that might be in progress
// must call freco_setup before (even with default parameters)
// typically 0.3 ms on 2.7 GHz CPU - src image can change after return
// returns 1 if successfully queued, 0 or negative for some error

extern "C" DEXP int freco_submit (const unsigned char *src, int iw, int ih, 
                                  int lf, int rt, int bot, int top);


//= Wait a certain amount to see if face analysis request has completed.
// passed feature array must have 256 elements for DNN output
// returns 1 if done (and sets vector), 0 if not ready yet, negative for error

extern "C" DEXP int freco_check (double *feat, int ms =0);


//= Computes a distance between two signature vectors (small is good).
// based on average of cosine distance and normalized correlation
// passed arrays must have 256 elements each
// return value between 0 and 1 (ID threshold around 0.2)

extern "C" DEXP double freco_dist (const double *f1, const double *f2);


///////////////////////////////////////////////////////////////////////////

#endif  // once