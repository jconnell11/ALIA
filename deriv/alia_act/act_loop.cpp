// act_loop.cpp : simple text-based test for ALIA reasoner DLL
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

#include "Interface/jms_x.h"           // common video

#include "API/alia_act.h"              // common audio


//= Main entry point for command line program.

int main (int argc, char *argv[])
{
  alia_reset(NULL, "Zeke", "act_loop");
  while (alia_think() > 0)
    jms_sleep(30);
  alia_done(0);
  return 0;
}
