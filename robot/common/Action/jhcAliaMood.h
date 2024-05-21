// jhcAliaMood.h : maintains slow changing state variables for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2021-2024 Etaoin Systems
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

#pragma once

#include "jhcGlobal.h"

#include "Data/jhcArr.h"               // common video
#include "Data/jhcParam.h"

#include "API/jhcAliaNote.h"           // common audio


//= Maintains slow changing state variables for ALIA system.
// collects data about many activities (raw available for display)
// adjust preference threshold, confidence threshold, and wildness
// also generates overall emotion bits:
// [ surprised angry scared happy : unhappy bored lonely tired ]

class jhcAliaMood
{
// PRIVATE MEMBER VARIABLES
private:
  class jhcActionTree *atree;          // for reasoning

  // --------------------- data collection ---------------------

  // operator monitoring (may be several calls)
  int win, lose, good, bad;

  // rule monitoring (may be several calls)
  double jump;
  int right, wrong, confirm, refute; 

  // body and battery data (once every cycle)
  double bspeed, fspeed, mtim;

  // user interaction (once every cycle)
  double itim;
  int people;

  // --------------------- state variables ---------------------

  // update time and emotion bits
  UL32 now;
  int melt, vect;

  // reasoning related variables
  double busy, wow, ctrl, sure;

  // drive related variables
  double motion, social, energy;
  double satis, antsy, isol, lack;


// PRIVATE MEMBER PARAMETERS
private:
  // reasoning and battery parameters 
  double btime, engaged, frantic, wtime, surp, vsurp, low, vlow;

  // motion drive parameters
  double fhand, fbase, ftalk, noise, mtime, mok, bore, vbore;

  // social drive parameters
  double fhear, fdude, lps, stime, sok, lone, vlone;

  // overall valence parameters
  double mmix, smix, hhys, hap, vhap, lmix, sad, vsad;
 
  // operator eval parameters
  double fgood, fbad, osamp, otime, cdes, chys, mad, vmad;

  // rule eval parameters
  double fconf, fref, rsamp, rtime, sdes, shys, scare, vscare;

  // threshold adjustment parameters
  double whi, wlo, bhi, blo;

  // activity weghting factors
  double mhi, mlo, shi, slo, ohi, olo, rhi, rlo;


// PUBLIC MEMBER VARIABLES
public:
  // parameter sets for GUI
  jhcParam cps, mps, sps, vps, ops, rps, aps, pps;


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaMood ();
  jhcAliaMood ();
  void Bind (class jhcActionTree& t) {atree = &t;}

  // processing parameter bundles 
  int LoadCfg (const char *fname =NULL);
  int SaveCfg (const char *fname) const;

  // main functions
  void Reset ();
  void Update ();
  int Quantized () const {return vect;}

  // -------------------- read only access ---------------------

  // thought intensity
  double Busy () const  {return busy;}
  int MeltDown () const {return melt;}

  // internal emotional levels
  double Motion () const   {return motion;}
  double Social () const   {return social;}
  double Valence () const  {return satis;}
  double Unhappy () const  {return lack;}
  double Surprise () const {return wow;}

  // details for charting
  double Active () const {return mok;}
  double BodyData (double& bsp, double& fsp, double& mt) const
    {bsp = bspeed; fsp = fspeed; mt = mtim; return energy;}
  int SocialData (double& it) const
    {it = itim; return people;}

  // ----------------- external data collection ----------------

  // user communication
  void Speak (int len, double hz =30.0);
  void Hear (int len, double hz =30.0);
  void Infer (int cnt =1);
  void React (int cnt =1);

  // body and environment
  void Travel (double rate =1.0);
  void Reach (double rate =1.0);
  void Battery (double pct);
  void Faces (int cnt =1);

  // ----------------- internal data collection ----------------

  // operator monitoring
  void OpLaunch ();
  void OpWin ();
  void OpLose ();
  void OpBelow ();
  void UserPref (int good =1);   

  // rule monitoring 
  void RuleEval (int hit, int miss, double chg);
  void RuleAdj (double adj =0.1);
  void UserConf (int correct =1); 


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int core_params (const char *fname);
  int motion_params (const char *fname);
  int social_params (const char *fname);
  int valence_params (const char *fname);
  int op_params (const char *fname);
  int rule_params (const char *fname);
  int adj_params (const char *fname);
  int pref_params (const char *fname);

  // main functions
  void clr_accum ();
  int bit_vector () const;
  int dual_under (int mask, double val, double on, double hys) const;
  int dual_over (int mask, double val, double on, double hys) const;

  // drives and evaluations
  void sm_busy (double dt);
  void sm_motion (double dt);
  void sm_social (double dt);
  void valence (double dt);
  void sm_ctrl (double dt);
  void sm_sure (double dt);

  // reasoning adjustment
  void adj_wild () const;
  void adj_belief () const;
  void adj_pref () const;

};

