// jhcSwapBody.cpp : body components with buffering of sensors and commands
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024-2026 Etaoin Systems
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

#include "Interface/jprintf.h"         // common video

#include "Body/jhcSwapBody.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcSwapBody::~jhcSwapBody ()
{
  pthread_mutex_destroy(&io_lock);
}


//= Default constructor initializes certain values.

jhcSwapBody::jhcSwapBody ()
{
  // set pose vector sizes
  pos_r.SetSize(4);
  dir_r.SetSize(4);
  pos_c.SetSize(4);
  dir_c.SetSize(4);
  pos_a.SetSize(4);
  dir_a.SetSize(4);

  // set buffer vector sizes
  pos_r0.SetSize(4);
  dir_r0.SetSize(4);
  pos_c0.SetSize(4);
  dir_c0.SetSize(4);
  pos_a0.SetSize(4);
  dir_a0.SetSize(4);

  // make up depth sensor input images
  img_r.SetSize(640, 480, 2);
  img_c.SetSize(640, 480, 3);
  img_a.SetSize(640, 480, 3);
  img_r0.SetSize(img_r);
  img_c0.SetSize(img_c);
  img_a0.SetSize(img_a);
  raw.SetSize(img_c);

  // exclusive data access
  pthread_mutex_init(&io_lock, NULL);

  // helper arrays for TOF image expansion
  tof_sampling();
}


//= Build arrays of sampling coordinate for each destination value.
// dest:   -5   -4   -3   -2   -1 |  0    1    2    3    4 |  5    6    7    8    9 |  A    B    C    D    E |  F
//    x:  -1.4 -1.2 -1.0 -0.8 -0.6|-0.4 -0.2  0.0  0.2  0.4| 0.6  0.8  1.0  1.2  1.4| 1.6  1.8  2.0  2.2  2.4| 2.6
//   ix:   -1   -1   -1   -1   -1 |  0'   0'   0    0    0 |  0    0    1    1    1 |  1    1    2    2    2 |  3
//  src:             -1           |            0           |            1           |            2           |
//   fx:                          | 0.0  0.0  0.0  0.2  0.4| 0.6  0.8  0.0  0.2  0.4| 0.6  0.8  0.0  0.0  0.0|
//                                |  *    *                |                        |                 *    * |
// image is 100x100 square pixels, 66.6 degs wide and high (flen = 76.1)
// set rf = 365.2 to fully fill VGA height (2 * atan(240/365.2) = 66.6 degs)

void jhcSwapBody::tof_sampling ()
{
  double x, y, step = 100.0 / 480.0;             // 0.2083
  int dx, dy, ix, iy;

  // determine input sample position for each output row
  for (dy = 0; dy < 480; dy++)
  {
    x = 49.5 + step * (239.5 - dy);
    ix = (int) x;
    sx[dy] = ix;                                 // pixel offset
    if ((ix <= 0) || (ix >= 99))
      fx[dy] = 0;
    else
      fx[dy] = ROUND(256.0 * (x - ix));          // mix next factor
  }

  // determine input sample line offset for each output column
  for (dx = 0; dx < 640; dx++)
  {
    y = 49.5 + step * (319.5 - dx);
    iy = (int) y;
    sy[dx] = 100 * iy;                           // line offset
    if ((iy <= 0) || (iy >= 99))
      fy[dx] = 0;
    else
      fy[dx] = ROUND(256.0 * (y - iy));          // mix next factor
  }
}


///////////////////////////////////////////////////////////////////////////
//                          Parameter Bundles                            //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcSwapBody::Defaults (const char *fname)
{
  int ok = 1;

  ok &= arm0.Defaults(fname);
  ok &= base0.Defaults(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcSwapBody::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= arm0.SaveVals(fname);
  ok &= base0.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                             Configuration                             //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.

void jhcSwapBody::Reset ()
{
  // clear body attitude
  pitch = 0.0;
  roll  = 0.0;

  // clear all poses
  pos_r0.Zero();
  dir_r0.Zero();
  pos_c0.Zero();
  dir_c0.Zero();
  pos_a0.Zero();
  dir_a0.Zero();

  // clear all images
  img_r0.FillArr(0);
  img_c0.FillArr(0);
  img_a0.FillArr(0);
  seen0 = 0;
  Update();

  // reset components
  neck0.Reset();
  arm0.Reset();
  lift0.Reset();
  base0.Reset();
}


///////////////////////////////////////////////////////////////////////////
//                           Overall Attitude                            //
///////////////////////////////////////////////////////////////////////////

//= Cache new accelerometer input from robot (call Update to transfer).
// up is body pitch (pos = front up), ccw is body roll (pos = right up)

void jhcSwapBody::Status (double up, double ccw)
{
  pitch0 = up;
  roll0  = ccw;
}


///////////////////////////////////////////////////////////////////////////
//                              Sensor Poses                             //
///////////////////////////////////////////////////////////////////////////

//= Set depth finder absolute position and orientation.
// y is forward, x is to right, z up from ground, pan 90 = forward, tilt 0 = level

void jhcSwapBody::RangePose (double x, double y, double z, double p, double t, double r)
{
  pos_r0.SetVec3(x, y, z);
  dir_r0.SetVec3(p, t, r);  
}


//= Set main camera absolute position and orientation (usually near range finder).
// y is forward, x is to right, z up from ground, pan 90 = forward, tilt 0 = level

void jhcSwapBody::ColorPose (double x, double y, double z, double p, double t, double r)
{
  pos_c0.SetVec3(x, y, z);
  dir_c0.SetVec3(p, t, r);  
}


//= Set secondary camera absolute position and orientation (sometimes on hand).
// y is forward, x is to right, z up from ground, pan 90 = forward, tilt 0 = level

void jhcSwapBody::AuxCamPose (double x, double y, double z, double p, double t, double r)
{
  pos_a0.SetVec3(x, y, z);
  dir_a0.SetVec3(p, t, r);  
}


///////////////////////////////////////////////////////////////////////////
//                              Input Images                             //
///////////////////////////////////////////////////////////////////////////

//= Ingest range-finder buffer full of depth pixels of some format.
// fmt: 0 = image not ready
//      1 = VGA bottom-up 16 bit (Kinect)
//      2 = VGA top down float (Astra)
//      3 = 100x100 bottom-up 16 bit (TOF cam)
// returns 0 if not accepted, 1 if okay (fmt reset to 0)

int jhcSwapBody::SetRange (const void *pels, int& fmt)
{
  jhcImg *dest = &img_r0;
  int ok = 0;

  // see if ready to be copied (seen: 2 = new, 1 = processable, 0 = none)
  seen0 = 0;
  if ((fmt <= 0) || (pels == NULL))
    return 0;
  seen0 = 2;

  // choose between some common input formats
  if (fmt == 1)
    ok = set_z16_bot(*dest, pels);     // Banzai
  else if (fmt == 2)
    ok = set_f32_top(*dest, pels);     // Wansui
  else if (fmt == 3)
    ok = set_z16_tof(*dest, pels);     // Ganbei

  // check for success
  if (ok <= 0)
    return 0;
  fmt = 0;                             // clear ready flag
  return 1;
}


//= Load depth image with DIB format bottom-up 16 bit data (0.25mm increments).

int jhcSwapBody::set_z16_bot (jhcImg& dest, const void *pels) const
{
  if ((dest.Fields() != 2) || (pels == NULL))
    return 0;
  memcpy(dest.PxlDest(), pels, dest.PxlSize());   // assumes not swizzled
  return 1;
}


//= Load depth image with Astra format top down 32 bit float data (in meters).

int jhcSwapBody::set_f32_top (jhcImg& dest, const void *pels) const
{
  // sanity check
  if ((dest.Fields() != 2) || (pels == NULL))
    return 0;

  // full image
  int x, y, w = dest.XDim(), h = dest.YDim();
  US16 *d = (US16 *) dest.PxlDest();
  const float *s, *s0 = ((const float *) pels) + w * (h - 1);
 
  // change top-down to bottom-up scan, 0.25mm resolution
  for (y = h; y > 0; y--, s0 -= w)
    for (x = w, s = s0; x > 0; x--, s++, d++)
      if (isnan(*s))
        *d = 0xFFFF;
      else
        *d = (US16)(4000.0 * (*s) + 0.5);
  return 1;
}


//= Expand original 100x100 image to 640x480 with bilinear interpolation.
// needs valid sx[], fx[], sy[], and fy[] arrays from tof_sampling()
// pels are scanned right-to-left, top-down from upper right corner
// needs a square of 4 valid pixels to make mixed pixel (shrinks mask)
// will not interpolate across big horizontal or vertical depth jumps

int jhcSwapBody::set_z16_tof (jhcImg& dest, const void *pels) const
{
  const unsigned short *s = (const unsigned short *) pels;
  unsigned short *d = (unsigned short *) dest.PxlDest();
  int dx, dy, i, hf, ref, vf, sw, se, nw, ne, bot, top, pel;
  int hjump = ROUND(4.0 * 101.6), vjump = ROUND(4.0 * 101.6);  // inches

  // rescan and uniformly stretch input image
  for (dy = 0; dy < 480; dy++)
  {
    // make sure source column is valid
    i = sx[dy];
    if ((i < 0) || (i >= 100))
    {
      memset(d, 0xFF, 1280);                                   // skip whole line
      d += 640;                                                
      continue;
    }
    hf = fx[dy];                                               // horizontal mixing

    // interpolate source pixels to get destination line
    // NW:NE:SW:SE square refers to pels array (rotated 90 degs wrt dest)
    for (dx = 0; dx < 640; dx++, d++)
    {
      // check that reference address is within source
      *d = 0xFFFF;                                             // default
      ref = sy[dx] + i;
      if ((ref < 0) || (ref >= 10000))
        continue;
      vf = fy[dx];                                             // vertical mixing

      // mix lower quad pixels (SW & SE) but only if valid 
      sw = s[ref];
      if (sw == 0xFFFF)                                        // quad partially invalid
        continue;
      bot = sw;                                                // left of edge 
      if (hf > 0)
      {      
        se = s[ref + 1];
        if (se == 0xFFFF)                                      // quad partially invalid
          continue;
        if (abs(se - sw) < vjump)
          bot = ((sw << 8) + hf * (se - sw)) >> 8;             // full mix 
        else if (hf >= 128)
          bot = se;                                            // right of edge
      }

      // check if vertical mix needed
      if (vf <= 0)                                             
      {
        *d = (US16) bot;
        continue;
      }

      // mix upper quad pixels (NW & NE) but only if valid 
      nw = s[ref + 100];
      if (nw == 0xFFFF)                                        // quad partially invalid
        continue;
      top = nw;                                                // left of edge 
      if (hf > 0)
      {      
        ne = s[ref + 101];
        if (ne == 0xFFFF)                                      // quad partially invalid
          continue;
        if (abs(ne - nw) < vjump)
          top = ((nw << 8) + hf * (ne - nw)) >> 8;             // full mix 
        else if (hf >= 128)
          top = ne;                                            // right of edge
      }
      
      // combine interpolated bottom with interpolated top
      pel = bot;                                               // lower half
      if (abs(top - bot) < hjump)
        pel = ((bot << 8) + vf * (top - bot)) >> 8;            // four way blend
      else if (vf >= 128)
        pel = top;                                             // upper half
      *d = (US16) pel;
    }
  } 
  return 1;
}


//= Ingest main camera buffer full of color pixels of some format.
// fmt: 0 = image not ready
//      1 = VGA bottom-up BGR (Kinect)
//      2 = VGA top down RGB (Astra)
// returns 0 if not accepted, 1 if okay (fmt reset to 0)

int jhcSwapBody::SetColor (const void *pels, int& fmt)
{
  jhcImg *dest = &img_c0;
  int ok = 0;

  // see if ready to be copied
  if ((fmt <= 0) || (pels == NULL))
    return 0;

  // choose between some common input formats
  if (fmt == 1)
    ok = set_bgr_bot(*dest, pels);     // Banzai
  else if (fmt == 2)
    ok = set_rgb_top(*dest, pels);     // Wansui
  else if (fmt == 3)
    ok = set_bgr_top(*dest, pels);     // Ganbei
 
  // check for success
  if (ok <= 0)
    return 0;
  fmt = 0;                             // clear ready flag
  return 1;
}


//= Ingest auxilliary camera buffer full of color pixels of some format.
// fmt: 0 = image not ready
//      1 = VGA bottom-up BGR (Kinect)
//      2 = VGA top down RGB (Astra)
// returns 0 if not accepted, 1 if okay (fmt reset to 0)

int jhcSwapBody::SetAuxCam (const void *pels, int& fmt)
{
  jhcImg *dest = &img_a0;
  int ok = 0;

  // see if ready to be copied
  if ((fmt <= 0) || (pels == NULL))
    return 0;

  // choose between some common input formats
  if (fmt == 1)
    ok = set_bgr_bot(*dest, pels);
  else if (fmt == 2)
    ok = set_rgb_top(*dest, pels); 
 
  // check for success
  if (ok <= 0)
    return 0;
  fmt = 0;                             // clear ready flag
  return 1;
}


//= Load color image with DIB format BGR bottom-up data.

int jhcSwapBody::set_bgr_bot (jhcImg& dest, const void *pels) const
{
  if ((dest.Fields() != 3) || (pels == NULL))
    return 0;
  memcpy(dest.PxlDest(), pels, dest.PxlSize());   // assumes not swizzled
  return 1;
}


//= Load color image from Orbbec Astra with RGB top-down data.

int jhcSwapBody::set_rgb_top (jhcImg& dest, const void *pels) const
{
  // sanity check
  if ((dest.Fields() != 3) || (pels == NULL))
    return 0;

  // full image
  int x, y, w = dest.XDim(), h = dest.YDim(), ln = dest.Line();
  const UC8 *s, *s0 = ((const UC8 *) pels) + ln * (h - 1);
  UC8 *d = dest.PxlDest();

  // change top-down to bottom-up scan
  for (y = h; y > 0; y--, s0 -= ln)
    for (x = w, s = s0; x > 0; x--, s += 3, d += 3)
    {
      // change RGB to BGR order
      d[0] = s[2];
      d[1] = s[1];
      d[2] = s[0];
    }
  return 1;
}


//= Load color image from OpenCV USB camera with BGR top-down data.

int jhcSwapBody::set_bgr_top (jhcImg& dest, const void *pels) const
{
  // sanity check
  if ((dest.Fields() != 3) || (pels == NULL))
    return 0;

  // full image
  int x, y, w = dest.XDim(), h = dest.YDim(), ln = dest.Line();
  const UC8 *s, *s0 = ((const UC8 *) pels) + ln * (h - 1);
  UC8 *d = dest.PxlDest();

  // change top-down to bottom-up scan
  for (y = h; y > 0; y--, s0 -= ln)
    for (x = w, s = s0; x > 0; x--, s += 3, d += 3)
    {
      d[0] = s[0];
      d[1] = s[1];
      d[2] = s[2];
    }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Load in fresh configuration data from all mechanical elements.
// ignores "voice" and "imgs" (included for jhcEliBody compatibility)

int jhcSwapBody::Update (int voice, int imgs)
{
  // new overall attitude
  Lock();
  pitch = pitch0;
  roll  = roll0;

  // swap in new component states
  neck0.Update();
  arm0.Update();
  lift0.Update();
  base0.Update();

  // change camera poses
  pos_r.Copy(pos_r0);
  dir_r.Copy(dir_r0);
  pos_c.Copy(pos_c0);
  dir_c.Copy(dir_c0);
  pos_a.Copy(pos_a0);
  dir_a.Copy(dir_a0);
 
  // accept newest input images (quick)
  img_r.CopyArr(img_r0);
  img_a.CopyArr(img_a0);
  raw.CopyArr(img_c0);       // for later dump
  seen = seen0;
  Unlock();
  return 1;
}


//= Harvest final component commands now that arbitration is done.
// ignores "lead" (included for jhcEliBody compatibility)

int jhcSwapBody::Issue (double lead)
{
  Lock();
  neck0.Issue();
  arm0.Issue();
  lift0.Issue();
  base0.Issue();
  Unlock();
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Exclusive Access                           //
///////////////////////////////////////////////////////////////////////////

//= Claim exclusive access to cached state in robot component classes.

void jhcSwapBody::Lock ()
{
  abstime_t one_sec;

  if (pthread_mutex_timedlock(&io_lock, abstime_wait(&one_sec, 1000)) != 0)
    jprintf(">>> Never got robot I/O mutex in jhcSwapBody::Lock\n");
}


//= Release exclusive access to cached state in robot component classes.

void jhcSwapBody::Unlock ()
{ 
  pthread_mutex_unlock(&io_lock);
}
