// jhcKinVSrc.cpp : gets color and depth images from Microsoft Kinect sensor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
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

// Project / Properties / Configuration / Link / Input must have OpenNI2.lib
#ifdef _WIN64
  #pragma comment (lib, "OpenNI2_x64.lib")
#else
  #pragma comment (lib, "OpenNI2.lib")
#endif


#include "Video/OpenNI2.h"
using namespace openni;

#include "Video/jhcKinVSrc.h"


///////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                       //
///////////////////////////////////////////////////////////////////////////

#ifdef JHC_GVID
  #include "Video/jhcVidReg.h"
  JREG_CAM(jhcKinVSrc, "kin kinh");   // "kinh" was "kin2"
#endif


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.
// base file name is which Kinect to use, "kinh" extension means hi-res color

jhcKinVSrc::jhcKinVSrc (const char *filename, int index)
{
	Device *dev;
	VideoStream *color, *depth;
	Array<DeviceInfo> list;
	VideoMode vmode;
  int num, big;

  // make up OpenNI interface objects
  dev = new Device;
  depth = new VideoStream;
  color = new VideoStream;

  // save in member variables
  dev0 = (void *) dev;
  depth0 = (void *) depth;
  color0 = (void *) color;

  // save class name and image source
  strcpy_s(kind, "jhcKinVSrc");
  ParseName(filename);
  num = atoi(BaseName) % 10;
  big = ((IsFlavor("kinh") > 0) ? 1 : 0);
  ok = -6;

  // try to start OpenNI 
  if (OpenNI::initialize() != STATUS_OK)
    return;
  ok = -5;

  // try to find device by number and open it
	OpenNI::enumerateDevices(&list);
  if (num >= list.getSize())
    return;
	if (dev->open(list[num].getUri()) != STATUS_OK)
    return;
  ok = -4;

  // create color and depth streams
  if (depth->create(*dev, SENSOR_DEPTH) != STATUS_OK)
    return;
  if (color->create(*dev, SENSOR_COLOR) != STATUS_OK)
    return;
  ok = -3;

  // set up desired sizes and overall frame rate for color
  w = 640;
  h = 480;
  d = 3;
  freq = 30.0; 
  flen = 525.0;      // 62.7 degs horizontal, 49.1 degs vertical
  dsc  = 1.0; 
  aspect = 1.0;
  
  if (big > 0)
  {
    // hi-res size and speed
    w = 1280;
    h = 960; 
    flen = 1050.0;
    freq = 11.0;
  }
  vmode.setResolution(w, h);
  vmode.setFps(ROUND(freq));
  vmode.setPixelFormat(PIXEL_FORMAT_RGB888);
  if (color->setVideoMode(vmode) != STATUS_OK)
    return;
  ok = -2;

  // set up desired sizes and overall frame rate for depth
  w2 = 640;
  h2 = 480;
  d2 = 2;
  freq2 = 30.0;
  flen2 = 525.0;     // usable about 55 x 42 degs
  dsc2  = 0.9659;    // from depth fitting
  aspect2 = 1.0;
  vmode.setResolution(w2, h2);
  vmode.setFps(ROUND(freq2));
  vmode.setPixelFormat(PIXEL_FORMAT_DEPTH_1_MM);
  if (depth->setVideoMode(vmode) != STATUS_OK)
    return;
  ok = -1;

  // spatially register images and time sync them
  if (dev->setImageRegistrationMode(IMAGE_REGISTRATION_DEPTH_TO_COLOR) != STATUS_OK)
    return;
  if (dev->setDepthColorSyncEnabled(TRUE) != STATUS_OK)
    return;
  ok = 0;

  // start streams
	if (color->start() != STATUS_OK)
    return;
  if (depth->start() != STATUS_OK)
    return;
  if (!color->isValid() && !depth->isValid())
    return;
  ok = 1;
}


//= Default destructor does necessary cleanup.

jhcKinVSrc::~jhcKinVSrc ()
{
	Device *dev = (Device *) dev0;
  VideoStream *color = (VideoStream *) color0;
	VideoStream *depth = (VideoStream *) depth0;

  // stop generating frames then get rid of streams
  color->stop();
  depth->stop();
  color->destroy();
  depth->destroy();

  // release device 
  dev->close();

  // deallocate from heap
  delete color;
  delete depth;
  delete dev;

  // close library
  OpenNI::shutdown();
}


///////////////////////////////////////////////////////////////////////////
//                              Core Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get just the color or depth image from the sensor.
// ignores "block" and always waits for new frame

int jhcKinVSrc::iGet (jhcImg& dest, int *advance, int src, int block)
{
  VideoStream *color = (VideoStream *) color0;
	VideoStream *depth = (VideoStream *) depth0;
  int n;

  if (ok < 1)
    return 0;

  if (src <= 0)
  {
  	// read a new color frame
    if (OpenNI::waitForAnyStream(&color, 1, &n) != STATUS_OK)
      return 0;
    return fill_color(dest);
  }
  else
  {
    // read a new depth frame
    if (OpenNI::waitForAnyStream(&depth, 1, &n) != STATUS_OK)
      return 0;
    return fill_depth(dest);
  }
}   


//= Get the color image (dest) and the depth image (dest2) from the sensor.
// returns 4x depth (z offset, not ray length) with depth and color pixels aligned
// raw depth = 440-10000mm (17.3"-32.8ft) ==> values 1760-40000, invalid = 65535
// use jhcLUT::Night8 for convenient viewing

int jhcKinVSrc::iDual (jhcImg& dest, jhcImg& dest2)
{
	VideoStream *depth = (VideoStream *) depth0;
  int n;

  if (ok < 1)
    return 0;

	// returns images at depth frame rate (30 Hz)
  if (OpenNI::waitForAnyStream(&depth, 1, &n) != STATUS_OK)
    return 0;
  fill_color(dest);
  fill_depth(dest2);
  return 1;
}


//= Copy color info from Kinect into image array.
// always returns 1 for convenience

int jhcKinVSrc::fill_color (jhcImg& dest)
{
  VideoStream *color = (VideoStream *) color0;
  VideoFrameRef frame;
  const RGB888Pixel *s;
  UC8 *d, *d0 = dest.RoiDest(w - 1, h - 1);
  int x, y, ln = dest.Line();

  // get pointer to Kinect buffer
  color->readFrame(&frame);
  s = (const RGB888Pixel *) frame.getData();

  // RGB -> BGR bottom-up with line padding
  // mirroring fails in version 2.2 so reverse line
  for (y = h; y > 0; y--, d0 -= ln)
  {
    d = d0;
    for (x = w; x > 0; x--, d -= 3, s++)
    {
      d[0] = s->b;
      d[1] = s->g;
      d[2] = s->r;
    }
  }
  return 1;
}


//= Copy depth info from Kinect into image array.
// always returns 1 for convenience

int jhcKinVSrc::fill_depth (jhcImg& dest)
{
	VideoStream *depth = (VideoStream *) depth0;
  VideoFrameRef frame;
  const DepthPixel *s;
  US16 *d, *d0 = (US16 *) dest.RoiDest(w2 - 1, h2 - 1);
  int x, y, ln = dest.Line() >> 1;

  // get pointer to Kinect buffer
  depth->readFrame(&frame);
  s = (const DepthPixel *) frame.getData();

  // copy to 16 bit depth image (bottom-up with line padding)
  // takes into account shift due to pixel alignment
  // mirroring fails in version 2.2 so reverse line
  for (y = h2; y > 0; y--, d0 -= ln)
  {
    d = d0;
    for (x = w2; x > 0; x--, d--, s++)
      if (*s == 0)
        *d = 0xFFFF;
      else
        *d = *s << 2;
  }
  return 1;
}


