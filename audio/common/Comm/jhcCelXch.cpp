// jhcCelXch.cpp : read and write messages in CEL tagged JSON format
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015 IBM Corporation
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
  #ifdef _DEBUG
    #pragma comment(lib, "zmq64_d.lib")
  #else
    #pragma comment(lib, "zmq64.lib")
  #endif
#else
  #ifdef _DEBUG
    #pragma comment(lib, "zmq_d.lib")
  #else
    #pragma comment(lib, "zmq.lib")
  #endif
#endif


#include <sys/types.h>                // needed for time
#include <sys/timeb.h>                // needed for time
#include <stdio.h>

#include "Comm/zmq.h"                 // common audio

#include "Comm/jhcCelXch.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcCelXch::jhcCelXch ()
{
  // initialize ZeroMQ socket library
  xok = -1;
  if ((ctx = zmq_ctx_new()) != NULL)
    xok = 0;

  // no connections yet
  sub = NULL;
  out = NULL;
  *buf = '\0';

  // set up connection parameters
  Defaults();
}


//= Default destructor does necessary cleanup.

jhcCelXch::~jhcCelXch ()
{
  Close();
  if (ctx != NULL)
    zmq_ctx_destroy(ctx);
}


//= Open specified sockets.
// ch: 1 = out only, 2 = in only, 3 = both
// returns status equal to channel open codes

int jhcCelXch::Open (int ch)
{
  char url[80];
  int n, ms = 100;

  // make sure ZeroMQ library is okay
  if (xok < 0)
    return xok;

  // output channel
  if ((ch & 0x01) != 0)
    while (1)
    {
      // set up output connection (push)
      if ((out = zmq_socket(ctx, ZMQ_PUSH)) == NULL)
        break;
      sprintf_s(url, "tcp://%s:%d", host, oport);
      if (zmq_connect(out, url) != 0)
        break;

      // timeout when closing but still has messages
      if (zmq_setsockopt(out, ZMQ_LINGER, &ms, sizeof(ms)) != 0)
        break;
      xok |= 0x01;
      break;
    }

  // input channel
  if ((ch & 0x02) != 0)
    while (1)
    {
      // set up input connection (sub)
      if ((sub = zmq_socket(ctx, ZMQ_SUB)) == NULL)
        break;
      sprintf_s(url, "tcp://%s:%d", host, iport);
      if (zmq_connect(sub, url) != 0)
        break;

      // specify what sort of messages to respond to
      n = (int) strlen(filter);
      if (filter[n - 1] != ' ')
      {
        filter[n++] = ' ';
        filter[n] = '\0';
      }
      if (zmq_setsockopt(sub, ZMQ_SUBSCRIBE, filter, n) != 0)
        break;
      xok |= 0x02;
      break;
    }
  return xok;
}


//= Shutdown all sockets.

void jhcCelXch::Close ()
{
  // make sure context was created
  if (xok < 0)
    return;

  // close anything that exists
  if (out != NULL)
    zmq_close(out);
  if (sub != NULL)
    zmq_close(sub);

  // clear variables
  out = NULL;  
  sub = NULL;
  xok = 0;
}


//= Tell communication parameters currently in use.
// returns bit vector for open channels, -1 if broken

int jhcCelXch::PrintCfg () const
{
  jprintf("CEL communication parameters:\n");
  jprintf("  input host    = %s\n", host);
  jprintf("  input port    = %d\n", iport);
  jprintf("  input filter  = %s\n", filter);
  jprintf("  output port   = %d\n", oport); 
  jprintf("  recognizer ID = %s\n", sid);
  if (xok < 0)
    jprintf(">> BROKEN !!!\n");
  else if (xok == 0)
    jprintf("Awaiting connections ...\n");
  else
    jprintf("Ready for:%s%s\n", (((xok & 0x02) != 0) ? " IN" : ""), 
                                (((xok & 0x01) != 0) ? " OUT" : ""));
  jprintf("\n");
  return xok;
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcCelXch::Defaults (const char *fname)
{
  int ok = 1;

  ok &= xps.LoadText(host,   fname, "xch_host",   "cel-service.watson.ibm.com", 80);
  ok &= xps.LoadText(filter, fname, "xch_filter", "CEL-TRANSCRIPT", 80);
  ok &= xps.LoadText(sid,    fname, "xch_src",    "JHC Microsoft",  80);
  ok &= xch_params(fname);
  return ok;
}


//= Parameters defining ZeroMQ communications ports.

int jhcCelXch::xch_params (const char *fname)
{
  jhcParam *ps = &xps;
  int ok;

  ps->SetTag("xch_ports", 0);
  ps->NextSpec4( &iport, 7045, "Input port");  
  ps->NextSpec4( &oport, 7046, "Output port");  
  ps->NextSpec4( &echo,     1, "Generate input messages");  

  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Write current processing variable values to a file.

int jhcCelXch::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= xps.SaveText(fname, "xch_host",   host);
  ok &= xps.SaveText(fname, "xch_filter", filter);
  ok &= xps.SaveText(fname, "xch_src",    sid);
  ok &= xps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Load any input message that might have arrived.
// returns 1 if properly parsed, 0 if something, -1 if nothing, -2 if broken

int jhcCelXch::Get (jhcJSON *msg, int noisy)
{
  int n, n0 = (int) strlen(filter);

  // check for message (non-blocking)
  if ((xok < 0) || ((xok & 0x02) == 0))
    return -2;
  if ((n = zmq_recv(sub, buf, 4000, ZMQ_DONTWAIT)) <= 0)
    return -1;
  buf[n] = '\0';

  // convert to JSON datastructure after stripping tag
  if (n <= n0)
    return -1;
  if ((msg == NULL) || (msg->Ingest(buf + n0) == NULL))
    return 0;

  // possibly show received packet
  if (noisy > 0)
  {
    jprintf("\nReceived from %s on port %d:\n\n", host, iport);
    jprintf("%s\n\n", buf);
  }
  return 1;
}


//= Send out message with given tag.
// noisy: 0 = just send, 1 = send & print, 2 = just print
// returns 1 for success, 0 or negative for failure

int jhcCelXch::Push (const char *tag, const jhcJSON *msg, int noisy)
{
  int n = 0;

  // make sure sockets are okay (unless only printing)
  if (noisy < 2) 
    if ((xok < 0) || ((xok & 0x01) == 0))
      return -1;

  // set up prefix then JSON datastructure
  if (tag != NULL)
  {
    sprintf_s(buf, "%s ", tag);
    n = (int) strlen(buf);
  }
  if (msg != NULL)
    msg->Dump(buf + n, 0, 4000 - n);

  // try transmitting (potentially blocks)
  if (noisy <= 1)
  {
    n = (int) strlen(buf);
    if (zmq_send(out, buf, n, ZMQ_DONTWAIT) != n)
      return 0;
  }

  // possibly show what was sent
  if (noisy > 0)
  {
    jprintf("\n------------------\n");
    jprintf("Sent to %s on port %d:\n\n", host, oport);
    jprintf("%s\n", buf);
  }
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                         Transcript Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Generates a message similar to input text.
// uses only first name of speaker

int jhcCelXch::Input (const char *txt, const char *user, const char *alist, int noisy)
{
  char show[500];
  jhcJSON j;
  struct _timeb t;
  __int64 t2;
  const char *s = alist;
  char *d = show;

  // can disable using configuration file
  if (echo <= 0)
    return 0;

  // get ms since Jan 1, 1970
  _ftime_s(&t);
  t2 = 1000 * t.time + t.millitm;

/*
  char first[80];
  char *end;

  // add HTML tag to front of transcript message 
  if (user == NULL)
    strcpy(show, txt);
  else
  {
    // get just first name of user in HTML tag
    strcpy(first, user);
    if ((end = strchr(first, ' ')) != NULL)
      *end = '\0';
    sprintf(show, "<FONT COLOR=#00E000>%s:</FONT> %s", first, txt);
  }
*/

  // build basic structure
  j.MakeMap();
  j.SetKey("message",  txt);
  j.SetKey("time",     (double) t2);
  j.SetKey("username", ((user != NULL) ? user : "unknown"));
  j.SetKey("source",   sid);

  // save parsing results (substitute slashes for tabs)
  if (alist != NULL)
  {
    while (*s != '\0')
    {
      *d++ = ((*s == '\t') ? '/' : *s);
      s++;
    }
    *d = '\0';
    j.SetKey("parse", show);
  }

  // send it back to inptu source
  return Push(filter, &j, noisy);
}

