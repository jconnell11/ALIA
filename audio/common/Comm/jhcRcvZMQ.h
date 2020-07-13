// jhcRcvZMQ.h : reads information from a ZeroMQ stream
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

#ifndef _JHCRCVZMQ_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCRCVZMQ_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Reads information from a ZeroMQ stream.
// if host is blank then use pull style, else use normal publish style
// default host is "localhost" so port specifies a SUB connection
// subscribes to a single channel unless topic is empty string
// generally derive class from this base and add report parser

class jhcRcvZMQ
{
// PRIVATE MEMBER VARIABLES
private:
  static const int bsz = 16384;             /** Max message size. */

  // general operation
  void *ctx, *sub;
  UC8 buf[bsz];
  char chan[80];
  int rd, fill;


// PUBLIC MEMBER VARIABLES
public:
  char host[80], topic[80];
  int port, noisy;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcRcvZMQ ();
  ~jhcRcvZMQ ();
  void Reset ();
 
  // main functions
  int ZChk ();
  int ZRead ();
  const char *Channel ();
  const char *Message ();
  int ZGets (char *line, int ssz);

  // reading - convenience
  template <size_t ssz>
    int ZGets (char (&line)[ssz])
      {return ZGets(line, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  int stream_start ();
  int stream_stop ();


};


#endif  // once




