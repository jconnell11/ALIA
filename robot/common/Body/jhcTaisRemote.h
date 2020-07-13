// jhcTaisRemote.h : interface to allow local GUI to run ALIA remotely
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019 IBM Corporation
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

#ifndef _JHCTAISREMOTE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTAISREMOTE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Acoustic/jhcGenIO.h"         // common audio
#include "Comm/jhcRcvZMQ.h"            
#include "Comm/jhcReportZMQ.h"

#include "Data/jhcImg.h"               // common video
#include "Data/jhcImgIO.h"
#include "Data/jhcParam.h"

#include "Body/jhcManusBody.h"


//= Interface to allow local GUI to run ALIA remotely.

class jhcTaisRemote
{
// PRIVATE MEMBER VARIABLES
private:
  char utxt[200], btxt[200];
  UC8 cvt[75];
  jhcReportZMQ tx;
  jhcRcvZMQ rx;
  jhcManusBody *body;
  jhcImgIO jio;
  int esc;


// PUBLIC MEMBER VARIABLES
public:
  jhcParam tps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcTaisRemote ();
  jhcTaisRemote ();
  void Bind (jhcManusBody *robot) {body = robot;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  int Init (int id, int noisy =0);
  int Accept (const char *txt, int done =0);
  int Respond (jhcGenIO *io =NULL);
  void Shutdown ();

  // intercepted I/O
  const char *NewInput () const  {return utxt;}
  const char *NewOutput () const {return btxt;}


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int tais_params (const char *fname);

  // outgoing data
  int send_listen (const char *txt);
  int send_sensor ();

  // incoming data
  int get_response ();
  const char *find_tag (const char *tag, const char *msg) const;
  const char *trim_txt (char *txt, const char *msg, int ssz) const;
  int report_msg (const char *kind, const char *data);
  int cmd_msg (const char *kind, const char *data);
  int pull_float (double &val, const char *tag, const char *msg) const;

  // image transmission 
  int send_image (const jhcImg *img);
  int get24 (UL32& val, FILE *in) const;
  void put24 (UL32 val, int n);
  void build_cvt ();


};


#endif  // once




