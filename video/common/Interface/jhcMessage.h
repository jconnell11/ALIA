// jhcMessage.h : stubs for standard communication 
//
// Written by Jonathan H. Connell, jconnell@alum.mit.edu
//
///////////////////////////////////////////////////////////////////////////
//
// Copyright 1998-2014 IBM Corporation
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

#include "jhcGlobal.h"


// global routines for dialog boxes and errors

int Fatal (const char *msg =NULL, ...);
int Complain (const char *msg =NULL, ...);
int Tell (const char *msg =NULL, ...);
int Ask (const char *msg =NULL, ...);
int AskNot (const char *msg =NULL, ...);
int AskStop (const char *msg =NULL, ...);
int Pause (const char *msg =NULL, ...);
int Ignore (const char *msg =NULL, ...);

