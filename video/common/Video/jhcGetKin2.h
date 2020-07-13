// jhcGetKin2.h : basic interface to libfreenect2 library for Kinect 2
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

#ifndef _JHCGETKIN2_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGETKIN2_
/* CPPDOC_END_EXCLUDE */

// Build.Settings.Link.General must have libraries freenect2, glfw3, libusb, and turbojpeg
#pragma comment(lib, "freenect2static.lib")
#pragma comment(lib, "glfw3dll.lib")                     // DLL
//#pragma comment(lib, "glfw3.lib")                        // static
#pragma comment(lib, "libusb-1.0.lib")                   // DLL
#pragma comment(lib, "turbojpeg.lib")                    // DLL

#include "jhcGlobal.h"

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>


//= Basic interface to libfreenect2 library for Kinect 2.
// requires matching versions of component DLLs:
//   freenect2.dll  lib-usb1.0.dll  turbojpeg.dll  glfw3.dll

class jhcGetKin2
{
// PRIVATE MEMBER VARIABLES
private:
  // interface to image reader
  libfreenect2::Freenect2 fn2;
  libfreenect2::Freenect2Device *dev;
  libfreenect2::SyncMultiFrameListener *csrc, *dsrc;
  libfreenect2::FrameMap dfrm, cfrm;
  libfreenect2::Freenect2Device::IrCameraParams dcam;
  libfreenect2::Freenect2Device::ColorCameraParams ccam;

  // precomputed registration values (for 512 * 424)
  double col_lf0[217088];
  int raw_off[217088], col_bot[217088];


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcGetKin2 ();
  jhcGetKin2 ();
 
  // main functions
  int Open (int unit);
  int Receive (unsigned char *rng, unsigned char *col =NULL, int big =0, int rot =0);
  void Close ();


// PRIVATE MEMBER FUNCTIONS
private:
  // color image
  void xfer_big (unsigned char *dest, const unsigned char *src) const;
  void xfer_big_180 (unsigned char *dest, const unsigned char *src) const;
  void xfer_sm (unsigned char *dest, const unsigned char *src) const;
  void xfer_sm_180 (unsigned char *dest, const unsigned char *src) const;

  // depth image
  void shift_depth (unsigned char *dest, const unsigned char *src) const;
  void shift_depth_180 (unsigned char *dest, const unsigned char *src) const;
  void xforms ();
  void raw_sample (double& sx, double& sy, int x, int y) const;
  void col_sample (double& sx, double& sy, int x, int y) const;


};


#endif  // once




