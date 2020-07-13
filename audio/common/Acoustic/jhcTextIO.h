// jhcTextIO.h : provides speech-like text input and output
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

#ifndef _JHCTEXTIO_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTEXTIO_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Acoustic/jhcGenIO.h"       // common audio
#include "Parse/jhcGramExec.h"

#include "Data/jhcParam.h"           // common video


//= Provides speech-like text input and output.
// supports main functions in jhcSpeechX class

class jhcTextIO : public jhcGramExec, public jhcGenIO
{
// PRIVATE MEMBER VARIABLES
private:
  // general status
  char ifile[200];
  unsigned int last, now;               
  int disable;

  // console input
  char line[500], raw[500], utt[500]; 
  char *fill; 
  int pend, hear, quit;

  // output messages
  char qtext[500], msg[500];    
  char *more;
  unsigned int tprev;
  int tlast, tlock;


// PUBLIC MEMBER VARIABLES
public:
  // message output parameters
  jhcParam tps;
  double firm;
  int blurt, ims, ivar, mms, mvar, barge;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTextIO ();
  ~jhcTextIO ();
  void Reset ();
  int Init (int dbg =0, int noisy =0);
  void Cripple (int doit =1) {disable = doit;}
  int Ready () const {return((NumRules() > 0) ? 1 : 0);}
  bool Escape ();

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;
  int FirstName (char *name, int ssz);
  int LastName (char *name, int ssz);

  // processing parameters - convenience
  template <size_t ssz>
    int FirstName (char (&name)[ssz])
      {return FirstName(name, ssz);}
  template <size_t ssz>
    int LastName (char (&name)[ssz])
      {return LastName(name, ssz);}

  // top level convenience functions and status
  void Inject (const char *txt, int stop =0);
  int Update (int reco =1, int prolong =0);
  void Issue ();
  int Hearing () const {return hear;}
  int Talking () const {return((emit != NULL) ? 1 : 0);}
  double Silence () const;
  void ClrTimer () {last = 0;}
  const char *Heard () const {return utt;}

  // output messages
  bool Instant () const {return(disable > 0);}
  int Say (const char *msg, ...);
  int Say (int bid, const char *msg, ...);
  int Utter ();
  int Finish (double secs =10.0);
  int ChkOutput () {return Talking();}
  void ShutUp ();
  const char *Said () const {return msg;}


// PRIVATE MEMBER FUNCTIONS
private:
  // configuration
  int text_params (const char *fname);

  // console input
  int listen ();

  // output messages
  void dribble ();


};


#endif  // once




