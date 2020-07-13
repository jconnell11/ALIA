// jhcAviVSink.h : save successive frames to an AVI video file
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

#ifndef _JHCAVIVSINK_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCAVIVSINK_
/* CPPDOC_END_EXCLUDE */


// order of includes is important

#include "jhcGlobal.h"

#include <windows.h>                // instead of stdafx.h
#include <VfW.h>

#include "Video/jhcVideoSink.h"


//= Save successive frames to an AVI video file.

class jhcAviVSink : public jhcVideoSink
{
private:
  PAVIFILE pfile;                           
  PAVISTREAM avistr, pavi, pcomp;
  LPBITMAPINFOHEADER hdr;
  char harr[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)];

  HDC copy_dc, scrn_dc;
  HBITMAP dest_bmap;
  UC8 *pixels;
  int bsz;
  int cx, cy;

  int compress;
  double quality;
  char ctag[5];

public:
  ~jhcAviVSink ();
  jhcAviVSink (const char *fname);
  jhcAviVSink (int cstyle =7);

  int SetSizeWin (int wdes =0, int hdes =0, int wtrim =4, int htrim =4);
  int Compress (int cstyle =1, double q =0.85);  
  int Compress (const char *cname, double q =0.85);
  const char *CompName () {return ctag;}   /** Four character compressor code. */

  int PutWin (int wtrim =4, int htrim =4);

protected:
  void iClose ();
  int iOpen ();
  int iPut (const jhcImg& src);

private:
  void init_cfg (const char *fname, int cstyle);
  void InitHdr ();
  void InitAvi ();

  void StrFormat (AVISTREAMINFO& str, int wid, int ht, double fps);
  long ImgFormat (LPBITMAPINFOHEADER hdr, int wid, int ht, int nf);
  int PickCodec (AVICOMPRESSOPTIONS& opts, int style);

};




/////////////////////////////////////////////////////////////////////////////

// part of mechanism for automatically associating class with file extensions

extern int jvreg_jhcAviVSink;



// patch for legacy code

typedef jhcAviVSink jhcAviSave;

#endif




