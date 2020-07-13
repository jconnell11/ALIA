// jhcGetKin2.cpp : basic interface to libfreenect2 library for Kinect 2
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

#include <windows.h>

#include <libfreenect2/logger.h>

#include "Video/jhcGetKin2.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcGetKin2::~jhcGetKin2 ()
{
  Close();
}


//= Default constructor initializes certain values.

jhcGetKin2::jhcGetKin2 ()
{
  dev = NULL;                            // nothing connected
  csrc = NULL;                           // not waiting on any color frames
  dsrc = NULL;                           // not waiting on any depth frames
  libfreenect2::setGlobalLogger(NULL);   // suppress performance messages
  fclose(stderr);                        // ignore libusb errors
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Attempt to connect to a particular enumerated Kinect 2 sensor.
// returns 1 if successful, 0 or negative for problem

int jhcGetKin2::Open (int unit)
{
  int n;

  // check for valid request, close if opened previously
  if ((unit < 0) || (unit >= 10))
    return -2;
  Close();

  // find Kinect 2 sensors and bind requested one
  n = fn2.enumerateDevices();
  if ((n <= 0) || (unit >= n))
    return -1;
	dev = fn2.openDevice(fn2.getDeviceSerialNumber(unit));

  // connect a frame listener then attempt to start the device
  csrc = new libfreenect2::SyncMultiFrameListener(libfreenect2::Frame::Color);
  dev->setColorFrameListener(csrc);
  dsrc = new libfreenect2::SyncMultiFrameListener(libfreenect2::Frame::Depth);
  dev->setIrAndDepthFrameListener(dsrc);
  if (!dev->start())
    return 0;

  // get sensor geometric calibration values (must be done after start)
  ccam = dev->getColorCameraParams();
  dcam = dev->getIrCameraParams();
  xforms();
  return 1;
}


//= Get next range image and optionally a big or small color image.
// always waits for new depth image, sometimes color image ready also
// Note: color camera is about 10 fps in the dark (30 fps if bright)
// assumes fixed size receiving buffers: rng = 960 x 540 x 2, flen = 540.685 
// col = 960 x 540 x 3 (big <= 0) or 1920 x 1080 x 3 (big >= 1)
// images are bottom-up, left to right, BGR or 16 bit depth (mm x 4)
// can optionally rotate both images by 180 for upside-down sensors
// returns 2 if color and depth, 1 if just depth, 0 or negative for problem
// takes about 2.8 ms for small, 3.5 ms for big

int jhcGetKin2::Receive (unsigned char *rng, unsigned char *col, int big, int rot)
{
  int ans = 1;

  // check for valid sensor and bound listeners
  if (dev == NULL)
    return -2;
  if ((csrc == NULL) || (dsrc == NULL))
    return -1;

  // wait for depth image to become available
  if (!dsrc->waitForNewFrame(dfrm, 500))
    return 0;
  if (rng != NULL)
  {
    // get pixels registered with respect to color image
    if (rot <= 0)
      shift_depth(rng, dfrm[libfreenect2::Frame::Depth]->data);
    else
      shift_depth_180(rng, dfrm[libfreenect2::Frame::Depth]->data);
  }
  dsrc->release(dfrm);

  // see if color image is also available 
  if (csrc->hasNewFrame())
  {
    if (!csrc->waitForNewFrame(cfrm, 0))     // blocks even if zero!
      return 0;
    if (col != NULL)
    {
      if (big > 0)
      {
        // copy full resolution color
        if (rot <= 0)
          xfer_big(col, cfrm[libfreenect2::Frame::Color]->data);   
        else
          xfer_big_180(col, cfrm[libfreenect2::Frame::Color]->data);   
      }
      else
      {
        // copy half resolution color
        if (rot <= 0)
          xfer_sm(col, cfrm[libfreenect2::Frame::Color]->data);   
        else
          xfer_sm_180(col, cfrm[libfreenect2::Frame::Color]->data);   
      }
      ans++;                                 // mark as received
    }
    csrc->release(cfrm);
  }
  return ans;
}


//= Release any bound Kinect 2 sensor and associated items.

void jhcGetKin2::Close ()
{
  if (dev != NULL)
  {
    dev->stop();
    dev->close();
    dev = NULL;
  }
  delete csrc;
  delete dsrc;
}


///////////////////////////////////////////////////////////////////////////
//                        Color Image Transfer                           //
///////////////////////////////////////////////////////////////////////////

//= Copy entire color image to buffer.
// rotates by 180 degrees and converts BGRX to BGR
// output is always 1920 x 1080 x 3, takes roughly 1.3 ms

void jhcGetKin2::xfer_big (unsigned char *dest, const unsigned char *src) const
{
  int i;
  UINT64 s0, s1, s2, s3, d0, d1, d2;
  const UINT64 *s = (const UINT64 *) src;
  UINT64 *d = ((UINT64 *) dest) + 777597;           // 3 * (1080 * 1920) / 8 - 3

  // 8 pels at a time - little-endian (Intel) only
  for (i = 259200; i > 0; i--, d -= 3, s += 4)      // 1080 * 1920 / 8
  {
    s0 = s[0];  // xxR1G1B1-xxR0G0B0
    s1 = s[1];  // xxR3G3B3-xxR2G2B2
    s2 = s[2];  // xxR5G5B5-xxR4G4B4
    s3 = s[3];  // xxR7G7B7-xxR6G6B6

    // G5B5:R6G6-B6:R7G7B7
    d0  = (s3 >> 32) & 0x0000000000FFFFFF;      
    d0 |= (s3 << 24) & 0x0000FFFFFF000000;
    d0 |= (s2 << 16) & 0xFFFF000000000000;
    d[0] = d0;

    // B2:R3G3B3-R4G4B4:R5
    d1  = (s2 >> 48) & 0x00000000000000FF;
    d1 |= (s2 <<  8) & 0x00000000FFFFFF00;
    d1 |=  s1        & 0x00FFFFFF00000000; 
    d1 |=  s1 << 56; 
    d[1] = d1;

    // R0G0B0:R1-G1B1:R2G2
    d2  = (s1 >>  8) & 0x000000000000FFFF;                      
    d2 |= (s0 >> 16) & 0x000000FFFFFF0000;
    d2 |=  s0 << 40;
    d[2] = d2; 
  }
}


//= Copy upside-down version of entire color image to buffer.
// rotates by 180 degrees and converts BGRX to BGR
// output is always 1920 x 1080 x 3, takes roughly 1.3 ms

void jhcGetKin2::xfer_big_180 (unsigned char *dest, const unsigned char *src) const
{
  int i;
  UINT64 s0, s1, s2, s3, d0, d1, d2;
  const UINT64 *s = (const UINT64 *) src;
  UINT64 *d = (UINT64 *) dest;

  // 8 pels at a time - little-endian (Intel) only
  for (i = 259200; i > 0; i--, d += 3, s += 4)      // 1080 * 1920 / 8
  {
    s0 = s[0];  // xxR1G1B1-xxR0G0B0
    s1 = s[1];  // xxR3G3B3-xxR2G2B2
    s2 = s[2];  // xxR5G5B5-xxR4G4B4
    s3 = s[3];  // xxR7G7B7-xxR6G6B6

    // G2B2:R1G1-B2:R0G0B0
    d0  =  s0        & 0x0000000000FFFFFF;      
    d0 |= (s0 >>  8) & 0x0000FFFFFF000000;
    d0 |=  s1 << 48;
    d[0] = d0;

    // B5:R4G4B4-R3G3B3:R2
    d1  = (s1 >> 16) & 0x00000000000000FF;
    d1 |= (s1 >> 24) & 0x00000000FFFFFF00;
    d1 |= (s2 << 32) & 0x00FFFFFF00000000; 
    d1 |= (s2 << 24) & 0xFF00000000000000; 
    d[1] = d1;

    // R7G7B7:R6-G6B6:R5G5
    d2  = (s2 >> 40) & 0x000000000000FFFF;                      
    d2 |= (s3 << 16) & 0x000000FFFFFF0000; 
    d2 |= (s3 <<  8) & 0xFFFFFF0000000000;
    d[2] = d2; 
  }
}


//= Copy half-scale color image to buffer.
// rotates by 180 degrees and converts BGRX to BGR
// output is always 960 x 540 x 3, takes roughly 0.6 ms

void jhcGetKin2::xfer_sm (unsigned char *dest, const unsigned char *src) const
{
  int x, y;
  unsigned long s0, s2, s4, s6;
  const unsigned long *s = (const unsigned long *) src;
  unsigned long *d = ((unsigned long *) dest) + 388797;  // 3 * (540 * 960) / 4 - 3

  // 4 pels at a time - little-endian (Intel) only
  for (y = 540; y > 0; y--, s += 1920)                   // skip line
    for (x = 240; x > 0; x--, d -= 3, s += 8)            // every other src
    {
      s0 = s[0];  // xxR0G0B0
      s2 = s[2];  // xxR2G2B2
      s4 = s[4];  // xxR4G4B4
      s6 = s[6];  // xxR6G6B6
      d[0] = (s4 << 24) | ( s6        & 0x00FFFFFF);      // B4:R6G6B6
      d[1] = (s2 << 16) | ((s4 >>  8) & 0x0000FFFF);      // G2B2:R4G4
      d[2] = (s0 <<  8) | ((s2 >> 16) & 0x000000FF);      // R0G0B0:R2
    }
}


//= Copy upside-down version of half-scale color image to buffer.
// converts BGRX to BGR, output is always 960 x 540 x 3

void jhcGetKin2::xfer_sm_180 (unsigned char *dest, const unsigned char *src) const
{
  int x, y;
  unsigned long s0, s2, s4, s6;
  const unsigned long *s = (const unsigned long *) src;
  unsigned long *d = (unsigned long *) dest; 

  // 4 pels at a time - little-endian (Intel) only
  for (y = 540; y > 0; y--, s += 1920)                   // skip line
    for (x = 240; x > 0; x--, d += 3, s += 8)            // every other src
    {
      s0 = s[0];  // xxR0G0B0
      s2 = s[2];  // xxR2G2B2
      s4 = s[4];  // xxR4G4B4
      s6 = s[6];  // xxR6G6B6
      d[0] = (s2 << 24) | ( s0        & 0x00FFFFFF);     // B2:R0G0B0
      d[1] = (s4 << 16) | ((s2 >>  8) & 0x0000FFFF);     // G4B4:R2G2
      d[2] = (s6 <<  8) | ((s4 >> 16) & 0x000000FF);     // R6G6B6:R4
    }
}


///////////////////////////////////////////////////////////////////////////
//                         Depth Registration                            //
///////////////////////////////////////////////////////////////////////////

//= Shift depth map to align it with color image.
// rotates by 180 degrees and converts to 16 bit depth as mm x 4
// result is always 960 x 540 x 2, flen = 540.685, takes roughly 2.3 ms 

void jhcGetKin2::shift_depth (unsigned char *dest, const unsigned char *src) const
{
  int i, j, left, z4;
  double z, sc = 0.5 * ccam.shift_m * ccam.fx;
  const float *s = (const float *) src;
  const double *lf0 = col_lf0;
  const int *raw = raw_off, *bot = col_bot;
  unsigned short *d;

  // initialize all (960 * 540 * 2) depth values to 65535 
  memset(dest, 0xFF, 1036800);

  // scan through all pixels in ideal (undistorted) depth image
  for (i = 217088; i > 0; i--, raw++, lf0++, bot++)
  {
    // get proper depth value (mm) for this pixel
    // undoes lens distortion then checks if value is okay
    if (*raw < 0)
      continue;
    z = s[*raw];
    if (z <= 0.0)
      continue;

    // despeckle by looking for a similar depth neighbor (217088 = 512 * 424)
    // ignores edge wrap, more picky on last line (since nothing below)
    if (((*raw >= (217088 - 1))   || (fabs(z - s[*raw + 1])   > 25.4)) &&
        ((*raw >= (217088 - 512)) || (fabs(z - s[*raw + 512]) > 25.4)))
      continue;    

    // figure out position wrt small-sized color image
    // shift x by disparity (no bounds checking needed)
    if (*bot < 0)
      continue;
    left = (int)((*lf0) - sc / z);
    d = (unsigned short *) dest + ((*bot) + left);

    // spread value as min over 3 x 2 patch of pixels
    z4 = ROUND(4.0 * z);
    for (j = 0; j < 3; j++)
      d[j] = __min(d[j], z4);
    for (j = 960; j < 963; j++)
      d[j] = __min(d[j], z4);
  }
}


//= Shift depth map to align it with upside-down version of color image.
// converts to 16 bit depth as mm x 4, result is always 960 x 540 x 2, flen = 540.685

void jhcGetKin2::shift_depth_180 (unsigned char *dest, const unsigned char *src) const
{
  int i, j, left, z4;
  double z, sc = 0.5 * ccam.shift_m * ccam.fx;
  const float *s = (const float *) src;
  const double *lf0 = col_lf0;
  const int *raw = raw_off, *bot = col_bot;
  unsigned short *d, *dlast = (unsigned short *) dest + 517437;  // (540 * 960 - 1) - 962

  // initialize all (960 * 540 * 2) depth values to 65535 
  memset(dest, 0xFF, 1036800);

  // scan through all pixels in ideal (undistorted) depth image
  for (i = 217088; i > 0; i--, raw++, lf0++, bot++)
  {
    // get proper depth value (mm) for this pixel
    // undoes lens distortion then checks if value is okay
    if (*raw < 0)
      continue;
    z = s[*raw];
    if (z <= 0.0)
      continue;

    // despeckle by looking for a similar depth neighbor (217088 = 512 * 424)
    // ignores edge wrap, more picky on last line (since nothing below)
    if (((*raw >= (217088 - 1))   || (fabs(z - s[*raw + 1])   > 25.4)) &&
        ((*raw >= (217088 - 512)) || (fabs(z - s[*raw + 512]) > 25.4)))
      continue;    

    // figure out position wrt small-sized color image
    // shift x by disparity (no bounds checking needed)
    if (*bot < 0)
      continue;
    left = (int)((*lf0) - sc / z);
    d = dlast - ((*bot) + left);

    // spread value as min over 3 x 2 patch of pixels
    z4 = ROUND(4.0 * z);
    for (j = 0; j < 3; j++)
      d[j] = __min(d[j], z4);
    for (j = 960; j < 963; j++)
      d[j] = __min(d[j], z4);
  }
}


//= Build coordinate transforms and cache results.
// saves in local arrays raw_off, col_x, and col_y
// math could be speeded up, but only runs once

void jhcGetKin2::xforms ()
{
  double sx, sy;
  int x, y, ix, iy;
  double *lf0 = col_lf0;
  int *raw = raw_off, *bot = col_bot;

  // scan through all pixels of ideal (undistorted) depth image
  for (y = 0; y < 424; y++) 
    for (x = 0; x < 512; x++, raw++, lf0++, bot++) 
    {
      // for ideal depth position, figure out where to sample raw depth
      // images are bottom-up, right-to-left (rotated 180 degrees)
      raw_sample(sx, sy, x, y);
      ix = ROUND(sx);
      iy = ROUND(sy);
      if ((ix < 0) || (ix >= 512) || (iy < 0) || (iy >= 424))
        *raw = -1;
      else
        *raw = iy * 512 + ix;        // offset properly inside raw depth

      // for ideal depth position, figure out where to sample color image
      // modify values to give half-sized image rotated 180 degrees
      col_sample(sx, sy, x, y);
      *lf0 = 958.5 - 0.5 * (sx * ccam.fx + ccam.cx);  // 3 pel filter so ROUND(x - 1)
      iy = 539 - (int)(0.5 * sy);    // 2 line filter so ROUND(y - 0.5)
      if ((iy < 0) || (iy >= 539))   // skip top line for filter overhang
        *bot = -1;
      else
        *bot = 960 * iy;             // line offset
    }
}


//= Simulate lens distortion process for depth camera.
// given ideal pixel position, find where it would be in the raw image
// based on formulae in libfreenect2 class "registration" (cf. distort)

void jhcGetKin2::raw_sample (double& sx, double& sy, int x, int y) const
{
  double dx = (x - dcam.cx) / dcam.fx, dy = (y - dcam.cy) / dcam.fy;
  double dx2 = dx * dx, dy2 = dy * dy, r2 = dx2 + dy2, dxdy2 = 2.0 * dx * dy;
  double kr = 1.0 + ((dcam.k3 * r2 + dcam.k2) * r2 + dcam.k1) * r2;

  sx = dcam.fx * (dx * kr + dcam.p2 * (r2 + 2.0 * dx2) + dcam.p1 * dxdy2) + dcam.cx;
  sy = dcam.fy * (dy * kr + dcam.p1 * (r2 + 2.0 * dy2) + dcam.p2 * dxdy2) + dcam.cy;
}


//= Compute registration between ideal depth image and full-sized color.
// given ideal pixel position, find where it would be in the color image 
// based on formulae in libfreenect2 class "registration" (cf. depth_to_color)

void jhcGetKin2::col_sample (double& sx, double& sy, int x, int y) const
{
  double depth_q = 0.01, color_q = 0.002199;        // pre-defined constants
  double mx = (x - dcam.cx) * depth_q, my = (y - dcam.cy) * depth_q;
  double wx =
    (mx * mx * mx * ccam.mx_x3y0) + (my * my * my * ccam.mx_x0y3) +
    (mx * mx * my * ccam.mx_x2y1) + (my * my * mx * ccam.mx_x1y2) +
    (mx * mx * ccam.mx_x2y0) + (my * my * ccam.mx_x0y2) + (mx * my * ccam.mx_x1y1) +
    (mx * ccam.mx_x1y0) + (my * ccam.mx_x0y1) + (ccam.mx_x0y0);
  double wy =
    (mx * mx * mx * ccam.my_x3y0) + (my * my * my * ccam.my_x0y3) +
    (mx * mx * my * ccam.my_x2y1) + (my * my * mx * ccam.my_x1y2) +
    (mx * mx * ccam.my_x2y0) + (my * my * ccam.my_x0y2) + (mx * my * ccam.my_x1y1) +
    (mx * ccam.my_x1y0) + (my * ccam.my_x0y1) + (ccam.my_x0y0);

  sx = (wx / (ccam.fx * color_q)) - (ccam.shift_m / ccam.shift_d);
  sy = (wy / color_q) + ccam.cy;
}
