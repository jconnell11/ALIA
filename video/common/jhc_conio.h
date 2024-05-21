// jhc_conio.h - Linux versions of low-level keyboard functions
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

#pragma once

#ifndef __linux__
  #include <conio.h>                   // already has _kbhit and _getch
  #define _kbdone()                    // non-standard
#else

// NOTE: must also add "jhc_conio.cpp" to project for Linux GCC

#include <stdio.h>

inline void _putch (int c) {putchar(c);}
inline void _ungetch (int c) {ungetc(c, stdin);}

// defined in jhc_conio.cpp file
int _getch ();
int _kbhit ();
void _kbdone ();                       // non-standard

#endif