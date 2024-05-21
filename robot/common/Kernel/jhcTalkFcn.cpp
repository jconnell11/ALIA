// jhcTalkFcn.cpp : string and semantic net language output for ALIA system
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2018-2020 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "Action/jhcAliaCore.h"        // common robot - since only spec'd as class in hdr

#include "Kernel/jhcTalkFcn.h"


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
  strcpy_s(tag, "TalkFcn");
  core = NULL;
  *winner = '\0';
  finish = 0;
  imp = 0;
}


//= Need access to core for morphology and speech status.

void jhcTalkFcn::Bind (jhcAliaCore *c) 
{ 
  core = c;
  if (c != NULL)
    dg.SetWords((c->net).mf);
}


//= Copy desired output to given string.
// returns importance of saying this (0 if nothing to say)

int jhcTalkFcn::Output (char *out, int ssz) 
{
  char mark[80] = "##  +------------------------------------------------------------------------";
  double lps = 12.0;                   // letters per second        
  int n = (int) strlen(winner);

  // reset arbitration if last output read and spoken
  if ((finish != 0) && (jms_diff(jms_now(), finish) > 0) &&
      ((core == NULL) || (core->BusyTTS() <= 0)))
  {
    finish = 0;
    imp = 0;
    n = 0;                   // imp = 0 for at least 1 cycle
  }
 
  // possibly nothing new to report
  if (n <= 0)
  {
    *out = '\0';
    return 0;
  }

  // report string just once but block lower priority for a while
  strcpy_s(out, ssz, winner);
  out[0] = (char) toupper(out[0]);
  *winner = '\0';
  finish = jms_now() + ROUND(1000.0 * n / lps);                             

  // show prominently in log file 
  if (n > 0)
  {
    n = __min(n, 70);
    mark[n + 9] = '\0';
    jprintf("\n%s+\n##  | \"%s\" |\n%s+\n\n", mark, out, mark);
  }
  return imp;
}


///////////////////////////////////////////////////////////////////////////
//                          Overridden Functions                         //
///////////////////////////////////////////////////////////////////////////

//= Set up for new run of system.

void jhcTalkFcn::local_reset (jhcAliaNote& top)
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

int jhcTalkFcn::local_start (const jhcAliaDesc& desc, int i)
{
  JCMD_SET(talk_echo);
  JCMD_SET(talk_wait);
  return -2;
}


//= Check on the status of some named function.
// variables "desc" and "i" must be bound for macro dispatcher to run properly
// returns 1 if done, 0 if still working, -1 if failed, -2 if function unknown

int jhcTalkFcn::local_status (const jhcAliaDesc& desc, int i)
{
  JCMD_CHK(talk_echo);
  JCMD_CHK(talk_wait);
  return -2;
}


///////////////////////////////////////////////////////////////////////////
//                          User Literal Output                          //
///////////////////////////////////////////////////////////////////////////

//= Assembles statement for output to user.
// returns 1 if okay, -1 for interpretation error

int jhcTalkFcn::talk_echo0 (const jhcAliaDesc& desc, int i)
{
  if (i >= smax)
    return -1;
  if (build_string(desc, i) <= 0)
    return -1;
  ct0[i] = jms_now();
  return 1;  
}


//= Assert already assembled statement as a good thing to say.
// saves utterance in member variable in case daydreaming
// returns 1 if done, 0 if still working, -1 for failure
// NOTE: waits for text to be queued as highest, not for utterance to complete

int jhcTalkFcn::talk_echo (const jhcAliaDesc& desc, int i)
{
  double patience = 2.0;     

  if (cbid[i] < imp)
  {
    if (jms_elapsed(ct0[i]) > patience)  
      return -1;
    return 0;                // still waiting
  }
  strcpy_s(winner, full[i]);        
  imp = cbid[i];
  return 1;
}


//= Assembles question for presentation to user.
// returns 1 if okay, -1 for interpretation error

int jhcTalkFcn::talk_wait0 (const jhcAliaDesc& desc, int i)
{
  if (i >= smax)
    return -1;
  if (build_string(desc, i) <= 0)
    return -1;
  q_mark(full[i]);           // enable speech input
  ct0[i] = jms_now();
  return 1;  
}


//= Assert already assembled question as a good thing to say.
// waits for user to respond or a long interval of silence
// saves utterance in member variable in case daydreaming
// returns 1 if done, 0 if still working, -1 for failure

int jhcTalkFcn::talk_wait (const jhcAliaDesc& desc, int i)
{
  double patience = 2.0, respond = 5.0, digest = 0.5;

  // wait until this is the most important thing to say
  if (cst[i] <= 0)
  {
    if (cbid[i] < imp)
    {
      if (jms_elapsed(ct0[i]) > patience)  
        return -1;
      return 0;              // still waiting
    }
    strcpy_s(winner, full[i]);        
    imp = cbid[i];
    cst[i] = 1;
  }

  // wait for TTS to finish (reading or speaking)
  if (cst[i] == 1)
  {
    if (imp != 0)            // imp = 0 for at least 1 cycle
      return 0;
    ct0[i] = jms_now();
    cst[i] = 2;
  }

  // wait for user to say something in response (or timeout)
  if (cst[i] == 2)
  {
    if ((core != NULL) && (core->SpeechRC() > 0))
      cst[i] = 3;
    else if (jms_elapsed(ct0[i]) > respond)
      return 1;
    else
      return 0;
  }

  // wait for user to finish speaking
  if (cst[i] == 3)
  {
    if ((core != NULL) && (core->SpeechRC() > 0))
      return 0;
    ct0[i] = jms_now();
    cst[i] = 4;
  }

  // small pause to digest response
  if (cst[i] >= 4)
    if (jms_elapsed(ct0[i]) < digest)
      return 0;
  return 1;
}


//= Add question mak to end of utterance to signal that user speech is next.

void jhcTalkFcn::q_mark (char *txt) const
{
  int i, n = (int) strlen(txt);

  for (i = n - 1; i >= 0; i--)
    if (txt[i] != ' ')
      break;
  txt[i + 1] = '?';
  txt[i + 2] = '\0';
}


//= Assemble full string with substitutions as required.
// <pre>
//   fcn-1 -lex-   echo_wds
//         -pat->  obj-1                   (directly describe object)
//
//   fcn-1 -lex-   echo_wds
//         -targ-> agt-1                   (optional binding for ?0)
//         -pat--> txt-1
//   txt-1 -str-   I think ?1 is ?2 ?0     (fill in slots with arguments)
//         -arg1-> obj-1
//         -arg2-> hq-1      
// </pre>
// returns 1 if successful (result in "full"), 0 or negative if some problem

int jhcTalkFcn::build_string (const jhcAliaDesc& desc, int inst)
{
  char slot[40] = "arg0";
  char *txt = full[inst];
  const char *form, *ref;
  jhcAliaDesc *pat, *n;
  char num;

  // if utterance is a single node try to generate a string for it 
  if ((pat = desc.Val("pat")) == NULL)
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
      if ((n = desc.Val("targ")) != NULL)
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
//  fix_itis(txt);
  fix_verb(txt);
  fix_det(txt);
  fix_abbrev(txt);
  fix_num(txt);
  return 1;
}


//= Drop off leading "it is" from property descriptions.
// example: "it is red" -> "red", need to run before fix_abbrev
// Note: bad idea for non-copula like "it is not working"

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
  convert_all("I is",    "I am",    txt, 0);  
  convert_all("you is",  "you are", txt, 0);  
  convert_all("are one", "is one",  txt, 0);  
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
  convert_all("I am",    "I'm",    txt, 1);
  convert_all("you are", "you're", txt, 1);
  convert_all("it is",   "it's",   txt, 1);
  convert_all("do not",  "don't",  txt, 0);
}


//= Convert lone leading digit to a word ("3 ..." -> "Three ...").

void jhcTalkFcn::fix_num (char *txt)
{
  char num[10][20] = {"zero", "one", "two", "three", "four", 
                      "five", "six", "seven", "eight", "nine"};
  char tail[500];
  int v;

  if (!isdigit(txt[0]) || isdigit(txt[1]))
    return;
  if (sscanf_s(txt, "%d", &v) != 1)
    return;
  if ((v < 0) || (v > 9))
    return;
  strcpy_s(tail, txt + 1);
  sprintf_s(txt, 500, "%s%s", num[v], tail);
}


//= Convert every occurrence of pattern string into replacement string.
// if wd > 0 then must have following word (prevent "That's who I'm") 
// NOTE: case insensitive

void jhcTalkFcn::convert_all (const char *pat, const char *rep, char *txt, int wd)
{
  char *hit = txt;
  const char *tail;
  int i, n, r = (int) strlen(rep), p = (int) strlen(pat), sh = r - p;

  // look for every properly delineated occurrence of pattern
  while (*hit != '\0')
     if ((_strnicmp(hit, pat, p) != 0) ||
         ((hit != txt) && (isalnum(*(hit - 1)) > 0)) ||     // space before
         ((hit[p] != '\0') && (isalnum(hit[p]) > 0)))       // space after
       hit++;
     else
     {
       // possibly check for whole word following match
       if (wd > 0)
       {
         tail = hit + p;
         while ((*tail != '\0') && (isalnum(*tail) <= 0))  // whitespace
           tail++;
         if (*tail == '\0')                                // no word
         {
           hit++;
           continue;
         }
       }

      // move remainder of string to left
      n = (int) strlen(hit);
      if (sh < 0)
        for (i = r; i < n; i++)
          hit[i] = hit[i - sh];
      else if (sh > 0)
        for (i = n; i >= p; i--)
          hit[i + sh] = hit[i];
       
      // copy in replacement (no null termination)
      for (i = 0; i < r; i++)     
        hit[i] = rep[i];
      hit += r;
    }
}
