// jhcManusTAIS.h : simulated forklift robot in TAIS internet application
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

#ifndef _JHCMANUSTAIS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCMANUSTAIS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Comm/jhcRcvZMQ.h"            // common audio
#include "Comm/jhcReportZMQ.h"

#include "Data/jhcParam.h"             // common video

#include "Body/jhcManusX.h"            // common robot
#include "Body/jhcTaisCam.h"


//= Simulated forklift robot in TAIS internet application.
// largely a copy of jhcManusBody with serial port replaced

class jhcManusTAIS : public jhcManusX
{
// PRIVATE MEMBER VARIABLES
private:
  // communications
  jhcRcvZMQ rx;
  jhcReportZMQ tx;

  // raw odometry
  double wx, wy, wdir, x0, y0, ang0;

  // images
  jhcTaisCam tc;


// PUBLIC MEMBER VARIABLES
public:
  // socket parameters
  jhcParam tps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcManusTAIS ();
  jhcManusTAIS ();

  // connection
  int RxPort () const       {return rx.port;}
  int TxPort () const       {return tx.port;}
  void SetRx (int pnum)     {rx.port = pnum;}
  void SetTx (int pnum)     {tx.port = pnum;}
  const char *Host () const {return rx.host;}
  void SetHost (const char *url) 
    {strcpy_s(rx.host, url); strcpy_s(tx.host, url);}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);
  int SaveVals (const char *fname) const;

  // main functions
  int Reset (int noisy =0);

  // rough odometry 
  void Zero ();

  // core interaction
  int Update ();
  int Issue ();


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int tais_params (const char *fname);

  // core interaction
  int sensor_msg ();
  int pull_float (double &val, const char *tag, const char *msg) const;


};


#endif  // once




