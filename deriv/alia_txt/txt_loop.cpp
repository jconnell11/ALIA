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

#ifndef __linux__
  #include <windows.h>
#else
  #include <signal.h>
#endif

#include <stdio.h>

#include "jhcGlobal.h"                 // common video
#include "jhc_conio.h"                 

#include "API/alia_txt.h"              // common audio


///////////////////////////////////////////////////////////////////////////

#ifndef __linux__
  void term_save () {}                 // Windows saves no info!
#else
  void clean_stop (int signum) {alia_done(0);}
  void term_save ()
  {
    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = clean_stop;
    sigaction(SIGTERM, &act, NULL);    // close log but drop KB
  }
#endif


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

    // handle simple edits via backspace (also delete for Linux)
    if (strchr("\b\x7F", ch) != NULL) 
    {
      if (fill > 0)
      {
        printf("\b \b");
        fflush(stdout);
        text[--fill] = '\0';
      }
      return 0;
    }

    // check for return or too many characters
    if (fill >= 199)
      _ungetch(ch);
    if ((strchr("\n\r\x0A", ch) != NULL) || (fill >= 199))
    {
      printf(" \n");
      text[fill] = '\0';
      fill = 0;
      return 1;
    }

    // echo keystroke and add character to end
    _putch(ch);
    fflush(stdout);
    text[fill++] = (char) ch;
    text[fill] = '\0';
  }
  return 0;
}


///////////////////////////////////////////////////////////////////////////

//= Main entry point for command line program.
// NOTE: any newly learned info is NOT saved if machine gets shutdown!

int main (int argc, char *argv[])
{
  char input[200];
  const char *output;
  int rc, n, fill = 0;

  // attempt to save newly learned info if program suddenly terminated
  term_save();
  alia_reset(NULL, "Joe", 1);

  // announce entry and prompt user for input
  printf("\nTest of DLL: %s -- jconnell@alum.mit.edu\n", alia_version());
  printf("Hit ESC to exit ...\n\n");
  printf("> ");
  fflush(stdout);

  // link reasoning agent to user
  while ((rc = get_line(input, fill)) >= 0)
  {
    // update sensor data from robot hardware
//    hw_update();

    // process any user statement
    if (rc > 0)
    {
      printf("> ");
      fflush(stdout);
    }
    output = alia_respond((rc > 0) ? input : NULL);

    // report any agent message
    if (output != NULL)
    {
      n = (int) strlen(output) - (fill + 3);
      printf("\r%s%*s\n", output, __max(0, n), "");
      printf("> %s", ((fill > 0) ? input : ""));
      fflush(stdout);
    }

    // issue command to actuators on robot hardware
//    hw_issue();

    // think some more then await next sensor cycle
    alia_daydream(1);
  }

  // cleanup
  alia_done(0);
  printf("\n\nDone -- see ALIA log file for details\n");
  return 1;
}
