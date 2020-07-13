// jhcPTZ.cpp : Pan Tilt Zoom camera platform controller
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2002-2013 IBM Corporation
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

#include <math.h>

#include "Interface/jhcMessage.h"

#include "System/jhcPTZ.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor closes serial connection.

jhcPTZ::~jhcPTZ ()
{
  defp.Close();
}


//= Default constructor sets parameter defaults.

jhcPTZ::jhcPTZ ()
{
  Defs();
}


//= Specify which port to use at creation time.

jhcPTZ::jhcPTZ (int n)
{
  Defs();
  SetPort(n);
  Init();
}


//= Set up defaults for local variables.
// these can be changed to tune up system

void jhcPTZ::Defs ()
{
  // assume old PTZ heads are in use
  SetD30();

  // general configuration: camera number and home position
  addr = 1;
  p0 = 0.0;
  t0 = 0.0;
  z0 = view(FMIN);

  // for keeping track of exclusive mode (gives quicker response)
  mode  = 0;
  count = 0;
  phase = 0;
}


//= Set up for Sony EVI-D30 pan/tilt/zoom camera head.

void jhcPTZ::SetD30 ()
{
  // overall motion ranges and speeds
  PRNG = 100.0;   // +/- 100 degrees horizontal
  TRNG =  25.0;   // +/- 25 degrees vertical
  PVEL =  80.0;   // 80 degs/sec pan max
  TVEL =  50.0;   // 50 degs/sec tilt max
  PMAX = 0x370;   // max pan encoder value
  TMAX = 0x12C;   // max tilt encoder value
  PVMX =  0x18;   // max pan speed value
  TVMX =  0x14;   // max tilt speed value

  // zoom lens parameters
  FMIN =   5.4;   // f = 5.4mm wide 
  FMAX =  64.8;   // f = 64.8mm tele (physical)
  AMAX =  48.8;   // maximum field of view 
  AMIN =   4.3;   // minimum angle (physical)
  ZTIM =   1.0;   // time to do full zoom (?)
  ZMAX = 0x3FF;   // max zoom encoder value
  ZVMN =  0x02;   // min zoom speed value
  ZVMX =  0x07;   // max zoom speed value

  // imager size
  HSZ2 = FMIN * tan(0.5 * D2R * AMAX);
}


//= Set up for Sony EVI-D100 pan/tilt/zoom camera head.

void jhcPTZ::SetD100 ()
{
  // overall motion ranges and speeds
  PRNG = 100.0;   // +/- 100 degrees horizontal
  TRNG =  25.0;   // +/- 25 degrees vertical
  PVEL = 300.0;   // 300 degs/sec pan max
  TVEL = 125.0;   // 125 degs/sec tilt max
  PMAX = 0x370;   // max pan encoder value
  TMAX = 0x12C;   // max tilt encoder value
  PVMX =  0x18;   // max pan speed value
  TVMX =  0x14;   // max tilt speed value

  // zoom lens parameters
  FMIN =   3.1;   // f = 3.1mm wide 
  FMAX =  31.0;   // f = 31mm tele (physical)
  AMAX =  65.0;   // maximum field of view
  AMIN =   6.6;   // minimum angle (physical)
  ZTIM =   1.0;   // time to do full zoom (?)
  ZMAX = 0x3FF;   // max zoom encoder value
  ZVMN =  0x01;   // min zoom speed value
  ZVMX =  0x07;   // max zoom speed value

  // imager size
  HSZ2 = FMIN * tan(0.5 * D2R * AMAX);
}


//= Specify which port to use (exclusive control).

int jhcPTZ::SetPort (int n)
{
  if (defp.SetSource(n, 9600, 8, 1, 0) < 1)
    return 0;
  port = &defp;
  return 1;
}


//= Use a shared serial port created elsewhere.
// barfs if unworkable communications parameters
// watch out for dangling pointers if external object deleted

int jhcPTZ::BindPort (jhcSerial *ser)
{
  if (ser == NULL)
    return -1;
  if ((ser->Baud()     != 9600)  || (ser->DataBits() != 8) ||
      (ser->StopBits() != 1)     || (ser->Parity()   != 0))
    return 0;
  defp.Close();
  port = ser;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Simple Commands                             //
///////////////////////////////////////////////////////////////////////////

//= Call at start to normalize settings.

int jhcPTZ::Init ()
{
  int a = addr;

  // broadcast commands
  addr = 8;
  command(0x30, 0x01);              // address set
  command(0x01, 0x00, 0x01);        // IF_clear
  addr = a;

  // set full auto mode
  command(0x01, 0x04, 0x38, 0x02);  // auto focus
  command(0x01, 0x04, 0x35, 0x00);  // auto white
  command(0x01, 0x04, 0x39, 0x00);  // auto expose

  // make sure camera is not moving
  return Freeze();
}


//= Stop all current pans, tilts, and zooms.

int jhcPTZ::Freeze ()
{
  int ok = 1;

  if (AngVel(0.0, 0.0, 0.0) < 1)
    ok = 0;
  if (stop_cmd() < 1)
    ok = 0;
  return ok;
}


//= Stop any command on "sockets" 1 and 2.

int jhcPTZ::stop_cmd ()
{
  int ok = 1;

  send(0x21);
  if (await_msg(0x61) < 0)
    ok = 0;
  else if (packet_end() < 1)
    ok = 0;
  send(0x22);

  return ok;
}


//= Moves camera as rapidly as possible to stored configuration.
// default: pan and tilt half way through their ranges, zoom to widest

int jhcPTZ::Home ()
{
  return Goto(p0, t0, z0, 1);
}


//= Save the current position as the palce for "Home" to return to.

int jhcPTZ::Mark ()
{
  return Where(&p0, &t0, &z0);
}


//= Tell if camera is connected and responding.

int jhcPTZ::Status ()
{
  int b;

  send(0x09, 0x04, 0x00);         // power on query
  if (await_msg(0x50) < 0)
    return 0;
  if ((b = port->Rcv()) < 0)
    return 0;
  if (packet_end() < 1)
    return 0;
  if (b != 0x02)
    return 0;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Basic Motion Commands                         //
///////////////////////////////////////////////////////////////////////////

//= Set velocity of head essentially in degrees per second.
// specified as how much angular change to make over what period of time
// this format used to accomodate zoom which is non-linear in FOV
// if force is nonzero, some motion guaranteed but magnitudes may be off
// Note: motion continues even after specified time

int jhcPTZ::AngVel (double pdegs, double tdegs, double zdegs, 
                    double secs, int force)
{
  double fov, ang, f, tsc = 1.0, narr = view(FMAX), wide = view(FMIN);
  int p, t, z, ok = 1;

  // compute desired pan and tilt speeds
  p = (int)(PVMX * pdegs / (secs * PVEL) + 0.5);
  t = (int)(TVMX * tdegs / (secs * TVEL) + 0.5);

  // compute zoom speed, if beyond ranges then pro-rate time interval
//  if (Where(NULL, NULL, &fov) < 1)
//    ok = 0;
//  else 
fov = 15.0;
  {
    f = focal(fov);
    ang = fov + zdegs;
    if (ang > wide)
    {
      tsc = fabs(zdegs) / __max(1.0, wide - fov);
//      z = (int)(tsc * ZVMX * (FMIN - f) / (secs * FVEL) + 0.5);
      z = (int)(tsc * ZVMX * (FMIN - f) * ZTIM / secs + 0.5);
    }
    else if (ang < narr)
    {
      tsc = fabs(zdegs) / __max(1.0, fov - narr);
//      z = (int)(tsc * ZVMX * (FMAX - f) / (secs * FVEL) + 0.5);
      z = (int)(tsc * ZVMX * (FMAX - f) * ZTIM / secs  + 0.5);
    }
    else
//      z = (int)(ZVMX * (Focal(ang) - f) / (secs * FVEL) + 0.5);
      z = (int)(ZVMX * (focal(ang) - f) * ZTIM / secs + 0.5);
  }

  // make sure some motion occurs
  if (force > 0)
  {
    if ((p == 0) && (pdegs != 0.0))
      p = ((pdegs > 0.0) ? 1 : -1);
    if ((t == 0) && (tdegs != 0.0))
      t = ((tdegs > 0.0) ? 1 : -1);
    if ((z == 0) && (zdegs != 0.0))
      z = ((zdegs > 0.0) ? ZVMN : -ZVMN);
  } 

  // issue commands
  RawAimSpeed(p, t);
//  if (ok > 0)
//    RawZoomSpeed(z);
  return ok;
}


//= Set velocity of head in terms of the camera image.
// will slew by given fraction of original image size in specified time
// zoom handled similarly, image will be e.g. 1.5x bigger after 1 second
// if force is nonzero, some motion guaranteed but magnitudes may be off
// Note: motion continues even after specified time

int jhcPTZ::FracVel (double xfrac, double yfrac, double sc, 
                     double secs, int force)
{
  double fov;

//  if (Where(NULL, NULL, &fov) < 1)
//    return 0;
fov = 15.0;
  return AngVel(xfrac * fov, yfrac * fov, sc * fov, secs, force);
}


//= Reports PTZ location in terms of angles (in degs) relative to center.
// zoom reported as camera field of view (also in degs)

int jhcPTZ::Where (double *pdeg, double *tdeg, double *fov)
{
  double f;
  int p, t, z;

  // get current data
  if ((pdeg != NULL) || (tdeg != NULL))
    if (RawAimPos(&p, &t) < 1)
      return 0;
  if (fov != NULL)
    if (RawZoomPos(&z) < 1)
      return 0;
  
  // convert pan and tilt to fractional ranges then true angles
  if (pdeg != NULL)
  {
    *pdeg = p / (double) PMAX;
    *pdeg = __max(-1.0, __min(*pdeg, 1.0));
    *pdeg *= -PRNG;
  }
  if (tdeg != NULL)
  {
    *tdeg = t / (double) TMAX;
    *tdeg = __max(-1.0, __min(*tdeg, 1.0));
    *tdeg *= TRNG;
  }

  // compute viewing angle assuming zoom is linear in focal length
  if (fov != NULL)
  {
    f = z / (double) ZMAX;
    f = __max(0.0, __min(f, 1.0));
    f = f * (FMAX - FMIN) + FMIN;
    *fov = view(f);
  }
  return 1;
}


//= Return to given pan and tilt angles and specified field of view (in degs).
// motion is generally not coordinated -- may finish pan before tilt
// attempts to follow roughly linera view path is slew is non-zero
// Note: does not wait for completion

int jhcPTZ::Goto (double pdeg, double tdeg, double fov, int slew)
{
  double f, pt, tt;
  int p, t, z, pv = PVMX, tv = TVMX, ok = 1;

  // convert degrees to pan and tilt encoder counts
  f = -pdeg / PRNG;
  f = __max(-1.0, __min(f, 1.0));
  p = (int)(f * PMAX + 0.5);
  f = tdeg / TRNG;
  f = __max(-1.0, __min(f, 1.0));
  t = (int)(f * TMAX + 0.5);
  
  // convert zoom FOV to focal length then encoder count
  f = focal(fov);
  f = __max(FMIN, __min(f, FMAX));
  f = (f - FMIN) / (FMAX - FMIN);
  z = (int)(f * ZMAX + 0.5);

  // adjust pan and tilt speeds if slew requested
  if (slew > 0)
    if (Where(&pt, &tt, NULL) >= 1)
    {
      pt = fabs(pdeg - pt) / PVEL;
      tt = fabs(tdeg - tt) / TVEL;
      if (pt > tt)
        tv = (int)(tv * (tt / pt) + 0.5);
      else if (tt > pt)
        pv = (int)(pv * (pt / tt) + 0.5);
      tv = __max(1, tv);
      pv = __max(1, pv);
    }

  // issue commands
  RawAimSet(p, t, pv, tv);
  RawZoomSet(z);
  return ok;
}


//= Center the camera on some image-based point.
// motion is generally not coordinated -- may finish pan before tilt
// attempts to follow roughly linear view path is slew is non-zero
// Note: does not wait for completion

int jhcPTZ::Shift (double xfrac, double yfrac, double sc, int slew)
{
  double p, t, fov;

  if (Where(&p, &t, &fov) < 1)
    return 0;
  return Goto(p + xfrac * fov, t + yfrac * fov, sc * fov, slew); 
}


//= Wait for command done signal (e.g. Homing).
// with an argument of zero still checks once
// each loop takes rough the timeout interval of the comm port

int jhcPTZ::AwaitDone (double max_secs)
{
  int i, n = (int)(max_secs / port->wtime + 0.5) + 1;

  await_ack();
  for (i = n; i > 0; i--)
    if (await_msg(0x50, 0xFE) >= 0)
//      if (PacketEnd() >= 1)           // not needed since unique
        return 1;
  return 0;
}


//= Converts field of view to focal length.

double jhcPTZ::focal (double view)
{
  return(HSZ2 / tan(view * 0.5 * 3.141592653 / 180.0));
}


//= Converts focal length to field of view.

double jhcPTZ::view (double focal)
{
  return(atan(HSZ2 / focal) * 2.0 * 180.0 / 3.141592653); 
}


///////////////////////////////////////////////////////////////////////////
//                        Mode Switching Commands                        //
///////////////////////////////////////////////////////////////////////////

// motion constrained to be one of the following modes only:
//   0 = idle      - no pan, tilt, zoom, or ballistic commands
//   1 = aiming    - using non-zero pan and/or tilt speeds
//   2 = zooming   - using non-zero zoom speed
//   3 = ballistic - in the middle of some Goto command


//= Stop all currently running commands.
// issues the relevant stop multiple times (twice)

void jhcPTZ::Idle ()
{
  int repeat = 2;

  if (count < repeat)
  {
    if (mode == 1)
      RawAimSpeed(0, 0);
    else if (mode == 2)
      RawZoomSpeed(0);
    else if (mode == 3)
    {
      send(0x21);
      send(0x22);
    }
    phase = 0;
    count++;
  }
  else
    mode = 0;
}


//= Set camera motion.
// assumes either moving, zooming, or ballistic

void jhcPTZ::AimVel (int pan, int tilt, int wait)
{
  if (((mode != 0) && (mode != 1)) || ((pan == 0) && (tilt == 0)))
  {
    Idle();
    return;
  }

  // send speed commands and mark mode
  RawAimSpeed(pan, tilt);
  mode = 1;
  count = 0;
}


//= Set zoom motion.
// assumes either moving, zooming, or ballistic

void jhcPTZ::ZoomVel (int zoom, int wait)
{
  int zcnt;

  if (((mode != 0) && (mode != 2)) || (zoom == 0))
  {
    Idle();
    return;
  }
  
  // alternate issuing zoom and checking position
  if (phase++ == 0)
    RawZoomSpeed(zoom);    
  else
  {
    RawZoomPos(&zcnt);    // blocking (1/60 sec on D30)
    fov_est = FOV(zcnt);
    phase = 0;
  }
  mode = 2;
  count = 0;
}


//= Convert raw zoom encoder value to field of view (in degrees).
// approx. (+/- 10%) based on data for EVI-D100 focal lengths 

double jhcPTZ::FOV (int zpos)
{
  double pi = 3.141592653, hd2r = pi / 90.0, dr2d = 360.0 / pi;
  double fsc = tan(hd2r * AMAX), zsc = log(FMIN / FMAX) / ZMAX;

  return(dr2d * atan(fsc * exp(zsc * zpos)));
}


///////////////////////////////////////////////////////////////////////////
//                           Low-Level Commands                          //
///////////////////////////////////////////////////////////////////////////

//= Set zoom speed to some raw signed value.

void jhcPTZ::RawZoomSpeed (int zvel)
{
  int zc = 0x00;

  // limit speed to valid range and determine direction
  if (zvel >= ZVMN)
    zc = 0x20 + __min( zvel, ZVMX);
  else if (zvel <= -ZVMN)
    zc = 0x30 + __min(-zvel, ZVMX);

  // issue command but don't wait for completion
  send(0x01, 0x04, 0x07, zc);
}


//= Set zoom position to some unsigned raw value.
// does not wait for completion, check later with "AwaitDone"

void jhcPTZ::RawZoomSet (int zpos)
{
  UC8 a = 0x80 + (addr & 0x07);
  int z;

  // limit command to valid range
  z = __max(0, __min(zpos, ZMAX));

  // issue command but don't wait for ACK
  port->Flush(0);
  port->Xmit(a);
  port->Xmit(0x01);
  port->Xmit(0x04);
  port->Xmit(0x47);
  port->Xmit((z & 0xF000) >> 12);
  port->Xmit((z & 0x0F00) >> 8);
  port->Xmit((z & 0x00F0) >> 4);
  port->Xmit( z & 0x000F);
  port->Xmit(0xFF);
}


//= Returns current setting for zoom in raw encoder counts.
// blocks until request is transmitted and response received

int jhcPTZ::RawZoomPos (int *zcnt)
{
  int sh, b;

  // initialize value
  if (zcnt == NULL)
    return 0;
  *zcnt = 0;

  // issue command and wait for response 
  send(0x09, 0x04, 0x47);
  if (await_msg(0x50) < 0)
    return 0;

  // assemble bytes from response into unsigned zoom value
  for (sh = 12; sh >= 0; sh -= 4)
    if ((b = port->Rcv()) < 0)
      return 0;
    else
      *zcnt |= (b << sh);

  // strip remainder of packet
  return packet_end();
}


//= Send pan & tilt velocity command, uses raw signed values for speed.

void jhcPTZ::RawAimSpeed (int pvel, int tvel)
{
  UC8 a = 0x80 + (addr & 0x07);
  int p = 0x00, t = 0x00, pd = 0x03, td = 0x03;

  // set up command parameters
  if (pvel > 0)
  {
    pd = 0x01;
    p = __min(pvel, PVMX);
  }
  else if (pvel < 0)
  {
    pd = 0x02;
    p = __min(-pvel, PVMX);
  }
  if (tvel > 0)
  {
    td = 0x01;
    t = __min( tvel, TVMX);
  }
  else if (tvel < 0)
  {
    td = 0x02;
    t = __min(-tvel, TVMX);
  }

  // issue command but don't wait for ACK
  port->Flush(0);
  port->Xmit(a);
  port->Xmit(0x01);
  port->Xmit(0x06);
  port->Xmit(0x01);
  port->Xmit(p);
  port->Xmit(t);
  port->Xmit(pd);
  port->Xmit(td);
  port->Xmit(0xFF);
}


//= Send pan & tilt positioning command.
// positions are signed raw values, velocities are raw values
// does not wait for completion, check later with "AwaitDone"

void jhcPTZ::RawAimSet (int ppos, int tpos, int pvel, int tvel)
{
  UC8 a = 0x80 + (addr & 0x07);
  int p = 0, t = 0, pv, tv;

  // limit values to valid ranges
  if (ppos > 0)
    p = __min(ppos, PMAX);
  else if (ppos < 0)
  {
    p = __min(-ppos, PMAX);
    p = (~p + 1) & 0xFFFF;
  }
  if (tpos > 0)
    t = __min(tpos, TMAX);
  else if (tpos < 0)
  {
    t = __min(-tpos, TMAX);
    t = (~t + 1) & 0xFFFF;
  }
  pv = __max(0, __min(pvel, PVMX));
  tv = __max(0, __min(tvel, TVMX));

  // issue command but don't wait for ACK
  port->Flush(0);
  port->Xmit(a);
  port->Xmit(0x01);
  port->Xmit(0x06);
  port->Xmit(0x02);
  port->Xmit(pv);  
  port->Xmit(tv);
  port->Xmit((p & 0xF000) >> 12);
  port->Xmit((p & 0x0F00) >> 8);
  port->Xmit((p & 0x00F0) >> 4);
  port->Xmit( p & 0x000F);
  port->Xmit((t & 0xF000) >> 12);
  port->Xmit((t & 0x0F00) >> 8);
  port->Xmit((t & 0x00F0) >> 4);
  port->Xmit( t & 0x000F);
  port->Xmit(0xFF);
}


//= Returns pan and tilt in signed raw encoder counts.
// blocks until request is transmitted and response received

int jhcPTZ::RawAimPos (int *pcnt, int *tcnt)
{
  int sh, b;

  // initialize values
  if ((pcnt == NULL) || (tcnt == NULL))
    return 0;
  *pcnt = 0;
  *tcnt = 0;

  // issue command and wait for response 
  send(0x09, 0x06, 0x12);
  if (await_msg(0x50) < 0)
    return 0;

  // assemble bytes from response into signed pan value
  for (sh = 12; sh >= 0; sh -= 4)
    if ((b = port->Rcv()) < 0)
      return 0;
    else
      *pcnt |= (b << sh);
  if ((*pcnt & 0x8000) != 0)
    *pcnt = -(~(*pcnt - 1) & 0xFFFF);

  // assemble bytes from response into signed tilt value
  for (sh = 12; sh >= 0; sh -= 4)
    if ((b = port->Rcv()) < 0)
      return 0;
    else
      *tcnt |= (b << sh);
  if ((*tcnt & 0x8000) != 0)
    *tcnt = -(~(*tcnt - 1) & 0xFFFF);

  // strip remainder of packet
  return packet_end();
}


///////////////////////////////////////////////////////////////////////////
//                           Packet Transmission                         //
///////////////////////////////////////////////////////////////////////////

//= Send command and waits for ACK to be returned (does not retry if fail).

void jhcPTZ::command (int c0, int c1, int c2, int c3)
{
  send(c0, c1, c2, c3);
  ack_pend = 1;          // next command should check for ACK
}


//= Prefixes address (8x), bytes of cmd until -1, then adds terminator (FF).

void jhcPTZ::send (int c0, int c1, int c2, int c3)
{
  UC8 a = 0x80 + (addr & 0x07);
  int cmd[4];
  int i;

  // fill command vector (could extend to more bytes)
  cmd[0]  = c0;
  cmd[1]  = c1;
  cmd[2]  = c2;
  cmd[3]  = c3;

  // get rid of any built up ACKs, etc.
  // then transmit command: 8x c0 c1 c2 c3 FF
  port->Flush(0);
  port->Xmit(a);
  for (i = 0; i < 4; i++)
    if (cmd[i] < 0)
      break;
    else
      port->Xmit((UC8)(cmd[i]));
  port->Xmit(0xFF);
}
 

//= Wait for the acknowledge signal (most commands).
// only really waits if ack_pend or force is non-zero

int jhcPTZ::await_ack (int force)
{
  if ((ack_pend <= 0) && (force <= 0))
    return 1;
  if (await_msg(0x40, 0xFE) < 0)
    return 0;
//  if (packet_end() < 1)             // not needed since unique
//    return 0;
  ack_pend = 0;
  return 1;
}


//= Look for response from proper camera of given class.
// a tag value of -1 takes any kind (kind is returned)

int jhcPTZ::await_msg (int tag, int mask)
{
  UC8 a = 0x80 + ((addr & 0x07) << 4);
  int b;

  while (1)
  {
    if ((b = port->Rcv()) < 0)   
      return -1;
    if (b == a)                     // camera match
    {
      if ((b = port->Rcv()) < 0)   
        return -1;
      if ((tag < 0) || ((b & mask) == tag))  // tag match
        break;
    }
//    if (packet_end() < 1)            // get next packet
//      return -1;
  }
  return b;
}


//= Advance until packet end marker (FF) is read.

int jhcPTZ::packet_end ()
{
  int b;

  while (1)
  {
    if ((b = port->Rcv()) < 0)   
      return 0;
    if (b == 0xFF)
      break;
  }
  return 1;
}

