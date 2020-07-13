// jhcName.cpp : creates standard substrings from a file name
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1999-2013 IBM Corporation
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

#include <string.h>
#include <ctype.h>

#include "Data/jhcName.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor.

jhcName::jhcName ()
{
  Init();
}


//= Construct with a given name (calls ParseName).

jhcName::jhcName (const char *name)
{
  Init();
  ParseName(name);
}


//= Set up defaults for internal strings.

void jhcName::Init ()
{
  *FileName  = '\0';
  *FileNoExt = '\0';
  *JustDir   = '\0';
  *DiskSpec  = '\0';
  *Flavor    = '\0';
  Ext       = FileName;
  BaseExt   = FileName;
  BaseName  = FileNoExt;
  DirNoDisk = JustDir;
}


///////////////////////////////////////////////////////////////////////////
//                           Basic Functionality                         //
///////////////////////////////////////////////////////////////////////////

//= Test to see if probe matches string saved in "Flavor" variable.

int jhcName::IsFlavor (const char *spec) const
{
  if (_stricmp(spec, Flavor) == 0)
    return 1;
  return 0;
}


//= Test to see if complete file name has any wildcards characters.

int jhcName::HasWildcard () const
{
  if (strchr(FileName, '*') != 0)
    return 1;
  return 0;
}


//= See if specifier refers to some sort of URL instead of a file.
// can look explicitly for things like "http" or "mms"

int jhcName::Remote (const char *service) const
{
  char svc[20];
  int n = (int) strlen(DiskSpec);

  // strip colon from "drive" specification part
  if (n == 0)
    return 0;
  strcpy_s(svc, DiskSpec);
  svc[n - 1] = '\0';

  // look for a particular service if requested
  if (service != NULL)
  {
    if (_stricmp(svc, service) == 0)
      return 1;
    return 0;
  }

  // see if "disk" is something other than a single letter
  if ((n == 2) && isalpha(svc[0]))
    return 0;
  return 1;
/*
  // handle a variety of possible services
  if (_stricmp(svc, "http") == 0)
    return 1;
  if (_stricmp(svc, "https") == 0)
    return 1;
  if (_stricmp(svc, "ftp") == 0)
    return 1;
  if (_stricmp(svc, "mms") == 0)
    return 1;
  if (_stricmp(svc, "bwims") == 0)
    return 1;
  if (_stricmp(svc, "axmphttp") == 0)
    return 1;
  return 0;
*/
}


//= Save source file name and some shorter versions.

void jhcName::ParseName (const char *fname)
{
  int i, n;
  char *sub;
  
  // check for strange case of no name and do something reasonable
  if (fname == NULL)
  {
    Init();
    return;
  }
  if (strcmp(fname, FileName) == 0)
    return;

  // trim whitespace from the end
  n = (int) strlen(FileName);
  for (i = 1; i <= n; i++)
    if (FileName[n - i] != ' ')
      break;
    else
      FileName[n - i] = '\0';

  // FileName presumably works for accessing data
  // BaseExt points to everything after last directory specification
  NoRestarts(FileName, fname, 500);
  if ((sub = LastMark(FileName)) != NULL)
    BaseExt = sub + 1;
  else
    BaseExt = FileName;

  // Ext points to tail of this string (including "." in extension)
  if ((Ext = strrchr(BaseExt, '.')) == NULL)
    Ext = BaseExt + strlen(BaseExt);
  else if (_strnicmp(Ext + 1, "cgi", 3) == 0)   // ".cgi" not valid
    Ext = BaseExt + strlen(BaseExt);

  // Flavor defaults to extension minus dot (often overridden)
  if (*Ext != '\0')
    strcpy0(Flavor, Ext + 1, 50);
  else
    *Flavor = '\0';

  // FileNoExt can be used for making related files (e.g. +".mpi")
  // BaseName strips directory, points into tail of FileNoExt
  strcpy0(FileNoExt, FileName, 500);
  if ((sub = LastMark(FileNoExt)) != NULL)
    BaseName = sub + 1;
  else
    BaseName = FileNoExt;
  if ((sub = strrchr(BaseName, '.')) != NULL)
    *sub = '\0';

  // JustDir holds whatever prefix is before the base name 
  // DirNoDisk omits any disk specifier, points into tail of JustDir
  strcpy0(JustDir, FileName, 500);
  if ((sub = LastMark(JustDir)) != NULL)
    *(sub + 1) = '\0';
  else
    *JustDir = '\0';
  if ((sub = strchr(JustDir, ':')) != NULL)
    DirNoDisk = sub + 1;
  else
    DirNoDisk = JustDir;
  
  // DiskSpec holds just whatever occurs before the first colon (if any)
  strcpy0(DiskSpec, JustDir, 50);
  if ((sub = strchr(DiskSpec, ':')) != NULL)
    *(sub + 1) = '\0';
  else
    *DiskSpec = '\0';

  // often protocol looks like a disk specification
  n = (int) strlen(DiskSpec);
  if ((n > 2) && (*Flavor == '\0'))
  {
    strcpy_s(Flavor, DiskSpec);
    Flavor[n - 1] = '\0';
  }
}


//= Fixes up problems caused by cascaded directories and disk specifiers.

void jhcName::NoRestarts (char *dest, const char *src, int maxlen) const
{
  int n, lim = maxlen;
  char *d = dest;
  const char *sub, *tail, *head = src;

  // check for rightmost disk spec
  if ((tail = strchr(src, ':')) != NULL)    // was strrchr
  {
    // back up to get drive letter(s)
    while (1)
    {
      if ((sub = strchr(head, '\\')) == NULL)
        if ((sub = strchr(head, '/')) == NULL)
          break;
      if (sub >= tail)
        break;
      head = sub + 1;
    }
    
    // copy just this part to the output and advance pointers
    n = (int)(tail - head + 1);
    strncpy_s(d, lim, head, n);          // always adds terminator
    d += n;
    lim -= n;
    head = tail + 1;
  }

  // skip over any root directory re-accesses
/*
  while ((sub = strstr(head, "\\\\")) != NULL)
    head = sub + 2;
  while ((sub = strstr(head, "//")) != NULL)
    head = sub + 2;
*/
  strcpy0(d, head, lim);
}


//= Finds last symbol (rightmost) involved in directory specification.

char *jhcName::LastMark (char *path) const
{
  char *find, *sub = path;

  if ((find = strrchr(sub, '\\')) != NULL)
    sub = find;
  if ((find = strrchr(sub, '/')) != NULL)
    sub = find;
  if ((find = strchr(sub, ':')) != NULL)      // was strrchr
    sub = find;
  if (sub == path)                            // added for no directory
    return NULL;
  return sub;
}


//= Protected version of strcpy that truncates if necessary.
// will not overflow buffer and always null terminates

char *jhcName::strcpy0 (char *dest, const char *src, int dest_size) const
{
  strncpy_s(dest, dest_size, src, dest_size - 2);    // always adds terminator
  return dest;
}


//= Walks through the current filename and returns successive subdirectories.
// names cascade in length c:/foo -> c:/foo/bar --> c:/foo/bar/baz
// return -1 when done, else returns current level (first call returns 1)

int jhcName::NextSubDir (char *spec, const char *full, int last, int ssz) const
{
  int cnt = 0;
  char *end, *start;

  // copy whole string then advance past any disk drive info
  strcpy_s(spec, ssz, full);
  start = spec;
  end = strchr(start, ':');
  if (end != NULL)
  {
    start = end + 1;
    end = strpbrk(start, "\\/");
    if (end != NULL)
      start = end + 1;
  }

  // read over a certain number of delimiters
  // make sure there is a next delimiter (else file name)
  while (1)
  {
    if (*start == '\0')
      return -1;
    end = strpbrk(start, "\\/");    
    if (end == NULL) 
      return -1;
    if (++cnt > last)
      break;
    start = end + 1;
  }

  // chop at next delimiter 
  *end = '\0';                
  return cnt;
}


