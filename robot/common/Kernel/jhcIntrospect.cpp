// jhcIntrospect.cpp : examines action tree in ALIA system to supply answers
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2022-2023 Etaoin Systems
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

#include "Kernel/jhcIntrospect.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcIntrospect::~jhcIntrospect ()
{
}


//= Default constructor initializes certain values.

jhcIntrospect::jhcIntrospect ()
{
  strcpy_s(tag, "Introspect");
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcIntrospect::local_start (const jhcAliaDesc& desc, int i)
{
  JCMD_SET(why_run);
  JCMD_SET(why_fail);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcIntrospect::local_status (const jhcAliaDesc& desc, int i)
{
  JCMD_CHK(why_run);
  JCMD_CHK(why_fail);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                          Execution Tracing                            //
///////////////////////////////////////////////////////////////////////////
 
//= Start trying to determine if some activity is occurring.
// returns 1 if okay, -1 for interpretation error

int jhcIntrospect::why_run0 (const jhcAliaDesc& desc, int i)
{
  if (atree == NULL)
    return -1;
  if (desc.Val("arg") == NULL)
    return -1;
  return 1;
}


//= Continue trying to determine if some activity is occurring. 
// returns 1 if done, 0 if still working, -1 for failure
// Note: hard to copy user-given command just to add agent

int jhcIntrospect::why_run (const jhcAliaDesc& desc, int i)
{
  const jhcAliaDesc *act = desc.Val("arg");
  jhcNetNode *act2;

  atree->StartNote();
  act2 = atree->CloneAct(dynamic_cast<const jhcNetNode *>(desc.Val("arg")));
  if (!atree->Recent(atree->nkey, act->Done()))
    act2->SetNeg(1);
  act2->AddArg("agt", atree->Robot());           // restore after search
  if ((act->Done()) > 0)
    act2->SetDone(1);
  atree->FinishNote();
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                        Failure Determination                          //
///////////////////////////////////////////////////////////////////////////

//= Start trying to determine failure reason for some directive.
// returns 1 if okay, -1 for interpretation error

int jhcIntrospect::why_fail0 (const jhcAliaDesc& desc, int i)
{
  const jhcAliaDesc *f, *v;

  if (atree == NULL)
    return -1;
  if ((f = desc.Val("arg")) == NULL)
    return -1;
  if ((v = f->Fact("fcn")) == NULL)
    return -1;
  if (!v->LexMatch("fail") || (f->Val("act") == NULL))
    return -1;
  return 1;
}


//= Continue trying to determine failure reason for directive.
// <pre>
//   trig:
//   ANTE[  fcn-1 -lex-  explain
//                -fcn-> act-1
//          act-1 -obj-> fail-2
//          fcn-2 -lex-  fail
//                -fcn-> fail-2
//         fail-2 -act-> plan-3 ]
// ---------------
//    GND[ gnd-1 -lex-  why_fail
//               -arg-> fail-2 ]
// </pre>
// returns 1 if done, 0 if still working, -1 for failure

int jhcIntrospect::why_fail (const jhcAliaDesc& desc, int i)
{
  jhcAliaDesc *fail = desc.Val("arg");
  const jhcAliaDir *dir; 
  const jhcGraphlet *sit;
  int k;

  // check for explicit failure reason else find failed step
  if ((sit = atree->Error()) != NULL)
    return cuz_err(fail, sit);
  if ((dir = atree->FindFail()) == NULL)
    return -1;

  // craft custom reason for various failed directive types
  k = (int) dir->Kind();
  if ((k == JDIR_FIND) || (k == JDIR_BIND) || (k == JDIR_EACH))
    return cuz_find(fail, dir);
  if (k == JDIR_GATE)
    return cuz_gate(fail, dir);
  if ((k == JDIR_DO) && (dir->NumTries() == 0))
    return cuz_do(fail, dir);
  return -1;
}


///////////////////////////////////////////////////////////////////////////
//                          Failure Messages                             //
///////////////////////////////////////////////////////////////////////////

//= Set reason as explicit error message from some grounding function.
// example "because the arm is broken" or "because I don't see it":
// <pre>
//   NOTE[  why-1 -lex-  because
//                -why-> fail-1
//                -sit-> hq-1         
//           hq-1 -lex-  broken        
//                -hq--> obj-1
//          ako-1 -lex-  arm
//                -ako-> obj-1 ]
// </pre>

int jhcIntrospect::cuz_err (jhcAliaDesc *fail, const jhcGraphlet *sit)
{
  jhcAliaDesc *act = sit->MainAct();
  jhcAliaDesc *cuz = ((act != NULL) ? act : sit->Main());

  atree->StartNote();
  atree->AddArg(atree->NewProp(fail, "why", "because"), "sit", cuz);
  atree->AddNode(fail);
  atree->FinishNote();
  return 1;
}


//= Set reason as a failure to find something with some description.
// example "because I couldn't find a black thing":
// <pre>
//   NOTE[ why-1 -lex-  because
//               -why-> fail-1
//               -sit-> act-1          
//         fcn-1 -lex-  find
//               -fcn-> act-1
//         act-1 -ach-  0
//               -obj-> obj-1          <-- only search obj, no props
//          hq-1 -lex-  black
//               -ext-  0
//               -hq--> obj-1 ]
// </pre>

int jhcIntrospect::cuz_find (jhcAliaDesc *fail, const jhcAliaDir *dir) 
{
  jhcNetNode *fact, *obj = dir->KeyMain();                 // description hypothetical

  // barf if trying to find the name of a property
  if (!obj->ObjNode() && (obj->Lex() == NULL))
    return -1;

  // copy find criteria and reference item sought
  atree->StartNote();
  if (dir->NumGuess() > 0)
    atree->AddProp(obj, "hq", "suitable", 0, 0.0);         // contrast to "any" ?

  // attach cause to reason fact
  fact = atree->MakeAct("find", 1, 1.0, 1);
  fact->AddArg("obj", obj);
  atree->AddArg(atree->NewProp(fail, "why", "because"), "sit", fact);
  atree->FinishNote();
  return 1;
}


//= Set reason as prohibition of a certain action.
// example "because that is not allowed":
// <pre>
//   NOTE[ why-1 -lex-  because
//               -why-> fail-1
//               -sit-> hq-1          
//          hq-1 -lex-  allowed
//               -neg-  1
//               -hq--> act-1 ]
// </pre>

int jhcIntrospect::cuz_gate (jhcAliaDesc *fail, const jhcAliaDir *dir) 
{
  jhcNetNode *fact, *act = dir->KeyAct();

  atree->StartNote();
  fact = atree->AddProp(act, "hq", "allowed", 1);
  atree->AddArg(atree->NewProp(fail, "why", "because"), "sit", fact);
  atree->FinishNote();
  return 1;
}


//= Set reason as a failure to do some particular action.
// example "because I couldn't grab the object":
// <pre>
//   NOTE[ why-1 -lex-  because
//               -why-> fail-1
//               -sit-> act-1          <-- only main verb, no adverbs
//         fcn-1 -lex-  grab
//               -fcn-> act-1
//         act-1 -ach-  0
//               -obj-> obj-1 ]
// </pre>

int jhcIntrospect::cuz_do (jhcAliaDesc *fail, const jhcAliaDir *dir) 
{
  jhcNetNode *act = dir->KeyAct(), *fact = act;

  // copy action specification and reference main verb
  atree->StartNote();
  if (dir->NumTries() > 0)
  {
    act->SetNeg(1);
    act->SetDone(1);
    act->SetBelief(1.0); 
  }
  else 
  {
    fact = atree->MakeAct("know", 1);
    atree->AddArg(fact, "how", act);
  }

  // attach cause to reason fact
  atree->AddArg(atree->NewProp(fail, "why", "because"), "sit", fact);
  atree->FinishNote();
  return 1;
}

