// jhcTxtSrc.cpp : extracts tokenized words from a file or string
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2013-2017 IBM Corporation
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

#include "Parse/jhcTxtSrc.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcTxtSrc::jhcTxtSrc ()
{
  in = NULL;
  *sent = '\0';
  Close();
}


//= Default destructor does necessary cleanup.

jhcTxtSrc::~jhcTxtSrc ()
{
  Close();
}


//= Open a new file and setup to extract words from this.
// returns 1 if ok, 0 or negative for error

int jhcTxtSrc::Open (const char *fname)
{
  if (fname == NULL)
    return -1;
  Close();
  if (fopen_s(&in, fname, "r") != 0)
    return 0;
  return 1;
}


//= Extract more words from an already opened file source.
// NOTE: retains pointer to file so make sure it persists
// returns 1 if ok, 0 or negative for error

int jhcTxtSrc::Bind (FILE *src)
{
  if (src == NULL)
    return 0;
  Close();
  in = src;
  f0 = ftell(in);
  shared = 1;
  return 1;
}


//= Extract words from a text string (consider SetSource instead).
// NOTE: retains pointer to string, so make sure it persists
// returns 1 if ok, 0 or negative for error

int jhcTxtSrc::Bind (const char *txt)
{
  if (txt == NULL)
    return 0;
  Close();
  s0 = txt;
  s = txt;
  return 1;
}


//= Copies given string (for persistence) then extracts words.
// returns 1 if ok, 0 or negative for error

int jhcTxtSrc::SetSource (const char *txt)
{
  if (txt == NULL)
    return 0;
  strcpy_s(sent, txt);
  return Bind(sent);
}


//= Close any open word source.
// returns -1 for convenience

int jhcTxtSrc::Close ()
{
  // close any file that this class opened
  if ((in != NULL) && (shared <= 0))
    fclose(in);

  // no file bound
  in = NULL;
  f0 = 0; 
  shared = 0; 
  saved = '\0';
  saved2 = '\0';

  // no string bound
  s0 = NULL;
  s = NULL; 
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Rewind to initial point in current word source.
// returns 1 if ok, 0 or negative for error

int jhcTxtSrc::Rewind ()
{
  // start at beginning of string again
  if ((s != NULL) && (s0 != NULL))
  {
    s = s0;
    return 1;
  }

  // start at original point of file again
  if (in != NULL)
  {
    fseek(in, f0, SEEK_SET);
    saved = '\0';
    saved2 = '\0';
    return 1;
  }
  return 0;
}


//= Extracts next token from source string or file.
// end of sentence punctuation (. or ! or ? or ; or :) treated as token
// multiple blank lines are aggregated and saved as a single '\n' token
// both string pointer and file pointer have valid next read state at end
// can ignore punctuation tokens if punc <= 0
// returns 2 if word, 1 if end of sentence, 0 if blank lines, -1 for end of input

int jhcTxtSrc::ReadWord (char *token, int punc, int ssz)
{
  int rc;

  if (token == NULL)
    return -2;
  rc = get_token(token, ssz);
  if (punc <= 0)
    while (Punctuation(token))
      if ((rc = get_token(token, ssz)) < 0)
        break;
  return rc;
}


//= Reads next token, either word or punctuation, from file or string.
// returns 2 if word, 1 if end of sentence, 0 if blank lines, -1 for end of input

int jhcTxtSrc::get_token (char *token, int ssz)
{
  UC8 c;
  int rc, n = 0;

  // look for start of next token
  *token = '\0';
  if ((rc = trim_wh()) <= 0)
    return rc;

  // copy token characters (respect txt max size)
  while (n < (ssz - 1))
  {
    // see if white space delimiter (or end of string)
    if (read_end(c))
      return 2;

    // add character to current token and terminate partial string
    token[n++] = c;
    token[n] = '\0';
    if ((n == 1) && (token[0] == '$'))    // separate off dollar sign
      return 2;

    // check for sentence delimiter (punctuation)
    if (has_punc(token))
    {
      // single punctuation string
      if (n == 1)
      {
        if ((c == '.') && (peek_c() == '.'))    // ellipsis (part 1)
          continue;
        if (strchr(".!?", c) != NULL)
          return 1;
        return 2;
      }

      // save punctuation for later
      token[--n] = '\0';
      push_c(c);        
      return 2;
    }
  }
  return 1;
}


//= Tell whether string corresponds to a single punctuation mark.

bool jhcTxtSrc::Punctuation (const char *txt) const
{
  if ((int) strlen(txt) != 1)
    return false;
  return pmark(*txt);
}


///////////////////////////////////////////////////////////////////////////
//                            Sentence Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Reconstitute source with or without punctuation from original input.
// assumes input is a single sentence
// returns pointer to sentence, NULL for error

char *jhcTxtSrc::Source (char *dest, int punc, int ssz)
{
  char *start = dest;
  int n, lim = ssz;

  // check argument and state
  if (dest == NULL) 
    return NULL;
  if (Rewind() <= 0)
    return NULL;

  // keep tacking words onto end of string
  while (ReadWord(start, punc, lim) > 0)
  {
    // insert next word after adding a space
    n = (int) strlen(start);
    if (n >= (lim - 2))
      break;
    start[n++] = ' ';
    start[n]   = '\0';
    start += n;
    lim -= n;
  }

  // erase last space (and make sure null terminated)
  n = (int) strlen(dest);
  if (n > 0)
    dest[n - 1] = '\0';
  return dest;
}


//= Reconstitutes a particular set of words (w0 to wn inclusive) from input.
// assumes input is a single sentence
// returns pointer to string, NULL if too few elements or error

char *jhcTxtSrc::Span (char *frag, int w0, int wn, int punc, int ssz) 
{
  char *start = frag;
  int n, lim = ssz, cnt = 0;

  // check argument and state
  if ((frag == NULL) || (w0 < 0))
    return NULL;
  if (Rewind() <= 0)
    return NULL;

  // start adding to output string when w0 encountered
  while (ReadWord(start, punc, lim) > 0)
    if (cnt++ >= w0)             
    {
      // stop after wn added
      if (cnt > wn)
        return frag;

      // insert next word after adding a space
      n = (int) strlen(start);
      if (n >= (lim - 2))
        break;        
      start[n++] = ' ';
      start[n]   = '\0';
      start += n;
      lim -= n;
    }
  return NULL;
}


//= Tell how many tokens can be read from source.
// assumes input is a single sentence
// returns negative for error

int jhcTxtSrc::Count (int punc)
{
  char token[80];
  int cnt = 0;

  if (Rewind() <= 0)
    return -1;
  while (ReadWord(token, punc) > 0)
    cnt++;
  return cnt;
}


///////////////////////////////////////////////////////////////////////////
//                     Word and Sentence Detection                       //
///////////////////////////////////////////////////////////////////////////

//= Remove leading spaces, tabs, and lines.
// returns 1 if okay, 0 if blank lines, -1 for end of input

int jhcTxtSrc::trim_wh ()
{
  UC8 c;
  int ln = 0;

  // strip leading whitespace
  while (1)
  {
    c = read_c();
    if ((c == '\0') || (strchr(" \t\n", c) == NULL))
      break;
    if (c == '\n')      
      ln++;
  }

  // restore non-whitespace character then check for early quit
  if (c == '\0')
    return -1;
  push_c(c); 
  if (ln >= 2)
    return 0;
  return 1;
}


//= Read next character and check if end of word has been found.

bool jhcTxtSrc::read_end (UC8 &c) 
{
  int dash = 0;           // whether dashes are ever break points

  // get character but return if not possible end condition
  c = read_c();
  if (c == '\0')
    return true;

  // get rid of hyphens in words but not minuses or ranges
  if (c == '-')
    if ((dash <= 0) || (isdigit(peek_c()) != 0))
      return false;

  // save potential paragraph marker
  if (strchr(" \t\n", c) == NULL)
    return false;   
  if (c == '\n')          
    push_c(c);
  return true;
}


//= See if punctuation mark has been found at end of current word.
// special cases for numbers and abbreviations

bool jhcTxtSrc::has_punc (char *token) 
{
  char abbrev[19][20] = {"Mr.", "Mrs.", "Ms.", "Dr.", 
                         "fig.", "figs.", "ex.", "eq.", "eqn.", "tab.",
                         "i.e.", "e.g.", "ie.", "eg.", "cf.", "al.", "cont.",
                         "..", "..."};            // ellipsis (part 2)
                         
  int i, n = (int) strlen(token);
  UC8 c2, c = token[n - 1];

  // return if not some standard punctuation mark
  if (!pmark(c))
    return false;

  // see if a number follows a period (aka decimal point)
  if (strchr(",.", c) != NULL)
  {
    c2 = peek_c();
    if ((c2 != '\0') && isdigit(c2))
      return false;
  }

  // check for a single letter followed by a period (C. Elegans)
  if ((n == 2) && (c == '.') && isalpha(*token))
    return false;

  // check preceding characters for standard abbreviations
  if (c == '.')
    for (i = 0; i < 19; i++)
      if (_stricmp(token, abbrev[i]) == 0)
        return false;
  return true;
}


//= Tell whether given character is from punctuation mark set.

bool jhcTxtSrc::pmark (char c) const
{
  if (strchr(",;:.!?()[]{}\"=/<>%+", c) == NULL)  // must not have * !!!
    return false;
  return true;
}


///////////////////////////////////////////////////////////////////////////
//                             Low Level Ingest                          //
///////////////////////////////////////////////////////////////////////////

//= Get next character from a string or file.
// always '\0' if end of string or file
// integrated with 2 character buffer for file lookahead
// return updated string pointer if valid

UC8 jhcTxtSrc::read_c ()
{
  UC8 c;

  // get next character in string 
  if (s != NULL)
  {
    // advance pointer except at end of string
    c = s[0];
    if (c != '\0')
      s++;
    return c;
  }

  // get next character from file
  if (in != NULL)
  {
    // check if any saved character
    if (saved != '\0')
    {
      c = saved;
      saved = saved2;
      saved2 = '\0';
      return c;
    }

    // get new character and check for end of file
    c = (UC8) fgetc(in);
    if (!feof(in) && !ferror(in))
      return c;
  }
  return '\0';
}


//= Put last character back on string or file.
// integrated with 2 character buffer for file lookahead
// return updated string pointer if valid

void jhcTxtSrc::push_c (UC8 c) 
{
  // never push if at end already
  if (c == '\0')
    return;

  // backup string pointer (worry about front?)
  if (s != NULL)
    if (s > s0)
      s--;

  // avoid ungetc by caching characters locally
  if (in != NULL)
  {               
    if (saved == '\0')
      saved = c;
    else
      saved2 = c;
  }
}


//= Look at next character without removing it from stream.
// integrated with 2 character buffer for file lookahead

UC8 jhcTxtSrc::peek_c ()
{
  // look at next character in string 
  if (s != NULL)
    return s[0];

  // look at next character in file 
  if (in != NULL)
  {               
    if (saved == '\0')
    {
      // read a new character if none cached 
      saved = (UC8) fgetc(in);
      if (feof(in) || ferror(in))
        saved = '\0';
    }
    return saved;
  }
  return '\0';
}

