// jhcTxtLine.h : utilities for chopping apart a line from a file
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2018 IBM Corporation
// Copyright 2021-2023 Etaoin Systems
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

#ifndef _JHCTXTLINE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTXTLINE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>               // needed for FILE
#include <string.h>


//= Utilities for getting and chopping apart a line from a file.

class jhcTxtLine
{
// PRIVATE MEMBER VARIABLES
private:
  FILE *in;
  char line[200], token[80];
  char *head;
  int read;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcTxtLine ();
  jhcTxtLine ();
  bool Open (const char *fname);
  int Close ();
  int Last () const {return read;}
  bool End () const 
    {return((in == NULL) || (feof(in) != 0));}
  bool Error () const 
    {return((in == NULL) || (ferror(in) != 0));}

  // line information
  const char *Head () const 
    {return((bad_ln()) ? NULL : head);}
  bool Blank () const
    {return(bad_ln() || (*head == '\0'));}
  bool First (const char *opts) const 
    {return(!Blank() && (strchr(opts, *head) != NULL));}
  bool Begins (const char *pre) const
    {return(!bad_ln() && (strncmp(head, pre, (int) strlen(pre)) == 0));}
  bool TagLine () const;

  // main functions
  const char *NextContent ();
  const char *NextBlank ();
  const char *Next (int force =0);
  const char *Skip (int n =1);
  const char *Skip (const char *head, int extra =0)
    {return Skip((int) strlen(head) + extra);}
  const char *Clean () 
    {return((bad_ln()) ? NULL : strip_wh());}
  int Flush (int ret =1) 
    {*line = '\0'; head = NULL; return ret;};
  char *Token (int under =0);
  char *Token (char *txt, int under, int ssz);
  template <size_t ssz>
    char *Token (char (&txt)[ssz], int under =0)           // for convenience
      {return Token(txt, under, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void init ();

  // main functions
  bool bad_ln () const 
    {return((in == NULL) || (head == NULL));}
  char *strip_wh ();


};


#endif  // once




