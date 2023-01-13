// jhcGenParse.cpp : interface class specifying typical parser functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2019 IBM Corporation
// Copyright 2022 Etaoin Systems
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

#include "Parse/jhcGenParse.h"


///////////////////////////////////////////////////////////////////////////
//                          Debugging Functions                          //
///////////////////////////////////////////////////////////////////////////

//= Show trees, etc. from most recently parsed sentence
// Note: used to include PrintInput also

void jhcGenParse::PrintResult (int lvl, int close)
{
  int i, n = NumTrees(), t = Selected();

  // LVL 0: possibly remain silent
  if ((lvl <= 0) || (n <= 0))
    return;

  // LVL 1: show slot value pairs
  PrintSlots(0, close);
  jprintf("\n");
  if (lvl <= 1)
    return;

  // LVL 2: show only chosen tree
//  jprintf("%d parse states\n\n", NumStates());
  jprintf("Tree %d:    <== CHOSEN out of %d\n", t, n);
  PrintTree();
  if (lvl <= 2)
    return;

  // LVL 3: show trees for multiple interpretations
  for (i = 0; i < n; i++)
    if (i != t)
    {
      jprintf("Tree %d:\n", i);
      PickTree(i);
      PrintTree();
    }
  PickTree(t);
}


//= Show final input string received by system.
// sep: 0 = no separator, 1 = just a line, 2 = number of parses
// allows diarization by pre-pending name of user (utag)

void jhcGenParse::PrintInput (const char *utag, int sep)
{
  int n = NumTrees();

  // possibly print separator line
  if ((sep >= 2) && (n > 1))
    jprintf("\n============= Ambiguous: %d possible parses! ==============\n\n", n);
  else if ((sep == 1) && (n <= 0))
    jprintf("\n====================== No parses! ========================\n\n");
  else if (sep == 1)
    jprintf("\n==========================================================\n\n");

  // print input, possibly tagged with user name
  if ((utag == NULL) || (*utag == '\0'))
    jprintf("\"%s\"\n", Input());
  else
    jprintf("%s: \"%s\"\n", utag, Input());
  if (sep > 0)
    jprintf("\n");
}


//= Prints outs out values for slots and full text of what was heard.
// if close > 0 then prints ending delimiters for phrases begun by !, $, and %
// Note: used to include PrintInput as well

void jhcGenParse::PrintSlots (int sc, int close) 
{
  char alist[1000] = "";

  AssocList(alist, close);  
  NoTabs(alist);
  if (sc > 0)
    jprintf(" [%d]", Confidence());
  jprintf("  -->%s\n", alist);
}


//= Replace all tabs in association list with spaces.
// NOTE: alters input string in place

char *jhcGenParse::NoTabs (char *alist) const
{
  char *c = alist;

  if (strchr(alist, '\t') != NULL)     // not already converted
    while (*c != '\0')
    {
      if (*c == ' ')
        *c = '_';
      else if (*c == '\t')
        *c = ' ';
      c++;
    }
  return alist;
}


//= Print out current top node of tree and full text of what was heard.

void jhcGenParse::PrintParse (int sc) 
{
  char nonterm[80] = "none";

  strcpy_s(nonterm, TopCat());
  if (sc > 0)
    jprintf(" [%d]", Confidence()); 
  jprintf("%s\t <- \"%s\"\n\n", nonterm, Input());
}



