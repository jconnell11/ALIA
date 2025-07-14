// main0.cpp : does something
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 IBM Corporation
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

#include <windows.h>                   // needed for Sleep

#include <stdio.h>
#include <conio.h>

#include "Acoustic/sp_reco_web.h"


///////////////////////////////////////////////////////////////////////////

//= Main entry point for command line program.

int main (int argc, char *argv[])
{
  char utt[200];
  int rc, reco = 1, quit = -1;

  // announce program
  printf("\n");
  printf("web_test version %4.2f -- jconnell@alum.mit.edu\n", 1.35);
  printf("Exercises Microsoft Azure Speech Recognition DLL.\n\n");

  // setup speech
  if ((rc = reco_setup()) <= 0)
    return printf("reco_setup = %d !\n", rc);
  printf("  %d names read from file\n", rc - 1);
  printf("  unknown: \"Sridhar\"\n\n");
  if ((rc = reco_start()) <= 0)
    return printf("reco_start = %d !\n", rc);

  // main loop
  printf("Connected - toggle listening with space bar\n");
  printf("Hit ESC or say \"goodbye\" to exit ...\n");
  while (quit <= 0)
  {
    // interact with speech engine
    Sleep(33);
    if (reco_status() == 2)
    {
      strcpy_s(utt, reco_heard());
      printf("--> %s\n", utt);
      if (_strnicmp(utt, "goodbye", 7) == 0)
        quit = 1;        
      else if (quit < 0)
      {
        // add a new name after first recognition
        reco_name("Sridhar");
        printf("  added: \"Sridhar\"\n");
        quit = 0;
      }
    }

    // use keyboard to mute and unmute
    if (_kbhit())
    {
      if (_getch() == '\x1B')
        quit = 1;
      else if (reco > 0)
      {
        reco = 0;
        reco_listen(reco);
        printf("    -muted\n");
      }
      else
      {
        reco = 1;
        reco_listen(reco);
        printf("    +listening\n");
      }
    }
  }

  // cleanup
  reco_cleanup();
  printf("\nDone. Hit any key to exit ...\n");
  _getch();
  return 1;
}
