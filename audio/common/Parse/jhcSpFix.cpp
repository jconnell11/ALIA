// jhcSpFix.cpp : text substitution utilities for speech reco and TTS
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2024 Etaoin Systems
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

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "Parse/jhcSpFix.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Ingest file containing raw speech recognition fixes.
// can optionally supply path instead of full file name
// typical "misheard.map" has format:
// <pre>
// = correct-form-a
//   mistake-a1
//   mistake-a2
//
// = correct-form-b
//   mistake-b1
// </pre>
// returns number of input fixes

int jhcSpFix::LoadFix (const char *fname, int path)
{
  FILE *in = map_file(fname, path, "misheard");
  char line[80], canon[80] = "";
  int n;

  nfix = 0;
  if (in == NULL)
    return 0;
  while ((fgets(line, 80, in) != NULL) && (nfix < fmax))
  {   
    if (line[0] == '=')     
      trim_wh(canon, 80, line + 1);
    else if (*canon != '\0')
      if ((n = trim_wh(reco[nfix], 40, line)) > 0)
      {
        strcpy_s(subst[nfix], canon);
        rlen[nfix++] = n;
      }
  }
  fclose(in);  
  return nfix;  
}


//= Ingest file containing phonetic re-spellings for words.
// can optionally just use path of supplied file name
// typical "pronounce.map" has format:
// <pre>
//   word re-spell-ing
// </pre>
// returns number of pronunciation respellings

int jhcSpFix::LoadPron (const char *fname, int path)
{
  FILE *in = map_file(fname, path, "pronounce");
  char line[80];
  char *tail;

  nsp = 0;
  if (in == NULL)
    return 0;
  while ((fgets(line, 80, in) != NULL) && (nsp < pmax))
    if ((tail = strpbrk(line, " \t")) != NULL)
    {
      // split line and do sscanf_s twice (for Linux port)
      *tail = '\0';
      if (sscanf_s(line, "%s", word[nsp], 40) == 1)
        if (sscanf_s(tail + 1, "%s", pron[nsp], 40) == 1)
        {
          wlen[nsp] = (int) strlen(word[nsp]);
          nsp++;
        }
    }
  fclose(in);
  return nsp; 
}


//= Try opening configuration file for reading.
// uses "fname" as either a path prefix or a complete file name

FILE *jhcSpFix::map_file (const char *fname, int path, const char *def) const
{
  FILE *in;
  char full[200];
  const char *cfg = full;
  int n = 0;

  // determine configuration file name
  if (fname != NULL)
    n = (int) strlen(fname);
  if (n <= 0)
    sprintf_s(full, "config/%s.map", def);
  else if (path <= 0)
    cfg = fname;
  else if (fname[n - 1] != '/')
    sprintf_s(full, "%s/config/%s.map", fname, def);
  else
    sprintf_s(full, "%sconfig/%s.map", fname, def);

  // try opening it
  if (fopen_s(&in, cfg, "r") == 0)
    return in;
  return NULL;
}


//= Trim off leading and trailing whitespace from raw string.
// assumes "nice" is "ssz" long which is big enough to hold all of "raw"
// returns length of trimmed string

int jhcSpFix::trim_wh (char *nice, int ssz, const char *raw) const
{
  const char *start = raw;
  char *trim;

  // strip leading spaces
  *nice = '\0';
  while (*start == ' ')
    start++;
  if ((*start == '\0') || (strchr("\n\r", *start) != NULL))
    return 0;
  strcpy_s(nice, ssz, start);

  // strip trailing spaces
  trim = nice + strlen(nice);
  while (--trim >= nice)
    if (strchr(" \n\r", *trim) == NULL)
      break; 
  trim++;
  *trim = '\0';
  return (int)(trim - nice);
}


///////////////////////////////////////////////////////////////////////////
//                            Main Functions                             //
///////////////////////////////////////////////////////////////////////////

//= Replaces matches from "k" array in "in" with text from "s" array. 
// arrays have "n" terms, and the the length of k[i] = cnt[i] 
// "out" holds fixed version and has size "ssz" 
// returns number of replacements made

int jhcSpFix::replace (char *out, int ssz, const char *in,
                       const char k[][40], const int cnt[], 
                       const char s[][40], int n) const
{
  const char *src = in;
  int i, chg = 0, fill = 0;

  // sanity check
  if ((out == NULL) || (ssz <= 0))
    return 0;
  *out = '\0';
  if ((in == NULL) || (*in == '\0'))
    return 0;

  // simplest case
  if (n <= 0)
  {
    sprintf_s(out, ssz, "%s", in);
    return 0;
  }

  // scan through word starts in input string
  while ((*src != '\0') && (fill < ssz))
  {
    // look for a key that matches current head 
    for (i = 0; i < n; i++)
      if (_strnicmp(src, k[i], cnt[i]) == 0)
        if (!isalnum(src[cnt[i]]))
          break;

    // if key found then copy substitution
    if (i < n)
    {
      sprintf_s(out + fill, ssz - fill, "%s", s[i]);
      fill += (int) strlen(s[i]);
      src += cnt[i];
      chg++; 
    } 
    
    // advance to next word start
    while (isalnum(*src) && (fill < ssz))
      out[fill++] = *src++;           
    while ((*src != '\0') && !isalnum(*src) && (fill < ssz))
      out[fill++] = *src++; 
  }

  // make sure final string is terminated
  if (fill >= ssz)
    fill = ssz - 1;
  out[fill] = '\0';
  return chg;
}
