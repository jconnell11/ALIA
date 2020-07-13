// jhcTxtLine.cpp : utilities for chopping apart a line from a file
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2018 IBM Corporation
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

#include "Parse/jhcTxtLine.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcTxtLine::~jhcTxtLine ()
{
  Close();
}


//= Default constructor initializes certain values.

jhcTxtLine::jhcTxtLine ()
{
  init();
}


//= Clear all state variables.

void jhcTxtLine::init ()
{
  in = NULL;
  *line = '\0';
  *token = '\0';
  head = NULL;
}


//= Connect to a particular file for reading.

bool jhcTxtLine::Open (const char *fname)
{
  Close();
  read = 0;
  return(fopen_s(&in, fname, "r") == 0);
}


//= Close the currently connected file (if any).

void jhcTxtLine::Close ()
{
  if (in != NULL)
    fclose(in);
  init();
}


///////////////////////////////////////////////////////////////////////////
//                            Line Information                           //
///////////////////////////////////////////////////////////////////////////

//= Tells if beginning of line looks like a tag (ends in a semicolon).
// also looks for "---", assumes leading whitespace has been stripped

bool jhcTxtLine::TagLine () const
{
  const char *c = head;
  char last = ' ';
  
  if (Blank())
    return false;
  if (strncmp(head, "---", 3) == 0)
    return true;
  while (*c != '\0')
  {
    if (strchr(" \t", *c) != NULL)
      break;
    last = *c++;
  }
  return(last == ':');
}


///////////////////////////////////////////////////////////////////////////
//                             Main Functions                            //
///////////////////////////////////////////////////////////////////////////

//= Get next non-blank line from file (could be current one).
// useful for finding the start of meaningful content
// returns pointer to begining of characters, NULL if end of file

const char *jhcTxtLine::NextContent ()
{
  if (Next() != NULL)
    while (Blank())
      if (Next(1) == NULL)
        break;
  return head;
}


//= Get next blank line from file (could be current one).
// useful for skipping over mis-formatted contetn

const char *jhcTxtLine::NextBlank ()
{
  if (Next() != NULL)
    while (!Blank())
      if (Next(1) == NULL)
        break;
  return head;
}


//= Get cleaned-up next line from file into string.
// ignores comment-only lines, strips leading whitespace & trailing comments
// blank lines (possibly stop markers) are strings with zero length
// continues using rest of previous line unless force > 0
// returns pointer to begining of characters, NULL if end of file

const char *jhcTxtLine::Next (int force)
{
  char *end, *end2;
  int last;

  // file must be open
  if (in == NULL)
    return NULL;

  // possibly continue with rest of current line
  if ((force <= 0) && (head != NULL))
    return head;
  *line = '\0';
  head = NULL;

  // keep reading file until non-empty line
  while (fgets(line, 200, in) != NULL)
  {
    // strip any final carriage return
    read++;
    head = line;
    if ((last = (int) strlen(line) - 1) >= 0)
      if (line[last] == '\n')
        line[last] = '\0';

    // find comment start (if any) and terminate earlier
    if ((end = strchr(line, ';')) != NULL)
      *end = '\0';
    if ((end2 = strstr(line, "//")) != NULL)
      *end2 = '\0';
    strip_wh();              // trim front of remainder

    // skip over full line comments, but return true blank lines
    if (*head == '\0')
      if ((end != NULL) || (end2 != NULL))
        continue;
    return head;
  }

  // no more lines
  *line = '\0';
  head = NULL;
  return NULL;
}


//= Trim N chara characters off the front of the current line.
// returns pointer to remainder of string (possibly empty)

const char *jhcTxtLine::Skip (int n)
{
  int len;

  if (bad_ln())
    return NULL;
  len = (int) strlen(head);
  head += __max(0, __min(n, len));
  return head;
}


//= Strip off leading white space from string.
// returns pointer to remainder (possibly empty)

char *jhcTxtLine::strip_wh ()
{
  while (*head != '\0')
  {
    if (strchr(" \t", *head) == NULL)
      break;
    head++;
  }
  return head;
}


//= Extract front part of string up until next whitepace.
// returns pointer to word found, NULL if nothing found
// NOTE: internal result string re-used, copy if Token called again

char *jhcTxtLine::Token ()
{
  char *end;

  // check that file is open
  if (bad_ln())
    return NULL;

  // find valid character then advance to first white space 
  end = strip_wh();
  if (*end == '\0')
    return NULL;
  while (*end != '\0')
  {
    if (strchr(" \t", *end) != NULL)
      break;
    end++;
  }

  // copy to tken and leave string pointer at next token
  strncpy_s(token, head, end - head);                      // always adds terminator
  head = end;
  strip_wh();
  return token;
}
