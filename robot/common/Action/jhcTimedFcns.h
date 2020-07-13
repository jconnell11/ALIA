// jhcTimedFcns.h : stub for connecting grounded procedures to ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2019 IBM Corporation
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

#ifndef _JHCTIMEDFCNS_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCTIMEDFCNS_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <string.h>                    // for macros

#include "Geometry/jhcMatrix.h"        // common robot

#include "Action/jhcAliaKernel.h"


///////////////////////////////////////////////////////////////////////////

//= Declares three procedures related to some function name.
// foo_new sets up command parameters, foo_chk monitors status, foo_end kills command 

#define JCMD_DEF(name) \
  int name ## _set (const jhcAliaDesc *act, int i); \
  int name ## _chk (const jhcAliaDesc *act, int i); \
  int name ## _end (const jhcAliaDesc *act, int i); 


//= Dispatch to analyzer and creator if name matches (local variables "desc" and "i" must be bound).
// foo_set should return 1 for successful interpretation, 0 or negative for problem

#define JCMD_SET(name) if (_stricmp(desc->Word(), #name) == 0) return name ## _set(desc, i);


//= Dispatch to status checker if name matches (local variables "desc" and "i" must be bound).
// foo_chk should return 1 for done, 0 for still working, -1 for problem

#define JCMD_CHK(name) if (_stricmp(desc->Word(), #name) == 0) return name ## _chk(desc, i);


//= Dispatch to command processor if name matches (local variables "desc" and "i" must be bound).
// foo_run can return any value (never checked)

#define JCMD_END(name) if (_stricmp(desc->Word(), #name) == 0) return name ## _end(desc, i);


///////////////////////////////////////////////////////////////////////////

//= Interface stub for connecting grounded procedures to ALIA system.
// maintains a importance bid and starting time for each function instance

class jhcTimedFcns : public jhcAliaKernel
{
// PRIVATE MEMBER VARIABLES
private:
  int nc;                          /** Max number of instances.   */


// PROTECTED MEMBER VARIABLES
protected:
  // call info
  char **cmd;                      /** Name of function called. */
  int *cbid;                       /** Importance of instance.  */
  UL32 *ct0;                       /** Start time of instance.  */

  // goal and progress
  jhcMatrix *cpos;                 /** Target position for action.  */
  jhcMatrix *cdir;                 /** Target direction for action. */
  double *camt;                    /** Desired amount of action.    */
  double *csp;                     /** Desired speed of action.     */
  double *cerr;                    /** Last deviation from target.  */
  int *cst;                        /** Current state of instance.   */


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and initialization
  ~jhcTimedFcns ();
  jhcTimedFcns (int n =100);
  void SetSize (int n);
  int MaxInst () const {return nc;}
  void AddFcns (jhcAliaKernel *pool);
  void Reset (jhcAliaNote *attn);                

  // main functions
  void Volunteer ();
  int Start (const jhcAliaDesc *desc, int bid);
  int Status (const jhcAliaDesc *desc, int inst);
  int Stop (const jhcAliaDesc *desc, int inst);

  // utlities
  bool Stuck (int i, double err, double prog, int start, int mid);


// PRIVATE MEMBER FUNCTIONS
private:
  // creation and initialization
  void clr_ptrs ();
  void dealloc ();

  // virtuals to override
  virtual void local_reset (jhcAliaNote *top) {};
  virtual void local_volunteer () {};
  virtual int local_start (const jhcAliaDesc *desc, int i)  {return -2;}
  virtual int local_status (const jhcAliaDesc *desc, int i) {return -2;}
  virtual int local_stop (const jhcAliaDesc *desc, int i)   {return -2;}

  
};


#endif  // once




