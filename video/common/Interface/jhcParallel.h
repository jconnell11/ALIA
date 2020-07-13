// jhcParallel.h : simple parallel port use under Windows NT/2K/XP
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2004 IBM Corporation
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

// The DirectIO driver must be installed already (from www.direct-io.com)
// the "I/O port" field of the "directio" Control Panel dialog must
// have address 0x378-0x379 reserved and the the "security" field must 
// include the FULL path name of your executable in its list.

#ifndef _JHCPARALLEL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCPARALLEL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"
#include <conio.h>     // Microsoft Visual C++ definitions of _inp() and _outp()


/////////////////////////////////////////////////////////////////////////////

//= Output to standard parallel port. 
// <BR> OUTP pins:  9  8  7  6  5  4  3  2  (pins 18-25 = GND)
#define OUTP(b) ((jhcParallel == INVALID_HANDLE_VALUE) ? (b) : _outp(0x378, (b)))           


//= Input from standard parallel port.
// <BR> INP pins: 11 10 12 13 15  X  X  X  (pins 18-25 = GND)
#define INP(b)  ((b) = ((jhcParallel == INVALID_HANDLE_VALUE) ? 0 : (_inp(0x379) ^ 0x80)))


/////////////////////////////////////////////////////////////////////////////

//= Global variable which provides access to the parallel port.
// connect to the driver if it is not already initialized
// Note: parallel port is run in oldest uni-directional mode

HANDLE jhcParallel = CreateFile("\\\\.\\DirectIo0",                 // name of resource
                                GENERIC_READ | GENERIC_WRITE,       // access (read-write) mode
                                FILE_SHARE_READ | FILE_SHARE_WRITE, // sharing modes
                                NULL,                               // no security attributes
                                OPEN_EXISTING,                      // how to create
                                0,                                  // file attributes
                                NULL);                              // no attributes to copy


//= Variable records what went wrong, if anything.
// Hopefully this gets call directly after the CreateFile call for jhcParallel
// Otherwise could embed handle binding inside a ( ? : ) structure here

int jhcParallel0 = GetLastError();


#endif



