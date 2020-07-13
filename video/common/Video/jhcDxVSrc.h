// jhcDxVSrc.h : uses Microsoft DirectShow for cameras
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2005-2019 IBM Corporation
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

#ifndef _JHCDXVSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCDXVSRC_
/* CPPDOC_END_EXCLUDE */

// Note: to compile this you need to download the DirectX SDK from Microsoft
//   known to work with the December 2002 version (233,197,112 bytes)

#include "jhcGlobal.h"

#include <dshow.h>

#if (_MSC_VER >= 1400)
  #include "qedit_vs8.h"  // altered version in common
#else
  #include <qedit.h>
#endif

#include "Video/jhcVideoSrc.h"


//= Uses Microsoft DirectShow to read from cameras with WDM drivers.

class jhcDxVSrc : public jhcVideoSrc, public ISampleGrabberCB
{
protected:	
  HANDLE grabbed;
  double f_time;    /** How long each frame takes (in secs).            */
  double p_time;    /** The play position (in secs) for the last frame. */
  double s_time;    /** The play position (in secs) for current frame.  */
  double snap;      /** Target time (in secs) to capture next frame.    */
  int run;          /** Whether the graph is running (-1 = prefetch).   */
  int request;      /** Whether next frame has been called for yet.     */

  ICaptureGraphBuilder2 *builder;
	IGraphBuilder *manager;
	IBaseFilter *source;
	IBaseFilter *sample;
	IBaseFilter *nop;
	IMediaControl *control;
	ISampleGrabber *extract;
  IAMStreamConfig *format;
  DWORD reg;
  long ref;
jhcImg buf;

// main user calls (more in the base class)
public:
  ~jhcDxVSrc ();
  jhcDxVSrc (const char *spec =NULL, int index =0);
  int SetSource (const char *spec);
  void SetRate (double fps);
  void SetSize (int xmax, int ymax, int bw =0);
  void Prefetch (int doit =1);
  int Running () {return run;};  /** Whether video stream has been started. */
  void Close ();

  int SetVal (const char *tag, int val);
  int GetVal (int *val, const char *tag);
  int SetDef (const char *tag =NULL, int servo =0);
  int GetDef (int *vdef, const char *tag, int *vmin =NULL, int *vmax =NULL, int *vstep =NULL);

// utilities
protected:
  void Init (int reset =0);
  int graph_parts ();
  int graph_connect (int mono);
  int graph_config ();
  void graph_reg ();

  int open_src ();
  int src_bnd (const char *spec, int n);
  int src_dlg ();
  int format_dlg ();
  int adjust_dlg ();

  int scan_sizes (int xmax, int ymax, int f);
  int chk_size (int x, int y, int f);
  int force_size (int i, int x, int y, int f);

  int vidamp_param (const char *tag, int *val, int action =0, 
                    int *def =NULL, int *lo =NULL, int *hi =NULL, int *step =NULL);
  int camera_param (const char *tag, int *val, int action =0,
                    int *def =NULL, int *lo =NULL, int *hi =NULL, int *step =NULL);
  int vidcon_param (const char *tag, int *val, int action =0, 
                    int *def =NULL, int *lo =NULL, int *hi =NULL, int *step =NULL);

  void stop_graph();

// required functions for jhcVideoSrc base class
protected:
  int iGet (jhcImg& dest, int *advance, int src, int block);

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

extern int jvreg_jhcDxVSrc;


#endif  // once




