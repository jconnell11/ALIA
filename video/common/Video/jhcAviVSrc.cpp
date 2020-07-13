// jhcAviVSrc.cpp : wrappers around Microsoft AVI video format
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2019 IBM Corporation
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

// Build.Settings.Link.General must have library vfw32.lib
#pragma comment(lib, "vfw32.lib")


#include "Interface/jhcMessage.h"
#include "Interface/jhcString.h"

#include "Video/jhcAviVSrc.h"


//////////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

JREG_VSRC(jhcAviVSrc, "avi");

#endif


//////////////////////////////////////////////////////////////////////////////
//                     Basic creation and deletion                          //
//////////////////////////////////////////////////////////////////////////////

//= Destructor closes any open file.

jhcAviVSrc::~jhcAviVSrc ()
{
  CloseAvi(1);
}


//= Create stream from file name.
// index request ignored since offsets can be computed algorithmically
// Note: MPEG-4 VKI is a problem for Windows 7.

jhcAviVSrc::jhcAviVSrc (const char *filename, int index) 
{
  InitAvi(1);
  SetSource(filename);
}


//= Clears any structure built.

int jhcAviVSrc::CloseAvi (int last)
{
  if (pgf != NULL)
    AVIStreamGetFrameClose(pgf);
  if (pavi != NULL)
    AVIStreamRelease(pavi);
  if (pfile != NULL)
    AVIFileRelease(pfile);
  InitAvi(0);
  if (ok > 0)
    ok = 0;
  if (last != 0)
    AVIFileExit();
  return ok;
}


//= Sets up standard default values.

void jhcAviVSrc::InitAvi (int first)
{
  if (first != 0)
  {
    strcpy_s(kind, "jhcAviVSrc");
    AVIFileInit();
  }
  pfile = NULL;
  pavi = NULL;
  pgf = NULL;
  pack = 0;
}


/////////////////////////////////////////////////////////////////////////////
//                           Basic functions                               //
/////////////////////////////////////////////////////////////////////////////

//= Makes up and initiatilizes an AVI file reader.

int jhcAviVSrc::SetSource (const char *filename) 
{
  jhcString fn(filename);
  LPBITMAPINFOHEADER hdr;
  AVISTREAMINFO str;
  char stype[5];

  // try to open file
  strcpy_s(kind, "jhcAviVSrc");
  ParseName(filename);
  strcpy_s(Flavor, "avi");
  ok = 0;
  if (AVIFileOpen(&pfile, fn.Txt(), OF_READ, NULL) != 0)
    return CloseAvi(0);

  // initialize all needed handlers
  ok = -4;
  if (AVIFileGetStream(pfile, &pavi, streamtypeVIDEO, 0) != 0)
    return CloseAvi(0);
  ok = -3;
  if ((pgf = AVIStreamGetFrameOpen(pavi, NULL)) == NULL)
  {
    AVIStreamInfo(pavi, &str, sizeof(AVISTREAMINFO));
    stype[0] = (char)( str.fccHandler & 0x000000FF);        
    stype[1] = (char)((str.fccHandler & 0x0000FF00) >> 8);        
    stype[2] = (char)((str.fccHandler & 0x00FF0000) >> 16);        
    stype[3] = (char)((str.fccHandler & 0xFF000000) >> 24);        
    stype[4] = '\0';
    if (noisy > 0)
      Complain("Can't decode <%s> AVI format!", stype);
    return CloseAvi(0);
  }
  ok = -2;
  if ((hdr = (LPBITMAPINFOHEADER) AVIStreamGetFrame(pgf, 0)) == NULL)
    return CloseAvi(0);

  // check and record color properties of video, assume standard RGB 
  if (hdr->biCompression != BI_RGB)
  {
    if (noisy > 0)
      Complain("Can't handle color compressed AVI!");
    return -1;
  }
  d = 3;
  pack = 0;
  if (hdr->biBitCount == 8)        // monochrome
    d = 1;
  else if (hdr->biBitCount == 16)  // 555 video color
    pack = 1;
  else if (hdr->biBitCount == 32)  // 0:B:G:R video (e.g. DV)
    pack = 2;
  else if (hdr->biBitCount != 24)  
  {
    if (noisy > 0)
      Complain("Can't handle AVI with %d bits per pixel!", hdr->biBitCount);
    return -1;
  }

  // record size and sampling frequency
  ok = 1;
  w = hdr->biWidth;
  h = hdr->biHeight;
  aspect = 1.0;
  nframes = AVIStreamLength(pavi);
//  freq = (double)(AVIStreamTimeToSample(pavi, 1000));  // used to add 1
  AVIStreamInfo(pavi, &str, sizeof(AVISTREAMINFO));
  freq = str.dwRate / (double) str.dwScale;
  return ok;
}


//= Read current frame as RGB into supplied array (random access mode).
// assumes array is big enough, writes BGR order, rows padded to next long word
// ignores "block" flag since not a live source
// returns number of frames advanced

int jhcAviVSrc::iGet (jhcImg& dest, int *advance, int src, int block)
{
  LPBITMAPINFOHEADER hdr;
  long off;
  US16 *s2;
  US16 v;
  const UC8 *s;
  UC8 *d = dest.PxlDest();
  int x, y, ssk2, dsk = dest.Skip();

  // get raw AVI bitmap data, hopefully already in correct format
  hdr = (LPBITMAPINFOHEADER) AVIStreamGetFrame(pgf, nextread - 1);
  off = sizeof(BITMAPINFOHEADER) + hdr->biClrUsed * sizeof(RGBQUAD);
  s = (UC8 *) hdr + off;
  if (pack == 0)
    return dest.CopyArr(s);

  // translate 16 bit 555 data to RGB format (assumes little-endian)
  if (pack == 1)
  {
    ssk2 = w % 2;
    s2 = (US16 *) s;
    for (y = h; y > 0; y--)
    {
      for (x = w; x > 0; x--)
      {
        v = *s2++;
        d[0] = (UC8)((v << 3) & 0xF8);   // blue
        d[1] = (UC8)((v >> 2) & 0xF8);   // green 
        d[2] = (UC8)((v >> 7) & 0xF8);   // red
        d += 3;
      }
      d  += dsk;
      s2 += ssk2;
    }   
    return 1;
  }

  // translate 32 bit 0:B:G:R data to RGB format (assumes little-endian)
  if (pack == 2)
  {
    for (y = h; y > 0; y--)
    {
      for (x = w; x > 0; x--)
      {
        d[0] = s[0];    // blue
        d[1] = s[1];    // green
        d[2] = s[2];    // red
        d += 3;
        s += 4;         // skip top byte
      }
      d += dsk;         // source is always integral number of 4 bytes
    } 
    return 1;
  }

  return 0;
}






