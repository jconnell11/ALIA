// jhcIntrospect.cpp : examines action tree in ALIA system to supply reasons
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

#include "Grounding/jhcIntrospect.h"


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
  ver = 1.10;
  strcpy_s(tag, "Introspect");
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcIntrospect::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(why_try);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcIntrospect::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(why_try);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                        Failure Determination                          //
///////////////////////////////////////////////////////////////////////////

//= Start trying to determine failure reason for some directive.
// returns 1 if okay, -1 for interpretation error

int jhcIntrospect::why_try0 (const jhcAliaDesc *desc, int i)
{
  const jhcAliaDesc *f;

  if (atree == NULL)
    return -1;
  if ((f = desc->Val("arg")) == NULL)
    return -1;
  if (!f->LexMatch("fail") || (f->Val("act") == NULL))
    return -1;
  return 1;
}


//= Continue trying to determine failure reason for directive.
// <pre>
//   trig:
//   ANTE[  act-1 -lex-  explain
//                -obj-> fail-2
//         fail-2 -lex-  fail
//                -act-> plan-3 ]
// ---------------
//    FCN[ fcn-1 -lex-  why_try 
//               -arg-> fail-2 ]
// </pre>
// returns 1 if done, 0 if still working, -1 for failure

int jhcIntrospect::why_try (const jhcAliaDesc *desc, int i)
{
  jhcAliaDesc *fail = desc->Val("arg"), *plan = fail->Val("act");
  const jhcAliaChain *step = atree->Current();
  const jhcAliaDir *dir, *ward = NULL; 
  const jhcGraphlet *sit;
  int k;

  // check for explicit failure reason 
  if ((sit = atree->Error()) != NULL)
    return cuz_err(fail, sit);

  // find relevant TRY - assumes top level with only continuations
  while ((step = step->cont) != NULL)
    if ((ward = step->GetDir()) != NULL)
      if ((ward->Kind() == JDIR_TRY) && (ward->KeyMain() == plan))
        break;
  if (step == NULL) 
    return -1;

  // craft custom reason for various failed directive types
  if ((dir = failed_dir(ward->Method())) == NULL)
    return -1;
  k = (int) dir->Kind();
  if ((k == JDIR_FIND) || (k == JDIR_BIND) || (k == JDIR_EACH))
    if (multi_act(ward->Method(), NULL, 0))
      return cuz_find(fail, dir);
  if (k == JDIR_DO) 
    if ((dir->NumTries() == 0) || multi_act(ward->Method(), NULL, 0))
      return cuz_do(fail, dir);
  return -1;
}


//= Find first failing directive in given sequence.

const jhcAliaDir *jhcIntrospect::failed_dir (const jhcAliaChain *start) const
{
  const jhcAliaChain *step = start;
  int v, first = 1;

  // follow actual execution path based on saved verdicts
  while (step != NULL)
  {
    if ((first <= 0) && (step == start))         // quit if loop 
      return NULL;
    if ((v = step->Verdict()) == -2)            
    {
      // if BIND/FIND retry fails, advance to following action
      if ((step->cont == NULL) || ((step->cont)->Verdict() == 0))
        return step->GetDir();
    }
    else if (v <= 0)                             // step still running
      return NULL;
    step = ((v == 2) ? step->alt : step->cont);
    first = 0;
  }
  return NULL;
} 


//= See if chain has more than one actual action (BIND does not count).
// state: 0 = initialize chain, 1 = some action found, 2 = second action found

bool jhcIntrospect::multi_act (const jhcAliaChain *start, const jhcAliaChain *now, int state0) const
{
  const jhcAliaChain *step = ((state0 <= 0) ? start : now); 
  const jhcAliaDir *dir;
  int state = __max(1, state0);

  if ((step == NULL) || (now == start))          // no looping
    return false;
  if ((dir = step->GetDir()) == NULL)
    return false;
  if (++state > 2)                              
    return true;
  return(multi_act(start, step->cont, state) ||     
         multi_act(start, step->alt,  state) ||
         multi_act(start, step->fail, state));
}


///////////////////////////////////////////////////////////////////////////
//                          Failure Messages                             //
///////////////////////////////////////////////////////////////////////////

//= Set reason as explicit error message from some grounding function.
// example "because the arm is broken":
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
  atree->StartNote();
  atree->AddArg(atree->NewProp(fail, "why", "because"), "sit", sit->Main());
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
//         act-1 -lex-  find
//               -ach-  0
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
  fact = atree->MakeNode("find", "find", 1, 1.0, 1);
  fact->AddArg("obj", obj);
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
//         act-1 -lex-  grab
//               -ach-  0
//               -obj-> obj-1 ]
// </pre>

int jhcIntrospect::cuz_do (jhcAliaDesc *fail, const jhcAliaDir *dir) 
{
  jhcNetNode *act = dir->KeyMain(), *fact = act;

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
    fact = atree->MakeNode("know", "know", 1);
    atree->AddArg(fact, "how", act);
  }

  // attach cause to reason fact
  atree->AddArg(atree->NewProp(fail, "why", "because"), "sit", fact);
  atree->FinishNote();
  return 1;
}


