// jhcTofVSrc.cpp : interface to MaixSense A010 Time-of-Flight sensor
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Etaoin Systems
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

#include <stdio.h>

#include "Interface/jms_x.h"

#include "Video/jhcTofVSrc.h"


///////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                       //
///////////////////////////////////////////////////////////////////////////

// "tof" is a dummy MIME-dispatch extension which gets stripped off

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

JREG_CAM(jhcTofVSrc, "tof");

#endif


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcTofVSrc::~jhcTofVSrc ()
{
  run = 0;                                // stop receiver
  pthread_join(hoover, NULL);             
  s.TxArray((UC8 *)"AT+DISP=1\r", 10);    // stop transmitter
}


//= Constructor initializes certain values given source specification.
// spec typically name like "3.tof" where 3 is the serial port number

jhcTofVSrc::jhcTofVSrc (const char *spec)
{
  const char *tail;
  int port;

  // try opening specified port
  ok = -2;
  ParseName(spec);
  if ((tail = strpbrk(BaseName, "0123456789")) == NULL)
    return;
  if (sscanf_s(tail, "%d", &port) != 1)
    return;
  ok = -1;
  if (s.SetSource(port, 115200) <= 0)     // baud rate irrelevant
    return;
  ok = 1;

  // bind standard properties
  w = 100;
  h = 100;
  d = 1;
  flen = 70.7;                            // for 70 deg VFOV
  aspect = 0.8246;                        // for 60 deg HFOV
  freq = 16.0;                            // approx (unchangeable)

  // configure and start sensor (ignores FPS command)
  s.TxArray((UC8 *)"AT+UNIT=1\r", 10);    // 1mm depth step
  jms_sleep(50);                          // 50ms min between commands
  s.TxArray((UC8 *)"AT+DISP=3\r", 10);    // needs live display!

  // launch receiver thread
  raw = (UC8 *) &raw0;
  fresh = 0;
  run = 1;
  pthread_create(&hoover, NULL, absorb, (void *) this);
}


///////////////////////////////////////////////////////////////////////////
//                          Core functionality                           //
///////////////////////////////////////////////////////////////////////////

//= Read most recent new frame into supplied array (random access mode).
// src = 0 usually color but here is 8 bit grayscale depth
// if block > 0 then waits until next frame is ready before returning
// returns 1 if loaded, 0 if no new frame yet, negative if done or broken

int jhcTofVSrc::iGet (jhcImg& dest, int *advance, int src, int block)
{
  UC8 *pels = (UC8 *) &raw0;

  // check if source is operational and new frame is ready
  if (ok <= 0)  
    return -1;
  while (fresh <= 0)
  {
    if (block <= 0)
      return 0;
    jms_sleep(1);
  }

  // rearrange pixels for correct image orientation
  pthread_mutex_lock(data);
  if (raw == pels)
    pels = (UC8 *) &raw1;    // use finished buffer
  dest.LoadTopRt(pels + 16);
  fresh = 1;
  pthread_mutex_lock(data);
  return 1;
}


//= Background thread continually receives serial bytes into raw buffers.

pthread_ret jhcTofVSrc::absorb (void *tof)
{
  jhcTofVSrc *me = (jhcTofVSrc *) tof;

  while (me->run > 0) 
  {
    if (me->sync() <= 0)
      break;
    if (me->fill_buf() <= 0)
      break;
  }
  me->ok = 0;               // stream ended
  return NULL;                            
}


//= Look for beginning of image packet = start code + correct length.
// returns 1 when found, 0 if stream broken

int jhcTofVSrc::sync () 
{
  int rc;

  while (1)
  {      
    // start code = 0x00 0xFF
    if ((rc = s.Rcv()) < 0)
      return 0;
    if (rc != 0x00)
      continue;
    if ((rc = s.Rcv()) < 0)
      return 0;
    if (rc != 0xFF)
      continue;

    // packet length 10016 = 0x2720 (little-endian)
    if ((rc = s.Rcv()) < 0)
      return 0;
    if (rc != 0x20)
      continue;
    if ((rc = s.Rcv()) < 0)
      return 0;
    if (rc == 0x27)
      break;
  }
  return 1;
}


//= Fills the current raw buffer with received serial bytes.
// returns 1 when successful, 0 if stream broken

int jhcTofVSrc::fill_buf ()
{
  UC8 *pels = (UC8 *) &raw0;
  int rc, sz = 10018, n = 0;                      

  // get image pixel data
  while (n < sz)
  {
    if ((rc = s.RxArray(raw + n, sz - n)) < 0)
      return 0;
    n += rc;
  }

  // swap input buffers and mark receipt
  pthread_mutex_lock(data);
  raw = ((raw != pels) ? pels : (UC8 *) &raw1);
  fresh = 1;
  pthread_mutex_unlock(data);
  return 1;
}

