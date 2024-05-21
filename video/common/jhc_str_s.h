// jhc_str_s.h : alternate versions of Microsoft safe string functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023-2024 Etaoin Systems
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

// NOTE: "inline" needed for GCC to accept definitions in header file

#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>


///////////////////////////////////////////////////////////////////////////

// do not support string lengths

#define sscanf_s  sscanf
#define fprintf_s fprintf
#define fscanf_s  fscanf


///////////////////////////////////////////////////////////////////////////
//                           String Writing                              //
///////////////////////////////////////////////////////////////////////////

inline void strcpy_s (char *dest, size_t dsz, const char *src) 
  {snprintf(dest, dsz, "%s", src);}

template <size_t dsz> 
  inline void strcpy_s (char (&dest)[dsz], const char *src)
    {strcpy_s(dest, dsz, src);}
  
inline void strncpy_s (char *dest, size_t dsz, const char *src, size_t cnt) 
{
  size_t n = ((cnt >= dsz) ? dsz - 1 : cnt);
  strncpy(dest, src, n);
  dest[(n >= 0) ? n : 0] = '\0';
}

template <size_t dsz> 
  inline void strncpy_s (char (&dest)[dsz], const char *src, size_t cnt)
    {strncpy_s(dest, dsz, src, cnt);}

inline void strcat_s (char *dest, size_t dsz, const char *src)
{
  size_t len = strlen(dest);
  snprintf(dest + len, dsz - len, "%s", src);
}

template <size_t dsz> 
  inline void strcat_s (char (&dest)[dsz], const char *src)
    {strcat_s(dest, dsz, src);}


///////////////////////////////////////////////////////////////////////////
//                    Formatted Output and Input                         //
///////////////////////////////////////////////////////////////////////////

inline void _itoa_s (int val, char *dest, size_t dsz)    // always radix 10 
  {snprintf(dest, dsz, "%d", val);}

inline void _strnset_s (char *dest, size_t dsz, char c, int cnt)
{
  size_t n = (size_t) cnt;
  memset(dest, c, ((dsz < n) ? dsz : n));        // never NULL terminated
}

inline void sprintf_s (char *dest, size_t dsz, const char *format, ...)
{
  va_list args;
  va_start(args, format);
  vsnprintf(dest, dsz, format, args);
  va_end(args);
}

template <size_t dsz> 
  inline void sprintf_s (char (&dest)[dsz], const char *format, ...)
  {
    va_list args;                      // body repeated for "..."
    va_start(args, format);
    vsnprintf(dest, dsz, format, args);
    va_end(args);
  }

inline int vsprintf_s (char *dest, size_t dsz, const char *format, va_list args)
  {return vsnprintf(dest, dsz, format, args);}

template <size_t dsz> 
  inline int vsprintf_s (char (&dest)[dsz], const char *format, va_list args)
    {return vsnprintf(dest, dsz, format, args);}


///////////////////////////////////////////////////////////////////////////
//                            File Functions                             //
///////////////////////////////////////////////////////////////////////////

inline int fopen_s (FILE **f, const char *name, const char *mode)
{
  if (f == NULL)
    return -2;
  if ((*f = fopen(name, mode)) == NULL)
    return -1;
  return 0;
}
