// jhc_conio.cpp - Linux versions of low-level keyboard functions
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

// NOTE: this file should NOT be in the project for Windows builds!

#include <stdio.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <stropts.h>
#include <unistd.h>

#include "jhc_conio.h"
 

//= Returns a single keyboard character (blocks).

int _getch ()
{
  termios old, now;
  int ch;

  // turn off any buffering and echo
  tcgetattr(0, &old);
  now = old;
  now.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &now);
  
  // get a character then restore settings
  ch = getchar();
  tcsetattr(0, TCSANOW, &old);
  return ch;
}


//= Returns non-zero if keyboard has been used (does not block).

int _kbhit () 
{
  termios att;
  int n;

  // turn off any buffering and echo
  tcgetattr(0, &att);
  att.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(0, TCSANOW, &att);

  // probe state of keyboard buffer 
  setbuf(stdin, NULL);
  ioctl(0, FIONREAD, &n);

  // revert if key registered
  if (n > 0)
    _kbdone();
  return n;
}


//= Make sure stdin functions properly after done with _kbhit.
// NOTE: forces buffering and echo (but might not have been enabled!) 

void _kbdone ()
{
  termios att;

  tcgetattr(0, &att);
  att.c_lflag |= (ICANON | ECHO);
  tcsetattr(0, TCSANOW, &att);
}


