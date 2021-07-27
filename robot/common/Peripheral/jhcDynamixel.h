// jhcDynamixel.h : sends commands to Robotis Dynamixel servo actuators
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

#ifndef _JHCDYNAMIXEL_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCDYNAMIXEL_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Peripheral/jhcSerialFTDI.h"


//= Sends commands to Robotis Dynamixel servo actuators.

class jhcDynamixel : public jhcSerialFTDI
{
// PRIVATE MEMBER VARIABLES
private:
  UC8 dn[256];       // maximum command packet size
  UC8 up[256];       // maximum response packet size (including mega)
  int fill;          // how many servos currently in MultiPosVel command
  int err;           // error on last operation suggests a purge
  int rc;            // last return code (stalled, overtemperature, etc.)
  int acc;           // size of accelerometer data in mega-update
  int m0;            // first ID of servo in mega update
  int nup;           // expected size of mega update pod
  int mcnt;          // size of mega update pod received

public:
  int noisy;         // set positive for debugging printouts
  int retry;         // automatically retry N times if fails
  int pic;           // ID of PIC for mega update (negative if none)
  int mpod;          // number of mega-updates request
  int mfail;         // number of mega-updates that failed


// PUBLIC MEMBER VARIABLES
public:


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcDynamixel (int port =0, int rate =1000000);
  ~jhcDynamixel ();
  void Reset ();

  // system status
  double Voltage (int id);
  double RunAX12 (double volts) const {return(6.6 * volts);}    /** 6 kg-cm torque @ 12.6V. */
  double HoldAX12 (double volts) const {return(17.4 * volts);}  /** 15 kg-cm torque @ 12V.  */
 
  // mode commands
  int Init (int id, int chk =1);
  int SetStop (int id, int state =0x24, int chk =1);
  int SetLims (int id, double deg0 =-150.0, double deg1 =150.0, int chk =1);
  int SetMargin (int id, double ccw =0.29, double cw =0.29);
  int SetSlope (int id, double ccw =2.6, double cw =2.6);
  int SetPunch (int id, double f =0.031);
  int SetSpin (int id, int chk =1);
  int GetStop (int id);
  int GetLims (double& deg0, double& deg1, int id);
  int GetSpin (int id);

  // multi-joint status read
  int MegaUpdate (int id0, int idn, int base =0);
  int MegaIssue (int id0, int idn, int base =0);
  int MegaCollect ();
  int RawAccel (int& xpk, int& ypk, int& xav4, int& yav4) const;
  int RobotID ();

  // joint status functions
  int GetPos (double& degs, int id);
  int GetVel (double& dps, int id);
  int GetForce (double& frac, int id);
  int GetPosVel (double& degs, double& dps, int id); 
  int GetState (double& degs, double& dps, double& frac, int id); 
  int QuadState (double *degs, double *dps, double *frac, int *flag, int *id);

  // joint and wheel command functions
  int Limp (int id, int doit =1);
  int SetPos (int id, double degs);
  int SetPosVel (int id, double degs, double dps);
  int SetSpeed (int id, double dps);
  int MultiLimp (int *id, int n, int doit =1);
  int MultiPos (int *id, double *degs, int n);
  int MultiPosVel (int *id, double *degs, double *dps, int n); 
  int MultiSend ();
  int MultiSpeed (int *id, double *dps, int n);

  // low level packets
  int ResetServo (int id);
  int Ping (int id);
  int Read8 (int id, int addr);
  int Read16 (int id, int addr);
  int ReadArr8 (int id, int addr, int *data, int n);
  int ReadArr16 (int id, int addr, int *data, int n);
  int Write8 (int id, int addr, int val, int queue =0);
  int Write16 (int id, int addr, int val, int queue =0);
  int WriteArr8 (int id, int addr, int *data, int n, int queue =0);
  int WriteArr16 (int id, int addr, int *data, int n, int queue =0);
  int Trigger (int id);

  // decoding last return value
  int Valid ()        {return((err <= 0) ? 1 : 0);} 
  int Flags ()        {return rc;}
  int Error (int f)        {return(((f & 0xEF) != 0) ? 1 : 0);}
  int Command_Err (int f)  {return(((f & 0x40) != 0) ? 1 : 0);}
  int Overload_Err (int f) {return(((f & 0x20) != 0) ? 1 : 0);}
  int Checksum_Err (int f) {return(((f & 0x10) != 0) ? 1 : 0);}
  int Argument_Err (int f) {return(((f & 0x08) != 0) ? 1 : 0);}
  int Heat_Err (int f)     {return(((f & 0x04) != 0) ? 1 : 0);}
  int Angle_Err (int f)    {return(((f & 0x02) != 0) ? 1 : 0);}
  int Voltage_Err (int f)  {return(((f & 0x01) != 0) ? 1 : 0);}


// PRIVATE MEMBER FUNCTIONS
private:
  int chk_mega (double& degs, double& dps, double& frac, int id);

  int degs2pos (double degs);
  int dps2vel (double dps);
  int frac2pwm (double frac);
  double pos2degs (int pos);
  double vel2dps (int vel);
  double pwm2frac (int pwm);

  void set_cmd (int id, int cmd, int argc =0);
  void set_arg (int n, int val);
  void set_val8 (int n, int val);
  void set_val16 (int n, int val);
  int get_val8 (int n);
  int get_val16 (int n);

  int cmd_ack ();
  int tx_pod ();
  int rx_pod ();
  void print_pod (UC8 pod[], int n =0, const char *tag =NULL);

};


#endif  // once




