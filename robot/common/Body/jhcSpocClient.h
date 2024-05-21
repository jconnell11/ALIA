// jhcSpocClient.h : single point of contact with robot via socket
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

#pragma once

// order of includes is important

#include "jhcGlobal.h"

#ifndef __linux__
  #include <winsock.h>
#else
  #include <sys/socket.h>
  #include <fcntl.h>
  #include <errno.h>
#endif


//= Single point of contact with robot via socket.
// derive a robot body from this and assemble/disassemble packets
// <pre>
// operation
//   cycle 0: Update() reads received data (none, so return 0!)
//              ALIA decides what to do
//            Issue() sends new commands (can be empty)
//                -- rx lag on robot side --
//                robot interprets commands
//                robot waits <delay> ms
//                robot auto-sends new sensors
//                -- rx lag on ALIA side --
//              ALIA waits until start of next cycle (hides both rx lags)
// 
//   cycle 1: Update() reads received data (recent incoming sensors)
//              ALIA decides what to do
//            Issue() sends new commands
//                -- rx lag on robot side --
//                robot interprets commands
//                robot waits <delay> ms
//                robot auto-sends new sensors
//                -- rx lag on ALIA side --
//              ALIA waits until start of next cycle (hides both rx lags)
// 
// servo loop determines <delay> value in ms to ensure freshest sensors
// msg structure:
//   send = I <delay> (<hdr1> <cmd1>[]) (<hdr2> <cmd2>[]) ... 
//   recv = S (<hdr1> <data1>[]) (<hdr2> <data2>[]) ...
// all modalities have <hdr> so order is irrelevant and some can be omitted
// </pre>

class jhcSpocClient
{
// PRIVATE DATA TYPES
private:
#ifdef __linux__
  typedef bool BOOL;
  typedef struct hostent *LPHOSTENT;
  typedef struct in_addr *LPIN_ADDR;
  typedef struct sockaddr_in SOCKADDR_IN;
  typedef struct sockaddr *LPSOCKADDR;
  typedef struct linger LINGER;
  typedef fd_set FD_SET;
  typedef timeval TIMEVAL;
  typedef int SOCKET;
#endif

// PRIVATE MEMBER VARIABLES
private:
  SOCKET bot;
  UC8 *in, *out;
  double delay;
  int rsz, tsz, rlim, rd, wr, miss;


// PUBLIC MEMBER VARIABLES
public:
  int ping, barf;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcSpocClient ();
  ~jhcSpocClient ();
  void SetSize (int rx =100, int tx =100);

  // top level functions
  int Connect (const char *body, int port =1149);
  int Update ();
  int Issue ();

  // sensor unpacking
  void RxInit ()     {rd = 1;}
  int RxLen () const {return((rd < rlim) ? rlim - rd : 0);}
  char RxC ()        {return((rd < rlim) ? (char) in[rd++] : 0);}
  int Rx8 ()         {return((rd < rlim) ?  (int) in[rd++] : 0);}
  double RxF ();
  void RxS (char *txt, int ssz);
  void Rx8 (int *v0, int *v1 =NULL, int *v2 =NULL, int *v3 =NULL);
  void RxF (double *v0, double *v1 =NULL, double *v2 =NULL, double *v3 =NULL);

  // command packing
  void TxInit ()      {wr = 2; if (tsz > 0) out[0] = 'I';}
  void TxC (char c)   {if (wr < tsz) out[wr++] = (UC8) c;}
  void Tx8 (int v256) {if (wr < tsz) out[wr++] = (UC8) v256;}
  void TxF (double v0); 
  void TxF (double v0, double v1) {TxF(v0); TxF(v1);} 
  void TxF (double v0, double v1, double v2) {TxF(v0, v1); TxF(v2);} 
  void TxF (double v0, double v1, double v2, double v3) {TxF(v0, v1); TxF(v2, v3);} 
  void TxS (const char *txt);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and configuration
  void defaults ();
  void reset ();
  int terminate ();

  // low-level socket interface
#if !defined(UNDER_CE) && !defined(__linux__)
  inline void ws_init () {WSADATA specs; WSAStartup(MAKEWORD(1, 1), &specs);}
  inline void ws_exit () {WSACleanup();}
  inline void set_wait (SOCKET s, int ms) 
    {DWORD t = ms; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char *) &t, sizeof(t));}
  inline bool timeout () {return(WSAGetLastError() == WSAETIMEDOUT);}
#else
  inline void ws_init () {}
  inline void ws_exit () {}
  inline void set_wait (SOCKET s, int ms) 
    {struct timeval t = {0, 1000 * ms}; setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &t, sizeof(t));}
  inline bool timeout () {return(errno == EWOULDBLOCK);}
  inline void closesocket (int s) {close(s);}
#endif


};

