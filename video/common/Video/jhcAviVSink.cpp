// jhcAviVSink.cpp : save images or screen to a video file
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2017 IBM Corporation
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
#include "Video/jhcVidReg.h"

#include "Video/jhcAviVSink.h"


///////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                       //
///////////////////////////////////////////////////////////////////////////

JREG_VSINK(jhcAviVSink, "avi");


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor closes file being written.

jhcAviVSink::~jhcAviVSink ()
{
  Close();
  AVIFileExit();
}


//= Basic constuctor sets up defaults and binds file name.

jhcAviVSink::jhcAviVSink (const char *fname)
{
  init_cfg(fname, 0);
}


//= Default constructor just sets up member variables.

jhcAviVSink::jhcAviVSink (int cstyle)
{
  init_cfg("foo.avi", cstyle);
}


//= Standard configuration at creation time.

void jhcAviVSink::init_cfg (const char *fname, int cstyle)
{
  AVIFileInit();
  InitHdr();
  InitAvi();

  // standard configuration
  SetSink(fname);
  SetSize(320, 240, 3);
  SetSpeed(30.0);
  Compress(cstyle);
}


//= Fill BITMAPINFOHEADER with linear color map.

void jhcAviVSink::InitHdr ()
{
  int i;
  BYTE v = 0;
  RGBQUAD *cols = (RGBQUAD *)(harr + sizeof(BITMAPINFOHEADER));

  for (i = 0; i < 256; i++)
  {
    cols->rgbRed   = v;
    cols->rgbGreen = v;
    cols->rgbBlue  = v;
    cols++;
    v++;
  }
  hdr = (LPBITMAPINFOHEADER) harr;
}


//= Clear specialized member variables.

void jhcAviVSink::InitAvi ()
{
  pfile = NULL;
  pavi = NULL;
  pcomp = NULL;

  scrn_dc = 0;
  copy_dc = 0;
  dest_bmap = 0;
  pixels = NULL;
  bsz = 0;
  cx = 0;
  cy = 0;
}


///////////////////////////////////////////////////////////////////////////
//                            Stream Configuration                       //
///////////////////////////////////////////////////////////////////////////

//= Tell whether stream should pop a dialog box to select compressor.
// style 1 asks the user to pick, higher numbers are specific codecs
// this must be done before the first write to the stream
// NOTE: Intel YUV works under WMP12, cstyle = 7

int jhcAviVSink::Compress (int cstyle, double q)
{
  if (bound == 1)
    return 0;
  ctag[0] = '\0';
  compress = cstyle;
  quality = q;
  return 1;
}


//= Set the compressor based on a 4 character designator.
// a NULL or empty string configures the system for no compression
// NOTE: Intel YUV works under WMP12, cname = "iyuv"

int jhcAviVSink::Compress (const char *cname, double q)
{ 
  if (bound == 1)
    return 0;
  compress = 0;
  if ((cname == NULL) || (*cname == '\0'))
    return 1;
  strncpy_s(ctag, cname, 4);     // always adds terminator
  compress = -1;
  quality = q;
  return 1;
}


//= Set up to save copies of the application window.
// can force application window to given size if wdes,hdes > 0
// can force an integral number of macroblocks for compressors

int jhcAviVSink::SetSizeWin (int wdes, int hdes, int wtrim, int htrim)
{
  jhcString tag("DISPLAY");
  HWND win;
  POINT pt = {1, 1};                  // beveled edges
  RECT rect, mid;
  int w0, h0, xpad, ypad, sbar = 19;  // typical value for StatusBar

  if (bound == 1)
    return 0;

  // possibly force application window size
  win = GetForegroundWindow();
  if ((wdes > 0) && (hdes > 0))
  {
    // adjust for borders, menu bar, status bar, etc.
    GetWindowRect(win, &rect);
    GetClientRect(win, &mid);
    xpad = (rect.right - rect.left) - (mid.right - mid.left) + 2;
    ypad = (rect.bottom - rect.top) - (mid.bottom - mid.top) + sbar;
    SetWindowPos(win, NULL, 0, 0, wdes + xpad, hdes + ypad, SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW);
  }

  // find application window and extract its position and dimensions
  GetClientRect(win, &rect);
  w  = (rect.right  - rect.left) - 2;
  h  = (rect.bottom - rect.top)  - sbar;
  d  = 3;
  ClientToScreen(win, &pt);
  cx = pt.x;
  cy = pt.y;

  // make w and h multiples of padding values and recenter copy region
  w0 = w;
  if (wtrim > 0)
    w = (w / wtrim) * wtrim;
  else if (wtrim < 0)
    w = ((w - wtrim - 1) / -wtrim) * -wtrim;
  h0 = h;
  if (htrim > 0)
    h = (h / htrim) * htrim;
  else if (htrim < 0)
    h = ((h - htrim - 1) / -htrim) * -htrim;
  cx += (w0 - w) / 2;
  cy += (h0 - h) / 2;

  // get display context for whole screen then make a copy and a new bitmap
  scrn_dc   = CreateDC(tag.Txt(), NULL, NULL, NULL); 
  copy_dc   = CreateCompatibleDC(scrn_dc); 
  dest_bmap = CreateCompatibleBitmap(scrn_dc, w, h);

  // make up an equivalent DIB buffer
  if (pixels != NULL)
    delete [] pixels;
  bsz = h * (((w * d + 3) >> 2) << 2); 
  pixels = new UC8 [bsz];
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Basic Operations                            //
///////////////////////////////////////////////////////////////////////////

//= Close all parts of the AVI interface.

void jhcAviVSink::iClose ()
{
  // standard AVI stuff
  if (pcomp != NULL)
    AVIStreamRelease(pcomp);
  if (pavi != NULL)
    AVIStreamRelease(pavi);
  if (pfile != NULL)
    AVIFileClose(pfile);

  // extra stuff for screen dumps
  if (pixels != NULL)
    delete [] pixels;
  if (dest_bmap != 0)
    DeleteObject(dest_bmap);
  if (copy_dc != 0)
    DeleteDC(copy_dc); 
  if (scrn_dc != 0)
    DeleteDC(scrn_dc); 

  // reset guts
  InitAvi();
}


//= Basic file opening call when all other parameters are bound.

int jhcAviVSink::iOpen ()
{
  jhcString fn(FileName);
  AVISTREAMINFO str;
  AVICOMPRESSOPTIONS opts;
  long sz;
  int rc;

  // open the AVI file for writing and bind file pointer
  // then create the raw stream with given image size
  if (AVIFileOpen(&pfile, fn.Txt(), OF_CREATE, NULL) != 0)
    return 0;
  StrFormat(str, w, h, freq);
  if (AVIFileCreateStream(pfile, &pavi, &str) != 0)
    return 0;
  avistr = pavi;

  // make up a compressed output stream  (possibly ask user for type)
  if (compress != 0)
  {
    if (PickCodec(opts, compress) < 1)
      return 0;
    rc = AVIMakeCompressedStream(&pcomp, pavi, &opts, NULL);
    if (rc != AVIERR_OK)
    {
      if (rc == AVIERR_NOCOMPRESSOR)
        Complain("jhcAviVSink::iOpen could not find compressor %s !", ctag);
      return 0;
    }
    avistr = pcomp;
  }

  // tell AVI stream the size of image to store
  sz = ImgFormat(hdr, w, h, d);
  if (AVIStreamSetFormat(avistr, 0, hdr, sz) != 0)
    return 0;
  return 1;
}


//= Fills in an avistreaminfo structure to reflect desired size.

void jhcAviVSink::StrFormat (AVISTREAMINFO& str, int wid, int ht, double fps)
{
  SetRect(&str.rcFrame, 0, 0, wid, ht);    
  str.szName[0]             = '\0'; 
  str.fccType               = streamtypeVIDEO;
  str.fccHandler            = (ctag[3] << 24) | (ctag[2] << 16) | (ctag[1] << 8) | ctag[0];  
  str.dwFlags               = 0;     
  str.dwCaps                = 0; 
  str.wPriority             = 0;     
  str.wLanguage             = 0;     
  str.dwScale               = 1000;     
  str.dwRate                = ROUND(1000.0 * fps);   
  str.dwStart               = 0;     
  str.dwLength              = 0;     
  str.dwInitialFrames       = 0; 
  str.dwSuggestedBufferSize = 0;     
  str.dwQuality             = (DWORD) -1;     
  str.dwSampleSize          = 0; 
  str.dwEditCount           = 0;     
  str.dwFormatChangeCount   = 0; 
}


//= Fills in a bitmapinfoheader structure to reflect desired size.

long jhcAviVSink::ImgFormat (LPBITMAPINFOHEADER hdr, int wid, int ht, int nf)
{
  DWORD sz = sizeof(BITMAPINFOHEADER);

  // fill in fields
  hdr->biSize          = sz;
  hdr->biWidth         = wid;    
  hdr->biHeight        = ht;    
  hdr->biPlanes        = 1; 
  hdr->biBitCount      = (WORD)(nf << 3);
  hdr->biCompression   = BI_RGB;    
  hdr->biSizeImage     = 0; 
  hdr->biXPelsPerMeter = 1000;    
  hdr->biYPelsPerMeter = 1000;    
  hdr->biClrUsed       = ((nf == 1) ? 256 : 0); 
  hdr->biClrImportant  = 0; 

  // return overall size of object with optional color tables
  if (nf == 1)
    sz += 256 * sizeof(RGBQUAD);
  return sz;
}


//= Dispatches on "compress" variable to select some type.
// style 1 asks user to choose codec via dialog box
// style -1 tries to use the already bound compressor tag string
// many compressors have restrictions on dimensions (e.g. multiple of 4)

int jhcAviVSink::PickCodec (AVICOMPRESSOPTIONS& opts, int style)
{
  AVICOMPRESSOPTIONS *popts = &opts, **aopts = &popts;
  char codecs[8][5] = {"DIB ",   // 0 = uncompressed
                       "DIB ",   //     (never used = menu)
                       "msvc",   // 2 = Microsoft  Video
                       "iv32",   // 3 = Indeo 3.2
                       "mp43",   // 4 = Microsoft MPEG-4 v3 (pre DivX)
                       "iv50",   // 5 = Indeo 5.1 (pretty good)
                       "cvid",   // 6 = Cinepack (slow but common)
                       "iyuv"};  // 7 = Intel YUV (supported in WMP12)

  // default values for all structure elements
  opts.fccType           = streamtypeVIDEO;     
  opts.fccHandler        = 0;
  opts.dwKeyFrameEvery   = 0; 
  opts.dwQuality         = (DWORD) -1;     
  opts.dwBytesPerSecond  = 0;
  opts.dwFlags           = AVICOMPRESSF_VALID; 
  opts.lpFormat          = NULL;     
  opts.cbFormat          = 0;     
  opts.lpParms           = NULL; 
  opts.cbParms           = 0; 
  opts.dwInterleaveEvery = 0;
	opts.dwQuality         = ROUND(10000.0 * quality);  // indeo 4.5 defaults to 8500

  // ask user to choose from dialog box list
  if (style == 1)
  {
    ctag[0] = '\0';
    opts.dwFlags = 0;
    if (!AVISaveOptions(NULL, 0, 1, &pavi, aopts))  
      return 0;

    // convert number to string
    ctag[0] = (char)( opts.fccHandler        & 0xFF);
    ctag[1] = (char)((opts.fccHandler >> 8)  & 0xFF);
    ctag[2] = (char)((opts.fccHandler >> 16) & 0xFF);
    ctag[3] = (char)( opts.fccHandler >> 24);
    ctag[4] = '\0';
    quality = opts.dwQuality / 10000.0;               // indeo 4.5 defaults to 8500
    return 1;
  }

  // select from a list of standard codecs (or leave user choice)
  // then convert compressor string into ID number 
  if ((style >= 0) && (style <= 7))
    strcpy_s(ctag, codecs[style]);
  else if ((style > 0) || (*ctag == '\0'))
    strcpy_s(ctag, codecs[0]);
  opts.fccHandler = (ctag[3] << 24) | (ctag[2] << 16) | (ctag[1] << 8) | ctag[0];
  return 1;
}


//= Save a new image in uncompressed format at end of file.
// will set size of output stream to match image if not set yet

int jhcAviVSink::iPut (const jhcImg& src)
{
  if (AVIStreamWrite(avistr, nextframe++, 1, 
                     (void *) src.PxlSrc(), src.PxlSize(),      
                     AVIIF_KEYFRAME, NULL, NULL)
      == 0)
    return 1;
  return 0;
}  


//= Dumps the whole current application window as a bitmap into the file.
// returns: 1 = successful, 0 = was good but just failed, -1 = bad stream
// Note: should generally call SetSizeWin before using this function

int jhcAviVSink::PutWin (int wtrim, int htrim)
{
  HBITMAP trash_bmap;

  // check that environment is set up properly
  if (bound == 0)
  {
    SetSizeWin(0, 0, wtrim, htrim);
    Open();
  }
  if (ok < 0)
    return -1;
  if ((scrn_dc == 0) || (copy_dc == 0) || (dest_bmap == 0) || 
      (pixels == NULL) || (bsz == 0))
    return 0;

  // swap in destination bitmap and copy to it
  trash_bmap = (HBITMAP) SelectObject(copy_dc, dest_bmap); 
  BitBlt(copy_dc, 0, 0, w, h, scrn_dc, cx, cy, SRCCOPY); 

  // swap out the filled bitmap and convert it to a DIB
  dest_bmap = (HBITMAP) SelectObject(copy_dc, trash_bmap); 
  GetDIBits(scrn_dc, dest_bmap, 0, h, pixels, (LPBITMAPINFO) hdr, DIB_RGB_COLORS);

  // do actual writing of pixels in extracted bitmap
  if (AVIStreamWrite(avistr, nextframe++, 1, pixels, bsz, 
                     AVIIF_KEYFRAME, NULL, NULL)
      == 0)
    return 1;
  ok = 0;                    // mark occasional failure on stream
  return 0;
}

