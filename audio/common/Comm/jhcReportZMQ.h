// jhcReportZMQ.h : writes out data to a ZeroMQ stream
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

#ifndef _JHCREPORTZMQ_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCREPORTZMQ_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Writes out data data to a ZeroMQ stream.
// if host is blank then normal publish style, else use push style
// generally derive class from this base and add report generator

class jhcReportZMQ
{
// PRIVATE MEMBER VARIABLES
private:
  void *ctx, *pub;
  UC8 *buf;
  int bsz, fill;


// PUBLIC MEMBER VARIABLES
public:
  char host[80];
  int port, noisy;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcReportZMQ ();
  ~jhcReportZMQ ();
  void SetBuf (int n);
  void Reset ();
 
  // main functions
  int ZChk ();
  int ZPrintf (char *fmt, ...);
  void ZSend (UC8 val);
  void ZEnd ();


// PRIVATE MEMBER FUNCTIONS
private:
  int stream_start ();
  int stream_stop ();


};


#endif  // once




