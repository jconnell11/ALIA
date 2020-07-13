// jhcKinectIO.cpp : access to Kinect LED, motor, and accelerometer
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

// Build.Settings.Link.General must have library OpenNI.lib
#pragma comment (lib, "OpenNI.lib")


#include "jhcKinectIO.h"

#include "Video/OpenNI.h"
using namespace xn;


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcKinectIO::jhcKinectIO ()
{
  // default values for member variables
  kok = -2;
  dev = NULL;

  // try talking
  Open();
}


//= Default destructor does necessary cleanup.

jhcKinectIO::~jhcKinectIO ()
{
  Close();
}


//= Connect to motor device via USB.
// returns status: 1 = working, 0 or negative for failure

int jhcKinectIO::Open ()
{
  const XnUSBConnectionString *choices;
  XN_USB_DEV_HANDLE device; 
  XnUChar buf[1]; 
  XnUInt32 n;

  // check if already connected else try again
  if ((kok > 0) && (dev != NULL))
    return kok;
  Close();

  // start up USB handling in OpenNI 
  if (xnUSBInit() != XN_STATUS_OK) 
    return -2;

  // get Kinect motor device
  if (xnUSBEnumerateDevices(0x045E, 0x02B0, &choices, &n) != XN_STATUS_OK) 
    return -1;
  if (xnUSBOpenDeviceByPath(choices[0], &device) != XN_STATUS_OK) 
    return -1;
  dev = (void *) device;

  // initialize motor
  if (xnUSBSendControl(device, (XnUSBControlType) 0xc0, 0x10, 0x00, 0x00, buf, sizeof(buf), 0) 
      != XN_STATUS_OK) 
    return Close();
  if (xnUSBSendControl(device, XN_USB_CONTROL_TYPE_VENDOR, 0x06, 0x01, 0x00, NULL, 0, 0)
      != XN_STATUS_OK) 
    return Close();

  // success
  kok = 1;
  return 1;
}


//= Release Kinect motor and OpenNI helpers.
// always returns 0 for convenience

int jhcKinectIO::Close ()
{
  if (dev != NULL)
    xnUSBCloseDevice((XN_USB_DEV_HANDLE) dev); 
  dev = NULL;
  if (kok > 0)
    xnUSBShutdown();
  kok = 0;
  return 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Read tilt of Kinect relative to gravity horizon (in degrees).
// Note: BLOCKS if motor is moving

double jhcKinectIO::Tilt ()
{
  XN_USB_DEV_HANDLE device = (XN_USB_DEV_HANDLE) dev;
  XnUChar info[10];
  XnUInt32 n;
  int i;

  if (kok <= 0)
    return -360.0;

  for (i = 0; i < 1000; i++)
  {
    if (xnUSBReceiveControl(device, XN_USB_CONTROL_TYPE_VENDOR, 0x32, 0, 0x00, info, 10, &n, 0)
        != XN_STATUS_OK) 
      return -360.0;
    if (info[8] != 0x80)
      return(1.0 * (char) info[8]);
    Sleep(1);
  }
  return -360.0;
}


//= Set tilt of Kinect to certain angle relative to gravity horizon.
// Note: does NOT wait for completion

int jhcKinectIO::SetTilt (double degs)
{
  XN_USB_DEV_HANDLE device = (XN_USB_DEV_HANDLE) dev;
  int ang = ROUND(degs);

  if (kok <= 0)
    return 0;

  if (xnUSBSendControl(device, XN_USB_CONTROL_TYPE_VENDOR, 0x31, ang, 0x00, NULL, 0, 0) 
      != XN_STATUS_OK) 
    return 0;
  return 1;
}


//= Set front LED to particular color and maybe blink.
// col: 0 = off, 1 = green, 2 = red, 3 = orange, 

int jhcKinectIO::SetLED (int col, int blink)
{ 
  XN_USB_DEV_HANDLE device = (XN_USB_DEV_HANDLE) dev;
  int c = __max(0, __min(col, 3)), pat = c;

  if (kok <= 0)
    return 0;

  if ((blink > 0) && (c > 0))
    pat += ((c == 3) ? 1 : 4);
  if (xnUSBSendControl(device, XN_USB_CONTROL_TYPE_VENDOR, 0x06, pat, 0x00, NULL, 0, 0) 
      != XN_STATUS_OK) 
    return 0;
  return 1;
}


