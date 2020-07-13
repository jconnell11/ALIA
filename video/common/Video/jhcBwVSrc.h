// jhcBwVSrc.h : reader for Broadware web video streams
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2006-2014 IBM Corporation
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

#ifndef _JHCBWVSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCBWVSRC_
/* CPPDOC_END_EXCLUDE */

// Build.Settings.Link.General must have library strmiids.lib
#pragma comment (lib,"strmiids.lib")


// Note: to compile this you need to download the DirectX SDK from Microsoft
//   known to work with the December 2002 version (233,197,112 bytes)

#include "jhcGlobal.h"

#include "stdafx.h"        // needed for CString
#include <dshow.h>

#if (_MSC_VER >= 1400)
  #include "qedit_vs8.h"  // altered version in common
#else
  #include <qedit.h>
#endif

#include "Video/jhcVideoSrc.h"


//= Uses Microsoft DirectShow to read Broadware web video streams.
// modified from jhcAsxVSrc since proper MJPEG codec returns 32 bit images

class jhcBwVSrc : public jhcVideoSrc, public ISampleGrabberCB
{
private:	
  HANDLE grabbed;
  double f_time;    /** How long each frame takes (in secs).            */
  double p_time;    /** The play position (in secs) for the last frame. */
  double s_time;    /** The play position (in secs) for current frame.  */
  double snap;      /** Target time (in secs) to capture next frame.    */
  int run;          /** Whether the graph is running (-1 = prefetch).   */

  ICaptureGraphBuilder2 *builder;
	IGraphBuilder *manager;
	IBaseFilter *source;
  IBaseFilter *codec;
	IBaseFilter *sample;
	IBaseFilter *nop;
	IMediaControl *control;
	ISampleGrabber *extract;
  DWORD reg;
  long ref;

// main user calls
public:
  ~jhcBwVSrc ();
  jhcBwVSrc (const char *filename, int index =0);
  void Prefetch (int doit =1);
  void Close ();

// utilities
private:
  void Init (int reset =0);
  int SetSource (const char *filename);
  int graph_parts (const char *filename);
  int graph_connect (int mono);
  int graph_config ();
  void graph_reg ();

// required functions for jhcVideoSrc base class
private:
  int iGet (jhcImg& dest, int *advance, int src);

// required functions for ISampleGrabberCB base class
public:
	STDMETHODIMP_(ULONG) AddRef () {return ++ref;};   /** Increment reference count. */
	STDMETHODIMP_(ULONG) Release () {return --ref;};  /** Decrement reference count. */
	STDMETHODIMP SampleCB (double SampleTime,  IMediaSample *pSample)
	  {return S_OK;};                                 /** Ignored callback function. */
	STDMETHODIMP BufferCB (double SampleTime, BYTE *pBuffer, long BufferLen);
	STDMETHODIMP QueryInterface (REFIID riid, void **ppv);

};


/////////////////////////////////////////////////////////////////////////////

// part of mechanism for automatically associating class with file extensions

extern int jvreg_jhcBwVSrc;


#endif  // once




