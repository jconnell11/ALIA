// jhcStdKern.h : stub for connecting grounded procedures to ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#ifndef _JHCSTDKERN_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCSTDKERN_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <string.h>                    // for macros

#include "API/jhcAliaKernel.h"


///////////////////////////////////////////////////////////////////////////

//= Declares three procedures related to some function name.
// foo0() sets up command parameters, foo() monitors status, foo2() kills command 

#define JCMD_DEF(name) \
  int name ## 0 (const jhcAliaDesc& act, int i); \
  int name (const jhcAliaDesc& act, int i); \
  int name ## 2 (const jhcAliaDesc& act, int i); 


//= Dispatch to analyzer and creator if name matches (local variables "desc" and "i" must be bound).
// foo0() should return 1 for successful interpretation, 0 or negative for problem

#define JCMD_SET(name) if (desc.LexMatch(#name)) return name ## 0(desc, i);


//= Dispatch to status checker if name matches (local variables "desc" and "i" must be bound).
// foo() should return 1 for done, 0 for still working, -1 for problem

#define JCMD_CHK(name) if (desc.LexMatch(#name)) return name(desc, i);


//= Dispatch to command processor if name matches (local variables "desc" and "i" must be bound).
// foo2() can return any value (never checked)

#define JCMD_END(name) if (desc.LexMatch(#name)) return name ## 2(desc, i);


///////////////////////////////////////////////////////////////////////////

//= Interface stub for connecting grounded procedures to ALIA system.
// maintains a importance bid and auxilliary variables for each function instance
// override functions like local_volunteer and local_start in derived class 
// Note: jhcNetBuild::ValRules can help generate rules for sensor values

class jhcStdKern : public jhcAliaKernel
{
// PRIVATE MEMBER VARIABLES
private:
  int nc;                          /** Max number of instances.   */


// PROTECTED MEMBER VARIABLES
protected:
  // call info
  char **cmd;                      /** Name of function called.    */
  jhcAliaDesc **cobj;              /** Focal object for command.   */
  jhcAliaDesc **cspot;             /** Description of destination. */
  double *csp;                     /** Desired speed of action.    */
  int *cbid;                       /** Importance of instance.     */

  // goal and progress
  double *camt;                    /** Desired amount of action.     */
  double *caux;                    /** Extra act info (e.g. angle).  */
  double *cerr;                    /** Last deviation from target.   */
  int *cmode;                      /** Special mode flag for action. */
  int *cst;                        /** Current step of sequence.     */
  int *cst2;                       /** Current sequence substate.    */
  int *ccnt;                       /** General purpose counter.      */
  UL32 *ct0;                       /** Millisecond timing reference. */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcStdKern ();
  jhcStdKern (int n =100);
  int MaxInst () const {return nc;}
  void AddFcns (jhcAliaKernel& pool);

  // main functions
  void Platform (void *soma);
  void Reset (jhcAliaNote& atree);                
  void Volunteer ();
  int Start (const jhcAliaDesc& desc, int bid);
  int Status (const jhcAliaDesc& desc, int inst);
  int Stop (const jhcAliaDesc& desc, int inst);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr_ptrs ();
  void alloc (int n);
  void dealloc ();

  // virtuals to override
  virtual void local_platform (void *soma) {};
  virtual void local_reset (jhcAliaNote& top) {};
  virtual void local_volunteer () {};
  virtual int local_start (const jhcAliaDesc& desc, int i)  {return -2;}
  virtual int local_status (const jhcAliaDesc& desc, int i) {return -2;}
  virtual int local_stop (const jhcAliaDesc& desc, int i)   {return -2;}

  
};


#endif  // once




