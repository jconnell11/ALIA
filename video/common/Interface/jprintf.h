// jprintf.h : replacement printf function which also saves output
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 2012-2019 IBM Corporation
// Copyright 2020-2024 Etaoin Systems
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

#include "jhcGlobal.h"

#include <stdio.h>      // needed for the definition of FILE and NULL


int jprintf_open (const char *log, ...);
const char *jprintf_log (int only =0);
void jprintf_sync ();
int jprintf_close ();
int jprintf_end (const char *msg =NULL, ...);

int jprintf (const char *msg, ...);
int jprintf (int th, int lvl, const char *msg, ...);
int jprint (const char *txt);
int jprint_back ();

int jfprintf (FILE *out, const char *msg, ...);
int jfputs (const char *msg, FILE *out);

