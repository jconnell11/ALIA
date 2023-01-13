// jhcAliaCore.h : top-level coordinator of components in ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2017-2020 IBM Corporation
// Copyright 2020-2023 Etaoin Systems
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
#include "Parse/jhcVocab.h"
#include "Reasoning/jhcActionTree.h"     
#include "Reasoning/jhcAssocMem.h"
#include "Reasoning/jhcDeclMem.h"
#include "Reasoning/jhcProcMem.h"

#include "Action/jhcAliaDLL.h"         // common robot
#include "Action/jhcAliaMood.h"
#include "Action/jhcAliaStats.h"
#include "Action/jhcEchoFcn.h"     
#include "Grounding/jhcTalkFcn.h"          
#include "Grounding/jhcMemStore.h"          
#include "Grounding/jhcIntrospect.h"          


//= Top-level coordinator of components in ALIA system.
// essentially contains the attentional buffer and several forms of memory
// this environment gets passed to many things in their Run calls
// which allows use of the halo processing and operator selection
// <pre>
// class tree overview (+ = member, > = pointer):
//
//   AliaCore
//     +AssocMem             rule collection
//       >AliaRule
//         Situation         condition part
//           NodePool
//         +Graphlet         result part
//     +ProcMem              operator collection
//       >AliaOp
//         Situation         trigger part
//           NodePool
//           +Graphlet   
//         >AliaChain        response part
//           +Bindings
//           >AliaDir
//             +Bindings
//           >AliaPlay
//             >AliaDir
//               +Bindings
//     +DeclMem              fact collection
//       NodePool
//         >NetNode
//     +ActionTree           directive control
//       WorkMem             main + halo facts
//         NodePool
//           >NetNode
//     +GramExec             input parse
//       TxtSrc
//       >GramRule
//         >GramStep
//     +NetBuild             network assembly
//       Graphizer
//         SlotVal
//         +MorphFcns
//     +TalkFcn              text output
//       StdKernel
//     +MemStore             long term memory formation
//       StdKernel
//     +Introspect           call stack examiner
//       StdKernel
//     +EchoFcn              missing function catcher
//       StdKernel
//     +AliaMood             operator preference
//     +AliaStats
// 
// </pre>

class jhcAliaCore
{
// PRIVATE MEMBER VARIABLES
private:
  static const int dmax = 30;  /** Maximum extra grounding DLLs. */

  jhcTalkFcn talk;             // literal text output
  jhcMemStore ltm;             // long term memory formation
  jhcIntrospect why;           // call stack examiner

  jhcAssocMem amem;            // working memory expansions
  jhcProcMem pmem;             // reactions and expansions
  char rob[40];                // name of active robot

  jhcAliaDLL gnd[dmax];        // extra grounding DLLs
  int ndll;                    // number of DLLs added
  double ver;                  // current code version

  double pess;                 // preference threshold (pessimism)
  double wild;                 // respect for operator preference
  double det;                  // determination to achieve intent
  double argh;                 // wait before retry of intention
  double waver;                // initial period to all re-FIND-ing
  int deep;                    // maximum subgoal stack depth

  int svc;                     // which focus is being worked on
  int bid;                     // importance of next activity in focus
  int topval;                  // unique ID for active NOTEs

  UL32 t0;                     // starting time of this run
  FILE *log;                   // user input conversion results
  int spact;                   // last speech act received


// PROTECTED MEMBER VARIABLES
protected:
  jhcEchoFcn kern;             // external procedure calls  


// PUBLIC MEMBER VARIABLES
public:
  jhcParam mps;
  jhcActionTree atree;         // working memory and call roots
  jhcDeclMem dmem;             // long term facts
  jhcNetBuild net;             // language to network conversion
  jhcGramExec gr;              // text parser
  jhcVocab vc;                 // known words and corrections
  jhcAliaStats stat;           // monitor for various activities
  jhcAliaMood mood;            // time varying goal preferences
  char cfile[80];              // preferred log file for conversions  
  int vol;                     // load volition operators
  int noisy;                   // controls diagnostic messages
  int finder;                  // controls FIND progress messages
  int pshow;                   // controls parser result messages
  int memhyp;                  // print hypotheticals at end


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcAliaCore ();
  jhcAliaCore ();
  double Version () const {return ver;}
  double Wild () const    {return wild;}
  double MinPref () const {return pess;}
  double Retry () const   {return argh;}
  double Dither () const  {return waver;}
  int NextBid () const    {return bid;}
  int LastTop () const    {return topval;}
  double Stretch (double secs) const {return(det * secs);}
  int MaxStack () const   {return deep;}

  // processing parameter bundles 
  int Defaults (const char *fname =NULL);       
  int SaveVals (const char *fname) const;

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
  int Interpret (const char *input =NULL, int awake =1, int amode =2, int spin =0);
  jhcAliaChain *Reinterpret ();
  int RunAll (int gc =1);
  int Response (char *out, int ssz) {return talk.Output(out, ssz);}
  void StopAll ();
  void CloseCvt ();

  // convenience
  template <size_t ssz>
    int Response (char (&out)[ssz])            
      {return Response(out, ssz);}

  // directive functions
  int MainMemOnly (jhcBindings& b, int note =2);
  jhcAliaChain *CopyMethod (const jhcAliaOp *op, jhcBindings& b, const jhcGraphlet *ctx =NULL)
    {return (op->meth)->Instantiate(atree, b, ctx);}
  int GetChoices (jhcAliaDir *d)
    {return pmem.FindOps(d, atree, pess, atree.MinBlf());}
  void SetPref (double pref)
    {bid = atree.ServiceWt(pref);}
  int HaltActive (jhcGraphlet& desc);
  jhcAliaOp *Probe () {return &(pmem.probe);}

  // halo control
  int Percolate (const jhcGraphlet& dkey);

  // external grounding
  int FcnStart (const jhcNetNode *fcn);
  int FcnStatus (const jhcNetNode *fcn, int inst);
  int FcnStop (const jhcNetNode *fcn, int inst);

  // language output
  int SayStart (const jhcGraphlet& g);
  int SayStatus (const jhcGraphlet& g, int inst);
  int SayStop (const jhcGraphlet& g, int inst);

  // debugging
  void ShowMem () {atree.PrintMain(memhyp);}
  void LoadLearned ();
  void DumpLearned () const;
  void DumpSession ();
  void DumpAll () const;


// PRIVATE MEMBER FUNCTIONS
private:
  // processing parameters
  int msg_params (const char *fname);

  // extensions
  int add_info (const char *dir, const char *base, int rpt, int level);
  bool readable (char *fname, int ssz, const char *msg, ...) const;

  // main functions
  int guess_cats (const char *txt);
  void gram_add_hq (const char *wd);
  void gather_stats ();

  // speech overrides for new words
  virtual void sp_listen (int doit) {};
  virtual void gram_add (const char *cat, const char *wd, int lvl) 
    {if (wd != NULL) gr.ExtendRule(cat, wd, lvl);}

  // debugging
  void copy_file (const char *dest, const char *src) const;


};


#endif  // once




