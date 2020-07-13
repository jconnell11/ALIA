// jhcTaisCam.cpp : reads base64 encoded JPEG images from ZeroMQ stream
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

// Project / Properties / Configuration / Link / Input must have zmq.lib
#ifdef _WIN64
  #pragma comment(lib, "zmq64.lib")
#else
  #pragma comment(lib, "zmq.lib")
#endif

#include <stdio.h>

#include "Comm/zmq.h"                  // common audio

#include "Body/jhcTaisCam.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcTaisCam::~jhcTaisCam ()
{
  Close();
}


//= Default constructor initializes certain values.

jhcTaisCam::jhcTaisCam ()
{
  ctx = NULL;
  sub = NULL;
  out = NULL;
  build_cvt();
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Subscribe to ZeroMQ stream from some machine.
// returns 1 if okay, 0 or negative for error

int jhcTaisCam::Open (const char *host, int port)
{
  Close();
  return stream_start(host, port, "from_camera");
}


//= Fill given image with most recent data (and resize if needed).
// returns 1 if successful, 0 if more data needed, negative for error

int jhcTaisCam::Get (jhcImg& dest)
{
  char fname[80] = "jhc_temp.jpg";
  UL32 val;
  int rc, n;

  // make sure stream is running
  if ((ctx == NULL) || (sub == NULL))
    return reset(-5);

  // accumulate pieces until whole message is in buffer
  while ((rc = read_chunk()) < 2)
  {
    if (rc < 0)
      return reset(-4);
    if (rc == 0)
      return 0;
  }

  // start new image JPEG file
  if (chk_hdr() < 0)
    return reset(-3);
  if (fopen_s(&out, fname, "wb") != 0)
    return reset(-2);

  // translate from base64 ascii to data bytes
  while ((n = get24(val)) > 0)
    put24(val, n);

  // close file then convert JPEG to image
  reset(1);
  if (jio.LoadResize(dest, fname) < 0)
    return -1;
  return 1;
}


//= Failure wrapper makes sure buffer is always reset.

int jhcTaisCam::reset (int rc)
{
  if (rc <= 0)
    jprintf(">>> jhcTaisCam::reset %d\n", rc);
  if (out != NULL)
    fclose(out);
  out = NULL;
  fill = 0;
  done = 0;
  return rc;
}


//= Get another aliquot of data from ZeroMQ stream.
// returns 2 if some and end of message, 1 if some and more expected, 0 if nothing new, -1 for problem

int jhcTaisCam::read_chunk ()
{
  size_t sz = sizeof(int);
  int more, n, ask = bsz - fill;

  // get a desired number of bytes from stream
  if (ask <= 0)
    return -1;
  n = zmq_recv(sub, buf + fill, ask, ZMQ_DONTWAIT);
  if ((errno == 0) || (errno == EAGAIN) || (errno == EBADF))
    return 0;
  if (n < 0)
    return 0;

  // set "fill" to just beyond last valid data byte
  if (n > ask)
    jprintf(">>> Got %d bytes (ask = %d) in jhcTaisCam::read_chunk!\n", n, ask);
  fill += __min(n, ask);
  zmq_getsockopt(sub, ZMQ_RCVMORE, &more, &sz);    
  return((more > 0) ? 1 : 2);
}


//= Looks for start of base64 image encoding in "from_camera" messages.
// never called until full message assembled in buffer, advances "rd" pointer
// returns 1 if found, 0 if bad format

int jhcTaisCam::chk_hdr ()
{
  char key[80] = "payload";
  int pos = 0;

  // look for payload marker 
  rd = 0;
  while (rd < fill)
  {
    if ((char) buf[rd++] != key[pos])
      pos = 0;
    else if (++pos >= 7)
      break;
  }
  if (pos < 7)
    return 0;

  // skip closing quote then look for next quote
  rd++;
  while (rd < fill)
    if ((char) buf[rd++] == '"')
      return 1;
  return 0;
}


//= Disconnect from ZeroMQ at end (destructor calls automatically).

void jhcTaisCam::Close ()
{
  stream_stop();
  reset(1);
}


///////////////////////////////////////////////////////////////////////////
//                           Base64 Conversion                           //
///////////////////////////////////////////////////////////////////////////

//= Get 24 bit value using 4 x 6 bit characters in buffer.
// for input sequence a,b,c,d generates val = a:b:c:d
// partial a,b,c -> a:b:c:0 and a,b -> a:b:0:0 and a -> a:0:0:0
// never called until full message assembled in buffer
// <pre>
//   N1(sh>=12)  N2(sh>=6)   N3(sh>=0)
//   222211 11   1111 1100   00 000000
//   321098 76   5432 1098   76 543210
//   aaaaaa:bb - bbbb:cccc - cc:dddddd 
// </pre>
// returns number of bytes in value, 0 if end reached (or quote found)

int jhcTaisCam::get24 (UL32& val)
{
  UC8 c;
  int i, v6, sh = 18;

  // if everything read then reset buffer for next part
  if ((done > 0) || (rd >= fill))
    return 0;
  
  // try to read a 4 character sequence
  val = 0;
  for (i = 0; i < 4; i++)
  {
    // convert next ascii character to 6 bit value
    if (rd >= fill)
      break;
    c = buf[rd++];
    if ((v6 = cvt[c]) < 0)
    {
      if ((char) c != '"')
        jprintf(">>> jhcTaisCam::get24 last char [%d] = %c\n", c, (char) c);
      break;
    }

    // merge into combined value from top down
    val |= (v6 << sh);
    sh -= 6;
  }

  // see how many bytes actually assembled
  if (i >= 4)
    return 3;
  done = 1;
  fill = 0;
  return i;      // = chars because overlap
}


//= Put N bytes of 24 bit value to already open output file.
// for val = A:B:C outputs sequence A,B,C
// if n = 2 then val A:B:0 outputs sequence A,B only
// returns 1 for success, 0 for problem

int jhcTaisCam::put24 (UL32 val, int n)
{
  int i, sh = 16;

  for (i = 0; i < n; i++, sh -= 8)
    if (fputc((val >> sh) & 0xFF, out) == EOF)
      return 0;
  return 1;
}


//= Build lookup table for converting 6 bit chunks to ASCII characters.
// this is standard base64 coding (62,63 = +,/) with no padding

void jhcTaisCam::build_cvt ()
{
  char c;
  int i;

  for (i = 0; i < 256; i++)     
    cvt[i] = -1;                      // default = invalid
  i = 0;
  for (c = 'A'; c <= 'Z'; c++)        // A-Z = 0-25
    cvt[(int) c] = i++;
  for (c = 'a'; c <= 'z'; c++)        // a-z = 26-51
    cvt[(int) c] = i++;
  for (c = '0'; c <= '9'; c++)        // 0-9 = 52-61
    cvt[(int) c] = i++;
  cvt[(int) '+'] = 62;                // + = 62
  cvt[(int) '/'] = 63;                // slash = 63
}


///////////////////////////////////////////////////////////////////////////
//                             ZeroMQ Stream                             //
///////////////////////////////////////////////////////////////////////////

//= Start a ZeroMQ subscribing stream on some port number. 
// returns 1 if succcessful, 0 for problem

int jhcTaisCam::stream_start (const char *host, int port, const char *topic)
{
  char spec[80];

  // initialize ZMQ package
  if (ctx == NULL)
    if ((ctx = zmq_ctx_new()) == NULL)
      return 0;

  // subscribe style (link to known source)
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

int jhcTaisCam::stream_stop ()
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

