// jhcTxtSrc.h : extracts tokenized words from a file or string
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

#ifndef _JHCTXTSRC_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTXTSRC_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdio.h>


//= Extracts tokenized words from a file or string.

class jhcTxtSrc
{
// PRIVATE MEMBER VARIABLES
private:
  // file input
  FILE *in;                  /** File handle for reading words.  */
  long f0;                   /** Starting location in file.      */
  int shared;                /** File pointer from Bind funtion. */
  UC8 saved, saved2;         /** Two character lookahead buffer. */

  // string input
  char sent[500];            /** Cached full input string.       */
  const char *s0;            /** Full string for reading words.  */
  const char *s;             /** Current read point in string.   */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTxtSrc ();
  ~jhcTxtSrc ();
  int Open (const char *fname);
  int Bind (FILE *src);
  int Bind (const char *src); 
  int SetSource (const char *txt);
  const char *Raw () const {return sent;}
  int Close ();

  // main functions
  int Rewind ();
  int ReadWord (char *token, int punc =1, int ssz =80);
  bool Punctuation (const char *txt) const;

  // main functions - convenience
  template <size_t ssz> 
    char *ReadWord (char (&token)[ssz], int punc =1) const 
      {return ReadWord(token, punc, ssz);}

  // sentence functions
  char *Source (char *dest, int punc =1, int ssz =500);
  char *Span (char *frag, int w0, int wn, int punc =1, int ssz =200);
  int Count (int punc =1);

  // sentence functions - convenience
  template <size_t ssz> 
    char *Source (char (&dest)[ssz], int punc =1) const 
      {return Source(dest, punc, ssz);}
  template <size_t ssz> 
    char *Span (char (&frag)[ssz], int w0, int w1, int punc =1) const 
      {return Span(frag, w0, w1, punc, ssz);}


// PRIVATE MEMBER FUNCTIONS
private:
  // main functions
  int get_token (char *token, int ssz);

  // word and sentence detection
  int trim_wh ();
  bool read_end (UC8 &c);
  bool has_punc (char *token);
  bool pmark (char c) const;

  // low level ingest
  UC8 read_c ();
  void push_c (UC8 c);
  UC8 peek_c ();

};


#endif  // once




