// jhcTripleCom.h : sends and receives semantic triples over a socket
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2015 IBM Corporation
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

#ifndef _JHCTRIPLECOM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTRIPLECOM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Data/jhcParam.h"               // common video
#include "Interface/jhcSocket.h"

#include "Semantic/jhcTripleMem.h"


//= Sends and receives semantic triples over a socket.

class jhcTripleCom : public jhcTripleMem, private jhcSocket
{
// PRIVATE MEMBER VARIABLES
private:
  char b[200];    /** Buffer for holding entire next message. **/
  int fill;       /** Next buffer character to fill.          **/


// PUBLIC MEMBER VARIABLES
public:
  // parameters
  jhcParam lps;
  char host[80];
  int add1, add2, add3, add4, port, echo, cmode;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcTripleCom ();
  void Reset ();
  int Connected () {return Status();}

  // underlying connection
  const char *Host ()     {return jhcSocket::Host();}     /** Name of this machine. **/
  const char *Address ()  {return jhcSocket::Address();}  /** URL of this machine.  **/
  int Port ()             {return port;}
  int AwaitIn (double timeout =30.0) {return jhcSocket::Listen(port, timeout);}
  int ForgeOut (const char *name_url);
  int ForgeOut (int use_name =0);

  // processing parameter manipulation 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // message passing
  int SendTriple (const char *obj, const char *slot, const char *val);
  int SendOver (int gnum =2);
  int AnyTriples ();
  int GetTriple (char *obj, char *slot, char *val, int osz, int ssz, int vsz);

  // message passing - convenience
  template <size_t osz, size_t ssz, size_t vsz>
    int GetTriple (char (&obj)[osz], char (&slot)[ssz], char (&val)[vsz])
      {return GetTriple(obj, slot, val, osz, ssz, vsz);}

  // communications functions
  int Sync ();
  void RewindReply () {focus = reply;}
  int NextReply (char *id, char *fcn, char *val, int isz, int fsz, int vsz);

  // communications - convenience
  template <size_t isz, size_t fsz, size_t vsz>
    int NextReply (char (&id)[isz], char (&fcn)[fsz], char (&val)[vsz])
      {return NextReply(id, fcn, val, isz, fsz, vsz);}

  // debugging functions
  void PrintReply ();


// PRIVATE MEMBER FUNCTIONS
private:
  int link_params (const char *fname);
  char *next_token (char **src, const char *delim);


};


#endif  // once




