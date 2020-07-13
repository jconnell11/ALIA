// jhcBwVSrc.cpp : reader for Broadware web video streams
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

#include "Video/jhcBwVSrc.h"


///////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                       //
///////////////////////////////////////////////////////////////////////////

// "bwims" is a dummy MIME-dispatch extension which gets stripped off

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

JREG_CAM(jhcBwVSrc, "bwims");

#endif


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcBwVSrc::~jhcBwVSrc ()
{
  Close();
  CloseHandle(grabbed);
  CoUninitialize(); 
}


//= Default constructor initializes certain values.

jhcBwVSrc::jhcBwVSrc (const char *filename, int index)
{
  strcpy_s(kind, "jhcBwVSrc");
  CoInitialize(NULL);
  grabbed = CreateEvent(NULL, TRUE, FALSE, NULL);  // must be un-named!!!
  ref = 0; 
  Init(1);
  SetSource(filename);
}


//= Set values of standard parameters.

void jhcBwVSrc::Init (int reset)
{
  // no filter graph yet
  builder = NULL;
	manager = NULL;
	source = NULL;
  codec = NULL;
	sample = NULL;
	nop = NULL;
	control = NULL;
  extract = NULL;
  reg = 0;
  run = 0;

  // unknown frame and video properties
  w = 0;
  h = 0;
  d = 0;
  aspect = 0;
  freq = 0;
  ResetEvent(grabbed);

  // possibly preserve error condition
  if ((ok > 0) || (reset > 0))
    ok = 0;
}


//= Completely reinitialize stream.

void jhcBwVSrc::Close ()
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
	  }

  // get rid of filter graph control points
  if (extract != NULL)
    extract->Release();
  if (control != NULL)
    control->Release();

  // get rid of all filter graph components
  if (nop != NULL)
    nop->Release();
	if (sample != NULL)
    sample->Release();
  if (codec != NULL)
    codec->Release();
  if (source != NULL)
    source->Release();

  // get rid of filter graph framework
  if (manager != NULL)
    manager->Release();
//  if (builder != NULL)             // cleaning up crashes the program!
//    builder->Release();

  // set default values but retain error value (if any)
  Init(0);
}


/////////////////////////////////////////////////////////////////////////////
//                           Stream construction                           //
/////////////////////////////////////////////////////////////////////////////

//= Makes up and initiatilizes a DirectShow file or stream reader.
// the manager element is a collection of filter objects
// the builder element strings them together to make them work
// the whole chain is: file_reader -> image_sampleor -> dummy_filter  
 
int jhcBwVSrc::SetSource (const char *filename) 
{
  const char *f;

  // check source name 
  Close();
  ok = -1;
  if (filename == NULL)
    return ok;
  ParseName(filename);
  f = FileNoExt;
  ok = 0;

  // build and configure the filter graph
  if (graph_parts(f) > 0)
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
// binds components builder, manager, source, sample, and nop
// binds interfaces control and extract

int jhcBwVSrc::graph_parts (const char *filename)
{
  CLSID target;
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

  // register stream reading element with the manager 
  len = (int) strlen(filename) + 1;
  wname = new WCHAR[len];
	mbstowcs_s(NULL, wname, len, filename, len);
  hr = manager->AddSourceFilter(wname, L"Video Stream", &source);
  if (wname != NULL)
    delete [] wname;
  if (FAILED(hr))
    return 0;

  // get proper decoder by looking up unique identifier
  CLSIDFromString(L"{301056D0-6DFF-11D2-9EEB-006008039E37}", &target);
  if (FAILED(CoCreateInstance(target, NULL, CLSCTX_INPROC_SERVER, 
                              IID_IBaseFilter, (void **)(&codec))))
    return 0;
	if (FAILED(manager->AddFilter(codec, L"jpegcodec")))
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
	if (FAILED(manager->AddFilter(nop, L"Null Renderer")))
    return 0;

  // get some internal control points of the filter graph
  if (FAILED(manager->QueryInterface(IID_IMediaControl, (void **)(&control))))
    return 0;
	if (FAILED(sample->QueryInterface(IID_ISampleGrabber, (void **)(&extract))))
    return 0;
  return 1;
}


//= Connects filter graph elements into a valid rendering chain

int jhcBwVSrc::graph_connect (int mono)
{
  IEnumPins *en;
  IPin *vpin = NULL, *gpin = NULL;
  PIN_INFO info;
	AM_MEDIA_TYPE mtype;

  // explicitly get the input pin to the codec
  // source must be configured before graph is connected in order to percolate
  if (FAILED(codec->EnumPins(&en)))
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

  // set the media type expected for the frame sampler
  // generally always opens as RGB, even if native monochrome source
  ZeroMemory(&mtype, sizeof(AM_MEDIA_TYPE));  // forces default values
	mtype.majortype = MEDIATYPE_Video;
  if (mono > 0)
    mtype.subtype = MEDIASUBTYPE_RGB8;
  else 
	  mtype.subtype = MEDIASUBTYPE_RGB24; 
	extract->SetMediaType(&mtype);

  // finish the graph by establishing the remaining codec->sample->nop connection
  if (FAILED(builder->RenderStream(NULL, NULL, codec, sample, nop)))
    return 0;
  return 1;
}


//= Set up graph for standard operation and get format of images returned.
// binds w, h, d, and freq

int jhcBwVSrc::graph_config ()
{
	AM_MEDIA_TYPE mtype;
  VIDEOINFOHEADER *vhdr;
  
  // register sample grabber to buffer samples and tell when this occurs
  extract->SetBufferSamples(TRUE);
	extract->SetCallback(this, 1);

  // get video information from frame sampleor
  if (FAILED(extract->GetConnectedMediaType(&mtype)))
    return 0;
  vhdr = (VIDEOINFOHEADER *)(mtype.pbFormat);

  // find out the size of images that will be returned and framerate
	w = (vhdr->bmiHeader).biWidth;   
	h = (vhdr->bmiHeader).biHeight;
  if ((vhdr->bmiHeader).biBitCount == 8)         
    d = 1;
  else
    d = 3;
  if (vhdr->AvgTimePerFrame == 0)
    freq = 15.0;                    // default frame rate
  else
    freq = 1.0e7 / vhdr->AvgTimePerFrame;
  f_time = 1.0 / freq;

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
// binds variable reg

void jhcBwVSrc::graph_reg ()
{
	IRunningObjectTable *rtab;
	IMoniker *id;
	WCHAR spec[128];
	
	if (FAILED(GetRunningObjectTable(0, &rtab))) 
    return;
	wsprintfW(spec, L"FilterGraph %08x pid %08x", (DWORD_PTR) manager, GetCurrentProcessId());
	if (SUCCEEDED(CreateItemMoniker(L"!", spec, &id)))
	{
		rtab->Register(0, manager, id, &reg);
		id->Release();
	}
	rtab->Release();
}


/////////////////////////////////////////////////////////////////////////////
//                           Core functionality                            //
/////////////////////////////////////////////////////////////////////////////

//= Read current frame as RGB into supplied array (random access mode).
// assumes array is big enough, writes BGR order, rows padded to next long word
// can adjust number of frames actually advanced

int jhcBwVSrc::iGet (jhcImg& dest, int *advance, int src)
{
  long sz = dest.PxlSize();
  double g_time;

  if (ok < 1)
    return 0;

  // set up to capture next frame
  Prefetch(1); 
  snap = p_time + (*advance - 0.5) * f_time;
  ResetEvent(grabbed);

  // wait for proper frame to arrive then find out when it occurred
  if (WaitForSingleObject(grabbed, 15000) == WAIT_TIMEOUT)  
    return 0;
  g_time = s_time;
  extract->GetCurrentBuffer(&sz, (long *)(dest.PxlDest()));
  *advance = ROUND((g_time - p_time) / f_time);
  p_time = g_time;
  return 1;
}


//= Starts reception and decoding of the video stream.
// frames can be grabbed with Get() quickly after this 
// cannot be stopped except by calling Close() method

void jhcBwVSrc::Prefetch (int doit)
{
  if ((ok < 1) || (doit <= 0) || ((doit > 0) && (run > 0)))
    return;
  run = -1;
  ResetEvent(grabbed);
  control->Run();
  WaitForSingleObject(grabbed, 60000);  // long timeout needed for stream buffering
  p_time = s_time;
  run = 1;
}


/////////////////////////////////////////////////////////////////////////////
//                      Required base class methods                        //
/////////////////////////////////////////////////////////////////////////////

//= Callback that occurs when a new frame is ready.
// SampleTime never changes for Broadware stream so use real time
// records time and sets completion flag

STDMETHODIMP jhcBwVSrc::BufferCB (double SampleTime, BYTE *pBuffer, long BufferLen)
{
  s_time = timeGetTime() / 1000.0;
  SetEvent(grabbed);
  return S_OK;
}

		
//= Return an object that mediates a certain type of interaction.
// since this class only samples frames, only acknowledge this kind of request 

STDMETHODIMP jhcBwVSrc::QueryInterface (REFIID iid, void **ppvObject)
{
	if ((iid != IID_ISampleGrabberCB) && (iid != IID_IUnknown))
  	return E_NOINTERFACE;
	*ppvObject = this;
  return S_OK;
}

