// jhcMbmVSink.cpp : saves images as a simple concatenated bitmap video file
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2009 IBM Corporation
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

#include "Video/jhcVidReg.h"

#include "jhcMbmVSink.h"


///////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                       //
///////////////////////////////////////////////////////////////////////////

JREG_VSINK(jhcMbmVSink, "mbm");


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcMbmVSink::jhcMbmVSink (const char *fname)
{
  // no file yet opened and unknown buffer size
  out = NULL;
  bsize = 0;

  // default characteristics
  if (fname != NULL)
    SetSink(fname);
  SetSize(320, 240, 3);
  SetSpeed(30.0);
}


//= Default destructor does necessary cleanup.

jhcMbmVSink::~jhcMbmVSink ()
{
  iClose();
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

// Go back and insert correct number of frames (little-endian).

void jhcMbmVSink::iClose ()
{
  if (out == NULL)
    return;
  fseek(out, 12, SEEK_SET);
  fputc( frames        & 0xFF, out);
  fputc((frames >>  8) & 0xFF, out);
  fputc((frames >> 16) & 0xFF, out);
  fputc((frames >> 24) & 0xFF, out);
  fclose(out);
}               


//= Create file for specified image size and framerate.

int jhcMbmVSink::iOpen ()
{
  if ((out = fopen(FileName, "wb")) == NULL)
    return 0;
  frames = 0;
  bsize = h * ((w * d + 3) & 0xFFFC);
  if (write_hdr(out) <= 0)
    return 0;
  return 1;
}


//= Create standard Motion BitMap header at front of file.
// format:
//   MBM   = type marker (ASCII)
//   3     = three bytes per pixel (ASCII)
//   640   = width (unsigned short)
//   480   = height (unsigned short)
//   30000 = frames per second (x1000 = unsigned long)
//   nnnn  = total count of frames in file (unsigned long)
//   <D1>  = first frame data (e.g. 640 * 480 * 3 = 921600 bytes)
//   <D2>  = second frame data (same fixed size)
//   ...   = rest of frames
// all lines are padded to multiples of 4 bytes (e.g. 750 * 3 -> 2252)

int jhcMbmVSink::write_hdr (FILE *dest)
{
  UL32 fk = (UL32)(1000.0 * freq + 0.5);

  // write 3 character marker plus field count in ascii
  fputc('M', dest);
  fputc('B', dest);
  fputc('M', dest);
  fputc('0' + d, dest);

  // write 16 bit width and height (little-endian)
  fputc(w & 0xFF, dest);
  fputc(w >> 8,   dest);
  fputc(h & 0xFF, dest);
  fputc(h >> 8,   dest);

  // save frame rate and zero frames so far
  fputc( fk        & 0xFF, dest);
  fputc((fk >>  8) & 0xFF, dest);
  fputc((fk >> 16) & 0xFF, dest);
  fputc((fk >> 24) & 0xFF, dest);
  fputc(0, dest);
  fputc(0, dest);
  fputc(0, dest);
  fputc(0, dest);

  //check if still okay
  if (ferror(dest) != 0)
    return 0;
  return 1;
}


//= Record next image into file. 

int jhcMbmVSink::iPut (jhcImg& src)
{
  if (fwrite(src.PxlSrc(), 1, bsize, out) != (size_t) bsize)
    return 0;
  frames++;
  return 1;
}       


