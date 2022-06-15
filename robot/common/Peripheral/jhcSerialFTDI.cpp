// jhcSerialFTDI.cpp : faster serial port using FTDI native drivers
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

// Build.Settings.Link.General must have library ftd2xx.lib
#ifdef _WIN64
  #ifdef FTD2XX_STATIC
    #pragma comment(lib, "ftd2xx_static64.lib")
  #else
    #pragma comment(lib, "ftd2xx_64.lib")
  #endif
#else
  #ifdef FTD2XX_STATIC
    #pragma comment(lib, "ftd2xx_static.lib")
  #else
    #pragma comment(lib, "ftd2xx.lib")
  #endif
#endif

#include "Interface/jms_x.h"               // common video

#include "Peripheral/jhcSerialFTDI.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcSerialFTDI::jhcSerialFTDI (int port, int rate, int rxn, int wait)
{
  sport = NULL;
  ftdi = NULL;
  SetSource(port, rate, rxn, wait);
}


//= Default destructor does necessary cleanup.

jhcSerialFTDI::~jhcSerialFTDI ()
{
  Release();
}


///////////////////////////////////////////////////////////////////////////
//                              Configuration                            //
///////////////////////////////////////////////////////////////////////////

//= Open a particular serial port for exclusive use,
// also takes expected received packet size (usually 256 or 64)
// this function uses special faster FTDI drivers allowing 1M baud
//   and latency and packet turnaround is much better than serial

int jhcSerialFTDI::SetSource (int port, int rate, int rxn, int wait)
{
  Release();
  return ftdi_open(port, rate, rxn, wait);
}


//= Communicate over an already opened serial port (shared).
// Note: Dynamixel servos come preset for 1M baud, not 256K baud (Win max)
// generally using the FTDI driver makes for better packet turnaround

int jhcSerialFTDI::Bind (jhcSerial *s)
{
  Release();
  sport = s;
  return sport->Valid();
}


//= Close or forget about any current serial port.
// Note: does NOT automatically close a shared Windows serial port

void jhcSerialFTDI::Release ()
{
  sport = NULL;
  ftdi_close();
}


//= Tells whether connected to an operational serial port.

int jhcSerialFTDI::Connection ()
{
  if (sport != NULL)
    return sport->Valid();
  if (ftdi != NULL)
    return 1;
  return 0;
}


//= Possible recovery solution attempts to more thoroughly reset port.
// need to call SetSource after an "unplug" to properly re-initialize port 

void jhcSerialFTDI::ResetPort (int unplug)
{
  if (ftdi == NULL)
    return;
  if (unplug > 0)
  {
    FT_CyclePort(ftdi);
    ftdi_close();
    return;
  }
  FT_ResetPort(ftdi);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Get serial character, wait if none received yet.
// -1 is stream error, -2 is timeout, else byte    

int jhcSerialFTDI::Rcv ()
{
  UC8 b;
  UC8 *pval = &b;
  int rc;

  if ((rc = RxArray(pval, 1)) <= 0)
    return rc;
  return b;
}


//= Send out a serial character and wait for completion.

int jhcSerialFTDI::Xmit (int val)
{
  UC8 b = (UC8) val;
  UC8 *pval = &b;

  return TxArray(pval, 1);
}


//= Receive a series of bytes over Windows or FTDI serial port.
// returns number of bytes actually read

int jhcSerialFTDI::RxArray (UC8 *vals, int n)
{
  DWORD qlen;
  int got;

  // see if Windows or FTDI serial port should be used
  if (sport != NULL)
    return sport->RxArray(vals, n);
  if (ftdi == NULL)
    return 0;

  // read bytes in and check for timeout (retry if not enough at first)
  if (FT_Read(ftdi, (LPVOID) vals, n, &qlen) == FT_IO_ERROR)
	  return -1;
  if ((got = (int) qlen) < n)
  {
    if (FT_Read(ftdi, (LPVOID)(vals + got), n - got, &qlen) == FT_IO_ERROR)
	    return -1;
    got += (int) qlen;
  }
  return got;
}


//= Transmit a series of bytes over Windows or FTDI serial port.
// returns number of bytes actually sent

int jhcSerialFTDI::TxArray (UC8 *vals, int n)
{
  DWORD sent;

  // see if Windows or FTDI serial port should be used
  if (sport != NULL)
    return sport->TxArray(vals, n);
  if (ftdi == NULL)
    return 0;

  // send array and make sure byte count matches
  if (FT_Write(ftdi, (LPVOID) vals, (DWORD) n, &sent) != FT_OK)
    return 0;
  return((int) sent);
}


//= See if any data has been received (1 = yes, 0 = no).

int jhcSerialFTDI::Check ()
{
  DWORD n;

  // see if Windows or FTDI serial port should be used
  if (sport != NULL)
    return sport->Check();
  if (ftdi == NULL)
    return 0;

  // find number of bytes in receive queue
  if (FT_GetQueueStatus(ftdi, &n) != FT_OK)
    return 0;
  return((n > 0) ? 1 : 0);
}


//= Pause a while then discard any received characters.

void jhcSerialFTDI::Flush ()
{
  jms_sleep(1);
  if (sport != NULL)
    sport->Flush(0);
  else if (ftdi != NULL)
    FT_Purge(ftdi, FT_PURGE_RX | FT_PURGE_TX);   // transmit buffer too
}


//= Set the Data Terminal Ready handshake signal (pin 4 on DB-9).

void jhcSerialFTDI::SetDTR (int val)
{
  if (sport != NULL)
    sport->SetDTR(val);
  else if (ftdi != NULL)
  {
    // use FTDI version of functions
    if (val > 0)
      FT_SetDtr(ftdi);
    else
      FT_ClrDtr(ftdi);
  }
}


//= Set the Ready To Send handshake signal (pin 7 on DB-9).

void jhcSerialFTDI::SetRTS (int val)
{
  if (sport != NULL)
    sport->SetRTS(val);
  else if (ftdi != NULL)
  {
    // use FTDI version of functions
    if (val > 0)
      FT_SetRts(ftdi);
    else
      FT_ClrRts(ftdi);
  }
}


//= Get input line from port as a bit string (DCD : RING : DSR : CTS).
// on DB-9 = pin 1 : pin 9 : pin 6 : pin 8

int jhcSerialFTDI::Handshake ()
{
  DWORD status;

  // see if Windows or FTDI serial port should be used
  if (sport != NULL)
    return sport->Handshake();
  if (ftdi == NULL)
    return 0;

  // get input bits into correct location
  if (FT_GetModemStatus(ftdi, &status) != FT_OK)
    return -1;
  return((status >> 4) & 0x0F);
}


///////////////////////////////////////////////////////////////////////////
//                        FTDI Serial Port Interface                     //
///////////////////////////////////////////////////////////////////////////

//= Opens specified serial port using FTDI driver.
// may need to use value other than 0 for dev if other FTDI serial ports
// also takes expected received packet size (usually 256 or 64)

int jhcSerialFTDI::ftdi_open (int port, int rate, int rxn, int wait)
{
  LONG snum;
  int i;

  // check for reasonable COM number
  ftdi_close();
  if (port <= 0)
    return 0;

  // find device mapped to a certain serial port number
  for (i = 0; i < 10; i++)
	  if (FT_Open(i, &ftdi) == FT_OK)
    {
      if (FT_GetComPortNumber(ftdi, &snum) == FT_OK)
        if (snum == port)
          break;
      ftdi_close();
    }
  if (ftdi == NULL)
    return 0;

  // configure port
	if (FT_ResetDevice(ftdi) != FT_OK)
    return ftdi_close();
	if (FT_SetDataCharacteristics(ftdi, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE) != FT_OK)
    return ftdi_close();
	if (FT_SetFlowControl(ftdi, FT_FLOW_NONE, (UCHAR) 0, (UCHAR) 0) != FT_OK)
    return ftdi_close();
  if (FT_SetBaudRate(ftdi, rate) != FT_OK)
    return ftdi_close();

  // set timeouts and packet sizes (fastest tx/rx cycle about 1-2ms)
  if (FT_SetLatencyTimer(ftdi, 2) != FT_OK)         // used to be 1 (hangs after a day?)
    return ftdi_close();
	if (FT_SetUSBParameters(ftdi, rxn, 64) != FT_OK)  // changed from 64 for mega-upload
    return ftdi_close();
	if (FT_SetTimeouts(ftdi, 20, 20) != FT_OK)
    return ftdi_close();

  // clear any garbage state
	if (FT_Purge(ftdi, FT_PURGE_RX | FT_PURGE_TX) != FT_OK)
    return ftdi_close();

  // wait before starting link (NEEDED!)
  if (wait > 0)
    jms_sleep(500);  
  return 1;
}


//= Closes any open FTDI driver instance.

int jhcSerialFTDI::ftdi_close ()
{
  if (ftdi != NULL)
    FT_Close(ftdi);
  ftdi = NULL;
  return 0;
}


