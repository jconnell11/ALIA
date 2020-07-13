// jhcReportZMQ.cpp : writes out data to a ZeroMQ stream
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

// Project / Properties / Configuration / Link / Input must have zmq.lib
#ifdef _WIN64
  #pragma comment(lib, "zmq64.lib")
#else
  #pragma comment(lib, "zmq.lib")
#endif


#include <stdio.h>

#include "Comm/zmq.h"                 // common audio
#include "Comm/jhcReportZMQ.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcReportZMQ::jhcReportZMQ ()
{
  // ZeroMQ not loaded yet
  ctx = NULL;

  // set up stream parameters
  *host = '\0';                        // publish not push
  port = 4571;
  noisy = 0;

  // clear buffer
  buf = NULL;
  SetBuf(4096);
  pub = NULL;
}


//= Default destructor does necessary cleanup.

jhcReportZMQ::~jhcReportZMQ ()
{
  stream_stop();
  delete [] buf;
}


//= Set the size of the message buffer.

void jhcReportZMQ::SetBuf (int n)
{
  if (n <= 0)
    return;
  delete [] buf;
  buf = new UC8[n];
  bsz = n;
  fill = 0;
}


//= Close any open port in case number has changed.

void jhcReportZMQ::Reset ()
{
  if (pub != NULL)
    zmq_close(pub);
  pub = NULL;
  fill = 0;
}


///////////////////////////////////////////////////////////////////////////
//                  Main Functions for Derived Classes                   //
///////////////////////////////////////////////////////////////////////////

//= Check if stream set up yet the get it ready for data.
// MUST be called before any data sent in order to set up stream
// returns 1 if okay, 0 if problem

int jhcReportZMQ::ZChk ()
{
  fill = 0;
  if (pub != NULL)
    return 1;
  if (stream_start() > 0)
    return 1;
  stream_stop();
  return 0;
}


//= Print something to ZeroMQ transmission buffer.
// automatically sends if buffer gets too full, calls limited to 500 chars
// returns 1 for convenience, 0 if message too big

int jhcReportZMQ::ZPrintf (char *fmt, ...)
{
  char msg[500] = "";
  va_list args;
  int n;

  // assemble message
  if (fmt != NULL)
  {
    va_start(args, fmt); 
    vsprintf_s(msg, fmt, args);
  }

  // flush buffer if new message does not fit
  if ((n = (int) strlen(msg)) <= 0)
    return 0;
  if ((fill + n) >= bsz)
  {
    zmq_send(pub, buf, fill, ZMQ_SNDMORE);
    fill = 0;
  }

  // add this message to end of buffer
  memcpy(buf + fill, msg, n);
  fill += n;
  if (noisy > 0)
    jprintf(msg);
  return 1;
}


//= Add a single byte to end of message queued for transmission.
// automatically sends first part if buffer gets too full

void jhcReportZMQ::ZSend (UC8 val)
{
  if (fill >= bsz)
  {
    zmq_send(pub, buf, fill, ZMQ_SNDMORE);
    fill = 0;
  }
  buf[fill++] = val;
  if (noisy > 0)
    jprintf("%c", (char) val);         // strange if not printable
}


//= Send whatever is in the ZeroMQ buffer right now and close packet.

void jhcReportZMQ::ZEnd ()
{
  if (fill <= 0)
    return;
  zmq_send(pub, buf, fill, 0);
  fill = 0;
  if (noisy > 0)
    jprintf("\n======================================\n\n");
}


///////////////////////////////////////////////////////////////////////////
//                             ZeroMQ Stream                             //
///////////////////////////////////////////////////////////////////////////

//= Start a ZeroMQ publishing stream on some port number. 

int jhcReportZMQ::stream_start ()
{
  char spec[80];

  // initialize ZMQ package 
  if (ctx == NULL)
    if ((ctx = zmq_ctx_new()) == NULL)
      return 0;

  // publish style (volunteer as a source)
  if (*host == '\0')
  {
    if (pub == NULL)
      if ((pub = zmq_socket(ctx, ZMQ_PUB)) == NULL)
        return 0;
    sprintf_s(spec, "tcp://*:%d", port);
    if (zmq_bind(pub, spec) != 0)
      return 0;
    return 1;
  }

  // push style (link to known sink)
  if (pub == NULL)
    if ((pub = zmq_socket(ctx, ZMQ_PUSH)) == NULL)
      return 0;
  sprintf_s(spec, "tcp://%s:%d", host, port);
  if (zmq_connect(pub, spec) != 0)
    return 0;
  return 1;
}


//= Break down stream publishing agent.
// returns 0 for convenience

int jhcReportZMQ::stream_stop ()
{
  // close socket
  if (pub != NULL)
    zmq_close(pub);

  // unload package
  if (ctx != NULL)
    zmq_ctx_destroy(ctx);

  // clear all pointers
  ctx  = NULL;
  pub = NULL;  
  return 0;
}
