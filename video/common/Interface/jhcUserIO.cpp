// jhcUserIO.cpp : simple interactive text console 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023-2024 Etaoin Systems
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

#include "jhc_conio.h" 

#include "Interface/jhcUserIO.h"


///////////////////////////////////////////////////////////////////////////
//                      Creation and Initialization                      //
///////////////////////////////////////////////////////////////////////////

//= Default destructor does necessary cleanup.

jhcUserIO::~jhcUserIO ()
{
  Stop();
}


//= Default constructor initializes certain values.

jhcUserIO::jhcUserIO ()
{
  *prior = '\0';
  *input = '\0';
  quit = 0;
  fill = 0;
  seq = -1;                  // not started
}


///////////////////////////////////////////////////////////////////////////
//                              Main Functions                           //
///////////////////////////////////////////////////////////////////////////

//= Take over all console interaction.

void jhcUserIO::Start ()
{
  *prior = '\0';
  quit = 0;
  fill = 0;
  seq = 0;
  printf("> ");                
  fflush(stdout);
}


//= See if any typed input has been completed by user.
// returns NULL if nothing ready (must echo explicitly with Post)
  
const char *jhcUserIO::Get ()
{
  int ch;

  // make sure interaction started
  if (seq < 0)
    Start();

  // process accumulated keystrokes
  while (_kbhit() > 0)
  {
    // get typed character and handle special cases
    ch = _getch();
    if ((seq = multikey(ch)) > 2)
      continue;
    if (special(ch))
      continue;

    // check for return or too many characters
    if (fill >= 199)
      _ungetch(ch);
    if ((strchr("\n\r\x0A", ch) != NULL) || (fill >= 199))
    {
      // erase line on console (explicitly echo fixed version)
      printf("\r> %*s\r> ", fill, "");
      fflush(stdout);

      // finish off line, save it, and return it
      input[fill] = '\0';
      strcpy_s(prior, input);
      fill = 0;
      return input;
    }

    // echo keystroke and add character to end
    if ((ch >= 0x20) && (ch <= 0x7E))
    {
      _putch(ch);
      fflush(stdout);
      input[fill++] = (char) ch;
      input[fill] = '\0';
    }
  }
  return NULL;
}
  

//= Check for progression of multi-key sequences.
// patterns: 1B:1B (quit), 1B:5B:41 (up arrow), 1B:5B:33:7E (delete), E0:48 (up arrow), E0:53 (delete)
//      seq:  6  7          6  5 -1-             6  5  4 -2-           3 -1-             3 -2-
// returns 0 if nothing, 1 if up arrow found, 2 if delete found, higher = in-progress

int jhcUserIO::multikey (int ch)
{
  // Linux sequences (begin with ESC)
  if (ch == 0x1B)                    
  {
    if (seq != 6)
    {
      printf("\x1B]12;red\x07\x1B[1 q");   // cursor = red blinking block
      fflush(stdout);
      return 6;
    }
    quit = 1;
    return 7;                              // ESC ESC pattern not printable
  }                                       
  if (seq == 6)
  {
    printf("\x1B]12;gray\x07\x1B[0 q");    // cursor = gray default
    fflush(stdout);
    return((ch == '[') ? 5 : 0);           // keep char after single ESC
  }
  if (seq == 5)
    return((ch == 'A') ? 1 : ((ch == '3') ? 4 : 7));
  if (seq == 4) 
    return((ch == '~') ? 2 : 7);       
  
  // Windows sequences (begin with E0)
  if (ch == 0xE0)                 
    return 3;                                     
  if (seq == 3)
    return((ch == 'H') ? 1 : ((ch == 'S') ? 2 : 7));
  return 0;                            
}


//= Handle non-character keys like backspace.
// seq: 1 = up arrow, 2 = delete

bool jhcUserIO::special (int ch)
{
  // retrieve previous typed line (^P) 
  if ((ch == 0x10) || (seq == 1))
  {
    if (*prior != '\0')
    {
      printf("\r> %*s\r> %s", fill, "", prior);
      fflush(stdout);
      strcpy_s(input, prior);  
      fill = (int) strlen(input);
    }
    return true;
  }
                   
  // erase whole current line (delete)
  if (seq == 2)
  {
    if (fill > 0)
    {
      printf("\r> %*s\r> ", fill, "");
      fflush(stdout);
      fill = 0;
    } 
    return true;
  }

  // remove last typed char (backspace)
  if ((ch == '\b') || (ch == 0x7F))                    
  {
    if (fill > 0)
    {
      printf("\b \b");
      fflush(stdout);
      input[--fill] = '\0';
    }   
    return true;
  }
  return false;                        
}


//= Print a message on the console moving any partial input down.
// if user > 0 precedes text with "> " (for echo or speech)

void jhcUserIO::Post (const char *msg, int user)
{ 
  int n, clr;

  // make sure interaction started
  if (seq < 0)
    Start();

  // sanity check
  if (msg == NULL) 
    return;
  if ((n = (int) strlen(msg)) <= 0)
    return;
      
  // remove any typing from line and replace with message 
  clr = (fill + 2) - n;
  if (user > 0)
    printf("\r> %s%*s\n", msg, __max(0, clr - 2), "");
  else
    printf("\r%s%*s\n", msg, __max(0, clr), "");

  // restore partial input (if any)
  printf("> %s", ((fill > 0) ? input : ""));
  fflush(stdout);
}


//= Stop hogging console I/O.

void jhcUserIO::Stop ()
{
  if (seq < 0)
    return;
  printf("\x1B]12;gray\x07\x1B[0 q");    // cursor = gray default
  fflush(stdout);
  _kbdone();
  seq = -1;
}