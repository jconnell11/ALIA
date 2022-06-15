// jhcTalkFcn.cpp : string and semantic net language output for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
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

#include <ctype.h>
#include <string.h>

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
  ver = 1.55;
  strcpy_s(tag, "TalkFcn");
  SetSize(smax);
  *winner = '\0';
  finish = 0;
  imp = 0;
}


//= Copy desired output to given string.
// returns importance of saying this (0 if nothing to say)

int jhcTalkFcn::Output (char *out, int ssz) 
{
  double lps = 12.0;                   // letters per second        
  int n = (int) strlen(winner);

  // reset arbitration if last output likely spoken
  if (finish != 0) 
    if (jms_diff(jms_now(), finish) > 0)
    {
      finish = 0;
      imp = 0;
    }

  // possibly nothing new to report
  if (n <= 0)
  {
    *out = '\0';
    return 0;
  }

  // report string just once but block lower priority for a while
  strcpy_s(out, ssz, winner);
  *winner = '\0';                             
  finish = jms_now() + ROUND(1000.0 * n / lps);
  return imp;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Set up for new run of system.

void jhcTalkFcn::local_reset (jhcAliaNote *top)
{
  dg.SetMem(top);
  *winner = '\0';
  finish = 0;
  imp = 0;
}


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

int jhcTalkFcn::echo_wds0 (const jhcAliaDesc *desc, int i)
{
  if (i >= smax)
    return -1;
  if (build_string(desc, i) <= 0)
    return -1;
  ct0[i] = jms_now();
  return i;  
}


//= Assert already assembled utterance as a good thing to say.
// saves utterance in member variable in case daydreaming
// returns 1 if done, 0 if still working, -1 for failure

int jhcTalkFcn::echo_wds (const jhcAliaDesc *desc, int i)
{
  double patience = 2.0;

  if (cbid[i] >= imp) 
  {
    strcpy_s(winner, full[i]);        
    imp = cbid[i];
    return 1;
  }
  if (jms_elapsed(ct0[i]) > patience)  
    return -1;
  return 0;
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
    return fix_surface(full[inst]);
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
        ref = dg.NameRef(n);
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
  return fix_surface(full[inst]);
}


///////////////////////////////////////////////////////////////////////////
//                            String Cleanup                             //
///////////////////////////////////////////////////////////////////////////

//= Perform a series of surface form checks to clean up final phrase.
// always returns 1 for covenience

int jhcTalkFcn::fix_surface (char *txt)
{
  fix_itis(txt);
  fix_verb(txt);
  fix_det(txt);
  fix_abbrev(txt);
  return 1;
}


//= Drop off leading "it is" from property descriptions.
// example: "it is red" -> "red", need to run before fix_abbrev

void jhcTalkFcn::fix_itis (char *txt)
{
  int i, trim = (int) strlen(txt) - 6;

  if (_strnicmp(txt, "it is ", 6) == 0)
  {
    for (i = 0; i < trim; i++)
      txt[i] = txt[i + 6];
    txt[trim] = '\0';
  }
}


//= Replaces obvious verb agreement problem like "I is".
// assumes text string can always be extended by one character
// only replaces first occurrence

void jhcTalkFcn::fix_verb (char *txt)
{
  char *fix;
  int i, n;

  // "I is" -> "I am"
  if ((fix = strstr(txt, "I is")) != NULL) 
  {
    fix[2] = 'a';
    fix[3] = 'm';
    return;
  }

  // "you is" -> "you are" (tail of string shifted)
  if ((fix = strstr(txt, "you is")) != NULL)
  {
    n = (int) strlen(fix);
    for (i = n; i >= 6; i--)
      fix[i + 1] = fix[i];
    fix[4] = 'a';
    fix[5] = 'r';
    fix[6] = 'e';
    return;
  }

  // "are one" -> "is one" (for counting)
  if ((fix = strstr(txt, "are one")) != NULL)
  {
    n = (int) strlen(fix);
    for (i = 3; i < n; i++)
      fix[i - 1] = fix[i];    
    fix[0] = 'i';
    fix[1] = 's';
    fix[n - 1] = '\0';
    return;
  }
}


//= Replaces obvious wrong determiner problems like "a object".
// assumes text string can always be extended by a few characters

void jhcTalkFcn::fix_det (char *txt)
{
  int i, j, n = (int) strlen(txt), mid = 0;

  // look for first occurrence of pattern
  for (i = 2; i < n; i++)
  {
    if ((mid <= 0) && (tolower(txt[i - 2]) == 'a') && (txt[i - 1] == ' ') && 
        (strchr("aeiou", tolower(txt[i])) != NULL))
    {
      // insert missing "n" after shifting other letters over
      n++;
      for (j = n; j >= i; j--) 
        txt[j] = txt[j - 1];
      txt[i - 1] = 'n';
    }
    mid = isalnum(txt[i - 2]);      
  }
}


//= Convert discrete words to standard contractions (e.g. "it is" -> "it's").

void jhcTalkFcn::fix_abbrev (char *txt)
{
  char *tail, *fix;
  int i, n;

  // "it is" -> "it's" (tail of string shifted)
  tail = txt;
  while ((fix = strstr(tail, "it is")) != NULL)
  {
    tail = fix + 5;                    // skip over match
    if (word_after(tail))
    {
      n = (int) strlen(fix);
      for (i = 5; i < n; i++)
        fix[i - 1] = fix[i];
      fix[n - 1] = '\0';
      fix[2] = '\'';
      fix[3] = 's';
      tail--;                          // one letter shorter now
    }
  }

  // "do not" -> "don't"
  tail = txt;
  while ((fix = strstr(tail, "do not")) != NULL)
  {
    tail = fix + 6;                    // skip over match
    if (word_after(tail))
    {
      n = (int) strlen(fix);
      for (i = 6; i < n; i++)
        fix[i - 1] = fix[i];
      fix[n - 1] = '\0';
      fix[2] = 'n';
      fix[3] = '\'';
      fix[4] = 't';
      tail--;                          // one letter shorter now
    }
  }
}


//= Make sure string starts with whitespace and has something non-whitepace later.
// prevents contraction of "it is not" -> "it isn't" -> "it'sn't"

bool jhcTalkFcn::word_after (const char *txt) const
{
  const char *tail = txt;

  if ((txt == NULL) || (*txt == '\0') || isalnum(*txt))
    return false;
  while (!isalnum(*tail++))
    if (*tail == '\0')
      return false;
  return true;
}
