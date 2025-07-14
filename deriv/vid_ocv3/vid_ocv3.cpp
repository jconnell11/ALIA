// vid_ocv3.cpp : simple interface for reading videos using OpenCV 3.4.5
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

#pragma comment(lib, "opencv_world345.lib")

#include <windows.h>
#include <direct.h>               // for _getcwd in Windows

#include "opencv2/opencv.hpp"

#include "vid_ocv3.h"


///////////////////////////////////////////////////////////////////////////
//                          Global Variables                             //
///////////////////////////////////////////////////////////////////////////

//= Version information string.

static char info[80];


//= Video capture instance.

static cv::VideoCapture vcap;


//= Frame to capture image into.

static cv::Mat img, bot;


///////////////////////////////////////////////////////////////////////////
//                      Initialization and Locking                       //
///////////////////////////////////////////////////////////////////////////

//= Clean up on exit.

BOOL APIENTRY DllMain (HANDLE hModule,
                       DWORD ul_reason_for_call, 
                       LPVOID lpReserved)
{
  if (ul_reason_for_call == DLL_PROCESS_DETACH)
    vid_ocv3_close();
  return TRUE;
}


///////////////////////////////////////////////////////////////////////////
//                             Information                               //
///////////////////////////////////////////////////////////////////////////

//= String with version number of DLL and possibly other information.

extern "C" DEXP const char *vid_ocv3_version ()
{
  sprintf_s(info, "vid_ocv3 %4.2f", 1.02);
  return info;
}


//= Returns image width in pixels of currently bound video source.

extern "C" DEXP int vid_ocv3_w ()
{
  cv::Size sz; 

  if (!vcap.isOpened())
    return 0;
  sz = img.size();
  return sz.width;
}


//= Returns image height in pixels of currently bound video source.

extern "C" DEXP int vid_ocv3_h ()
{
  cv::Size sz; 

  if (!vcap.isOpened())
    return 0;
  sz = img.size();
  return sz.height;
}


//= Returns image number of fields (bytes per pixel) of currently bound video source.

extern "C" DEXP int vid_ocv3_nf ()
{
  if (!vcap.isOpened())
    return 0;
  return img.channels();
}


//= Returns the nomimal framerate of the source (better for files than live).

extern "C" DEXP double vid_ocv3_fps ()
{
  if (!vcap.isOpened())
    return 0.0;
  return vcap.get(CV_CAP_PROP_FPS);
}


///////////////////////////////////////////////////////////////////////////
//                           Main Functions                              //
///////////////////////////////////////////////////////////////////////////

//= Tries to open a video source (file or stream) and grabs a test frame.
// only a single source can be active at a time with this DLL
// returns positive if successful, 0 or negative for failure
// NOTE: needs opencv_world345.dll and opencv_ffmpeg345_64.dll to function

extern "C" DEXP int vid_ocv3_open (const char *fname)
{
  if (!vcap.open(fname))
    return -1;
  if (!vcap.read(img))
    return 0;
  return 1;
}


//= Tries to open a local camera for input and grabs a test frame.
// returns positive if successful, 0 or negative for failure
// NOTE: needs opencv_world345.dll and opencv_ffmpeg345_64.dll to function

extern "C" DEXP int vid_ocv3_cam (int unit)
{
  if (!vcap.open(unit))
    return -1;
  if (!vcap.read(img))
    return 0;
  return 1;
}


//= Get next frame into supplied buffer (assumed to be big enough).
// images are left-t-right, bottom-up, with BGR color order
// returns 1 if successful, 0 for problem
// NOTE: initiaties framegrab and blocks until fully decoded

extern "C" DEXP int vid_ocv3_get (unsigned char *buf)
{
  cv::Size sz; 

  if (!vcap.read(img))
    return 0;
  cv::flip(img, bot, 0);
  sz = img.size();
  memcpy(buf, bot.data, sz.width * sz.height * img.channels());
  return 1;
}


//= Disconnect from current video source (automatically called on exit).

extern "C" DEXP void vid_ocv3_close ()
{
  vcap.release();
}

