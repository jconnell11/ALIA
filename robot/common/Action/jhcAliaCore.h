// jhcAliaCore.h : top-level coordinator of components in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
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

#ifndef _JHCALIACORE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCALIACORE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Acoustic/jhcGenIO.h"         // common audio
#include "Language/jhcNetBuild.h"  
#include "Parse/jhcGramExec.h"
#include "Reasoning/jhcActionTree.h"     
#include "Reasoning/jhcAssocMem.h"
#include "Reasoning/jhcProcMem.h"

#include "Action/jhcAliaDLL.h"         // common robot
#include "Action/jhcEchoFcn.h"     
#include "Grounding/jhcTalkFcn.h"          


//= Top-level coordinator of components in ALIA system.
// essentially contains the attentional buffer and several forms of memory
// this environment gets passed to many things in their Run calls
// which allows use of the halo processing and operator selection

class jhcAliaCore
{
// PRIVATE MEMBER VARIABLES
private:
  static const int dmax = 30;  /** Maximum extra grounding DLLs. */

  jhcTalkFcn talk;             // literal text output
  jhcAssocMem amem;            // working memory expansions
  jhcProcMem pmem;             // reactions and expansions

  jhcAliaDLL gnd[dmax];        // extra grounding DLLs
  int ndll;                    // number of DLLs added
  double ver;                  // current code version

  double pth;                  // operator preference threshold
  double wild;                 // respect for operator preference

  int svc;                     // which focus is being worked on
  int bid;                     // importance of next activity in focus
  int topval;                  // unique ID for active NOTEs

  UL32 t0;                     // starting time of this run
  FILE *log;                   // user input conversion results


// PROTECTED MEMBER VARIABLES
protected:
  jhcEchoFcn kern;             // external procedure calls  
  jhcGramExec gr;              // text parser


// PUBLIC MEMBER VARIABLES
public:
  jhcNetBuild net;             // language to network conversion
  jhcActionTree atree;         // working memory and call roots
  char cfile[80];              // preferred log file for conversions
  int noisy;                   // controls diagnostic messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaCore ();
  jhcAliaCore ();
  double Version () const {return ver;}
  double Wild () const    {return wild;}
  double MinPref () const {return pth;}
  int NextBid () const    {return bid;}

  // extensions
  void KernExtras (const char *kdir);
  int Baseline (const char *fname, int add =0, int rpt =0);
  int AddOn (const char *fname, void *body, int rpt =0);
  int Accept (jhcAliaRule *r, jhcAliaOp *p);
  void Remove (const jhcAliaRule *rem) {amem.Remove(rem);}
  void Remove (const jhcAliaOp *rem)   {pmem.Remove(rem);}

  // main functions
  int MainGrammar (const char *gfile, const char *top, const char *rname =NULL);
  void Reset (int forget =0, const char *rname =NULL, int cvt =1);
  int Interpret (const char *input =NULL, int awake =1, int amode =2);
  int RunAll (int gc =1);
  int Response (char *out, int ssz) {return talk.Output(out, ssz);}
  void StopAll ();
  void CloseCvt ();

  // convenience
  template <size_t ssz>
    int Response (char (&out)[ssz])            
      {return Response(out, ssz);}

  // directive functions
  int MainMemOnly (jhcBindings& b);
  jhcAliaChain *CopyMethod (const jhcAliaOp *op, jhcBindings& b, const jhcGraphlet *ctx =NULL)
    {return (op->meth)->Instantiate(atree, b, ctx);}
  int GetChoices (jhcAliaDir *d)
    {return pmem.FindOps(d, atree, pth, atree.MinBlf());}
  void SetPref (double pref)
    {bid = atree.ServiceWt(pref);}
  int HaltActive (jhcGraphlet& desc);
  jhcAliaOp *Probe () {return &(pmem.probe);}

  // halo control
  void RecomputeHalo ();
  int Percolate (const jhcAliaDir& dir);
  int ZeroTop (const jhcAliaDir& dir);

  // external grounding
  int FcnStart (const jhcNetNode *fcn);
  int FcnStatus (const jhcNetNode *fcn, int inst);
  int FcnStop (const jhcNetNode *fcn, int inst);

  // language output
  int SayStart (const jhcGraphlet& g);
  int SayStatus (const jhcGraphlet& g, int inst);
  int SayStop (const jhcGraphlet& g, int inst);

  // debugging
  void ShowMem () const {atree.PrintMain();}
  void LoadLearned ();
  void DumpLearned () const;
  void DumpSession () const;
  void DumpAll () const;


// PRIVATE MEMBER FUNCTIONS
private:
  // extensions
  int add_info (const char *dir, const char *base, int rpt, int level);
  bool readable (char *fname, int ssz, const char *msg, ...) const;

};


#endif  // once




