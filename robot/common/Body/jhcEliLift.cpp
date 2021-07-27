// jhcEliLift.cpp : control of Eli robot's motorized forklift stage
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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

#include <math.h>

#include "Interface/jhcMessage.h"
#include "Interface/jms_x.h"

#include "Body/jhcEliLift.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcEliLift::~jhcEliLift ()
{
  Limp();
}


//= Default constructor initializes certain values.

jhcEliLift::jhcEliLift ()
{
  // communications
  lok = -1;

  // profile generator
  strcpy_s(rname, "fork_ramp");
  done = 0.5;

  // motion control
  stiff = 0;
  clr_lock(1);

  // processing parameters
  LoadCfg();
  Defaults();
  ht = ht0;
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for connecting to linear actuator controller.
// nothing geometric that differs between bodies

int jhcEliLift::lift_params (const char *fname)
{
  jhcParam *ps = &fps;
  int ok;

  ps->SetTag("lift_cfg", 0);
  ps->NextSpec4( &lport,      7,   "Serial port number");         // was 6
  ps->NextSpec4( &lbaud, 115200,   "Serial baud rate");
  ps->NextSpecF( &ht0,       23.0, "Good starting height (in)");
  ps->NextSpecF( &ldone,      0.5, "Blocking lift done test (in)");
  ps->NextSpecF( &quit,       0.5, "Blocking move timeout (sec)");
  ps->NextSpec4( &ms,        33,   "Default condition check (ms)");

  ps->NextSpecF( &vstd,      16.0, "Std move speed (ips)");
  ps->NextSpecF( &astd,      64.0, "Std acceleration (ips^2)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for converting JRK readings into heights.

int jhcEliLift::geom_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("lift_origin", 0);
  ps->NextSpecF( &top, 37.85, "Max arm shelf height (in)");
  ps->NextSpecF( &bot,  1.85, "Min arm shelf height (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Read all relevant defaults variable values from a file.

int jhcEliLift::Defaults (const char *fname)
{
  return lift_params(fname);
}


//= Read just body specific values from a file.

int jhcEliLift::LoadCfg (const char *fname)
{
  return geom_params(fname);
}


//= Write current processing variable values to a file.

int jhcEliLift::SaveVals (const char *fname) const
{
  return fps.SaveVals(fname);
}


//= Write current body specific values to a file.

int jhcEliLift::SaveCfg (const char *fname) const
{
  return gps.SaveVals(fname);
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// generally lok: -1 = no port, 0 = comm error, 1 = fine
// if rpt > 0 then prints to log file, ignores chk flag

int jhcEliLift::Reset (int rpt, int chk)
{
  UC8 pod[2];

  // announce entry
  if (rpt > 0)
    jprintf("\nLift reset ...\n");
  clr_lock(1);
  LiftClear();

  // connect to proper serial port (if needed)
  if (lok < 0)
    if (lcom.SetSource(lport, lbaud) <= 0)
    {
      if (rpt >= 2)
        Complain("Could not open serial port %d in jhcEliLift::Reset", lport);
      else if (rpt > 0)
        jprintf(">>> Could not open serial port %d in jhcEliLift::Reset !\n", lport);
      return fail(rpt);
    }
  lok = 1;

  // clear lift errors
  if (rpt > 0)
    jprintf("  error clear ...\n");
  if (lcom.Xmit(0xB3) <= 0)
    return fail(rpt);
  if (lcom.RxArray(pod, 2) < 2)
    return fail(rpt);

  // stop all motion
  if (rpt > 0)
    jprintf("  freeze ...\n");
  Update();
  if (Freeze() <= 0)
    return fail(rpt);

  // initialize targets and positions
  if (rpt > 0)
    jprintf("  current height ...\n");
  Update();
  if (rpt > 0)
    jprintf("    %3.1f inches\n", ht);
  Freeze();

  // speed estimate
  now = 0;
  ips = 0.0;

  // finished
  if (rpt > 0)
    jprintf("    ** good **\n");
  return lok;
}


//= Failure message for some part of initialization.

int jhcEliLift::fail (int rpt) 
{
  if (lok > 0)
    lok = 0;
  if (rpt > 0)
    jprintf("    >> BAD <<\n");
  return lok;
}


//= Check that servo board is responding.
// if rpt > 0 then pops dialog boxes for failure

int jhcEliLift::Check (int rpt, int tries)
{
  UC8 pod[2];
  int n, yack = 0;

  if (rpt > 0)
    jprintf("\nLift check ...\n");
  for (n = 1; n <= tries; n++)
  {
    // only potentially complain on last trial
    if ((rpt > 0) && (n >= tries))
      yack = 1;

    // get current error flags (but do not reset)
    lok = 0;
    if (lcom.Xmit(0xB3) > 0)
      if (lcom.RxArray(pod, 2) == 2)
        lok = 1;
    if (lok <= 0)
      continue;

    // see if no flags set
    if ((pod[0] == 0) && (pod[1] == 0))
    {
      if (rpt > 0)
        jprintf("    ** good **\n");
      break;
    }

    // complain about problems
    if (rpt > 0)
      jprintf("    error flags %03X (hex)\n", (pod[1] << 8) | pod[0]);
  }
  return lok;
}
 

///////////////////////////////////////////////////////////////////////////
//                           Low Level Commands                          //
///////////////////////////////////////////////////////////////////////////

//= Make lift stage stop in place with brakes on (beware potential bounce!).
// generally should call Update just before this
// if tupd > 0 then calls Issue after this
// NOTE: for continuing freeze set rate = 0 like LiftShift(0.0, 0.0, 0.0, 1000)

int jhcEliLift::Freeze (int doit, double tupd)
{
  // tell ramp controller to remember position
  if (doit <= 0)
    return lok;
//  LiftStop(1.5, 2001);
  rt = 0.0;

  // possibly talk to lift stage
  stiff = 1;
  if (tupd > 0.0)
    Issue(tupd);
  return lok;
}


//= Make lift stage stop and turn motor off and immediately talk to motor controller.
// for continuing limp set current position like LiftShift(0.0, 1.0, 1000)
// NOTE: this is "freer" then recycling current position since motor is disabled

int jhcEliLift::Limp ()
{
  // make sure hardware is working
  if (lok < 0)
    return lok;
  lok = 1;

  // no motion
  stiff = 0;
  LiftClear();

  // disable servo control
  if (lcom.Xmit(0xFF) <= 0)
    lok = 0;

  // make sure readings are up to date
  Update();
  RampTarget(ht);
  return lok;
}


///////////////////////////////////////////////////////////////////////////
//                            Core Interaction                           //
///////////////////////////////////////////////////////////////////////////

//= Find out where the lift is now.
// automatically resets "llock" for new bids
// should take about 0.26ms

int jhcEliLift::Update ()
{
  if (UpdateStart() <= 0)
    return lok;
  return UpdateFinish();
}


//= Request current height but don't wait for a response.

int jhcEliLift::UpdateStart ()
{
  // make sure hardware is working
  if (lok < 0)
    return lok;

  // request current position of stage
 if (lcom.Xmit(0xA7) < 1)
    lok = 0;
  return lok;
}


//= Retrieve height of forklift from previous request.
// height only good to about +/- 0.25" really

int jhcEliLift::UpdateFinish ()
{
  UC8 pod[2];
  UL32 last = now;
  double s, i, h0 = ht, mix = 0.5;
  int pos;

  // make sure hardware is working
  if (lok < 0)
    return lok;
  lok = 0;

  // collect current position of stage
  if (lcom.RxArray(pod, 2) < 2)
    return lok;
  pos = (pod[1] << 8) | pod[0];
  lok = 1;

  // convert to inches and save
  ht = bot + (top - bot) * pos / 4095.0;
  now = jms_now();
  if (last != 0)
    if ((s = jms_secs(now, last)) > 0.0)
    {
      // instantaneous speed estimte
      i = fabs(ht - h0) / s;
      ips += mix * (i - ips);
    }

  // set default command for next cycle
  clr_lock(0);
  return lok;
}


//= Clear winning command bid for resource.
// can optionally clear previous bid also

void jhcEliLift::clr_lock (int hist)
{
  llock0 = ((hist > 0) ? 0 : llock);
  llock = 0;
}


//= Move toward target position (ignores update rate).

int jhcEliLift::Issue (double tupd, double lead)
{
  UC8 pod[2];
  double pos;
  int val;

  // check if lift stage is under active command
  if (stiff > 0)
  {
    // set default if no lift target specified
    Freeze(((llock <= 0) ? 1 : 0), 0.0);           

    // continue along profile then convert profile position to servo command value
    pos = RampNext(ht, tupd, lead);
    val = ROUND(4095.0 * (pos - bot) / (top - bot));

    // send to controller
    pod[0] = (UC8)(0xC0 | (val & 0x1F));  // low 5 bits
    pod[1] = (UC8)((val >> 5) & 0x7F);    // high 7 bits 
    if (lcom.TxArray(pod, 2) < 2)
      lok = 0;
  }
  return lok;
}


///////////////////////////////////////////////////////////////////////////
//                            Profiled Motion                            //
///////////////////////////////////////////////////////////////////////////

//= Copy parameters for lift target height.
// negative rate does not scale acceleration (for snappier response)
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliLift::LiftTarget (double height, double rate, int bid)
{
  if (bid <= llock)
    return 0;
  llock = bid;
  stiff = 1;
  RampTarget(__max(bot, __min(height, top)), rate);
  return 1;
}


//= Gives error (in inches) between current height and goal.
// can optionally give absolute value and constrain goal to valid range

double jhcEliLift::LiftErr (double high, int abs, int lim) const 
{
  double goal = __max(bot, __min(high, top));
  double err = ((lim > 0) ? (ht - goal) : (ht - high));

  return((abs > 0) ? fabs(err) : err);
}


///////////////////////////////////////////////////////////////////////////
//                             Atomic Actions                            //
///////////////////////////////////////////////////////////////////////////

//= Drive lift to a particular height (blocks).

int jhcEliLift::SetLift (double ins)
{
  // check hardware and current position
  if (Update() <= 0)
    return -1;
  
  // drive lift until timeout
  while (1)
  {
    // reiterate command
    LiftTarget(ins);

    // change height pursuit point if needed then wait
    Issue(0.001 * ms);
    jms_sleep(ms);
    Update();

    // see if close enough yet
    if (LiftClose() || LiftFail())
      break;
  }
    
  // stop lift and report if timeout occurred
  LiftClear();
  Freeze();
  return((LiftClose()) ? 1 : 0);
}
