// txt_loop.cpp : simple text-based test for ALIA reasoner DLL
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2023 Etaoin Systems
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

#include <windows.h>
#include <conio.h>
#include <stdio.h>

#include "API/alia_txt.h"


///////////////////////////////////////////////////////////////////////////

//= Non-blocking gets() for retrieving a full line from user.
// also handles limited editing using backspace and ESC for exit
// builds up input in "text" by putting new character at "fill"
// hardcoded for a "text" string length of 200
// returns 1 if string ready, 0 if waiting, -1 for exit

int get_line (char *text, int& fill)
{
  int ch;

  // process accumulated keystrokes
  while (_kbhit() > 0)
  {
    // signal exit if ESC pressed
    if ((ch = _getch()) == 0x1B)
      return -1;

    // handle simple edits via backspace
    if (ch == '\b') 
    {
      if (fill > 0)
      {
        printf("\b \b");
        text[--fill] = '\0';
      }
      return 0;
    }

    // check for return or too many characters
    if (fill >= 199)
      _ungetch(ch);
    if ((ch == '\r') || (fill >= 199))
    {
      printf(" \n");
      text[fill] = '\0';
      fill = 0;
      return 1;
    }

    // echo keystroke and add character to end
    _putch(ch);
    text[fill++] = ch;
    text[fill] = '\0';
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////

//= Main entry point for command line program.

int main (int argc, char *argv[])
{
  char input[200];
  const char *output;
  int rc, n, fill = 0;

  // announce entry and prompt user for input
  printf("\nTest of DLL: %s -- jconnell@alum.mit.edu\n", alia_version());
  printf("Hit ESC to exit ...\n\n");
  printf("> ");
  alia_reset("Joe", 1);

  // link reasoning agent to user
  while ((rc = get_line(input, fill)) >= 0)
  {
    // update sensor data from robot hardware
//    hw_update();

    // process any user statement
    if (rc > 0)
      printf("> ");
    output = alia_respond((rc > 0) ? input : NULL);

    // report any agent message
    if (output != NULL)
    {
      n = (int) strlen(output) - (fill + 3);
      printf("\r%s%*s\n", output, __max(0, n), "");
      printf("> %s", ((fill > 0) ? input : ""));
    }

    // issue command to actuators on robot hardware
//    hw_issue();

    // think some more then await next sensor cycle
    alia_daydream(1);
  }

  // cleanup
  alia_done(0);
  printf("\n\nDone -- see log file for details\n");
  return 1;
}
