// jhcSocket.h : generic interface to socket communication
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

#ifndef _JHCSOCKET_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSOCKET_
/* CPPDOC_END_EXCLUDE */


// order of includes is important

#include "jhcGlobal.h"

#include <winsock.h>


//= Simple generic interface to socket communication between computers.

class jhcSocket
{
// PRIVATE MEMBER VARIABLES
private:
	SOCKET target, link;
  char hname[80], ip[20];
  int pnum, atom;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcSocket ();
  ~jhcSocket ();
  const char *Host ();
  const char *Address ();
  int Port () {return pnum;};   /** Tell outgoing or incoming port number. **/
  void AtomicTx (int doit =1);

  // establish connection
  int Listen (int myport =51779, double timeout =30.0);
  int Connect (const char *hspec, int port =51779, int block =1);
  int Status ();
  int Close ();

  // transfer data
  int Any ();
  int Receive (UC8 *data, int cnt, double timeout =10.0);
  int Transmit (const UC8 *data, int cnt, double timeout =1.0);
  int Rx8 (UC8 *val, double timeout =10.0);
  int Tx8 (const UC8 val, double timeout =1.0);
  int Rx32 (UL32 *val, double timeout =10.0);
  int Tx32 (const UL32 val, double timeout =1.0);
  int Tx40 (const UC8 tag, const UL32 val, double timeout =1.0);
  int FlushIn ();

};


#endif  // once




