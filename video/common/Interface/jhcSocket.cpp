// jhcSocket.cpp : generic interface to socket communication
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2007-2015 IBM Corporation
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

// Build.Settings.Link.General must have library ws2_32.lib
#ifndef UNDER_CE
  #pragma comment(lib, "ws2_32.lib")
#endif


#include <string.h>

#include "Interface/jms_x.h"

#include "Interface/jhcSocket.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcSocket::jhcSocket ()
{
	WSADATA specs;

	WSAStartup(MAKEWORD(1, 1), &specs);                     
  target = INVALID_SOCKET; 
  link = INVALID_SOCKET; 
  pnum = 0;
  atom = 0;
}


//= Default destructor does necessary cleanup.

jhcSocket::~jhcSocket ()
{
  Close();
  if (target != INVALID_SOCKET)
  {
    shutdown(target, 2);  // 2 = SD_BOTH
	  closesocket(target);
  }
	WSACleanup();
}


//= Tells the network name of this computer.

const char *jhcSocket::Host ()
{
  strcpy_s(hname, "<unknown>");
  gethostname(hname, 80);
  return((const char *) hname);
}


//= Tells the IP address of this computer.

const char *jhcSocket::Address ()
{
	LPHOSTENT host;

  strcpy_s(ip, "127.0.0.1");
  if (gethostname(hname, 80) != SOCKET_ERROR)
  {
    host = gethostbyname(hname);
    strcpy_s(ip, inet_ntoa(*((LPIN_ADDR)(*(host->h_addr_list)))));
  }
  return((const char *) ip);
}


//= Immediately generate a packet for each transmit call.
// does not coalesce data using Nagel algorithm, even if packet mostly empty

void jhcSocket::AtomicTx (int doit)
{
  BOOL now = TRUE;

  atom = doit;
  if (link == INVALID_SOCKET)
    return;
  if (atom > 0)
    now = FALSE;
  setsockopt(link, IPPROTO_TCP, TCP_NODELAY, (const char *)(&now), sizeof(BOOL));
}


///////////////////////////////////////////////////////////////////////////
//                           Establish Connection                        //
///////////////////////////////////////////////////////////////////////////

//= Get ready to receive an incoming TCP/IP link from some remote host.
// will try for up to "timeout" seconds before giving up
// can call multiple times until incoming found (listens between calls)
// returns 1 if successful, 0 if no incoming connection linked, -1 for error

int jhcSocket::Listen (int myport, double timeout)
{
  LINGER targ_x = {1, 1}, link_x = {1, 1};
	LPHOSTENT host;
	SOCKADDR_IN info;
  char myname[80];
  unsigned long async = 1;
  int i, wait = 100, n = (int)(timeout / (0.001 * wait));

  // if first call or change of port then make up a receiver socket
  if ((target == INVALID_SOCKET) || (myport != pnum))
  {
    // kill any previous child socket
    Close();

    // possibly kill old receiver if different port requested
    if (target != INVALID_SOCKET)
      closesocket(target);
    if ((target = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
      return -1;

  	// get own ip address
    if (gethostname(myname, 80) == SOCKET_ERROR)
      return -1;
    host = gethostbyname(myname);

    // fill in address info
    info.sin_family = AF_INET;
    info.sin_addr = *((LPIN_ADDR)(*(host->h_addr_list)));
    info.sin_port = htons(myport);
    pnum = myport;

    // attach address info to target and set it up to listen for a connection
    // "mother" socket (non-blocking mode with hard shutdown) spawns actual sockets to use
    bind(target, (LPSOCKADDR)(&info), sizeof(struct sockaddr));
    ioctlsocket(target, FIONBIO, &async);                                           
    setsockopt(target, SOL_SOCKET, SO_LINGER, (const char *)(&targ_x), sizeof(LINGER));
    listen(target, 1);
  }

  // wait a while for something to connect 
  if (link != INVALID_SOCKET)
    return 1;
  for (i = 0; i <= n; i++)
    if ((link = accept(target, NULL, NULL)) != INVALID_SOCKET)
    {
      // make sure non-blocking mode with hard shutdown
      ioctlsocket(link, FIONBIO, &async);                                           
      setsockopt(link, SOL_SOCKET, SO_LINGER, (const char *)(&link_x), sizeof(LINGER));
      AtomicTx(atom);
      return 1;
    }
    else if (i < n)
      jms_sleep(wait);
  return 0;
}


//= Open a two way TCP/IP link to some remote host which is already listening.
// can take either a name like "beltaine" or an IP address "192.168.0.2"
// can set to block until completion or some timeout (typically 20 seconds)
// always initiates connection attempt, can check back later with Status to monitor
// returns 1 if successfully linked, 0 if failed

int jhcSocket::Connect (const char *hspec, int port, int block)
{
  LINGER link_x = {1, 0}; // {1, 0} needed so other end knows connection is gone
	LPHOSTENT host;
	SOCKADDR_IN info;
  unsigned long addr, async = 1;

  // try resolving either host name or ip address
  if (isascii(*hspec))
    host = gethostbyname(hspec);
  else
  {
    addr = inet_addr(hspec);
    host = gethostbyaddr((const char *)(&addr), 4, AF_INET);
  }
  if (host == NULL)
    return 0;

	// fill in address information 
  info.sin_family = AF_INET;
  info.sin_addr = *((LPIN_ADDR)(*(host->h_addr_list)));
  info.sin_port = htons(port);
  pnum = port;

  // always make a new socket (with hard shutdown, possibly non-blocking)
  Close();
  if ((link = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == INVALID_SOCKET)
    return 0;
  if (block <= 0)
    ioctlsocket(link, FIONBIO, &async);  
  setsockopt(link, SOL_SOCKET, SO_LINGER, (const char *)(&link_x), sizeof(LINGER));  
  AtomicTx(atom);

  // try connecting to specified host 
  if (connect(link, (LPSOCKADDR)(&info), sizeof(struct sockaddr)) == SOCKET_ERROR)
    return 0;
  ioctlsocket(link, FIONBIO, &async);    // set for non-blocking mode
  return 1;
}


//= Says whether socket is successfully connected currently.
// returns -1 if non-operational, 0 if attempting connection, 1 if okay

int jhcSocket::Status ()
{
  FD_SET wr, ex;
  TIMEVAL wait = {0, 0};

  // make sure some sort of connection was attempted
  if (link == INVALID_SOCKET)
    return -1;

  // poll status of sockect 
  FD_ZERO(&wr);
  FD_ZERO(&ex);
  FD_SET(link, &wr);
  FD_SET(link, &ex);
  if (select(0, NULL, &wr, &ex, &wait) == SOCKET_ERROR)
    return -1;

  // check to see if connection failed, else if socket is writeable yet
  if (FD_ISSET(link, &ex))
    return -1;
  if (FD_ISSET(link, &wr))
    return 1;
  return 0;
}


//= Terminate the current incoming or outgoing connection.
// always returns -1

int jhcSocket::Close ()
{
  if (link != INVALID_SOCKET)
  {
    shutdown(target, 2);  // 2 = SD_BOTH
	  closesocket(link);
  }
  link = INVALID_SOCKET;
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                              Transfer Data                            //
///////////////////////////////////////////////////////////////////////////

//= Peek to see if any bytes are waiting to be picked up.
// return 1 if bytes waiting, 0 if none, -1 if error

int jhcSocket::Any ()
{
  char dummy;

  if (link == INVALID_SOCKET)
    return -1;
  if (recv(link, &dummy, 1, MSG_PEEK) != SOCKET_ERROR)
    return 1;
  if (WSAGetLastError() != WSAEWOULDBLOCK)
    return -1;
  return 0;
}


//= Attempt to load a buffer of bytes from a remote computer.
// will try for up to "timeout" seconds before giving up
// returns number actually read or -1 if connection broken

int jhcSocket::Receive (UC8 *data, int cnt, double timeout)
{
  char *dest = (char *) data;
  int i, inc, total = 0, wait = 5, n = (int)(timeout / (0.001 * wait));

  if (link == INVALID_SOCKET)
    return -1;
  if (cnt <= 0)
    return 0;

  // get as much data as time allows
  for (i = 0; i <= n; i++)
  {
    // try to receive a blob of data
    inc = recv(link, dest, cnt - total, 0);
    if (inc != SOCKET_ERROR)
    {
      dest += inc;
      total += inc;
      if (total >= cnt)
        break;
    }
    else if (WSAGetLastError() != WSAEWOULDBLOCK)
      return Close();
    if (i < n)
      jms_sleep(wait);
  }
  return total;
}


//= Attempt to send a buffer of bytes to a remote computer.
// will try for up to "timeout" seconds before giving up (generally immediate)
// returns number actually sent or -1 if connection broken

int jhcSocket::Transmit (const UC8 *data, int cnt, double timeout)
{
  const char *src = (const char *) data;
  int i, inc, total = 0, wait = 5, n = (int)(timeout / (0.001 * wait));

  if (link == INVALID_SOCKET)
    return -1;
  if (cnt <= 0)
    return 0;

  // send as much data as time allows (usually all)
  for (i = 0; i <= n; i++)
  {
    // try to send a blob of data
    inc = send(link, src, cnt - total, 0);
    if (inc != SOCKET_ERROR) 
    {
      src += inc;
      total += inc;
      if (total >= cnt)
        break;
    }
    else if (WSAGetLastError() != WSAEWOULDBLOCK)
      return Close();
    if (i < n)
      jms_sleep(wait);
  }
  return total;
}


//= Receive a single byte from socket.
// will try for up to "timeout" seconds before giving up 
// does not alter value if no bytes received
// return 1 if successful, -1 for broken connection

int jhcSocket::Rx8 (UC8 *val, double timeout)
{
  if (link == INVALID_SOCKET)
    return -1;
  if (val == NULL)
    return 0;
  return Receive(val, 1, timeout);
}


//= Send out a single byte on oscket.
// will try for up to "timeout" seconds before giving up (generally immediate)
// return 1 if successful, -1 for broken connection

int jhcSocket::Tx8 (const UC8 val, double timeout)
{
  if (link == INVALID_SOCKET)
    return -1;
  return Transmit(&val, 1, timeout);
}


//= Receive a 4 byte number LSB first (useful for finding size of following transfer).
// will try for up to "timeout" seconds before giving up 
// does not alter value if no bytes received
// return 1 if successful, -1 for broken connection

int jhcSocket::Rx32 (UL32 *val, double timeout)
{
  UC8 digits[4];
  int ans;

  // check for proper conditions
  if (link == INVALID_SOCKET)
    return -1;
  if (val == NULL)
    return 0;

  // attempt reception
  ans = Receive(digits, 4, timeout);
  if (ans < 0)
    return -1;
  if (ans != 4)
    return 0;

  // reassemble value from bytes
  *val = (UL32) digits[0];
  *val |= ((UL32) digits[1]) << 8;
  *val |= ((UL32) digits[2]) << 16;
  *val |= ((UL32) digits[3]) << 24;
  return 1;
}


//= Send out a 4 byte number LSB first (useful for marking size of following transfer).
// will try for up to "timeout" seconds before giving up (generally immediate)
// returns 1 if successful, -1 for broken connection

int jhcSocket::Tx32 (const UL32 val, double timeout)
{
  UC8 digits[4];
  int ans;

  if (link == INVALID_SOCKET)
    return -1;

  // chop value into bytes
  digits[0] = (UC8)(val & 0xFF);
  digits[1] = (UC8)((val >> 8) & 0xFF);
  digits[2] = (UC8)((val >> 16) & 0xFF);
  digits[3] = (UC8)((val >> 24) & 0xFF);

  // attempt transmission
  ans = Transmit(digits, 4, timeout);
  if (ans < 0)
    return -1;
  if (ans != 4)
    return 0;
  return 1;
}


//= Send out a single byte followed by a 4 byte number (LSB first).
// will try for up to "timeout" seconds before giving up (generally immediate)

int jhcSocket::Tx40 (const UC8 tag, const UL32 val, double timeout)
{
  UC8 digits[5];
  int ans;

  if (link == INVALID_SOCKET)
    return -1;

  // chop value into bytes
  digits[0] = tag;
  digits[1] = (UC8)(val & 0xFF);
  digits[2] = (UC8)((val >> 8) & 0xFF);
  digits[3] = (UC8)((val >> 16) & 0xFF);
  digits[4] = (UC8)((val >> 24) & 0xFF);

  // attempt transmission
  ans = Transmit(digits, 5, timeout);
  if (ans < 0)
    return -1;
  if (ans != 5)
    return 0;
  return 1;
}


//= Get rid of any extra bytes currently in the input buffer.
// returns number of bytes dumped, -1 if broken socket

int jhcSocket::FlushIn ()
{
  UC8 val;
  int ans, cnt = 0;

  while ((ans = Rx8(&val, 0)) >= 1)
    cnt++;
  if (ans < 0)
    return -1;
  return cnt;
}

