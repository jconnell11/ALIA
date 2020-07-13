// jhcOcvVSrc.cpp : reads videos using OpenCV infrastructure
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

// Linker additional library directories must have:
//   ../../OpenCV/build/x64/vc10/staticlib
// Note: ALWAYS needs opencv_ffmpeg245_64.dll !!!

#ifndef _DEBUG
  // needed for DLL or static libs
  #pragma comment(lib, "opencv_core245.lib")       
  #pragma comment(lib, "opencv_highgui245.lib")

  // needed for static libs only
  #pragma comment(lib, "zlib.lib")                 
  #pragma comment(lib, "IlmImf.lib")
  #pragma comment(lib, "libjpeg.lib")
  #pragma comment(lib, "libjasper.lib")
  #pragma comment(lib, "libpng.lib")
  #pragma comment(lib, "libtiff.lib")
#else
  // needed for DLL or static libs
  #pragma comment(lib, "opencv_core245d.lib")   
  #pragma comment(lib, "opencv_highgui245d.lib")

  // needed for static libs only
  #pragma comment(lib, "zlibd.lib")                 
  #pragma comment(lib, "IlmImfd.lib")
  #pragma comment(lib, "libjpegd.lib")
  #pragma comment(lib, "libjasperd.lib")
  #pragma comment(lib, "libpngd.lib")
  #pragma comment(lib, "libtiffd.lib")
#endif

// extra Windows libraries
#pragma comment(lib, "Comctl32.lib")            
#pragma comment(lib, "vfw32.lib")


///////////////////////////////////////////////////////////////////////////

// C++ include directoires must have:
//   ../../OpenCv/sources/include
//   ../../OpenCV/build/include
//   ../../OpenCV/3rdparty/include/ffmpeg_

#include <windows.h>                         // needed for mutex
#include <process.h>                         // for thread
#include <string.h>

#include "opencv2/opencv.hpp"

#include "Interface/jhcMessage.h"
#include "Interface/jms_x.h"

#include "Video/jhcOcvVSrc.h"


//////////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                          //
//////////////////////////////////////////////////////////////////////////////

// "ocv" is a dummy MIME-dispatch extension which gets stripped off

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

JREG_VURL(jhcOcvVSrc, "ocv");

#endif


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcOcvVSrc::~jhcOcvVSrc ()
{
  HANDLE mutex = (HANDLE) lock, ready = (HANDLE) done;
  cv::VideoCapture *capture = (cv::VideoCapture *) cap;
  cv::Mat *frame;
  int i;

  // stop capturing and deallocate thread control items
  Prefetch(0);
  CloseHandle(mutex);
  CloseHandle(ready);

  // deallocate OpenCV items
  delete capture;
  for (i = 0; i < bsz; i++)
  {
    frame = (cv::Mat *) buf[i];
    delete frame;
  }
}


//= Default constructor initializes certain values.

jhcOcvVSrc::jhcOcvVSrc (const char *name, int index)
{
  cv::VideoCapture *capture;
  int i;

  // save details of source
  strcpy_s(kind, "jhcOcvVSrc");
  ParseName(name);
  ok = 0;

  // make up thread control items
  lock = (void *) CreateMutex(NULL, false, NULL);
  done = (void *) CreateEvent(NULL, TRUE, FALSE, NULL);
  bg = NULL;
  run = 0;

  // make up OpenCV capture object and array of frames 
  capture = new cv::VideoCapture;
  cap = (void *) capture;
  for (i = 0; i < bsz; i++)
    buf[i] = (void *) new cv::Mat;

  // try opening source
  if (!capture->open(Trimmed()))
    return;
  if (!capture->isOpened())
    return;
  ok = 1;

  // read size of images and get framerate
  w = ROUND(capture->get(CV_CAP_PROP_FRAME_WIDTH));
  h = ROUND(capture->get(CV_CAP_PROP_FRAME_HEIGHT));
  d = 3;
  freq = capture->get(CV_CAP_PROP_FPS);

  // set maximum staleness for newly captured frames
  lag = ROUND(2.0 * 1000.0 / freq);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Start background thread reading images from stream.

void jhcOcvVSrc::Prefetch (int doit)
{
  HANDLE backg = (HANDLE) bg, ready = (HANDLE) done;
  int i;

  if ((doit > 0) && (run <= 0))
  {
    // clear frame times
    for (i = 0; i < bsz; i++)
      tdec[i] = 0;
    fill = 0;

    // start background loop
    run = 1;
    ResetEvent(ready);
    bg = (void *) _beginthreadex(NULL, 0, grab_backg, this, 0, NULL);
  }
  else if ((doit <= 0) && (run > 0))
  {
    // signal background thread to end then wait for exit
    run = 0;
    WaitForSingleObject(backg, 1000);
    CloseHandle(backg);
    bg = NULL;
  }
}


//= Read next frame from already open stream.

int jhcOcvVSrc::iGet (jhcImg &dest, int *advance, int src, int block)
{
  HANDLE mutex = (HANDLE) lock, ready = (HANDLE) done;
  cv::Mat *dump;
  DWORD tnow;
  int attempt, i, j, ans = 0;

  // make sure class was setup properly
  if (ok < 1)
    return 0;
  Prefetch(1);

  // possible retry a few times if bad frame received
  for (attempt = 0; (attempt < 10) && (ans <= 0); attempt++)
  {
    // wait for a new frame to be ready then lock buffers
    if (WaitForSingleObject(ready, 5000) != WAIT_OBJECT_0)
      return Complain("No new frame ready in jhcOcvVSrc::iGet");
    if (WaitForSingleObject(mutex, 1000) != WAIT_OBJECT_0)
      return Complain("Failed to get buffer lock in jhcOcvVSrc::iGet");

    // figure out which frame in circular buffer to read
    tnow = jms_now();
    for (i = 1; i < bsz; i++)
    {
      // find oldest unread that is within "lag" ms of current time
      j = (fill + i) % bsz;
      if (tdec[j] == 0)
        continue;
      if (jms_diff(tnow, tdec[j]) <= lag)
        break;
      tdec[j] = 0;                     // frame unusable
    }

    // see if all cached frames now used up
    if (i >= (bsz - 1))
      ResetEvent(ready);  

    // check that a suitable frame was found
    if (i < bsz)
    {
      tdec[j] = 0;                     // prevent re-selection 
      dump = (cv::Mat *) buf[j];
      if (dump->data != NULL)
        ans = 1;
    }
    
    // minimize buffer lock time so no UDP packets dropped
    ReleaseMutex(mutex);
  }

  // image copy de facto guarded by long ring buffer
  if (ans > 0)
    dest.LoadFlip(dump->data);
  return ans;
}


//= Continuously grab frames into buffers (run as a separate thread).

int jhcOcvVSrc::grab_loop ()
{
  HANDLE mutex = (HANDLE) lock, ready = (HANDLE) done;
  cv::VideoCapture *capture = (cv::VideoCapture *) cap;
  cv::Mat *frame;

  while (run > 0)
  {
    // get a frame into current array slot
    frame = (cv::Mat *) buf[fill];
    if (!capture->read(*frame))
      return 0;

    // lock buffers then save time and advance pointer
    if (WaitForSingleObject(mutex, 1000) != WAIT_OBJECT_0)
      return Complain("Failed to get buffer lock in jhcOcvVSrc::grab_loop");
    tdec[fill] = jms_now();
    fill = (fill + 1) % bsz;

    // signal new frame is available and unlock buffers
    SetEvent(ready);
    ReleaseMutex(mutex); 
  }
  return 1;   
}

