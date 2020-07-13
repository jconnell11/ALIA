// jhcGenParse.h : interface class specifying typical parser functions
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2019 IBM Corporation
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

#ifndef _JHCGENPARSE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGENPARSE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"


//= Interface class specifying speech-like input and output.
// covers main functions in jhcSpeechX and jhcGramExec classes

class jhcGenParse
{
// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  virtual int PrintCfg () =0;

  // parsing 
  virtual void SetGrammar (const char *fname, ...) =0;
  virtual void ClearGrammar (int keep =1) =0;
  virtual int LoadGrammar (const char *fname, ...) =0;
  virtual const char *GrammarFile () const  =0;
  virtual int MarkRule (const char *name =NULL, int val =1) =0;
  virtual int ExtendRule (const char *name, const char *phrase) =0;

  // parsing status functions
  virtual int NumStates () const {return 0;}
  virtual int NumTrees () const  {return 1;}
  virtual int PickTree (int n)   {return 0;}
  virtual int Selected () const  {return 0;}

  // parsing results
  virtual int Confidence () const {return 100;}
  virtual const char *Input () const =0;
  virtual const char *Clean () const =0;
  virtual const char *Root () =0;
  virtual const char *TopCat () =0;
  virtual char *AssocList (char *alist, int close, int ssz) =0;

  // parse results - convenience
  template <size_t ssz> 
    char *AssocList (char (&alist)[ssz], int close =0) 
      {return AssocList(alist, close, ssz);}

  // debugging
  virtual void PrintTree (int top =1) =0;
  void PrintResult (int lvl =2, int close =0);
  void PrintInput (const char *utag =NULL, int sep =1);
  void PrintSlots (int sc =0, int close =0);
  void PrintParse (int sc =0);


};


#endif  // once




