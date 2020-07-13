// jhcActSeq.cpp : holds sequence of steps for an action
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2011-2012 IBM Corporation
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

#include <stdio.h>
#include <string.h>

#include "Action/jhcActSeq.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcActSeq::jhcActSeq ()
{
  int i;

  // no current action or expansion
  *verb = '\0';
  for (i = 0; i < 10; i++)
    step[i][0] = '\0';
  sub = NULL;

  // set default database and clear expansion state 
  Database("act_models.txt");
  Reset();
}


//= Default destructor does necessary cleanup.

jhcActSeq::~jhcActSeq ()
{
  clr_sub();
}


//= Set file for saving new actions and looking up old actions.
// returns -1 if not writable, 0 if does not exist yet, 1 for okay

int jhcActSeq::Database (const char *fname)
{
  FILE *f;
  
  // save name
  strcpy_s(file, fname);

  // try opening for writing
  if (fopen_s(&f, file, "a") != 0)
    return -1;
  fclose(f);

  // try opening for reading
  if (fopen_s(&f, file, "r") != 0)
    return 0;
  fclose(f);
  return 1;
}


//= Reset state for the beginning of a sequence.

void jhcActSeq::Reset ()
{
  len = 0;
  next = 0;
  clr_sub();
}


//= Get rid of any subaction (and children) that might be pending.

void jhcActSeq::clr_sub ()
{
  if (sub == NULL)
    return;
  delete sub;
  sub = NULL;
}


///////////////////////////////////////////////////////////////////////////
//                                New Action                             //
///////////////////////////////////////////////////////////////////////////

//= Start learning about some particular action by accumulating steps.
// can optionally assign name after sequence entered if clr is zero

void jhcActSeq::Start (const char *act, int clr)
{
  if (clr > 0)
    Reset();
  strcpy_s(verb, act);
}


//= Add next step to verb being learned (if any).
// can optionally clear whole entry if too many steps
// returns -1 if too many steps, 0 if no verb, 1 if okay

int jhcActSeq::AddStep (const char *act, double amt, int over)
{
  if (*verb == '\0')
    return 0;
  if (len < 10)
  {
    strcpy_s(step[len], act);
    adv[len] = amt;
    len++;
    return 1;
  }
  if (over > 0)
    Reset();
  return -1;
}


//= See if supplied action matches verb already present.
// if verb field is empty, this new value is copied
// if no action passed in, then always matched
// returns 0 if different, 1 if same (or copied)

int jhcActSeq::ChkVerb (const char *act)
{
  if (*act == '\0')
    return 1;
  if (*verb == '\0')
  {
    strcpy_s(verb, act);
    return 1;
  }
  if (strcmp(verb, act) == 0)
    return 1;
  return 0;
}


//= Save last thing learned in database.
// does NOT erase current verb name or sequences (use Learn)
// returns 1 if okay, 0 if nothing to save, -1 for file error

int jhcActSeq::Save ()
{
  FILE *out;
  int i;

  // check for various problems
  if ((*verb == '\0') || (len <= 0))
    return 0;
  if ((*file == '\0') || (fopen_s(&out, file, "a+") != 0))
    return -1;

  // write a line showing expansion
  fprintf(out, "%s \t=", verb);
  for (i = 0; i < len; i++)
    fprintf(out, " %s %4.2f", step[i], adv[i]);
  fprintf(out, "\n");
  fclose(out);
  return 1;
}
 

///////////////////////////////////////////////////////////////////////////
//                                Old Action                             //
///////////////////////////////////////////////////////////////////////////

//= Look for expansion of named action in database.
// returns 1 if found, 0 if no expansion, -1 for file error

int jhcActSeq::Load (const char *act)
{
  FILE *in;
  char line[200], sep[10] = " \t\n", comment[10] = "/;";
  char *item, *mod, *tail;
  float amt;                          // must be float for sscanf

  // try opening database file
  if (fopen_s(&in, file, "r") != 0)
    return -1;
  Start(act);

  // scan each line in the file
  while (fgets(line, 200, in) != NULL)
  {
    // ignore line if commented out
    if (strchr(comment, *line) != NULL)
      continue;

    // check if matches desired verb
    tail = line;
    if ((item = next_token(&tail, sep)) == NULL)
      continue;
    if (strcmp(item, act) != 0)
      continue;

    // strip equal sign
    if ((item = next_token(&tail, sep)) == NULL)
      continue;
    if (strcmp(item, "=") != 0)
      continue;

    // copy steps and adverbial modifiers to local array
    while ((item = next_token(&tail, sep)) != NULL) 
    {
      amt = 1.0;
      if ((mod = next_token(&tail, sep)) != NULL)
        sscanf_s(mod, "%f", &amt);
      AddStep(item, amt);
    }
    break;
  }

  // clean up
  fclose(in);
  if (len > 0)
    return 1;
  return 0;
}


//= Safer form of strtok function that allows call nesting.
// alters input to point to remainder of original string (NULL when done)
// returns pointer to found token (original string is altered with '\0')

char *jhcActSeq::next_token (char **src, const char *delim)
{
  char *start, *end;

  if ((src == NULL) || (*src == NULL))
    return NULL;
    
  // skip over delimiters to start of token
  start = *src;
  while ((*start != '\0') && (strchr(delim, *start) != NULL))
    start++;
  if (*start == '\0')
  {
    *src = NULL;       // no more tokens
    return NULL;
  }

  // pass through valid characters to first delimiter
  end = start + 1;
  while ((*end != '\0') && (strchr(delim, *end) == NULL))
    end++;
  if (*end == '\0')
  {
    *src = NULL;       // no more tokens
    return start;
  }

  // terminate token and point at where to look next
  *end = '\0';
  *src = end + 1;
  return start;
}


//= Return the next base-level step in the overall expansion.
// base actions prefixed by an exclamation point
// automatically tries to load expansion if needed
// returns 1 if okay, 0 if done, -1 if should expand but doesn't

int jhcActSeq::NextStep (char *act, double& amt, int ssz)
{
  // check for end of sequence 
  *act = '\0';
  if (next >= len)
    return 0;
  
  // if in subroutine then ask for its next step
  if (sub != NULL)
  {
    if (sub->NextStep(act, amt, ssz) > 0)
      return 1;
    clr_sub();
    if (++next >= len)
      return 0;
  }

  // check for base action
  if (strncmp(step[next], "x_", 2) == 0)
  {
    strcpy_s(act, ssz, step[next]);
    amt = adv[next];
    next++;
    return 1;
  }

  // try to create an expansion then fill in steps
  if (sub == NULL)
    if ((sub = new jhcActSeq) == NULL)
      return -1;
  if (sub->Load(step[next]) <= 0)
    return -1;
  return sub->NextStep(act, amt, ssz);
}


//= Returns the number of levels in the current expansion.

int jhcActSeq::Levels ()
{
  if (sub == NULL)
    return 1;
  return(1 + sub->Levels());
}


//= Returns the current step being executed at some level down from the base.

int jhcActSeq::StepNum (int level)
{
  if (level == 0)
    return next;
  if (sub == NULL)
    return 0;
  return sub->StepNum(level - 1);
}


//= Gets the symbolic action name at some level down from the base.
// returns 0 if level is invalid, 1 if okay

int jhcActSeq::ActName (char *act, int level, int ssz)
{
  // this action
  if (level == 0)
  {
    strcpy_s(act, ssz, verb);
    return 1;
  }

  // a deeper action
  if (level > 1)
  {
    if (sub == NULL)
      return 0;
    return sub->ActName(act, level - 1, ssz);
  }

  // some step in this action
  if (next >= len)
    return 0;
  strcpy_s(act, ssz, step[next]);
  return 1;
}

