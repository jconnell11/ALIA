// jhcAliaOp.h : advice on what to do given some stimulus or desire
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2020-2022 Etaoin Systems
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

#ifndef _JHCALIAOP_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIAOP_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Parse/jhcTxtLine.h"          // common audio
#include "Reasoning/jhcWorkMem.h"
#include "Semantic/jhcBindings.h"
#include "Semantic/jhcSituation.h"

#include "Action/jhcAliaChain.h"       // common robot
#include "Action/jhcAliaDir.h"


//= Advice on what to do given some stimulus.
//
// overall OP has single preference
//   used to both gate and order selection of competitors
//   if OP is matched > current threshold then input belief irrelevant
//
// adjustment of preference:  
//   if top level OP succeeds go backwards in time through NRI
//     if success then make sure above default threshold
//     if failure before success (needs one) then decrement 
//
// special terminations:
//   if FIND or CHK succeeds then running OP is considered to succeed
//   if no more OPs for ANTE or POST then they succeed
//   if NOTE becomes invalidated (blf=0) then it succeeded
//
// free choice NOTE OPs maintain expected completion time
//   allows subgoal method to be delayed or retried several times
// 
// alteration mostly performed in jhcAliaDir::alter_pref 

class jhcAliaOp : public jhcSituation
{
friend class jhcProcMem;               // collection
friend class jhcGraphizer;             // creation


// PRIVATE MEMBER VARIABLES
private:
  static const int t0 = 5;             /** Default completion time. */
  static const int s0 = 2;             /** Default time deviation.  */

  // definition and list structure
  char gist[200];       
  jhcAliaOp *next;
  JDIR_KIND kind; 
  double pref0, pref;
  double time0, tavg, tstd;
  int id, lvl;

  // matching state
  int first, tval, omax;


// PUBLIC MEMBER VARIABLES
public:
  // source of info
  char prov[80];
  int pnum;

  // proposed action and desirability
  jhcAliaChain *meth;


// PUBLIC MEMBER FUNCTIONS
public:
  // simple functions
  int OpNum () const      {return id;}
  double Pref () const    {return pref;}
  void SetPref (double v) {pref = 0.01 * ROUND(100.0 * v);}
  double Time () const    {return tavg;}
  double Dev () const     {return tstd;}
  double Budget () const  {return(tavg + tstd);}
  void SetTime (double a, double d)
    {tavg = 0.1 * ROUND(10.0 * a); tstd = 0.1 * ROUND(10.0 * d);}
  JDIR_KIND Kind () const {return kind;}
  const char *KindTag () const 
    {jhcAliaDir dcvt; return dcvt.CvtTag(kind);}
  void SetGist (const char *sent);

  // main functions
  int FindMatches (jhcAliaDir& dir, const jhcWorkMem& f, double mth);
  bool SameEffect (const jhcBindings& b1, const jhcBindings& b2) const;
  void AdjPref (double dv) {SetPref(pref + dv);}
  void AdjTime (double secs);

  // file functions
  int Load (jhcTxtLine& in); 
  int Save (FILE *out, int detail =1);
  int Print (int detail =1) {return Save(stdout, detail);}


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  ~jhcAliaOp ();
  jhcAliaOp (JDIR_KIND k =JDIR_DO);
 
  // main functions
  int try_mate (const jhcNetNode *focus, jhcNetNode *mate, jhcAliaDir& dir, const jhcWorkMem& f);

  // file functions
  int load_pattern (jhcTxtLine& in);

  // virtual override
  int match_found (jhcBindings *m, int& mc);


};


#endif  // once




