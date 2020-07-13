// jhcWmVSrc.cpp : uses Microsoft DirectShow for video files and streams
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

// Build.Settings.Link.General must have extra entries
#pragma comment (lib, "strmiids.lib")
#pragma comment (lib, "dmoguids.lib")
#pragma comment (lib, "wmcodecdspuuid.lib")

#include <dmodshow.h>
#include <dmoreg.h>
#include <wmcodecdsp.h>

#include "Video/jhcWmVSrc.h"


//////////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                          //
//////////////////////////////////////////////////////////////////////////////

// about 1.3x faster than jhcMpegVSrc and jhcAviVSrc
// handles URLs for files but cannot do MPEG-2 easily (add jhcMpegVSrc)
// Note: problem with AVI files with uncompressed frames (add jhcAviVSrc)

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

JREG_VURL(jhcWmVSrc, "wmv mov avi mpg mpeg m1v mp4");

#endif


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcWmVSrc::~jhcWmVSrc ()
{
  Close();
  CloseHandle(grabbed);
  CoUninitialize();   
}


//= Default constructor initializes certain values.

jhcWmVSrc::jhcWmVSrc (const char *filename, int index)
{
  strcpy_s(kind, "jhcWmVSrc");
  CoInitialize(NULL);
  grabbed = CreateEvent(NULL, TRUE, FALSE, NULL);  // must be un-named!!!
  ref = 0; 
  Init(1);
  SetSource(filename);
}


//= Set values of standard parameters.

void jhcWmVSrc::Init (int reset)
{
  // no filter graph yet
  builder = NULL;
	manager = NULL;
	source = NULL;
  colcvt = NULL;
	sample = NULL;
	nop = NULL;
	control = NULL;
	seek = NULL;
  extract = NULL;
  reg = 0;

  // unknown frame and video properties
  w = 0;
  h = 0;
  d = 0;
  aspect = 0;
  freq = 15.0;      // typical for surveillance cameras
  fstep = 666667;   // 1.0e7 / freq
  nframes = 0;
  ResetEvent(grabbed);

  // possibly preserve error condition
  if ((ok > 0) || (reset > 0))
    ok = 0;
}


/////////////////////////////////////////////////////////////////////////////
//                           Basic functions                               //
/////////////////////////////////////////////////////////////////////////////

//= Completely reinitialize stream.

void jhcWmVSrc::Close ()
{
	IRunningObjectTable *rtab;
  long state;

  // make sure video has stopped
  if (control != NULL)
  {
    control->Stop();
    control->GetState(10, &state);
  }

  // unregister graph with system (for GraphEdit utility)
  if (reg != 0)
	  if (SUCCEEDED(GetRunningObjectTable(0, &rtab)))
	  {
		  rtab->Revoke(reg);
		  rtab->Release();
      reg = 0;
	  }

  // get rid of filter graph control points
  if (extract != NULL)
    extract->Release();
  if (control != NULL)
    control->Release();
  if (seek != NULL)
    seek->Release();

  // get rid of all filter graph components
  if (nop != NULL)
    nop->Release();
	if (sample != NULL)
    sample->Release();
  if (colcvt != NULL)
    colcvt->Release();
  if (source != NULL)
    source->Release();

  // get rid of filter graph framework
  if (manager != NULL)
    manager->Release();
  if (builder != NULL)
    builder->Release();

  // set default values but retain error value (if any)
  Init(0);
}


//= Makes up and initiatilizes a DirectShow file or stream reader.
// the manager element is a collection of filter objects
// the builder element strings them together to make them work
// the whole chain is: file_reader -> image_sampleor -> dummy_filter  
 
int jhcWmVSrc::SetSource (const char *filename) 
{
  // check source name then determine video parameters
  Close();
  ok = -1;
  if (filename == NULL)
    return ok;
  ParseName(filename);
  ok = 0;

  // build and configure the filter graph
  if (graph_parts(filename) > 0)
    if (graph_connect(0) > 0)
      if (graph_config() > 0)
        ok = 1;

  // check result and clean up as necessary
  if (ok <= 0)
    Close();
  graph_reg();
  return ok;
}


//= Build basic filter graph structure for source.
// binds components builder, manager, source, sample, nop
// binds interfaces control, seek, and extract

int jhcWmVSrc::graph_parts (const char *filename)
{
  IDMOWrapperFilter *wrapper;
  WCHAR *wname = NULL;
  HRESULT hr;
  int len;

  // create graph builder and attach filter graph manager
  if (FAILED(CoCreateInstance(CLSID_CaptureGraphBuilder2, 0, CLSCTX_INPROC_SERVER,
	                            IID_ICaptureGraphBuilder2, (void **)(&builder))))
    return 0;
	if (FAILED(CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, 
	                            (void **)(&manager))))
    return 0;
	if (FAILED(builder->SetFiltergraph(manager)))
    return 0;

  // register file reading element with the manager 
  len = (int) strlen(filename) + 1;
  wname = new WCHAR[len];
	mbstowcs_s(NULL, wname, len, filename, len);
  hr = manager->AddSourceFilter(wname, L"Video File", &source);
  if (wname != NULL)
    delete [] wname;
  if (FAILED(hr))
    return 0;

  // create optional color convert module (for DTV-DVD decoder)
  // just having this in the graph seems to be enough
  if (FAILED(CoCreateInstance(CLSID_DMOWrapperFilter, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, 
                              reinterpret_cast<void**>(&colcvt))))
    return 0;
  if (FAILED(colcvt->QueryInterface(IID_IDMOWrapperFilter, reinterpret_cast<void**>(&wrapper))))
    return 0;
  hr = wrapper->Init(CLSID_CColorConvertDMO, DMOCATEGORY_VIDEO_EFFECT);
  wrapper->Release();
  if (FAILED(hr))
    return 0;
  if (FAILED(manager->AddFilter(colcvt, L"My DMO")))
    return 0;

  // create frame sampling filter and register it with the manager
	if (FAILED(CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
	                            IID_IBaseFilter, (void **)(&sample))))
    return 0;
	if (FAILED(manager->AddFilter(sample, L"Sample Grabber")))
    return 0;

  // create dummy rendering (sink) filter for end and register it with the manager
	if (FAILED(CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER,
	                            IID_IBaseFilter, (void **)(&nop))))
    return 0;
	if (FAILED(manager->AddFilter(nop, L"Null Filter")))
    return 0;

  // get some internal control points of the filter graph
	if (FAILED(sample->QueryInterface(IID_ISampleGrabber, (void **)(&extract))))
    return 0;
  if (FAILED(manager->QueryInterface(IID_IMediaControl, (void **)(&control))))
    return 0;
  if (FAILED(manager->QueryInterface(IID_IMediaSeeking, (void **)(&seek))))
    return 0;
  return 1;
}


//= Connects filter graph elements into a valid rendering chain
// leaves member variables vpin and gpin attached to proper filter ports

int jhcWmVSrc::graph_connect (int mono)
{
	AM_MEDIA_TYPE mtype;
  IEnumPins *en;
  IPin *vpin = NULL, *gpin = NULL;
  PIN_INFO info;

  // set the media type expected for the frame sampler
  // generally always opens as RGB, even if native monochrome source
  ZeroMemory(&mtype, sizeof(AM_MEDIA_TYPE));  // forces default values
	mtype.majortype = MEDIATYPE_Video;
  if (mono > 0)
    mtype.subtype = MEDIASUBTYPE_RGB8;
  else 
	  mtype.subtype = MEDIASUBTYPE_RGB24;
	extract->SetMediaType(&mtype);

  // explicitly get the input pin to the sample grabber
  // source must be configured before graph is connected in order to percolate
  if (FAILED(sample->EnumPins(&en)))
    return 0;
  while (en->Next(1, &gpin, 0) == S_OK)
  { 
    gpin->QueryPinInfo(&info);
    (info.pFilter)->Release();
    if (info.dir == PINDIR_INPUT)
      break;
    gpin->Release();
    gpin = NULL;
  }
  en->Release();
  if (gpin == NULL)
    return 0;

  // try all possible output pins from the source until one works
  // the three stage RenderStream call seems to get confused if audio present also
  // do not have to explicitly connect to the color converter for DTV-DVD source
  if (FAILED(source->EnumPins(&en)))
    return 0;
  while (en->Next(1, &vpin, 0) == S_OK)
  {   
    vpin->QueryPinInfo(&info);
    (info.pFilter)->Release();
    if (info.dir == PINDIR_OUTPUT)
      if (SUCCEEDED(manager->Connect(vpin, gpin)))
        break;
    vpin->Release();
    vpin = NULL;
  }
  en->Release();
  gpin->Release();
  if (vpin == NULL)
    return 0;
  vpin->Release();

  // finish the graph by establishing the remaining sample->nop connection
  if (FAILED(builder->RenderStream(NULL, NULL, sample, NULL, nop)))
    return 0;
  return 1;
}


//= Set up graph for standard operation and get format of images returned.
// binds w, h, d, freq, nframes, and fstep

int jhcWmVSrc::graph_config ()
{
  IMediaFilter *filter = NULL;
	AM_MEDIA_TYPE mtype;
  VIDEOINFOHEADER *vhdr = NULL;
  __int64 vtime = 0;

  // tell the graph to run as fast as possible
  if (FAILED(manager->QueryInterface(IID_IMediaFilter, (void **)(&filter))))
    return 0;
  filter->SetSyncSource(NULL);
  filter->Release();

  // tell sampler to buffer frames and tell when this occurs
  extract->SetBufferSamples(TRUE);
  extract->SetCallback(this, 1);

  // find out the size of images that will be returned
  if (FAILED(extract->GetConnectedMediaType(&mtype)))
    return 0;
  vhdr = (VIDEOINFOHEADER *)(mtype.pbFormat);
	w = (vhdr->bmiHeader).biWidth;   
	h = (vhdr->bmiHeader).biHeight;
  if ((vhdr->bmiHeader).biBitCount == 8)         
    d = 1;
  else
    d = 3;

  // check frame rate and total number of frames
  // Note: some WMV have time stamped frames so no explict rate or length
  if (vhdr->AvgTimePerFrame > 0)
    fstep = vhdr->AvgTimePerFrame;     
  if (fstep > 0)
  {
    freq = 1.0e7 / fstep;
    if (SUCCEEDED(seek->GetDuration(&vtime)))   
      nframes = (long)(vtime / fstep) - 1;  
  }

  // clean up from format query
  if (mtype.cbFormat != 0)
  {
    CoTaskMemFree((PVOID)(mtype.pbFormat));
    mtype.cbFormat = 0;
    mtype.pbFormat = NULL;
  }
  return 1;
}


//= Register the graph with the system for utility GraphEdit (not really required).
// for newer versions of Windows use must do "regsvr32 proppage.dll" to make it work
// binds variable reg

void jhcWmVSrc::graph_reg ()
{
	IRunningObjectTable *rtab;
	IMoniker *id;
	WCHAR spec[128];

	if (FAILED(GetRunningObjectTable(0, &rtab))) 
    return;
//	wsprintfW(spec, L"FilterGraph %08x pid %08x", (DWORD_PTR) manager, GetCurrentProcessId());
  StringCchPrintf(spec, 128,  L"FilterGraph %08x pid %08x", (DWORD_PTR) manager, GetCurrentProcessId()); 
	if (SUCCEEDED(CreateItemMoniker(L"!", spec, &id)))
	{
		rtab->Register(0, manager, id, &reg);
		id->Release();
	}
	rtab->Release();
}


/////////////////////////////////////////////////////////////////////////////
//                            Basic video functions                        //
/////////////////////////////////////////////////////////////////////////////

//= Position file pointer so desired line is the next one read.

int jhcWmVSrc::iSeek (int number)
{
  int n = __max(0, number - 1);
  __int64 foo = n * fstep;
  long state;

//  control->Stop();                // deadlock for AVI uncompressed?
  control->Pause();
  control->GetState(10, &state);
  ResetEvent(grabbed);
  if (FAILED(seek->SetPositions(&foo, AM_SEEKING_AbsolutePositioning, 
                                NULL, AM_SEEKING_NoPositioning)))
    return 0;
  return 1;
}


//= Read current frame as RGB into supplied array (random access mode).
// assumes array is big enough, writes BGR order, rows padded to next long word
// can adjust number of frames actually advanced

int jhcWmVSrc::iGet (jhcImg& dest, int *advance, int src, int block)
{
  long state, sz = dest.PxlSize();
  int patience = ((block > 0) ? 5000 : 0);

  wait = *advance;
  control->GetState(10, &state);
  if ((block > 0) && (state == State_Stopped))   // wait longer for first frame
    patience = 10000;
  control->Run();
  if (WaitForSingleObject(grabbed, patience) != WAIT_OBJECT_0)
    return 0;
  ResetEvent(grabbed);
  extract->GetCurrentBuffer(&sz, (long *)(dest.PxlDest()));
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                      Required base class methods                        //
/////////////////////////////////////////////////////////////////////////////

//= Callback that occurs when a new frame is ready.
// stops graph, records time, and sets completion flag

STDMETHODIMP jhcWmVSrc::BufferCB (double SampleTime, BYTE *pBuffer, long BufferLen)
{
  if (--wait <= 0)
  {
    control->Pause();
    SetEvent(grabbed);
    s_time = SampleTime;
  }
  return S_OK;
}

		
//= Return an object that mediates a certain type of interaction.
// since this class only samples frames, only acknowledge this kind of request 

STDMETHODIMP jhcWmVSrc::QueryInterface (REFIID iid, void **ppvObject)
{
	if ((iid != IID_ISampleGrabberCB) && (iid != IID_IUnknown))
  	return E_NOINTERFACE;
	*ppvObject = this;
  return S_OK;
}

