// jhcLiveVSrc.h : specialization of jhcVideoSrc to framegrabbers
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2016 IBM Corporation
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

#ifndef _JHCLIVEVSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCLIVEVSRC_
/* CPPDOC_END_EXCLUDE */

// Build.Settings.Link.General must have library vfw32.lib and winmm.lib
#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "winmm.lib")


// order of includs is important

#include "jhcGlobal.h"
#include <windows.h>
#include <VfW.h>

#include "Video/jhcVideoSrc.h"


//= Specialization of jhcVideoSrc to framegrabbers.
// Binds and fetches from framegrabber with VFW driver as if it were a file.
// Seek is pretty much meaningless for this type, and Rate is just a limit,
// Get returns next complete video frame (so Step really has no effect).
// Can run in fast streaming mode (prefetch = 1) or one-shot grabbing.

class jhcLiveVSrc : public jhcVideoSrc
{
// internal member variables
private:
  HWND capWin;
  HANDLE capDone;
  HIC codec;
  BITMAPINFOHEADER nat, exp;
  LPBITMAPINFOHEADER native, expand;
  jhcImg *img;
  UC8 *rawa, *rawb, *big, *grab;
  long locka, lockb, locki, ready;
  UL32 rawsize, bigsize;
  DWORD tgrab, tgrab0;
  int b, mskip, cskip, streaming;
  UC8 avg5[94];

// basic video source operations and interactive user configuration
public:
  ~jhcLiveVSrc ();
  jhcLiveVSrc ();
  jhcLiveVSrc (const char *name);

  int SetSource (const char *spec);
  void Close ();
  void SelectDriver ();
  int ListDrivers (char dest[10][80]);
  void SelectParams (int format =0);

  void SetSize (int xmax, int ymax, int bw =0);
  void SetStep (int offset, int key =0);
  void Prefetch (int doit =1);
  int TimeStamp () {return tgrab;};  /** Return time of last frame grab. */

// creation and configuration utilities
private:
  void ReleaseCodec ();
  void Disconnect ();
  void Init ();
  void StreamConfig ();
  void BindDriver ();

  int RecordFormat (int bw);
  void ReadFormat (LPBITMAPINFOHEADER actual);
  void CopyFormat (LPBITMAPINFOHEADER dest, LPBITMAPINFOHEADER src);
  void RecordSizes (LPBITMAPINFOHEADER hdr, int bw =0);
  void ResizeBuffers ();
  int FindCodec ();

  int ScanFormats (LPBITMAPINFOHEADER target, int xmax, int ymax, int bw);
  int ScanSizes (LPBITMAPINFOHEADER target, int xmax, int ymax);
  int SetDims (LPBITMAPINFOHEADER hdr, int x, int y);
  int TestFormat (LPBITMAPINFOHEADER target);
  int WriteFormat (LPBITMAPINFOHEADER actual);

// when framegrabber has a new image one of these two function is called
// however, they are common to all instances of jhcLiveVSrc created
private:
  static LRESULT CALLBACK NewFrame (HWND cwin, LPVIDEOHDR vhd);
  static LRESULT CALLBACK FrameReady (HWND cwin, LPVIDEOHDR vhd);

  void SaveFrameData (UC8 *src);
  UC8 *PickBuffer (UC8 *last);
  void UnlockBuffer (UC8 *last);
  void CopyAll (UC8 *dest, UC8 *src);
  void CopyGreen (UC8 *dest, UC8 *src);
  void CopyGreen32 (UC8 *dest, UC8 *src);

// required basic frame retrieval function and support
private:  
  int iGet (jhcImg& dest, int *advance, int get);
  int GrabGet (jhcImg& dest, int *advance);
  int StreamGet (jhcImg& dest, int *advance);
  void Percolate ();
  void ExtractPixels (jhcImg& dest, UC8 *src);
  
  int C555toRGB (jhcImg& dest, UC8 *src);
  int C555toRGB_4 (jhcImg& dest, UC8 *src);
  int C555toMono (jhcImg& dest, UC8 *src);
  int C32toRGB (jhcImg& dest, UC8 *src);

};


/////////////////////////////////////////////////////////////////////////////

// part of mechanism for automatically associating class with file extensions

extern int jvreg_jhcLiveVSrc;


#endif
