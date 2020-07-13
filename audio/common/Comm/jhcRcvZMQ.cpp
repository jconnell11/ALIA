// jhcRcvZMQ.cpp : reads information from a ZeroMQ stream
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#include "Interface/jms_x.h"           // common video

#include "Comm/zmq.h"                  // common audio

#include "Comm/jhcRcvZMQ.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcRcvZMQ::jhcRcvZMQ ()
{
  // ZeroMQ not loaded yet
  ctx = NULL;

  // set up stream parameters
  strcpy_s(host, "localhost");
  port = 3845;
  *topic = '\0';
  noisy = 0;

  // clear buffer
  sub = NULL;
  fill = 0;
  rd = 0;
}


//= Default destructor does necessary cleanup.

jhcRcvZMQ::~jhcRcvZMQ ()
{
  stream_stop();
}


//= Close any open port in case number or host has changed.

void jhcRcvZMQ::Reset ()
{
  if (sub != NULL)
    zmq_close(sub);
  sub = NULL;
  fill = 0;
  rd = 0;
}


///////////////////////////////////////////////////////////////////////////
//                  Main Functions for Derived Classes                   //
///////////////////////////////////////////////////////////////////////////

//= Check if stream set up yet the get it ready for data.
// MUST be called before any data read in order to set up stream
// returns 1 if okay, 0 if problem

int jhcRcvZMQ::ZChk ()
{
  fill = 0;
  rd = 0;
  if (sub != NULL)
    return 1;
  if (stream_start() > 0)
    return 1;
  stream_stop();
  return 0;
}


//= Get packet from channel if available (occassionally blocks for multi-part messages).
// first part of buffer is channel spec, must set up proper host and port first
// return message size if successful, 0 if not ready, -1 for problem

int jhcRcvZMQ::ZRead ()
{
  size_t sz = sizeof(int);
  int n, more;

  // start filling buffer again (dumps any old message)
  fill = 0;
  rd = 0;
  while (1)
  {
    // try to read a message part 
    if ((n = zmq_recv(sub, buf + fill, bsz - fill - 1, ZMQ_DONTWAIT)) < 0)
    {
      fill = 0;
      if ((errno == 0) || (errno == EAGAIN) || (errno == EBADF))
        return 0;
      return -1;
    }

    // advance fill location then check to see if more parts
    fill += n;
    zmq_getsockopt(sub, ZMQ_RCVMORE, &more, &sz);    
    if (more <= 0)
      break;
    jms_sleep(1);                                    // busy wait
  }

  // add terminator then possibly print whole received packet
  buf[fill] = '\0';
  if (noisy > 0)
    jprint((const char *) buf);
  return fill;
}


//= Get channel part of message and advance read pointer.
// can call at any time during reading (gives cached value)
// returns NULL if a particular channel specified by "topic"

const char *jhcRcvZMQ::Channel ()
{
  const char *in = (const char *) buf;
  int lim = __min(fill, 79);

  // use cached value if already extracted
  if ((fill < 0) || (rd > 0))
    return chan;

  // copy buffer up to first space or open delimiter
  for (rd = 0; rd < lim; rd++)
  {
    if (strchr(" {[", in[rd]) != NULL)
      break;
    chan[rd] = in[rd];
  }

  // terminate string and set read pointer
  chan[rd] = '\0';   
  if (rd < fill)
    rd++;
  return chan;
}


//= Give whole message as a giant block of text.
// always strips channel spec, ignores read pointer, NULL terminated
// returns NULL if nothing left in buffer

const char *jhcRcvZMQ::Message ()
{
  int n;

  Channel();
  n = (int) strlen(chan) + 1;
  if (n >= fill)
    return NULL;
  return((const char *) buf + n);
}


//= Get next line from local buffer, up to new-line (removed, if any).
// returns 2 if successful, 1 if length limited, 0 if no more characters 

int jhcRcvZMQ::ZGets (char *line, int ssz)
{
  const char *in = (const char *) buf + rd;
  int n, nc, len = fill - rd;

  // check special cases
  if ((line == NULL) || (ssz <= 0))
    return 1;
  if (len <= 0)
    return 0;

  // look for CR or end of buffer 
  for (n = 0; n < len; n++)
    if (in[n] == 0x0A)
      break;

  // figure out how many characters can be copied
  nc = __min(n, ssz - 1);
  strncpy_s(line, ssz, in, nc);
  line[nc] = '\0';
  rd += (nc + 1);
  return((n > nc) ? 1 : 2);
}


///////////////////////////////////////////////////////////////////////////
//                             ZeroMQ Stream                             //
///////////////////////////////////////////////////////////////////////////

//= Start a ZeroMQ subscribing stream on some port number. 

int jhcRcvZMQ::stream_start ()
{
  char spec[80];

  // initialize ZMQ package
  if (ctx == NULL)
    if ((ctx = zmq_ctx_new()) == NULL)
      return 0;

  // pull style (volunteer as a sink)  
  if (*host == '\0')
  {
    if (sub == NULL)
      if ((sub = zmq_socket(ctx, ZMQ_PULL)) == NULL)
        return 0;
    sprintf_s(spec, "tcp://*:%d", port);
    if (zmq_bind(sub, spec) != 0)
      return 0;
    return 1;
  }

  // subscribe style (link to known source)
  if (sub == NULL)
    if ((sub = zmq_socket(ctx, ZMQ_SUB)) == NULL)
      return 0;
  sprintf_s(spec, "tcp://%s:%d", host, port);
  if (zmq_connect(sub, spec) != 0)
    return 0;
  if (zmq_setsockopt(sub, ZMQ_SUBSCRIBE, topic, strlen(topic)) != 0)
    return 0;
  return 1;
}


//= Break down stream subscribing agent.
// returns 0 for convenience

int jhcRcvZMQ::stream_stop ()
{
  // close socket
  if (sub != NULL)
    zmq_close(sub);

  // unload package
  if (ctx != NULL)
    zmq_ctx_destroy(ctx);

  // clear all pointers
  ctx  = NULL;
  sub = NULL;  
  return 0;
}
