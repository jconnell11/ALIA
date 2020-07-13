// jhcManusTAIS.cpp : simulated forklift robot in TAIS internet application
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

#include <conio.h>
#include <math.h>
#include <string.h>

#include "Interface/jms_x.h"           // common video

#include "Body/jhcManusTAIS.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcManusTAIS::~jhcManusTAIS ()
{
}


//= Default constructor initializes certain values.

jhcManusTAIS::jhcManusTAIS ()
{
  frame.SetSize(640, 360, 3);
  strcpy_s(rx.topic, "from_body");
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters for interacting with remote simulator.

int jhcManusTAIS::tais_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("tais_port", 0);
  ps->NextSpec4( &(rx.port), 4815, "Sensor subscribe port");
  ps->NextSpec4( &(tx.port), 4816, "Command push port");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcManusTAIS::Defaults (const char *fname)
{
  int ok = 1;

  ok &= tps.LoadText(rx.host, fname, "tais_host", "52.116.19.88");
  ok &= tais_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcManusTAIS::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= tps.SaveText(fname, "tais_host", rx.host);
  ok &= tps.SaveVals(fname);
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// returns 2 if both direction good, 1 if only outgoing, 0 or negative for problem

int jhcManusTAIS::Reset (int noisy)
{
  // clear variables and reset connections
  clr_state();
  tx.Reset();
  rx.Reset();
  strcpy_s(tx.host, rx.host);

  // try establishing links
  if (tx.ZChk() <= 0)
  {
    jprintf(1, noisy, ">>> No commands to %s on port %d !\n", tx.host, tx.port);
    return mok;
  }
  mok = 1;
  if (rx.ZChk() <= 0)
  {
    jprintf(1, noisy, ">>> No sensors from %s on port %d !\n", rx.host, rx.port); 
    return mok;
  }
  mok = 2;

  // connect to video
  if (tc.Open(rx.host, rx.port) <= 0)
  {
    jprintf(1, noisy, ">>> No images from %s on port %d !\n", rx.host, rx.port); 
    return mok;
  }
  mok = 3;

  // initialize odometry based on first pose
  jprintf(1, noisy, "Getting initial robot state ...\n");
  while (!_kbhit())          // wait forever
  {
    if (Update() > 0)
      break;
    jms_sleep(100);
  }
  Zero();
  jprintf(1, noisy, "\n");
  return mok;
}


///////////////////////////////////////////////////////////////////////////
//                             Rough Odometry                            //
///////////////////////////////////////////////////////////////////////////

//= Reset odometry so current direction is angle zero and path length zero.
// also resets Cartesian coordinates to (0, 0) and x axis points forward

void jhcManusTAIS::Zero ()
{
  clr_odom();
  x0 = wx;
  y0 = wy;
  ang0 = wdir;
}


///////////////////////////////////////////////////////////////////////////
//                             Core Interaction                          //
///////////////////////////////////////////////////////////////////////////

//= Read and interpret base odometry as well as grip force and distance.
// automatically resets "lock" for new bids and specifies default motion
// returns positive if successful, 0 for no update, negative for problem (BLOCKS)

int jhcManusTAIS::Update ()
{
  double rads = D2R * ang0, c = cos(rads), s = sin(rads);
  double dx, dy, wx0, wy0, wd0;
  int n = 0;

  // make sure stream is working then get any image
  if (rx.ZChk() <= 0)
    return -2;
  if (tc.Get(frame) < 0)
    return -1;

  // read in each set of values in order
  while (rx.ZRead() > 0)
  {
    // remember starting pose then get new sensor values
    wx0 = wx;
    wy0 = wy;
    wd0 = wdir;
    n += sensor_msg();

    // update path length and total rotation (windup)
    dx = wx - wx0;
    dy = wy - wy0;
    trav += sqrt(dx * dx + dy * dy);
    head += wdir - wd0;
  }

  // save final local position from most recent message
  if (n > 0)
  {
    dx = wx - x0;
    dy = wy - y0;
    xpos = s * dy + c * dx;
    ypos = c * dy - s * dx;
  }
    
  // set up for next cycle
  cmd_defs();
  return n;
}


//= Parse packet of ZeroMQ sensor data.
// returns number of values extracted

int jhcManusTAIS::sensor_msg ()
{
  const char *msg = rx.Message();
  int n = 0;

  n += pull_float(wx,   "xpos",     msg);
  n += pull_float(wy,   "ypos",     msg);
  n += pull_float(wdir, "aim",      msg);
  n += pull_float(wid,  "width",    msg);
  n += pull_float(ht,   "height",   msg);
  n += pull_float(dist, "distance", msg);
  return n;
}


//= Extract floating point value after given tag.
// returns 1 if successful, 0 if not found (value left unaltered)

int jhcManusTAIS::pull_float (double &val, const char *tag, const char *msg) const
{
  const char *start;

  if ((start = strstr(msg, tag)) != NULL)
    if ((start = strchr(start, ':')) != NULL)
      if (sscanf_s(start + 1, "%lf", &val) == 1)
        return 1;
  return 0;
}


//= Send drive speeds, desired forklift height, and adjust gripper.
// returns 1 if successful, 0 or negative for problem

int jhcManusTAIS::Issue ()
{
  // make sure stream is working 
  if (tx.ZChk() <= 0)
    return 0;

  // send all motor commands
  tx.ZPrintf("{\n");
  tx.ZPrintf("  \"message\": \"cmd\",\n");
  tx.ZPrintf("  \"payload\": {\n");
  tx.ZPrintf("    \"move\": %3.1f,\n", move);
  tx.ZPrintf("    \"turn\": %3.1f,\n", turn);
  tx.ZPrintf("    \"lift\": %3.1f,\n", fork);
  tx.ZPrintf("    \"grab\": %d\n",     grip);
  tx.ZPrintf("  }}\n");

  // clean up
  tx.ZEnd();
  return 1;
}

