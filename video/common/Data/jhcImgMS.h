// jhcImgMS.h : still image codec using Windows operating system
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

#ifndef _JHCIMGMS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCIMGMS_
/* CPPDOC_END_EXCLUDE */


#include "jhcGlobal.h"

#include <wincodec.h>
#include <wincodecsdk.h>

#include "Data/jhcImg.h"
#include "Data/jhcImgIO.h"


//= Still image codec using Windows operating system.
// does normal BMP and RAS images in addition to JPEG, TIFF, GIF, and PNG
// can also save images in JPEG using "quality" variable in base class
// if global macro JHC_MSIO defined then this class becomes jhcImgIO
// uses Windows Imaging Components library

class jhcImgMS : public jhcImgIO_0
{
// PRIVATE MEMBER VARIABLES
private:
  IWICImagingFactory *wic;
  IWICBitmapDecoder *dec;
  IWICBitmapFrameDecode *frame;
  HANDLE file;
  char cached[500];
  int dok;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcImgMS ();
  ~jhcImgMS ();

  // reading and writing images
  int Decode (const char *fname);
  int Encode (const char *fname, const jhcImg& src);


// PROTECTED MEMBER FUNCTIONS
protected:
  virtual int ReadAltHdr (int *w, int *h, int *f, const char *fname);
  virtual int LoadAlt (jhcImg& dest, const char *fname);
  virtual int SaveAlt (const char *fname, const jhcImg& src);


// PRIVATE MEMBER FUNCTIONS
private:
  void clr_cache ();

  // reading images
  int color_raw (jhcImg& dest);
  int color_rgb (jhcImg& dest);
  int color_alpha (jhcImg& dest);
  int color_table (jhcImg& dest);

};


/////////////////////////////////////////////////////////////////////////////

//= Allows transparent insertion of other classes over top of normal class.
// define global macro JHC_MSIO to allow use of Windows image reader code
// define global macro JHC_JRST for medical 2Kx2K 16 bit reader instead

#if defined(JHC_JRST)
  #include "Data/jhcRead2K.h"
#elif defined(JHC_MSIO)
  typedef jhcImgMS jhcImgIO;
#endif


#endif  // once




