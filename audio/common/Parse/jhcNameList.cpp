// jhcNameList.cpp : simple expansion of nickname to full name
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#include "Parse/jhcNameList.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcNameList::~jhcNameList ()
{

}


//= Default constructor initializes certain values.

jhcNameList::jhcNameList ()
{
  nn = 0;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Load a list of full names.
// returns number added

int jhcNameList::Load (const char *fname, int append)
{
  char line[200];
  const char *sep;
  FILE *in;
  int i, end, nn0;

  // possibly purge old list then open specified file
  if (append <= 0)
    nn = 0;
  if (fopen_s(&in, fname, "r") != 0)
    return 0;
  nn0 = nn;

  // read successive non-blank lines without comment markers
  while (fgets(line, 200, in) != NULL)
  {
    // check for comment
    if ((line[0] == '\\') && (line[1] == '\\'))
      continue;

    // look for space separating first name from other parts
    if ((sep = strchr(line, ' ')) == NULL)
    {
      strcpy_s(first[nn], line);
      strcpy_s(full[nn], first[nn]);
      nn++;
      continue;
    }
    i = (int)(sep - line);
    strncpy_s(first[nn], line, i);
    strcpy_s(full[nn], first[nn]);

    // copy last name (discard whitespace at end) 
    end = i - 1;
    full[nn][i++] = ' ';
    while (line[i] != '\0')
    {
      full[nn][i] = line[i];
      if ((line[i] != ' ') && (line[i] != '\n'))
        end = i;
      i++;
    }
    full[nn][end + 1] = '\0';
    nn++;
  }

  // cleanup 
  fclose(in);
  return(nn - nn0);
}


//= Gets normalized name if in list of VIPs.
// returns canonical version if found, else NULL if missing

const char *jhcNameList::Canonical (const char *name) const
{
  int i;
 
  if (name == NULL)
    return NULL;
  for (i = 0; i < nn; i++)
    if (_stricmp(name, full[i]) == 0)
      return full[i];
  return NULL;
}


//= Given first name find first full name in list that matches.
// returns NULL if no plausible expansion 

const char *jhcNameList::LongName (const char *given) const
{
  int i;

  if (first == NULL)
    return NULL;
  for (i = 0; i < nn; i++)
    if (_stricmp(given, first[i]) == 0)
      return full[i];
  return NULL;
}

