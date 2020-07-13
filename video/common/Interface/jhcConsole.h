// jhcConsole.h : makes a window for printf and gets within a GUI application
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2009-2019 IBM Corporation
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

#ifndef _JHCCONSOLE_
/* CPPDOC_BEGIN_EXCLUDE */
#define _JHCCONSOLE_
/* CPPDOC_END_EXCLUDE */

#include "jhcGlobal.h"

#include <stdlib.h>           // needed for NULL


//= Makes a window for printf and gets within a GUI application.

class jhcConsole
{
// PRIVATE MEMBER VARIABLES
private:
  char name[200];


// PUBLIC MEMBER FUNCTIONS
public:
  // creation and configuration
  jhcConsole (const char *title =NULL, int x =-1, int y =-1);
  ~jhcConsole ();

  void SetTitle (const char *title, int full =0);
  void SetPosition (int x, int y, int w =0, int h =0);

};


#endif  // once




