// jhcWebVSrc.cpp : repeatedly reads a still image from a website
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2010-2016 IBM Corporation
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

#include "Interface/jhcString.h"

#include "jhcWebVSrc.h"


//////////////////////////////////////////////////////////////////////////////
//                        Register File Extensions                          //
//////////////////////////////////////////////////////////////////////////////

#ifdef JHC_GVID

#include "Video/jhcVidReg.h"

#if defined(JHC_TIFF)
  JREG_VURL(jhcWebVSrc, "bmp pgm ras jpg jpeg tif tiff");
#elif defined(JHC_JPEG)
  JREG_VURL(jhcWebVSrc, "bmp pgm ras jpg jpeg");
#else
  JREG_VURL(jhcWebVSrc, "bmp pgm ras");
#endif

#endif  // JHC_GVID


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcWebVSrc::~jhcWebVSrc ()
{
  dealloc();
}


//= Get rid of any created structures.

int jhcWebVSrc::dealloc ()
{
  if (s != NULL)
  {
    s->Close();
    delete s;
    s = NULL;
  }
  return 0;
}


//= Default constructor initializes certain values.

jhcWebVSrc::jhcWebVSrc (const char *name, int index)
{
  // set basic reader properties
  strcpy_s(kind, "jhcWebVSrc");
  s = NULL;
  ok = -1;

  // unknown frame and video properties
  w = 0;
  h = 0;
  d = 0;
  aspect = 1.0;
  freq = 1.0;          // default webcam rate

  // set up to talk to internet
  try
  {
    s = new CInternetSession(_T("jhcWebVSrc"), 1, PRE_CONFIG_INTERNET_ACCESS, 
                             NULL, NULL, INTERNET_FLAG_DONT_CACHE);
  }
  catch (...)
  {
    s = NULL;
    return;
  }

  // set up name for temporary copy of image
  ok = 0;
  ParseName(name);
  sprintf_s(tmp, "jhc_temp%s", Extension());

  // try reading image once to get size
  copy_img(tmp);
  if (jio.Specs(&w, &h, &d, tmp, 1) > 0)
    ok = 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Read web image into supplied array after proper decoding.
// assumes array is big enough, writes BGR order, rows padded to next long word

int jhcWebVSrc::iGet (jhcImg& dest, int *advance, int src)
{
  copy_img(tmp);
  return jio.Load(dest, tmp, 1);
}


//= Read image at website and copy it all to some local file.

int jhcWebVSrc::copy_img (const char *dest)
{
  FILE *out;
  CStdioFile *in;
  jhcString f;
  char buffer[4096];
  int n;

  // attempt to get connection to desired image file
  f.Set(File());
  if ((in = s->OpenURL(f.Txt(), 1, INTERNET_FLAG_TRANSFER_BINARY | INTERNET_FLAG_RELOAD | INTERNET_FLAG_DONT_CACHE)) 
      == NULL)
    return -1;

  // check for valid output
  if (fopen_s(&out, dest, "wb") != 0)
    return 0;

  // transfer image piece by piece
  while ((n = in->Read(buffer, 4096)) > 0)
    fwrite(buffer, 1, n, out);

  // cleanup
  fclose(out);
  in->Close();
  return 1;
}


