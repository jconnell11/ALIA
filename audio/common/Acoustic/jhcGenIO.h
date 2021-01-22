// jhcGenIO.h : interface class specifying speech-like input and output
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2019 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#ifndef _JHCGENIO_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCGENIO_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <string.h>
#include <conio.h>


//= Interface class specifying speech-like input and output.
// covers main functions in jhcSpeechX and jhcTextIO classes

class jhcGenIO
{
// PROTECTED MEMBER VARIABLES
protected:
  char user[80];
  const char *rcv, *emit;
  int acc;


// PUBLIC MEMBER FUNCTIONS
public:
  jhcGenIO () {strcpy_s(user, "unknown"); rcv = NULL; emit = NULL; acc = 0;}

  // creation and configuration
  virtual int Init (int dbg =0, int noisy =0) {return 0;}
  virtual void Reset ()                       {}
  virtual int Ready () const                  {return 0;}
  virtual bool Escape ()                      {return((_kbhit() == 0) ? false : true);}  
  virtual bool Accepting () const             {return(acc > 0);}                 

  // processing parameter manipulation 
  virtual int Defaults (const char *fname =NULL)       {return 0;}
  virtual int SaveVals (const char *fname) const       {return 0;}
  virtual int SetUser (const char *name, int build =1) {strcpy_s(user, name); return 1;}
  virtual const char *UserName ()                      {return user;}

  // input status and processing
  virtual void Inject (const char *txt, int stop =0) {}
  virtual int Update (int reco =1, int prolong =0)   {acc = 1; return 0;}
  virtual void Issue ()               {acc = 0;}
  virtual int Hearing () const        {return 0;}
  virtual int Talking () const        {return 0;}
  virtual double Silence () const     {return 0.0;}
  virtual void ClrTimer ()            {}
  virtual const char *Heard () const  {return NULL;}
  virtual const char *LastIn () const {return rcv;}

  // output messages
  virtual bool Instant () const                   {return false;}
  virtual int Say (const char *msg, ...)          {return 0;}
  virtual int Say (int bid, const char *msg, ...) {return 0;}
  virtual int Utter ()                            {return 0;}
  virtual int Finish (double secs =10.0)          {return 1;}
  virtual int ChkOutput ()                        {return 0;}
  virtual void ShutUp ()                          {}
  virtual const char *Said () const               {return NULL;}
  virtual const char *LastOut () const            {return emit;}

};


#endif  // once




