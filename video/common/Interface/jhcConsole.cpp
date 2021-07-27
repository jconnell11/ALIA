// jhcConsole.cpp : makes a window for printf and gets within a GUI application
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2009-2019 IBM Corporation
// Copyright 2020-2021 Etaoin Systems
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

#include "stdafx.h"

#include <stdio.h>
#include <fcntl.h>
#include <io.h>

#include "Interface/jhcString.h"

#include "Interface/jhcConsole.h"


///////////////////////////////////////////////////////////////////////////
//                       Creation and Configuration                      //
///////////////////////////////////////////////////////////////////////////

//= Default constructor initializes certain values.
// can write title at top and control top left corner position

jhcConsole::jhcConsole (const char *title, int x, int y)
{
	FILE *fpstdin = stdin, *fpstdout = stdout, *fpstderr = stderr;
  HANDLE in;
  DWORD prev_mode;

  // make console window and possibly customize
  AllocConsole();
  ShowWindow(GetConsoleWindow(), SW_RESTORE);

  // possibly customize
  SetTitle(title);
  SetPosition(x, y);

  // redirect streams to the console
  freopen_s(&fpstdin, "CONIN$", "r", stdin);
	freopen_s(&fpstdout, "CONOUT$", "w", stdout);
	freopen_s(&fpstderr, "CONOUT$", "w", stderr);

  // prevent application lock-up on mouse selection in window
  in = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(in, &prev_mode); 
  SetConsoleMode(in, ENABLE_EXTENDED_FLAGS |  (prev_mode & ~ENABLE_QUICK_EDIT_MODE));
}


//= Default destructor does necessary cleanup.

jhcConsole::~jhcConsole ()
{
  fclose(stderr);
  fclose(stdin);
  fclose(stdout);
//  FreeConsole();
}


//= Change the title bar at the top of the window.

void jhcConsole::SetTitle (const char *title, int full)
{
  jhcString cvt;

  if (title == NULL)
    strcpy_s(name, "> console");
  else if (full > 0)
    strcpy_s(name, title);
  else
    sprintf_s(name, "> %s console", title);
  cvt.Set(name);
  SetConsoleTitle(cvt.Txt());
}


//= Change the upper left corner's position on the screen and possibly the size.
// change position if x or y non-negative, change size if w or h positive 
// NOTE: default width = 673, default height = 339

void jhcConsole::SetPosition (int x, int y, int w, int h)
{
  HWND c = GetConsoleWindow();
  CRect r;
  int x2 = x, y2 = y, w2 = w, h2 = h;

  GetWindowRect(c, &r);
  if (x < 0)
    x2 = r.left;
  if (y < 0)
    y2 = r.top;
  if (w <= 0)
    w2 = r.Width();
  if (h <= 0)
    h2 = r.Height();
  SetWindowPos(c, HWND_TOP, x2, y2, w2, h2, SWP_SHOWWINDOW);
}

