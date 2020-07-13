// jhcLiveVSrc.cpp : wrappers around Microsoft AVI capture functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
// Based on code from Max McGuire
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

#include <stdio.h>
#include "Data/jhcBitMacros.h"
#include "Interface/jhcMessage.h"
#include "Interface/jhcString.h"

#include "Video/jhcLiveVSrc.h"


//////////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

JREG_CAM(jhcLiveVSrc, "vfw");

#endif


//////////////////////////////////////////////////////////////////////////////
//                     Basic creation and deletion                          //
//////////////////////////////////////////////////////////////////////////////

//= Destructor closes stream and free allocated buffers.

jhcLiveVSrc::~jhcLiveVSrc ()
{
  Close();
}


//= Default constructor.

jhcLiveVSrc::jhcLiveVSrc ()
{
  Init();
}

  
//= Create, bind to a named driver, and initialize basic parameters.
// @see jhcLiveVSrc#SetSource

jhcLiveVSrc::jhcLiveVSrc (const char *name)
{
  Init();
  SetSource(name);
}


//= Bind to a named driver (can append ".vfw" to end if desired).
// A NULL name (or just ".vfw") binds to the default driver (if any).
// Can also use a number (e.g. "0.vfw" and "1.vfw") for multiple grabbers 
//   or directory name and number (e.g. "MSVIDEO\\1.vfw") for a certain class.
// A plus sign at the end (e.g. "0.vfw+") allows user configuration of source.
// A base name of "*" pops a dialog box and lets user choose from a list.

int jhcLiveVSrc::SetSource (const char *spec)
{
  int n, wild = 0;

  // get rid of old stream if needed, remember new name
  if (capWin != NULL)
    Close();
  ParseName(spec);

  // create an invisible capture preview window 
  // embed a pointer to this jhcLiveVSrc object in the capture window
  // (needed because callback accesses vars of instance, not class)
  capWin = capCreateCaptureWindow(NULL, 0, 0, 0, 0, 0, NULL, 0); 
  if (capWin == NULL)
    return -1;
  capPreview(capWin, FALSE);
  capSetUserData(capWin, (LONG)(this));
  StreamConfig();

  // attempt to bind framegrabber driver, clean up if fails
  // framegrabber might return RGB or compressed image at this point
  if (*FileName == '*')
  {
    wild = 1;
    SelectDriver();
  }
  else
    BindDriver();
  if (ok <= 0)
  {
    Disconnect();
    return -2;
  }

  // possibly let user select imaging parameters (if last char is "+")
  // make sure decompressor is available and started if needed
  n = (int) strlen(Ext) - 1;
  if (Ext[n] == '+') 
  {
    SelectParams(wild);
    Ext[n] = '\0';
  }
  else
    ok = RecordFormat(0);
  SetAspect();
  StreamConfig();   // again to make sure set for 30 fps
  
  // create a auto-resetting synchronization event 
  // bind capture callback functions for one-shot and streaming cases
  capDone = CreateEvent(NULL, FALSE, FALSE, TEXT("CaptureFinished"));
  capSetCallbackOnFrame(capWin, NewFrame);  
  capSetCallbackOnVideoStream(capWin, FrameReady);  
  return ok;
}


//= Completely reinitialize stream.

void jhcLiveVSrc::Close ()
{
  Prefetch(0);
  ReleaseCodec();
  Disconnect();
  if (big != NULL)
    delete [] big;
  if (rawa != NULL)
    delete [] rawa;
  if (rawb != NULL)
    delete [] rawb;
  Init();
}


//= Turn off decompressor if one running, reset member variable.

void jhcLiveVSrc::ReleaseCodec ()
{
  if (codec != NULL)
  {
    ICDecompressEnd(codec);
    ICClose(codec);
    codec = NULL;
  }
}


//= Disconnects from the capture device driver.
// failure to do so will require Windows to be restarted!
// also closes the capture event handle (if any)

void jhcLiveVSrc::Disconnect ()
{
  if (capWin != NULL)
  {
    capSetCallbackOnFrame(capWin, NULL);  
    capSetCallbackOnVideoStream(capWin, NULL);  
    if (capDone != NULL)
      CloseHandle(capDone);
    capDriverDisconnect(capWin);
    DestroyWindow(capWin);
    capWin = NULL;
  }
}


//= Set values of standard parameters.

void jhcLiveVSrc::Init ()
{
  int i;
  double sc = 255.0 / (3.0 * 31.0);

  strcpy(kind, "jhcLiveVSrc");

  for (i = 0; i < 94; i++)
    avg5[i] = (UC8) ROUND(sc * i);

  ok = 0;
  freq = 29.97;
  ParseName("");
  strcpy(Flavor, "vfw");

  capWin  = NULL;
  codec   = NULL;
  native  = (LPBITMAPINFOHEADER)(&nat);
  expand  = (LPBITMAPINFOHEADER)(&exp);
  capDone = NULL;
  
  bigsize = 0;
  rawsize = 0;
  big  = NULL;
  rawa = NULL;
  rawb = NULL;

  img  = NULL;
  grab = NULL;
  locka = 0;
  lockb = 0;
  locki = 0;
  tgrab = 0;
  streaming = 0;
  ready = 0;
}


//= Set streaming mode for 30 fps, allow yielding to foreground thread.
// might want to set time longer depending on DispRate and Increment
// (but streaming capture is usually used to get maximum frame rate)

void jhcLiveVSrc::StreamConfig ()
{
  CAPTUREPARMS cp;

  cp.dwRequestMicroSecPerFrame = 33333;      // 29.97 fps = 33367
  cp.fMakeUserHitOKToCapture   = FALSE;
  cp.wPercentDropForError      = 100;        // was 10
  cp.fYield                    = TRUE;       // important 
  cp.dwIndexSize               = 0;
  cp.wChunkGranularity         = 0;
  cp.fUsingDOSMemory           = FALSE;
  cp.wNumVideoRequested        = 1; 
  cp.fCaptureAudio             = FALSE;      // may want to change this
  cp.wNumAudioRequested        = 0;
  cp.vKeyAbort                 = VK_ESCAPE;  // emergency bail-out key
  cp.fAbortLeftMouse           = FALSE;
  cp.fAbortRightMouse          = FALSE;
  cp.fLimitEnabled             = FALSE;
  cp.wTimeLimit                = 0; 
  cp.fMCIControl               = FALSE;      // VCR controls not used
  cp.fStepMCIDevice            = FALSE;
  cp.dwMCIStartTime            = 0;
  cp.dwMCIStopTime             = 0;
  cp.fStepCaptureAt2x          = FALSE;
  cp.wStepCaptureAverageFrames = 1;
  cp.dwAudioBufferSize         = 0;
  cp.fDisableWriteCache        = FALSE;
  cp.AVStreamMaster            = AVSTREAMMASTER_NONE;

  capCaptureSetSetup(capWin, &cp, sizeof(CAPTUREPARMS));
}

 
/////////////////////////////////////////////////////////////////////////////
//                VFW driver selection and configuration                   //
/////////////////////////////////////////////////////////////////////////////

//= Attempt to bind selected driver -- no guarantee that video stream is valid.
// member variable ok = 1 means success, 0 means failure
// this non-MFC method uses a dialog box to cycle through choices
//    can also use with jhcPickDev to get a nicer combo-box

void jhcLiveVSrc::SelectDriver ()
{
  int i, n, s = 0;
  jhcString name;
  char msg[1000];

  Prefetch(0);
  if (capWin == NULL)
    return;

  // find out how many drivers there are
  for (n = 0; n < 10; n++)
    if (!capGetDriverDescription(n, name.Txt(), 0, NULL, 0))
      break;
  if (n == 0)
    return;
 
  // pop multiple dialog boxes with moving caret "==>"
  while (1)
  {
    strcpy(msg, "Select this driver?\n\n");
    for (i = 0; i < n; i++)
    {
      if (i == s)
        strcat(msg, "==>");
      strcat(msg, "\t");
      capGetDriverDescription(i, name.Txt(), 80, NULL, 0);
      name.Sync();
      strcat(msg, name.ch);
      strcat(msg, "\n");
    }
    if (Ask(msg))
      break;
    s++;
    if (s >= n)
      return;
  }

/*
  // alternative combo-box version
  jhcPickDev dlg;
  if (dlg.DoModal() != IDOK)
    return;
  s = dlg.sel;
*/

  // try connecting to driver
  ok = 0;
  if (capDriverConnect(capWin, s) == 0)
  {
    capGetDriverDescription(s, name.Txt(), 80, NULL, 0);
    name.Sync();
    if (noisy > 0)
      Complain("Could not connect VFW driver: %s", name.ch);
    return;
  }
  sprintf(name.ch, "%d%s", s, Ext);
  ParseName(name.ch);
  ok = 1;
}


//= Provides a textual list of possible drivers.
// returns number of entries made

int jhcLiveVSrc::ListDrivers (char dest[10][80])
{
  jhcString name;
  int i;

  for (i = 0; i < 10; i++)
  {
    if (!capGetDriverDescription(i, name.Txt(), 80, NULL, 0))
      break;
    name.Sync();
    strcpy(dest[i], name.ch);
  }
  return i;
}


//= Find correct copy of correct capture driver based on parsed "file" name.
// attempt to bind specified driver -- no guarantee that video stream is valid
// member variable ok = 1 means success, 0 means failure

void jhcLiveVSrc::BindDriver ()
{
  jhcString driver("non-existent");
  char *target = NULL;
  int i, n, tlen = 0, pass = 0, bound = 0;

  if (capWin == NULL) 
    return;
  
  // do more parsing to figure out class of driver and instance number
  ok = 0;
  if (*JustDir != '\0')
  {
    target = JustDir;
    tlen = (int) strlen(target) - 1;  // strip off final separator
  }
  if (isdigit(*BaseName) != 0)
    pass = atoi(BaseName);
  else if (*BaseName != '\0')         // a base name beats a dir name
  {
    target = BaseName;
    tlen = (int) strlen(target);
  }

  // find driver number for given device name then bind to a window
  n = pass;
  for (i = 0; i < 10; i++)
  {
    if (!capGetDriverDescription(i, driver.Txt(), 200, NULL, 0))
      break;
    driver.Sync();
    if ((tlen == 0) || (_strnicmp(driver.ch, target, tlen) == 0))
      if (n-- <= 0)
        if ((bound = (int) capDriverConnect(capWin, i)) != 0)
          break;
  }

  // if failure report reason
  if (bound == 0)
  {
    if (i == 0)
      Complain("No available VFW capture devices");
    else
      Complain("Could not bind camera driver %d of: %s\nIt may be necessary to reboot.", 
                pass + 1, driver);
    return;
  }
  ok = 1;
}


//= Let user choose input connector and look at available sizes.
// always ask for connector choice and possibly ask for size
// member variable "ok" reflects success (1) or failure (0)

void jhcLiveVSrc::SelectParams (int format)
{
  CAPDRIVERCAPS params; 
  
  Prefetch(0);
  if (capWin == NULL)
    return;

  capDriverGetCaps(capWin, &params, sizeof(CAPDRIVERCAPS)); 
  if (params.fHasDlgVideoSource)    
    capDlgVideoSource(capWin);  
  if ((format != 0) && params.fHasDlgVideoFormat)
    capDlgVideoFormat(capWin);     
  ok = RecordFormat(0);
}


/////////////////////////////////////////////////////////////////////////////
//                   Video formatting and compression                      //
/////////////////////////////////////////////////////////////////////////////

//= If RGB or can be decompressed to RGB, then record output format.
// if decompressor unavailable returns zero, else returns one

int jhcLiveVSrc::RecordFormat (int bw)
{
  ReleaseCodec();
  ReadFormat(native);
  if (native->biCompression == BI_RGB)
    RecordSizes(native, bw);  
  else if (FindCodec() > 0)
    RecordSizes(expand, bw);  
  else
    return 0;
  ResizeBuffers();
  return 1;
}


//= Determine current framegrabber raw grab format and copy to "actual".
// makes up a properly sized temporary bitmapinfo object to receive data

void jhcLiveVSrc::ReadFormat (LPBITMAPINFOHEADER actual)
{
  LPBITMAPINFO fgspec;
  long nsize;

  nsize = capGetVideoFormatSize(capWin);
  fgspec = (LPBITMAPINFO) new char[nsize];
  capGetVideoFormat(capWin, fgspec, nsize);
  CopyFormat(actual, &(fgspec->bmiHeader));
  delete [] fgspec;
}


//= Transfer fields from one bitmap info header to another.

void jhcLiveVSrc::CopyFormat (LPBITMAPINFOHEADER dest, LPBITMAPINFOHEADER src)
{
  dest->biSize          = src->biSize; 
  dest->biWidth         = src->biWidth; 
  dest->biHeight        = src->biHeight; 
  dest->biPlanes        = src->biPlanes;
  dest->biBitCount      = src->biBitCount;
  dest->biCompression   = src->biCompression;
  dest->biSizeImage     = src->biSizeImage;     
  dest->biXPelsPerMeter = src->biXPelsPerMeter; 
  dest->biYPelsPerMeter = src->biYPelsPerMeter; 
  dest->biClrUsed       = src->biClrUsed;
  dest->biClrImportant  = src->biClrImportant;
}


//= Extract sizes from image header to use as overall stream return size.
// "d" is how many fields in output (certain monochrome conversions are OK)

void jhcLiveVSrc::RecordSizes (LPBITMAPINFOHEADER hdr, int bw)
{
  b = hdr->biBitCount;
  w = hdr->biWidth;
  h = hdr->biHeight;
  if ((b == 8) || 
      ((bw > 0) && ((b == 24) || (b == 32) || (b == 16))))
    d = 1;
  else
    d = 3;
  mskip = (((w + 3) >> 2) << 2) - w;
  cskip = (((3 * w + 3) >> 2) << 2) - (3 * w);
}


//= Change size of buffers to match raw output and decompressed output.
// Implemented as array not images because compressed formats unknown.

void jhcLiveVSrc::ResizeBuffers ()
{
  UL32 rsz = 0, bsz = 0;

  // always make new raw buffers if size has changed
  rsz = native->biSizeImage;
  if ((rsz > 0) && (rsz != rawsize))
  {
    if (rawa != NULL)
      delete [] rawa;
    if (rawb != NULL)
      delete [] rawb;
    rawa = new UC8[rsz]; 
    rawb = new UC8[rsz]; 
    rawsize = rsz;
  }

  // only make new big buffer if compressor is in use
  if (codec != NULL)
    bsz = expand->biSizeImage; 
  if ((bsz > 0) && (bsz != bigsize))
  {
    if (big != NULL)
      delete [] big;
    big = new UC8[bsz];
    bigsize = bsz;
  }
}


//= Find new decompressor that translates from current native format.
// records new expanded format and starts up decompressor
// Note: ICDecompressOpen seems to ignore attributes like bits per pixel 

int jhcLiveVSrc::FindCodec ()
{
  int bpp;

  // get default decompression format
  ReleaseCodec();
  codec = ICDecompressOpen(ICTYPE_VIDEO, NULL, native, NULL);
  if (codec == NULL)
    return 0;

  // check that it is RGB with 8, 16, or 24 bits per pixel
  ICDecompressGetFormat(codec, native, expand);
  bpp = expand->biBitCount;
  if ((expand->biCompression == BI_RGB) &&
      ((bpp == 32) || (bpp == 24) || (bpp == 16) || (bpp == 8)))
    if (ICDecompressBegin(codec, native, expand) == ICERR_OK)
      return 1;
  ReleaseCodec();
  return 0;
}


/////////////////////////////////////////////////////////////////////////////
//                         Image size alteration                           //
/////////////////////////////////////////////////////////////////////////////

//= See if capture device can automatically subsample.
// try a bunch of standard formats until one works 
// works with both compressed and uncompressed formats
// if all parameters are zero, keeps current format
// member variable "ok" reflects success (1) or failure (0)
// NOTE: will NOT switch basic mode (e.g. IYUV to RGB24 or Gray8)
// NOTE: to force a LARGER size, use a big non-zero dimension
// NOTE: to force back to RGB, set bw to a negative value

void jhcLiveVSrc::SetSize (int xmax, int ymax, int bw)
{
  BITMAPINFOHEADER targ;
  LPBITMAPINFOHEADER target = &targ;

  Prefetch(0);

  // make sure source exist and configuration needs changing
  if (capWin == NULL)
    return;
  if ((xmax == 0) && (ymax == 0) && (bw == 0))
    return;
  if ((xmax == w) && (ymax == h) && 
      (((bw <= 0) && (d == 3)) || ((bw > 0) && (d == 1))))
    return;

  // try some new default formats, keep original format if all fail
  ReadFormat(native);
  if (ScanFormats(target, xmax, ymax, bw) > 0)   
    CopyFormat(native, target);  

  // send new format to framegrabber, set up decompressor if needed
  ok = 0;
  if (capSetVideoFormat(capWin, native, sizeof(BITMAPINFOHEADER)) == NULL)
    return;
  ok = RecordFormat(bw);
}


//= Try a variety of uncompressed and compressed formats.
// returns 0 if all formats fail, else target gets best native format

int jhcLiveVSrc::ScanFormats (LPBITMAPINFOHEADER target, int xmax, int ymax, int bw)
{
  int i, p = ((bw > 0) ? 0 : 1);
  int search[2][4] = {{8, 24 /*16*/, 24, 32}, {24, 32, 16, 8}};

  // if framegrabber already set to compressed, then use only that format
  // sometimes driver lies about whether uncompressed formats are accepted
  CopyFormat(target, native);
  if ((target->biCompression != BI_RGB) || (bw == 0))
  {
    if (ScanSizes(target, xmax, ymax) > 0)     
      return 1;
    return 0;
  }

  // look for direct RGB grab of 24 or 16 bits (8 bits if mono requested)
  target->biCompression = BI_RGB;
  for (i = 0; i < 4; i++)
  {
    target->biBitCount = search[p][i];
    if (ScanSizes(target, xmax, ymax) >= 0)
      return 1;
  }
  return 0;
}


//= Modify given image description to try a bunch of standard sizes.
// modifies "target" to be best format, return 0 is nothing works
// list has tricky ordering to ensure reasonable defaults

int jhcLiveVSrc::ScanSizes (LPBITMAPINFOHEADER target, int xmax, int ymax)
{
  BITMAPINFOHEADER orig;
  int i, wbest = 0, hbest = 0, match = 0;
  static int sz[31][2] = {{640, 480}, {704, 480}, {720, 480},   // NTSC
                          {320, 240}, {352, 240}, {360, 240}, 
                          {240, 180}, {264, 180}, {270, 180}, 
                          {240, 176}, {264, 176}, {270, 176}, 
                          {160, 120}, {176, 120}, {180, 120}, 
                          {512, 480}, {512, 512},               // Legacy
                          {256, 240}, {256, 256}, 
                          {192, 176}, {192, 192}, 
                          {128, 120}, {128, 128},
                          {704, 576}, {720, 576},               // PAL
                          {352, 288}, {360, 288}, 
                          {264, 216}, {270, 216}, 
                          {176, 144}, {180, 144}}; 

  // record original size (sometimes buffer size computation wrong)
  ReadFormat(&orig);

  // try sizes from big to small (square pixels preferred so first)
  for (i = 0; i < 31; i++)
  {
    // work down until under limit (if any)
    if ((match > 0) && ((sz[i][0] > wbest) || (sz[i][1] > hbest)))
      continue;

    // see if the capture driver will take the new format
    SetDims(target, sz[i][0], sz[i][1]);
    if (TestFormat(target) == 1)
    {
      wbest = sz[i][0];
      hbest = sz[i][1];
      match = 1;
      if (((xmax <= 0) || (wbest <= xmax)) &&
          ((ymax <= 0) || (hbest <= ymax)))
        break;
    }
  }

  // record biggest acceptable size in "target"
  // return -1 if no good format, 0 if same as original
  if (match == 0) 
    return -1;
  SetDims(target, wbest, hbest);
  if ((wbest == orig.biWidth) && (hbest == orig.biHeight) &&
      (target->biBitCount == orig.biBitCount))
    return 0;
  return 1;
}


//= Force format to new dimensions, recalculate and return buffer size.

int jhcLiveVSrc::SetDims (LPBITMAPINFOHEADER hdr, int x, int y)
{
  int n;

  if (x != 0)
    hdr->biWidth = x;
  if (y != 0)
    hdr->biHeight = y;
  n = (((hdr->biBitCount * hdr->biWidth + 31) / 32) << 2) * hdr->biHeight;
  hdr->biSizeImage = n;
  return n;
}


//= See if framegrabber can output requested format.
// for uncompressed formats, talk directly to framegrabber
// otherwise try resizing native format and look for a decompressor

int jhcLiveVSrc::TestFormat (LPBITMAPINFOHEADER target)
{
  BITMAPINFOHEADER act;
  LPBITMAPINFOHEADER actual = &act;
  CAPSTATUS stat;

  // tests if format is really installed on framegrabber
  if (WriteFormat(target) == 0)
    return 0;

  // double checks sizes to see if capSetVideoFormat is lying
  ReadFormat(actual);
  if ((actual->biWidth  != target->biWidth) ||
      (actual->biHeight != target->biHeight))
    return 0;
  if (target->biCompression == BI_RGB)
    if ((actual->biCompression != BI_RGB) || 
        (actual->biBitCount    != target->biBitCount))
    return 0;
  if (target->biCompression == BI_RGB)
    if (actual->biSizeImage != target->biSizeImage)
      return 0;

  // triple checks to see if ReadFormat is lying
  if (!capGetStatus(capWin, &stat, sizeof(CAPSTATUS)))
    return 0;
  if (((int) stat.uiImageWidth  != target->biWidth) ||
      ((int) stat.uiImageHeight != target->biHeight))
    return 0;

  // if native format is compressed, look for a decompressor
  // do not remember or start it up, just look for its existence
  // accept whatever output format is the default (specs often ignored)
  if (target->biCompression != BI_RGB)
  	if (ICDecompressOpen(ICTYPE_VIDEO, NULL, target, NULL) == NULL)
      return 0;
  return 1;
}


//= Fussier setting of capture parameters.

int jhcLiveVSrc::WriteFormat (LPBITMAPINFOHEADER target)
{
  LPBITMAPINFO fgspec;
  long nsize;
  int ans = 1;

  nsize = capGetVideoFormatSize(capWin);
  fgspec = (LPBITMAPINFO) new char[nsize];
  CopyFormat(&(fgspec->bmiHeader), target);  
  if (capSetVideoFormat(capWin, fgspec, nsize) != TRUE)
    ans = 0;
  delete [] fgspec;
  return ans;
}


/////////////////////////////////////////////////////////////////////////////
//                            Stream Configuration                         //
/////////////////////////////////////////////////////////////////////////////

//= Change interval between grabbed frames.

void jhcLiveVSrc::SetStep (int offset, int key)
{
  Increment = offset;
  Prefetch(0);
}


//= Specifies whether to do background streaming capture or one-shot grab.
// for streaming capture always allow yield to user programs
// about 2x faster in streaming mode (prefetch = 1)

void jhcLiveVSrc::Prefetch (int doit)
{
  HWND appWin;

  if ((doit == 0) && (streaming != 0))
  {
    // stop streaming, invalidate last frame
    capCaptureStop(capWin);
    streaming = 0;
    grab = NULL;
    ready = 0;
    return;
  }

  if ((doit != 0) && (streaming == 0))
  {
    // mark streaming mode started but no frame yet, init ping-pong buffers
    ready = 0;
    grab = NULL;
    streaming = 1;
    appWin = GetActiveWindow();
    capCaptureSequenceNoFile(capWin);

    // restore display capability to main window after spawning background thread
    // this must be here and the time delay must be long enough
    Sleep(1000);
    SetActiveWindow(appWin);
    Percolate();
  }
}


/////////////////////////////////////////////////////////////////////////////
//                          Callback functions                             //
/////////////////////////////////////////////////////////////////////////////

//= Grab callback function, uses embedded pointer to access instance data.
// Binds "dest" to most recent buffer written and sets finished flag.
// Always records time of capture.

LRESULT CALLBACK jhcLiveVSrc::NewFrame (HWND cwin, LPVIDEOHDR vhd)
{
  jhcLiveVSrc *me = (jhcLiveVSrc *) capGetUserData(cwin);

  me->tgrab = timeGetTime();
  me->SaveFrameData(vhd->lpData);
  SetEvent(me->capDone);
  return (LRESULT) TRUE;  
}


//= Stream callback function, uses embedded pointer to access instance data.
// Binds "dest" to most recent buffer written and sets finished flag.
// Can selectively ignore certain frames based on Increment value.
  
LRESULT CALLBACK jhcLiveVSrc::FrameReady (HWND cwin, LPVIDEOHDR vhd)
{
  jhcLiveVSrc *me = (jhcLiveVSrc *) capGetUserData(cwin);

  me->SaveFrameData(vhd->lpData);
//  SetEvent(me->capDone);
  me->ready = 1;

  return (LRESULT) TRUE;  
}


//= Saves raw frame data (possibly still compressed) to some buffer.
// Selects and locks buffer (dest), then copies data and unlocks.
// Rawa and rawb always get full data, "img" may get only ROI region.
// Decompression and color conversion deferred to "iGet" function.

void jhcLiveVSrc::SaveFrameData (UC8 *src)
{
  grab = PickBuffer(grab);
  if (grab == NULL)
    return;

  if ((codec == NULL) && (d == 1) && ((b == 24) || (b == 32)))
  {
    if (b == 24)
      CopyGreen(grab, src);
    else
      CopyGreen32(grab, src);
  }
  else
    CopyAll(grab, src);

  UnlockBuffer(grab);
}


//= Select buffer to copy raw framegrabber data to.
// Lock selection to ensure data consistency.

UC8 *jhcLiveVSrc::PickBuffer (UC8 *last)
{
  // If raw image is uncompressed and either RGB, mono, or RGB to be 
  //   converted to mono then use destination image (img) directly.
  if ((img != NULL) && (codec == NULL) &&
      (( (b == 24)               && (d == 3)) || 
       (((b == 24) || (b == 32)) && (d == 1))))
    if (InterlockedExchange(&locki, 1) == 0)
      return img->PxlDest();

  // Otherwise alternate between bufa and bufb. 
  if (last == rawa)
    if (InterlockedExchange(&lockb, 1) == 0)
      return rawb;
  if (InterlockedExchange(&locka, 1) == 0)
    return rawa;
  if (InterlockedExchange(&lockb, 1) == 0)
    return rawb;

  // should never get here
  Complain("Could not lock a buffer for framegrabbing");
  return NULL;
}


//= Unlock whatever buffer was just used.

void jhcLiveVSrc::UnlockBuffer (UC8 *last)
{
  if ((img != NULL) && (last == img->PxlSrc()))
    locki = 0;
  else if (last == rawa)
    locka = 0;
  else if (last == rawb)
    lockb = 0;
}


//= Copy all fields from buffer. Use ROI of destination image if selected.

void jhcLiveVSrc::CopyAll (UC8 *dest, UC8 *src)
{
  if ((img != NULL) && (dest == img->PxlSrc()))
    img->CopyArr(src);
  else
    memcpy(dest, src, rawsize);
}


//= Copy second field from buffer. Use ROI of destination image if selected.
// version for 24 bit color R:G:B

void jhcLiveVSrc::CopyGreen (UC8 *dest, UC8 *src)
{
  int x, y;
  UC8 *d = dest, *s = src + 1;

  if ((img != NULL) && (dest == img->PxlSrc()))
  {
    img->CopyField(src, 1, 3, 0);
    return;
  }
  
  for (y = h; y > 0; y--)
  {
    for (x = w; x > 0; x--)
    {
      *d++ = *s;
      s += 3;
    }
    d += mskip;
    s += cskip;
  }
}


//= Copy second field from buffer. Use ROI of destination image if selected.
// version for 32 bit color = 0:R:G:B

void jhcLiveVSrc::CopyGreen32 (UC8 *dest, UC8 *src)
{
  int x, y;
  UC8 *d = dest, *s = src + 1;

  if ((img != NULL) && (dest == img->PxlSrc()))
  {
    img->CopyField(src, 1, 4, 0);
    return;
  }
  
  for (y = h; y > 0; y--)
  {
    for (x = w; x > 0; x--)
    {
      *d++ = *s;
      s += 4;
    }
    d += mskip;
    s += cskip;
  }
}


/////////////////////////////////////////////////////////////////////////////
//                            Image retrieval                              //
/////////////////////////////////////////////////////////////////////////////

//= Load up specified image (actually done by some callback function).
// different depending on whether prefetch (streaming) is used

int jhcLiveVSrc::iGet (jhcImg& dest, int *advance, int src)
{
  if (streaming != 0)
    return StreamGet(dest, advance);
  return GrabGet(dest, advance);
}


//= Load up specified image (mostly done by callback function really).
// allows callback function to run once, or times out after 1 sec
// if processing does not take too long, will respect Increment value

int jhcLiveVSrc::GrabGet (jhcImg& dest, int *advance)
{
  DWORD ans, tnext, tnow, tclose, tdiff = tgrab;

  // make sure enough time has elapsed since last grab
  // always leave at least one frame time plus fraction of another
  if (Increment < 0)
    Increment = -Increment;
  if (tgrab != 0)
  {
    tnext = tgrab + (DWORD)(1000.0 * Increment / freq);
    tclose = (DWORD)(2.0 * 1000.0 / freq + 0.5);
    tnow = timeGetTime();
    if ((tnext > tnow) && ((tnext - tnow) > tclose))
      Sleep(tnext - tnow - tclose);
  }

  // initiate capture (allow callback to run) and wait for completion
  // signal automatically clears immediately after it is tested as true
  img = &dest;
  grab = NULL;
  capGrabFrameNoStop(capWin);
  ans = WaitForSingleObject(capDone, 1000);  
  if ((ans == WAIT_FAILED) || (ans == WAIT_TIMEOUT))
  {
    Complain("Camera grab timeout");
    ok = -1;
    return -1;
  }

  // copy, decompress, and color convert data as necessary
  // figure how many frames actually went by (tdiff = previous tgrab)
  img = NULL;
  ExtractPixels(dest, grab);
  if ((tdiff != 0) && (jumped == 0))
  {
    tdiff = tgrab - tdiff;
    *advance = ROUND(freq * tdiff / 1000.0);
  }
  return 1;
}


//= Gets next frame (usually) from video stream.
// If next streaming frame has not arrived yet, writes straight to 
// output image. Otherwise, copies most recent buffer to output 
// (which may be up to one frame time stale).

int jhcLiveVSrc::StreamGet (jhcImg& dest, int *advance)
{
//  DWORD ans;
  DWORD tdiff;
  UC8 *where;
  
  // bind image as a possible destination buffer for pixel data
  // then wait for background process to say a frame is ready
  // signal automatically clears immediately after it is tested as true
  img = &dest;

/*
  // this does not work at all
  ans = WaitForSingleObject(capDone, 10000);  
  if ((ans == WAIT_FAILED) || (ans == WAIT_TIMEOUT))
  {
    Complain("Camera stream timeout");
    ok = -1;
    return -1;
  }
*/

/*
  // one of the cleaner versions
  // this works too -- Percolate in loop seems to be key -- but 15fps
  int i;
  for (i = 0; i < 1000; i++)
  {
    Percolate();
    if (InterlockedExchange(&ready, 0) != 0)
      break;
    Sleep(1);
  }
*/

/*
  // works fairly well, but also 15 fps, some problem with restart?
  int i;
  DWORD fnum, begin;
  CAPSTATUS stat;

  capGetStatus(capWin, &stat, sizeof(CAPSTATUS));
  begin = stat.dwCurrentVideoFrame;
  for (i = 0; i < 1000; i++)
  {
    Percolate();
    capGetStatus(capWin, &stat, sizeof(CAPSTATUS));
    fnum = stat.dwCurrentVideoFrame;
    if (fnum != begin)
      break;
    Sleep(1);
  }
//  *advance = fnum - begin;
//  *advance = fnum - previous;
*/


  DWORD tnext, tnow, tclose = 4;  // was 8 then 2

  // seems to give true 30 fps instead of 15 fps with other methods
  tdiff = tgrab;
  if (Increment < 0)
    Increment = -Increment;
  if (tgrab != 0)
  {
    tnext = tgrab + (DWORD)(1000.0 * Increment / freq);
    tnow = timeGetTime();
    if ((tnext > tnow) && ((tnext - tnow) > tclose))
      Sleep(tnext - tnow - tclose);
  }

  // claim output image to prevent any more potential grabs into it
  while (InterlockedExchange(&locki, 1) != 0);
  img = NULL;
  locki = 0;

  // see where background process most recently wrote a frame
  // loops in case a buffer swap happens exactly when checking
  while (1)
  {
    where = grab;
    if (where == dest.PxlSrc())
      break;
    if (where == rawa)
      if (InterlockedExchange(&locka, 1) == 0)
        break;
    if (where == rawb)
      if (InterlockedExchange(&lockb, 1) == 0)
        break;
    Percolate();  // as suggested by Vit Libal
  }

/*
Pause("video from buffer %c", 
  ((where == rawa) ? 'A' : 
  ((where == rawb) ? 'B' : 
  ((where == dest.Pixels) ? 'I' : '?'))));
*/

tgrab0 = tgrab;
tgrab = timeGetTime();

  // copy, decompress, and color convert data as necessary
  ExtractPixels(dest, where);
  UnlockBuffer(where);  

  if ((tgrab0 != 0) && (jumped == 0))
  {
    tdiff = tgrab - tgrab0;
    *advance = ROUND(freq * tdiff / 1000.0);
  }

  return 1;
}


//= Allow background and foreground threads to sync up.

void jhcLiveVSrc::Percolate ()
{
  MSG msg;

  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
	  DispatchMessage(&msg);
  }
}


//= Load pixels from video capture buffer into output image.

void jhcLiveVSrc::ExtractPixels (jhcImg& dest, UC8 *src)
{
  // if already in the right place it will be in the right format since
  // uncompressed RGB, mono, and RGB-to-mono cases handled by callback
  if (src == dest.PxlSrc()) 
    return;

  // special cases already handled by SaveFrameData
  // these should be already covered by previous clause
  if ((codec == NULL) && 
      (d == 1) && ((b == 8) || (b == 24) || (b == 32)))
    dest.CopyArr(src);

  // 24 or 8 bit straight copying, possibly with decompression
  if (((b == 24) && (d == 3)) || ((b == 8) && (d == 1)))
  {
    if (codec != NULL)
      ICDecompress(codec, 0, native, src, expand, dest.PxlDest());      
    else
      dest.CopyArr(src);       // already covered for 8 bits
    return;
  }

  // 32 bit color results with NULL top byte
  if ((b == 32) && (d == 3))
  {
    if (codec != NULL)
    {
      ICDecompress(codec, 0, native, src, expand, big);
      C32toRGB(dest, big);
    }
    else
      C32toRGB(dest, big);
    return;
  }
  
  // 24 or 32 bit monochrome conversion, possibly after decompression
  if (((b == 24) || (b == 32)) && (d == 1))
  {
    if (codec != NULL)
    {
      ICDecompress(codec, 0, native, src, expand, big);
      dest.CopyField(big, 1, ((b == 24) ? 3 : 4), 0);
    }
    else
      dest.CopyField(src, 1, ((b == 24) ? 3 : 4), 0);  // already covered
    return;
  }

  // 16 bit color conversion case (decompress first if necessary)
  if ((b == 16) && (d == 3))
  {
    if (codec != NULL)
    {
      ICDecompress(codec, 0, native, src, expand, big);
      C555toRGB(dest, big);      
    }
    else
      C555toRGB(dest, src);      
    return;
  }

  // 16 bit mono conversion, averages channels together 
  if ((b == 16) && (d == 1))
  {
    if (codec != NULL)
    {
      ICDecompress(codec, 0, native, src, expand, big);
      C555toMono(dest, big);      
    }
    else
      C555toMono(dest, src);      
    return;
  }

  // should never get here
  Complain("Cannot convert data saved by video callback");
}


//////////////////////////////////////////////////////////////////////////////
//                     RGB 5:5:5 color format parsing                       //
//////////////////////////////////////////////////////////////////////////////

/* CPPDOC_BEGIN_EXCLUDE */

// uncomment the line below in case image was stored
// in little-endian order and read big-endian or vice versa

// #define SWAP555

#ifndef SWAP555

#define RED555(v) (((v) & 0x7C00) >> 7)  
#define GRN555(v) (((v) & 0x03E0) >> 2)
#define BLU555(v) (((v) & 0x001F) << 3)


#else

#define RED555(v) (((v) & 0x007C) << 1)  
#define GRN555(v) (((v) & 0x0003) << 6) | (((v) & 0xE000) >> 10)) 
#define BLU555(v) (((v) & 0x1F00) >> 5)

#endif

/* CPPDOC_BEGIN_EXCLUDE */
 

//= Convert a buffer of 16 bit RGB (0:B5:G5:R5) into 24 bit RGB (R8:G8:B8).
// assumes source buffer is in correct format and of correct size
// only does translation in specified ROI (for speed)
// written in generic big-endian / little-endian style for portability

int jhcLiveVSrc::C555toRGB (jhcImg& dest, UC8 *src)
{
  if (dest.RoiMod4() != 0)
    return C555toRGB_4(dest, src);

  // general purpose routine  
  // "d" points to output byte array while "rw" and "rh" give image size 
  // "sinc" and "dinc" tell number of padding bytes at end of each line
  int x, y, rw = dest.RoiW(), rh = dest.RoiH();
  int sinc, dinc = dest.RoiSkip();
  UL32 soff;
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  US16 *s;
  US16 in3;

  dest.RoiParams(&soff, &sinc, 2);
  s = (US16 *)(src + soff);
  sinc >>= 1;
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      in3 = *s++;
      *d++ = BLU555(in3);
      *d++ = GRN555(in3);
      *d++ = RED555(in3);
    }
    s += sinc;
    d += dinc;
  }
  return 1;
}


//= Faster color conversion works in 32 bit chunks (4 pixels at a time).
// written in generic big-endian / little-endian style for portability

int jhcLiveVSrc::C555toRGB_4 (jhcImg& dest, UC8 *src)
{
  int x, y, rw4 = dest.RoiW() >> 2, rh = dest.RoiH();
  int sinc, dinc = dest.RoiSkip() >> 2;
  UL32 in6, out4, soff;
  UL32 *s, *d = (UL32 *)(dest.PxlSrc() + dest.RoiOff());
    
  dest.RoiParams(&soff, &sinc, 2);
  s = (UL32 *)(src + soff);
  sinc >>= 2;
  for (y = rh; y > 0; y--)
  {
    for (x = rw4; x > 0; x--)
    { 
      in6 = *s++;
      out4 =  MBYTE0(BLU555(in6));  // blue 1
      out4 |= MBYTE1(GRN555(in6));  // green 1
      out4 |= MBYTE2(RED555(in6));  // red 1
      in6 >>= 16;
      out4 |= MBYTE3(BLU555(in6));  // blue 2
      *d++ = out4;                  // (b2:r1:g1:b1)
      out4 =  MBYTE0(GRN555(in6));  // green 2
      out4 |= MBYTE1(RED555(in6));  // red 2
      in6 = *s++;
      out4 |= MBYTE2(BLU555(in6));  // blue 3
      out4 |= MBYTE3(GRN555(in6));  // green 3
      *d++ = out4;                  // (g3:b3:r2:g2)
      out4 =  MBYTE0(RED555(in6));  // red 3
      in6 >>= 16;
      out4 |= MBYTE1(BLU555(in6));  // blue 4
      out4 |= MBYTE2(GRN555(in6));  // green 4
      out4 |= MBYTE3(RED555(in6));  // red 4
      *d++ = out4;                  // (r4:g4:b4:r3)
    }
    s += sinc;
    d += dinc;
  }
  return 1;
}


//= Takes 15 bit stacked RGB and averages channels to give 8 bit monochrome.

int jhcLiveVSrc::C555toMono (jhcImg& dest, UC8 *src)
{
  int x, y, sum, rw = dest.RoiW(), rh = dest.RoiH();
  int sinc, dinc = dest.RoiSkip();
  UL32 soff;
  UC8 *d = dest.PxlDest() + dest.RoiOff();
  US16 *s;
  US16 in3;

  dest.RoiParams(&soff, &sinc, 2);
  s = (US16 *)(src + soff);
  sinc >>= 1;
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    {
      in3 = *s++;
      sum = (BLU555(in3) >> 3) + (GRN555(in3) >> 3) + (RED555(in3) >> 3);
      *d++ = avg5[sum];
    }
    s += sinc;
    d += dinc;
  }
  return 1;
}


//= Takes 0:R:G:B and translates it to just sequential B:G:R.

int jhcLiveVSrc::C32toRGB (jhcImg& dest, UC8 *src)
{
  int x, y, rw = dest.XDim(), rh = dest.RoiH();
  int ssk4, dsk = dest.RoiSkip(); 
  UL32 in, soff;
  UC8 *d = dest.RoiDest();
  UL32 *s;

  // figure out corresponding ROI in source buffer    
  dest.RoiParams(&soff, &ssk4, 4);
  s = (UL32 *)(src + soff);
  ssk4 >>= 2;

  // do copying of pixels
  for (y = rh; y > 0; y--)
  {
    for (x = rw; x > 0; x--)
    { 
      in = *s++;
      d[0] = (UC8) BYTE0(in);   // blue               
      d[1] = (UC8) BYTE1(in);   // green               
      d[2] = (UC8) BYTE2(in);   // red          
      d += 3;     
    }
    d += dsk;
    s += ssk4;
  }
  return 1;
}


