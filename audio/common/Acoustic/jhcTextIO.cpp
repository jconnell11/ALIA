// jhcTextIO.cpp : provides speech-like text input and output
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2015-2019 IBM Corporation
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

// Project / Properties / Configuration / Link / Input must have winmm.lib
#pragma comment(lib, "winmm.lib")


#include <windows.h>                // for Sleep
#include <string.h>
#include <stdarg.h>
#include <conio.h>

#include "Interface/jhcMessage.h"   // common video
#include "Interface/jrand.h"

#include "Acoustic/jhcTextIO.h"


///////////////////////////////////////////////////////////////////////////
//                         Creation and Destruction                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.

jhcTextIO::jhcTextIO ()
{
  // general status
  jrand_seed();
  *ifile = '\0';
  disable = 0;

  // set up variables
  Defaults();
  Reset();
}


//= Default destructor does necessary cleanup.

jhcTextIO::~jhcTextIO ()
{
  Finish();
}


//= Sets up components of text I/O system for initial use.
// returns positive if successful, 0 or negative for failure

int jhcTextIO::Init (int dbg, int noisy)
{
  int ans;

  ClearGrammar();
  ans = LoadGrammar(gfile);
  if (noisy > 0)
    PrintCfg();
  parse_start(dbg, NULL);
  return ans;
}


//= Clear state for beginning of run.
// assumes Init has already been called to set up system

void jhcTextIO::Reset ()
{
  // silence timer
  now = 0;
  last = 0;

  // console input
  fill = line;
  *fill = '\0';
  *raw  = '\0';
  *utt  = '\0';
  rcv = NULL;
  pend = -1;
  hear = 0;
  quit = 0;

  // output messages
  *qtext = '\0';
  *msg = '\0';
  more = NULL;
  emit = NULL;
  tprev = 0;
  tlast = 0;
  tlock = 0;

  // not between Update and Issue
  acc = 0;
}


//= Check whether the ESC key has been hit.
// also checks the state of "quit" set by Ingest()

bool jhcTextIO::Escape ()
{
  char ch;

  if ((disable <= 0) && (_kbhit() != 0))
  {
    if ((ch = (char) _getch()) == '\x1B')
      quit = 1;
    else
      _ungetch(ch);                    // save for later
  }
  return(quit > 0);
}


///////////////////////////////////////////////////////////////////////////
//                               Configuration                           //
///////////////////////////////////////////////////////////////////////////

//= Read all relevant defaults variable values from a file.

int jhcTextIO::Defaults (const char *fname)
{
  int ok = 1;

  // save configuration source file (if any)
  *ifile = '\0';
  if (fname != NULL)
    strcpy_s(ifile, fname);

  // load parameter values
  ok &= tps.LoadText(user,  fname, "txio_user", "Jon", 80);
  ok &= tps.LoadText(gfile, fname, "txio_gram", NULL, 200);
  ok &= text_params(fname);
  return ok;
}


//= Write current processing variable values to a file.

int jhcTextIO::SaveVals (const char *fname) const
{
  int ok = 1;

  ok &= tps.SaveText(fname, "txio_user", user);
  ok &= tps.SaveText(fname, "txio_gram", gfile);
  ok &= tps.SaveVals(fname);
  return ok;
}


//= Parameters used for typing out responses.

int jhcTextIO::text_params (const char *fname)
{
  jhcParam *ps = &tps;
  int ok;

  ps->SetTag("txio_say", 0);
  ps->NextSpec4( &blurt,  1,   "Dump whole string");  
  ps->NextSpec4( &ims,   50,   "Initial char rate (ms)");    
  ps->NextSpec4( &ivar,  20,   "Initial variance (ms)");
  ps->NextSpec4( &mms,   30,   "Middle char rate (ms)");     
  ps->NextSpec4( &mvar,  10,   "Middle variance (ms)"); 
  ps->Skip();

  ps->NextSpec4( &barge,  1,   "Stop output on key down");
  ps->NextSpecF( &firm,  30.0, "Timeout for entry (sec)");  // was 3
  ok = ps->LoadDefs(fname);
  ps->RevertAll();
  return ok;
}


//= Attempt to parse default user to extract first name. 
// returns 1 if okay, 0 if name string not changed

int jhcTextIO::FirstName (char *name, int ssz)
{
  char *end;

  if (*user == '\0')
    return 0;
  strcpy_s(name, ssz, user);
  if ((end = strpbrk(name, " -_\t")) != NULL)
    *end = '\0';
  return 1;
}


//= Attempt to parse default user to extract last name. 
// returns 1 if okay, 0 if name string not changed

int jhcTextIO::LastName (char *name, int ssz)
{
  char *end;

  if ((end = strpbrk(user, " -_\t")) == NULL)
    return 0;
  strcpy_s(name, ssz, end + 1);
  if ((end = strpbrk(name, " -_\t")) != NULL)
    *end = '\0';
  return 1;
}


///////////////////////////////////////////////////////////////////////////
//                          Convenience Functions                        //
///////////////////////////////////////////////////////////////////////////

//= Overwrite input buffer with a fixed string.
// useful for reading inputs from a file or GUI interface
// can also signal that session should end if stop > 0

void jhcTextIO::Inject (const char *txt, int stop)
{
  int n;

  // skip if blank or has comment character
  quit = __max(quit, stop);
  if ((txt == NULL) || (*txt == '\0') || (*txt == ';'))
    return;

  // strip any final carriage return
  n = (int) strlen(txt);
  if (txt[n - 1] == '\n')
    n--;

  // copy to input buffer
  strncpy_s(line, txt, n);
  fill = line + n;
  pend = 1;

  // pretend input has timed out
  last = now - ROUND(1000.0 * firm) - 1;
}


//= Update status of all text-related status variables.
// clears parse tree so association list will be empty if nothing new
// returns status (0 = nothing, 1 = still typing, 2 = recognition, -1 = failed parse)

int jhcTextIO::Update (int reco, int prolong)
{
  int any;

  // continue with text output (call before listen)
  rcv = NULL;
  emit = NULL;
  dribble();

  // see if input finished 
  now = timeGetTime();
  hear = __max(0, pend);
  if ((any = listen()) > 1)
  {
    // possibly try to recognize then clear line
    hear = -1;
    *utt = '\0';
    strcpy_s(raw, line);           // valid listen input
    rcv = raw;
    if (Parse(line) > 0)
    {
      // save raw text for later examination
      strcpy_s(utt, line);
      hear = 2;
    }
    fill = line;
    *fill = '\0';
  }
  else
    ClrTree();

  // adjust silence counter (neither user nor robot speaking)
  if ((any > 0) || (more != NULL))
    last = 0;
  else if (last == 0)
    last = now;
  acc = 1;
  return hear;
}


//= Look for text input from console.
// sets member variable "pend" if user has started a line
// returns 3 if time out, 2 if Enter hit, 1 if some key, 0 if waiting

int jhcTextIO::listen ()
{
  char ch;

  // see if output happening and cannot be interrupted
  if ((barge <= 0) && (more != NULL)) 
    return 0;
  if (disable > 0)
  {
    *fill = '\0';
    pend = 0;
    return((fill > line) ? 2 : 0);
  }

  // see if should give up on user typing
  if (_kbhit() == 0) 
  {
    if ((pend > 0) && (Silence() > firm))
    {
      jprintf(" ...\n");     // show forced termination
      *fill = '\0';
      pend = -1;
      return 3;
    }
    return 0;
  }

  while (_kbhit() != 0)
  {
    // read typed character
    ch = (char) _getch();
    pend = 1;

    // remove a character for backspace
    if (ch == '\b')        
    {
      if (fill > line)
      {
        jprint_back();      // needs character erased
        fill--;
      }
      continue;
    }

    // finish line when enter key
    if (ch == '\r')   
    {
      jprintf("\n");        // needs LF added
      *fill = '\0';
      pend = -1;
      return 2;
    }

    // insert character at end of string
    *fill++ = ch;
    jprintf("%c", ch);
  }
  return 1;
}


//= Start any pending actions.

void jhcTextIO::Issue ()  
{ 
  Utter();
  tlast = tlock;
  tlock = 0;
  acc = 0;
}


//= Length of the current silence interval (in seconds).

double jhcTextIO::Silence () const
{
  if (last == 0)
    return 0.0;
  return(0.001 * (now - last));
}


///////////////////////////////////////////////////////////////////////////
//                            Output Messages                            //
///////////////////////////////////////////////////////////////////////////

//= Print string immediately.
// returns 1 for convenience

int jhcTextIO::Say (const char *msg, ...)
{
  va_list args;

  *qtext = '\0';
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(qtext, msg, args);
  }
  return Utter();
}


//= Propose typing some output message.
// returns 1 if string accepted (call Utter to cause output)

int jhcTextIO::Say (int bid, const char *msg, ...)
{
  va_list args;

  // see if previous command takes precedence
  if (bid < tlock)
    return 0;
  tlock = bid;

  // assemble full message
  *qtext = '\0';
  if (msg != NULL)
  {
    va_start(args, msg); 
    vsprintf_s(qtext, msg, args);
  }

  // only start saying if bid is higher than what is in progress
  if (tlock > tlast)
    emit = qtext;
  return 1;
}


//= Actually cause synthetic typing to start printing queued string.
// returns 1 if something started, does not await completion

int jhcTextIO::Utter ()
{
  // see if anything new to say
  if (emit != NULL)
  {
    // see if any ongoing utterance and whether it can be interrupted
    if (tlock <= tlast)
      return 0;
    ShutUp();

    // start new sentence and dequeue message
    strcpy_s(msg, 500, qtext);
    more = msg;
    return 1;
  }

  // see if time to print a new prompt
  if ((*qtext == '\0') && (pend < 0))
  {
    jprintf("\ninput-> ");
    pend = 0;
    return 0;
  }
  return 1;
}


//= Blocks when any typed utterance is in progress.
// returns when finished or given time elapses
// value indicates whether still busy (0) or silent (1)

int jhcTextIO::Finish (double secs)
{
  int i, wait = ROUND(secs / 0.1);

  // check at regular intervals whether done yet
  for (i = 0; i <= wait; i++)
  {
    dribble();
    if (more == NULL)
      return 1;
    Sleep(100);
  }
  return 0;
}


//= Stop printing right away and ignore any queued phrases.

void jhcTextIO::ShutUp ()
{
  if (more == NULL)
    return;
  jprintf(" ...\"\n\n");
  more = NULL;
}


//= Basic output routine to provide jittery text.

void jhcTextIO::dribble ()
{
  unsigned int tnow, wait;
  double delay;

  // see if there is anything left to say and whether printing is allowed
  if (more == NULL)
    return;
  if (disable > 0)
  {
    more = NULL;
    return;
  }

  // check if can be said all at once
  if (blurt > 0)
  {
    jprintf("\n==> \"%s\"\n\n", msg);
    more = NULL;
    return;
  }

  // possibly truncate output if user starts typing
  if ((barge > 0) && ((pend > 0) || (_kbhit() != 0)))
  {
    ShutUp();
    return;
  }

  // print indicator tag immediately at start of message
  if (more == msg)
  {
    jprintf("\n==> \"");
    tprev = 0;
  }

  // see if ready to output next character(s)
  while (1)
  {
    // wait longer at front of words
    if ((more[0] == ' ') || (more == msg))
      delay = jrand_cent(ims, ivar);
    else
      delay = jrand_cent(mms, mvar);
    wait = (unsigned int) delay;

    // check if waited long enough
    tnow = timeGetTime();
    if (tprev == 0)
      tprev = tnow;
    else if (tnow < (tprev + wait))
      break;
   
    // print next character 
    if (more == msg)
      jprintf("%c", toupper(more[1]));
    else
      jprintf("%c", more[1]);
    more++;
    tprev += wait;

    // possibly end line
    if ((more[0] == '\0') || (more[1] == '\0'))
    {
      jprintf("\"\n\n");
      more = NULL;
      break;
    }
  }
}
