// jhcWorkMem.h : main temporary semantic network for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#ifndef _JHCWORKMEM_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCWORKMEM_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include "Semantic/jhcBindings.h"      // common audio
#include "Semantic/jhcGraphlet.h"
#include "Semantic/jhcNetNode.h"
#include "Semantic/jhcNodePool.h"


//= Main temporary semantic network for ALIA system.
// lower 2 levels of memory: main > halo (attention on top)
// this holds all the facts directly linked to attention items
// main forms a basis for running halo rules to generate deductions
// main memory plus halo are used to match operator conditions

class jhcWorkMem : public jhcNodePool
{
  static const int emax = 50;          /** Max external references. */

// PRIVATE MEMBER VARIABLES
private:
  // main vs halo separation
  int rim, nimbus, mode;     // rim = last LTM, nimbus = last single rule
  jhcNetNode *self;          // fixed node representing the robot
  jhcNetNode *user;          // node for current person communicating

  // external linkages
  jhcNetNode *nref[emax];
  int ext[emax], cat[emax];

  // global condition belief threshold (skepticism)
  double skep;               


// PROTECTED MEMBER VARIABLES
protected:
  jhcNodePool halo;          // expectations


// PUBLIC MEMBER VARIABLES
public:
  int noisy;                 // control of diagnostic messages


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcWorkMem ();
  jhcWorkMem ();
  void Border ()  {rim = halo.LastLabel();}
  void Horizon () {nimbus = halo.LastLabel();}
  int LastGhost () const  {return rim;}
  int LastSingle () const {return nimbus;} 
  void MaxBand (int lvl =3) {mode = lvl;}
  int WmemSize (int hyp =0) const {return NodeCnt(hyp);}
  int HaloSize (int hyp =0) const {return halo.NodeCnt(hyp);}

  // belief threshold
  double MinBlf () const {return skep;}
  void SetMinBlf (double s) {skep = __max(0.1, __min(1.0, s));}

  // conversation participants
  jhcNetNode *Robot () const {return self;}
  jhcNetNode *Human () const {return user;}
  jhcNetNode *SetUser (jhcNetNode *n);
  void AddName (jhcNetNode *n, const char *name, int neg =0);
  jhcNetNode *FindName (const char *full) const;
  bool NameClash (const jhcNetNode *n, const char *name, int neg =0) const;

  // list access (overrides virtual)
  jhcNetNode *NextNode (const jhcNetNode *prev =NULL, int bin =-1) const;
  int Length () const {return NodeCnt(1);}
  bool Prohibited (const jhcNetNode *n) const;
  int SameBin (const jhcNetNode& focus, const jhcBindings *b) const;
  int NumBands () const {return(mode + 1);}
  bool InBand (const jhcNetNode *n, int part) const;
  bool InList (const jhcNetNode *n) const 
    {return(jhcNodePool::InList(n) || halo.InList(n));}
  bool InMain (const jhcNetNode *n) const
    {return jhcNodePool::InList(n);}

  // halo functions
  void ClearHalo () {halo.PurgeAll();}
  void AssertHalo (const jhcGraphlet& pat, jhcBindings& b) 
    {halo.Assert(pat, b, 0.0, 0, NULL);} 
  jhcNetNode *CloneHalo (const jhcNetNode& n)
    {return halo.CloneNode(n);}
  bool VisMem (const jhcNetNode* n, int h) const;

  // truth maintenance
  void RevealAll (const jhcGraphlet& desc);
  int Endorse (const jhcGraphlet& key, int dbg =0);

  // external nodes
  int ExtLink (int rnum, jhcNetNode *obj, int kind =0);
  jhcNetNode *ExtRef (int rnum, int kind =0) const;
  int ExtRef (const jhcNetNode *obj, int kind =0) const;
  int ExtEnum (int rnum, int kind =0) const;

  // writing functions
  int Save (const char *fname, int lvl =0)
    {MaxBand(0); return jhcNodePool::Save(fname, lvl);}
  int Print (int lvl =0, int hyp =0) 
    {MaxBand(0); return jhcNodePool::Print(lvl, hyp);}

  // debugging
  void PrintMain (int hyp =0);  
  void PrintHalo (int hyp =0) const; 
  int PrintRaw (int hyp =0) const;


// PROTECTED MEMBER FUNCTIONS
protected:
  // creation and initialization
  void Reset ();

  // conversation participants
  void InitPeople (const char *rname);

  // garbage collection
  int CleanMem (int dbg =0);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr_ext ();

  // conversation participants
  bool incompatible (const char *node, int nn, const char *full, const char *first, int fn) const;

  // garbage collection
  void keep_party (jhcNetNode *anchor) const;
  void keep_from (jhcNetNode *anchor, int dbg) const;
  int rem_unmarked (int dbg);

  // external nodes
  int ann_link (const jhcNetNode *obj, const jhcNetNode *former, int kind, int rnum) const;
  void rem_ext (const jhcAliaDesc *obj);


};


#endif  // once




