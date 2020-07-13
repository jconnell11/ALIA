// ftrain.h : generic functions provided by face training package
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

// Simple programming example of training (ftrain accumulates instance data internally):
//
//    // start system
//    ftrain_setup(cfg_file);
//    ftrain_start();
//
//    // ingest training example face images 
//    ftrain_clr();
//    ftrain_add(0, person0a);
//    ftrain_add(0, person0b);
//    ftrain_add(1, person1);
//    ftrain_add(2, person2a);
//    ftrain_add(2, person2b);
//    ftrain_add(2, person2c);
//
//    // build and save metric
//    ftrain_build();
//    ftrain_save(metric_file);
//
//    // cleanup
//    ftrain_done();
//    ftrain_cleanup();
//

/////////////////////////////////////////////////////////////////////////////

#ifndef _FTRAIN_
#define _FTRAIN_

#include <stdlib.h>            // needed for NULL


// function declarations (possibly combined with other header files)

#ifndef DEXP
  #ifndef STATIC_LIB
    #ifdef FTRAIN_EXPORTS
      #define DEXP __declspec(dllexport)
    #else
      #define DEXP __declspec(dllimport)
    #endif
  #else
    #define DEXP
  #endif
#endif


// link to library stub

#ifndef FTRAIN_EXPORTS
  #ifdef _DEBUG
    #pragma comment(lib, "ftrain_d.lib")
  #else
    #pragma comment(lib, "ftrain.lib")
  #endif
#endif


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Fills string with version number of processing code.
// returns pointer to input string for convenience

extern "C" DEXP const char *ftrain_version (char *spec) const;


//= Loads all configuration and calibration data from a file.
// if this function is not called, default values will be used for all parameters
// returns positive if successful, 0 or negative for failure

extern "C" DEXP int ftrain_setup (const char *fname);


//= Start the system for training a face matching metric.
// takes a debugging level specification and log file designation
// use this to initially fire up the system 
// returns 1 if successful, 0 or negative for some error

extern "C" DEXP int ftrain_start (int level =0, const char *log_file =NULL);


//= Call at end of run to close log file or before loading a new configuration.

extern "C" DEXP void ftrain_done ();


//= Releases any allocated resources (call at very end of program before exit).

extern "C" DEXP void ftrain_cleanup ();


///////////////////////////////////////////////////////////////////////////
//                           Metric Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Returns expected width of input grayscale face image.

extern "C" DEXP int ftrain_wid () const;


//= Returns expected width of input grayscale face image.

extern "C" DEXP int ftrain_ht () const;


//= Clears any metric training data accumulated so far.

extern "C" DEXP void ftrain_clr ();


//= Adds an example image and class number to training set.
// image is generally 8 bit gray scanned left-to-right, bottom up
// assumes image is correct size (ftrain_wid x ftrain_ht)
// IMPORTANT: must call ftrain_build to finalize training
// returns the number of training examples so far (0 or negative for error) 

extern "C" DEXP int ftrain_add (int kind, const unsigned char *img);


//= Uses training data to build comparison metric for examples.
// returns the number of training examples used (0 or negative for error) 

extern "C" DEXP int ftrain_build ();


//= Saves comparison metric derived from training samples.
// returns 1 if successful, 0 or negative for error 

extern "C" DEXP int ftrain_save (const char *fname) const;



///////////////////////////////////////////////////////////////////////////

#endif  // once