// jhcVideoSink.cpp : common functions for video stream writing base class
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2004-2012 IBM Corporation
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

#include "Video/jhcVideoSink.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor sets standard start up values.

jhcVideoSink::jhcVideoSink ()
{
  ok = -1;
  w = 0;
  h = 0;
  d = 0;
  freq = 30.0;
  bound = 0;
  nextframe = 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Configuration                            //
///////////////////////////////////////////////////////////////////////////

//= Tell stream what size of images to store.
// this must be done before the first write to the stream
// stream must be closed for this to take effect

int jhcVideoSink::SetSize (int x, int y, int f)
{
  if (bound == 1)
    return 0;

  if ((x <= 0) || (y <= 0) || ((f != 1) && (f != 3)))
    return 0;  
  w = x;
  h = y;
  d = f;
  return 1;
}


//= Base size on a sample image.
// stream must be closed for this to take effect

int jhcVideoSink::SetSize (const jhcImg &src)
{
  return SetSize(src.XDim(), src.YDim(), src.Fields());
}


//= Record playback rate for video.
// this must be done before the first write to the stream

int jhcVideoSink::SetSpeed (double fps)
{
  if (bound == 1)
    return 0;
  if (fps <= 0.0)
    return -1;
  freq = fps;
  return 1;
}


//= Set output name for file.
// stream must be closed for this to take effect

int jhcVideoSink::SetSink (const char *fname)
{
  if (bound == 1) 
    return 0;
  ParseName(fname);
  return 1;
}


//= Sets standard fields but does not try to open video file yet.
// stream must be closed for this to take effect

int jhcVideoSink::SetSpecs (jhcImg& ref, const char *fname, double fps)
{
  if (bound == 1)
    return 0;
  SetSize(ref);
  SetSink(fname);
  SetSpeed(fps);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                            Basic Operations                           //
///////////////////////////////////////////////////////////////////////////

//= Close out file being written.
// does basic house-keeping then calls specialized iClose function
// always returns -1

int jhcVideoSink::Close ()
{
  iClose();
  bound = 0;
  ok = -1;
  return -1;
}


//= Create file for writing frames into.
// does basic house-keeping then calls specialized iOpen function
// returns: 1 = ready, 0 = was good but then failed, -1 = not opened 

int jhcVideoSink::Open ()
{
  FILE *out;

  // check if already opened cleanly
  if (bound == 1)
    return ok;
  ok = -1;
  
  // make sure frame size and rate specifications make sense
  if ((w <= 0) || (h <= 0) || ((d != 1) && (d != 3)) || (freq <= 0.0))
    return ok;

  // clear selected file if it already exists
  if (fopen_s(&out, FileName, "w") != 0)
    return ok;
  fclose(out);

  // call specialized opening function
  if (iOpen() <= 0)
    return Close();
  bound = 1;
  ok = 1;
  nextframe = 0;
  return 1;
}


//= Combines file specification and stream initialization for output.

int jhcVideoSink::Open (const char *fname)
{
  if (SetSink(fname) <= 0)
    return 0;
  return Open();
}


//= Save a new image in uncompressed format at end of file.
// will set size of output stream to match image if not set yet
// does basic house-keeping then calls specialized iPut function
// returns: 1 = successful, 0 = was good but just failed, -1 = bad stream

int jhcVideoSink::Put (const jhcImg& src)
{
  if (bound == 0)
  {
    SetSize(src);
    Open();
  }
  if (ok < 0)
    return -1;
  if ((src.XDim() != w) || (src.YDim() != h) || (src.Fields() != d))
    return 0;
  if (iPut(src) > 0)         // call specialized function
    return 1;
  ok = 0;                    // mark occasional failure on stream
  return 0;
}  


//= Attempts to write pixel buffer to currently open file.
// buffer size must already be specified via SetSize
// does not alter buffer contents

int jhcVideoSink::Put (UC8 *pixels)
{
  jhcImg fake;

  fake.Wrap(pixels, w, h, d);
  return Put(fake);
}




