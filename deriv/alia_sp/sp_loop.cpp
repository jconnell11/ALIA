// sp_loop.cpp : simple speech-based test for ALIA reasoner DLL
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

#include "API/alia_sp.h"


///////////////////////////////////////////////////////////////////////////

//= Main entry point for command line program.
// NOTE: needs sp_reco_web.dll and Microsoft.CognitiveServices.Speech.core.dll

int main (int argc, char *argv[])
{
  const char *input, *output;

  // announce entry and prompt user for input
  printf("\nTest of DLL: %s -- jconnell@alum.mit.edu\n", alia_version());
  printf("Hit any key to exit ...\n\n");
  alia_ioctrl(2, 0, 1);
  if (alia_reset("Nancy", "Zira", 1) <= 0)
  {
    printf("Failed to initialize -- check sp_reco_web.key file?\n");
    return 0;
  }

  // link reasoning agent to user
  while (!_kbhit())
  {
    // update sensor data from robot hardware
//    hw_update();

    // process any user statement (spoken or text)
    output = alia_respond(NULL, 0);
    if ((input = alia_input()) != NULL)
      printf("> %s\n", input);
    if (output != NULL)
      printf("%s\n", output);

    // issue command to actuators on robot hardware
//    hw_issue();

    // think some more then await next sensor cycle
    alia_daydream(1);
  }

  // cleanup
  printf("\nExiting (please wait) ...\n");
  alia_done(0);
  printf("Done -- see log file for details\n");
  return 1;
}
