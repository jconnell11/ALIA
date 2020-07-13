// jhcMbmVSrc.cpp : handles concatenated bitmap images as one long video
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2009-2014 IBM Corporation
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

#include <string.h>
#include <io.h>

#include "jhcMbmVSrc.h"


//////////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

JREG_VSRC(jhcMbmVSrc, "mbm");

#endif  // JHC_GVID


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcMbmVSrc::jhcMbmVSrc (const char *name, int index)
{
  // save details of source
  strcpy(kind, "jhcMbmVSrc");
  ParseName(name);

  // try opening file and finding image size
  if ((in = fopen(FileName, "rb")) == NULL)
    return;
  if (read_hdr(in) <= 0)
    return;
  ok = 1;
}


//= Default destructor does necessary cleanup.

jhcMbmVSrc::~jhcMbmVSrc ()
{
  if (in != NULL)
    fclose(in);
}


//= Pull out video details from front of file (assumes rewound).
// leaves file pointer at start of first frame's data
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
// returns 0 if bad markings or unreasonable sizes

int jhcMbmVSrc::read_hdr (FILE *src)
{
  char ch;
  UL32 fk;

  // read first three letters (ascii)
  if (getc(in) != 'M')
    return 0;
  if (getc(in) != 'B')
    return 0;
  if (getc(in) != 'M')
    return 0;

  // get number of bytes per pixel (ascii)
  if ((ch = fgetc(src)) == EOF)
    return 0;
  d = (int)(ch - '0');
  if ((d < 1) || (d > 8))
    return 0;

  // get image dimensions (assumes little-endian)
  w =   getc(in);
  w |= (getc(in) << 8);
  h =   getc(in);
  h |= (getc(in) << 8);
  if ((w < 1) || (w > 20000) || (h < 1) || (h > 15000))
    return 0;

  // get framerate (x1000 little-endian)
  fk =   getc(in);
  fk |= (getc(in) << 8);
  fk |= (getc(in) << 16);
  fk |= (getc(in) << 24);
  freq = 0.001 * fk;
  if ((freq < 0.0) || (freq > 1000.0))
    return 0;

  // determine total number of frames (little-endian)
  nframes =   getc(in);
  nframes |= (getc(in) << 8);
  nframes |= (getc(in) << 16);
  nframes |= (getc(in) << 24);

  // success
  aspect = 1.0;
  bsize = h * ((w * d + 3) & 0xFFFC);
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Set up to read some particular frame next (skips header).

int jhcMbmVSrc::iSeek (int number)
{
  if (_lseeki64(_fileno(in), ((number - 1) * (__int64) bsize) + 16, SEEK_SET) < 0)
    return 0;
  fflush(in);
  return 1;
}


//= Read next frame from long file.

int jhcMbmVSrc::iGet (jhcImg &dest, int *advance, int src)
{
  if (fread(dest.PxlDest(), 1, bsize, in) != (size_t) bsize)
    return 0;
  return 1; 
}



