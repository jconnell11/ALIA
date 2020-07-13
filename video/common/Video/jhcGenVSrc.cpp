// jhcGenVSrc.cpp : can create any type of video source given name
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

// NOTE: Global macro JHC_GVID must be declared to get types to self-register.
//       For static libraries include edited form of jhcGenVSrc_Reg.cpp file.
//       Project must be relinked if the selection of video readers changes.

#include <stdio.h>
#include <string.h>

#include "Interface/jhcMessage.h" 
#include "Video/jhcVidReg.h"

#include "Video/jhcGenVSrc.h" 


//////////////////////////////////////////////////////////////////////////
//                   Basic creation and deletion                        //
//////////////////////////////////////////////////////////////////////////

//= Get rid of any basic video streams created.

jhcGenVSrc::~jhcGenVSrc ()
{
  Release();
}


//= Standard initialization.

jhcGenVSrc::jhcGenVSrc ()
{
  init_vals();
}


//= Open file right away (like other video source constructors).

jhcGenVSrc::jhcGenVSrc (const char *name)
{
  init_vals();
  SetSource(name);
}


//= Initialize various parameters.

void jhcGenVSrc::init_vals ()
{
  // size and step characteristics
  strcpy_s(kind, "jhcGenVSrc");
  xlim = 0;
  ylim = 0;
  Mono = 0;
  Increment = 1;
  ByKey = 0;

  // generally never create MPEG index (this can be changed)
  index = -1;                

  // embedded video attributes
  ok = -1;
  gvid = NULL;

  // basic video attributes (these also get set by jhcVideoSrc constructor)
  w  = 0;
  h  = 0;
  d  = 0;
  w2 = 0;
  h2 = 0;
  d2 = 0;
  aspect  = 0.0;  // used to be 1.0
  aspect2 = 0.0;  
  freq  = 30.0;
  freq2 = 30.0;
  flen  = 554.3;
  flen2 = 554.3;
  dsc   = 1.0;
  dsc2  = 1.0;
  nframes = 0;

  // basic audio attributes (these also get set by jhcVideoSrc constructor)
  ach = 0;
  adim = 0;
  asps = 0;

  // what unmarked videos should be interpreted as
  Default("mpg");

  // see if video types need registering
#ifdef _LIB
  RegisterAll();
#endif
}


//= Sets the default extension for files with no extension.

void jhcGenVSrc::Default (const char *ext)
{
  strcpy_s(def_ext, ext);
}


///////////////////////////////////////////////////////////////////////////
//                   Basic Construction and Operation                    //
///////////////////////////////////////////////////////////////////////////

//= Function that dispatches on file extension.
// can tag on extension as a "hint" about file type
// will strip off this part if it can't find exact file

int jhcGenVSrc::SetSource (const char *name)
{
  FILE *tst;
  char hint[80] = "";

  // unbind any previous video source
  // then analyze name and make up default file type
  Release();
  ParseName(name);
  if ((*Ext == '\0') && (*Flavor == '\0') && (*def_ext != '\0'))
    sprintf_s(hint, "foo.%s", def_ext);

  // open specified file and check that it is okay 
  // try removing extension if file does not exist
  // omit test for framegrabber and pattern specifications
  if ((jvreg.Camera(FileName) == 0) && !HasWildcard() && !Remote())
  {       
    if (fopen_s(&tst, FileName, "rb") == 0)
      fclose(tst);
    else if (fopen_s(&tst, FileNoExt, "rb") == 0)
    {
      fclose(tst);
      sprintf_s(hint, "foo.%s", Ext);
      *Ext = '\0';
    }
    else 
    {
      if (noisy > 0)
        Complain("Could not open video file: %s", FileName);
      return -1;
    }
  }

  // try making up stream of correct type (from filename) 
  if (new_source(hint) < 1)
    return -1;
  ParseName(gvid->File());
  if (gvid->Valid() <= 0)
  {
    if (noisy > 0)
      Complain("Video source not operational: %s", FileName);
    return -1;
  }

  // copy wrapper settings down to underlying stream
  gvid->SetSize(xlim, ylim, Mono);
  gvid->SetStep(Increment, ByKey);
//  gvid->noisy = noisy;

  // copy relevant static member variables from underlying stream type
  ok = 1;
  w = gvid->XDim();
  h = gvid->YDim();
  d = gvid->Fields();
  w2 = gvid->XDim(1);
  h2 = gvid->YDim(1);
  d2 = gvid->Fields(1);
  aspect  = gvid->Ratio();
  aspect2 = gvid->Ratio(1);
  freq  = gvid->Rate();
  freq2 = gvid->Rate(1);
  flen  = gvid->Focal();
  flen2 = gvid->Focal(1);
  dsc   = gvid->Scaling();
  dsc2  = gvid->Scaling(1);
  nframes = gvid->Frames();
  strcpy_s(kind, gvid->StrClass());  // added

  // copy relevant audio variables
  ach = gvid->AChan();
  adim = gvid->ABits();
  asps = gvid->ARate();

  // get ready to read from beginning of source
  Rewind();
  return 1;
}


//= Determine which type of stream to make from extension.
// uses global registry object "jvreg" to build correct source class 

int jhcGenVSrc::new_source (const char *hint)
{
  const char *key = FileName;

  // see if extension is known
  if ((hint != NULL) && (*hint != '\0'))
    key = hint;
  if (jvreg.Known(key, 0) < 0)
  {
    if (noisy > 0)
      Complain("Unknown extension: %s", key);
    return -2;
  }

  // try reading stream
  gvid = jvreg.Reader(FileName, index, hint);
  if (gvid == NULL)
  {
    if (noisy > 0)
      Complain("Cannot read source: %s", FileName);
    return -1;
  }
  gvid->noisy = 0;
  return 1;
}


//= Destroy whatever type of stream might have been bound previously.
// although embedded source cast to jhcVideoSrc for most operations
// must remember actual type in order to call correct destructor

void jhcGenVSrc::Release ()
{
  Prefetch(0);
  if (gvid != NULL)
    delete gvid;
  gvid = NULL;
  ok = -1;
  nframes = 0;
}


///////////////////////////////////////////////////////////////////////////
//                       Pass-through Functions                          //
///////////////////////////////////////////////////////////////////////////

// ==> RESPONDS TO THESE MESSAGES BY RUNNING HANDLER IN UNDERLYING TYPE. <==

//= Change the number of frames skipped between calls.

void jhcGenVSrc::SetStep (int offset, int key)
{
  Increment = offset;
  ByKey = key;
  if (gvid == NULL)
    return;
  gvid->SetStep(offset, key);
  ok = gvid->Valid();  
}


//= Pass along request for different framerate.

void jhcGenVSrc::SetRate (double fps)
{
  if (gvid == NULL)
    return;
  gvid->SetRate(fps);
  freq = gvid->Rate();
  freq2 = gvid->Rate(1);
}


//= Change the size and color depth of the image requested.

void jhcGenVSrc::SetSize (int xmax, int ymax, int bw)
{
  xlim = xmax;                   // record in case stream changes
  ylim = ymax;
  Mono = bw;
  if (gvid == NULL)
    return;
  gvid->SetSize(xmax, ymax, bw);
  w = gvid->XDim();
  h = gvid->YDim();
  d = gvid->Fields();
  w2 = gvid->XDim(1);
  h2 = gvid->YDim(1);
  d2 = gvid->Fields(1);
  ok = gvid->Valid();
}


//= Tell name of base class being used to read images.

const char *jhcGenVSrc::StrClass ()
{
  if (gvid != NULL)
    return gvid->StrClass();
  return((const char *) kind);
}


//= See if the underlying class of the stream matches some probe.

int jhcGenVSrc::BaseClass (const char *cname)
{
  if (gvid == NULL)
    return 0;
  if (strcmp(cname, gvid->StrClass()) == 0)
    return 1;
  return 0;
}


//= How long to pause between frames during display.

int jhcGenVSrc::StepTime (double rate, int src)
{
  if (gvid == NULL)
    return 0;
  if (rate >= 0.0)
    return gvid->StepTime(rate, src);
  return gvid->StepTime(DispRate, src);
}


///////////////////////////////////////////////////////////////////////////
//                           Core Functions                              //
///////////////////////////////////////////////////////////////////////////

//= Set up so next read is of a particular frame.

int jhcGenVSrc::iSeek (int number)
{
  int ans;

  if (gvid == NULL)
    return 0;
  ans = gvid->Seek(number);
  ok = gvid->Valid();
  return ans;
}


//= Get next image and report how much many frames ahead this is.
// src = 0 for basic color, src = 1 for depth (if dual stream sensor)
// generally returns 1 for success (or number of aux bytes + 1)

int jhcGenVSrc::iGet (jhcImg& dest, int *advance, int src, int block)
{
  int ans;

  // communicate possible auto-looping
  if (gvid == NULL)
    return 0;  
  gvid->LastFrame = LastFrame;               

  // pass on request to internally bound stream
  ans = gvid->Get(dest, src, block);
  ok = gvid->Valid();
  if (ok > 0)
    *advance = gvid->Advance();  // extract "n"
  else
    *advance = 0;
  ParseName(gvid->File());
  return ans;
}


//= Get next pair of images from source.

int jhcGenVSrc::iDual (jhcImg& dest, jhcImg& dest2)
{
  int ans;

  // communicate possible auto-looping
  if (gvid == NULL)
    return 0;  
  gvid->LastFrame = LastFrame;               

  // pass on request to internally bound stream
  ans = gvid->DualGet(dest, dest2);
  ok = gvid->Valid();
  ParseName(gvid->File());
  return ans;
}


//= Get a specific number of samples starting at the current read point.
// returns number of samples (up to n) loaded into output array

int jhcGenVSrc::iAGet (US16 *snd, int n, int ch)
{
  int ans;

  if (gvid == NULL)
    return 0;
  ans = gvid->AGet(snd, n, ch);
  ok = gvid->Valid();
  anum = gvid->ALast();
  return ans;
}


