// jhcString.cpp : does something
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2014-2019 IBM Corporation
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

#include <windows.h>

#include "Interface/jhcString.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes as empty string.

jhcString::jhcString ()
{
  len = 0;
  ch[0] = '\0';
  wch[0] = L'\0';
}


//= Default constructor initializes with normal character string.

jhcString::jhcString (const char *val)
{
  Set(val);
}


//= Default constructor initializes with a wide character string.

jhcString::jhcString (const wchar_t *val)
{
  Set(val);
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Set the value from a normal string.
// can force conversion of embedded \0 chars using "n"

void jhcString::Set (const char *val, int n)
{
  int i;

  // invalid string
  if (val == NULL)
  {
    len = 0;
    *ch = '\0';
    *wch = L'\0';
    return;
  }

  // normal string
  len = ((n > 0) ? n : (int) strlen(val));
  for (i = 0; i <= len; i++)
    ch[i] = val[i];
  ch[len + 1] = '\0';        // double terminated
  C2W();
}


//= Set the value from a wide string.
// can force conversion of embedded \0 chars using "n"

void jhcString::Set (const wchar_t *val, int n)
{
  int i;

  // invalid string
  if (val == NULL)
  {
    len = 0;
    *ch = '\0';
    *wch = L'\0';
    return;
  }

  // normal string
  len = ((n > 0) ? n : (int) wcslen(val));
  for (i = 0; i <= len; i++)
    wch[i] = val[i];
  wch[len + 1] = L'\0';      // double terminated
  W2C();
}


//= Force double termination after "n" characters.
// useful when string was set from Txt pointer

void jhcString::Terminate (int n)
{
  if ((n < 0) || (n >= 500))
    return;
  ch[n]  = '\0';
  wch[n] = L'\0';
  if (n < 499)
  {
    ch[n + 1]  = '\0';
    wch[n + 1] = L'\0';
  }
}


//= Use wide string as source and make normal string consistent.
// returns character conversion for convenience

const char *jhcString::W2C ()
{
  int n;

  // do sanity check on length in case direct manipulation of "wch"
  len = __max(len, (int) wcslen(wch));
  n = len + 1;
  wch[n] = L'\0';            // double terminated

  // copy characters and terminators
#ifndef _UNICODE
  WideCharToMultiByte(CP_UTF8, 0, wch, n, ch, 500, NULL, NULL);
#else
  for (int i = 0; i <= n; i++)
    ch[i] = (char)(wch[i] & 0xFF);
#endif
  return ch;
}


//= Use normal string as source and make wide string consistent.
// returns wide character conversion for convenience

const wchar_t *jhcString::C2W ()
{
  int n;

  // do sanity check on length in case direct manipulation of "ch"
  len = __max(len, (int) strlen(ch));
  n = len + 1;
  ch[n] = '\0';              // double terminated
  
  // copy characters and terminators
#ifndef _UNICODE
  MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, ch, n, wch, 500); 
#else
  for (int i = 0; i <= n; i++)
    wch[i] = ch[i];
#endif
  return wch;
}

