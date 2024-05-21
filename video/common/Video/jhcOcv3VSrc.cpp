// jhcOcv3VSrc.cpp : reads videos using OpenCV infrastructure
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

// Note: ALWAYS needs vid_ocv3.dll, opencv_world345.dll, and opencv_ffmpeg345_64.dll !!!

#include <windows.h>                         // needed for mutex
#include <process.h>                         // for thread
#include <string.h>

#include "Interface/jms_x.h"
#include "Interface/jprintf.h"         
#include "Video/vid_ocv3.h"

#include "Video/jhcOcv3VSrc.h"


//////////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                          //
//////////////////////////////////////////////////////////////////////////////

// "ocv3" is a dummy MIME-dispatch extension which gets stripped off

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

JREG_VURL(jhcOcv3VSrc, "ocv3");

#endif


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcOcv3VSrc::~jhcOcv3VSrc ()
{
  HANDLE mutex = (HANDLE) lock, ready = (HANDLE) done;

  // stop capturing and deallocate thread control items
  Prefetch(0);
  CloseHandle(mutex);
  CloseHandle(ready);

  // deallocate OpenCV items
  vid_ocv3_close();
}


//= Default constructor initializes certain values.

jhcOcv3VSrc::jhcOcv3VSrc (const char *name, int index)
{
  int i;

  // save details of source
  strcpy_s(kind, "jhcOcv3VSrc");
  ParseName(name);
  ok = 0;

  // make up thread control items
  lock = (void *) CreateMutex(NULL, false, NULL);
  done = (void *) CreateEvent(NULL, TRUE, FALSE, NULL);
  bg = NULL;
  run = 0;

  // try opening source
  if (vid_ocv3_open(Trimmed()) <= 0)
    return;
  ok = 1;

  // read size of images and get framerate
  w = vid_ocv3_w();
  h = vid_ocv3_h();
  d = vid_ocv3_nf();
  freq = vid_ocv3_fps();

  // configure buffers for correct size
  for (i = 0; i < bsz; i++)
    buf[i].SetSize(w, h, d);

  // set maximum staleness for newly captured frames
  lag = ROUND(2.0 * 1000.0 / freq);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Start background thread reading images from stream.

void jhcOcv3VSrc::Prefetch (int doit)
{
  HANDLE backg = (HANDLE) bg, ready = (HANDLE) done;
  int i;

  if ((doit > 0) && (run <= 0))            // START
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
  else if ((doit <= 0) && (run > 0))       // STOP
  {
    // signal background thread to end then wait for exit
    run = 0;
    WaitForSingleObject(backg, 1000);
    CloseHandle(backg);
    bg = NULL;
  }
}


//= Read next frame from already open stream.
// can call multiple times with block = 0 until frame is acquired

int jhcOcv3VSrc::iGet (jhcImg &dest, int *advance, int src, int block)
{
  HANDLE mutex = (HANDLE) lock, ready = (HANDLE) done;
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
    if (block <= 0)
    {
      // possibly just give up if frame not ready yet
      if (WaitForSingleObject(ready, 0) != WAIT_OBJECT_0)
        return 0;
    }
    else if (WaitForSingleObject(ready, 1000) != WAIT_OBJECT_0)
    {
      ok = 0;                                                      // force to reopen
      jprintf(">>> No new frame ready in jhcOcv3VSrc::iGet !\n");
      return -1;
    }
    if (WaitForSingleObject(mutex, 1000) != WAIT_OBJECT_0)
    {
      jprintf(">>> Failed to get buffer lock in jhcOcv3VSrc::iGet !\n");
      return -1;
    }

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
      ans = 1;
    }
    
    // minimize buffer lock time so no UDP packets dropped
    ReleaseMutex(mutex);
  }

  // image copy de facto guarded by long ring buffer
  if (ans > 0)
    dest.CopyArr(buf[j]);
  return ans;
}


//= Continuously grab frames into buffers (run as a separate thread).

int jhcOcv3VSrc::grab_loop ()
{
  HANDLE mutex = (HANDLE) lock, ready = (HANDLE) done;

  while (run > 0)
  {
    // get a frame into current array slot
    if (vid_ocv3_get(buf[fill].PxlDest()) <= 0)
      return 0;

    // lock buffers then save time and advance pointer
    if (WaitForSingleObject(mutex, 1000) != WAIT_OBJECT_0)
    {
      jprintf(">>> Failed to get buffer lock in jhcOcv3VSrc::grab_loop !\n");
      return -1;
    }
    tdec[fill] = jms_now();
    fill = (fill + 1) % bsz;

    // signal new frame is available and unlock buffers
    SetEvent(ready);
    ReleaseMutex(mutex); 
  }
  return 1;   
}

