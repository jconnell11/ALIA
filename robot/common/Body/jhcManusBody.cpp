// jhcManusBody.cpp : basic control of Manus small forklift robot
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 IBM Corporation
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

#include <math.h>

#include "Interface/jhcMessage.h"      // common video
#include "Interface/jms_x.h"      
#include "Video/jhcOcv3VSrc.h"

#include "Body/jhcManusBody.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcManusBody::~jhcManusBody ()
{
  ser.Close();
  if (wifi > 0)
    delete vid;
}


//= Default constructor initializes certain values.
// large transaction pod send to servo control for best speed
//   ask = 4 channel target commands  (4*4 = 16)
//         1 channel speed command    (1*4 = 4)
//         6 channel position request (6*2 = 12)

jhcManusBody::jhcManusBody ()
{
  UC8 *req = ask + 20;
  int i;

  // prepare multiple target commands (start at 0, 4, 8, 12)
  for (i = 0; i < 16; i += 4)
    ask[i] = 0x84;
  ask[1]  = LFW;
  ask[5]  = LIFT;
  ask[9]  = RTW;
  ask[13] = HAND;

  // prepare multiple position requests (all 6 channels)
  for (i = 0; i < 12; i += 2)
  {
    req[i] = 0x90;
    req[i + 1] = (UC8)(i / 2);
  }

  // expect external video source to be bound
  now.SetSize(640, 360, 3);
  vid = NULL;
  wifi = 0;

  // default robot ID and serial port
  port0 = 10;
  id = 0;

  // set processing parameters and initial state
  Defaults();
  clr_state();
}


//= Set image sizes directly.

void jhcManusBody::SetSize (int x, int y)
{
  jhcManusX::SetSize(x, y);
  now.SetSize(frame);
}


///////////////////////////////////////////////////////////////////////////
//                         Processing Parameters                         //
///////////////////////////////////////////////////////////////////////////

//= Parameters describing camera warping and pose.

int jhcManusBody::cam_params (const char *fname)
{
  jhcParam *ps = &cps;
  int ok;

  ps->SetTag("man_cam", 0);
  ps->NextSpecF( &w2,   -3.0, "R^2 warp coefficient");
  ps->NextSpecF( &w4,    6.5, "R^4 warp coefficient");
  ps->NextSpecF( &mag,   0.9, "Magnification");
  ps->NextSpecF( &roll, -1.0, "Roll (deg)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters for distance sensor and miscellaneous reporting.

int jhcManusBody::range_params (const char *fname)
{
  jhcParam *ps = &rps;
  int ok;

  ps->SetTag("man_rng", 0);
  ps->NextSpec4( &v0,  426,   "Sensor close value");
  ps->NextSpecF( &r0,    0.0, "Close range (in)");
  ps->NextSpec4( &v4,  106,   "Sensor middle val");
  ps->NextSpecF( &r4,    4.0, "Middle range (in)");
  ps->NextSpec4( &v12,  64,   "Sensor far value");
  ps->NextSpecF( &r12,   6.0, "Far range (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for interpreting gripper width.

int jhcManusBody::width_params (const char *fname)
{
  jhcParam *ps = &wps;
  int ok;

  ps->SetTag("man_wid", 0);
  ps->NextSpecF( &vmax,  23.0,  "Full open val (us)");
  ps->NextSpecF( &vfat,  99.0,  "Holding fat val (us)");
  ps->NextSpecF( &vmed, 121.5,  "Holding medium val (us)");
  ps->NextSpecF( &vmin, 125.0,  "Full close val (us)");
  ps->NextSpecF( &wfat,   1.7,  "Fat object (in)");  
  ps->NextSpecF( &wmed,   1.4,  "Medium object (in)");  

  ps->NextSpecF( &wsm,    0.94, "Decrease for inner pads (in)");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for controlling wheel motion.

int jhcManusBody::drive_params (const char *fname)
{
  jhcParam *ps = &dps;
  int ok;

  ps->SetTag("man_drive", 0);
  ps->NextSpec4( &lf0,  1484,   "Left zero value (us)");  
  ps->NextSpec4( &rt0,  1484,   "Right zero value (us)");  
  ps->NextSpecF( &vcal,    9.0, "Calibrated speed (ips)");     // cal 18" @ 9 ips 
  ps->NextSpec4( &ccal,  339,   "Diff cmd for speed (us)");    
  ps->NextSpecF( &bal,     0.0, "Right vs. left balance");
  ps->NextSpecF( &sep,     4.2, "Virtual turn circle (in)");   // cal 180 degs @ 9 ips

  ps->NextSpec4( &dacc,   40,   "Acceleration limit");         
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for controlling forklift stage.

int jhcManusBody::lift_params (const char *fname)
{
  jhcParam *ps = &lps;
  int ok;

  ps->SetTag("man_lift", 0);
  ps->NextSpec4( &ldef, 1780,   "Default lift value (us)");  
  ps->NextSpecF( &hdef,    0.3, "Default height (in)");
  ps->NextSpec4( &lout, 1320,   "Horizontal lift val (us)");  
  ps->NextSpecF( &hout,    2.0, "Horizontal height (in)");
  ps->NextSpecF( &arm,     2.5, "Lift arm length (in)");
  ps->Skip();

  ps->NextSpec4( &lsp,   100,   "Lift speed limit");        // was 50
  ps->NextSpec4( &lacc,   15,   "Lift acceleration");       
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Parameters used for controlling gripper.

int jhcManusBody::grip_params (const char *fname)
{
  jhcParam *ps = &gps;
  int ok;

  ps->SetTag("man_grip", 0);
  ps->NextSpec4( &gmax,  433,  "Open gripper value (us)");  
  ps->NextSpec4( &gmin, 2282,  "Closed gripper value (us)");  
  ps->Skip();
  ps->NextSpecF( &wtol,  0.1,  "Offset for closed test (in)");
  ps->NextSpecF( &wprog, 0.05, "Insignificant change (in)");
  ps->NextSpec4( &wstop, 5,    "Count for no motion");

  ps->NextSpec4( &gsp,   100,  "Grip speed limit");
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


///////////////////////////////////////////////////////////////////////////
//                           Parameter Bundles                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcManusBody::Defaults (const char *fname)
{
  int ok = 1;

  ok &= cam_params(fname);
  ok &= range_params(fname);
  ok &= width_params(fname);
  ok &= drive_params(fname);
  ok &= lift_params(fname);
  ok &= grip_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcManusBody::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= cps.SaveVals(fname);
  ok &= rps.SaveVals(fname);
  ok &= wps.SaveVals(fname);
  ok &= dps.SaveVals(fname);
  ok &= lps.SaveVals(fname);
  ok &= gps.SaveVals(fname);
  return ok;
}


//= Possibly change robot ID number then reload calibration parameters.

int jhcManusBody::LoadCfg (const char *dir, int robot, int noisy)
{
  if (robot > 0) 
    id = robot;
  CfgName(dir);
  jprintf(1, noisy, "Reading robot calibration from: %s\n", cfile); 
  return Defaults(cfile);
}


//= Save values to standard file name.

int jhcManusBody::SaveCfg (const char *dir)
{
  return SaveVals(CfgName(dir));
}


//= Get canonical name of configuration file based on retrieved ID.
// can optionally preface name with supplied path
// returns pointer to internal string 

const char *jhcManusBody::CfgName (const char *dir)
{
  if (dir != NULL)
    sprintf_s(cfile, "%s/Manus-%d.cfg", dir, id);
  else
    sprintf_s(cfile, "Manus-%d.cfg", id);
  return cfile;
}


///////////////////////////////////////////////////////////////////////////
//                            Camera Connection                          //
///////////////////////////////////////////////////////////////////////////

//= Bind an external video source to be used.

void jhcManusBody::BindVideo (jhcVideoSrc *v)
{
  if (wifi > 0)
    delete vid;
  wifi = 0;
  vid = v;
  if (vid != NULL)
    vid->SizeFor(frame);
}


//= Bind the SQ13 WiFi cube camera for image acquistion.

int jhcManusBody::SetWifiCam (int rpt)
{
  jhcOcv3VSrc *v;

  // make sure not already bound
  if (wifi > 0)
    return 1;

  // try connecting
  if (rpt > 0)
    jprintf("Connecting to wifi camera ...\n");
  if ((v = new jhcOcv3VSrc("ttp://192.168.25.1:8080/?action=stream.ocv3")) == NULL)
  {
    if (rpt >= 2)
      Complain("Could not connect to SQ13 camera");
    else if (rpt > 0)
      jprintf(">>> Could not connect to SQ13 camera !\n");
    return 0;
  }

  // configure images
  if (rpt > 0)
    jprintf("    ** good **\n\n");
  BindVideo(v);
  wifi = 1;
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Reset state for the beginning of a sequence.
// will also automatically read in correct calibration file from "dir"
// returns 1 if connected, 0 or negative for problem

int jhcManusBody::Reset (int noisy, const char *dir, int prefer)
{
  UC8 pod[2];
  int com = 0;

  // try to connect to particular robot 
  if (prefer > 0)
    com = find_robot(prefer, noisy);

  // set up conversion factors and gripper state
  chan_coefs();
  v2d_eqn();   
  clr_state();
  lvel = 0.0;          // no translation or rotation
  rvel = 0.0;
  wcmd = wmax;         // fully open
  pgrip = 0;

  // reconnect serial port and clear any controller errors
  mok = 0;
  if (com > 0)
    if (ser.Xmit(0xA1) > 0)
    {
      // get response but ignore details (wait required)
      jms_sleep(30);
      if (ser.RxArray(pod, 2) >= 2)
        mok = 1;
 
      // set up initial pose then wait for it to be achieved
      servo_defs();
      rcv_all();
      jms_sleep(500);
      req_all();         // request positions for first Update
    }

  // create image rectification pattern and rewind video (if file)
  wp.InitSize(frame.XDim(), frame.YDim(), 3);
  wp.Rectify(w2, w4, mag, roll);
  if (vid != NULL)
    if (vid->Rewind() <= 0)
      return 0;
  return mok;
}


//= Look for the preferred robot then set the valid port and id.
// if prefer = 0 then scans all standard serial ports
// returns 1 something found, 0 for no robots available

int jhcManusBody::find_robot (int prefer, int noisy)
{
  int rid, rnum = prefer;

  // try port associated with preferred robot or scan all
  if (prefer > 0)
    rid = test_port(rnum, noisy);
  else
    for (rnum = 1; rnum <= 9; rnum++)
      if ((rid = test_port(rnum, noisy)) > 0)
         break;
  
  // save parameters if robot actually found
  if (rid > 0)
  { 
    id = rid;
    return 1;
  }

  // failure
  jprintf(1, noisy, ">>> Could not talk to robot!\n");
  return 0;
}


//= Try connecting to robot on given serial port.
// returns hardware ID if successful (and leaves port open)

int jhcManusBody::test_port (int n, int noisy)
{
  UC8 pod[2];
  int i, p = port0 + n;

  // see if connection exists
  jprintf(1, noisy, "Looking for robot on serial port %d ...\n", p);
  if (ser.SetSource(p, 230400) <= 0)
    return 0;
  ser.wtime = 0.2;                     // for HC-05 Bluetooth latency

  // make sure a robot is there
  if (ser.Xmit(0xA1) > 0)
    if (ser.RxArray(pod, 2) >= 2)
    {
      // try to get ID number
      if (test_id(n))
        return n;
      for (i = 1; i < 256; i++)
        if (i != n)
          if (test_id(i))
           return i;

      // assume given number was correct
      jprintf(1, noisy, ">>> Unable to determine robot ID!\n\n");
      return n;
    }  

  // close port if no response to basic probe
  ser.Close();
  return 0;
}


//= See if the robot responds to the given hardware ID.

bool jhcManusBody::test_id (int i)
{
  UC8 pod[3] = {0xAA, (UC8) i, 0x21};

  if (ser.TxArray(pod, 3) < 3)
    return false;
  return(ser.RxArray(pod, 2) >= 2);
}


//= Precompute coefficients for turning commands into pulse widths.

void jhcManusBody::chan_coefs ()
{
  // fork rate at 50 Hz based on change from default to straight out
  lsf = 4.0 * abs(lout - ldef) / (50.0 * (hout - hdef));

  // width sensor conversion
  wsc = (wfat - wmed) / (vfat - vmed);
  wmin = get_grip(vmin);
  wmax = get_grip(vmax);

  // position command conversion factors
  msc = ccal / vcal;
  tsc = msc * PI * 0.5 * sep / 180.0; 
  lsc = (ldef - lout) / asin((hdef - hout) / arm);  
  gsc = (gmax - gmin) / (wmax - wmin);
}


//= Precompute values for turning voltage into distance.
// distance is roughly proportional to inverse voltage
// <pre>
//   r = rsc / (v + voff) + roff
//   v = rsc / (r - roff) - voff
//
//   (v0 - v4) = rsc * [ 1 / (r0 - roff) - 1 / (r4 - roff) ]
//             = rsc * [ (r4 - roff) - (r0 - roff) ] / (r0 - roff) * (r4 - roff)
//             = rsc * (r4 - r0) / (r0 - roff) * (r4 - roff)
//
//   rsc = (v0 - v4) * (r0 - roff) * (r4 - roff) / (r4 - r0)
//       = [ (v0 - v4) / (r4 - r0) ] * (r0 - roff) * (r4 - roff)
//       = S * (r0 - roff) * (r4 - roff) where S = (v0 - v4) / (r4 - r0)
//
//   rsc = T * (r0 - roff) * (r12 - roff) where T = (v0 - v12) / (r12 - r0)
//
//   S * (r0 - roff) * (r4 - roff) = T * (r0 - roff) * (r12 - roff)
//                 S * (r4 - roff) = T * (r12 - roff)
//               S * r4 - S * roff = T * r12 - T * roff 
//                  (T - S) * roff = T * r12 - S * r4
//                            roff = (T * r12 - S * r4) / (T - S)
//
//   (v0 - v4) = rsc * [ 1 / (r0 - roff) - 1 / (r4 - roff) ]
//         rsc = (v0 - v4) / [ 1 / (r0 - roff) - 1 / (r4 - roff) ]
//
//    v12 = rsc / (r12 - roff) - voff
//   voff = rsc / (r12 - roff) - v12
// 
// </pre>

void jhcManusBody::v2d_eqn ()
{
  double S = (v0 - v4) / (r4 - r0), T = (v0 - v12) / (r12 - r0);

  roff = (T * r12 - S * r4) / (T - S);
  rsc = (v0 - v4) / (1.0 / (r0 - roff) - 1.0 / (r4 - roff));
  voff = rsc / (r12 - roff) - v12;
}


//= Set initial servo positions and motion profiling parameters.

void jhcManusBody::servo_defs ()
{
  // set servo max speeds and accelerations
  set_speed(HAND, gsp, 1);
  set_speed(LIFT, lsp, 1);
  set_accel(LIFT, lacc);
  set_speed(LFW,  dacc, 1);          // really acceleration
  set_speed(RTW,  dacc, 1);

  // set initial targets (in microseconds)
  set_target(LFW,  lf0);
  set_target(RTW,  rt0);
  set_target(LIFT, ldef);
  set_target(HAND, gmax);
  req_all();
}


//= Freezes all motion servos, sets hand to passive.

void jhcManusBody::Stop ()
{ 
  // set up actuator commands
  send_wheels(0.0, 0.0);
  send_lift(0.0);
  send_grip(0);

  // send to robot (skip getting sensors)
  if (mok > 0)
  {
    ser.TxArray(ask, 20);
    jms_sleep(100);
  }

  // always kill comm link (must be re-established later)
  ser.Close();  
  mok = 0;
}


///////////////////////////////////////////////////////////////////////////
//                             Rough Odometry                            //
///////////////////////////////////////////////////////////////////////////

//= Reset odometry so current direction is angle zero and path length zero.
// also resets Cartesian coordinates to (0, 0) and x axis points forward

void jhcManusBody::Zero ()
{
}


///////////////////////////////////////////////////////////////////////////
//                             Core Interaction                          //
///////////////////////////////////////////////////////////////////////////

//= Read and interpret base odometry as well as grip force and distance.
// assumes req_all() has already been called to prompt status return
// automatically resets "lock" for new bids and specifies default motion
// HC-05 Bluetooth has an instrinsic latency of 36 +/- 10ms so 27 Hz max
// returns 1 for okay, 0 or negative for problem

int jhcManusBody::Update (int img)
{
  int rc = 0;

  // wait until next video frame is ready then rectify
  if (img > 0)
    if ((rc = UpdateImg(1)) < 0)
      return -1;

  // check for sensors on Bluetooth
  if (rcv_all() > 0)
  {
    // record sensor values
    lvel = get_lf(pos[LFW]);
    rvel = get_rt(pos[RTW]);
    ht   = get_lift(pos[LIFT]);
    wid  = get_grip(pos[WID]);
    dist = get_dist(pos[DIST]);

    // do additional interpretation 
    inc_odom(jms_now());
  }

  // set up for next cycle
  cmd_defs();
  return mok;
}


//= Load new image from video source and possibly rectify.
// sometimes useful for debugging vison (robot sensors are irrelevant)
// can forego rectification in order to leave shared images unlocked
// returns 1 if okay, 0 or negative for problem
// NOTE: always blocks until new frame is ready

int jhcManusBody::UpdateImg (int rect)
{
  got = 0;
  if (vid == NULL) 
    return 0;
  if (vid->Get(now) <= 0)              // only affected image
    return -1;
  if (rect > 0)
    Rectify();
  return 1;
}


//= Correct lens distortion in recently acquired image.

void jhcManusBody::Rectify ()
{
  wp.Warp(frame, now);
  got = 1;
}


//= Compute likely speed of left wheel based on current servo set points.
// assume wheel's actual velocity is zero if zero microseconds reported

double jhcManusBody::get_lf (double us) const
{
  if (us == 0.0)
    return 0.0;
  return((us - lf0) / ((1.0 - bal) * msc));
}


//= Compute likely speed of right wheel based on current servo set points.
// assume wheel's actual velocity is zero if zero microseconds reported

double jhcManusBody::get_rt (double us) const
{
  if (us == 0.0)
    return 0.0;
  return((rt0 - us) / ((1.0 + bal) * msc));
}


//= Determine actual position of lift stage.

double jhcManusBody::get_lift (double us) const
{
  return(hout + arm * sin((us - lout) / lsc)); 
}


//= Determine width of gripper.
// converts of A/D reading to actual separation in inches

double jhcManusBody::get_grip (double ad) const
{
  return(wsc * (ad - vmed) + wmed);
}


//= Determine forward distance to obstacle.

double jhcManusBody::get_dist (double ad) const
{
  double d = roff + rsc / (4.0 * ad + voff);

  return __max(0.0, d);                      // 2cm min
}


//= Update odometry based on wheel speeds over last time interval.
// uses saved values "lvel" and "rvel"

void jhcManusBody::inc_odom (UL32 tnow)
{
  UL32 t0 = tlast;
  double secs, ins, degs, mid;

  // set up for next cycle then find elapsed time 
  tlast = tnow;
  if (t0 == 0)
    return;
  secs = jms_secs(tnow, t0);
    
  // find length of recent segment and change in direction
  ins  = 0.5 * (rvel + lvel) * secs;
  degs = 0.5 * (rvel - lvel) * secs / tsc;

  // update inferred global Cartesian position
  mid = D2R * (head + 0.5 * degs);
  xpos += ins * cos(mid);
  ypos += ins * sin(mid);

  // update path length and current global orientation
  trav += ins;
  head += degs;
//jprintf("[%3.1 %3.1] -> dist %4.2f, turn %4.2f -> x = %3.1f, y = %3.1f\n", lvel, rvel, ins, degs, xpos, ypos); 
}


//= Send wheel speeds, desired forklift height, and adjust gripper.
// assumes Update has already been called to get gripper force
// returns 1 if successful, 0 or negative for problem

int jhcManusBody::Issue ()
{
  // send motor commands
  send_wheels(move, turn);
  send_lift(fork);
  send_grip(grip);

  // update local state and request new sensor data
  inc_odom(jms_now());
  req_all();
  return mok;
}


//= Compute wheel speeds based on commands and send to robot.
// relies on digital servo's deadband (will keep moving otherwise)

void jhcManusBody::send_wheels (double ips, double dps)
{
  double mv = msc * ips, tv = tsc * dps;

  set_target(LFW, lf0 + (1.0 - bal) * (mv - tv));
  set_target(RTW, rt0 - (1.0 + bal) * (mv + tv));
}


//= Compute lift setting and send to robot.
// assumes Update already called to provide current height

void jhcManusBody::send_lift (double ips)
{
  double hcmd = ht, dead = 0.25, stop = 4.0;   // inches per second
  int fvel = ROUND(lsf * fabs(ips));

  if (ips > dead)
    hcmd = 4.0;
  else if (ips < -dead)
    hcmd = 0.0;
  else
    fvel = ROUND(lsf * stop);
  set_target(LIFT, lout + lsc * asin((hcmd - hout) / arm));
  set_speed(LIFT, __max(1, fvel), 0);
}


//= Compute grip setting and send to robot.
// 1 = active close, -1 = active open, 0 = finish last action

void jhcManusBody::send_grip (int dir)
{
  // zero stability if active motion changes
  if ((dir != 0) && (dir != pgrip))
    wcnt = 0;
//jprintf("grip dir %d vs prev %d -> wcnt = %d\n", dir, pgrip, wcnt);
  pgrip = dir;

  // open or close gripper, else remember single stable width (no drift)
  if (dir != 0)
    wcmd = ((dir > 0) ? 2500.0 : 500.0);
  else if (wcnt == wstop)
    wcmd = gmin + gsc * (wid - wmin);
//jprintf("  hand target: %4.2f\n", wcmd);
  set_target(HAND, wcmd);  
}


///////////////////////////////////////////////////////////////////////////
//                            Low Level Serial                           //
///////////////////////////////////////////////////////////////////////////

//= Set the target position for some channel to given number of microseconds.
// part of single large "ask" packet, initialized in constructor
// NOTE: commands only transmitted when req_all() called

void jhcManusBody::set_target (int ch, double us)
{
  int off[5] = {0, 4, 0, 8, 12};     
  int v = ROUND(4.0 * us);
  UC8 *pod;

  if ((ch < LFW) || (ch == DIST) || (ch > HAND))
    return;
  pod = ask + off[ch];         // where channel packet starts
  pod[2] = v & 0x7F;
  pod[3] = (v >> 7) & 0x7F;
}


//= Set the maximum speed for changing position of servo toward target.
// typically updates pulse width by inc_t / 4 microseconds at 50 Hz
// if "imm" > 0 then sends command right now, else waits for req_all()

void jhcManusBody::set_speed (int ch, int inc_t, int imm)
{
  UC8 *pod = ask + 16;       // same slot for all (usually LIFT)

  if ((ch < LFW) || (ch == DIST) || (ch > HAND) || (mok <= 0))
    return;
  pod[0] = 0x87;
  pod[1] = (UC8) ch;
  pod[2] = inc_t & 0x7F;
  pod[3] = (inc_t >> 7) & 0x7F;
  if (imm > 0)
    if (ser.TxArray(pod, 4) < 4)
      mok = 0;
}


//= Set the maximum acceleration for changing speed of target position.
// typically updates speed by inc_v at 50 Hz

void jhcManusBody::set_accel (int ch, int inc_v)
{
  UC8 pod[4];

  if ((ch < LFW) || (ch == DIST) || (ch > HAND) || (mok <= 0))
    return;
  pod[0] = 0x89;
  pod[1] = (UC8) ch;
  pod[2] = inc_v & 0x7F;
  pod[3] = (inc_v >> 7) & 0x7F;
  if (ser.TxArray(pod, 4) < 4)
    mok = 0;
}


//= Ask for positions of all channels (but do not wait for response).

int jhcManusBody::req_all () 
{
  if (mok <= 0)
    return mok;
  if (ser.TxArray(ask, 32) < 32)       // should just instantly queue
    mok = 0;
  return mok;
}


//= Read position of all channels in terms of microseconds.
// assumes servo controller has already been prompted with req_all()
// returns 1 if okay, 0 or negative for error
// NOTE: always blocks until all data received

int jhcManusBody::rcv_all ()
{
  int i;

  // get full response from robot
  if (mok > 0)
  {
    // convert 16 bit values to milliseconds
    if (ser.RxArray(pod, 12) < 12)
      mok = 0;
    else
      for (i = 0; i < 12; i += 2)
        pos[i / 2] = 0.25 * ((pod[i + 1] << 8) | pod[i]);
  }
  return mok;
}

/*
// non-blocking version

int jhcManusBody::rcv_all ()
{
  int i;

  // sanity check
  if (mok <= 0)
    return mok;
  if (fill >= 12)
    return 1;

  // read as many bytes as currently available
  while (ser.Check() > 0)
  {
    // save each byte in "pod" array
    if ((i = ser.Rcv()) < 0)
    {
      mok = 0;
      return -1;
    }
    pod[fill++] = (UC8) i;

    // if all received, convert 16 bit values to milliseconds
    if (fill >= 12)
    {
      for (i = 0; i < 12; i += 2)
        pos[i / 2] = 0.25 * ((pod[i + 1] << 8) | pod[i]);
      return 1;
    }
  }

  // still more to go
  return 0;                  
}
*/
