// jhcTalkFcn.cpp : string and semantic net language output for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020 Etaoin Systems
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

#include "Interface/jms_x.h"           // common video

#include "Grounding/jhcTalkFcn.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Initialization                     //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcTalkFcn::~jhcTalkFcn ()
{
}


//= Default constructor initializes certain values.

jhcTalkFcn::jhcTalkFcn ()
{
  ver = 1.30;
  strcpy_s(tag, "TalkFcn");
  SetSize(smax);
  winner = NULL;
  imp = 0;
}


//= Copy desired output to given string.
// returns importance of saying this (0 if nothing to say)

int jhcTalkFcn::Output (char *out, int ssz) 
{
  int imp0 = imp;

  if (winner == NULL)                  // blank string means shutup
  {
    *out = '\0';
    return 0;
  }
  strcpy_s(out, ssz, winner);
  winner = NULL;                       // reset arbitration
  imp = 0;
  return imp0;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Start up a new instance of some named function.
// starting time and bid are already speculatively bound by base class
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if successful, -1 for problem, -2 if function unknown

int jhcTalkFcn::local_start (const jhcAliaDesc *desc, int i)
{
  JCMD_SET(echo_wds);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcTalkFcn::local_status (const jhcAliaDesc *desc, int i)
{
  JCMD_CHK(echo_wds);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                          User Literal Output                          //
///////////////////////////////////////////////////////////////////////////

//= Assembles utterance for output to user.
// returns new instance number (>= 0) if success, -1 for problem, -2 for unknown

int jhcTalkFcn::echo_wds_set (const jhcAliaDesc *desc, int i)
{
  if (i >= smax)
    return -1;
  if (build_string(desc, i) <= 0)
    return -1;
  return i;  
}


//= Assemble full string with substitutions as required.
// <pre>
//   fcn-1 -lex-   echo_wds
//         -pat->  obj-1                   (directly describe object)
//
//   fcn-1 -lex-   echo_wds
//         -dest-> agt-1                   (optional binding for ?0)
//         -pat--> txt-1
//   txt-1 -str-   I think ?1 is ?2 ?0     (fill in slots with arguments)
//         -arg1-> obj-1
//         -arg2-> hq-1      
// </pre>
// returns 1 if successful (result in "full"), 0 or negative if some problem

int jhcTalkFcn::build_string (const jhcAliaDesc *desc, int inst)
{
  char slot[40] = "arg0";
  char *txt = full[inst];
  const char *form, *ref;
  const jhcAliaDesc *pat, *n;
  char num;

  // if utterance is a single node try to generate a string for it 
  if ((pat = desc->Val("pat")) == NULL)
    return -2;
  if ((form = pat->Literal()) == NULL)
  {
    if ((ref = dg.NodeRef(pat, -1)) == NULL)
      return 0;
    strcpy_s(full[inst], ref);
    return 1;
  }

  // find substitution points in a format like: "I see ?1 ?2 things ?0"
  while (*form != '\0')
  {
    // check for variable markerand strip it off when found
    num = form[1];
    if ((*form != '?') || (num < '0') || (num > '9'))
    {
      *txt++ = *form++;
      continue;
    }
    form += 2;                   

    // find string to substitute for variable
    if (num == '0')
    {
      // ?0 means user so see if proper name given or previously known
      if ((n = desc->Val("dest")) != NULL)   
        ref = dg.LexRef(n);
      else
        ref = dg.UserRef();
    }
    else
    {
      // otherwise find correct node and generate a string
      slot[3] = num;
      if ((n = pat->Val(slot)) == NULL)
        return -1;
      if ((ref = dg.NodeRef(n)) == NULL)
        return 0;
    }

    // if unknown user erase leading space, else copy string to output
    if (ref == NULL)
      txt--;                     
    else
      while (*ref != '\0')
        *txt++ = *ref++;
   }

  // clean up
  *txt = '\0';
  return 1;
}


//= Assert already assembled utterance as a good thing to say.
// returns 1 if done, 0 if still working, -1 for failure

int jhcTalkFcn::echo_wds_chk (const jhcAliaDesc *desc, int i)
{
  if ((cbid[i] >= imp) || (winner == NULL))
  {
    winner = full[i];
    imp = cbid[i];
  }
  return 1;
}

