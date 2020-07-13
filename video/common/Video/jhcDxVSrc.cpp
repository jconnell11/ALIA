// jhcDxVSrc.cpp : uses Microsoft DirectShow for cameras
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

// Build.Settings.Link.General must have library strmiids.lib
#pragma comment (lib, "strmiids.lib")

#if _MSC_VER > 1200
  #ifdef _DEBUG
    #pragma comment(lib, "comsuppwd.lib")
  #else
    #pragma comment(lib, "comsuppw.lib")
  #endif
#endif


#include <comdef.h>                   // needed for _bstr_t
#include <dshow.h>
#include <ks.h>                       // for camera control
#include <ksmedia.h>        
#include <stdio.h>

#include "Interface/jhcMessage.h"

#include "Video/jhcDxVSrc.h"


///////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                       //
///////////////////////////////////////////////////////////////////////////

// "dx" is a dummy MIME-dispatch extension which gets stripped off

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

//JREG_CAM(jhcDxVSrc, "dx wdm vfw");
JREG_CAM(jhcDxVSrc, "dx wdm");

#endif


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcDxVSrc::~jhcDxVSrc ()
{
  Close();
  CloseHandle(grabbed);
  CoUninitialize(); 
}


//= Default constructor initializes certain values.

jhcDxVSrc::jhcDxVSrc (const char *filename, int index)
{
  strcpy_s(kind, "jhcDxVSrc");
  CoInitialize(NULL);
  grabbed = CreateEvent(NULL, TRUE, FALSE, NULL);  // must be un-named!!!
  ref = 0; 
  Init(1);
  SetSource(filename);
}


//= Set values of standard parameters.

void jhcDxVSrc::Init (int reset)
{
  // no filter graph yet
  builder = NULL;
	manager = NULL;
	source = NULL;
	sample = NULL;
	nop = NULL;
	control = NULL;
  extract = NULL;
  format = NULL;
  reg = 0;
  run = 0;
  s_time = 0;
  snap = 0;

  // unknown frame and video properties
  w = 0;
  h = 0;
  d = 0;
  aspect = 0;
  freq = 0;
  ResetEvent(grabbed);
  request = 0;

  // possibly preserve error condition
  if ((ok > 0) || (reset > 0))
    ok = 0;
}


//= Completely reinitialize stream.

void jhcDxVSrc::Close ()
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

  // get rid of camera controls
  if (format != NULL)
    format->Release();

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


/////////////////////////////////////////////////////////////////////////////
//                           Stream construction                           //
/////////////////////////////////////////////////////////////////////////////

//= Makes up and initiatilizes a DirectShow file or stream reader.
// A NULL name (or just ".dx") binds to the default driver (if any).
// Can also use a number (e.g. "0.dx" and "1.dx") for multiple grabbers 
//   or directory name and number (e.g. "logi/1.dx") for a certain class.
// A plus sign at the end (e.g. "0.dx+") allows user configuration of source.
// A base name of "*" pops a dialog box and lets user choose from a list.
 
int jhcDxVSrc::SetSource (const char *filename) 
{
  // check source name 
  Close();
  ok = -1;
  if (filename == NULL)
    return ok;
  ParseName(filename);
  ok = 0;

  // build and configure the filter graph
  if (graph_parts() > 0)
    if (open_src() > 0)
      if (graph_connect(0) > 0)
        if (graph_config() > 0)
          ok = 1;

  // check result and clean up as necessary
  if (ok <= 0)
    Close();
  graph_reg();
  return ok;
}


//= Build basic filter graph structure (except source).
// the manager element is a collection of filter objects
// the builder element strings them together to make them work
// the whole chain is: file_reader -> image_sampler -> dummy_filter  
// binds components builder, manager, sample, and nop
// binds interfaces control and extract

int jhcDxVSrc::graph_parts ()
{
  // create graph builder and attach filter graph manager
  if (FAILED(CoCreateInstance(CLSID_CaptureGraphBuilder2, 0, CLSCTX_INPROC_SERVER,
	                            IID_ICaptureGraphBuilder2, (void **)(&builder))))
    return 0;
	if (FAILED(CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, 
	                            (void **)(&manager))))
    return 0;
	if (FAILED(builder->SetFiltergraph(manager)))
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
  if (FAILED(manager->QueryInterface(IID_IMediaControl, (void **)(&control))))
    return 0;
	if (FAILED(sample->QueryInterface(IID_ISampleGrabber, (void **)(&extract))))
    return 0;
  return 1;
}


//= Connects filter graph elements into a valid rendering chain.

int jhcDxVSrc::graph_connect (int mono)
{
	AM_MEDIA_TYPE mtype;

  // set the media type expected for the frame sampler
  // generally always opens as RGB, even if native monochrome source
  ZeroMemory(&mtype, sizeof(AM_MEDIA_TYPE));  // forces default values
	mtype.majortype = MEDIATYPE_Video;
  if (mono > 0)
    mtype.subtype = MEDIASUBTYPE_RGB8;
  else 
	  mtype.subtype = MEDIASUBTYPE_RGB24;
	extract->SetMediaType(&mtype);

  // connect three major blocks of the graph, inserting an input analog mux if needed
  if (FAILED(builder->RenderStream(&PIN_CATEGORY_CAPTURE, NULL, source, sample, nop)))
    return 0;
  return 1;
}


//= Set up graph for standard operation and get format of images returned.
// binds w, h, d, and freq

int jhcDxVSrc::graph_config ()
{
	AM_MEDIA_TYPE mtype;
  VIDEOINFOHEADER *vhdr;
  
  // register sample grabber to buffer samples and tell when this occurs
  extract->SetBufferSamples(TRUE);
	extract->SetCallback(this, 1);

  // get video information from frame sampler
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

  // build temporary frame buffer
  buf.SetSize(w, h, d);

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
// use GraphEdit menu "File : Connect to Remote Graph" to view structure
// binds variable reg

void jhcDxVSrc::graph_reg ()
{
	IRunningObjectTable *rtab;
	IMoniker *id;
	WCHAR spec[128];
	
	if (FAILED(GetRunningObjectTable(0, &rtab))) 
    return;
//	wsprintfW(spec, L"FilterGraph %08x pid %08x", (DWORD_PTR) manager, GetCurrentProcessId());
  StringCchPrintf(spec, 128, L"FilterGraph %08x pid %08x", (DWORD_PTR) manager, GetCurrentProcessId());
	if (SUCCEEDED(CreateItemMoniker(L"!", spec, &id)))
	{
		rtab->Register(0, manager, id, &reg);
		id->Release();
	}
	rtab->Release();
}


/////////////////////////////////////////////////////////////////////////////
//                Interactive framegrabber configuration                   //
/////////////////////////////////////////////////////////////////////////////

//= Find the correct camera based on the specification string.
// assumes base class ParseName() has been called on specification string
// binds component source as well as interface format

int jhcDxVSrc::open_src ()
{
  char spec_name[80] = "";
  int i, n, dnum = 0;

  // if wildcard, bind source filter from user list
  if (*BaseName == '*')                                    
  {
    if (src_dlg() <= 0)
      return 0;
  }
  else 
  {
    // set unit to given value if filename is a single number
    n = (int) strlen(BaseName);
    for (i = 0; i < n; i++)
      if (isdigit(BaseName[i]) == 0)
        break;
    if (i >= n)
      sscanf_s(BaseName, "%d", &dnum);

    // get complete or partial device name (if any)
    if (*DirNoDisk != '\0')
    {
      strcpy_s(spec_name, DirNoDisk);
      n = (int) strlen(spec_name) - 1;
      spec_name[n] = '\0';
    }

    // attempt to find and bind a camera with specified name
    if (src_bnd(spec_name, dnum) <= 0)
      return 0;
  }

  // bind some camera adjustment interfaces
  if (FAILED(builder->FindInterface(&PIN_CATEGORY_CAPTURE, NULL, source, 
                                    IID_IAMStreamConfig, (void **)(&format))))
    return 0;

  // see if camera should be user configured
  n = (int) strlen(Ext) - 1;
  if (Ext[n] == '+') 
  {
    format_dlg();
    adjust_dlg();
  }
  return 1;
}


//= Connects a video capture device with a particular index.
// for instance, ("logi", 1) sorts through all Logitech cameras to find the second one 

int jhcDxVSrc::src_bnd (const char *spec, int n)
{
	ICreateDevEnum *gen_enum;
	IEnumMoniker *vid_enum;
  IMoniker *item;
	IPropertyBag *props;
	VARIANT var_name;
  char item_name[200];
  int len = (int) strlen(spec), i = -1, ans = 0;

	// create a framegrabber enumerator 
  if (FAILED(CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, 
	                            IID_ICreateDevEnum, reinterpret_cast<void**>(&gen_enum))))
    return 0;
  if (gen_enum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &vid_enum, 0) != S_OK)
  {
    gen_enum->Release();
    return 0;
  }

  // scan through list of available devices
  while (vid_enum->Next(1, &item, NULL) == S_OK)
  {
    if (*spec == '\0')
      i++;
    else if (SUCCEEDED(item->BindToStorage(0, 0, IID_IPropertyBag, (void **)(&props))))
	  {
      // find the device name as a normal string
      *item_name = '\0';
		  VariantInit(&var_name);
		  if (FAILED(props->Read(L"Description", &var_name, 0)))
        props->Read(L"FriendlyName", &var_name, 0);
      _bstr_t b_name(var_name.bstrVal, FALSE);
      strcpy_s(item_name, (char *) b_name);
		  VariantClear(&var_name); 

      // if front of name matches then update instance count
      if (_strnicmp(item_name, spec, len) == 0)
        i++;
      props->Release();
		}

    // if correct instance of device found, bind it as the source filter
    if (i == n)
    {
		  if (SUCCEEDED(item->BindToObject(0, 0, IID_IBaseFilter, (void **)(&source))))
        if (SUCCEEDED(manager->AddFilter(source, L"Camera")))
          ans = 1;
      item->Release();
      break;
    }
    item->Release();   
  }

  // clean up device enumerators
	vid_enum->Release();
	gen_enum->Release();
  return ans;
}


//= Pop up a dialog box allowing user to choose between cameras.
// used this method instead of a ComboBox to make ascii interfaces easier

int jhcDxVSrc::src_dlg () 
{
	ICreateDevEnum *gen_enum;
	IEnumMoniker *vid_enum = NULL;
  IMoniker *item;
	IPropertyBag *props;
	VARIANT var_name;
  char item_name[200], msg[1000], dev_name[20][80];
  int i, n = 0, s = 0, ans = 0;

	// create a framegrabber enumerator 
  if (FAILED(CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, 
	                            IID_ICreateDevEnum, reinterpret_cast<void**>(&gen_enum))))
    return 0;
  if (FAILED(gen_enum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &vid_enum, 0)) || (vid_enum == NULL))
  {
    gen_enum->Release();
    return 0;
  }

  // accumulate print names into array
	while (vid_enum->Next(1, &item, NULL) == S_OK)
    if (n >= 20)
      break;
    else if (SUCCEEDED(item->BindToStorage(0, 0, IID_IPropertyBag, (void **)(&props))))
		{
      // find the device name as a normal string
      *item_name = '\0';
		  VariantInit(&var_name);
		  if (FAILED(props->Read(L"Description", &var_name, 0)))
        props->Read(L"FriendlyName", &var_name, 0);
      _bstr_t b_name(var_name.bstrVal, FALSE);
      strcpy_s(item_name, (char *) b_name);
		  VariantClear(&var_name); 

      // copy to local array
      strcpy_s(dev_name[n++], item_name);
      props->Release();
      item->Release();
    }

  // pop multiple dialog boxes with moving caret "==>"
  for (s = 0; s < n; s++)
  {
    strcpy_s(msg, "Select this driver?\n\n");
    for (i = 0; i < n; i++)
    {
      if (i == s)
        strcat_s(msg, "==>");
      strcat_s(msg, "\t");
      strcat_s(msg, dev_name[i]);
      strcat_s(msg, "\n");
    }
    if (Ask(msg))
      break;
  }

  if (s < n)
  {
    // go back through list to find selection
    vid_enum->Reset();
    vid_enum->Skip(s);
    vid_enum->Next(1, &item, NULL);

    // try binding source and change video name if successful
    if (SUCCEEDED(item->BindToObject(0, 0, IID_IBaseFilter, (void **)(&source))))
      if (SUCCEEDED(manager->AddFilter(source, L"Camera")))
      {
        sprintf_s(msg, "%d%s", s, Ext);
        ParseName(msg);
        ans = 1;
      }
    item->Release();
  }

  // clean up device enumerators
  vid_enum->Release();
	gen_enum->Release();
  return ans;
}


//= Let user edit things like image size and native color space.

int jhcDxVSrc::format_dlg ()
{
	ISpecifyPropertyPages *props;
	CAUUID pages;

  // get property page for encoder
	if (FAILED(format->QueryInterface(IID_ISpecifyPropertyPages, (void **)(&props))))
    return 0;
	if (FAILED(props->GetPages(&pages)))
  {
  	props->Release();
    return 0;
  }

  // pop up the standard dialog box then clean up
	OleCreatePropertyFrame(NULL, 30, 30, NULL, 1, (IUnknown **)(&format), 
                         pages.cElems, pages.pElems, 0, 0, NULL);
	CoTaskMemFree(pages.pElems);
	props->Release();
  return 1;
}


//= Let user edit things like exposure and color balance.

int jhcDxVSrc::adjust_dlg ()
{
	ISpecifyPropertyPages *props;
	CAUUID pages;

  // get property pages for camera
  if (FAILED(source->QueryInterface(IID_ISpecifyPropertyPages, (void **)(&props))))
    return 0;
	if (FAILED(props->GetPages(&pages)))
  {
  	props->Release();
    return 0;
  }

  // pop up the standard dialog box then clean up
  OleCreatePropertyFrame(NULL, 30, 30, NULL, 1, (IUnknown **)(&source),
	  	                   pages.cElems, pages.pElems, 0, 0, NULL);
  CoTaskMemFree(pages.pElems);
	props->Release();
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                   Framerate and image size adjustments                  //
/////////////////////////////////////////////////////////////////////////////

//= Request a particular number of frames per second.
// not guaranteed to set exactly (or even do anything)
// check rate afterwards to see what happened

void jhcDxVSrc::SetRate (double fps)
{
  VIDEOINFOHEADER *vhdr;
  AM_MEDIA_TYPE *mtype;

  // stop the video rendering chain
  stop_graph();

  // attempt to set the curent mode to use the new framerate
  if (FAILED(format->GetFormat(&mtype)))
    return;
  vhdr = reinterpret_cast<VIDEOINFOHEADER*>(mtype->pbFormat);
  vhdr->AvgTimePerFrame = ROUND(1e7 / fps);
  format->SetFormat(mtype);

  // clean up media type object
  if (mtype->cbFormat != 0)
  {
    CoTaskMemFree((PVOID)(mtype->pbFormat));
    mtype->cbFormat = 0;
    mtype->pbFormat = NULL;
  }
  CoTaskMemFree((PVOID) mtype);

  // update output image parameters 
  graph_config();
}


//= See if capture device can automatically subsample images.
// if all parameters are zero, keeps current output format
// member variable "ok" reflects success (1) or failure (0)
// NOTE: to force a LARGER size, use a big non-zero dimension

void jhcDxVSrc::SetSize (int xmax, int ymax, int bw)
{
  int x = xmax, y = ymax;

  if (!Valid())
    return;

  // see if any action is needed 
  // if only changing the color depth then preserve the size
  if ((xmax <= 0) && (ymax <= 0))
  {
    if (((bw > 0) && (d == 1)) || ((bw <= 0) && (d == 3)))
      return;
    x = w;
    y = h;
  }

  // stop the video rendering chain
  stop_graph();
 
  // try to get a native monochrome mode if requested
  if (bw > 0)
  {
    if (scan_sizes(x, y, 1) <= 0)
      scan_sizes(x, y, 3);
    return; 
  }

  // else prefer an RGB color mode
  if (scan_sizes(x, y, 3) <= 0)
    scan_sizes(x, y, 1);
}


//= Tries to pick a standard image format that satisfies the constraints.
// width should be less than x (if nonzero) and height less than y (if nonzero) 

int jhcDxVSrc::scan_sizes (int xmax, int ymax, int f)
{
  int i, k, wbest = 0, hbest = 0, winner = -1;
  static int sz[35][2] = {{1600, 1200}, {1280, 1024},                // hi-def 
                          {1280,  960}, { 800,  600},  
                          { 640,  480}, { 704,  480}, { 720,  480},  // NTSC
                          { 320,  240}, { 352,  240}, { 360,  240}, 
                          { 240,  180}, { 264,  180}, { 270,  180}, 
                          { 240,  176}, { 264,  176}, { 270,  176}, 
                          { 160,  120}, { 176,  120}, { 180,  120}, 
                          { 512,  480}, { 512,  512},                // Legacy
                          { 256,  240}, { 256,  256}, 
                          { 192,  176}, { 192,  192}, 
                          { 128,  120}, { 128,  128},
                          { 704,  576}, { 720,  576},                // PAL
                          { 352,  288}, { 360,  288}, 
                          { 264,  216}, { 270,  216}, 
                          { 176,  144}, { 180,  144}}; 

  // try sizes from big to small (square pixels preferred so first)
  for (i = 0; i < 35; i++)
  {
    // work down until under limit (if any)
    if ((winner >= 0) && ((sz[i][0] > wbest) || (sz[i][1] > hbest)))
      continue;

    // see if the capture driver will take the new format
    if ((k = chk_size(sz[i][0], sz[i][1], f)) >= 0)
    {
      wbest = sz[i][0];
      hbest = sz[i][1];
      winner = k;
      if (((xmax <= 0) || (wbest <= xmax)) &&
          ((ymax <= 0) || (hbest <= ymax)))
        break;
    }
  }

  // set framegrabber to the best size found (if any are okay)
  if (winner >= 0) 
    if (force_size(winner, wbest, hbest, f) > 0)
      return 1;
  return 0;
}


//= Check if framegrabber can produce images of the given size.
// always passed a complete size specification (no zeroes)
// returns index of mode if okay, returns -1 if fails

int jhcDxVSrc::chk_size (int x, int y, int f)
{
  VIDEO_STREAM_CONFIG_CAPS mode;
  AM_MEDIA_TYPE *mtype;
  int i, k = 0, n = 0, ans = -1;

  // look through all video modes in the list to find something that matches
  format->GetNumberOfCapabilities(&n, &i);
  for (i = 0; i < n; i++)
    if (SUCCEEDED(format->GetStreamCaps(i, &mtype, reinterpret_cast<BYTE*>(&mode))))
    {
      // check that this is a proper RGB or mono video format
      if ((mtype->formattype == FORMAT_VideoInfo) &&          
          (((f == 3) && (mtype->subtype == MEDIASUBTYPE_RGB24)) ||  
           ((f == 1) && (mtype->subtype == MEDIASUBTYPE_RGB8))))
        while (1)
        {
          // see if width can be generated by device
          if (mode.OutputGranularityX > 0)
            k = (x - (mode.MinOutputSize).cx) / mode.OutputGranularityX;
          if (x != ((mode.MinOutputSize).cx + k * mode.OutputGranularityX))
            break;

          // see if width can be generated by device
          if (mode.OutputGranularityY > 0)
            k = (y - (mode.MinOutputSize).cy) / mode.OutputGranularityY;
          if (y != ((mode.MinOutputSize).cy + k * mode.OutputGranularityY))
            break;

          // apparently the given size is acceptable
          ans = i;
          break;
        }

      // clean up media type object
      if (mtype->cbFormat != 0)
      {
        CoTaskMemFree((PVOID)(mtype->pbFormat));
        mtype->cbFormat = 0;
        mtype->pbFormat = NULL;
      }
      CoTaskMemFree((PVOID) mtype);

      // stop early if success
      if (ans >= 0)
        return ans;
   }
  return -1;
}


//= Set a particular mode of the framegrabber to certain dimensions.

int jhcDxVSrc::force_size (int i, int x, int y, int f)
{
  VIDEO_STREAM_CONFIG_CAPS mode;
  VIDEOINFOHEADER *vhdr;
  AM_MEDIA_TYPE *mtype;
  HRESULT hr;
  int sz;

  // get pointer to requested mode
  if (FAILED(format->GetStreamCaps(i, &mtype, reinterpret_cast<BYTE*>(&mode))))
    return 0;

  // modify the mode to match the given specifications
  vhdr = reinterpret_cast<VIDEOINFOHEADER*>(mtype->pbFormat);
  sz = (((x * f) + 3) & ~3) * y;
  mtype->lSampleSize = sz;
  (vhdr->bmiHeader).biSizeImage = sz;
  (vhdr->bmiHeader).biWidth = x;
  (vhdr->bmiHeader).biHeight = y;

  // attempt to set the device to this new format            
  hr = format->SetFormat(mtype);

  // clean up media type object
  if (mtype->cbFormat != 0)
  {
    CoTaskMemFree((PVOID)(mtype->pbFormat));
    mtype->cbFormat = 0;
    mtype->pbFormat = NULL;
  }
  CoTaskMemFree((PVOID) mtype);

  // if it worked then update output image parameters 
  if (FAILED(hr))
    return 0;
  graph_config();
  return 1;
}


/////////////////////////////////////////////////////////////////////////////
//                     Run-time camera adjustments                         //
/////////////////////////////////////////////////////////////////////////////

//= Set value of some source input parameter.
// useful for framegrabber properties such as exposure and color balance
// returns 0 if property is unknown or not implemented
// only responds to property names below: 
//
//   Brightness:  For NTSC, the value is expressed in IRE units * 100. 
//                For non-NTSC sources, the units are arbitrary, with zero 
//                representing blanking and 10000 representing pure white. 
//   Contrast:    Expressed as gain factor * 100. Values range from zero to 10000.
//   Hue:         Value is in degrees * 100. Values range -180 to +180 degrees.
//   Saturation:  Values range from 0 to 10,000.
//   Sharpness:   Values range from 0 to 100.
//   Gamma :      Value is gamma * 100. Values range from 1 to 500. 
//   Color:       The possible values are 0 (mono) and 1 (color). 
//   Color_Temp:  Value is a temperature in degrees Kelvin (sometimes). This is the AWB setting.
//   BLC:         Possible values are 0 (off) and 1 (on).
//   Gain:        Zero is normal. Positive values are brighter and negative values 
//                are darker. The range of values depends on the device.
//
//   Pan:         Values range from -180 to +180 degrees, with the default set to zero. 
//                Positive values are clockwise from the origin (the camera rotates clockwise when
//                viewed from above), and negative values are counterclockwise from the origin. 
//   Tilt:        Values range from -180 to +180 degress, with the default set to zero. 
//                Positive values point the imaging plane up, and negative values point the imaging plane down. 
//   Roll:        Values range from -180 to +180, with the default set to zero. 
//                Positive values cause a clockwise rotation of the camera along the image-viewing axis, 
//                and negative values cause a counterclockwise rotation of the camera. 
//   Zoom:        Values range from 10 to 600 millimeters (sometimes). 
//   Exposure:    Values are in log base 2 seconds (sometimes). This is the AGC setting.
//   Iris:        Units are fstop * 10 (uncommon parameter). 
//   Focus:       Distance to the optimally focused target, in millimeters (uncommon parameter). 
//
//   FlipH:       Flip the image left to right (mirror). Typically toggles, read is meaningless.
//   FlipV:       Flip the image top to bottom (invert). Typically toggles, read is meaningless.
//   Trig_En:     Sets up a stream to capture a trigger when button on camera pushed.
//   Trigger:     Simulates an external trigger (often on for normal streaming capture).
//
// Brightness, Contrast, Saturation, and Exposure work on most devices.
// Pan, Tilt, and Zoom are useful with newer Logitech cameras (software PTZ!).

int jhcDxVSrc::SetVal (const char *tag, int val)
{
  int ans;

  if ((ans = vidamp_param(tag, &val, 1)) > 0)
    return ans;
  if ((ans = camera_param(tag, &val, 1)) > 0)
    return ans;
  if ((ans = vidcon_param(tag, &val, 1)) > 0)
    return ans;
  return 0;
}


//= Get current value of some source input parameter.
// useful for framegrabber properties such as brightness and contrast
// returns 0 if unknown or unimplemented, 1 if automatic now, 2 if manual now

int jhcDxVSrc::GetVal (int *val, const char *tag)
{
  int ans;

  if ((ans = vidamp_param(tag, val)) > 0)
    return ans;
  if ((ans = camera_param(tag, val)) > 0)
    return ans;
  if ((ans = vidcon_param(tag, val)) > 0)
    return ans;
  return 0;
}


//= Set some framegrabber parameter to its default value.
// use NULL for tag to set all, use servo > 0 for automatic adjustment
// returns 0 if unknown or unimplemented

int jhcDxVSrc::SetDef (const char *tag, int servo)
{
  int ans, mode = ((servo > 0) ? 2 : 1);

  // special case of all parameters
  if (tag == NULL)
  {
    vidamp_param(NULL, NULL, mode);
    camera_param(NULL, NULL, mode);
    vidcon_param(NULL, NULL, mode);
    return 1;
  }

  // normal case of named parameter
  if ((ans = vidamp_param(tag, NULL, mode)) > 0)
    return ans;
  if ((ans = camera_param(tag, NULL, mode)) > 0)
    return ans;
  if ((ans = vidcon_param(tag, NULL, mode)) > 0)
    return ans;
  return 0;
}

 
//= Get default value for some framegrabber property.
// also reports range and adjustment step size
// returns 0 if unknown or unimplemented, 

int jhcDxVSrc::GetDef (int *vdef, const char *tag, int *vmin, int *vmax, int *vstep)
{
  int ans;

  if ((ans = vidamp_param(tag, NULL, 0, vdef, vmin, vmax, vstep)) > 0)
    return ans;
  if ((ans = camera_param(tag, NULL, 0, vdef, vmin, vmax, vstep)) > 0)
    return ans;
  if ((ans = vidcon_param(tag, NULL, 0, vdef, vmin, vmax, vstep)) > 0)
    return ans;
  return 0;
}

                                     
//= See if the tag is associated with the video processor amplifier. 
// returns 0 if unknown or unimplemented, 1 if automatic, 2 if manual
// binds the min and max range values and value step size if requested
// action: 0 = read, 1 = write (possibly default), 2 = switch to automatic 

int jhcDxVSrc::vidamp_param (const char *tag, int *val, int action,
                             int *def, int *lo, int *hi, int *step)
{
  char props[10][40] = {"Brightness", "Contrast", "Hue",        "Saturation", "Sharpness", 
                        "Gamma",      "Color",    "Color_Temp", "BLC",        "Gain"};
  VideoProcAmpProperty keys[10] = {VideoProcAmp_Brightness,            VideoProcAmp_Contrast, 
                                   VideoProcAmp_Hue,                   VideoProcAmp_Saturation, 
                                   VideoProcAmp_Sharpness,             VideoProcAmp_Gamma, 
                                   VideoProcAmp_ColorEnable,           VideoProcAmp_WhiteBalance, 
                                   VideoProcAmp_BacklightCompensation, VideoProcAmp_Gain};
	IAMVideoProcAmp *adjust = NULL;
  long lval, vmin, vmax, vstep, vdef, mode;
  int i, n, ans = 0;

  // special case of all parameters
  if ((tag == NULL) && (val == NULL) && (action > 0))
  {
    for (i = 0; i < 10; i++)
      vidamp_param(props[i], NULL, action);
    return action;
  }

  // find index (if any) of property in list
  if (tag == NULL)
    return 0;
  for (i = 0; i < 10; i++)
    if (_stricmp(tag, props[i]) == 0)
      break;
  if (i >= 10)
    return 0;

  // see if proper interface exists
  if (FAILED(source->QueryInterface(IID_IAMVideoProcAmp, (void **)(&adjust))))
    return 0;

  while (1)
  {
    // maybe try reading current value
    if ((action <= 0) && (val != NULL))
    {
      if (FAILED(adjust->Get(keys[i], &lval, &mode)))
        break;
      *val = (int) lval;
      ans = 1;
      if ((mode & VideoProcAmp_Flags_Manual) != 0)
        ans = 2;
      break;
    }

    // read in overall range specifications
    if (FAILED(adjust->GetRange(keys[i], &vmin, &vmax, &vstep, &vdef, &mode)))
      break;

    // see if only defaults requested 
    if (action <= 0) 
    {
      if (def != NULL)
        *def = (int) vdef;
      if (lo != NULL)
        *lo = (int) vmin;
      if (hi != NULL)
        *hi = (int) vmax;
      if (step != NULL)
        *step = (int) vstep;
      ans = 1;
      break;
    }
    
    // maybe try resetting value to automatic mode
    if (action >= 2)
    {
      if (SUCCEEDED(adjust->Set(keys[i], vdef, VideoProcAmp_Flags_Auto)))
        ans = 1;
      break;
    }

    // maybe try setting value to specific value supplied
    if (val != NULL)
    {
      n = ROUND((*val - vmin) / (double) vstep);
      lval = (long)(vmin + n * vstep);
      lval = __max(vmin, __min(lval, vmax));
      if (SUCCEEDED(adjust->Set(keys[i], lval, VideoProcAmp_Flags_Manual)))
        ans = 2;
      break;
    }

    // otherwise try setting value to default 
    if (SUCCEEDED(adjust->Set(keys[i], vdef, VideoProcAmp_Flags_Manual)))
      ans = 2;
    break;
  }
  
  // clean up 
  adjust->Release();
  return ans;
}


//= See if the tag is associated with the camera control interface. 
// returns 0 if unknown or unimplemented, 1 if automatic, 2 if manual
// binds the min and max range values and value step size if requested
// set_mode: 0 = read, 1 = write, 2 = switch to automatic 

int jhcDxVSrc::camera_param (const char *tag, int *val, int action,
                             int *def, int *lo, int *hi, int *step)
{
  char props[7][40] = {"Pan", "Tilt", "Roll", "Zoom", "Exposure", "Iris", "Focus"};
  CameraControlProperty keys[7] = {CameraControl_Pan,  CameraControl_Tilt,     CameraControl_Roll, 
                                   CameraControl_Zoom, CameraControl_Exposure, CameraControl_Iris, 
                                   CameraControl_Focus};
	IAMCameraControl *adjust = NULL;
  long lval, vmin, vmax, vstep, vdef, mode, flags;
  int i, n, ans = 0;

  // special case of all parameters
  if ((tag == NULL) && (val == NULL) && (action > 0))
  {
    for (i = 0; i < 7; i++)
      camera_param(props[i], NULL, action);
    return action;
  }

  // find index (if any) of property in list
  if (tag == NULL)
    return 0;
  for (i = 0; i < 7; i++)
    if (_stricmp(tag, props[i]) == 0)
      break;
  if (i >= 7)
    return 0;

  // see if proper interface exists
  if (FAILED(source->QueryInterface(IID_IAMCameraControl, (void **)(&adjust))))
    return 0;

  while (1)
  {
    // maybe try reading current value
    if ((action <= 0) && (val != NULL))
    {
      if (FAILED(adjust->Get(keys[i], &lval, &mode)))
        break;
      *val = (int) lval;
      ans = 1;
      if ((mode & CameraControl_Flags_Manual) != 0)
        ans = 2;
      break;
    }

    // read in overall range specifications 
    if (FAILED(adjust->GetRange(keys[i], &vmin, &vmax, &vstep, &vdef, &mode)))
      break;

    // see if only defaults requested 
    if (action <= 0) 
    {
      if (def != NULL)
        *def = (int) vdef;
      if (lo != NULL)
        *lo = (int) vmin;
      if (hi != NULL)
        *hi = (int) vmax;
      if (step != NULL)
        *step = (int) vstep;
      ans = 1;
      break;
    }
    
    // maybe try resetting value to automatic mode
    if (action >= 2)
    {
      if (SUCCEEDED(adjust->Set(keys[i], vdef, CameraControl_Flags_Auto)))
        ans = 1;
      break;
    }

    // maybe try setting value to specific value supplied
    if (val != NULL)
    {
      // coerce actual request to a valid value
      n = ROUND((*val - vmin) / (double) vstep);
      lval = (long)(vmin + n * vstep);
      lval = __max(vmin, __min(lval, vmax));

      // use special flags for mechanical pan and tilt

      if ((i == 0) || (i == 1))
        flags = KSPROPERTY_CAMERACONTROL_FLAGS_RELATIVE | KSPROPERTY_CAMERACONTROL_FLAGS_MANUAL;
      else
        flags = CameraControl_Flags_Manual;
      if (SUCCEEDED(adjust->Set(keys[i], lval, flags)))
        ans = 2;
      break;
    }

    // otherwise maybe try setting value to default 
    if (SUCCEEDED(adjust->Set(keys[i], vdef, CameraControl_Flags_Manual)))
      ans = 2;
    break;
  }
  
  // clean up 
  adjust->Release();
  return ans;
}


//= See if the tag is associated with the video control interface. 
// returns 0 if unknown or unimplemented, 1 if automatic, 2 if manual
// binds the min and max range values and value step size if requested
// action: 0 = read, 1 = write (possibly default), 2 = switch to automatic 

int jhcDxVSrc::vidcon_param (const char *tag, int *val, int action,
                             int *def, int *lo, int *hi, int *step)
{
  char props[4][40] = {"FlipV", "FlipH", "Trig_En", "Trigger"};
  VideoControlFlags keys[4] = {VideoControlFlag_FlipHorizontal, VideoControlFlag_FlipVertical,
                               VideoControlFlag_ExternalTriggerEnable, VideoControlFlag_Trigger};
  int vdef[4] = {0, 0, 1, 1};
  IAMVideoControl *adjust = NULL;
  IPin *capture = NULL;
  long mode;
  int i, ans = 0;

  // no way to set any of these to automatic
  if (action >= 2)
    return 0;

  // special case of all parameters
  if ((tag == NULL) && (val == NULL) && (action > 0))
  {
    for (i = 0; i < 4; i++)
      vidcon_param(props[i], NULL, action);
    return action;
  }

  // find index (if any) of property in list
  if (tag == NULL)
    return 0;
  for (i = 0; i < 4; i++)
    if (_stricmp(tag, props[i]) == 0)
      break;
  if (i >= 4)
    return 0;

  // see if proper interface exists and get video output pin from source 
  if (FAILED(source->QueryInterface(IID_IAMVideoControl, (void **)(&adjust))))
    return 0;
  if (FAILED(builder->FindPin(source, PINDIR_OUTPUT, &PIN_CATEGORY_CAPTURE, 
                              NULL, FALSE, 0, &capture)))
  {
    adjust->Release();
    return 0;
  }

  while (1)
  {
    // see if specific property is implemented
    if (FAILED(adjust->GetCaps(capture, &mode)))
      break;
    if (((mode >> i) & 0x01) == 0)
      break;

    // see if just defaults requested 
    if ((action <= 0) && (val == NULL))
    {
      if (def != NULL)
        *def = vdef[i];
      if (lo != NULL)
        *lo = 0;
      if (hi != NULL)
        *hi = 1;
      if (step != NULL)
        *step = 1;
      ans = 1;
      break;
    }

    // try reading the current value
    if (FAILED(adjust->GetMode(capture, &mode)))
      break;

    // maybe just interpret the current value 
    if (action <= 0)
    {
      *val = 0;
      if ((mode & keys[i]) != 0)
        *val = 1; 
      ans = 2;
      break;
    }

    // maybe try setting property to specific value 
    if (val != NULL)
    {
      if (*val > 0)
        mode |= keys[i];
      else
        mode &= ~(keys[i]);
      if (SUCCEEDED(adjust->SetMode(capture, mode)))
        ans = 2;
      break;
    }

    // otherwise try setting to the default value
    if (vdef[i] > 0)
      mode |= keys[i];
    else
      mode &= ~(keys[i]);
    if (SUCCEEDED(adjust->SetMode(capture, mode)))
      ans = 2;
    break;
  }

  // clean up 
  capture->Release();
  adjust->Release();
  return ans;
}


/////////////////////////////////////////////////////////////////////////////
//                           Core functionality                            //
/////////////////////////////////////////////////////////////////////////////

//= Read current frame as RGB into supplied array (random access mode).
// assumes array is big enough, writes BGR order, rows padded to next long word
// can adjust number of frames actually advanced
// can call multiple times with block = 0 until frame is acquired

int jhcDxVSrc::iGet (jhcImg& dest, int *advance, int src, int block)
{
//  long sz = dest.PxlSize();
  double g_time;
  int ms = ((block > 0) ? 1000 : 0);

  if (ok < 1)
    return 0;

  // make sure done only once (for non-blocking)
  if (request <= 0)
  {
    // set up to capture next frame
    Prefetch(1); 
    snap = p_time + (*advance - 0.5) * f_time;
    ResetEvent(grabbed);
    request = 1;
  }

  // wait for proper frame to arrive then find out when it occurred
  if (WaitForSingleObject(grabbed, ms) != WAIT_OBJECT_0)  
    return 0;
  g_time = s_time;
//  extract->GetCurrentBuffer(&sz, (long *)(dest.PxlDest()));  // glitches
  dest.CopyArr(buf);
  *advance = ROUND((g_time - p_time) / f_time);
  p_time = g_time;
  request = 0;
  return 1;
}


//= Starts reception and decoding of the video stream.
// frames can be grabbed with Get() quickly after this 
// cannot be stopped except by calling Close() method

void jhcDxVSrc::Prefetch (int doit)
{
  if ((ok < 1) || (doit <= 0) || ((doit > 0) && (run > 0)))
    return;
  run = -1;
  ResetEvent(grabbed);
  control->Run();
  WaitForSingleObject(grabbed, 3000);  
  p_time = s_time;
  run = 1;
  request = 0;
}


//= Stops the graph to allow adjustments to be made.

void jhcDxVSrc::stop_graph ()
{
  Prefetch(0);
//  control->Stop();
  control->StopWhenReady();
  run = 0;
}


/////////////////////////////////////////////////////////////////////////////
//                      Required base class methods                        //
/////////////////////////////////////////////////////////////////////////////

//= Callback that occurs when a new frame is ready.
// records time and sets completion flag

STDMETHODIMP jhcDxVSrc::BufferCB (double SampleTime, BYTE *pBuffer, long BufferLen)
{
  if ((run < 0) || (SampleTime >= snap))
  {
    s_time = SampleTime;
    buf.LoadAll(pBuffer);     // requires double copying, but glitch-free
    SetEvent(grabbed);
  }
  return S_OK;
}

		
//= Return an object that mediates a certain type of interaction.
// since this class only samples frames, only acknowledge this kind of request 

STDMETHODIMP jhcDxVSrc::QueryInterface (REFIID iid, void **ppvObject)
{
	if ((iid != IID_ISampleGrabberCB) && (iid != IID_IUnknown))
  	return E_NOINTERFACE;
	*ppvObject = this;
  return S_OK;
}

