// jhcMessage.cpp : standard communication from programs
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2014 IBM Corporation
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <conio.h>
#include <stdarg.h>
#include <ctype.h>

#include "Interface/jhcMessage.h"


// ==========================================================================

#ifndef _CONSOLE

#include <windows.h>

#include "Interface/jhcString.h"


/////////////////////////////////////////////////////////////////////////////
//                   Windows dialog version of functions                   //
/////////////////////////////////////////////////////////////////////////////

//= Serious error.

int Fatal (const char *msg, ...)
{
  va_list args;
  jhcString val(""), hdr("Error");

  // assemble message
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val.ch, msg, args);
    val.C2W();
  }

  // show to user
  MessageBox(NULL, val.Txt(), hdr.Txt(), MB_OK | MB_ICONSTOP);
  throw "JHC: User Halt";                                        // exit(-1);
  return -2;
}


//= Non-fatal error.

int Complain (const char *msg, ...)
{
  va_list args;
  jhcString val(""), hdr("Warning");

  // assemble message
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val.ch, msg, args);
    val.C2W();
  }

  // show to user
  MessageBox(NULL, val.Txt(), hdr.Txt(), MB_OK | MB_ICONEXCLAMATION);
  return -1;
}


//= Information for user.

int Tell (const char *msg, ...)
{
  va_list args;
  jhcString val(""), hdr("Note");
  
  // assemble message
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val.ch, msg, args);
    val.C2W();
  }

  // show to user 
  MessageBox(NULL, val.Txt(), hdr.Txt(), MB_OK);
  return 0;
}
 

//= Question for user (1 = yes, 0 = no).
// default is "yes" (1) for this version

int Ask (const char *msg, ...)
{
  va_list args;
  jhcString val(""), hdr("Question");
  int ans;

  // assemble message
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val.ch, msg, args);
    val.C2W();
  }

  // show to user and set up return value
  ans = MessageBox(NULL, val.Txt(), hdr.Txt(), MB_YESNO);
  if (ans == IDNO)
    return 0;
  return 1;
}


//= Question for user (1 = yes, 0 = no).
// default is "no" (0) for this version

int AskNot (const char *msg, ...)
{
  va_list args;
  jhcString val(""), hdr("Question");
  int ans;

  // assemble message
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val.ch, msg, args);
    val.C2W();
  }

  // show to user and set up return value
  ans = MessageBox(NULL, val.Txt(), hdr.Txt(), MB_YESNO | MB_DEFBUTTON2);
  if (ans == IDNO)
    return 0;
  return 1;
}


//= Question for user with ability to quit (1 = yes, 0 = no, -1 = quit).
// default is "no" (0) for this version

int AskStop (const char *msg, ...)
{
  va_list args;
  jhcString val(""), hdr("Question");
  int ans;

  // assemble message
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val.ch, msg, args);
    val.C2W();
  }

  // show to user and set up return value
  ans = MessageBox(NULL, val.Txt(), hdr.Txt(), MB_YESNOCANCEL);
  if (ans == IDCANCEL)
    return -1;
  if (ans == IDNO)
    return 0;
  return 1;
}


//= Allow user to halt program.

int Pause (const char *msg, ...)
{
  va_list args;
  jhcString val(""), hdr("Pause -- Cancel aborts program");
  int ans;

  // assemble message
  if (msg != NULL)
  {
    va_start(args, msg);
    vsprintf_s(val.ch, msg, args);
    val.C2W();
  }

  // show to user and set up return value
  ans = MessageBox(NULL, val.Txt(), hdr.Txt(), MB_OKCANCEL);
  if (ans != IDCANCEL)
    return 1;
  throw "JHC: User Halt";                                        // exit(0);
  return 0;
}


//= Special form for disabling printouts/popups.

int Ignore (const char *msg, ...)
{
  return -1;
}


// ==========================================================================

#else   // _CONSOLE

#include "Interface/jprintf.h"


/////////////////////////////////////////////////////////////////////////////
//                     text only version of functions                      //
/////////////////////////////////////////////////////////////////////////////

//= Serious error.

int Fatal (const char *msg, ...)
{
  va_list args;
  char val[500];

  // show message (printf has problems with "%")
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val, msg, args);
    jprint("*** ");
    jprint(val);
    jprint(" ! ***\n"); 
  }

  // wait for acknowledgement
  while (_kbhit())
    _getch();
  jprint("  Press any key to quit ... ");
  _getch();
  jprint("\n");

  // quit
#ifdef _WIN32_WCE
  exit(-1);
#else
  throw "JHC: Major Problem"; 
#endif
  return -2;
}


//= Non-fatal error.

int Complain (const char *msg, ...)
{
  va_list args;
  char val[500];

  // tell problem
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val, msg, args);
    jprint(">>> ");
    jprint(val);
    jprint(" !\n");
  }

  // wait for acknowledgement
  while (_kbhit())
    _getch();
  jprintf("  Hit any key to continue ... ");
  _getch();
  jprintf("\n");
  return -1;
}


//= Information for user.

int Tell (const char *msg, ...)
{
  va_list args;
  char val[500];

  // report message
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val, msg, args);
    jprint(val);
    jprint("\n");
  }

  // wait for acknowledgement
  while (_kbhit())
    _getch();
  jprintf("  Hit any key to continue ... ");
  _getch();
  jprintf("\n");
  return 0;
}


//= Question for user (1 = yes, 0 = no).

int Ask (const char *msg, ...)
{
  va_list args;
  char val[500];

  // show message (printf has problems with "%")
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val, msg, args);
    jprint(val);
  }

  // clear any earlier keystrokes
  while (_kbhit())
    _getch();
  jprint("  (y or n): ");

  // interpret reply
  if (tolower(_getch()) == 'n')
  {
    jprint("n\n");
    return 0;
  }

  // default "yes"
  jprint("y\n");
  return 1;
}


//= Alternate call for question (default = no).

int AskNot (const char *msg, ...)
{
  va_list args;
  char val[500];

  // show message (printf has problems with "%")
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val, msg, args);
    jprint(val);
  }

  // clear any earlier keystrokes
  while (_kbhit())
    _getch();
  jprint("  (y or n): ");

  // interpret reply
  if (tolower(_getch()) == 'y')
  {
    jprint("y\n");
    return 1;
  }

  // default "no"
  jprint("n\n");
  return 0;
}


//= Question for user (1 = yes, 0 = no, -1 = quit).

int AskStop (const char *msg, ...)
{
  va_list args;
  char val[500];
  char c;

  // show message (printf has problems with "%")
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val, msg, args);
    jprint(val);
  }

  // clear any earlier keystrokes
  while (_kbhit())
    _getch();
  jprint("  (y, n, or q): ");

  // interpret specific replies
  c = tolower(_getch());
  if (c == 'n')
  {
    jprint("n\n");
    return 0;
  }
  if (c == 'q')
  {
    jprint("q\n");
    return -1;
  }

  // default "yes"
  jprint("y\n");
  return 1;
}


//= Allow user to halt program.

int Pause (const char *msg, ...)
{
  va_list args;
  char val[500];

  // show message (printf has problems with "%")
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(val, msg, args);
    jprint(val);
    jprint("\n");
  }

  // clear any earlier keystrokes
  while (_kbhit())
    _getch();
  jprint("  Continue (y or n)?: ");

  // interpret reply (default = "yes")
  if (tolower(_getch()) != 'n')
  {
    jprint("y\n");
    return 1;
  }
  jprint("n\n");

  // quit
#ifdef _WIN32_WCE
  exit(0);
#else
  throw "JHC: User Halt";  // exit(0);
#endif
  return 0;
} 


//= Special form for disabling printouts/popups.

int Ignore (const char *msg, ...)
{
  return -1;
}


#endif  // JHC_TTY




