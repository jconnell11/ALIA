// jhcTaisCam.h : reads base64 encoded JPEG images from ZeroMQ stream
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _JHCTAISCAM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTAISCAM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcImg.h"               // common video
#include "Data/jhcImgIO.h"


//= Reads base64 encoded JPEG images from ZeroMQ stream.

class jhcTaisCam 
{
// PRIVATE MEMBER VARIABLES
private:
  static const int bsz = 200000;       /** Standard image size. */

  // ZeroMQ reading
  void *ctx, *sub;
  UC8 buf[bsz];
  int rd, done, fill;

  // image conversion
  jhcImgIO jio;
  UC8 cvt[256];
  FILE *out;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcTaisCam ();
  jhcTaisCam ();

  // main functions
  int Open (const char *host, int port);
  int Get (jhcImg& dest);
  void Close();


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  int read_chunk ();
  int chk_hdr ();
  int reset (int rc);

  // base64 conversion
  int get24 (UL32& val);
  int put24 (UL32 val, int n);
  void build_cvt ();

  // ZeroMQ stream
  int stream_start (const char *host, int port, const char *topic);
  int stream_stop ();


};


#endif  // once




