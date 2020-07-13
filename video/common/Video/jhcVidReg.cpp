// jhcVidReg.cpp : remembers file extensions and classes for videos
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2005-2015 IBM Corporation
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

// NOTE: Project must be relinked if the selection of video readers changes.

#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES 1
#define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES_COUNT 1

#include "jhcGlobal.h"

#include <string.h>
#include <stdio.h>

#include "Video/jhcVidReg.h"


///////////////////////////////////////////////////////////////////////////
//                          Global variables                             //
///////////////////////////////////////////////////////////////////////////

//= Always create one video registry object for binding classes to extensions.
 
jhcVidReg jvreg;


///////////////////////////////////////////////////////////////////////////
//                              Utilities                                //
///////////////////////////////////////////////////////////////////////////

//= Tells whether a class factory is associated with a particular extension.
// if wr is positive, checks that system can write files in that format 
// if full filename is given, checks just extension minus any final "+"
// does not verify that file exists

int jhcVidReg::Known (const char *fname, int wr)
{
  if (find_index(fname, wr) >= 0)
    return 1;
  return 0;
}


//= Tells whether the filename refers to a video capture device.

int jhcVidReg::Camera (const char *fname)
{
  int i;

  if ((i = find_index(fname, 0)) < 0)
    return 0;
  if (mode[i] >= 2)
    return 1;
  return 0;
}


//= Simply list all known extensions.
// if wr is positive then lists only video output types
// example: "avi mpg m2v m2p";

const char *jhcVidReg::ListAll (int wr)
{
  int i, n, ssz = sizeof filter;
  char *insert = filter;

  *insert = '\0';
  if (wr > 0)
    for (i = 0; i < nwr; i++)
    {
      sprintf_s(insert, ssz, "%s ", wrtag[i]);
      n = (int) strlen(insert);
      insert += n;
      ssz -= n;
    }
  else
    for (i = 0; i < nrd; i++)
    {
      sprintf_s(insert, ssz, "%s ", rdtag[i]);
      n = (int) strlen(insert);
      insert += n;
      ssz -= n;
    }
  if (insert > filter)
    *(insert - 1) = '\0';
  return (const char *) filter;
}


//= Generate filter text string for file dialog boxes.
// if wr is positive then lists only video output types
// example: "Videos\0*.avi;*.mpg;*.m2v;*.m2p\0Lists\0*.lst;*.txt\0All Files (*.*)\0*.*\0";
// can write full string length to "len" if non-NULL (includes embedded \0 chars)

const char *jhcVidReg::FilterTxt (int wr, int *len)
{
  int i, n, ssz = sizeof filter, cnt = 0;
  char *insert = filter;

  // write standard string start
  strcpy(filter, "Videos");
  n = (int) strlen(insert) + 1;              // keep the NULL terminator
  insert += n;
  ssz -= n;

  // add all the known extension types
  if (wr > 0)
    for (i = 0; i < nwr; i++)
    {
      sprintf_s(insert, ssz, "*.%s;", wrtag[i]);
      n = (int) strlen(insert);
      insert += n;
      ssz -= n;
      cnt++;
    }
  else
    for (i = 0; i < nrd; i++)
      if (strstr(filter, rdtag[i]) == NULL)  // not in list already
      {
        sprintf_s(insert, ssz, "*.%s;", rdtag[i]);
        n = (int) strlen(insert);
        insert += n;
        ssz -= n;
        cnt++;
      }
 
  // replace last semicolon with terminator
  if (cnt <= 0)
    insert = filter;
  else 
    *(insert - 1) = '\0';                

  // add text file filter for lists of image names
  strcpy_s(insert, ssz, "Image lists");
  n = (int) strlen(insert) + 1;              // keep the NULL terminator
  insert += n;
  ssz -= n;
  strcpy_s(insert, ssz, "*.lst;*.txt");
  n = (int) strlen(insert) + 1;              // keep the NULL terminator
  insert += n;
  ssz -= n;
  
  // add standard escape for all files (or just this if no extensions)  
  strcpy_s(insert, ssz, "All Files (*.*)");
  n = (int) strlen(insert) + 1;              // keep the NULL terminator
  insert += n;
  ssz -= n;
  strcpy_s(insert, ssz, "*.*");

  // tell true string length with embedded nulls
  if (len != NULL)
    *len = (int)(insert - filter + strlen(insert));
  return (const char *) filter;
}


///////////////////////////////////////////////////////////////////////////
//                         Registration and Lookup                       //
///////////////////////////////////////////////////////////////////////////

//= Associate an extension with a particular video source class factory.
// m stands for mode: 0 = file, 1 = URL, 2 = camera 
// if m > 1, then extension not listed in dialog file filter

int jhcVidReg::RegReader (jhcVideoSrc *fcn (const char *f, int i), const char *exts, int m)
{
  char list[200];
  char *end, *start = list;
  int cnt = 0;

  // do some sanity checking
  chk_lists();
  if ((nrd >= JVREG_MAX) || (fcn == NULL) || (exts == NULL) || (*exts == '\0'))
    return 0;
  strcpy(list, exts);
  while (nrd < JVREG_MAX)
  {
    // pull next extension out of list
    while (*start == ' ')
      start++;
    if (*start == '\0')
      break;
    end = strchr(start, ' ');
    if (end != NULL)
      *end = '\0';

    // add constructor and extension to arrays
    strcpy(rdtag[nrd], start);
    read[nrd] = fcn;
    mode[nrd] = m;
    nrd++;
    cnt++;

    // advance to next extension in list
    if (end == NULL)
      break;
    start = end + 1;
  }
  return cnt;
}


//= Associate an extension with a particular video sink class factory.

int jhcVidReg::RegWriter (jhcVideoSink *fcn (const char *f), const char *exts)
{
  char list[200];
  char *end, *start = list;
  int cnt = 0;

  // do some sanity checking
  chk_lists();
  if ((nwr >= JVREG_MAX) || (fcn == NULL) || (exts == NULL) || (*exts == '\0'))
    return 0;
  strcpy(list, exts);

  while (nwr < JVREG_MAX)
  {
    // pull next extension out of list
    while (*start == ' ')
      start++;
    if (*start == '\0')
      break;
    end = strchr(start, ' ');
    if (end != NULL)
      *end = '\0';
    
    // add constructor and extension to arrays
    strcpy(wrtag[nwr], start);
    write[nwr] = fcn;
    nwr++;
    cnt++;

    // advance to next extension in list
    if (end == NULL)
      break;
    start = end + 1;
  }
  return cnt;
}


//= Checks that reader and writer counts make sense.
// since there is no constructor called, assumes all member variables start at zero

void jhcVidReg::chk_lists ()
{
  int i, r = nrd, w = nwr;

  // assume bad
  nrd = 0;
  nwr = 0;
  
  // test against limits and verify function and tag arrays
  if ((r < 0) || (w < 0) || (r > JVREG_MAX) || (w > JVREG_MAX))
    return;
  for (i = 0; i < r; i++)
  {
    if (read[i] == NULL)
      return;
    if (rdtag[i][0] == '\0')
      return;
    if ((mode[i] < 0) || (mode[i] > 2))
      return;
  }
  for (i = 0; i < w; i++)
  {
    if (write[i] == NULL)
      return;
    if (wrtag[i][0] == '\0')
      return;
  }
         
  // restore original values
  nrd = r;
  nwr = w;
}


//= Given an extension or filename with extension, returns an appropriate reader.
// if hint is given, then looks for reader based on this extension instead
// if extension not known, then returns NULL

jhcVideoSrc *jhcVidReg::Reader (const char *fname, int index, const char *hint)
{
  const char *key = fname;
  int i;

  if ((hint != NULL) && (*hint != '\0'))
    key = hint;
  if ((i = find_index(key, 0)) < 0)
    return NULL;
  return (read[i])(fname, index);
}


//= Given an extension or filename with extension, returns an appropriate writer.
// if hint is given, then looks for reader based on this extension instead
// if extension not known, then returns NULL

jhcVideoSink *jhcVidReg::Writer (const char *fname, const char *hint)
{
  const char *key = fname;
  int i;

  if ((hint != NULL) && (*hint != '\0'))
    key = hint;
  if ((i = find_index(key, 1)) < 0)
    return NULL;
  return (write[i])(fname);  
}


//= Given a filename find the associated internal array index.
// returns -1 if no match

int jhcVidReg::find_index (const char *fname, int wr)
{
  jhcVideoSrc *trial;
  jhcName name;
  char ext[20];
  int i, ok;

  // get just basic extension
  name.ParseName(fname);
  strcpy(ext, name.Kind());
  i = (int) strlen(ext) - 1;
  if ((i > 0) && (ext[i] == '+'))
    ext[i] = '\0';

  // check registered writer extensions
  if (wr > 0)
  {
    for (i = 0; i < nwr; i++)
      if (_stricmp(ext, wrtag[i]) == 0)
        return i;
    return -1;
  }

  // limit selection to URL capable types only (or cameras)
  if (name.Remote())
  {
    for (i = 0; i < nrd; i++)
      if ((_stricmp(ext, rdtag[i]) == 0) && ((mode[i] == 1) || (mode[i] == 2)))
        return i;
    return -1;
  }

  // check registered reader extensions for files first 
  // added special drop-through test for AVI MPEG-4 VKI under Windows 7
  for (i = 0; i < nrd; i++)
    if ((_stricmp(ext, rdtag[i]) == 0) && ((mode[i] == 0) || (mode[i] == 2)))
    {
      if (mode[i] == 2)                             // cameras (e.g. selection)
        return i;
      if ((trial = (read[i])(fname, 0)) != NULL)    // AVI uncompressed & MPEG-4
      {
        ok = trial->Status();
        delete trial; 
        if (ok > 0)
          return i;
      }
    }

  // check registered reader extensions for URLs if no file reader found
  for (i = 0; i < nrd; i++)
    if ((_stricmp(ext, rdtag[i]) == 0) && (mode[i] == 1))
      return i;

  // if unknown then try WMV (generic DirectShow)
//  for (i = 0; i < nrd; i++)
//    if ((_stricmp("wmv", rdtag[i]) == 0) && (mode[i] == 1))
//      return i;  
  return -1;
}

