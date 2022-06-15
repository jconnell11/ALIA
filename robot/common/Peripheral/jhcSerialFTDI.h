// jhcSerialFTDI.h : faster serial port using FTDI native drivers
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2017 IBM Corporation
// Copyright 2021 Etaoin Systems
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

#ifndef _JHCSERIALFTDI_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSERIALFTDI_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Interface/jhcSerial.h"     // common video

#include "Peripheral/ftd2xx.h"       // common robot


//= Faster serial port using FTDI native drivers.
// can either use a shared Windows serial port or dedicated FTDI driver
// can revert to normal VCP-style serial port using Bind function
// NOTE: must be able to find ftd2xx.dll at run-time!

class jhcSerialFTDI
{
// PRIVATE MEMBER VARIABLES
private:
  jhcSerial *sport;  // serial port to use (if shared with others)
  FT_HANDLE ftdi;    // special FTDI serial driver (sole use)


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcSerialFTDI (int port =0, int rate =38400, int rxn =64, int wait =1);
  ~jhcSerialFTDI ();
 
  // connection
  int SetSource (int port, int rate =38400, int rxn =64, int wait =1);
  int Bind (jhcSerial *s);
  void Release ();
  int Connection ();
  void ResetPort (int unplug =0);

  // main functions
  int Rcv ();
  int Xmit (int val);
  int RxArray (UC8 *vals, int n); 
  int TxArray (UC8 *vals, int n); 
  int Check ();
  void Flush ();
  void SetDTR (int val =1);
  void SetRTS (int val =1);
  int Handshake ();


// PRIVATE MEMBER FUNCTIONS
private:
  int ftdi_open (int port, int rate, int rxn, int wait);
  int ftdi_close ();

};


#endif  // once




