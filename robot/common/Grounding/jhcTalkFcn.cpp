// jhcTalkFcn.cpp : string and semantic net language output for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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
  ver = 1.20;
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

//= Assembles utterance for ouput to user.
// returns new instance number (>= 0) if success, -1 for problem, -2 for unknown

int jhcTalkFcn::echo_wds_set (const jhcAliaDesc *desc, int i)
{
  if (i >= 10)
    return -1;
  if (build_string(desc, i) <= 0)
    return -1;
  return i;  
}


//= Assemble full string with substitutions as required.
// Examples:
//   Compose("I see ?1 ?2 things", "two", "red") --> "I see two red things"
//   Compose("I see ?1 of them",   "two", "red") --> "I see two of them"
//   Compose("I see some ?1 ones", "two", "red") --> "I see some red ones"
// if no text supplied for some argument, assumed to be blank string
// NOTE: arguments start with "?0" (listener name) NOT "?1"
// returns 1 if successful (result in "full"), 0 if some problem

int jhcTalkFcn::build_string (const jhcAliaDesc *desc, int inst)
{
  const jhcAliaDesc *n;
  const char *item, *in;
  char *out = full[inst];
  char args[10][80];
  int i, j, w;

  // get main pattern
  if ((n = desc->Val("pat")) == NULL)
    return -1;
  if ((in = n->Literal()) == NULL)
    return -1;

  // create substitution strings (if any specified)
  for (i = 0; i < 10; i++)
  {
    // stop after first missing arg
    args[i][0] = '\0';
    sprintf_s(full[inst], "arg%d", i);
    if ((n = desc->Val(full[inst])) == NULL)
      break;

    // use most recent real word (not pronoun) associated with node (if any)
    w = 0;
    while ((item = n->Word(w++, 1.0)) != NULL)
      if ((_stricmp(item, "me") != 0) && (_stricmp(item, "I") != 0) && (_stricmp(item, "you") != 0))
        break;
    if (item == NULL)
      continue;
    strcpy_s(args[i], item);

//    if ((item = dg.GenRef(n, attn)) == NULL)
//      return -1;
//    strcpy_s(args[i], item);
  }

  // find possible substitution points in pattern
  // this code borrowed from jhcTxtAssoc::Compose
  while (*in != '\0')
  {
    // check for variable marker (?0, ?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, or ?9)
    i = (int)(in[1] - '0');
    if ((*in != '?') || (i < 0) || (i > 9))
    {
      *out++ = *in++;
      continue;
    }

    // strip variable symbol and bind argument
    in += 2;
    item = args[i];
    if (item == NULL)
      continue;

    // copy argument to output 
    j = (int) strlen(item);
    if (j > 0)
      while (j-- > 0)
        *out++ = *item++;
    else if (out > full[inst])
      out--;                     // blank argument erases leading space
  }

  // clean up
  *out = '\0';
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



