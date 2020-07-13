// jhcImgMS.cpp : still image codec using Windows operating system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2019 IBM Corporation
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

// Build.Settings.Link.General must have library WindowsCodecs.lib
#pragma comment(lib, "WindowsCodecs.lib")


#include "Data/jhcImgMS.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcImgMS::jhcImgMS ()
{
  // no components yet
  wic = NULL;
  dec = NULL;
  frame = NULL;
  file = NULL;
  clr_cache();

  // open COM and build factory 
  CoInitialize(NULL);
  CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&wic));
}


//= Default destructor does necessary cleanup.

jhcImgMS::~jhcImgMS ()
{
  clr_cache();
  if (wic != NULL)
    wic->Release();
  CoUninitialize(); 
}


//= Clear any old decoder and frame.

void jhcImgMS::clr_cache ()
{
  // get rid of components
  if (frame != NULL)
    frame->Release();
  if (dec != NULL)
    dec->Release();
  if (file != NULL)  
    CloseHandle(file);
  frame = NULL;
  dec = NULL;
  file = NULL;

  // no cached decoded image
  *cached = '\0';
  dok = 0;
}


/////////////////////////////////////////////////////////////////////////////
//                            Reading Images                               //
/////////////////////////////////////////////////////////////////////////////

//= Find out sizes of stored still image.
// returns -1 if bad type, 1 if OK, or 0 if format error 

int jhcImgMS::ReadAltHdr (int *w, int *h, int *f, const char *fname)
{
  WICPixelFormatGUID fmt;
  UINT w0, h0;

  // set up default values
  *w = 0;
  *h = 0;
  *f = 0;

  // give up unless file known to be in a readable format
  if (!IsFlavor("jpg") && !IsFlavor("jpeg") && 
      !IsFlavor("tif") && !IsFlavor("tiff") && !IsFlavor("gif") && !IsFlavor("png"))
    return -1;

  // open file and decode image
  if (Decode(fname) <= 0) 
    return 0;

  // check pixel format
  if (FAILED(frame->GetPixelFormat(&fmt)))
    return 0;
  if ((fmt == GUID_WICPixelFormat24bppBGR)    ||
      (fmt == GUID_WICPixelFormat24bppRGB)    ||   // swapped red and blue
      (fmt == GUID_WICPixelFormat8bppIndexed) ||   // 8 bit indexed
      (fmt == GUID_WICPixelFormat32bppBGRA))       // 32 bit with alpha
    *f = 3;
  else if (fmt == GUID_WICPixelFormat16bppGray)
    *f = 2;
  else if (fmt == GUID_WICPixelFormat8bppGray)
    *f = 1;
  else
    return 0;

  // get image dimensions
  if (frame->GetSize(&w0, &h0) != S_OK)
    return 0;
  *w = w0;
  *h = h0;
  return 1;
}


//= Read an image file and completely decode it.
// returns 1 if successful, 0 for failute

int jhcImgMS::Decode (const char *fname)
{
  WCHAR *wname = NULL;
  int len;

  // see if image already decoded (e.g. for size test)
  if ((dok > 0) && (strcmp(fname, cached) == 0))
    return 1;
  if (wic == NULL)
    return 0;
  clr_cache();

  // get wide version of file name and open it (must remain open)
  len = (int) strlen(fname) + 1;
  wname = new WCHAR[len];
	mbstowcs_s(NULL, wname, len, fname, len);
  file = CreateFile(wname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    
  // create decoder and get frame
  if (SUCCEEDED(wic->CreateDecoderFromFileHandle((ULONG_PTR) file, NULL, WICDecodeMetadataCacheOnDemand, &dec)))
    if (SUCCEEDED(dec->GetFrame(0, &frame)))
      dok = 1;

  // cleanup (close file immediately if failure)
  if (dok < 1)
    clr_cache();
  if (wname != NULL)
    delete [] wname;
  return dok;
}


//= Fill dest with image in requested format.
// assumes destination image is already in correct format
// returns -1 if bad type, 1 if OK, or 0 if format error 

int jhcImgMS::LoadAlt (jhcImg& dest, const char *fname)
{
  WICPixelFormatGUID fmt;
  int ans;

  // give up unless file known to be in a readable format
  if (!IsFlavor("jpg") && !IsFlavor("jpeg") && 
      !IsFlavor("tif") && !IsFlavor("tiff") && !IsFlavor("gif") && !IsFlavor("png"))
    return -1;
  if (!dest.Valid(1, 3))
    return 0;
  if (dok <= 0)
    return 0;

  // dispatch on color type
  if (FAILED(frame->GetPixelFormat(&fmt)))
    ans = 0;
  else if (fmt == GUID_WICPixelFormat24bppRGB)
    ans = color_rgb(dest);
  else if (fmt == GUID_WICPixelFormat32bppBGRA)
    ans = color_alpha(dest);
  else if (fmt == GUID_WICPixelFormat8bppIndexed) 
    ans = color_table(dest);
  else 
    ans = color_raw(dest);
  clr_cache();                         // release file
  return ans;
}


//= Simple pixel copy (8 bit gray or 24 bit BGR) with bottom-up conversion.

int jhcImgMS::color_raw (jhcImg& dest)
{
  BYTE *pels = NULL;
  int n, fill = 0;

  // allow early termination
  while (1)
  {
    // copy original pixels from already decoded frame
    // could flip in situ to avoid allocation (e.g. jhcResize::FlipV)
    n = dest.PxlSize();
    if ((pels = new BYTE [n]) == NULL)
      break;
    if (FAILED(frame->CopyPixels(NULL, dest.Line(), n, pels)))
      break;

    // convert to bottom-up image
    dest.LoadFlip(pels);
    fill = 1;
    break;
  }

  // cleanup
  delete [] pels;
  return fill;
}


//= Translate RGB to BGR color order with bottom-up conversion.

int jhcImgMS::color_rgb (jhcImg& dest)
{
  BYTE *pels = NULL;
  UC8 *s, *d = dest.PxlDest();
  int w = dest.XDim(), h = dest.YDim(), ln = dest.Line(), dsk = dest.Skip();
  int n, x, y, ssk = 3 * w + ln, fill = 0;

  // allow early termination
  while (1)
  {
    // copy original pixels from already decoded frame
    // could conceivably do this in situ to avoid allocation
    n = dest.PxlSize();
    if ((pels = new BYTE [n]) == NULL)
      break;
    if (FAILED(frame->CopyPixels(NULL, ln, n, pels)))
      break;

    // swap colors and convert to bottom-up
    s = (UC8 *) pels + (h - 1) * ln;
    for (y = h; y > 0; y--, d += dsk, s -= ssk)
      for (x = w; x > 0; x--, d += 3, s += 3)
      {
        d[0] = s[2];
        d[1] = s[1];
        d[2] = s[0];
      }

    // success
    fill = 1;
    break;
  }

  // cleanup
  delete [] pels;
  return fill;
}


//= Translation of 32 bit BGRA color and bottom-up conversion.

int jhcImgMS::color_alpha (jhcImg& dest)
{
  BYTE *pels = NULL;
  UC8 *s, *d = dest.PxlDest();
  int w = dest.XDim(), h = dest.YDim(), dsk = dest.Skip();
  int n, x, y, ln = 4 * w, ssk = 2 * ln, fill = 0;

  // allow early termination
  while (1)
  {
    // copy original pixels from already decoded frame
    // could conceivably do this in situ to avoid allocation
    n = h * ln;
    if ((pels = new BYTE [n]) == NULL)
      break;
    if (FAILED(frame->CopyPixels(NULL, ln, n, pels)))
      break;

    // copy BGR only and convert to bottom-up
    s = (UC8 *) pels + (h - 1) * ln;
    for (y = h; y > 0; y--, d += dsk, s -= ssk)
      for (x = w; x > 0; x--, d += 3, s += 4)
      {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
      }

    // success
    fill = 1;
    break;
  }

  // cleanup
  delete [] pels;
  return fill;
}


//= Translation of 8 bit color table and bottom-up conversion.

int jhcImgMS::color_table (jhcImg& dest)
{
  IWICPalette *pal = NULL;
  WICColor *cols = NULL;
  BYTE *pels = NULL;
  UC8 *s, *d = dest.PxlDest();
  UINT nc, nc2;
  UL32 argb;
  int w = dest.XDim(), h = dest.YDim(), dsk = dest.Skip();
  int n, x, y, ln = ((w + 3) >> 2) << 2, ssk = w + ln, fill = 0;

  // allow early termination
  while (1)
  {
    // get color table from already decoded frame
    if (FAILED(wic->CreatePalette(&pal)))
      break;
    if (FAILED(frame->CopyPalette(pal)))
      break;
    if ((FAILED(pal->GetColorCount(&nc))) || (nc <= 0))
      break;
    if ((cols = new WICColor [nc]) == NULL)
      break;
    if (FAILED(pal->GetColors(nc, cols, &nc2)))
      break;

    // copy original pixels from already decoded frame
    // could conceivably do this in situ to avoid allocation
    n = h * ln;
    if ((pels = new BYTE [n]) == NULL)
      break;
    if (FAILED(frame->CopyPixels(NULL, ln, n, pels)))
      break;

    // lookup color indices and convert to bottom-up
    s = (UC8 *) pels + (h - 1) * ln;
    for (y = h; y > 0; y--, d += dsk, s -= ssk)
      for (x = w; x > 0; x--, d += 3, s++)
      {
        // decode WICColor entry as Alpha:Red:Green:Blue
        argb = cols[*s];
        d[0] = (UC8)(argb & 0xFF);
        d[1] = (UC8)((argb >> 8) & 0xFF);
        d[2] = (UC8)((argb >> 16) & 0xFF);
      }

    // success
    fill = 1;
    break;
  }

  // cleanup
  delete [] pels;
  delete [] cols;
  pal->Release();
  return fill;
}


/////////////////////////////////////////////////////////////////////////////
//                             Writing Images                              //
/////////////////////////////////////////////////////////////////////////////

//= Save image out in requested format (JPEG, TIFF, GIF, PNG).
// returns -1 if bad type, 1 if OK, or 0 if format error 

int jhcImgMS::SaveAlt (const char *fname, const jhcImg& src)
{
  int nf = src.Fields();

  // give up unless file known to be in a readable format
  if (!IsFlavor("jpg") && !IsFlavor("jpeg") && 
      !IsFlavor("tif") && !IsFlavor("tiff") && !IsFlavor("gif") && !IsFlavor("png"))
    return -1;
  if ((nf < 1) || (nf > 3))
    return 0;
  return Encode(fname, src);
}


//= Save an image to a file in requested format.
// returns 1 if successful, 0 for failure
// NOTE: works for JPEG and TIFF, not tested for GIF and PNG

int jhcImgMS::Encode (const char *fname, const jhcImg& src)
{
  WCHAR *wname = NULL;
  GUID kind = GUID_ContainerFormatBmp;
  IWICStream *out = NULL;
  IWICBitmapEncoder *enc = NULL;
  IWICBitmapFrameEncode *img = NULL;
  IPropertyBag2 *props = NULL;
  UC8 *pels = NULL;
  PROPBAG2 opts = { 0 };
  VARIANT val;    
  WICPixelFormatGUID fmt = GUID_WICPixelFormat24bppBGR;
  int len, n, comp = 0, eok = 0;

  if (wic == NULL)
    return 0;

  // allow early exit
  while (1)
  {
    // get wide version of file name
    len = (int) strlen(fname) + 1;
    wname = new WCHAR[len];
	  mbstowcs_s(NULL, wname, len, fname, len);

    // create stream for given filename
    if (FAILED(wic->CreateStream(&out)))
      break;
    if (FAILED(out->InitializeFromFilename(wname, GENERIC_WRITE)))
      break;

    // determine codec type
    if (IsFlavor("jpg") || IsFlavor("jpeg"))
      kind = GUID_ContainerFormatJpeg;
    else if (IsFlavor("tif") || IsFlavor("tiff"))
      kind = GUID_ContainerFormatTiff;
    else if (IsFlavor("gif"))
      kind = GUID_ContainerFormatGif;
    else if (IsFlavor("png"))
      kind = GUID_ContainerFormatPng;

    // create encoder attached to stream and make new frame
    if (FAILED(wic->CreateEncoder(kind, NULL, &enc)))
      break;
    if (FAILED(enc->Initialize(out, WICBitmapEncoderNoCache)))
      break;
    if (FAILED(enc->CreateNewFrame(&img, &props)))
      break;

    // set compression parameters
    VariantInit(&val);
    if (IsFlavor("jpg") || IsFlavor("jpeg"))
    {
      // set compression quality based on member variable
      opts.pstrName = L"ImageQuality";
      val.vt = VT_R4;
      val.fltVal = (float)(0.01 * quality);
      comp = 1;
    }
    else if (IsFlavor("tif") || IsFlavor("tiff"))
    {
      // select lossless ZIP compression
      opts.pstrName = L"TiffCompressionMethod";
      val.vt = VT_UI1;
      val.bVal = WICTiffCompressionZIP;      
      comp = 1;
    }
    if (comp > 0)
      if (FAILED(props->Write(1, &opts, &val)))
        break;
    if (FAILED(img->Initialize(props)))
      break;

    // set pixel format (assume supported) and image size
    if (src.Fields() == 2)
      fmt = GUID_WICPixelFormat16bppGray;
    else if (src.Fields() == 1)
      fmt = GUID_WICPixelFormat8bppGray;
    if (FAILED(img->SetPixelFormat(&fmt)))
      break;
    if (FAILED(img->SetSize(src.XDim(), src.YDim())))
      break;

    // convert to top down image
    n = src.PxlSize();
    pels = new UC8 [n];
    src.DumpFlip(pels);

    // push pixels through encoder to file
    if (FAILED(img->WritePixels(src.YDim(), src.Line(), n, (BYTE *) pels)))
      break;
    if (FAILED(img->Commit()))
      break;
    if (FAILED(enc->Commit()))
      break;

    // success
    eok = 1;
    break;
  }

  // cleanup 
  if (props != NULL)
    props->Release();
  if (img != NULL)
    img->Release();
  if (enc != NULL)
    enc->Release();
  if (out != NULL)
    out->Release();
  if (pels != NULL)
    delete [] pels;
  if (wname != NULL)
    delete [] wname;
  return eok;
}
