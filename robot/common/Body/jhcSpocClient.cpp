// jhcSpocClient.cpp : single point of contact with robot via socket
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023 Etaoin Systems
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

#if !defined(UNDER_CE) && !defined(__linux__)
  #pragma comment(lib, "ws2_32.lib")   
  #define NET_ERROR SOCKET_ERROR
#else
  #include <sys/ioctl.h>
  #include <sys/select.h>
  #include <netdb.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <stdbool.h>
  #include <ctype.h>
  #define INVALID_SOCKET -1
  #define NET_ERROR -1
  #define SEL_ERROR -1
  #define SOCKET_ERROR NULL
  #define TRUE true
  #define FALSE false
#endif

#include <ctype.h>

#include "Interface/jms_x.h"

#include "Body/jhcSpocClient.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcSpocClient::jhcSpocClient ()
{
  // initialize socket and library
  ws_init();
  bot = INVALID_SOCKET; 

  // create default size buffers
  in = NULL;
  out = NULL;
  rsz = 0;
  tsz = 0;
  SetSize();

  // configure timing and clear state
  defaults();
  reset();
}


//= Set up default values for interaction timing.

void jhcSpocClient::defaults ()
{
  ping = 50;                 // max ms for reply
  barf = 5;                  // max non-responses
}


//= Default destructor does necessary cleanup.

jhcSpocClient::~jhcSpocClient ()
{
  terminate();
  ws_exit();
  SetSize(0, 0);             // release buffers
}


//= Set the maximum incoming and outgoing buffer sizes.

void jhcSpocClient::SetSize (int rx, int tx)
{
  // possibly make new incoming sensor buffer
  if (rx != rsz)
  {
    delete [] in;
    in = NULL;
    rsz = 0;
    if (rx > 0)
    {
      in = new UC8[rx];
      rsz = rx;
    }
  }

  // possibly make new outgoing command buffer
  if (tx != tsz)
  {
    delete [] out;
    out = NULL;
    tsz = 0;
    if (tx > 0)
    {
      out = new UC8[tx];
      tsz = tx;
    }
  }

  // always reset read and write pointers
  reset();
}


//= Initiatialize communication state.

void jhcSpocClient::reset ()
{
  RxInit();
  TxInit();
  delay = 20.0;              // 20 ms robot pause
  miss = -1;                 // robot not prompted yet
}


//= Get rid of the current connection (if any).
// returns -1 always for convenience

int jhcSpocClient::terminate ()
{
  if (bot != INVALID_SOCKET)
  {
    shutdown(bot, 2);        // SD_BOTH or SHUT_RDWR
    closesocket(bot);
  }
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                          Top Level Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Open a two way TCP/IP link to some remote host which is already listening.
// can take either a name like "beltaine" or an IP address "192.168.0.2"
// returns 1 if successfully linked, -1 if failed

int jhcSpocClient::Connect (const char *body, int port)
{
	LPHOSTENT host;
	SOCKADDR_IN info;
  LINGER end_t = {1, 0};     // {1, 0} needed so other end knows connection is gone
  unsigned long addr;
  BOOL nagle = FALSE;

  // try resolving either host name or ip address
  if (isascii(*body))
    host = gethostbyname(body);
  else
  {
    addr = inet_addr(body);
    host = gethostbyaddr((const char *)(&addr), 4, AF_INET);
  }
  if (host == NULL)
    return 0;

  // make a new raw socket (recv blocks, send immediate, hard shutdown)
  terminate();
  if ((bot = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    return 0;
  set_wait(bot, ping);
  setsockopt(bot, IPPROTO_TCP, TCP_NODELAY, (const char *)(&nagle), sizeof(BOOL));
  setsockopt(bot, SOL_SOCKET, SO_LINGER, (const char *)(&end_t), sizeof(LINGER));  

  // try to link to host
  info.sin_family = AF_INET;
  info.sin_addr = *((LPIN_ADDR)(*(host->h_addr_list)));
  info.sin_port = htons((US16) port);
  if (connect(bot, (LPSOCKADDR)(&info), sizeof(struct sockaddr)) == NET_ERROR)
    return terminate();
  reset();
  return 1;
}


//= Get recent sensor data from robot (blocks about 6ms via wifi).
// returns number of bytes actually received or -1 if connection broken

int jhcSpocClient::Update ()
{
  double blend = 0.1;        // delay update rate
  UL32 t0;
  int ms;

  // sanity check
  if ((bot == INVALID_SOCKET) || (in == NULL))
    return -1;
  if (miss < 0)              // nothing expected
    return 0;

  // wait a while for sensor packet to arrive 
  t0 = jms_now();
  if ((rlim = (int) recv(bot, (char *) in, rsz, 0)) == NET_ERROR)
  {
    if (!timeout() || (miss++ >= barf))
      return terminate();
    return 0;                // sensor data is late
  } 

  // adjust robot auto-send delay so recv waits only 1ms
  if (miss <= 0)
  {
    ms = jms_diff(jms_now(), t0);
    delay += blend * (1.0 - ms);
    delay = __max(0.0, __min(delay, ping));
  }

  // check response header (strips) and set up for next cycle
  if (in[0] != 'S')          
    rlim = 0;
  RxInit();                  
  miss = -1;                 // robot not prompted yet
  return rlim;
}


//= Send assembled command packet to robot (never blocks).
// returns number of bytes actually sent or -1 if connection broken

int jhcSpocClient::Issue ()
{
  int n;

  // sanity check
  if ((bot == INVALID_SOCKET) || (out == NULL))
    return -1;
  if (miss > 0)              // still waiting for sensor data
    return 0;
  if (tsz < 2)
    return 0;

  // main transmission includes robot return delay
  out[1] = (UC8) ROUND(delay);
  if ((n = (int) send(bot, (char *) out, wr, 0)) == NET_ERROR)
    return terminate();
  TxInit();                  // sets up new command header
  miss = 0;                  // robot just prompted for sensors
  return n;
}


///////////////////////////////////////////////////////////////////////////
//                           Sensor Unpacking                            //
///////////////////////////////////////////////////////////////////////////

//= Get a 32 bit floating point value from head of receive buffer.

double jhcSpocClient::RxF ()
{
  float fv;

  if ((rd + 4) > rlim)
    return 0.0;
  fv = *((float *)(in + rd));          // little endian
  rd += 4;
  return((double) fv);
}


//= Get a string up to given size from head of receive buffer.
// always removes full null-terminated string and null-terminate output

void jhcSpocClient::RxS (char *txt, int ssz)
{
  char c;
  int last = ssz - 1, cnt = 0;

  while (rd < rlim) 
    if ((c = (char)(in[rd++])) == '\0')
      break;
    else if (cnt < last)
      txt[cnt++] = c;
  txt[cnt] = '\0';
}


//= Load a sequence of several 8 bit positive integers.

void jhcSpocClient::Rx8 (int *v0, int *v1, int *v2, int *v3)
{
  if (v0 == NULL)
    return;
  *v0 = Rx8();
  if (v1 == NULL)
    return;
  *v1 = Rx8();
  if (v2 == NULL)
    return;
  *v2 = Rx8();
  if (v3 == NULL)
    return;
  *v3 = Rx8();
}


//= Load a sequence of 32 bit floating point numbers.

void jhcSpocClient::RxF (double *v0, double *v1, double *v2, double *v3)
{
  if (v0 == NULL)
    return;
  *v0 = RxF();
  if (v1 == NULL)
    return;
  *v1 = RxF();
  if (v2 == NULL)
    return;
  *v2 = RxF();
  if (v3 == NULL)
    return;
  *v3 = RxF();
}


///////////////////////////////////////////////////////////////////////////
//                            Command Packing                            //
///////////////////////////////////////////////////////////////////////////

//= Put a 32 bit floating point value at the head of the transmit buffer.

void jhcSpocClient::TxF (double val)
{
  float fv = (float) val;

  if ((wr + 4) > tsz)
    return;
  memcpy((void *)(out + wr), &fv, 4);  // little endian
  wr += 4;  
}


//= Put a string at the head of the transmit buffer.
// truncates and terminates if would exceed size of buffer

void jhcSpocClient::TxS (const char *txt)
{
  char c;
  int last = tsz - 1, i = 0;

  if (wr >= tsz)
    return;
  while (wr < last)
    if ((c = txt[i++]) == '\0')
      break;
    else
      in[wr++] = (UC8) c;
  in[wr++] = '\0';
}


