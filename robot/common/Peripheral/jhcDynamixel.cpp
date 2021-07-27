// jhcDynamixel.cpp : sends commands to Robotis Dynamixel servo actuators
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2019 IBM Corporation
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

#include <stdio.h>
#include <math.h>

#include "Interface/jhcMessage.h"
#include "Interface/jms_x.h"

#include "Peripheral/jhcDynamixel.h"


///////////////////////////////////////////////////////////////////////////

//= A commanded position of 0x3FF translates to a 300 degree rotation.
// this is the integer command for each degree of position

const double SV_POS = 0x3FF / 300.0;


//= A commanded velocity of 0x3FF translates to 114 rpm (= 684 degs/sec).
// this is the integer command for each degree/second of speed 

const double SV_VEL = 0x3FF / (6.0 * 114.0);


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.
// this function uses special faster FTDI drivers allowing 1M baud
// and latency and packet turnaround is much better than serial

jhcDynamixel::jhcDynamixel (int port, int rate) 
{
  int i;
  
  // clear communications buffers then write header
  for (i = 0; i < 256; i++)
  {
    up[i] = 0;
    dn[i] = 0;
  }
  dn[0] = 0xFF;
  dn[1] = 0xFF;
  
  // initialize options and state
  noisy = 0;
  retry = 2;
  pic = 0x7F;

  // clear TX and RX state
  Reset();
 
  // possibly open a serial port (big rxn for mega-upload)
  if (SetSource(port, rate, 209) > 0)
    err = 0;
}


//= Default destructor does necessary cleanup.

jhcDynamixel::~jhcDynamixel ()
{
}


//= Clear internal state.

void jhcDynamixel::Reset ()
{
  // normal packet reception
  fill = 0;
  err = 1;
  rc = 0;

  // MegaUpdate values
  m0 = 0;
  mcnt = 0;
  mpod = 0;
  mfail = 0;
}


///////////////////////////////////////////////////////////////////////////
//                             System Status                             //
///////////////////////////////////////////////////////////////////////////

//= Returns main motor supply voltage (to nearest 100mv).

double jhcDynamixel::Voltage (int id)
{
  double v = 0.0;
  int v10;

  if ((v10 = Read8(id, 0x2A)) >= 0)
    v = 0.1 * v10;
  return v;
}


///////////////////////////////////////////////////////////////////////////
//                              Mode Commands                            //
///////////////////////////////////////////////////////////////////////////

//= Initialize settings on some servo (e.g. recover from automatic shutdown).
// otherwise only way to clear a shutdown is to cold boot the servo
// NOTE: ID is temporarily assigned to 1 so place no other servo there
// NOTE: only works at 1M baud because of intrinsic servo reset

int jhcDynamixel::Init (int id, int chk)
{
  if ((id < 0) || (id >= 254))
    return -3;

  // skip if no overload error latched
  if (chk > 0) 
    if (Ping(id) > 0)
      if ((rc & 0x20) == 0x00)
        return 1;

  // cold boot and configure servo
  ResetServo(id);
  jms_sleep(250);               // cold boot time
  Write8(1, 0x03, id);          // reassign servo 1 to this ID
  jms_sleep(20);                // eeprom write time
  Write8(id, 0x05, 5);          // cut return delay to 10 usec
  jms_sleep(20);                // eeprom write time
  return SetStop(id, 0x24);     // over current and temperature
}


//= Set the maximum motion limits for some servo.

int jhcDynamixel::SetLims (int id, double deg0, double deg1, int chk)
{
  int data[2];
  double bot, top;

  if ((chk > 0) && (GetLims(bot, top, id) > 0))
    if ((deg0 == bot) && (deg1 == top))
      return 1;
  data[0] = degs2pos(deg0);
  data[1] = degs2pos(deg1);
  if (WriteArr16(id, 0x06, data, 2) <= 0)
    return 0;
  jms_sleep(20);                              // eeprom write time
  return 1;
}


//= Set deadband for position controller (in degrees).

int jhcDynamixel::SetMargin (int id, double ccw, double cw)
{
  int data[2];
  int val;

  val = ROUND(cw * SV_POS);
  data[0] = __max(0, __min(val, 254));
  val = ROUND(ccw * SV_POS);
  data[1] = __max(0, __min(val, 254));
  return WriteArr8(id, 0x1A, data, 2);  
}


//= Set compliance slope (in degrees) for softer stops.
// <pre>
// discrete steps: 0.0, 0.3, 0.9, 1.5, 2.6, 5.0, 9.7, 19.1   
//                  *    *    *    *    *    *    *    *
//                  |--0-|--1-|--2-|--3-|--4-|--5-|--6-|--7-|
// typical value:     0.2  0.5   1    2    4    7   10   20
// </pre>

int jhcDynamixel::SetSlope (int id, double ccw, double cw)
{
  int data[2];
  int val;

  val = ROUND(cw * SV_POS);
  data[0] = __max(1, __min(val, 254));
  val = ROUND(ccw * SV_POS);
  data[1] = __max(1, __min(val, 254));
  return WriteArr8(id, 0x1C, data, 2);  
}


//= Set minimum error response current fraction (default = 0.031).

int jhcDynamixel::SetPunch (int id, double f)
{
  int val = ROUND(f * 1023.0);

  val = __max(0, __min(val, 0x3FF));
  return Write16(id, 0x30, val);
}


//= Set servo to wheel mode so SetSpeed commands will work.
// useful primarily for gripper

int jhcDynamixel::SetSpin (int id, int chk)
{
  return SetLims(id, -150.0, -150.0, chk);
}


//= Set under what conditions the servo should shutdown.
// state = instruction overload checksum : range heat angle voltage

int jhcDynamixel::SetStop (int id, int state, int chk)
{
  int data[2] = {state, state};
  
  if ((chk > 0) && (GetStop(id) == state))
    return 1;
  if (WriteArr8(id, 0x11, data, 2) <= 0)  // set LED also
    return 0;
  jms_sleep(20);                          // eeprom write time 
  return 1;
} 


//= Find current position limits of servo.
// both values being zero means servo is in free-spinning "wheel" mode
// returns 0 or negative if error

int jhcDynamixel::GetLims (double& deg0, double& deg1, int id)
{
  int data[2];

  if (ReadArr16(id, 0x06, data, 2) <= 0)
    return 0;
  deg0 = pos2degs(data[0]);
  deg1 = pos2degs(data[1]);
  return 1;
}


//= See if servo set for wheel mode or not.
// returns 0 or negative if error

int jhcDynamixel::GetSpin (int id)
{
  double lo, hi;

  if (GetLims(lo, hi, id) <= 0)
    return -1;
  if ((lo == -150.0) && (hi == -150.0))
    return 1;
  return 0;
}


//= Tell under what conditions the servo will shutdown.
// state = instruction overload checksum : range heat angle voltage
// returns negative if error

int jhcDynamixel::GetStop (int id)
{
  return Read8(id, 0x12);
}


///////////////////////////////////////////////////////////////////////////
//                           Multi-Joint Read                            //
///////////////////////////////////////////////////////////////////////////

//= Prompt PIC network processor to solicit states of multiple servos.
// result is one giant pod of all requests and responses (20 bytes per servo)
// gets position, velocity, and force for servos ID0 through IDN inclusive
//
//   USB  => FF FF 7F 04 - 84 01 03 : ch              (cmd to PIC @ 7F, acc = 85)
//
//   PIC  <- FF FF 7F 05 - xp yp x0 y0 : ch           (optional accelerometers if 85) 
//
//   PIC   * FF FF 01 04 - 02 24 06 : ch              (read from servo 1)
//   AX12 <- FF FF 01 08 - ee p0 p1 v0 v1 f0 f1 : ch  (pos, vel, and force)
//   PIC   * FF FF 02 04 - 02 24 06 : ch              (read from servo 2)
//   AX12 <- FF FF 02 08 - ee p0 p1 v0 v1 f0 f1 : ch  (pos, vel, and force)
//   PIC   * FF FF 03 04 - 02 24 06 : ch              (read from servo 3)
//   AX12 <- FF FF 03 08 - ee p0 p1 v0 v1 f0 f1 : ch  (pos, vel, and force)
//
// returns positive if successful, 0 if no response, negative if error
// automatically retries request if fails the first time
// about twice as fast for set of 10 servos (4.8ms with 2.5% retries)

int jhcDynamixel::MegaUpdate (int id0, int idn, int base)
{
  int i, rc;

  for (i = 0; i <= retry; i++)
  {
    if ((rc = MegaIssue(id0, idn, base)) <= 0)
      return rc;
    if (MegaCollect() > 0)
      return(i + 1);
  }
  return 0;
}


//= Send request for state of multiple servos.
// can optionally read base accelerometers also
// takes about 5ms for data to be ready to read

int jhcDynamixel::MegaIssue (int id0, int idn, int base)
{
  int cnt = __max(0, idn - id0 + 1);

  // compute amount of serial traffic to be captured
  acc = ((base > 0) ? 9 : 0);
  nup = 20 * cnt + acc;

  // check for valid arguments and record decoding info
  if ((pic < 0) || (pic > 0xFD) || (nup <= 0))
    return -2;
  m0 = id0;

  // send prompt to PIC processor 
  set_cmd(pic, ((base <= 0) ? 0x84 : 0x85), 2);
  set_arg(0, id0);
  set_arg(1, idn);
  if (tx_pod() <= 0)                // zeroes mcnt
    return -1;
  mpod++;
  return 1;
}


//= Pickup state of multiple servos.
// if fails then retry must be done explicitly

int jhcDynamixel::MegaCollect ()
{
  UC8 *pod = up + acc;
  int i;

  // get queries and responses for multiple servos
  rc = 0;
  err = 0;
  if ((mcnt = RxArray(up, nup)) != nup)
  {
    mfail++;
    return 0;        // too few bytes
  }

  // possibly show traffic
  if (noisy > 0)
  {
    if (acc > 0)
      print_pod(up, acc, " <-");
    for (i = acc; i < nup; i += 20, pod += 20)
    {
      print_pod(pod,      8, "  *");
      print_pod(pod + 8, 12, " <-");
    }   
  }
  return 1;
}


//= Looks in mega-response pod for information about this servo.
// returns 1 if all info found, 0 or negative if failed

int jhcDynamixel::chk_mega (double& degs, double& dps, double& frac, int id)
{
  UC8 *pod;
  int i, n = 20 * (id - m0) + acc, chk = ~(id + 0x04 + 0x02 + 0x24 + 0x06) & 0xFF;

  // check for a big enough mega-return (possibly including accelerometers)
  if ((pic < 0) || (id < m0) || (mcnt <= 0) || (mcnt < (n + 20)))
    return -3;

  // check for proper request packet
  pod = up + n;
  if ((pod[0] != 0xFF) || (pod[1] != 0xFF) || (pod[2] != id)   || (pod[3] != 0x04) ||
      (pod[4] != 0x02) || (pod[5] != 0x24) || (pod[6] != 0x06) || (pod[7] != chk))
    return -2;

  // check for proper return packet
  pod += 8;
  if ((pod[0] != 0xFF) || (pod[1] != 0xFF) || (pod[2] != id) || (pod[3] != 0x08))
    return -1;

  // validate return checksum
  chk = 0;
  for (i = 2; i <= 10; i++)
    chk += pod[i];
  chk = ~chk & 0xFF;
  if (pod[11] != chk)
    return 0;

  // extract values
  rc = pod[5];
  degs = pos2degs((pod[6]  << 8) | pod[5]);
  dps  = vel2dps( (pod[8]  << 8) | pod[7]);
  frac = pwm2frac((pod[10] << 8) | pod[9]);
  return 1;
}


//= Extract raw accelerometer peaks and averages from PIC controller packet.
// hw accel X points forward, Y points to left (i.e. rotated +90 degs)
// peaks are 16.13mG/bit, averages are 4.03mG/bit (zero @ approx)
// returns 1 if valid, 0 or negative otherwise

int jhcDynamixel::RawAccel (int& xpk, int& ypk, int& xav4, int& yav4) const
{
 int i, chk = 0;

  // check for recent mega command
  if ((pic < 0) || (acc <= 0) || (mcnt < acc))
    return -2;

  // check for proper return packet
  if ((up[0] != 0xFF) || (up[1] != 0xFF) || (up[2] != pic) || (up[3] != (acc - 4)))
    return -1;

  // validate return checksum
  for (i = 2; i <= (acc - 2); i++)
    chk += up[i];
  chk = ~chk & 0xFF;
  if (up[acc - 1] != chk)
    return 0;

  // bind values
  xpk  = up[4];
  ypk  = up[5];
  xav4 = up[6];
  yav4 = up[7];
  return 1;
}


//= Send special command to PIC board to get robot serial number.
//   USB  => FF FF 7F 02 - 86 : ch              (cmd to PIC @ 7F)
//   PIC  <- FF FF 7F 02 - id : ch 
// returns negative for problem, 0 if not programmed, else ID

int jhcDynamixel::RobotID ()
{
  int chk;

  // send special hardware ID command
  set_cmd(pic, 0x86, 0);
  if (tx_pod() <= 0)                
    return -4;

  // check for proper return packet
  if (RxArray(up, 6) != 6)
    return -3;

  if (noisy > 0)
    print_pod(up, 6, " <-");
  if ((up[0] != 0xFF) || (up[1] != 0xFF) || (up[2] != pic) || (up[3] != 0x02))
    return -2;

  // validate return checksum
  chk = up[2] + up[3] + up[4];
  chk = ~chk & 0xFF;
  if (up[5] != chk)
    return -1;

  // extract answer (8 bits)
  return up[4];
}


///////////////////////////////////////////////////////////////////////////
//                              Joint Status                             //
///////////////////////////////////////////////////////////////////////////

//= Return current angular position of servo in degrees.
// reports angle wrt neutral center position (+/- 150 degrees)
// returns 0 or negative if error
  
int jhcDynamixel::GetPos (double& degs, int id)
{
  int pos;

  if ((pos = Read16(id, 0x24)) < 0)
    return 0;
  degs = pos2degs(pos);
  return 1;
}


//= Return current angular velocity of servo in degrees per second.
// returns 0 or negative if error

int jhcDynamixel::GetVel (double& dps, int id)
{
  int vel;

  if ((vel = Read16(id, 0x26)) < 0)
    return 0;
  dps = vel2dps(vel);
  return 1;
}


//= Return current force being exerted by servo.
// reports force in PWM percentage on-time (1.0 = 220 oz-in approx).
// returns 0 or negative if error

int jhcDynamixel::GetForce (double& frac, int id)
{
  int pwm;

  if ((pwm = Read16(id, 0x28)) < 0)
    return 0;
  frac = pwm2frac(pwm);
  return 1;
}


//= Return current position and velocity of servo.
// reports angle wrt neutral center position (+/- 150 degrees).
// reports force in PWM percentage on-time (1.0 = 220 oz-in approx).
// returns 0 or negative if error

int jhcDynamixel::GetPosVel (double& degs, double& dps, int id)
{
  int data[2];

  if (ReadArr16(id, 0x24, data, 2) <= 0)
    return 0;
  degs = pos2degs(data[0]);
  dps  = vel2dps( data[1]);
  return 1;
}


//= Return current position, velocity, and force of servo.
// reports angle wrt neutral center position (+/- 150 degrees).
// reports force in PWM percentage on-time (+/- 1.0 = 220 oz-in approx)
// takes about 2ms total (= 1ms tx_pod + 1ms rx_pod) @ 1Mbaud
// 8 byte request + 12 byte return = 200 bits -> 1.7ms @ 115.2Kbaud
// can read from faster bulk update if MegaUpdate called before
// returns 0 or negative if error

int jhcDynamixel::GetState (double& degs, double& dps, double& frac, int id)
{
  int data[3];

  if (chk_mega(degs, dps, frac, id) > 0)
    return 1;
  if (ReadArr16(id, 0x24, data, 3) <= 0)
    return 0;
  degs = pos2degs(data[0]);
  dps  = vel2dps( data[1]);
  frac = pwm2frac(data[2]);
  return 2;
}


//= Quickly get states of four servos (4.5ms for 8 servos vs 8ms normally).
// sends out 4 full commands then uses servo response delays to gather results
// works fairly reliably with delays = 100, 250, 370, 508us (in order)
// delays seem to be relative to ID check of most recent packet
// only reasonable at fastest serial rate (1M = 921600 baud)
// <pre>
//           time (on Dynamixel serial bus)
//         +--------------------------------------------> 
//                                           
//  HOST:    [T0][T1][T2][T3]
//             |   |   |   |       508
//   SV3:      |   |   |   +----------------> [R3]
//             |   |   |       370
//   SV2:      |   |   +---------------> [R2]
//             |   |       250
//   SV1:      |   +--------------> [R1]
//             |       100
//   SV0:      +-------------> [R0]
//
// </pre>
// returns a bit vector for valid reads, e.g. 12 = 0x0C = servos 0 and 1 bad

int jhcDynamixel::QuadState (double *degs, double *dps, double *frac, int *flag, int *id)
{
  UC8 rx[48], tx[32] = {0xFF, 0xFF, 0x00, 0x04, 0x02, 0x24, 0x06, 0xCF,
                        0xFF, 0xFF, 0x00, 0x04, 0x02, 0x24, 0x06, 0xCF,
                        0xFF, 0xFF, 0x00, 0x04, 0x02, 0x24, 0x06, 0xCF,
                        0xFF, 0xFF, 0x00, 0x04, 0x02, 0x24, 0x06, 0xCF};
  UC8 *arr;
  int i, j, sum, ok = 0;

  // check for valid arguments
  if (id == NULL)
    return -3;
  for (i = 0; i < 4; i++)
    if ((id[i] == 254) || (id[i] == -1))
      return -3;

  // alter packet for correct IDs and checksums
  arr = tx;
  for (i = 0; i < 4; i++, arr += 8)
  {
    arr[2] =  (UC8) id[i];
    arr[7] -= (UC8) id[i];
  }

  // send big packet and wait for all responses
  if (TxArray(tx, 32) <= 0)
    return -2;
  if (RxArray(rx, 48) < 48)
    return -1;

  // see which returns are okay
  arr = rx;
  for (i = 0; i < 4; i++, arr += 12)
  {
    // check expected header structure, ID, and length
    if ((arr[0] != 0xFF)  || (arr[1] != 0xFF) || 
        (arr[2] != id[i]) || (arr[3] != 8))
      continue;

    // make sure checksum matches
    sum = 0;
    for (j = 2; j <= 10; j++) 
      sum += (int)(arr[j]);
    sum = (~sum) & 0xFF;
    if (arr[11] != sum)
      continue;

    // mark as valid and parse data into return arrays
    ok |= (0x01 << i);
    if (flag != NULL)
      flag[i] = arr[4];        // overheating, etc.
    if (degs != NULL)
      degs[i] = pos2degs((arr[6]  << 8) | arr[5]);
    if (dps != NULL)
      dps[i]  = vel2dps( (arr[8]  << 8) | arr[7]);
    if (frac != NULL)
      frac[i] = pwm2frac((arr[10] << 8) | arr[9]);
  }

  // return bit vector of valid reads
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                      Joint and Wheel Commands                         //
///////////////////////////////////////////////////////////////////////////

//= Turn off power to servo so it can be backdriven.
// alternatively set doit > 0 to freeze at current position

int jhcDynamixel::Limp (int id, int doit)
{
  return Write8(id, 0x18, ((doit > 0) ? 0 : 1));
}


//= Set the goal position of some servo to a particular value.
// angles are +/- 150 relative to center

int jhcDynamixel::SetPos (int id, double degs)
{
  return Write16(id, 0x1E, degs2pos(degs));
}


//= Set the goal position and velocity to get there for some servo.
// angles are +/- 150 relative to center, max speed about 684 degs/sec

int jhcDynamixel::SetPosVel (int id, double degs, double dps)
{
  int data[2] = {degs2pos(degs), dps2vel(dps)}; 

  return WriteArr16(id, 0x1E, data, 2);
}


//= Set signed speed for wheel (servo) to turn.
// this seems to be PWM force (not so useful) vs. true speed
// most feedback (position, speed, force) not available in this mode

int jhcDynamixel::SetSpeed (int id, double dps)
{
  return Write16(id, 0x20, dps2vel(dps));
}


//= Disable a bunch of servos quickly with one packet transmitted.
// takes an array of n different ids for which servos to set to limp
// alternatively set doit <= 0 to stiffen at current position

int jhcDynamixel::MultiLimp (int *id, int n, int doit)
{
  int i, t = ((doit > 0) ? 0 : 1);

  if (n <= 0)
    return 1;
  set_cmd(-1, 0x83, 2 * n + 2);
  set_arg(0, 0x18);                 // torque enable register
  set_arg(1, 1); 
  for (i = 0; i < n; i++)
  {
    dn[2 * i + 7] = (UC8)(id[i] & 0xFF);
    dn[2 * i + 8] = (UC8) t;
  }
  return tx_pod();
}


//= Write new goal positions to multiple servos.
// angles are +/- 150 relative to center

int jhcDynamixel::MultiPos (int *id, double *degs, int n)
{
  int i, pos;

  if (n <= 0)
    return 1;
  set_cmd(-1, 0x83, 3 * n + 2);
  set_arg(0, 0x1E);                 // goal position register
  set_arg(1, 2); 
  for (i = 0; i < n; i++)
  {
    pos = degs2pos(degs[i]);
    dn[3 * i + 7] = (UC8)(id[i] & 0xFF);
    dn[3 * i + 8] = (UC8)(pos & 0xFF);         // LSB
    dn[3 * i + 9] = (UC8)((pos >> 8) & 0xFF);  // MSB
  }
  return tx_pod();
}


//= Write new goal positions and velocities to multiple servos.
// angles are +/- 150 relative to center, max speed about 684 degs/sec
// NOTE: must call MultiSend after this to transmit (can wait for more appends)

int jhcDynamixel::MultiPosVel (int *id, double *degs, double *dps, int n)
{
  UC8 *vals;
  int i, pos, vel;

  // see if any servos, then check if download packet beyond bounds (255)
  if (n <= 0)
    return 1;
  if ((fill + n) > 48)
    return Complain("Bad fill (%d) in jhcDynamixel::MultiPosVel", fill + n);

  // set up to alter goal position register if start of new command
  set_cmd(-1, 0x83, 5 * (fill + n) + 2);
  if (fill == 0)
  {
    set_arg(0, 0x1E);                
    set_arg(1, 4); 
  }

  // add in commands for individual servos
  vals = dn + (5 * fill) + 7;
  for (i = 0; i < n; i++, vals += 5)
  {
    pos = degs2pos(degs[i]);
    vel = dps2vel(dps[i]);
    vals[0] = (UC8)(id[i] & 0xFF);
    vals[1] = (UC8)(pos & 0xFF);         // LSB
    vals[2] = (UC8)((pos >> 8) & 0xFF);  // MSB
    vals[3] = (UC8)(vel & 0xFF);         // LSB
    vals[4] = (UC8)((vel >> 8) & 0xFF);  // MSB
  }
  fill += n;
  return 1;
}


//= Finish up sequence even if no new data written.
// useful when only neck or arm is in limp mode

int jhcDynamixel::MultiSend ()
{
  if (fill <= 0)
    return 1;
  return tx_pod();
}


//= Write new speeds to multiple servos.
// this seems to be PWM force (not so useful) vs. true speed
// most feedback (position, speed, force) not available in this mode

int jhcDynamixel::MultiSpeed (int *id, double *dps, int n)
{
  int i, vel;

  if (n <= 0)
    return 1;
  set_cmd(-1, 0x83, 3 * n + 2);
  set_arg(0, 0x20);                    // goal velocity register
  set_arg(1, 2);
  for (i = 0; i < n; i++)
  {
    vel = dps2vel(dps[i]);
    dn[3 * i + 7] = (UC8)(id[i] & 0xFF);
    dn[3 * i + 8] = (UC8)( vel & 0xFF);        // LSB
    dn[3 * i + 9] = (UC8)((vel >> 8) & 0xFF);  // MSB
  }
  return tx_pod();
}


///////////////////////////////////////////////////////////////////////////
//                           Unit Conversions                           //
///////////////////////////////////////////////////////////////////////////

//= Converts a position in degrees (+/- 150) to a Dynamixel value.

int jhcDynamixel::degs2pos (double degs)
{
  int pos = ROUND((degs + 150.0) * SV_POS);
  
  return __max(0, __min(pos, 0x3FF));
}


//= Converts a velocity in degrees per second to a Dynamixel value.
// can take negative values for wheel mode
// Note: never generates 0 since this is fastest!

int jhcDynamixel::dps2vel (double dps)
{
  int vel = ROUND(fabs(dps) * SV_VEL);
  int v = __max(1, __min(vel, 0x3FF));

  if (dps < 0.0)
    v |= 0x400;
  return v;
}


//= Converts a fractional force command into a Dynamixel value.
// conceptually this is always a positive number (no force servoing)

int jhcDynamixel::frac2pwm (double frac)
{
  int f = ROUND(1023.0 * frac);
  
  return __max(0, __min(f, 0x3FF));
}


//= Converts a Dynamixel position reading into degrees (+/- 150).
// normal output range 0 = 0, 0x3FF = 300 degs, 0x1FF = middle

double jhcDynamixel::pos2degs (int pos)
{
  return((pos / SV_POS) - 150.0);
}


//= Converts a Dynamixel velocity reading into degrees per second.
// servos report value as an 11 bit sign + value number

double jhcDynamixel::vel2dps (int vel)
{
  int mag = vel & 0x03FF;

  if ((vel & 0x0400) != 0)
    mag = -mag;
  return(mag / SV_VEL);
}


//= Converts a Dynamixel force reading into a fraction.
// servos report value as a sign + 10 bit magnitude

double jhcDynamixel::pwm2frac (int pwm)
{
  int mag = pwm & 0x03FF;

  if ((pwm & 0x0400) != 0)
    mag = -mag;
  return(mag / -1024.0);
}


///////////////////////////////////////////////////////////////////////////
//                            Low Level Commands                         //
///////////////////////////////////////////////////////////////////////////

//= Reset some servo to factory defaults.
// Note: always forces this servo to be id = 1 at least temporarily
// returns 1 if okay

int jhcDynamixel::ResetServo (int id)
{
  set_cmd(id, 0x06);
  return cmd_ack();
}


//= Get status packet from some servo.
// status code saved in internal variable rc 
// returns 1 if okay (not status value)
// Note: Dynamixel servos come preset for 1M baud, not 256K baud (Win max)

int jhcDynamixel::Ping (int id)
{
  set_cmd(id, 0x01);
  return cmd_ack();
}


//= Reads a single 8 bit value from some servo.
// returns -1 for error else value

int jhcDynamixel::Read8 (int id, int addr)
{
  if ((id < 0) || (id == 254))
    return -2;

  set_cmd(id, 0x02, 2);
  set_arg(0, addr);
  set_arg(1, 1);

  if (cmd_ack() <= 0)
    return -1;
  return get_val8(0);
}


//= Reads a single 16 bit value from some servo.
// returns -1 for error else value

int jhcDynamixel::Read16 (int id, int addr)
{
  if ((id < 0) || (id == 254))
    return -2;

  set_cmd(id, 0x02, 2);
  set_arg(0, addr);
  set_arg(1, 2);

  if (cmd_ack() <= 0)
    return -1;
  return get_val16(0);
}


//= Read a section of 8 bit memory from some servo.
// returns 1 if okay

int jhcDynamixel::ReadArr8 (int id, int addr, int *data, int n)
{
  int i;
  
  if ((id < 0) || (id == 254))
    return -1;

  set_cmd(id, 0x02, 2);
  set_arg(0, addr);
  set_arg(1, n);

  if (cmd_ack() <= 0)
    return 0;
  for (i = 0; i < n; i++)
    data[i] = get_val8(i);
  return 1;
}


//= Read a section of 16 bit memory from some servo.
// returns 1 if okay

int jhcDynamixel::ReadArr16 (int id, int addr, int *data, int n)
{
  int i;

  if ((id < 0) || (id == 254))
    return -1;

  set_cmd(id, 0x02, 2);
  set_arg(0, addr);
  set_arg(1, 2 * n);

  if (cmd_ack() <= 0)
    return -1;
  for (i = 0; i < n; i++)
    data[i] = get_val16(i);
  return 1;
}


//= Write a single 8 bit value to some servo.
// returns 1 if okay, must call Trigger if queued

int jhcDynamixel::Write8 (int id, int addr, int val, int queue)
{
  int cmd = ((queue > 0) ? 0x04 : 0x03);

  set_cmd(id, cmd, 2);
  set_arg(0, addr);
  set_val8(0, val);

  return cmd_ack();
}


//= Write a single 16 bit value to some servo.
// returns 1 if okay, must call Trigger if queued

int jhcDynamixel::Write16 (int id, int addr, int val, int queue)
{
  int cmd = ((queue > 0) ? 0x04 : 0x03);

  set_cmd(id, cmd, 3);
  set_arg(0, addr);
  set_val16(0, val);

  return cmd_ack();
}


//= Write a section of memory in some servo with 8 bit data.
// returns 1 if okay, must call Trigger if queued

int jhcDynamixel::WriteArr8 (int id, int addr, int *data, int n, int queue)
{
  int i, cmd = ((queue > 0) ? 0x04 : 0x03);

  set_cmd(id, cmd, n + 1);
  set_arg(0, addr);
  for (i = 0; i < n; i++)
    set_val8(i, data[i]);

  return cmd_ack();
}


//= Write a section of memory in some servo with 16 bit data.
// returns 1 if okay, must call Trigger if queued

int jhcDynamixel::WriteArr16 (int id, int addr, int *data, int n, int queue)
{
  int i, cmd = ((queue > 0) ? 0x04 : 0x03);

  set_cmd(id, cmd, 2 * n + 1);
  set_arg(0, addr);
  for (i = 0; i < n; i++)
    set_val16(i, data[i]);

  return cmd_ack();
}


//= Activate a queued register write to some servo.
// returns 1 if okay

int jhcDynamixel::Trigger (int id)
{
  set_cmd(id, 5);
  return cmd_ack();
}


///////////////////////////////////////////////////////////////////////////
//                           Packet Formation                            //
///////////////////////////////////////////////////////////////////////////

//= Write command (no arguments) to packet being assembled.
// a negative id means to use the broadcast address

void jhcDynamixel::set_cmd (int id, int cmd, int argc)
{
  dn[2] = ((id < 0) ? 0xFE : (UC8)(id & 0xFF));  // id
  dn[3] = (UC8)((argc + 2) & 0xFF);              // length
  dn[4] = (UC8)(cmd & 0xFF);                     // instruction
}


//= Set up packet byte for a particular argument.

void jhcDynamixel::set_arg (int n, int val)
{
  dn[n + 5] = (UC8)(val & 0xFF);
}


//= Set up packet with 8 bit value.
// assumes an 8 bit address and then all 8 bit values

void jhcDynamixel::set_val8 (int n, int val)
{
  int v = __max(0, __min(val, 255));

  dn[n + 6] = (UC8)(v & 0xFF);
}


//= Set up packet with 16 bit value.
// assumes an 8 bit address first then all 16 bit values

void jhcDynamixel::set_val16 (int n, int val)
{
  int v = __max(0, __min(val, 65535));

  dn[2 * n + 6] = (UC8)(v & 0xFF);          // LSB
  dn[2 * n + 7] = (UC8)((v >> 8) & 0xFF);   // MSB
}


//= Get a particular response 8 bit value.
// assumes all return values in pod are 8 bit

int jhcDynamixel::get_val8 (int n)
{
  return((int) up[n + 5]);
}


//= Get a particular response 16 bit value.
// assumes all return values in pod are 16 bit

int jhcDynamixel::get_val16 (int n)
{
  int lsb = get_val8(2 * n), msb = get_val8(2 * n + 1);

  return((int)((msb << 8) + lsb));
}


///////////////////////////////////////////////////////////////////////////
//                             Packet Transfer                           //
///////////////////////////////////////////////////////////////////////////

//= Send current packet and get response (if not broadcast).
// Note: fastest tx then rx cycle is about 1-2ms due to USB
// returns -1 if tx failed, 0 if rx failed, else 1 for okay

int jhcDynamixel::cmd_ack ()
{
  int i;

  for (i = 0; i <= retry; i++)
  {
    if (tx_pod() <= 0)          // send command (again)
      return -1;
    if (dn[2] == 0xFE)          // no ack for broadcast
      return 1;
    if (rx_pod() > 0)           // possibly try again if fails
      return 1;
  }
  return 0;
}


//= Add packet prefix and checksum to already constructed array of bytes.
// main command body starts at pod + 2 and is pod[3] + 1 bytes long
// returns -1 for no port, 0 for sending error, 1 if okay
// <pre>
// example with len = 4 (two args + 2):
//
//           0xFF 0xFF  ID   LEN  CMD ARG0 ARG1  CHK
//   pod[] =   0    1    2    3    4    5    6    7
//    body =             *    *    *    *    *
//                                           n
// </pre>

int jhcDynamixel::tx_pod ()
{
  int i, n = dn[3] + 2, sum = 0;

  // make room for next packet
  fill = 0;
  mcnt = 0;
  if (Connection() <= 0)
    return -1;

  // if some error occurred then purge receive buffer.
  if (err > 0)
  {
    Flush();
    err = 0;
  }

  // compute checksum and add to end of packet
  for (i = 2; i <= n; i++) 
    sum += (int)(dn[i]);
  dn[n + 1] = (UC8)((~sum) & 0xFF);

  // send packet and check for success
  if (noisy > 0)
    print_pod(dn, 0, "==>");
  if (TxArray(dn, n + 2) != (n + 2))     // body + header
    return 0;
  return 1;
}


//= Listen for return and save status code, values, and value count internally.
// Note: assumes rx pod is in response to last tx pod so sets expected length
// main command body starts at pod + 2 and is pod[3] + 1 bytes long
// returns: -3 = no port, -2 = not enough bytes, -1 = bad format,
//          0 = bad checksum, 1 = correct read
// Note: blocks until valid packet received or timeout occurs
// <pre>
// example with len = 2 (no params + 2):
//
//           0xFF 0xFF  ID   LEN  ERR  CHK
//   pod[] =   0    1    2    3    4    5 
//    body =             *    *    *
//                                 n
// </pre>

int jhcDynamixel::rx_pod ()
{
  int i, id = dn[2], n = 4, sum = 0;

  // make sure port works but assume some problem will occur
  if (Connection() <= 0)
    return -3;
  err = 1;

  // try to read expected size based on tx pod
  if (dn[4] == 0x02)                        // possible read command of N bytes
    n += dn[6];
  if ((i = RxArray(up, n + 2)) != (n + 2))  // body + header
    return -2;
  if (noisy > 0)
    print_pod(up, n + 2, " <-");

  // check packet format, servo id, and length
  if ((up[0] != 0xFF) || (up[1] != 0xFF) || 
      (up[2] != id)   || (up[3] != (n - 2)))
    return -1;

  // make sure checksum matches
  for (i = 2; i <= n; i++) 
    sum += (int)(up[i]);
  sum = (~sum) & 0xFF;
  if (up[n + 1] != sum)
    return 0;

  // sensible packet received so no problem
  err = 0;
  rc = up[4];                              // error codes (if any)
  return 1;
}


//= Debugging function prints transmit or receive pod.

void jhcDynamixel::print_pod (UC8 pod[], int n, const char *tag)
{
  int i, cnt = ((n > 0) ? n : pod[3] + 4);

  if ((tag != NULL) && (*tag != '\0'))
    jprintf("%s ", tag);
  jprintf("[ ");
  for (i = 0; i < cnt; i++)
  {
    if ((i > 0) && ((i % 18) == 0))
      jprintf("\n  ");
    if (i == 4) 
      jprintf("- ");
    if (i == (cnt - 1))
      jprintf(": ");
    jprintf("%02X ", pod[i]);
  }
  jprintf("]\n\n");
}
