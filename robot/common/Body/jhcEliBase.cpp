// jhcEliBase.cpp : control of Eli mobile robot base
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2020 IBM Corporation
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

#include <math.h>

#include "Interface/jhcMessage.h"
#include "Interface/jms_x.h"

#include "Body/jhcEliBase.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcEliBase::~jhcEliBase ()
{
  Limp();
}


//= Default constructor initializes certain values.

jhcEliBase::jhcEliBase ()
{
  // basic info
  *ver = '\0';
  noisy = 0;
  berr = -1;           // no port yet
  pend = 0;
  grn = 0;             // assume encoders not reversed
  c16 = 0;             // assume 1 byte checksum 

  // profile generators
  strcpy_s(mctrl.rname, "move_ramp");
  strcpy_s(tctrl.rname, "turn_ramp");
  mctrl.done = 0.5;
  tctrl.done = 2.0;  

  // motion control
  clr_locks(1);
  stiff = 0;
  ice = 0;
  ice2 = 0;
  ms = 33;             // blocking update time (ms)

  // current raw state
  lf  = 0;
  rt  = 0;
  lf0 = 0;
  rt0 = 0;

  // current refined state
  head = 0.0;
  trav = 0.0;
  xpos = 0.0;
  ypos = 0.0;
  dm  = 0.0;
  dr  = 0.0;
  dx0 = 0.0;
  dy0 = 0.0;
  dx  = 0.0;
  dy  = 0.0;

  // processing parameters
  LoadCfg();
  Defaults();
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters used for communication with RoboClaw controller.
// PID values need to be about 500x bigger than nominal (44000 / 90)
// with I = D = 0, crank P up until robot starts pulsing
// then with D = 0, crank I up until robot starts pulsing
// nothing geometric that differs between bodies

int jhcEliBase::ctrl_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("base_cfg", 0);
  ps->NextSpec4( &bport,     6,   "Serial port number");        // was 7
  ps->NextSpec4( &bbaud, 38400,   "Serial baud rate");
  ps->NextSpecF( &ploop,     1.0, "Proportional factor");       // was 12M, 512, then 32 (ignored?)
  ps->NextSpecF( &iloop,   100.0, "Integral factor");           // was 4M, 256, then 8 (important!)
  ps->NextSpecF( &dloop,     0.0, "Derivative factor");         // was 128, 8, then 9 (not needed?)
  ps->NextSpec4( &pwm,       0,   "Use PWM mode instead");      // was 1 for old encoders

  ps->NextSpecF( &rpm,     150.0, "Max rotation rate (rpm)");
  ps->NextSpec4( &ppr,     144,   "Pulses per revolution");     // was 36 for old encoders
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for profiled motion.
// nothing geometric that differs between bodies

int jhcEliBase::move_params (const char *fname)
{
  jhcParam *ps = &mps;
  int ok;

  ps->SetTag("base_move", 0);
  ps->NextSpecF( &(mctrl.vstd),  30.0, "Std move speed (ips)");    // 1.5x = 2.5 mph (max 47 ips)
  ps->NextSpecF( &(mctrl.astd),  20.0, "Std move accel (ips^2)");  // 22.5" to full speed 
  ps->NextSpecF( &(mctrl.dstd),  10.0, "Std move decel (ips^2)");  // 45" slow down zone 
  ps->NextSpecF( &(tctrl.vstd),  90.0, "Std turn speed (dps)");  
  ps->NextSpecF( &(tctrl.astd), 360.0, "Std turn accel (dps^2)");  // 11.25 deg to full (was 180)
  ps->NextSpecF( &(tctrl.dstd),  90.0, "Std turn decel (dps^2)");  // 45.0 deg slow zone (was 180)

  ps->NextSpecF( &mdead,          0.5, "Move deadband (in)");
  ps->NextSpecF( &tdead,          2.0, "Turn deadband (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for interpreting encoder values and battery charge.

int jhcEliBase::geom_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("base_geom", 0);
  ps->NextSpecF( &wd,   6.0, "Wheel diameter (in)");  
  ps->NextSpecF( &ws,  13.0, "Wheel separation (in)");  
  ps->Skip();
  ps->NextSpecF( &vmax, 0.0, "Full battery voltage");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Read all relevant defaults variable values from a file.

int jhcEliBase::Defaults (const char *fname)
{
  int ok = 1;

  ok &= ctrl_params(fname);
  ok &= move_params(fname);
  return ok;
}


//= Read just body specific values from a file.

int jhcEliBase::LoadCfg (const char *fname)
{
  return geom_params(fname);
}


//= Write current processing variable values to a file.

int jhcEliBase::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= cps.SaveVals(fname);
  ok &= mps.SaveVals(fname);
  return ok;
}


//= Write current body specific values to a file.

int jhcEliBase::SaveCfg (const char *fname) const
{
  return gps.SaveVals(fname);
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence (and stop all motion).
// if rpt > 0 then prints to log file, if chk > 0 measures battery
// generally berr: -1 = no port yet, 0 = fine, pos = comm error count
// returns 1 if port bound correctly and base initialized, 0 or negative for error 

int jhcEliBase::Reset (int rpt, int chk)
{
  double v;

  // announce entry 
  jprintf(1, rpt, "\nBase reset ...\n");
  clr_locks(1);
  DriveClear();
  bcom.SetRTS(0);            // force light off
  led0 = 0;                  

  // connect to proper serial port (if needed)
  if (berr == 0)
    bcom.Flush();
  else if (bcom.SetSource(bport, bbaud) <= 0)
  {
    if (rpt >= 2)
      Complain("Could not open serial port %d in jhcEliBase::Reset", bport);
    else if (rpt > 0)
      jprintf(">>> Could not open serial port %d in jhcEliBase::Reset !\n", bport);
    return fail(-4, rpt);
  }
  jms_sleep(1000);     // await initialization or flush
  pend = 0;
  berr = 0;

  // read version information and set interpretation parameters
  jprintf("  version ...\n");
  if (Version() == NULL)
    return fail(-3, rpt);
  jprintf("    %s [%s] %s\n", ver, ((grn > 0) ? "grn" : "red"), ((c16 > 0) ? "crc16" : "sum7"));

  // make sure all motion has stopped then configure control loop
  jprintf(1, rpt, "  freeze ...\n");
  Update();
  if (Freeze() <= 0)
    return fail(-2, rpt);
  loop_vals();

  // possibly check battery level
  if (chk > 0)
  {
    jprintf(1, rpt, "  battery ...\n");
    if ((v = Battery()) <= 0.0)
      return fail(-1, rpt);
    jprintf(1, rpt, "    %3.1f volts nominal\n", v);
  }

  // clear wheel encoders to zero
  jprintf(1, rpt, "  clr odom ...\n");
  if (Zero() <= 0)
    return fail(0, rpt);

  // initialize targets and positions
  Update();
  ice = 0;
  ice2 = 0;
  Freeze();

  // instantaneous speed estimates
  now = 0;
  mvel = 0.0;
  tvel = 0.0;
  imv  = 0.0;
  itv  = 0.0;
  parked = 0;

  // finished
  jprintf(1, rpt, "    ** good **\n");
  return 1;
}


//= Failure message for some part of initialization.
// does not alter berr count, already set by failing fcn (if any)
// always returns ans (and possibly complains) 

int jhcEliBase::fail (int ans, int rpt) 
{
  jprintf(1, rpt, "    >> BAD <<\n");
  return ans;
}


//= Send down PID control loop parameters for base motor controller.
// PID values have 16 bit integer and 16 bit fractional parts
// new (2106) 2x30 controller needs 4x higher PID values than older red boards
// NOTE: does not block, tx takes about 11.7ms = (7 + 2 * 19) * 10 / 38400
// returns 1 if successful, 0 or negative for problem

int jhcEliBase::loop_vals ()
{
  double sh = ((grn > 0) ? 512.0 : ((c16 <= 0) ? 65536.0 : 262144.0));
  UL32 p = (UL32)(ploop * sh), i = (UL32)(iloop * sh), d = (UL32)(dloop * sh);
  UL32 q = (UL32)(rpm * ppr / 60.0 + 0.5);
  int k, n = 0; 

  // clear any old acknowledgements
  fail_pend();

  // set battery to be 10-14 volts (cmd = 57)
  if (c16 > 0)
  {
    pod[n++] = 0x80;
    pod[n++] = 57;
    pod[n++] = 0;
    pod[n++] = 100;
    pod[n++] = 0;
    pod[n++] = 140;
    start_crc(pod, n);
    n = set_crc(pod, n);
    bcom.TxArray(pod, n);
    BBARF(-1, fail_ack(1));
  }

  // set M1/M2 parameters (cmd = 28/29)
  for (k = 0; k < 2; k++)
  {
    // controller ID plus command
    n = 0;
    pod[n++] = 0x80;
    pod[n++] = (UC8)(28 + k);

    // derivative scale factor (yes, it comes first)
    pod[n++] = (UC8)((d >> 24) & 0xFF);
    pod[n++] = (UC8)((d >> 16) & 0xFF);
    pod[n++] = (UC8)((d >>  8) & 0xFF);
    pod[n++] = (UC8)( d        & 0xFF);
  
    // proportional scale factor
    pod[n++] = (UC8)((p >> 24) & 0xFF);
    pod[n++] = (UC8)((p >> 16) & 0xFF);
    pod[n++] = (UC8)((p >>  8) & 0xFF);
    pod[n++] = (UC8)( p        & 0xFF);

    // integral scale factor
    pod[n++] = (UC8)((i >> 24) & 0xFF);
    pod[n++] = (UC8)((i >> 16) & 0xFF);
    pod[n++] = (UC8)((i >>  8) & 0xFF);
    pod[n++] = (UC8)( i        & 0xFF);

    // max quadrature pulses per second
    pod[n++] = (UC8)((q >> 24) & 0xFF);
    pod[n++] = (UC8)((q >> 16) & 0xFF);
    pod[n++] = (UC8)((q >>  8) & 0xFF);
    pod[n++] = (UC8)( q        & 0xFF);

    // compute checksum and send packet
    start_crc(pod, n);
    n = set_crc(pod, n);
    bcom.TxArray(pod, n);
  }

  // wait for and strip off any acknowledgements
  BBARF(0, fail_ack(2));
  return 1;
}


//= Check that hardware is still working.
// does not affect any motion that is in progress
// returns 1 if okay, 0 for problem

int jhcEliBase::Check (int rpt, int tries)
{
  int n;

  jprintf(1, rpt, "\nBase check ...\n");
  for (n = 1; n <= tries; n++)
    if (Battery() > 8.0)
    {
      jprintf(1, rpt, "    ** good **\n");
      return 0;
    }
  return 0;
}


//= Tells current voltage of main battery (to nearest 100mv).
// BLOCKS: transaction takes about 1.3ms = (2 + 3) * 10 / 38400
// returns 0.0 if problem

double jhcEliBase::Battery ()
{
  // make sure hardware is working
  BBARF(0, fail_pend());

  // ask about main battery voltage (no CRC needed)
  pod[0] = 0x80;
  pod[1] = 24;
  bcom.TxArray(pod, 2);

  // get response in 100mv units and check if valid
  BBARF(0, (bcom.RxArray(pod + 2, 3 + c16) < (3 + c16)));
  start_crc(pod, 4);
  BBARF(0, fail_crc(pod + 4))

  // convert to volts
  return(0.1 * ((pod[2] << 8) + pod[3]));
}


//= Retrieve version number string from motor controller.
// also sets interpretation variables "grn" and "c16"
// returns pointer to string, NULL if problem reading 
// BLOCKS: transaction takes about 6.5ms = (2 + 23) * 10 / 38400

const char *jhcEliBase::Version ()
{
  int ch, n, i = 0;

  // make sure hardware is working
  BBARF(0, fail_pend());

  // set up query to board (no CRC needed)
  pod[0] = 0x80;
  pod[1] = 21;
  bcom.TxArray(pod, 2);

  // read basic response string (slow)
  jms_sleep(10);
  for (i = 0; i < 32; i++)
  {
    BBARF(0, (ch = bcom.Rcv()) < 0);
    ver[i] = (char) ch;
    if (ch == '\0')
      break;
  }

  // extract board type
  if (ver[0] == 'U')
    c16 = 1;                         // new USB board has 16 bit CRC
  else
  {
    // old green PCB version has channels reversed
    n = atoi(ver + 9);               
    if ((n != 15) && (n != 30))      // check number after "RoboClaw"
      grn = 1;
  }

  // compute packet check then verify
  start_crc(pod, 2);
  add_crc((UC8 *) ver, i + 1);
  BBARF(0, bcom.RxArray(pod, 1 + c16) < (1 + c16));
  BBARF(0, fail_crc(pod));

  // strip final CR if name was received properly
  n = __max(0, i - 1);
  if (ver[n] == '\n')
    ver[n] = '\0';
  return ver;
}


///////////////////////////////////////////////////////////////////////////
//                           Packet Validation                           //
///////////////////////////////////////////////////////////////////////////

//= Create check value from bytes using proper method.
// value saved in global variable "crc", should be zero at start

void jhcEliBase::add_crc (const UC8 *data, int n)
{
  int i, j;

  // for older boards
  if (c16 <= 0)
  {
    // simple 7 bit sum of bytes
    for (i = 0; i < n; i++)                    
      crc += data[i]; 
    crc &= 0x7F;
    return;
  }

  // for new "USB" boards (2016)
  for (i = 0; i < n; i++)
  {
    // XOR in shifted new byte
    crc = crc ^ (data[i] << 8);

    // compute polynomial by shifting
    for (j = 0; j < 8; j++)
      if ((crc & 0x8000) != 0)
        crc = (crc << 1) ^ 0x1021;
      else
        crc <<= 1;
  }
  crc &= 0xFFFF;
}


//= Add one or two bytes to packet with validation code.
// returns new length of packet

int jhcEliBase::set_crc (UC8 *data, int n) const
{
  if (c16 <= 0)
  {
    // 7 bit value
    data[n] = (UC8) crc;
    return(n + 1);
  }

  // 16 bit (big-endian) value
  data[n] = (crc >> 8) & 0xFF;
  data[n + 1] = crc & 0xFF;
  return(n + 2);
}


//= Read next one or two bytes and see if matches computed check.
// must be proper number of bytes in pod, uses global "crc" for comparison

bool jhcEliBase::fail_crc (const UC8 *data) const
{
  if (c16 <= 0)
    return((data[0] & 0x7F) != crc);             // 7 bit sum
  return(((data[0] << 8) | data[1]) != crc);     // 16 bit CRC
}


//= Read and check command acknowledgements (if any).
// returns true if any expected missing (should all be 0xFF)

bool jhcEliBase::fail_ack (int n)
{
  int i, val, bad = 0;

  // does not exist for old board
  if (c16 <= 0)                
    return false;

  // always strip the requested number of bytes (if possible)
  for (i = 0; i < n; i++)
  {
    if ((val = bcom.Rcv()) < 0)
      return true;
    if (val != 0xFF)
      bad++; 
  }
  return(bad > 0);
}


///////////////////////////////////////////////////////////////////////////
//                          Low Level Commands                           //
///////////////////////////////////////////////////////////////////////////

//= Make base stop in place (active braking).
// generally should call Update just before this
// if tupd > 0 then calls Issue after this
// returns 1 if successful, 0 for likely problem

int jhcEliBase::Freeze (int doit, double tupd)
{
  // set soft stop goal positions (only)
  FreezeMove(doit, 0.0);
  FreezeTurn(doit, 0.0);

  // possibly talk to wheel motor controller
  stiff = 1;
  if (tupd > 0.0)
    Issue(tupd);

  // light off
  bcom.SetRTS(0);
  led0 = 0;
  return CommOK();
}


//= Bring base translation to a gentle stop obeying max deceleration limit.

int jhcEliBase::FreezeMove (int doit, double tupd)
{
  // reset edge trigger
  if (doit <= 0)
  {
    ice = 0;
    return CommOK();
  }

  // remember position and heading at first call (prevents drift)
//  if (ice <= 0)                                                   // bounces!
  {
//    MoveStop(1.5, 2001);                                          // always
    mctrl.RampTarget(trav);
    ice = 1;
  }

  // possibly talk to wheel motor controller
  stiff = 1;
  if (tupd > 0.0)
    Issue(tupd);
  return CommOK();
}


//= Bring base rotation to a gentle stop obeying max deceleration limit.

int jhcEliBase::FreezeTurn (int doit, double tupd)
{
  // reset edge trigger
  if (doit <= 0)
  {
    ice2 = 0;
    return CommOK();
  }

  // remember position and heading at first call (prevents drift)
//  if (ice2 <= 0)                                                   // bounces!
  {
//    TurnStop(1.5, 2001);                                           // always
    tctrl.RampTarget(head);
    ice2 = 1;
  }

  // possibly talk to wheel motor controller
  stiff = 1;
  if (tupd > 0.0)
    Issue(tupd);
  return CommOK();
}


//= Make base stop and go passive (pushable).
// immediately talks to motor controller
// returns 1 if successful, 0 for likely problem

int jhcEliBase::Limp ()
{
  // no motion or light
  stiff = 0;
  DriveClear();
  bcom.SetRTS(0);
  led0 = 0;

  // make sure readings are up to date
  Update();
  mctrl.RampTarget(trav);
  tctrl.RampTarget(head);

  // make sure it takes effect
  Issue(ms);
  return CommOK();
}


///////////////////////////////////////////////////////////////////////////
//                           Core Interaction                            //
///////////////////////////////////////////////////////////////////////////

//= Attempt to read and interpret base odometry.
// automatically resets "lock" for new bids and specifies default motion
// move steps are about 0.26 in, turn steps are about 2.3 degs
// full update takes about 8 ms at 38.4 kbaud

int jhcEliBase::Update ()
{
  if (UpdateStart() <= 0)
    return -2;
  if (UpdateContinue() <= 0)
    return -1;
  if (UpdateFinish() <= 0)
    return 0;
  return 1;
}


//= Clear commands bids and issue request for right encoder.
// takes about 3ms for right value to be ready to read
// returns 1 if successful, 0 or negative for likely problem

int jhcEliBase::UpdateStart ()
{
  // save previous encoder values
  rt0 = rt;
  lf0 = lf;

  // make sure hardware is working
  BBARF(0, fail_pend());

  // ask for M1 counts (right - no CRC needed)
  pod[0] = 0x80;
  pod[1] = 16;
  bcom.TxArray(pod, 2);

  // initialize CRC with command
  start_crc(pod, 2);
  return 1;
}


//= Pick up right encoder value and issue request for left encoder.
// takes about 3ms for left value to be ready to read
// returns 1 if successful, 0 or negative for likely problem

int jhcEliBase::UpdateContinue ()
{
  // read in 32 bit right value (only good to 10 bits --> work with bottom 8)
  BBARF(-1, bcom.RxArray(pod, 6 + c16) < (6 + c16));
  add_crc(pod, 5);
  BBARF(0, fail_crc(pod + 5))
  rt = (pod[0] << 24) | (pod[1] << 16) | (pod[2] << 8) | pod[3];

  // ask for M2 counts (left - no CRC needed)
  pod[0] = 0x80;
  pod[1] = 17; 
  bcom.TxArray(pod, 2);

  // initialize CRC with command
  start_crc(pod, 2);
  return 1;
}


//= Pick up left encoder value and interpret pair.
// automatically resets "lock" for new bids and specifies default motion
// returns 1 if successful, 0 or negative for likely problem

int jhcEliBase::UpdateFinish ()
{
  UL32 last = now;
  double s, m, t, t0 = trav, h0 = head, mmix = 0.5, tmix = 0.3, scoot = 1.0, swivel = 2.0;

  // read in 32 bit value (only good to 10 bits --> work with bottom 8)
  BBARF(-1, bcom.RxArray(pod, 6 + c16) < (6 + c16));
  add_crc(pod, 5);
  BBARF(0, fail_crc(pod + 5));
  lf = (pod[0] << 24) | (pod[1] << 16) | (pod[2] << 8) | pod[3];

  // all data gathered successfully so resolve odometry into robot motion
  cvt_cnts();
  now = jms_now();
  if (last != 0)
    if ((s = jms_secs(now, last)) > 0.0)
    {
      // instantaneous estimate speeds
      m = (trav - t0) / s;
      t = (head - h0) / s;
      imv += mmix * (m - imv); 
      itv += tmix * (t - itv); 
    }

  // do qualitative evaluation of motion
  if ((fabs(imv) >= scoot) || (fabs(itv) >= swivel))
    parked = __min(0, parked - 1);
  else
    parked = __max(1, parked + 1);

  // set up to receive new round of commands and bids
  clr_locks(0);      
  return 1;
}


//= Clear winning command bids for all resources.
// can optionally clear previous bids also

void jhcEliBase::clr_locks (int hist)
{
  // remember winning bid of last arbitration cycle
  mlock0 = ((hist > 0) ? 0 : mlock);
  tlock0 = ((hist > 0) ? 0 : tlock);
  llock0 = ((hist > 0) ? 0 : llock);

  // set up for new target arbitration
  mlock = 0;
  tlock = 0;
  llock = 0;
  led = 0;
}


//= Convert wheel encoder counts into turn and travel vectores.

void jhcEliBase::cvt_cnts ()
{
  double mid, avg, ipp = (PI * wd) / ppr;
  int d1 = 0, d2 = 0;

  // get wheel clicks (if available)
  if (berr == 0)
  {
    // find left side (M2) change since last read (+/- 16")
    d2 = (lf & 0xFF) - (lf0 & 0xFF);
    if (d2 <= -128)
      d2 += 256;
    else if (d2 > 128)
      d2 -= 256;

    // find right side (M1) change since last read (+/- 16")
    d1 = (rt & 0xFF) - (rt0 & 0xFF);
    if (d1 <= -128)
      d1 += 256;
    else if (d1 > 128)
      d1 -= 256;

    // adjust for different controller boards
    if (grn > 0)
      d1 = -d1;
    else
      d2 = -d2;
  }

  // find length of recent segment and change in direction
  dm = 0.5 * (d1 + d2) * ipp;
  dr = R2D * (d1 - d2) * ipp / ws;

  // find offset in former local coordinate system (y = forward, x = RIGHT)
  avg = D2R * 0.5 * dr;
  dx0 = dm * sin(avg);    // sideways
  dy0 = dm * cos(avg);    // forward

  // update inferred global Cartesian position
  mid = D2R * (head + 0.5 * dr);
  dx = dm * cos(mid);
  dy = dm * sin(mid);
  xpos += dx;
  ypos += dy;

  // update path length and current global orientation
  trav += dm;
  head += dr;
}


//= Keep an angle in the range -180 to +180 degrees.

double jhcEliBase::norm_ang (double degs) const
{
  double a = degs;

  if (a > 180.0)
    a -= 360.0 * ROUND(a / 360.0);
  else if (a <= -180.0)
    a += 360.0 * ROUND(a / 360.0);
  return a;
}


//= Move in curved path toward target pose with acceleration limits.
// takes typical call-back interval, automatically handles accelerations
// assumes Update has already been called to get position
// sets instantaneous mvel and tvel based on mwin, ttarg and msp, tsp

int jhcEliBase::Issue (double tupd, double lead)
{
  // check if base stage is under active command
  if (stiff > 0)
  {
    // set default if no base target specified
    FreezeMove(((mlock <= 0) ? 1 : 0), 0.0);
    FreezeTurn(((tlock <= 0) ? 1 : 0), 0.0);

    // continue with move profile to get signed speed
    mctrl.RampNext(trav, tupd, lead);
    mvel = mctrl.RampVel(mdead);
    if (mctrl.RampAxis() < 0.0)
      mvel = -mvel;

    // continue with turn profile to get signed speed
    tctrl.RampNext(head, tupd, lead);
    tvel = tctrl.RampVel(tdead);
    if (tctrl.RampAxis() < 0.0)
      tvel = -tvel;
    wheel_vels(mvel, tvel);
  }
  else
    wheel_pwm(0.0, 0.0);

  // possibly change LED state
  if (led != led0)
    bcom.SetRTS(led);
  led0 = led;
  led = 0;                   // default for next cycle is off
  return CommOK();
}


//= Send drive and turn specs to base using velocity mode.
// "ips" is in inches per second, "dps" is in degrees per second
// returns 1 for success, zero or negative for error 
// does not block, pod transmit takes about 2.9ms = 11 * 10 / 38400

int jhcEliBase::wheel_vels (double ips, double dps)
{
  UL32 vel;
  double dv, lsp, rsp, ppi = ppr / (PI * wd);
  int lf, rt, n = 0;

  // possible run PWM mode if older encoders 
  if (pwm > 0)
    return wheel_pwm(ips, dps);

  // check hardware
  BBARF(0, fail_pend());

  // convert to encoder speeds for differential steering
  dv = dps * (D2R * 0.5 * ws);
  lsp = ppi * (ips - dv);
  rsp = ppi * (ips + dv);

  // adjust for different controller boards 
  if (grn > 0)
    rsp = -rsp;
  else
    lsp = -lsp; 

  // convert to signed 32 bit integer
  lf = ROUND(fabs(lsp));
  if (lsp < 0.0)
    lf = -lf;
  rt = ROUND(fabs(rsp));
  if (rsp < 0.0)
    rt = -rt;

  // use "signed velocity" dual command with wheel speeds
  pod[n++] = 0x80;
  pod[n++] = 37;          
  vel = (UL32) ROUND(rt);                   // M1
  pod[n++] = (UC8)((vel >> 24) & 0xFF);
  pod[n++] = (UC8)((vel >> 16) & 0xFF);
  pod[n++] = (UC8)((vel >> 8) & 0xFF);
  pod[n++] = (UC8)( vel & 0xFF);
  vel = (UL32) ROUND(lf);                   // M2
  pod[n++] = (UC8)((vel >> 24) & 0xFF);
  pod[n++] = (UC8)((vel >> 16) & 0xFF);
  pod[n++] = (UC8)((vel >> 8) & 0xFF);
  pod[n++] = (UC8)( vel & 0xFF);

  // generate checksum 
  start_crc(pod, n);
  n = set_crc(pod, n);

  // send packet but do not wait for completion 
  bcom.TxArray(pod, n);
  pend++;                  // strip ack later
  return 1;
}


//= Send drive and turn specs to base using raw pulse width mode.
// "ips" is in inches per second, "dps" is in degrees per second
// returns 1 for success, zero or negative for error 
// does not block, pod transmit takes about 1.8ms = 7 * 10 / 38400
// NOTE: deprecated, only needed for old 9 vane Parallax encoders

int jhcEliBase::wheel_pwm (double ips, double dps)
{
  US16 pwm;
  double dv, lsp, rsp, s2p = 60.0 / (PI * wd * rpm);
  int lf, rt, lim, n = 0;

  // make sure hardware is working
  BBARF(0, fail_pend());

  // compute left and right speeds for differential steering
  dv = dps * (D2R * 0.5 * ws);
  lsp = ips - dv;
  rsp = ips + dv;

  // adjust for different controller boards 
  if (grn > 0)
  {
    lim = 127;        // 8 bit signed PWM
    rsp = -rsp;
  }
  else
  {
    lim = 511;        // 10 bit signed PWM
    lsp = -lsp; 
  }

  // apply open loop forward physical plant model
  lf = ROUND(lim * s2p * lsp);
  lf = __max(-lim, __min(lf, lim));
  rt = ROUND(lim * s2p * rsp);
  rt = __max(-lim, __min(rt, lim));

  // use "signed duty cycle" PWM dual command with wheel powers
  pod[n++] = 0x80;
  pod[n++] = 34;          
  pwm = (US16) ROUND(rt);                   // M1
  pod[n++] = (UC8)((pwm >> 8) & 0xFF);
  pod[n++] = (UC8)( pwm & 0xFF);
  pwm = (US16) ROUND(lf);                   // M2
  pod[n++] = (UC8)((pwm >> 8) & 0xFF);
  pod[n++] = (UC8)( pwm & 0xFF);

  // generate checksum 
  start_crc(pod, n);
  n = set_crc(pod, n);

  // send packet but do not wait for completion 
  bcom.TxArray(pod, n);
  pend++;                  // strip ack later
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Current Information                         //
///////////////////////////////////////////////////////////////////////////

//= Reset odometry so current direction is angle zero and path length zero.
// also resets Cartesian coordinates to (0, 0) and x axis points forward
// NOTE: does not block, pod transmit takes about 0.8ms = 3 * 10 / 38400

int jhcEliBase::Zero ()
{
  int n = 0;

  // make sure hardware is working
  BBARF(-1, fail_pend());

  // reset internal variables
  xpos = 0.0;
  ypos = 0.0;
  trav = 0.0;
  head = 0.0;

  // clear previous encoder values
  lf0 = 0;
  rt0 = 0;
  lf  = 0;
  rt  = 0;

  // create zeroing command
  pod[n++] = 0x80;
  pod[n++] = 20;
  start_crc(pod, n);
  n = set_crc(pod, n);

  // send command then wait for and strip off any acknowledgement
  bcom.TxArray(pod, n);
  BBARF(0, fail_ack(1));
  return 1;
}


//= Move some point to adjust for recent motion of base (since last step).
// (tx0 ty0) is origin of (tx ty) coord system wrt center of wheelbase
// assumes y points forward and x points to the right

void jhcEliBase::AdjustXY (double& tx, double& ty, double tx0, double ty0) const
{
  double r = D2R * dr, c = cos(r), s = sin(r);
  double x = (tx - tx0) - dx0, y = (ty - ty0) - dy0;

  tx =  x * c + y * s + tx0;
  ty = -x * s + y * c + ty0;
}


//= Changes a static target location (in place) based on recent motion of the base.
// assumes origin of coordinate system for given vector is midway between wheels
// does not alter Z (no interaction with lift stage)

void jhcEliBase::AdjustTarget (jhcMatrix& pos) const
{
  double tx = pos.X(), ty = pos.Y();

  AdjustXY(tx, ty);
  pos.SetVec3(tx, ty, pos.Z());
}


//= Change planar angle (e.g. object orientation) if base rotates.
// adjusts in place and returns new value for convenience

double jhcEliBase::AdjustAng (double& ang) const
{
  double adj = ang - dr;

  if (adj > 180.0)
    adj -= 360.0;
  else if (adj <= -180.0)
    adj += 360.0;
  ang = adj;
  return ang;
}


///////////////////////////////////////////////////////////////////////////
//                         Goal Specification                            //
///////////////////////////////////////////////////////////////////////////

//= Drive to particular odometric goal, not relative to current state.
// if turn rate = 0 then copies move rate
// negative rate does not scale acceleration (for snappier response)
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliBase::DriveAbsolute (double tr, double hd, double m_rate, double t_rate, int bid)
{
  double r = ((t_rate != 0) ? t_rate : m_rate);
  int mok, tok;

  mok = MoveAbsolute(tr, m_rate, bid);
  tok = TurnAbsolute(hd, r, bid);
  return __min(mok, tok);
}


//= Drive until a particular cumulative path distance has been reached.
// negative rate does not scale acceleration (for snappier response)
// bid value must be greater than previous command to take effect
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliBase::MoveAbsolute (double tr, double rate, int bid) 
{
  if (bid <= mlock)
    return 0;
  mlock = bid;
  stiff = 1;
  mctrl.RampTarget(tr, rate);
  return 1;
}


//= Turn until a particular cumulative angle has been reached.
// negative rate does not scale acceleration (for snappier response)
// bid value must be greater than previous command to take effect
// NOTE: can command multiple revolutions, e.g. hd = hd0 + 720
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliBase::TurnAbsolute (double hd, double rate, int bid)
{
  if (bid <= tlock)
    return 0;
  tlock = bid;
  stiff = 1;
  tctrl.RampTarget(hd, rate);
  return 1;
}


//= Directly set movement velocity both forward and backward (negative).

int jhcEliBase::SetMoveVel (double ips, int bid)
{
  double rate = fabs(ips) / mctrl.vstd, dist = ((ips < 0.0) ? -60.0 : 60); 

  return MoveTarget(dist, rate, bid);
}


//= Directly set turn velocity both left and right (negative).

int jhcEliBase::SetTurnVel (double dps, int bid)
{
  double rate = fabs(dps) / tctrl.vstd, ang = ((dps < 0.0) ? -180.0 : 180.0);

  return TurnTarget(ang, rate, bid);
}


///////////////////////////////////////////////////////////////////////////
//                           Base Extras                                 //
///////////////////////////////////////////////////////////////////////////

//= Estimate time (in seconds) to reach target from given start position.
// if turn rate = 0 then copies move rate
// assumes base is currently at zero velocity (i.e. move start)
// negative rate does not scale acceleration (for snappier response)
// timeout should be about 1.5x this

double jhcEliBase::DriveAbsTime (double tr2, double hd2, double tr1, double hd1, 
                                 double m_rate, double t_rate) const
{
  double mt, tt, r = ((t_rate != 0.0) ? t_rate : m_rate);

  mt = MoveAbsTime(tr2, tr1, m_rate);
  tt = TurnAbsTime(hd2, hd1, r);
  return __max(mt, tt);
}


//= Pick single rate to achieve new position and reorientation within given time.
// if secs < 0 then does not scale acceleration (for snappier response)
// based on trapezoidal profile from stopped to stopped
// Note: use MoveAbsRate and TurnAbsRate to get simultaneous completion

double jhcEliBase::DriveAbsRate (double tr2, double hd2, double tr1, double hd1, double secs, double rmax) const
{
  double mr, tr;
  
  mr = MoveAbsRate(tr2, tr1, secs, rmax);
  tr = TurnAbsRate(hd2, hd1, secs, rmax);
  return __max(mr, tr);
}


///////////////////////////////////////////////////////////////////////////
//                             Nose Light                                //
///////////////////////////////////////////////////////////////////////////

//= Request that light under head be on or off.
// part of base (not neck) because driven by serial port handshake line
// returns 1 if newly set, 0 if pre-empted by higher priority

int jhcEliBase::AttnLED (int on, int bid)
{
  if (bid <= llock)
    return 0;
  llock = bid;
  led = on;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                           Blocking Routines                           //
///////////////////////////////////////////////////////////////////////////

//= Move the base a certain distance and change the heading a certain amount.
// returns 1 for success, zero or negative for error 
// Note: BLOCKS until move finishes 

int jhcEliBase::Drive (double dist, double degs)
{
  double tr0, hd0;
  int tup = 9, ms = 33, ok = 1;

  // check hardware and referesh state 
  if (Update() <= 0)
    return -1;

  // set overall goal 
  tr0 = MoveGoal(dist);
  hd0 = TurnGoal(degs);
  DriveAbsolute(tr0, hd0);

  // drive base until timeout
  while (!DriveClose())
  {
    // reiterate command
    DriveAbsolute(tr0, hd0);

    // change wheel speeds if needed then wait
    Issue(0.001 * ms);
    jms_sleep(ms - tup);
    Update();
  }
    
  // stop base and report if timeout occurred
  DriveClear();
  Limp();
  return ok;
}

