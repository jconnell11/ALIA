// jhcFileList.cpp : get file name strings one line at a time from text file
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013 IBM Corporation
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

// Build.Settings.Link.General must have library winmm.lib (for time)
#pragma comment(lib, "winmm.lib")


#include <windows.h>          // for time
#include <string.h>

#include "Data/jhcFileList.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcFileList::jhcFileList (const char *fname)
{
  in = NULL;
  ListOpen(fname);
}


//= Default destructor does necessary cleanup.

jhcFileList::~jhcFileList ()
{
  ListClose();
}


//= Returns the max string length of filenames minus extensions.
// useful for report generation formatting

int jhcFileList::MaxBase ()
{
  jhcName s;
  const char *src;
  int len, n = 0;
  ListRewind();
  while ((src = ListNext()) != NULL)
  {
    s.ParseName(src);
    len = (int) strlen(s.Base());
    n = __max(n, len);
  }
  ListRewind();
  return n;
}


//= Returns the max string length of filenames including extensions.
// useful for report generation formatting

int jhcFileList::MaxName ()
{
  jhcName s;
  const char *src;
  int len, n = 0;

  ListRewind();
  while ((src = ListNext()) != NULL)
  {
    s.ParseName(src);
    len = (int) strlen(s.Name());
    n = __max(n, len);
  }
  ListRewind();
  return n;
}


//= Convert input name to exactly n characters by padding or truncation.
// can optional peel off a certain number of characters from end of name
// useful for reports, can be used in conjunction with MaxBase()

const char *jhcFileList::PadName (char *dest, const char *src, int n, int peel, int dsz) const
{
  jhcName s;
  const char *tag;
  int i, len, tail = 0, nc = __min(n, dsz - 1);

  // check for real output string
  if (dest == NULL)
    return NULL;

  // check for missing or blank string
  if ((src != NULL) && (*src != '\0'))
  {
    // parse file name then copy most of it
    s.ParseName(src);
    tag = s.Name();
    len = (int) strlen(tag) - peel;
    tail = __min(nc, len);
    for (i = 0; i < tail; i++)
      dest[i] = tag[i];
  }

  // pad with spaces afterward
  for (i = tail; i < nc; i++)
    dest[i] = ' ';
  dest[nc] = '\0';
  return dest;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Open text file for reading names, possibly based on a pattern.
// returns number of names in list, -1 if file error, -2 if no such file

int jhcFileList::ListOpen (const char *fname)
{
  const char *item;
  char full[500];

  // get rid of any previous file and reset state
  ListClose();
  now = 0;
  total = 0;
  t0 = 0;
  tnow = 0;
  
  // try opening given name 
  if (fname == NULL)
    return -1;
  if ((in = fopen(fname, "r")) == NULL)
    return -1;

  // save name of list file and read optional header
  if (read_hdr() <= 0)
    return -1;
  base.ParseName(fname);

  // if first line is not a default specification then unread line
  start = ftell(in);
  if ((item = ListNext()) == NULL)
    return -1;
  if (strchr(item, '*') == NULL)
    fseek(in, start, SEEK_SET);
  else
  {
    // append listing file's directory unless a different disk is mentioned
    start = ftell(in);
    if (strchr(item, ':') != NULL) 
      strcpy(full, item);
    else if ((strstr(item, "//") != NULL) || (strstr(item, "\\\\") != NULL))
      sprintf(full, "%s%s", base.Disk(), item + 1);
    else
      sprintf(full, "%s%s", base.Dir(), item); 
    base.ParseName(full);
    def = 1;
  }

  // determine how many valid names are in file 
  total = 0;
  while (ListNext() != NULL)
    total++;
  ListRewind();
  return total;
}


//= Close current list of file names but retains last state.
// always returns false for convenience

bool jhcFileList::ListClose ()
{
  // close any open list
  if (in != NULL)
    fclose(in);

  // clear out file and entry names
  base.ParseName(NULL);
  entry.ParseName(NULL);
  def = 0;
  return false;
}


//= Go back to first entry in list of file names.

int jhcFileList::ListRewind ()
{
  // go to first line after deafult spec (if any)
  if (in == NULL)
    return 0;
  if (fseek(in, start, SEEK_SET) != 0)
    return 0;

  // nothing read yet, reset timing
  now = 0;
  t0 = 0;
  tnow = 0;
  return 1;
}


//= Retrieve next file name, substituting as needed.
// eliminates leading and trailing whitespace
// allows embedded spaces but alters given string
// copies valid portion to "entry" variable
// increments current name number (starts at 1)
// returns NULL if end of list of names

const char *jhcFileList::ListNext ()
{
  char line[500];
  const char *d, *p, *e;
  char *begin;
  int i = 0;

  while (i == 0)
  {
    // find next non-blank line in file
    if ((in == NULL) || feof(in) || ferror(in))
      return NULL;
    if (fgets(line, 500, in) == NULL) 
      return NULL;

    // prune whitespace from front
    begin = line;
    while (*begin != '\0')
      if (strchr(" \n\r", *begin) != NULL)
        begin++;
      else
        break;

    // prune whitespace from back
    for (i = (int) strlen(begin); i > 0; i--)
      if (strchr(" \n\r", begin[i - 1]) == NULL)
        break;
  }

  // valid portion of line found, so copy to output
  begin[i] = '\0';
  entry.ParseName(begin);
  tnow = timeGetTime();
  if (now++ <= 0)
    t0 = tnow;

  // possibly fill in missing parts of full file name 
  if (def > 0)
  {
    d = entry.Disk();
    p = entry.Path();
    e = entry.Extension();
    sprintf(line, "%s%s%s%s", 
            ((*d == '\0') ? base.Disk() : ""), 
            (((*p == '\0') || ((*p != '\\') && (*p != '/'))) ? base.Path() : ""),
            entry.File(),
            ((*e == '\0') ? base.Extension() : ""));
    entry.ParseName(line);
  }
  return entry.File();
}


///////////////////////////////////////////////////////////////////////////
//                                 Status                                //
///////////////////////////////////////////////////////////////////////////

//= Tells what percetnage of list has been processed so far.

double jhcFileList::ListProgress () const 
{
  if (total <= 0)
    return 100.0;
  return((100.0 * now) / total);
}


//= Estimates time to completion in seconds.
// presumably ListRemaining is called after processing occurs
// t0 records first call to List Next which is followed by processing
// tnow is the current call to List Next, processing not yet done
// thus elapsed time covers only n-1 processing intervals

double jhcFileList::ListRemaining () const
{
  if (now <= 1)
    return 0.0;
  return(ListElapsed() * (total - now) / (double)(now - 1));
}


//= Estimates hours, minutes, and seconds left to convert.
// returns pointer to formated string

const char *jhcFileList::ListTime () 
{
  int hr, min, secs = ROUND(ListRemaining());

  // find hours and minutes
  hr = secs / 3600;
  secs -= 3600 * hr;
  min = secs / 60;
  secs -= 60 * min;

  // compose string
  sprintf(time, "%d:%02d:%02d", hr, min, secs);
  return time;
}


///////////////////////////////////////////////////////////////////////////
//                                 Debugging                             //
///////////////////////////////////////////////////////////////////////////

//= Generate a report file listing all duplicated entries.
// notes line number (starting at 1) for each repetition of name
// can be called with a NULL file to just get count
// returns total number of duplicates (original does not count)
// NOTE: if 0 duplicates then no output file generated

int jhcFileList::ListDups (const char *report)
{
  FILE *out = NULL;
  char **items;
  int *marks;
  int i, j, n0, n = 0;

  // generate arrays to hold all names and report flags
  if (total <= 0)
    return -2;
  if ((marks = new int[total]) == NULL)
    return -2;
  if ((items = new char *[total]) == NULL)
    return -2;
  for (i = 0; i < total; i++)
    if ((items[i] = new char[200]) == NULL)
      return -2;

  // load names into array
  ListRewind();
  for (i = 0; i < total; i++)
  {
    strcpy(items[i], ListNext());
    marks[i] = 0;
  }
  ListRewind();

  // check that report file can be opened and write header
  if ((report != NULL) && ((out = fopen(report, "w")) == NULL))
    return -1;
  if (out != NULL)
    fprintf(out, "Duplicates (with line numbers):\n");

  // scan through each name on list
  for (i = 0; i < total; i++)
    if (marks[i] <= 0)
    {
      // compare to all names in tail of list
      n0 = n;
      for (j = i + 1; j < total; j++)
        if (strcmp(items[i], items[j]) == 0)
        {
          // make output file if none yet
          if ((out == NULL) && (report != NULL))
          {
            if ((out = fopen(report, "w")) == NULL)
              return -1;
            fprintf(out, "Duplicates (with line numbers):\n");
          }

          // report search term when first match found
          if ((n <= n0) && (out != NULL))
            fprintf(out, "\n* %4d = %s\n", i + 1, items[i]);

          // report match and invalidate for later
          if (out != NULL)
            fprintf(out, "  %4d = %s\n", j + 1, items[j]);
          marks[j] = 1;
          n++;
        }
    }

  // report total
  if (out != NULL)
  {
    fprintf(out, "\n%d duplicate names\n", n); 
    fclose(out);
  }

  // clean up arrays
  for (i = total - 1; i >= 0; i--)
    delete items[i];
  delete [] items;
  return n;
}


//= Remove duplicates to give a new list with one mention only.
// generates a new list with full file names (no wildcards)
// if all > 0 then removes BOTH original and later copies
// returns number of items removed, negative for error
// NOTE: always generates new list, even if no duplicates

int jhcFileList::RemDups (const char *clean, int all) 
{
  FILE *out;
  char **items;
  int *marks;
  int i, j, any, n = 0;

  // try opening output file
  if ((clean == NULL) || ((out = fopen(clean, "w")) == NULL))
    return -2;

  // generate arrays to hold all names and report flags
  if (total <= 0)
    return -1;
  if ((marks = new int[total]) == NULL)
    return -1;
  if ((items = new char *[total]) == NULL)
    return -1;
  for (i = 0; i < total; i++)
    if ((items[i] = new char[200]) == NULL)
      return -1;

  // load names into array
  ListRewind();
  for (i = 0; i < total; i++)
  {
    strcpy(items[i], ListNext());
    marks[i] = 0;
  }
  ListRewind();

  // scan through each name on list
  for (i = 0; i < total; i++)
    if (marks[i] <= 0)
    {
      // compare to all names in tail of list
      any = 0;
      for (j = i + 1; j < total; j++)
        if (strcmp(items[i], items[j]) == 0)
        {
          // invalidate all following matches
          marks[j] = 1;
          any = 1;
          n++;
        }

      // if clean then copy to output
      if ((all > 0) && (any > 0))
        n++;
      else
        fprintf(out, "%s\n", items[i]);
    }

  // close file and clean up arrays
  fclose(out);
  for (i = total - 1; i >= 0; i--)
    delete items[i];
  delete [] items;
  return n;
}


//= Find elements of the list which cannot be opened for reading.
// returns the number of problems and lists them in the output file
// NOTE: does not generate file unless problems found

int jhcFileList::ListMiss (const char *report) 
{
  FILE *tst, *out = NULL;
  const char *src;
  int n = 0;

  // go through all entries in list
  ListRewind();
  while ((src = ListNext()) != NULL)
  {
    // try opening file
    if ((tst = fopen(src, "r")) != NULL)
    {
      fclose(tst);
      continue;
    }

    // create a file to list problems in
    if ((out == NULL) && (report != NULL))
      if ((out = fopen(report, "w")) == NULL)
      {
        fclose(in);
        return -1;
      }

    // add entry concerning bad item
    if (out != NULL)
      fprintf(out, "%s\n", src);
    n++;
  }

  // clean up
  if (out != NULL)
  {
    fprintf(out, "\n%d out of %d items missing\n", n, total);
    fclose(out);
  }
  ListRewind();
  return n;
}


